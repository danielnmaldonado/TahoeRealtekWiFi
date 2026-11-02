#!/bin/bash
# TahoeRealtekWiFi — read-only diagnostic collector for macOS Tahoe / Intel.
# Compatible with macOS Bash 3.2; no Homebrew, Python or kext required.
# Does not install/load drivers or change SIP/AMFI or network settings.
set -u
set -o pipefail
umask 077

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd -P)"
COLLECTOR_REVISION=3
MINUTES=30
INCLUDE_DEBUG=0
RAW=0
ADMIN=0
FULL_UNIFIED=0
OUTPUT_BASE="$HOME/Library/Logs/Tahoe-WiFi-USB/Diagnostics"
COMMAND_TIMEOUT=30
NO_PAUSE=0

usage() {
    printf 'Collector revision: %s\n\n' "$COLLECTOR_REVISION"
    cat <<'HELP'
TahoeRealtekWiFi — WiFi-Diagnostics.command

Double-click: collect detailed diagnostics from the last 30 minutes with basic
personal-data masking. Save a directory and ZIP in ~/Library/Logs/Tahoe-WiFi-USB/Diagnostics/.
Includes two readings per adapter and USB/RX/TX counter differences.
If the main log query times out, try an independent recent 2-minute window.

Terminal options:
  --minutes N       Log window: 30 or 60 minutes (default: 30)
  --hours 1         Compatibility shortcut: select 60 minutes
  --debug           Include DEBUG messages in the query (default: INFO)
  --admin           Use sudo ONLY for log reads and 'wdutil info'
  --raw             Do not mask personal data: share privately only
  --full-unified    Include the COMPLETE logarchive window (requires --raw);
                    it may be large and contain sensitive data
  --output-dir DIR  Custom destination for the directory and ZIP
  --command-timeout N  Per-command limit: 1–120 seconds (default: 30)
  --no-pause        Do not wait for Enter when finished
  --help            Show this help

Never requests your Wi-Fi password. Without --admin, no macOS password is requested.
Does not send data over the Internet; attach the ZIP manually to the chat.
HELP
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --minutes)
            if [ "$#" -lt 2 ]; then echo "Missing value after --minutes" >&2; exit 2; fi
            MINUTES="$2"; shift 2 ;;
        --hours)
            if [ "$#" -lt 2 ]; then echo "Missing value after --hours" >&2; exit 2; fi
            if [ "$2" != "1" ]; then echo "--hours accepts only 1 (60 minutes)." >&2; exit 2; fi
            MINUTES=60; shift 2 ;;
        --debug) INCLUDE_DEBUG=1; shift ;;
        --admin) ADMIN=1; shift ;;
        --raw) RAW=1; shift ;;
        --full-unified) FULL_UNIFIED=1; shift ;;
        --output-dir)
            if [ "$#" -lt 2 ]; then echo "Missing directory after --output-dir" >&2; exit 2; fi
            OUTPUT_BASE="$2"; shift 2 ;;
        --command-timeout)
            if [ "$#" -lt 2 ]; then echo "Missing value after --command-timeout" >&2; exit 2; fi
            COMMAND_TIMEOUT="$2"; shift 2 ;;
        --no-pause) NO_PAUSE=1; shift ;;
        --help|-h) usage; exit 0 ;;
        *) echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
    esac
done

case "$MINUTES" in
    30|60) ;;
    *) echo "--minutes accepts only 30 or 60 minutes." >&2; exit 2 ;;
esac
case "$COMMAND_TIMEOUT" in
    ''|*[!0-9]*|????*) echo "--command-timeout accepts integers from 1 to 120." >&2; exit 2 ;;
