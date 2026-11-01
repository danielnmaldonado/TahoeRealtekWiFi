/* Published vectors exercise the real imported implementation. Platform-only
 * mbuf/allocation substitutes are in tests/net80211_host, never in the kext. */
#include "net80211_host/host_platform.hpp"
#include <net80211/ieee80211_var.h>
#include <crypto/sha1.h>
#include <crypto/aes.h>
#include <crypto/key_wrap.h>

static unsigned checks;
static void require(bool condition, const char *description) {
    ++checks;
    if (!condition) { fprintf(stderr, "FAIL: %s\n", description); exit(1); }
}
static mbuf_t packet(const uint8_t *bytes, size_t length) {
    mbuf_t m = nullptr;
    require(mbuf_get(0, 0, &m) == 0, "allocate host mbuf");
    memcpy(m->bytes, bytes, length);
    mbuf_setlen(m, int(length));
    mbuf_pkthdr_setlen(m, int(length));
    return m;
}
static void expect_hex(const uint8_t *actual, size_t length, const char *hex,
                       const char *description) {
    require(strlen(hex) == length * 2, "expected vector length");
    for (size_t i = 0; i < length; ++i) {
        unsigned expected;
        require(sscanf(hex + i * 2, "%2x", &expected) == 1, "decode expected vector");
        require(actual[i] == expected, description);
    }
}

static void pbkdf2_vectors() {
    /* RFC 6070 section 2; no embedded-NUL password (API uses a C string). */
    const struct { int rounds; const char *expected; } vectors[] = {
        {1, "0c60c80f961f0e71f3a9b524af6012062fe037a6"},
        {2, "ea6c014dc72d6f8ccd1ed92ace1d41f0d8de8957"},
        {4096, "4b007901b765489abead49d926f721d065a429c1"},
    };
    uint8_t derived[32];
    for (const auto &vector : vectors) {
        require(pbkdf2_sha1("password", (const u8 *)"salt", 4, vector.rounds,
                            derived, 20) == 0, "PBKDF2 status");
        expect_hex(derived, 20, vector.expected, "RFC 6070 PBKDF2-SHA1");
    }
    require(pbkdf2_sha1("password", (const u8 *)"IEEE", 4, 4096,
                        derived, sizeof(derived)) == 0, "WPA2 PMK derivation");
    expect_hex(derived, sizeof(derived),
               "f42c6fc52df0ebef9ebb4b90b38a5f902e83fe1b135a70e23aed762e9710a12e",
               "IEEE 802.11i password/IEEE PMK");
}

static void key_wrap_vectors() {
    /* RFC 3394 section 4.1: 128-bit key data, 128-bit KEK. */
    const uint8_t kek[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    const uint8_t plain[] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,
                             0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff};
    uint8_t wrapped[24], unwrapped[16];
    aes_key_wrap_ctx context;
    aes_key_wrap_set_key(&context, kek, sizeof(kek));
    aes_key_wrap(&context, plain, 2, wrapped);
    expect_hex(wrapped, sizeof(wrapped),
               "1fa68b0a8112b447aef34bd8fb5a7b829d3e862371d2cfe5", "RFC 3394 wrap");
    require(aes_key_unwrap(&context, wrapped, unwrapped, 2) == 0,
            "RFC 3394 unwrap status");
    require(memcmp(plain, unwrapped, sizeof(plain)) == 0, "RFC 3394 unwrap bytes");
    wrapped[8] ^= 1;
    require(aes_key_unwrap(&context, wrapped, unwrapped, 2) != 0,
            "key unwrap rejects modified ciphertext");
}

