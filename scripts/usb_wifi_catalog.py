#!/usr/bin/env python3
"""Strict reader for the pinned functional USB matching metadata."""
from pathlib import Path
import hashlib
import json
import re
from functools import lru_cache
ROOT = Path(__file__).resolve().parents[1]
DEFAULT_DATA = ROOT / 'data/legacy-usb-adapters.json'
FAMILIES = (
    'rtl8192cu','rtl8188eu','rtl8192eu','rtl8192du','rtl8192fu','rtl8710bu',
    'rtl8723bu','rtl8812au','rtl8814au','rtl8814bu','rtl8821au','rtl8821cu',
    'rtl8822bu','rtl8822cu',
)
# These are the assembled station implementations. This is deliberately separate
# from the inventory's list of recognized families and their support claims.
READY_FAMILIES = frozenset({'rtl8192eu', 'rtl8192cu', 'rtl8188eu', 'rtl8814au',
    'rtl8192du', 'rtl8192fu', 'rtl8710bu', 'rtl8723bu', 'rtl8812au', 'rtl8821au', 'rtl8821cu', 'rtl8822bu', 'rtl8822cu'})
PRIMARY_DRIVER_FAMILIES = {
    'RTWN_CHIP_92C | RTWN_CHIP_88C':'rtl8192cu',
    'RTWN_CHIP_92E':'rtl8192eu', 'RTWN_CHIP_88E':'rtl8188eu',
    'RTL8192C_USB_IDS':'rtl8192cu', 'rtl92cu_hal_cfg':'rtl8192cu',
    'rtl8192cu_fops':'rtl8192cu', 'rtl8188eu_fops':'rtl8188eu',
    'rtl8192eu_fops':'rtl8192eu', 'rtl92du_hal_cfg':'rtl8192du',
    'rtl8192fu_fops':'rtl8192fu', 'rtl8710bu_fops':'rtl8710bu',
    'rtl8723bu_fops':'rtl8723bu', 'rtw8812a_hw_spec':'rtl8812au',
    'rtw8814a_hw_spec':'rtl8814au', 'rtw8821a_hw_spec':'rtl8821au',
    'rtw8821c_hw_spec':'rtl8821cu', 'rtw8822b_hw_spec':'rtl8822bu',
    'rtw8822c_hw_spec':'rtl8822cu', 'RTL8814B':'rtl8814bu', 'RTL8821':'rtl8821au',
}

# This registry contains only reviewed numeric bindings and small INF excerpts,
# not redistributable driver binaries. Its whole-file pin prevents a changed
# source record from authorizing a new pair without an explicit reviewed update.
OEM_PROOF_REGISTRY_SHA256 = '519e4710bf71742736fe4112d0d92bf53e6abfc403ba82a7255d26bd8774c0a0'
OEM_NDI_FAMILIES = {
    'RTL8188eu.ndi':'rtl8188eu', 'RTL8192cu.ndi':'rtl8192cu', 'RTL8192eu.ndi':'rtl8192eu',
    'RTL8723bu.ndi':'rtl8723bu', 'RTL8811au.ndi':'rtl8821au',
    'LC_RTL8821au.ndi':'rtl8821au', 'Dual_Actiontec8811A.ndi':'rtl8821au', '5GOnly_RTL8811au.ndi':'rtl8821au',
    'RTL8811cu.ndi':'rtl8821cu', 'RTL8812au.ndi':'rtl8812au',
    'RTL8814au.ndi':'rtl8814au', '3T3R_RTL8814au.ndi':'rtl8814au',
}

@lru_cache(maxsize=1)
def oem_proof_registry():
    raw = (ROOT/'data/primary-oem-usb-identities.json').read_bytes()
    if hashlib.sha256(raw).hexdigest() != OEM_PROOF_REGISTRY_SHA256:
        raise ValueError('pinned OEM USB identity proof registry changed')
    return json.loads(raw)

def oem_evidence_family(row, evidence):
    if evidence.get('kind') != 'oem_inf': return None
    registry = oem_proof_registry()
    binding = next((b for b in registry['bindings'] if b['vid_pid'] == row['vid_pid']), None)
    if binding is None: return None
    source = registry['sources'][binding['source_id']]
    expected = {**source, **binding, 'kind':'oem_inf'}
    if any(evidence.get(key) != value for key, value in expected.items()): return None
    hardware = re.fullmatch(r'USB\\VID_([0-9A-F]{4})&PID_([0-9A-F]{4})(?:&[A-Z0-9_]+)*', binding['hardware_id'])
    if not hardware or ':'.join(hardware.groups()).lower() != row['vid_pid']: return None
    if (row.get('idVendor'), row.get('idProduct')) != tuple(int(v,16) for v in hardware.groups()): return None
    family = OEM_NDI_FAMILIES.get(binding['install_section'])
    if family != binding['family']: return None
    mask = row.get('station_band_restriction', 0)
    required = 2 if binding['install_section'].startswith('5GOnly_') else 0
    if type(mask) is not int or mask != required: return None
    matching = dict(row.get('matching') if row.get('origin') == 'upstream_extension' else row.get('original_matching', {}))
    matching.update(row.get('effective_matching', {}))
    for field, proof_key in (('bInterfaceNumber','bInterfaceNumber_constraint'), ('bcdDevice','bcdDevice_constraint')):
        if binding[proof_key] is not None and matching.get(field) != binding[proof_key]: return None
    return family