esac
# A decimal prefix prevents octal interpretation of values such as 08 in Bash 3.2.
COMMAND_TIMEOUT=$((10#$COMMAND_TIMEOUT))
if [ "$COMMAND_TIMEOUT" -lt 1 ] || [ "$COMMAND_TIMEOUT" -gt 120 ]; then
    echo "--command-timeout accepts integers from 1 to 120." >&2; exit 2
fi
if [ "$FULL_UNIFIED" -eq 1 ] && [ "$RAW" -ne 1 ]; then
    echo "--full-unified requires --raw: logarchive files cannot be masked." >&2
    exit 2
fi
if ! /usr/bin/perl -MJSON::PP -MPOSIX -MTime::HiRes -e 1 >/dev/null 2>&1; then
    echo "This Mac needs native Perl and its standard modules for collection. Nothing was collected." >&2
    exit 1
fi
if [ "$(uname -s)" != "Darwin" ]; then
    echo "Run this .command directly on macOS." >&2
    exit 1
fi

if ! mkdir -p "$OUTPUT_BASE"; then
    echo "No write permission for: $OUTPUT_BASE" >&2
    exit 1
fi
OUTPUT_BASE="$(cd "$OUTPUT_BASE" && pwd -P)"
STAMP="$(date '+%Y%m%d-%H%M%S')"
RUN_DIR="$(mktemp -d "$OUTPUT_BASE/WiFi-Diagnostics-${STAMP}-XXXXXX")" || exit 1
ZIP_PATH="$RUN_DIR.zip"
mkdir -p "$RUN_DIR/system" "$RUN_DIR/usb" "$RUN_DIR/network" \
         "$RUN_DIR/drivers" "$RUN_DIR/logs" "$RUN_DIR/crashes"
STATUS="$RUN_DIR/STATUS.tsv"
printf 'file\tresult\texit_status\tcommand\n' > "$STATUS"
SUCCESS=0
FAILURE=0

# Each command has its own group. The collector terminates only its children
# and descendants; the PID remains reserved until waitpid completes.
# The monotonic clock is independent of date/NTP adjustments during collection.
BOUNDED_PERL=$(cat <<'PERL_TIMEOUT'
use strict;
use warnings;
use POSIX qw(WNOHANG setpgid);
use Time::HiRes qw(clock_gettime CLOCK_MONOTONIC sleep);
my $limit=shift @ARGV;
my $pid=fork();
die "fork: $!\n" unless defined $pid;
if (!$pid) {
    defined(setpgid(0,0)) or POSIX::_exit(125);
    exec {$ARGV[0]} @ARGV or do {
        print STDERR "exec $ARGV[0]: $!\n";
        POSIX::_exit(127);
    };
}
my $interrupted=0;
$SIG{INT}=sub {$interrupted=130};
$SIG{TERM}=sub {$interrupted=143};
my $end=clock_gettime(CLOCK_MONOTONIC)+$limit;
while (1) {
    my $done=waitpid($pid,WNOHANG);
    if ($done==$pid) {my $status=$?;exit(($status&127)?128+($status&127):$status>>8);}
    if ($done<0) {next if $!{EINTR};die "waitpid: $!\n";}
    if ($interrupted || clock_gettime(CLOCK_MONOTONIC)>=$end) {
        print STDERR "Command time limit exceeded (${limit}s)\n" unless $interrupted;
        kill "TERM",-$pid;
        sleep(0.2);
        kill "KILL",-$pid;
        while (waitpid($pid,0)<0) {last unless $!{EINTR};}
        exit($interrupted||124);
    }
    sleep(0.025);
}
PERL_TIMEOUT
)
run_bounded() {
    /usr/bin/perl -e "$BOUNDED_PERL" "$COMMAND_TIMEOUT" "$@"
}
run_admin_bounded() {
    # The monitor also needs root to terminate its own children.
    sudo -n /usr/bin/perl -e "$BOUNDED_PERL" "$COMMAND_TIMEOUT" "$@"
}
trap 'exit 130' INT
trap 'exit 143' TERM

# Include individual failures in the package without interrupting other checks.
run_capture() {
    local rel="$1" rc quoted="" a
    shift
    printf '  • %s\n' "$rel"
    for a in "$@"; do quoted="$quoted $(printf '%q' "$a")"; done
    if run_bounded "$@" > "$RUN_DIR/$rel" 2>&1; then
        printf '%s\tok\t0\t%s\n' "$rel" "$quoted" >> "$STATUS"
        SUCCESS=$((SUCCESS + 1))
    else
        rc=$?
        printf '%s\tfailed\t%s\t%s\n' "$rel" "$rc" "$quoted" >> "$STATUS"
        FAILURE=$((FAILURE + 1))
    fi
}

run_admin_capture() {
    local rel="$1" rc quoted="" a
    shift
    printf '  • %s (administrative read)\n' "$rel"
    for a in "$@"; do quoted="$quoted $(printf '%q' "$a")"; done
    if run_admin_bounded "$@" > "$RUN_DIR/$rel" 2>&1; then
        printf '%s\tok\t0\tsudo%s\n' "$rel" "$quoted" >> "$STATUS"
        SUCCESS=$((SUCCESS + 1))
    else
        rc=$?
        printf '%s\tfailed\t%s\tsudo%s\n' "$rel" "$rc" "$quoted" >> "$STATUS"
        FAILURE=$((FAILURE + 1))
    fi
}

# spctl uses exit1 for the disabled state. Accept only this exact response;
# additional messages, other errors or other exit codes remain failures.
capture_gatekeeper_state() {
    local rc
    echo '  • system/gatekeeper.txt'
    if run_bounded spctl --status > "$RUN_DIR/system/gatekeeper.txt" 2>&1; then
        printf 'system/gatekeeper.txt\tok\t0\tspctl --status\n' >> "$STATUS"
        SUCCESS=$((SUCCESS + 1))
    else
        rc=$?
        if [ "$rc" -eq 1 ] && /usr/bin/perl -0777 -e '
            my $text=<>;
            exit(defined($text) && ($text eq "assessments disabled\n" ||
                $text eq "assessments disabled")?0:1);
        ' "$RUN_DIR/system/gatekeeper.txt"; then
            printf 'system/gatekeeper.txt\texpected_state\t1\tspctl --status\n' >> "$STATUS"
            SUCCESS=$((SUCCESS + 1))
        else
            printf 'system/gatekeeper.txt\tfailed\t%s\tspctl --status\n' "$rc" >> "$STATUS"
            FAILURE=$((FAILURE + 1))
        fi
    fi
}

cat <<EOM

========================================================
 TahoeRealtekWiFi — macOS USB / Wi-Fi diagnostics
========================================================
Collector revision: ${COLLECTOR_REVISION}
Log window: ${MINUTES} minutes
Privacy: $(if [ "$RAW" -eq 1 ]; then echo 'original data (SENSITIVE)'; else echo 'basic masking'; fi)
Collection: read-only system queries; creates report files only.
Destination: $RUN_DIR

EOM

ADMIN_READY=0
if [ "$ADMIN" -eq 1 ]; then
    echo "--admin requested: sudo will be used only to read diagnostics."
    if sudo -v; then ADMIN_READY=1; else echo "No sudo: continuing without additional privileges."; fi
fi

# Capture early to preserve failed-adapter evidence before slow commands.
run_capture drivers/RTW-before-time.txt date -u '+%Y-%m-%dT%H:%M:%SZ'
run_capture drivers/RTW-before.txt ioreg -r -c RTWUSBController -l -w0
run_capture drivers/loaded-RTW-kext.txt kextstat -l -b org.research.TahoeRealtekWiFi
if [ -f "$SCRIPT_DIR/kext/Info.plist" ]; then
    run_capture drivers/source-kext-Info.txt plutil -p "$SCRIPT_DIR/kext/Info.plist"
fi
if [ -f "/Applications/Tahoe Wi-Fi USB.app/Contents/Info.plist" ]; then
    run_capture drivers/installed-app-Info.txt plutil -p "/Applications/Tahoe Wi-Fi USB.app/Contents/Info.plist"
fi

# Numeric request/result metadata written by app48; no credential payloads.
app_control_log="$HOME/Library/Logs/Tahoe-WiFi-USB/control.jsonl"
if [ -f "$app_control_log" ] && [ ! -L "$app_control_log" ]; then
    run_capture logs/app-control.jsonl tail -c 2097152 "$app_control_log"
fi

# System inventory: do not export complete NVRAM, passwords or Wi-Fi profiles.
echo '[1/7] System and security state'
run_capture system/macOS.txt sw_vers
run_capture system/kernel.txt uname -sr
run_capture system/architecture.txt uname -m
run_capture system/sip.txt csrutil status
capture_gatekeeper_state
run_capture system/boot_args.txt nvram boot-args
run_capture system/hardware.txt system_profiler SPHardwareDataType
run_capture system/software.txt system_profiler SPSoftwareDataType
run_capture system/power.txt pmset -g

# macOS Tahoe 26 renamed SPUSBDataType to SPUSBHostDataType.
echo '[2/7] Complete USB inventory: VID:PID, ports, interfaces and endpoints'
run_capture usb/system_profiler_types.txt system_profiler -listDataTypes
USB_TYPE_FOUND=0
if grep -Eq '(^|[[:space:]])SPUSBHostDataType([[:space:]]|$)' "$RUN_DIR/usb/system_profiler_types.txt"; then
    run_capture usb/SPUSBHostDataType.txt system_profiler -detailLevel full SPUSBHostDataType
    run_capture usb/SPUSBHostDataType.json system_profiler -json SPUSBHostDataType
    USB_TYPE_FOUND=1
fi
if grep -Eq '(^|[[:space:]])SPUSBDataType([[:space:]]|$)' "$RUN_DIR/usb/system_profiler_types.txt"; then
    run_capture usb/SPUSBDataType.txt system_profiler -detailLevel full SPUSBDataType
    run_capture usb/SPUSBDataType.json system_profiler -json SPUSBDataType
    USB_TYPE_FOUND=1
fi
if [ "$USB_TYPE_FOUND" -eq 0 ]; then
    # Fall back if type enumeration fails or varies between builds.
    run_capture usb/SPUSBHostDataType.txt system_profiler -detailLevel full SPUSBHostDataType
    run_capture usb/SPUSBDataType.txt system_profiler -detailLevel full SPUSBDataType
fi
run_capture usb/ioreg_USB_tree.txt ioreg -p IOUSB -l -w0
run_capture usb/ioreg_USBHostDevice.txt ioreg -r -c IOUSBHostDevice -l -w0
run_capture usb/ioreg_USBHostInterface.txt ioreg -r -c IOUSBHostInterface -l -w0
run_capture usb/ioreg_USBHostPipe.txt ioreg -r -c IOUSBHostPipe -l -w0
run_capture usb/ioreg_legacy_USBDevice.txt ioreg -r -c IOUSBDevice -l -w0

# Enumerate ALL USB devices: rebranded adapters may not use VID 0BDA.
# If IORegistry does not expose both IDs, use the complete reports.
{
    echo 'USB DEVICES FOUND (names and IDs read from IOUSBHostDevice)'
    echo 'Notice: VID:PID identifies the USB device, but does not guarantee its internal chipset.'
    echo
    if command -v perl >/dev/null 2>&1; then
        perl - "$RUN_DIR/usb/ioreg_USBHostDevice.txt" <<'PERL_USB'
use strict;
use warnings;
my $source = shift;
open my $fh, '<', $source or die "Could not read $source: $!\n";
my ($name, $vid, $pid, $revision, $manufacturer, $product);
my $emit = sub {
    return unless defined($vid) && defined($pid);
    my $fmt = sub {
        my $v=shift;
        return $v =~ /^0x/i ? hex($v) : int($v);
    };
    my ($v, $p) = ($fmt->($vid), $fmt->($pid));
    return if $v > 65535 || $p > 65535;
    my $n = defined($product) ? $product : (defined($name) ? $name : '(unnamed)');
    $n =~ s/[\r\n\t]+/ /g;
    printf "VID:PID %04X:%04X | %s", $v, $p, $n;
    print " | manufacturer: $manufacturer" if defined($manufacturer);
    print " | bcdDevice: $revision" if defined($revision);
    print "\n";
};
while (my $line = <$fh>) {
    if ($line =~ /\+\-o\s+([^<]+?)\s+</) {
        $emit->();
        ($name, $vid, $pid, $revision, $manufacturer, $product) = ($1, undef, undef, undef, undef, undef);
    }
    $vid = $1 if $line =~ /["'](?:idVendor)["']\s*=\s*(0x[\da-fA-F]+|\d+)/;
    $pid = $1 if $line =~ /["'](?:idProduct)["']\s*=\s*(0x[\da-fA-F]+|\d+)/;
    $revision = $1 if $line =~ /["'](?:bcdDevice)["']\s*=\s*(0x[\da-fA-F]+|\d+)/;
    $manufacturer = $1 if $line =~ /"(?:USB Vendor Name|USB Vendor String)"\s*=\s*"([^"]*)"/;
    $product = $1 if $line =~ /"(?:USB Product Name|USB Product String)"\s*=\s*"([^"]*)"/;
}
$emit->();
PERL_USB
    else
        echo 'Perl unavailable; check the ioreg_USB*.txt files.'
    fi
} > "$RUN_DIR/USB-IDENTIFICATION.txt" 2>&1
if ! grep -q '^VID:PID ' "$RUN_DIR/USB-IDENTIFICATION.txt"; then
    printf '\nNo VID:PID extracted automatically. See the complete ioreg and system_profiler output in usb/.\n' >> "$RUN_DIR/USB-IDENTIFICATION.txt"
fi

{
    echo 'POSSIBLE USB WI-FI ADAPTERS (excerpts, not an exhaustive list)'
    echo 'For confirmation, check the COMPLETE USB inventory in the package.'
    echo
    for f in "$RUN_DIR"/usb/SPUSB*txt "$RUN_DIR"/usb/ioreg_USB_tree.txt; do
        [ -f "$f" ] || continue
        echo "--- $(basename "$f") ---"
        grep -niE -C 10 'Realtek|rtl8[0-9]|rtwlan|wlan|802\.11|wireless|wi-?fi|0x0bda|0x2357|0x7392|0x0b05|0x2001' "$f" || true
    done
} > "$RUN_DIR/USB-WIFI-CANDIDATES.txt"

# Active networks and how macOS maps services to en0, en1 and other interfaces.
echo '[3/7] Networking, native Wi-Fi and Ethernet interfaces'
run_capture network/hardware_ports.txt networksetup -listallhardwareports
run_capture network/network_services.txt networksetup -listallnetworkservices
run_capture network/ifconfig_all.txt ifconfig -a
run_capture network/scutil_nwi.txt scutil --nwi
run_capture network/routes.txt netstat -rn
run_capture network/network_profiler.txt system_profiler SPNetworkDataType
run_capture network/airport_profiler.txt system_profiler SPAirPortDataType
if command -v wdutil >/dev/null 2>&1; then
    if [ "$ADMIN_READY" -eq 1 ]; then
        run_admin_capture network/wdutil_info.txt wdutil info
    else
        run_capture network/wdutil_info.txt wdutil info
    fi
fi

# Running drivers and the devices claimed by IOKit services.
echo '[4/7] Kexts, System Extensions and driver matching'
run_capture drivers/kmutil_showloaded.txt kmutil showloaded
run_capture drivers/kextstat.txt kextstat -l
run_capture drivers/systemextensionsctl.txt systemextensionsctl list
run_capture drivers/ethernet_controllers.txt ioreg -r -c IOEthernetController -l -w0
run_capture drivers/network_interfaces.txt ioreg -r -c IONetworkInterface -l -w0
run_capture drivers/installed_kexts.txt ls -la /Library/Extensions
run_capture drivers/installed_systemextensions.txt ls -la /Library/SystemExtensions

# One main unified-log query; only timeout124 allows a second, independent
# query for the last 2 minutes. Preserve partial history.
# The filter prioritizes USB and Wi-Fi/Ethernet messages rather than all
# configd/networkd events over 24 hours. Include INFO and default messages;
# enable DEBUG only with --debug to avoid unnecessarily large files.
echo "[5/7] Unified logs (${MINUTES} minutes, main query; DEBUG=$(if [ "$INCLUDE_DEBUG" -eq 1 ]; then echo yes; else echo no; fi))"
USB_PRED='(subsystem CONTAINS[c] "usb" OR eventMessage CONTAINS[c] "Realtek" OR eventMessage CONTAINS[c] "RtWlan" OR eventMessage CONTAINS[c] "rtl8192" OR eventMessage CONTAINS[c] "2357" OR eventMessage CONTAINS[c] "TahoeRealtekWiFi")'
WIFI_PRED='(subsystem CONTAINS[c] "wifi" OR subsystem CONTAINS[c] "80211" OR eventMessage CONTAINS[c] "AirPort" OR eventMessage CONTAINS[c] "itlwm")'
NET_PRED='(eventMessage CONTAINS[c] "IOEthernet" OR eventMessage CONTAINS[c] "IONetwork" OR subsystem CONTAINS[c] "network")'
UNIFIED_PRED="($USB_PRED OR $WIFI_PRED OR $NET_PRED)"
# Each query uses the same converter: up to 6 MiB per category, retaining the
# last RECEIVED lines. A timeout may stop before reaching the present.
# Coverage records timestamps actually received, including when no line has
# a recognized timestamp; that does not establish the absence of events.
UNIFIED_CONVERTER=$(cat <<'PERL_UNIFIED'
    use strict;
    use warnings;
    use JSON::PP;
    my ($outdir, $max, $minutes) = @ARGV;
    my @names = ("USB_Realtek", "WiFi_80211", "Network_Ethernet");
    my (%queue, %size, %count, %dropped);
    my ($first_timestamp,$last_timestamp);
    my $received_lines=0;
    for my $n (@names) {
        $queue{$n} = [];
        $size{$n} = 0;
        $count{$n} = 0;
        $dropped{$n} = 0;
    }
    my @previous_tags;
    while (my $line = <STDIN>) {
        $received_lines++;
        if ($line =~ /^(\d{4}-\d{2}-\d{2}[ T]\d{2}:\d{2}:\d{2}(?:\.\d+)?(?:Z|[+-]\d{2}:?\d{2})?)(?=\s|$)/) {
            $first_timestamp=$1 unless defined $first_timestamp;
            $last_timestamp=$1;
        }
        my @tags;
        push @tags, "USB_Realtek" if $line =~ /usb|realtek|rtwlan|rtl8192|2357|0108|IOUSBHost|TahoeRealtekWiFi/i;
        push @tags, "WiFi_80211" if $line =~ /wi-?fi|802\.11|80211|airport|wifid|itlwm|corewlan/i;
        push @tags, "Network_Ethernet" if $line =~ /network|ethernet|IONetwork|en[0-9]+|dhcp/i;
        if (@tags) { @previous_tags = @tags; }
        elsif ($line =~ /^[\t ]/ && @previous_tags) { @tags = @previous_tags; }
        else { @previous_tags = (); }
        for my $n (@tags) {
            push @{$queue{$n}}, $line;
            $size{$n} += length($line);
            $count{$n}++;
            while ($size{$n} > $max && @{$queue{$n}}) {
                $size{$n} -= length(shift @{$queue{$n}});
                $dropped{$n}++;
            }
        }
    }
    open my $summary, ">", "$outdir/unified-summary.txt" or die "$outdir: $!";
    print $summary "Unified-log query; requested window: last ${minutes}min.\n";
    print $summary "Content limit per category: $max bytes, retaining the last RECEIVED messages.\n";
    for my $n (@names) {
        open my $fh, ">", "$outdir/$n.log" or die "$n: $!";
        print $fh "# $n: messages received in this query, possibly truncated by the size limit.\n";
        print $fh "# Old messages discarded by the limit: $dropped{$n}\n";
        if (!@{$queue{$n}}) {
            print $fh "# No matching line received in this query; check coverage and status.\n";
        } else {
            print $fh $_ for @{$queue{$n}};
        }
        close $fh or die "close $n: $!";
        printf $summary "%s: matched=%d; retained=%d; old_discarded=%d; bytes_retained=%d\n",
            $n, $count{$n}, scalar(@{$queue{$n}}), $dropped{$n}, $size{$n};
    }
    close $summary or die "close summary: $!";
    open my $coverage, ">", "$outdir/unified-coverage.json" or die "coverage: $!";
    print $coverage JSON::PP->new->canonical->pretty->encode({
        requested_window_minutes=>0+$minutes,
        command_exit_status=>undef,
        capture_status=>"unfinalized",
        first_received_timestamp=>$first_timestamp,
        last_received_timestamp=>$last_timestamp,
        received_lines=>$received_lines,
        present_time_coverage=>"not_established"
    });
    close $coverage or die "close coverage: $!";
PERL_UNIFIED
)

capture_unified_query() {
    local minutes="$1" rel="$2" outdir query_rc converter_rc coverage_rc
    local -a log_args log_cmd pipeline_rc
    outdir="$RUN_DIR/$rel"
    if ! mkdir -p "$outdir"; then
        printf '%s/unified-onepass\tfailed\t125\tcould not create destination\n' "$rel" >> "$STATUS"
        FAILURE=$((FAILURE + 1))
        return 125
    fi
    log_args=(show --last "${minutes}m" --style syslog --info --no-pager --predicate "$UNIFIED_PRED")
    if [ "$INCLUDE_DEBUG" -eq 1 ]; then log_args+=(--debug); fi
    if [ "$ADMIN_READY" -eq 1 ]; then
        log_cmd=(run_admin_bounded log "${log_args[@]}")
    else
        log_cmd=(run_bounded log "${log_args[@]}")
    fi
    # Terminate the log process group at the deadline; EOF preserves partial output.
    # Save both statuses immediately to preserve the real query return code,
    # even if the converter also fails.
    if "${log_cmd[@]}" 2>"$outdir/log-show.stderr.txt" | /usr/bin/perl -e "$UNIFIED_CONVERTER" "$outdir" 6291456 "$minutes"; then
        pipeline_rc=("${PIPESTATUS[@]}")
    else
        pipeline_rc=("${PIPESTATUS[@]}")
    fi
    query_rc=${pipeline_rc[0]}
    converter_rc=${pipeline_rc[1]}
    if [ "$query_rc" -eq 0 ]; then
        printf '%s/unified-onepass\tok\t0\tlog show --last %sm\n' "$rel" "$minutes" >> "$STATUS"
        SUCCESS=$((SUCCESS + 1))
    else
        printf '%s/unified-onepass\tfailed\t%s\tlog show --last %sm\n' "$rel" "$query_rc" "$minutes" >> "$STATUS"
        FAILURE=$((FAILURE + 1))
        echo "[WARNING] ${minutes}min query failed ($query_rc): see $rel/log-show.stderr.txt" >&2
    fi
    if [ "$converter_rc" -ne 0 ]; then
        printf '%s/unified-converter\tfailed\t%s\tPerl: received categories\n' "$rel" "$converter_rc" >> "$STATUS"
        FAILURE=$((FAILURE + 1))
    fi
    if /usr/bin/perl - "$outdir" "$minutes" "$query_rc" "$converter_rc" <<'PERL_COVERAGE'
use strict;
use warnings;
use JSON::PP;
my ($root,$minutes,$query_rc,$converter_rc)=@ARGV;
my $path="$root/unified-coverage.json";
my $coverage={first_received_timestamp=>undef,last_received_timestamp=>undef};
if (open my $in,"<",$path) {
    local $/;
    $coverage=decode_json(<$in>);
    close $in or die "close coverage input: $!";
}
$coverage->{requested_window_minutes}=0+$minutes;
$coverage->{command_exit_status}=0+$query_rc;
$coverage->{converter_exit_status}=0+$converter_rc;
$coverage->{capture_status}=($query_rc==0 && $converter_rc==0)?"query_completed":"partial_or_unavailable";
$coverage->{present_time_coverage}="not_established";
open my $out,">",$path or die "coverage output: $!";
print $out JSON::PP->new->canonical->pretty->encode($coverage);
close $out or die "close coverage output: $!";
open my $summary,">>","$root/unified-summary.txt" or die "coverage summary: $!";
print $summary "\nRequested window: last ${minutes}min; query exit status: $query_rc.\n";
print $summary "Capture: ".($coverage->{capture_status} eq "query_completed"?
    "query completed without error":"partial or unavailable").".\n";
print $summary "First received timestamp: ".($coverage->{first_received_timestamp}//"unavailable")."\n";
print $summary "Last received timestamp: ".($coverage->{last_received_timestamp}//"unavailable")."\n";
print $summary "Coverage through the present: not established; inspect the received timestamps.\n";
close $summary or die "close coverage summary: $!";
PERL_COVERAGE
    then
        :
    else
        coverage_rc=$?
        printf '%s/unified-coverage\tfailed\t%s\tPerl: received coverage\n' "$rel" "$coverage_rc" >> "$STATUS"
        FAILURE=$((FAILURE + 1))
    fi
    return "$query_rc"
}

PRIMARY_LOG_RC=0
capture_unified_query "$MINUTES" logs || PRIMARY_LOG_RC=$?
if [ "$PRIMARY_LOG_RC" -eq 124 ]; then
    echo '[5b/7] Partial history; independent last-2min attempt in logs/recent/'
    capture_unified_query 2 logs/recent || true
fi

# Historical reports selected by name, without age filtering/sorting/deduplication.
echo '[6/7] Historical reports by name (up to 20 files, up to 10 MB each)'
CRASH_COUNT=0
for crash_root in "$HOME/Library/Logs/DiagnosticReports" /Library/Logs/DiagnosticReports; do
    [ -d "$crash_root" ] || continue
    while IFS= read -r -d '' crash; do
        [ "$CRASH_COUNT" -lt 20 ] || break
        name="$(basename "$crash")"
        if ! printf '%s' "$name" | grep -qiE '(wifi|wireless|airport|wifid|realtek|rtl|rtw|usb|kernel|panic)'; then
            continue
        fi
        nbytes="$(wc -c < "$crash" 2>/dev/null | tr -d ' ')"
        case "$nbytes" in *[!0-9]*|'') continue ;; esac
        [ "$nbytes" -le 10485760 ] || continue
        dest="$RUN_DIR/crashes/$(printf '%02d' "$CRASH_COUNT")-$name"
        if cp "$crash" "$dest" 2>/dev/null; then
            CRASH_COUNT=$((CRASH_COUNT + 1))
        fi
    done < <(find "$crash_root" -maxdepth 1 -type f \( -name '*.ips' -o -name '*.crash' -o -name '*.panic' \) -print0 2>/dev/null)
done
printf '%s\n' "Crash reports copied: $CRASH_COUNT" > "$RUN_DIR/crashes/INDEX.txt"

# This option does not change 'log config' or 'wdutil log'.
if [ "$FULL_UNIFIED" -eq 1 ]; then
    echo '[6b/7] Complete unified-log archive (cannot be anonymized)'
    if [ "$ADMIN_READY" -eq 1 ]; then
        run_admin_capture logs/unified_collection_status.txt log collect --last "${MINUTES}m" --output "$RUN_DIR/logs/unified-complete.logarchive"
    else
        run_capture logs/unified_collection_status.txt log collect --last "${MINUTES}m" --output "$RUN_DIR/logs/unified-complete.logarchive"
    fi
fi

# Second reading: enumeration order may change; compare only by ID.
run_capture drivers/RTW-after-time.txt date -u '+%Y-%m-%dT%H:%M:%SZ'
run_capture drivers/RTW-after.txt ioreg -r -c RTWUSBController -l -w0
if /usr/bin/perl - "$RUN_DIR" 2>"$RUN_DIR/drivers/RTW-summary.stderr.txt" <<'PERL_RTW'
use strict;
use warnings;
use JSON::PP;
my $root=shift;
my $json=JSON::PP->new->canonical->pretty;
my %snapshots;
my %capture_rc;
my @au_numeric=qw(RTWAUBootstrapStage RTWAUBootstrapResult RTWAUBootstrapFirstError
    RTWAUBootstrapFailures RTWAUWarmResetAttempts RTWAUWarmResetStage
    RTWAUWarmResetMCU RTWAUWarmResetResult RTWAU2GHzChannel1PowerResult
    RTWAU5GHzChannel36PowerResult);
my %au_numeric=map {$_=>1} @au_numeric;
my @au_stage_names=qw(RTWAUBootstrapStageName RTWAUWarmResetStageName);
open my $status,"<","$root/STATUS.tsv" or die "capture status: $!";
while (my $line=<$status>) {
    my ($path,$result,$rc)=split /\t/,$line;
    if ($path =~ m{^drivers/RTW-(before|after)\.txt$}) {
        $capture_rc{$1}=($result eq "ok" && $rc eq "0")?0:($rc//-1);
    } elsif ($path eq "usb/ioreg_USBHostInterface.txt") {
        $capture_rc{usb}=($result eq "ok" && $rc eq "0")?0:($rc//-1);
    }
}
close $status;
for my $phase (qw(before after)) {
    open my $in,"<","$root/drivers/RTW-$phase.txt" or die "$phase: $!";
    my (@rows,$row);
    while (my $line=<$in>) {
        if ($line =~ /<class RTWUSBController,\s*id\s+(0x[0-9a-f]+),/i) {
            # IORegistry IDs are 64-bit integers on the target macOS, including values above 2^32.
            {no warnings "portable";$row={registry_id=>hex($1)};}
            push @rows,$row;next;
        }
        next unless $row;
        if ($line =~ /"(idVendor|idProduct)"\s*=\s*(\d+)/) {
            $row->{$1 eq "idVendor"?"vid":"pid"}=0+$2;
        } elsif ($line =~ /"RTWSiliconFamily"\s*=\s*"(RTL[A-Z0-9]+)"/) {
            $row->{family}=$1;
        } elsif ($line =~ /"(RTWUSBDiscoveryStage|RTWAUBootstrapStageName|RTWAUWarmResetStageName)"\s*=\s*"([A-Za-z0-9_.:-]{1,64})"\s*$/) {
            $row->{$1}=$2;
        } elsif ($line =~ /"BSD Name"\s*=\s*"(en\d+)"/) {
            $row->{interface}=$1;
        } elsif ($line =~ /"((?:RTWDiag|Signal)[A-Za-z0-9_]+|IOLinkSpeed|IOLinkStatus|RTWStartResult)"\s*=\s*(-?\d+)\s*$/) {
            $row->{$1}=0+$2;
        } elsif ($line =~ /"(RTWAU[A-Za-z0-9]+)"\s*=\s*(-?\d{1,20})\s*$/ && exists $au_numeric{$1}) {
            # Deliberately exclude factory OSData and every unlisted AU field.
            $row->{$1}=0+$2;
        }
    }
    close $in;
    open my $out,">","$root/drivers/RTW-$phase.json" or die "$phase JSON: $!";
    print $out $json->encode(\@rows);close $out or die "close JSON: $!";
    $snapshots{$phase}={map {$_->{registry_id}=>$_} @rows};
}
# Failed start() providers can outlive the controller that failed to attach.
# Only copy each service's own properties, before its first child header.
my @usb_rows;
my %usb_fields=(idVendor=>"vid",idProduct=>"pid",bConfigurationValue=>"configuration_value",
    bInterfaceNumber=>"interface_number",bAlternateSetting=>"alternate_setting",
    bInterfaceClass=>"interface_class",bInterfaceSubClass=>"interface_subclass",
    bInterfaceProtocol=>"interface_protocol",bNumEndpoints=>"endpoint_count");
if (open my $usb_in,"<","$root/usb/ioreg_USBHostInterface.txt") {
    my $row;
    while (my $line=<$usb_in>) {
        if ($line =~ /^[ |]*\+-o /) {
            $row=undef;
            next unless $line =~ /<class ([^,>]+),\s*id\s+(0x[0-9a-f]+),/i;
            my ($class,$id)=($1,$2);
            if ($class eq "IOUSBHostInterface" || $class eq "RTWUSBController") {
                {no warnings "portable";$row={source_class=>$class,registry_id=>hex($id)};}
                push @usb_rows,$row;
            }
            next;
        }
        next unless $row;
        if ($line =~ /"(RTWStartStage|RTWUSBDiscoveryStage)"\s*=\s*"([A-Za-z0-9_.:-]{1,64})"\s*$/) {
            $row->{$1 eq "RTWStartStage"?"start_stage":"discovery_stage"}=$2;
        } elsif ($line =~ /"([A-Za-z0-9_]{1,80})"\s*=\s*(-?\d{1,20})\s*$/) {
            my ($key,$digits)=($1,$2);
            my $unsigned=$digits;$unsigned=~s/^-//;
            next if length($unsigned)==20 && $unsigned gt "18446744073709551615";
            if (exists $usb_fields{$key}) {
                next if $digits<0 || $digits>65535;
                $row->{$usb_fields{$key}}=0+$digits;
            } elsif ($key eq "RTWStartResult") {
                $row->{start_result}=(0+$digits)&0xffffffff;
            } elsif ($key =~ /^RTWDiagUSBDiscovery[A-Za-z0-9_]+$/) {
                $row->{$key}=0+$digits;
            }
        }
    }
    close $usb_in;
} else {
    $capture_rc{usb}=-1;
}
@usb_rows=grep {exists $_->{start_result} || exists $_->{start_stage} ||
    exists $_->{discovery_stage} || grep {/^RTWDiagUSBDiscovery/} keys %$_} @usb_rows;
open my $usb_json,">","$root/drivers/USB-STARTUP.json" or die "USB startup JSON: $!";
print $usb_json $json->encode(\@usb_rows);close $usb_json or die "close USB startup JSON: $!";
open my $usb_summary,">","$root/USB-STARTUP.txt" or die "USB startup summary: $!";
print $usb_summary "TAHOE WI-FI USB - startup observed on the provider/controller\n";
print $usb_summary "Source: usb/ioreg_USBHostInterface.txt; properties belonging to each Registry ID.\n";
if (($capture_rc{usb}//-1)!=0) {
    print $usb_summary "USB capture unavailable/partial; status=".($capture_rc{usb}//"missing")."; does not establish the absence of an adapter or a failure.\n";
}
print $usb_summary "No RTW startup properties observed; this does not establish the absence of an adapter/failure.\n" unless @usb_rows;
for my $r (@usb_rows) {
    my $vid=exists $r->{vid}?sprintf("%04x",$r->{vid}):"????";
    my $pid=exists $r->{pid}?sprintf("%04x",$r->{pid}):"????";
    print $usb_summary "\n$r->{source_class} $r->{registry_id} | USB $vid:$pid\n";
    for my $k (sort keys %$r) {
        next if $k =~ /^(?:source_class|registry_id|vid|pid)$/;
        if ($k eq "start_result") {printf $usb_summary "start_result=0x%08x\n",$r->{$k};}
        else {print $usb_summary "$k=$r->{$k}\n";}
    }
}
close $usb_summary or die "close USB startup summary: $!";
open my $out,">","$root/RTW-SUMMARY.txt" or die "summary: $!";
print $out "TAHOE WI-FI USB - two readings per Registry ID\n";
print $out "Timestamps in drivers/RTW-before-time.txt and RTW-after-time.txt.\n";
print $out "Counters are cumulative; a missing field does not mean zero.\n";
print $out "IORegistry contains the last published values; this read does not request driver STATUS.\n";
print $out "Delta0 means equal published values, not a proven absence of new errors/traffic.\n";
print $out "Generation/time, when available, belong to the controller; comparison is not atomic.\n";
print $out "WPA2 authorization does not confirm DHCP or Internet access.\n";
print $out "USB startup, including providers without a controller: USB-STARTUP.txt and drivers/USB-STARTUP.json.\n";
my %ids=map {$_=>1} (keys %{$snapshots{before}},keys %{$snapshots{after}});
unless (keys %ids) {
    if (($capture_rc{before}//-1)==0 && ($capture_rc{after}//-1)==0) {
        print $out "No RTWUSBController found; check USB, the loaded kext and STATUS.tsv.\n";
    } else {
        print $out "RTW read unavailable/partial; does not establish the absence of an adapter. See STATUS.tsv.\n";
    }
}
for my $id (sort {$a<=>$b} keys %ids) {
    my $before=$snapshots{before}{$id};my $after=$snapshots{after}{$id};my $r=$after||$before;
    printf $out "\nController %s | %s | USB %04x:%04x | %s\n",$id,
        $r->{family}//"UNKNOWN",$r->{vid}//0,$r->{pid}//0,$r->{interface}//"interface missing";
    my $both_complete=($capture_rc{before}//-1)==0 && ($capture_rc{after}//-1)==0;
    unless ($both_complete) {
        printf $out "Capture unavailable/partial: before status=%s; after status=%s; deltas unavailable.\n",
            $capture_rc{before}//"missing",$capture_rc{after}//"missing";
    }
    if (!$after) {
        print $out (($capture_rc{after}//-1)==0?
            "Removed/missing in the second reading; no cross-device delta.\n":
            "Second reading failed; previous evidence preserved, current state unknown.\n");
        next;
    }
    print $out "New Registry ID; no previous reading for comparison.\n" if !$before && $both_complete;
    my @publication=qw(RTWDiagPublicationGeneration RTWDiagPublicationMonotonicMS);
    my $publication_complete=$both_complete && $before &&
        !grep {!exists $before->{$_} || !exists $after->{$_}} @publication;
    my $publication_unsafe=0;
    if (!$publication_complete) {
        print $out "Publication: freshness unknown; incomplete markers/capture.\n";
    } elsif (($before->{$publication[0]} % 2) || ($after->{$publication[0]} % 2)) {
        print $out "Publication: in progress; comparison unavailable.\n";
        $publication_unsafe=1;
    } elsif ($after->{$publication[0]}<$before->{$publication[0]} ||
             $after->{$publication[1]}<$before->{$publication[1]}) {
        print $out "Publication: generation/time regressed; comparison unavailable.\n";
        $publication_unsafe=1;
    } elsif ($after->{$publication[0]}==$before->{$publication[0]}) {
        print $out "Publication: no new publication observed; generation=$after->{$publication[0]}.\n";
    } else {
        print $out "Publication: generation advanced $before->{$publication[0]} -> $after->{$publication[0]}.\n";
    }
    for my $k (@publication) {print $out "$k=$after->{$k}\n" if exists $after->{$k};}
    for my $k (qw(RTWDiagCurrentChannel RTWDiagWPAPortValid RTWDiagWPAState RTWDiagRXRecoveryStage IOLinkStatus IOLinkSpeed
        RTWDiagHTNegotiated RTWDiagVHTNegotiated RTWDiagHTChannelWidthMHz RTWDiagHTTXStreams RTWDiagHTTXBATIDs RTWDiagHTRXBATIDs
        RTWDiagLinkSpeedKnown RTWDiagLinkSpeedBps RTWDiagNativeIfnetIndex RTWDiagNativeIfnetStatsValid RTWDiagNativeIfnetStatsError)) {
        print $out "$k=$after->{$k}\n" if exists $after->{$k};
    }
    for my $k (qw(RTWDiagRXRecoveryLastError RTWDiagRXRecoveryOriginalError)) {
        printf $out "%s=0x%08x\n",$k,$after->{$k}&0xffffffff if exists $after->{$k};
    }
    print $out "RTWUSBDiscoveryStage=$after->{RTWUSBDiscoveryStage}\n" if exists $after->{RTWUSBDiscoveryStage};
    for my $k (sort grep {/^RTWDiagUSBDiscovery/} keys %$after) {print $out "$k=$after->{$k}\n";}
    for my $k (@au_numeric,@au_stage_names) {print $out "$k=$after->{$k}\n" if exists $after->{$k};}
    # These are cumulative before/after values around ONE sampled input batch,
    # not counters spanning the collector's capture interval. Export as gauges.
    for my $k (sort grep {/^RTWDiagRXNative/ || /^RTWDiagRXInputInterfaceMatchesChild$/} keys %$after) {
        print $out "$k=$after->{$k}\n";
    }
    for my $k (qw(RTWDiagRXRecoveryErrors RTWDiagRXRecoveryAttempts RTWDiagRXRecoveryFailures RTWDiagRXRecoveryCompleted RTWDiagRXRecoveryExhausted RTWDiagTXUSBTransferErrors RTWDiagTXQueueDrops RTWDiagRXAggregateErrors RTWDiagRXAggregateTruncated RTWDiagRXAggregateMalformed RTWDiagRXAggregateTooMany RTWDiagRXAggregateBadArgument RTWDiagRXBytes RTWDiagRXAggregates RTWDiagRXEthernetQueued
        RTWDiagNet80211InputErrors RTWDiagNet80211RXBABelowWindow RTWDiagNet80211RXBAAboveWindow
        RTWDiagNet80211RXBAWindowSlides RTWDiagNet80211RXBAWindowJumps RTWDiagNet80211RXBANoBuffer
        RTWDiagNet80211RXBAFramesLost RTWDiagNet80211RXBAWindowGapTimeouts RTWDiagNet80211RXBATimeouts
        RTWDiagNet80211TXBATimeouts RTWDiagHTTXAMPDURequested
        RTWDiagRXInputCalls RTWDiagRXInputOfferedPackets RTWDiagRXInputOfferedBytes
        RTWDiagRXInputImmediateReturns RTWDiagRXInputFlushCalls RTWDiagRXInputFlushPackets
        RTWDiagRXInputSubmissionMismatches RTWDiagRXInputNoIfnetCalls
        RTWDiagNativeIfnetInputPackets RTWDiagNativeIfnetInputBytes RTWDiagNativeIfnetInputErrors
        RTWDiagNativeIfnetOutputPackets RTWDiagNativeIfnetOutputBytes RTWDiagNativeIfnetOutputErrors)) {
        next unless exists $after->{$k};
        if ($k =~ /^RTWDiagNativeIfnet(?:Input|Output)/ &&
            (!$before || ($after->{RTWDiagNativeIfnetStatsValid}//0)!=1 ||
             ($before->{RTWDiagNativeIfnetStatsValid}//0)!=1 ||
             !($after->{RTWDiagNativeIfnetIndex}//0) ||
             ($after->{RTWDiagNativeIfnetIndex}//0)!=($before->{RTWDiagNativeIfnetIndex}//0))) {
            print $out "$k native read unavailable/incomparable; see StatsValid/Index.\n";next;
        }
        if (!$both_complete) {print $out "$k observed=$after->{$k}; delta unavailable because the capture is partial\n";next;}
        if ($publication_unsafe) {print $out "$k observed=$after->{$k}; delta unavailable because publication is in progress/regressed\n";next;}
        if (!$before || !exists $before->{$k}) {print $out "$k current=$after->{$k}; previous missing\n";next;}
        if ($after->{$k}<$before->{$k}) {print $out "$k before=$before->{$k} after=$after->{$k}; reset/restarted, delta unavailable\n";next;}
        print $out "$k before=$before->{$k} after=$after->{$k} delta=".($after->{$k}-$before->{$k})."\n";
    }
    print $out "AU calibration timing: not instrumented; RTWDiagCalib*=0 does not measure the absence of delay.\n" if ($r->{family}//"") =~ /AU$/;
}
close $out or die "close summary: $!";
PERL_RTW
then
    printf 'drivers/RTW-summary\tok\t0\tPerl: RTW snapshots by Registry ID\n' >> "$STATUS"
    SUCCESS=$((SUCCESS + 1))
else
    rc=$?
    printf 'drivers/RTW-summary\tfailed\t%s\tPerl: RTW snapshots by Registry ID\n' "$rc" >> "$STATUS"
    FAILURE=$((FAILURE + 1))
fi

cat > "$RUN_DIR/README-DIAGNOSTICS.txt" <<EOM
TAHOEREALTEKWIFI — DIAGNOSTIC CAPTURE
Collector revision: ${COLLECTOR_REVISION}
Run date: $(date '+%Y-%m-%d %H:%M:%S %z')
Event window: last ${MINUTES} minutes (existing macOS logs)
Per-command deadline: ${COMMAND_TIMEOUT}s; status124 means timeout, not success.
Mode: $(if [ "$RAW" -eq 1 ]; then echo 'RAW / NOT ANONYMIZED'; else echo 'basic masking of sensitive lines'; fi)
Administrator: $(if [ "$ADMIN_READY" -eq 1 ]; then echo 'read-only diagnostic commands'; else echo 'no'; fi)

READ FIRST:
  RTW-SUMMARY.txt - family/VID:PID/interface, USB error and per-adapter differences.
  drivers/RTW-before/after.txt and .json - each controller's properties and counters.
  USB-STARTUP.txt / drivers/USB-STARTUP.json - startup, including providers without a controller.
  drivers/loaded-RTW-kext.txt / installed-app-Info.txt - versions actually in use.
  USB-IDENTIFICATION.txt - all USB devices with VID:PID extracted from IORegistry.
  USB-WIFI-CANDIDATES.txt - possible matches, including rebranded devices.
  usb/SPUSBHostDataType.txt or SPUSBDataType.txt - USB tree and per-device IDs.
  usb/ioreg_USB_tree.txt - USB topology, vendor and providers/drivers.
  usb/ioreg_USBHostInterface.txt - USB interfaces and their drivers.
  drivers/kmutil_showloaded.txt / systemextensionsctl.txt - active drivers.
  network/hardware_ports.txt / ifconfig_all.txt - interface and status.
  logs/USB_Realtek.log / WiFi_80211.log / Network_Ethernet.log - events.
  logs/unified-summary.txt - lines retained and discarded because of limits.
  logs/unified-coverage.json - requested window, return code and received timestamps.
  logs/recent/ - independent 2min query, only after main-query timeout124.
  STATUS.tsv - details of failed or blocked commands.

LOGS: the main query requests ${MINUTES}min. If it times out, partial output is
preserved and ONE additional 2min query is attempted, with the same deadline of
${COMMAND_TIMEOUT}s per query. logs/recent/ does not complete the partial history.
First/last timestamps describe only events actually received. If unavailable,
they do not establish the absence of events. Even when data is received,
coverage through the present is not guaranteed; inspect the timestamps.
Gatekeeper disabled: the exact 'assessments disabled'/exit1 response is an
expected state; STATUS preserves rc1 without counting a failure. No settings change.
CRASHES: historical reports selected by name, without age filtering, sorting or
deduplication. The file count does not equal new events or guarantee a relationship
to the requested unified-log window.

FRESHNESS: IORegistry snapshots contain the last values published by the driver.
STATUS is not forced for adapters; delta0 does not prove the absence of new errors
or traffic. Build54 publishes per-controller generation/monotonic time; an odd
generation means publication is in progress. Without markers, freshness is unknown.
IORegistry reads are not atomic; the markers do not replace real traffic tests.

IMPORTANT: VID/PID, hardware revision and driver binding identify the hardware
macOS DETECTS. Definitive identification of the internal silicon, correct firmware
and an operational driver may require physical inspection or a safe chipset read
using a dedicated tool.

PRIVACY:
Basic masking may miss identifiers, paths, network names, IPs, tokens or personal
data in logs; REVIEW before sharing. RAW logs have no anonymization. This program
does not send reports to servers or change driver/security settings.

EOM

# Mask by default while preserving VID/PID, error codes, register addresses,
# versions, endpoints and part of the MAC OUI. This is not a cryptographic
# guarantee of anonymity, especially in arbitrary log text.
echo '[7/7] Privacy, integrity and ZIP'
if [ "$RAW" -eq 0 ]; then
    while IFS= read -r -d '' file; do
        case "$file" in
            *.txt|*.log|*.json|*.tsv|*.ips|*.crash|*.panic) ;;
            *) continue ;;
        esac
        if command -v perl >/dev/null 2>&1; then
            if perl -pe '
                s{((?:IOMACAddress|local-mac-address|MAC Address)"?\s*=\s*)<[^>]*>}{${1}<[REDACTED]>}gi;
                s{(/Users/)[^/\s"\x27]+}{${1}[USER]}gi;
                s{\b([A-Z0-9._%+\x2d]+)\@([A-Z0-9.\x2d]+\.[A-Z]{2,})\b}{[EMAIL]}gi;
                s{\b([0-9a-f]{2}(?::[0-9a-f]{2}){2}):(?:[0-9a-f]{2}:){2}[0-9a-f]{2}\b}{$1:XX:XX:XX}gi;
                s{\b(192\.168\.\d+)\.\d+\b}{$1.X}g;
                s{\b(10\.\d+\.\d+)\.\d+\b}{$1.X}g;
                s{\b(172\.(?:1[6-9]|2[0-9]|3[01])\.\d+)\.\d+\b}{$1.X}g;
                s{(\b(?:SSID|BSSID|IOPlatformSerialNumber|IOPlatformUUID|Hardware UUID|Computer Name|Local Host Name|LocalHostName|Host Name|HostName|Serial Number(?:\s*\([^)]*\))?|USB Serial Number|SerialNumber|serial_num|serial_number|serial-number|WiFiPassword|Passphrase|WPA-PSK|PreSharedKey|Password)"?\s*(?:=|:)\s*)"[^"]*"}{${1}"[REDACTED]"}gi;
                s{(\b(?:SSID|BSSID|IOPlatformSerialNumber|IOPlatformUUID|Hardware UUID|Computer Name|Local Host Name|LocalHostName|Host Name|HostName|Serial Number(?:\s*\([^)]*\))?|USB Serial Number|SerialNumber|serial_num|serial_number|serial-number|WiFiPassword|Passphrase|WPA-PSK|PreSharedKey|Password)"?\s*(?:=|:)\s*)(?!")([^;,\r\n]+)}{${1}[REDACTED]}gi;
            ' "$file" > "$file.redacting"; then
                mv -f "$file.redacting" "$file"
            else
                rm -f "$file.redacting"
                echo "ANONYMIZATION FAILED: $file" >&2
                echo "ANONYMIZATION FAILED: $file" >> "$RUN_DIR/ANONYMIZATION-ERROR.txt"
            fi
        else
            echo 'Perl was not found. Packaging non-anonymized data is unsafe.' >&2
            echo 'Use --raw to allow collection without anonymization after considering privacy.' >&2
            exit 1
        fi
    done < <(find "$RUN_DIR" -type f -print0)
    if [ -f "$RUN_DIR/ANONYMIZATION-ERROR.txt" ]; then
        echo 'One or more anonymization attempts failed. The ZIP will not be created.' >&2
        exit 1
    fi
fi

cat > "$RUN_DIR/RESULT.txt" <<EOM
TAHOEREALTEKWIFI — COLLECTION RESULT
Collector revision: ${COLLECTOR_REVISION}
Date: $(date '+%Y-%m-%d %H:%M:%S %z')
Log window: ${MINUTES} minutes
Successful commands: $SUCCESS
Unavailable / denied / failed commands: $FAILURE
Selected historical crash files: $CRASH_COUNT
Mode: $(if [ "$RAW" -eq 1 ]; then echo 'RAW (SENSITIVE)'; else echo 'basic masking'; fi)

Driver readings:
  RTW-SUMMARY.txt separates the adapters and records drops/resets.
  USB-STARTUP.txt records USB provider/controller stages and errors.
  STATUS.tsv records failures, including timeout124 and denied log access.
  Gatekeeper: expected_state/exit1 preserves assessments disabled without counting an error.
  logs/unified-coverage.json records the window, return code and received timestamps.
  logs/recent/ is a separate 2min attempt after timeout; history remains partial.

USB identification:
  1. USB-IDENTIFICATION.txt
  2. usb/SPUSBHostDataType.txt or usb/SPUSBDataType.txt
  3. usb/ioreg_USB_tree.txt
  4. usb/ioreg_USBHostInterface.txt

NOTE: no messages does not establish compatible hardware or a working driver.
If a command failed, see STATUS.tsv and the corresponding file's error output.
Email, SSID, passwords, serials and MAC addresses are partially masked by default.
Review files before sharing; masking is not infallible.

EOM

# Hash exported files to detect corruption when sharing.
if command -v shasum >/dev/null 2>&1; then
    (
        cd "$RUN_DIR" || exit 1
        find . -type f ! -name 'SHA256SUMS.txt' -print0 | while IFS= read -r -d '' f; do
            shasum -a 256 "$f"
        done
    ) > "$RUN_DIR/SHA256SUMS.txt"
fi
chmod -R go-rwx "$RUN_DIR" 2>/dev/null || true

# Native macOS 'ditto' preserves the directory; fall back to 'zip' if necessary.
if command -v ditto >/dev/null 2>&1; then
    if ! ditto -c -k --keepParent "$RUN_DIR" "$ZIP_PATH"; then
        rm -f "$ZIP_PATH"
    fi
fi
if [ ! -s "$ZIP_PATH" ]; then
    if command -v zip >/dev/null 2>&1; then
        (cd "$OUTPUT_BASE" && zip -q -r "$ZIP_PATH" "$(basename "$RUN_DIR")") || true
    fi
fi
if [ ! -s "$ZIP_PATH" ]; then
    echo 'Could not create the ZIP; the directory containing all reports was preserved:' >&2
    echo "$RUN_DIR" >&2
    exit 1
fi

printf '\n==============================================\n'
printf ' DIAGNOSTICS COMPLETE\n'
printf '==============================================\n'
printf 'ZIP TO ATTACH TO THIS CHAT:\n%s\n' "$ZIP_PATH"
printf '\nDirectory for inspection:\n%s\n' "$RUN_DIR"
printf '\nReview the ZIP before sharing (logs may contain private data).\n'
printf 'No driver, SIP or network setting was changed.\n\n'
if [ "$NO_PAUSE" -eq 0 ] && [ -t 0 ]; then
    printf 'Press Enter to close this window... '
    IFS= read -r ignored || true
fi
exit 0
