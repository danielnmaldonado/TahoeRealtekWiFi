#!/usr/bin/env python3
"""Run the real collector with a failed WPA handshake and offline tool shims."""
from pathlib import Path
import plistlib
import os
import subprocess
import sys
import tempfile

project = Path(__file__).resolve().parent.parent
source = (project / "scripts/validate-stability-pack.command").read_text()
with tempfile.TemporaryDirectory(prefix="rtw-pack-diagnostics-") as temporary:
    root = Path(temporary)
    tools = root / "fake"
    tools.mkdir()
    def command(name, body):
        path = tools / name
        path.write_text("#!/bin/bash\n" + body + "\n")
        path.chmod(0o755)
        return str(path)
    # Exercise both sides of the handoff, without privileges or USB access.
    source = source.replace('[ "$EUID" -ne 0 ]', '[ "${RTW_TEST_ADMIN:-0}" -ne 1 ]')
    source = source.replace("with administrator privileges", "")
    source = source.replace('"/bin/bash " &', '"/usr/bin/env RTW_TEST_ADMIN=1 /bin/bash " &')
    for path, body in {
        "/usr/bin/plutil": "echo org.research.TahoeRealtekWiFi",
        "/usr/bin/xcrun": "echo 'UUID: TEST-UUID (x86_64) fixture'",
        "/usr/bin/kmutil": "echo 'org.research.TahoeRealtekWiFi (15) TEST-UUID'",
        "/bin/sleep": "exit 0",
        "/usr/bin/curl": "echo UNEXPECTED_NETWORK_TEST; exit 89",
        "/sbin/ifconfig": "echo UNEXPECTED_NETWORK_TEST; exit 89",
    }.items():
        source = source.replace(path, command(Path(path).name, body))
    fixture = root / "ioreg.plist"
    fixture.write_bytes(plistlib.dumps([{"IORegistryEntryID": 12345,"RTWDiagWPASupplicantState": 1,
                                       "RTWDiagWPAEAPOLReceived": 0}]))
    source = source.replace("/usr/sbin/ioreg", command("ioreg", 'cat "' + str(fixture) + '"'))
    (root / "Tools").mkdir()
    cli = root / "Tools/rtw-control"
    cli.write_text("#!/bin/bash\nif [ \"$1\" = list ]; then printf '12345\\tRTL8192EU\\n'; exit 0; fi\n[ \"$1:$2:$3\" = --device:12345:status ] || exit 91\necho 'device=yes radio=yes running=yes authorized=no scanning=no'\n"
                   "echo 'state=4 channel=6 rssi=-40 dBm usbError=0'\n"
                   "echo 'TX packets=2 errors=0 RX packets=4 errors=0'\n")
    cli.chmod(0o755)
    script = root / "validate.command"
    script.write_text(source)
    for launch_as_admin in ([True, False] if sys.platform == "darwin" else [True]):
        env = os.environ.copy()
        env["RTW_TEST_ADMIN"] = "1" if launch_as_admin else "0"
        result = subprocess.run(["/bin/bash", str(script)], capture_output=True, text=True,
                                env=env, timeout=30)
        assert result.returncode == 2, (launch_as_admin, result.returncode, result.stdout, result.stderr)
        assert '"RTWDiagWPAEAPOLReceived": 0' in result.stdout, result.stdout
        assert '"RTWDiagWPASupplicantState": 1' in result.stdout, result.stdout
        assert "UNEXPECTED_NETWORK_TEST" not in result.stdout
        if not launch_as_admin:
            logs = list((root / "Validation").glob("postboot-*.txt"))
            assert logs and '"RTWDiagWPASupplicantState": 1' in logs[0].read_text()
    print("Collector preserves failed-WPA diagnostics, output and exit2 through the real AppleScript handoff")

    # Authorized performance collection must separate the RX-rate samples
    # around download/upload and bind both synthetic transfers to the USB.
    authorized_source = source.replace('/usr/sbin/ipconfig', command('ipconfig',
        'if [ "$1" = getifaddr ]; then echo 192.0.2.2; else echo 192.0.2.1; fi'))
    authorized_source = authorized_source.replace('/sbin/ping', command('ping', 'exit 0'))
    command('ifconfig', 'if [ "$1" = -l ]; then echo en7; else echo "ether 02:00:00:00:00:07"; fi')
    command('curl', '''case "$*" in
 *__up*) /usr/bin/python3 -c 'import sys; data=sys.stdin.buffer.read(); assert len(data)==4194304 and not any(data); print("MOCK_UPLOAD_ZERO_BYTES="+str(len(data))); print("MOCK_UPLOAD_INTERFACE="+sys.argv[sys.argv.index("--interface")+1]); print("MOCK_UPLOAD_URL="+sys.argv[-1])' "$@" || exit "$?";;
 *) echo MOCK_DOWNLOAD "$*";;
esac''')
    cli.write_text("#!/bin/bash\nif [ \"$1\" = list ]; then printf '12345\\tRTL8192EU\\n'; exit 0; fi\n[ \"$1:$2:$3\" = --device:12345:status ] || exit 91\necho 'device=yes radio=yes running=yes authorized=yes scanning=no'\n"
                   "echo 'state=4 channel=6 rssi=-40 dBm usbError=0'\n"
                   "echo 'MAC=02:00:00:00:00:07'\n")
    fixture.write_bytes(plistlib.dumps([{'IORegistryEntryID':12345,'RTWDiagRXRate11Packets': 100,
                                       'RTWDiagRXRate11Bytes': 145000}]))
    script.write_text(authorized_source)
    env = os.environ.copy()
    env['RTW_TEST_ADMIN'] = '1'
    result = subprocess.run(['/bin/bash', str(script)], capture_output=True,
                            text=True, env=env, timeout=30)
    assert result.returncode == 0, (result.returncode, result.stdout, result.stderr)
    for phase in ('before-download', 'after-download', 'after-upload'):
        assert 'PHASE=' + phase in result.stdout, (phase, result.stdout)
    assert 'MOCK_UPLOAD_INTERFACE=en7' in result.stdout, result.stdout
    assert 'MOCK_UPLOAD_URL=https://speed.cloudflare.com/__up' in result.stdout, result.stdout
    assert 'MOCK_UPLOAD_ZERO_BYTES=4194304' in result.stdout, result.stdout
    assert '"RTWDiagRXRate11Bytes": 145000' in result.stdout, result.stdout
    print('Collector records rate snapshots and bound synthetic upload/download without live network access')

    # Multiple adapters must retain one explicit target through every status
    # sample and the IORegistry filter. Never include a sibling's diagnostics.
    fixture.write_bytes(plistlib.dumps([
        {'IORegistryEntryID':12345,'RTWDiagRXRate11Bytes':145000},
        {'IORegistryEntryID':98765,'RTWDiagRXRate11Bytes':999999}]))
    body=cli.read_text().replace("printf '12345\\tRTL8192EU\\n'", "printf '12345\\tRTL8192EU\\n98765\\tRTL8814AU\\n'")
    cli.write_text(body)
    result=subprocess.run(['/bin/bash',str(script),'12345'],capture_output=True,text=True,env=env,timeout=30)
    assert result.returncode==0,(result.returncode,result.stdout,result.stderr)
    assert 'SELECTED_REGISTRY_ID=12345' in result.stdout and '145000' in result.stdout
    assert '999999' not in result.stdout,result.stdout
    result=subprocess.run(['/bin/bash',str(script)],capture_output=True,text=True,env=env,timeout=30)
    assert result.returncode==4,(result.returncode,result.stdout,result.stderr)
    assert 'MOCK_DOWNLOAD' not in result.stdout and 'PHASE=' not in result.stdout
    print('Collector pins the chosen registry ID and refuses ambiguous automatic selection')