static void ccmp_vectors() {
    /* IEEE Std 802.11-2012 M.6.4, also wlantest/test_vectors.c by Jouni Malinen.
     * Expected ciphertext independently checked with Node/OpenSSL AES-128-CCM.
     * PN is b5:03:97:76:e7:0c; CCMP stores PN least significant byte first. */
    const uint8_t key[] = {0xc9,0x7c,0x1f,0x67,0xce,0x37,0x11,0x85,
                           0x51,0x4a,0x8a,0x19,0xf2,0xbd,0xd5,0x2f};
    const uint8_t frame[] = {
        0x08,0x48,0xc3,0x2c,0x0f,0xd2,0xe1,0x28,0xa5,0x7c,0x50,0x30,
        0xf1,0x84,0x44,0x08,0xab,0xae,0xa5,0xb8,0xfc,0xba,0x80,0x33,
        0xf8,0xba,0x1a,0x55,0xd0,0x2f,0x85,0xae,0x96,0x7b,0xb6,0x2f,
        0xb6,0xcd,0xa8,0xeb,0x7e,0x78,0xa0,0x50,
    };
    ieee80211com controller = {};
    ieee80211_key state = {};
    memcpy(state.k_key, key, sizeof(key));
    state.k_tsc = 0xb5039776e70bULL;
    require(ieee80211_ccmp_set_key(&controller, &state) == 0, "CCMP key setup");
    mbuf_t encrypted = ieee80211_ccmp_encrypt(&controller, packet(frame, sizeof(frame)), &state);
    require(encrypted != nullptr, "CCMP encrypt status");
    require(mbuf_pkthdr_len(encrypted) == sizeof(frame) + 16, "CCMP packet length");
    expect_hex(encrypted->bytes + 24, 8, "0ce70020769703b5", "CCMP PN header");
    expect_hex(encrypted->bytes + 32, 28,
               "f3d0a2fe9a3dbf2342a643e43246e80c3c04d0197845ce0b16f97623",
               "IEEE M.6.4 CCMP ciphertext and MIC");
    const size_t encrypted_size = size_t(mbuf_pkthdr_len(encrypted));
    uint8_t saved[128];
    memcpy(saved, encrypted->bytes, encrypted_size);
    mbuf_t decrypted = ieee80211_ccmp_decrypt(&controller, encrypted, &state);
    require(decrypted != nullptr, "CCMP decrypt status");
    require(mbuf_pkthdr_len(decrypted) == sizeof(frame), "CCMP decrypted length");
    uint8_t expected_frame[sizeof(frame)];
    memcpy(expected_frame, frame, sizeof(frame));
    expected_frame[1] &= ~IEEE80211_FC1_PROTECTED;
    require(memcmp(decrypted->bytes, expected_frame, sizeof(frame)) == 0,
            "CCMP decrypted frame clears protected bit");
    mbuf_freem(decrypted);
    require(state.k_rsc[0] == 0xb5039776e70cULL, "CCMP replay counter commits after MIC");
    require(ieee80211_ccmp_decrypt(&controller, packet(saved, encrypted_size), &state) == nullptr,
            "CCMP rejects replay");
    require(controller.ic_stats.is_ccmp_replays == 1, "CCMP replay statistic");
    state.k_rsc[0] = 0;
    saved[encrypted_size - 1] ^= 1;
    require(ieee80211_ccmp_decrypt(&controller, packet(saved, encrypted_size), &state) == nullptr,
            "CCMP rejects invalid MIC");
    require(state.k_rsc[0] == 0, "invalid MIC leaves replay counter unchanged");
    require(controller.ic_stats.is_ccmp_dec_errs == 1, "CCMP MIC statistic");
    require(ieee80211_ccmp_decrypt(&controller, packet(saved, 24), &state) == nullptr,
            "CCMP rejects truncated packet");
    ieee80211_ccmp_delete_key(&controller, &state);
    require(state.k_priv == nullptr, "CCMP key state released");
}

int main() {
    pbkdf2_vectors();
    key_wrap_vectors();
    ccmp_vectors();
    printf("PASS imported PBKDF2, AES key wrap, CCMP vectors and rejection checks (%u assertions)\n", checks);
}