def evidence_family(row, evidence):
    if evidence.get('kind') == 'oem_inf': return oem_evidence_family(row, evidence)
    if evidence.get('kind') is not None: return None
    return PRIMARY_DRIVER_FAMILIES.get(evidence.get('driver_info'))

def primary_verified(row):
    family = row['candidate_backend_family_from_personality']
    return (row['family_mapping_status'] == 'exact_numeric_id_correspondence'
            and row['primary_usb_table_families'] == [family]
            and bool(row['primary_usb_table_evidence'])
            and {evidence_family(row, evidence)
                 for evidence in row['primary_usb_table_evidence']} == {family})

def discovery_only(row):
    """Separate runtime silicon discovery from static station authorization.

    Both pinned Linux USB tables restrict13b1:0043 toFF/FF/FF. The Controller
    checks its immutable conflict row before one checked chip-ID read, and
    the Boot constructor independently consumes the resulting proof.
    """
    m = matching_constraints(row)
    return (row.get('vid_pid') == '13b1:0043' and
            row.get('family_mapping_status') == 'conflicting_primary_usb_tables' and
            set(row.get('primary_usb_table_families', [])) == {'rtl8822bu', 'rtl8822cu'} and
            (m.get('idVendor'), m.get('idProduct'), m.get('bConfigurationValue'),
             m.get('bInterfaceNumber')) == (0x13b1, 0x0043, 1, 0))

def ready(row):
    return primary_verified(row) and row['candidate_backend_family_from_personality'] in READY_FAMILIES

# Pin identities, not support claims: new aliases must not erase or rewrite the
# original inventory. Family evidence can still be corrected independently.
LEGACY_IDENTITY_SHA256 = "f5e5eeb1640704c9b3a80be57d4f472d430bf803d5ec1c71522fbe0192d716fe"
INTERFACE_FIELDS = ('bInterfaceClass', 'bInterfaceSubClass', 'bInterfaceProtocol')

def matching_constraints(row):
    """Return the active match without pretending extensions came from a plist."""
    original = row.get('matching') if row.get('origin') == 'upstream_extension' else row.get('original_matching')
    if not isinstance(original, dict):
        raise ValueError('missing USB matching dictionary')
    result = dict(original)
    extra = row.get('interface_matching', {})
    if not isinstance(extra, dict) or set(extra) - set(INTERFACE_FIELDS):
        raise ValueError('unknown interface matching constraint')
    result.update(extra)
    effective = row.get('effective_matching', {})
    if not isinstance(effective, dict) or set(effective) - {'bInterfaceNumber', 'bcdDevice'}:
        raise ValueError('unknown effective USB matching constraint')
    for field, value in effective.items():
        proof_key = 'bInterfaceNumber_constraint' if field == 'bInterfaceNumber' else 'bcdDevice_constraint'
        proofs = [e for e in row.get('primary_usb_table_evidence', []) if oem_evidence_family(row, e)]
        if type(value) is not int or not proofs or any(e.get(proof_key) != value for e in proofs):
            raise ValueError('effective USB match requires exact primary OEM constraint')
        result[field] = value
    return result

