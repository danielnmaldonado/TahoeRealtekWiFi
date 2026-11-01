#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-only
# Place beside Experimental/ and Tools/ in the hardware validation pack.
set -euo pipefail
base="$(cd "$(dirname "$0")" && pwd -P)"
cli="$base/Tools/rtw-control"
target_id="${1:-}"
if [ -z "$target_id" ]; then
 devices=$("$cli" list)
 ids=$(/bin/echo "$devices" | /usr/bin/awk -F '\t' '$1 ~ /^[0-9]+$/ {print $1}')
 count=$(/bin/echo "$ids" | /usr/bin/awk 'NF{n++} END{print n+0}')
 if [ "$count" = 1 ]; then target_id="$ids";
 elif [ "$count" -gt 1 ] && [ "$EUID" -ne 0 ]; then
  target_id=$(/usr/bin/osascript - "$devices" <<'CHOOSE'
on run argv
 set chosen to choose from list (paragraphs of item 1 of argv) with prompt "Qual adaptador USB deseja validar?" without multiple selections allowed
 if chosen is false then return ""
 set text item delimiters to tab
 return text item 1 of item 1 of chosen
end run
CHOOSE
)
 else /bin/echo "$devices"; /bin/echo 'Execute este coletor com o ID decimal do adaptador como argumento.'; exit 4;
 fi
fi
case "$target_id" in ''|0|*[!0-9]*) /bin/echo 'ID de adaptador inválido ou seleção cancelada.'; exit 4;; esac
if [ "$EUID" -ne 0 ]; then
 /bin/mkdir -p "$base/Validation"
 collector_log="$base/Validation/postboot-$(/bin/date '+%Y%m%d-%H%M%S').txt"
 /usr/bin/osascript - "$0" "$target_id" <<'AS' | /usr/bin/tr '\r' '\n' | /usr/bin/tee "$collector_log"
on run argv
 -- Keep diagnostics on stdout even when the collector reports incomplete WPA.
 -- Return its real exit status separately; do shell script otherwise discards
 -- stdout by raising an AppleScript error on every nonzero shell exit.
 return do shell script ("/bin/bash " & quoted form of (item 1 of argv) & " " & quoted form of (item 2 of argv) & " 2>&1; rtw_status=$?; /bin/echo RTW_COLLECTOR_EXIT=$rtw_status; exit 0") with administrator privileges
end run
AS
 collector_exit=$(/usr/bin/awk -F= '/^RTW_COLLECTOR_EXIT=[0-9]+$/{code=$2} END{print code}' "$collector_log")
 case "$collector_exit" in ''|*[!0-9]*) exit 1;; esac
 [ "$collector_exit" -le 255 ] || exit 1
 exit "$collector_exit"
