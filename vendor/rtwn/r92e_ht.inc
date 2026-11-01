/* RTL8192EU generation-2 firmware rate control. The 0x40 payload, RAID
 * indices and AP-peer media status follow pinned Linux rtl8xxxu. */
int
rtwn_r92e_ra_init(struct rtwn_softc *sc, uint32_t rates, int maxbasicrate)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_node *ni = ic->ic_bss;
	rtw_92e_rate_config config;
	uint8_t cmd[RTW_92E_MACID_CONFIG_BYTES];
	uint32_t reg;
	int error;

	memset(&sc->rate_config, 0, sizeof(sc->rate_config));
	if (!ni || !rtw_92e_build_rate_config_width((uint16_t)rates,
	    ni->ni_rxmcs, sc->ntxchains,
	    (ni->ni_flags & IEEE80211_NODE_HT) != 0,
	    (ic->ic_htcaps & IEEE80211_HTCAP_SGI20) != 0,
	    (ni->ni_flags & IEEE80211_NODE_HT_SGI20) != 0,
    sc->channel_bw40,
    (ic->ic_htcaps & IEEE80211_HTCAP_SGI40) != 0,
    (ni->ni_flags & IEEE80211_NODE_HT_SGI40) != 0, &config) ||
	    !rtw_92e_make_macid_config(&config, R92C_MACID_BSS, cmd))
		return EINVAL;
	/* Hardware response rates remain legacy; never write MCS into RRSR. */
	rtwn_write_1(sc, R92C_INIRTS_RATE_SEL, maxbasicrate);
	reg = rtwn_read_4(sc, R92C_RRSR);
	reg = RW(reg, R92C_RRSR_RATE_BITMAP, rates & 0x0fffU);
	rtwn_write_4(sc, R92C_RRSR, reg);
	if ((error = rtwn_io_error(sc)) != 0)
		return error;
	error = rtwn_fw_cmd(sc, R92E_CMD_MACID_CONFIG, cmd, sizeof(cmd));
	if (error != 0)
		return error;
	sc->rate_config = config;
	return 0;
}

int
rtwn_r92e_media_status(struct rtwn_softc *sc, int connect)
{
	uint8_t cmd[RTW_92E_MEDIA_STATUS_BYTES];
	int error;

	if (!rtw_92e_make_media_status(connect, R92C_MACID_BSS, cmd))
		return EINVAL;
	if (!connect) {
		memset(&sc->rate_config, 0, sizeof(sc->rate_config));
		if (!sc->fw_station_connected)
			return 0;
		sc->fw_station_connected = 0;
	}
	error = rtwn_fw_cmd(sc, R92E_CMD_MEDIA_STATUS, cmd, sizeof(cmd));
	if (!error && connect)
		sc->fw_station_connected = 1;
	else if (error && connect)
		memset(&sc->rate_config, 0, sizeof(sc->rate_config));
	return error;
}

int
rtwn_r92e_ampdu_params(struct rtwn_softc *sc, uint8_t factor, uint8_t density)
{
	/* Linux rtl8xxxu_set_ampdu_factor/min_space: peer receive limits
	 * constrain our transmitter. RX reordering remains in net80211. */
	static const uint8_t limits[4] = { 0x41, 0xa8, 0x72, 0xb9 };
	uint8_t max_agg, value;
	unsigned i;

	if (!sc || !(sc->chip & RTWN_CHIP_92E) || factor > 3 || density > 7)
		return EINVAL;
	max_agg = (uint8_t)(1U << (factor + 2));
	if (max_agg > 0x0f)
		max_agg = 0x0f;
	for (i = 0; i < nitems(limits); i++) {
		value = limits[i];
		if ((value & 0xf0) > (max_agg << 4))
			value = (uint8_t)((value & 0x0f) | (max_agg << 4));
		if ((value & 0x0f) > max_agg)
			value = (uint8_t)((value & 0xf0) | max_agg);
		rtwn_write_1(sc, R92C_AGGLEN_LMT + i, value);
	}
	value = rtwn_read_1(sc, R92C_AMPDU_MIN_SPACE);
	rtwn_write_1(sc, R92C_AMPDU_MIN_SPACE, (value & 0xf8) | density);
	return rtwn_io_error(sc);
}