def load_catalog(path=DEFAULT_DATA):
    data = json.loads(Path(path).read_text(encoding='utf-8'))
    if data.get('schema_version') != 2:
        raise ValueError('unsupported USB catalog schema')
    rows = data['devices']
    if not isinstance(rows, list) or not 250 <= len(rows) <= 4096:
        raise ValueError('catalog must retain 250 legacy profiles and stay within 4096 entries')
    legacy = [row for row in rows if row.get('origin', 'legacy') == 'legacy']
    if len(legacy) != 250:
        raise ValueError('pinned inventory must retain all 250 legacy personalities')
    identity = sorted([{'vid_pid': row['vid_pid'], 'original_matching': row['original_matching']}
                       for row in legacy], key=lambda row: row['vid_pid'])
    digest = hashlib.sha256(json.dumps(identity, sort_keys=True, separators=(',', ':')).encode('utf-8')).hexdigest()
    if digest != LEGACY_IDENTITY_SHA256:
        raise ValueError('legacy USB matching identities changed')
    seen = set()
    for row in rows:
        band = row.get('station_band_restriction', 0)
        if type(band) is not int or band not in (0,1,2,3):
            raise ValueError('invalid per-profile station band restriction')
        if band and not any(oem_evidence_family(row, e) for e in row.get('primary_usb_table_evidence', [])):
            raise ValueError('station band restriction requires exact primary OEM proof')
        if row.get('origin', 'legacy') not in ('legacy', 'upstream_extension'):
            raise ValueError('unknown catalog origin')
        m = matching_constraints(row)
        for field in ('idVendor', 'idProduct'):
            if type(row.get(field)) is not int or row[field] != m.get(field):
                raise ValueError('row and matching USB identities disagree')
        key = (m['idVendor'], m['idProduct'])
        if key in seen:
            raise ValueError('duplicate VID:PID requires explicit conflict resolution')
        seen.add(key)
        if row['vid_pid'] != f'{key[0]:04x}:{key[1]:04x}':
            raise ValueError('numeric and textual USB identities disagree')
        for field in ('idVendor', 'idProduct', 'bcdDevice'):
            if field in m and (type(m[field]) is not int or not 0 <= m[field] <= 65535):
                raise ValueError(f'invalid 16-bit matching constraint: {field}')
        for field in ('bConfigurationValue', 'bInterfaceNumber'):
            if type(m.get(field)) is not int or not 0 <= m[field] <= 255:
                raise ValueError(f'invalid 8-bit matching constraint: {field}')
        if m['bConfigurationValue'] == 0:
            raise ValueError('configuration zero is not an active USB configuration')
        for field in INTERFACE_FIELDS:
            if field in m and (type(m[field]) is not int or not 0 <= m[field] <= 255):
                raise ValueError(f'invalid interface descriptor constraint: {field}')
        if ('bInterfaceSubClass' in m or 'bInterfaceProtocol' in m) and 'bInterfaceClass' not in m:
            raise ValueError('subclass/protocol constraints require an interface class')
        if row['candidate_backend_family_from_personality'] not in FAMILIES:
            raise ValueError('unknown candidate backend family')
        if not isinstance(row['personality'], str) or not row['personality'] or '\\x00' in row['personality']:
            raise ValueError('a nonempty, NUL-free personality name is required')
        if row.get('origin') == 'upstream_extension':
            # Unknown vendor-specific USB interfaces must never be claimed by
            # a broad VID:PID match on composite adapters. An explicit class
            # constraint is mandatory even if an upstream table uses USB_DEVICE.
            if m.get('bInterfaceClass') != 0xff:
                raise ValueError('extensions must bind only vendor-specific USB interfaces')
            if not primary_verified(row):
                raise ValueError('extensions require unambiguous primary chipset evidence')
            if row['candidate_backend_family_from_personality'] not in READY_FAMILIES:
                raise ValueError('extensions require an integrated station backend')
            if 'original_matching' in row:
                raise ValueError('extension cannot impersonate an original plist entry')
            for evidence in row['primary_usb_table_evidence']:
                revision = evidence.get('commit', evidence.get('revision', ''))
                if not re.fullmatch(r'[0-9a-f]{40}|v[0-9]+\.[0-9]+(?:\.[0-9]+)?', revision):
                    raise ValueError('extension evidence must name an immutable commit or release')
                args = evidence.get('usb_device_arguments', '').lower().replace(' ', '')
                if args != f'0x{key[0]:04x},0x{key[1]:04x}':
                    raise ValueError('extension evidence must identify the exact numeric USB ID')
                if not evidence.get('repository') or not evidence.get('path') or not evidence.get('url', '').startswith('https://'):
                    raise ValueError('extension evidence requires a primary source location')
                # A USB_DEVICE_AND_INTERFACE_INFO entry is narrower than a
                # USB_DEVICE entry. Preserve all three descriptor constraints
                # rather than silently broadening the published personality.
                arguments = evidence.get('interface_match_arguments')
                if arguments is not None:
                    values = [part.strip() for part in arguments.lower().split(',')]
                    if len(values) != 3 or any(not re.fullmatch(r'0x[0-9a-f]{1,2}', v) for v in values):
                        raise ValueError('invalid pinned upstream interface descriptor')
                    for field, value in zip(INTERFACE_FIELDS, values):
                        if m.get(field) != int(value, 16):
                            raise ValueError('extension broadens a pinned upstream interface match')
    return data, sorted(rows, key=lambda row: (row['idVendor'], row['idProduct']))

def c_string(text):
    # Three-digit octal escapes preserve UTF-8 bytes without C hex-escape greed.
    return '"' + ''.join(chr(b) if 32 <= b < 127 and b not in (34,92)
                         else '\\%03o' % b for b in text.encode('utf-8')) + '"'