fi
bundle="$base/Experimental/TahoeRealtekWiFi.kext"
binary="$bundle/Contents/MacOS/TahoeRealtekWiFi"
[ "$(/usr/bin/plutil -extract CFBundleIdentifier raw -o - "$bundle/Contents/Info.plist")" = org.research.TahoeRealtekWiFi ]
expected_uuid=$(/usr/bin/xcrun dwarfdump --uuid "$binary" | /usr/bin/awk '/\(x86_64\)/{print $2}')
[ -n "$expected_uuid" ] || { /bin/echo 'UUID Intel não encontrado no pacote.';exit 3; }
/bin/echo "EXPECTED_UUID=$expected_uuid"
/bin/echo "SELECTED_REGISTRY_ID=$target_id"
dump_diagnostics() {
/usr/sbin/ioreg -r -c RTWUSBController -a | /usr/bin/python3 -c '
import plistlib,sys,json
objects=plistlib.loads(sys.stdin.buffer.read())
def walk(value):
 if isinstance(value,dict):
  props={k:v for k,v in value.items() if (k.startswith("RTWDiag") or k in ("IQCalibrationMask","RTWStartStage","RTWStartResult")) and isinstance(v,(int,str))}
  if props: print(json.dumps(props,sort_keys=True))
  for child in value.get("IORegistryEntryChildren",[]):walk(child)
for obj in objects:
 if obj.get("IORegistryEntryID")==int(sys.argv[1]):walk(obj)
' "$target_id"
}
loaded=$(/usr/bin/kmutil showloaded --list-only 2>/dev/null | /usr/bin/awk '/org.research.TahoeRealtekWiFi/')
/bin/echo "$loaded"
case "$loaded" in *"$expected_uuid"*) ;; *) /bin/echo 'A kext deste pacote ainda não está carregada.';exit 3;; esac
status=$(/usr/bin/env -i PATH=/usr/bin:/bin:/usr/sbin:/sbin "$cli" --device "$target_id" status)
/bin/echo "$status" | /usr/bin/awk '/^device=|^state=|^TX packets=/'
dump_diagnostics
case "$status" in *'authorized=yes'*) ;; *) /bin/echo 'Autorização WPA2 incompleta; diagnóstico coletado acima. Testes de tráfego dependem da conexão.';exit 2;; esac
mac=$(/bin/echo "$status" | /usr/bin/awk '/^MAC=/{sub(/^MAC=/,"",$1); print $1}')
iface=''
for candidate in $(/sbin/ifconfig -l); do
 case "$candidate" in en[0-9]*) ;; *) continue;; esac
 actual=$(/sbin/ifconfig "$candidate" | /usr/bin/awk '/ether /{print $2;exit}')
 if [ "$actual" = "$mac" ]; then
  [ -z "$iface" ] || { /bin/echo 'Mais de uma interface com o MAC USB.';exit 4; }
  iface="$candidate"
 fi
done
[ -n "$iface" ] || { /bin/echo 'Interface USB não encontrada.';exit 4; }
ip=$(/usr/sbin/ipconfig getifaddr "$iface")
gateway=$(/usr/sbin/ipconfig getoption "$iface" router | /usr/bin/head -1)
[ -n "$ip" ] && [ -n "$gateway" ]
/bin/echo "USB_INTERFACE=$iface IPv4=$ip ROUTER=$gateway"
/sbin/ping -b "$iface" -S "$ip" -i 0.2 -c 50 -W 1000 -t 20 "$gateway" || true
for attempt in 1 2 3; do
 /usr/bin/curl --interface "$iface" --noproxy '*' --ipv4 --connect-timeout 10 --max-time 20 \
  --silent --show-error --output /dev/null \
  --write-out 'HTTPS code=%{http_code} local=%{local_ip} seconds=%{time_total}\n' https://example.com || true
done
sample_performance() {
 /bin/echo "PHASE=$1"
 "$cli" --device "$target_id" status | /usr/bin/awk '/^device=|^state=|^TX packets=/'
 dump_diagnostics
}
sample_performance before-download
/usr/bin/curl --interface "$iface" --noproxy '*' --ipv4 --connect-timeout 10 --max-time 45 \
 --silent --show-error --output /dev/null \
 --write-out 'DOWNLOAD code=%{http_code} local=%{local_ip} bytes=%{size_download} bytes_per_second=%{speed_download} seconds=%{time_total}\n' \
 'https://speed.cloudflare.com/__down?bytes=2097152' || true
sample_performance after-download
# Upload only synthetic zero bytes; never read a user's file as test data.
/usr/bin/python3 - "$iface" <<'PY'
import subprocess,sys
result=subprocess.run(['/usr/bin/curl', '--interface', sys.argv[1], '--noproxy', '*',
 '--ipv4', '--connect-timeout', '10', '--max-time', '30', '--silent', '--show-error',
 '--output', '/dev/null', '--write-out',
 'UPLOAD code=%{http_code} local=%{local_ip} bytes=%{size_upload} bytes_per_second=%{speed_upload} seconds=%{time_total}\n',
 '-H', 'Content-Type: application/octet-stream', '--data-binary', '@-',
 'https://speed.cloudflare.com/__up'], input=bytes(4194304))
if result.returncode: print('UPLOAD_EXIT=' + str(result.returncode))
PY
sample_performance after-upload
/bin/echo 'Resultados coletados; perdas/timeout impedem aprovação automática.'
