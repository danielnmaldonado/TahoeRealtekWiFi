#!/bin/bash
# TahoeRealtekWiFi — coletor de diagnósticos somente leitura para macOS Tahoe / Intel.
# Compatível com o Bash 3.2 fornecido pelo macOS; sem Homebrew, Python ou kext.
# Não instala, não carrega drivers e não altera SIP/AMFI ou a configuração de rede.
set -u
set -o pipefail
umask 077

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd -P)"
COLLECTOR_REVISION=2
MINUTES=30
INCLUDE_DEBUG=0
RAW=0
ADMIN=0
FULL_UNIFIED=0
OUTPUT_BASE="$HOME/Library/Logs/Tahoe-WiFi-USB/Diagnostics"
COMMAND_TIMEOUT=30
NO_PAUSE=0

usage() {
    printf 'Revisão do coletor: %s\n\n' "$COLLECTOR_REVISION"
    cat <<'HELP'
TahoeRealtekWiFi — WiFi-Diagnostics.command

Duplo clique: coleta detalhada dos últimos 30 minutos, com mascaramento básico de dados
pessoais. Salva uma pasta e um ZIP em ~/Library/Logs/Tahoe-WiFi-USB/Diagnostics/.
Inclui duas leituras por adaptador e diferenças dos contadores USB/RX/TX.
Se a consulta principal de logs exceder o prazo, tenta uma janela recente de 2min.

Opções no Terminal:
  --minutes N       Janela de logs: 30 ou 60 minutos (padrão: 30)
  --hours 1         Atalho compatível: seleciona 60 minutos
  --debug           Inclui mensagens DEBUG na consulta (padrão: INFO)
  --admin           Usa sudo SOMENTE em leituras de logs e 'wdutil info'
  --raw             Não mascara dados pessoais: compartilhe apenas em privado
  --full-unified    Inclui o logarchive COMPLETO da janela (exige --raw);
                    pode ser grande e conter dados sensíveis
  --output-dir DIR  Destino personalizado para a pasta e ZIP
  --command-timeout N  Limite de cada comando: 1–120 segundos (padrão: 30)
  --no-pause        Não aguarda Enter ao finalizar
  --help            Mostra esta ajuda

Nunca solicita sua senha de Wi-Fi. Sem --admin não solicita senha de macOS.
Não envia dados pela internet; é necessário anexar o ZIP manualmente ao chat.
HELP
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --minutes)
            if [ "$#" -lt 2 ]; then echo "Falta valor após --minutes" >&2; exit 2; fi
            MINUTES="$2"; shift 2 ;;
        --hours)
            if [ "$#" -lt 2 ]; then echo "Falta valor após --hours" >&2; exit 2; fi
            if [ "$2" != "1" ]; then echo "--hours aceita somente 1 (60 minutos)." >&2; exit 2; fi
            MINUTES=60; shift 2 ;;
        --debug) INCLUDE_DEBUG=1; shift ;;
        --admin) ADMIN=1; shift ;;
        --raw) RAW=1; shift ;;
        --full-unified) FULL_UNIFIED=1; shift ;;
        --output-dir)
            if [ "$#" -lt 2 ]; then echo "Falta diretório após --output-dir" >&2; exit 2; fi
            OUTPUT_BASE="$2"; shift 2 ;;
        --command-timeout)
            if [ "$#" -lt 2 ]; then echo "Falta valor após --command-timeout" >&2; exit 2; fi
            COMMAND_TIMEOUT="$2"; shift 2 ;;
        --no-pause) NO_PAUSE=1; shift ;;
        --help|-h) usage; exit 0 ;;
        *) echo "Opção desconhecida: $1" >&2; usage >&2; exit 2 ;;
    esac
done

case "$MINUTES" in
    30|60) ;;
    *) echo "--minutes aceita somente 30 ou 60 minutos." >&2; exit 2 ;;
esac
case "$COMMAND_TIMEOUT" in
    ''|*[!0-9]*|????*) echo "--command-timeout aceita inteiros de 1 a 120." >&2; exit 2 ;;
esac
# Prefixo decimal evita interpretação octal de valores como 08 no Bash3.2.
COMMAND_TIMEOUT=$((10#$COMMAND_TIMEOUT))
if [ "$COMMAND_TIMEOUT" -lt 1 ] || [ "$COMMAND_TIMEOUT" -gt 120 ]; then
    echo "--command-timeout aceita inteiros de 1 a 120." >&2; exit 2
fi
if [ "$FULL_UNIFIED" -eq 1 ] && [ "$RAW" -ne 1 ]; then
    echo "--full-unified exige --raw: arquivos logarchive não podem ser mascarados." >&2
    exit 2
fi
if ! /usr/bin/perl -MJSON::PP -MPOSIX -MTime::HiRes -e 1 >/dev/null 2>&1; then
    echo "Este Mac precisa do Perl nativo e seus módulos padrão para coletar. Nada coletado." >&2
    exit 1
fi
if [ "$(uname -s)" != "Darwin" ]; then
    echo "Este .command deve ser executado diretamente no macOS." >&2
    exit 1
fi

if ! mkdir -p "$OUTPUT_BASE"; then
    echo "Sem permissão de escrita em: $OUTPUT_BASE" >&2
    exit 1
fi
OUTPUT_BASE="$(cd "$OUTPUT_BASE" && pwd -P)"
STAMP="$(date '+%Y%m%d-%H%M%S')"
RUN_DIR="$(mktemp -d "$OUTPUT_BASE/WiFi-Diagnostics-${STAMP}-XXXXXX")" || exit 1
ZIP_PATH="$RUN_DIR.zip"
mkdir -p "$RUN_DIR/system" "$RUN_DIR/usb" "$RUN_DIR/network" \
         "$RUN_DIR/drivers" "$RUN_DIR/logs" "$RUN_DIR/crashes"
STATUS="$RUN_DIR/STATUS.tsv"
printf 'arquivo\tresultado\tstatus_saida\tcomando\n' > "$STATUS"
SUCCESS=0
FAILURE=0

# Cada comando tem um grupo próprio. O coletor só termina seus filhos,
# incluindo descendentes; o PID permanece reservado até waitpid concluir.
# O relógio monotônico independe de ajustes de data/NTP durante a coleta.
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
    # O monitor também precisa ser root para poder encerrar seus próprios filhos.
    sudo -n /usr/bin/perl -e "$BOUNDED_PERL" "$COMMAND_TIMEOUT" "$@"
}
trap 'exit 130' INT
trap 'exit 143' TERM

# Uma falha pontual deve constar no pacote, sem interromper os demais exames.
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
        printf '%s\tfalhou\t%s\t%s\n' "$rel" "$rc" "$quoted" >> "$STATUS"
        FAILURE=$((FAILURE + 1))
    fi
}

run_admin_capture() {
    local rel="$1" rc quoted="" a
    shift
    printf '  • %s (leitura administrativa)\n' "$rel"
    for a in "$@"; do quoted="$quoted $(printf '%q' "$a")"; done
    if run_admin_bounded "$@" > "$RUN_DIR/$rel" 2>&1; then
        printf '%s\tok\t0\tsudo%s\n' "$rel" "$quoted" >> "$STATUS"
        SUCCESS=$((SUCCESS + 1))
    else
        rc=$?
        printf '%s\tfalhou\t%s\tsudo%s\n' "$rel" "$rc" "$quoted" >> "$STATUS"
        FAILURE=$((FAILURE + 1))
    fi
}

# spctl usa exit1 para o estado desabilitado. Aceitar somente essa resposta
# exata; mensagens adicionais, outros erros ou códigos continuam sendo falha.
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
            printf 'system/gatekeeper.txt\testado_esperado\t1\tspctl --status\n' >> "$STATUS"
            SUCCESS=$((SUCCESS + 1))
        else
            printf 'system/gatekeeper.txt\tfalhou\t%s\tspctl --status\n' "$rc" >> "$STATUS"
            FAILURE=$((FAILURE + 1))
        fi
    fi
}

cat <<EOM

========================================================
 TahoeRealtekWiFi — diagnóstico USB / Wi-Fi do macOS
========================================================
Revisão do coletor: ${COLLECTOR_REVISION}
Janela dos logs: ${MINUTES} minutos
Privacidade: $(if [ "$RAW" -eq 1 ]; then echo 'dados originais (SENSÍVEL)'; else echo 'mascaramento básico'; fi)
Coleta: somente leitura de sistema; cria apenas arquivos de relatório.
Destino: $RUN_DIR

EOM

ADMIN_READY=0
if [ "$ADMIN" -eq 1 ]; then
    echo "--admin solicitado: sudo será usado apenas para ler diagnósticos."
    if sudo -v; then ADMIN_READY=1; else echo "Sem sudo: prosseguindo sem privilégios adicionais."; fi
fi

# Capturar cedo mantém evidência do adaptador falho antes dos comandos lentos.
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

# Inventário de sistema: sem exportação de NVRAM completo, senhas ou perfis Wi-Fi.
echo '[1/7] Sistema e estado de segurança'
run_capture system/macOS.txt sw_vers
run_capture system/kernel.txt uname -sr
run_capture system/architecture.txt uname -m
run_capture system/sip.txt csrutil status
capture_gatekeeper_state
run_capture system/boot_args.txt nvram boot-args
run_capture system/hardware.txt system_profiler SPHardwareDataType
run_capture system/software.txt system_profiler SPSoftwareDataType
run_capture system/power.txt pmset -g

# No macOS Tahoe 26 o nome passou de SPUSBDataType para SPUSBHostDataType.
echo '[2/7] Inventário USB completo: VID:PID, portas, interfaces e endpoints'
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
    # Fallback caso a enumeração de tipos falhe ou varie entre builds.
    run_capture usb/SPUSBHostDataType.txt system_profiler -detailLevel full SPUSBHostDataType
    run_capture usb/SPUSBDataType.txt system_profiler -detailLevel full SPUSBDataType
fi
run_capture usb/ioreg_USB_tree.txt ioreg -p IOUSB -l -w0
run_capture usb/ioreg_USBHostDevice.txt ioreg -r -c IOUSBHostDevice -l -w0
run_capture usb/ioreg_USBHostInterface.txt ioreg -r -c IOUSBHostInterface -l -w0
run_capture usb/ioreg_USBHostPipe.txt ioreg -r -c IOUSBHostPipe -l -w0
run_capture usb/ioreg_legacy_USBDevice.txt ioreg -r -c IOUSBDevice -l -w0

# Enumerar TODOS os USB: adaptadores rebatizados podem não usar VID 0BDA.
# Se o IORegistry não revelar os dois IDs, os relatórios completos são a fonte.
{
    echo 'DISPOSITIVOS USB ENCONTRADOS (nome e IDs lidos de IOUSBHostDevice)'
    echo 'Atenção: VID:PID identifica o dispositivo USB, mas não garante o chip interno.'
    echo
    if command -v perl >/dev/null 2>&1; then
        perl - "$RUN_DIR/usb/ioreg_USBHostDevice.txt" <<'PERL_USB'
use strict;
use warnings;
my $source = shift;
open my $fh, '<', $source or die "Não consegui ler $source: $!\n";
my ($name, $vid, $pid, $revision, $manufacturer, $product);
my $emit = sub {
    return unless defined($vid) && defined($pid);
    my $fmt = sub {
        my $v=shift;
        return $v =~ /^0x/i ? hex($v) : int($v);
    };
    my ($v, $p) = ($fmt->($vid), $fmt->($pid));
    return if $v > 65535 || $p > 65535;
    my $n = defined($product) ? $product : (defined($name) ? $name : '(sem nome)');
    $n =~ s/[\r\n\t]+/ /g;
    printf "VID:PID %04X:%04X | %s", $v, $p, $n;
    print " | fabricante: $manufacturer" if defined($manufacturer);
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
        echo 'Perl indisponível; confira os arquivos ioreg_USB*.txt.'
    fi
} > "$RUN_DIR/USB-IDENTIFICATION.txt" 2>&1
if ! grep -q '^VID:PID ' "$RUN_DIR/USB-IDENTIFICATION.txt"; then
    printf '\nNenhum VID:PID extraído automaticamente. Consulte as saídas integrais de ioreg e system_profiler em usb/.\n' >> "$RUN_DIR/USB-IDENTIFICATION.txt"
fi

{
    echo 'POSSÍVEIS ADAPTADORES WI-FI USB (extratos, não lista exaustiva)'
    echo 'Para confirmação, conferir o inventário USB INTEGRAL do pacote.'
    echo
    for f in "$RUN_DIR"/usb/SPUSB*txt "$RUN_DIR"/usb/ioreg_USB_tree.txt; do
        [ -f "$f" ] || continue
        echo "--- $(basename "$f") ---"
        grep -niE -C 10 'Realtek|rtl8[0-9]|rtwlan|wlan|802\.11|wireless|wi-?fi|0x0bda|0x2357|0x7392|0x0b05|0x2001' "$f" || true
    done
} > "$RUN_DIR/USB-WIFI-CANDIDATES.txt"

# Redes ativas e como o macOS associa serviços a interfaces en0, en1, etc.
echo '[3/7] Rede, Wi-Fi nativo e interfaces Ethernet'
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

# Drivers em execução e os dispositivos que os serviços IOKit reivindicaram.
echo '[4/7] Kexts, System Extensions e matching de drivers'
run_capture drivers/kmutil_showloaded.txt kmutil showloaded
run_capture drivers/kextstat.txt kextstat -l
run_capture drivers/systemextensionsctl.txt systemextensionsctl list
run_capture drivers/ethernet_controllers.txt ioreg -r -c IOEthernetController -l -w0
run_capture drivers/network_interfaces.txt ioreg -r -c IONetworkInterface -l -w0
run_capture drivers/installed_kexts.txt ls -la /Library/Extensions
run_capture drivers/installed_systemextensions.txt ls -la /Library/SystemExtensions

# Uma consulta principal ao log unificado; apenas timeout124 permite uma
# segunda consulta independente dos ultimos 2min. Historico parcial e preservado.
# O filtro prioriza USB e mensagens de Wi-Fi/Ethernet; evita coletar todos os
# eventos de configd/networkd por 24h. INFO e mensagens padrao por default;
# DEBUG somente com --debug para nao inflar os arquivos desnecessariamente.
echo "[5/7] Logs unificados (${MINUTES} minutos, consulta principal; DEBUG=$(if [ "$INCLUDE_DEBUG" -eq 1 ]; then echo sim; else echo nao; fi))"
USB_PRED='(subsystem CONTAINS[c] "usb" OR eventMessage CONTAINS[c] "Realtek" OR eventMessage CONTAINS[c] "RtWlan" OR eventMessage CONTAINS[c] "rtl8192" OR eventMessage CONTAINS[c] "2357" OR eventMessage CONTAINS[c] "TahoeRealtekWiFi")'
WIFI_PRED='(subsystem CONTAINS[c] "wifi" OR subsystem CONTAINS[c] "80211" OR eventMessage CONTAINS[c] "AirPort" OR eventMessage CONTAINS[c] "itlwm")'
NET_PRED='(eventMessage CONTAINS[c] "IOEthernet" OR eventMessage CONTAINS[c] "IONetwork" OR subsystem CONTAINS[c] "network")'
UNIFIED_PRED="($USB_PRED OR $WIFI_PRED OR $NET_PRED)"
# Cada consulta usa o mesmo conversor: ate 6MiB por categoria, mantendo as
# ultimas linhas RECEBIDAS. Um timeout pode parar antes de chegar ao presente.
# A cobertura registra os timestamps efetivamente recebidos, inclusive quando
# nenhuma linha tem timestamp reconhecivel; isso nao confirma ausencia de eventos.
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
    print $summary "Consulta de logs unificados; janela solicitada: ultimos ${minutes}min.\n";
    print $summary "Limite de conteudo por categoria: $max bytes, mantendo as ultimas mensagens RECEBIDAS.\n";
    for my $n (@names) {
        open my $fh, ">", "$outdir/$n.log" or die "$n: $!";
        print $fh "# $n: mensagens recebidas nesta consulta, eventualmente truncadas por tamanho.\n";
        print $fh "# Mensagens antigas descartadas pelo limite: $dropped{$n}\n";
        if (!@{$queue{$n}}) {
            print $fh "# Nenhuma linha correspondente recebida nesta consulta; confira cobertura e status.\n";
        } else {
            print $fh $_ for @{$queue{$n}};
        }
        close $fh or die "close $n: $!";
        printf $summary "%s: identificadas=%d; retidas=%d; antigas_descartadas=%d; bytes_retidos=%d\n",
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
        printf '%s/unified-onepass\tfalhou\t125\tnao foi possivel criar destino\n' "$rel" >> "$STATUS"
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
    # O grupo do log e encerrado no prazo; EOF permite guardar a saida parcial.
    # Guardar os dois status imediatamente preserva o rc real da consulta,
    # mesmo se o conversor tambem falhar.
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
        printf '%s/unified-onepass\tfalhou\t%s\tlog show --last %sm\n' "$rel" "$query_rc" "$minutes" >> "$STATUS"
        FAILURE=$((FAILURE + 1))
        echo "[AVISO] Consulta de ${minutes}min falhou ($query_rc): confira $rel/log-show.stderr.txt" >&2
    fi
    if [ "$converter_rc" -ne 0 ]; then
        printf '%s/unified-converter\tfalhou\t%s\tPerl: categorias recebidas\n' "$rel" "$converter_rc" >> "$STATUS"
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
print $summary "\nJanela solicitada: ultimos ${minutes}min; status de saida da consulta: $query_rc.\n";
print $summary "Captura: ".($coverage->{capture_status} eq "query_completed"?
    "consulta encerrada sem erro":"parcial ou indisponivel").".\n";
print $summary "Primeiro timestamp recebido: ".($coverage->{first_received_timestamp}//"indisponivel")."\n";
print $summary "Ultimo timestamp recebido: ".($coverage->{last_received_timestamp}//"indisponivel")."\n";
print $summary "Cobertura ate o presente: nao estabelecida; consulte os timestamps recebidos.\n";
close $summary or die "close coverage summary: $!";
PERL_COVERAGE
    then
        :
    else
        coverage_rc=$?
        printf '%s/unified-coverage\tfalhou\t%s\tPerl: cobertura recebida\n' "$rel" "$coverage_rc" >> "$STATUS"
        FAILURE=$((FAILURE + 1))
    fi
    return "$query_rc"
}

PRIMARY_LOG_RC=0
capture_unified_query "$MINUTES" logs || PRIMARY_LOG_RC=$?
if [ "$PRIMARY_LOG_RC" -eq 124 ]; then
    echo '[5b/7] Historico parcial; tentativa independente dos ultimos 2min em logs/recent/'
    capture_unified_query 2 logs/recent || true
fi

# Relatórios históricos por nome, sem filtro de idade/ordem/dedup.
echo '[6/7] Relatórios históricos por nome (até 20 arquivos, até 10 MB cada)'
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
printf '%s\n' "Relatórios de falha copiados: $CRASH_COUNT" > "$RUN_DIR/crashes/INDEX.txt"

# Esta opção NÃO executa alterações em 'log config' nem 'wdutil log'.
if [ "$FULL_UNIFIED" -eq 1 ]; then
    echo '[6b/7] Arquivo completo de logs unificados (não pode ser anonimizado)'
    if [ "$ADMIN_READY" -eq 1 ]; then
        run_admin_capture logs/unified_collection_status.txt log collect --last "${MINUTES}m" --output "$RUN_DIR/logs/unified-complete.logarchive"
    else
        run_capture logs/unified_collection_status.txt log collect --last "${MINUTES}m" --output "$RUN_DIR/logs/unified-complete.logarchive"
    fi
fi

# Segunda leitura: a ordem de enumeração pode mudar; comparar somente por ID.
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
            # IORegistry IDs são inteiros64 no macOS alvo, inclusive acima de2^32.
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
print $usb_summary "TAHOE WI-FI USB - inicializacao observada no provider/controlador\n";
print $usb_summary "Fonte: usb/ioreg_USBHostInterface.txt; propriedades proprias de cada Registry ID.\n";
if (($capture_rc{usb}//-1)!=0) {
    print $usb_summary "Captura USB indisponivel/parcial; status=".($capture_rc{usb}//"ausente")."; nao conclui ausencia de adaptador ou de falha.\n";
}
print $usb_summary "Sem propriedades RTW de inicializacao observadas; isto nao confirma ausencia de adaptador/falha.\n" unless @usb_rows;
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
print $out "TAHOE WI-FI USB - duas leituras por Registry ID\n";
print $out "Horários em drivers/RTW-before-time.txt e RTW-after-time.txt.\n";
print $out "Contadores são cumulativos; ausência de campo não significa zero.\n";
print $out "IORegistry contem os ultimos valores publicados; esta leitura nao solicita STATUS ao driver.\n";
print $out "Delta0 significa valores publicados iguais, nao ausencia comprovada de erros/trafego novos.\n";
print $out "Geracao/tempo, quando presentes, pertencem ao controlador; comparacao nao atomica.\n";
print $out "WPA2 autorizado não confirma DHCP nem acesso à internet.\n";
print $out "Inicializacao USB, inclusive sem controlador: USB-STARTUP.txt e drivers/USB-STARTUP.json.\n";
my %ids=map {$_=>1} (keys %{$snapshots{before}},keys %{$snapshots{after}});
unless (keys %ids) {
    if (($capture_rc{before}//-1)==0 && ($capture_rc{after}//-1)==0) {
        print $out "Nenhum RTWUSBController encontrado; confira USB, kext carregada e STATUS.tsv.\n";
    } else {
        print $out "Leitura RTW indisponivel/parcial; nao conclui ausencia de adaptador. Consulte STATUS.tsv.\n";
    }
}
for my $id (sort {$a<=>$b} keys %ids) {
    my $before=$snapshots{before}{$id};my $after=$snapshots{after}{$id};my $r=$after||$before;
    printf $out "\nController %s | %s | USB %04x:%04x | %s\n",$id,
        $r->{family}//"UNKNOWN",$r->{vid}//0,$r->{pid}//0,$r->{interface}//"interface ausente";
    my $both_complete=($capture_rc{before}//-1)==0 && ($capture_rc{after}//-1)==0;
    unless ($both_complete) {
        printf $out "Captura indisponivel/parcial: before status=%s; after status=%s; deltas indisponiveis.\n",
            $capture_rc{before}//"ausente",$capture_rc{after}//"ausente";
    }
    if (!$after) {
        print $out (($capture_rc{after}//-1)==0?
            "Removido/ausente na segunda leitura; sem delta entre dispositivos.\n":
            "Segunda leitura falhou; evidencia anterior preservada, estado atual desconhecido.\n");
        next;
    }
    print $out "Novo Registry ID; sem leitura anterior para comparar.\n" if !$before && $both_complete;
    my @publication=qw(RTWDiagPublicationGeneration RTWDiagPublicationMonotonicMS);
    my $publication_complete=$both_complete && $before &&
        !grep {!exists $before->{$_} || !exists $after->{$_}} @publication;
    my $publication_unsafe=0;
    if (!$publication_complete) {
        print $out "Publicacao: frescor desconhecido; marcadores/captura incompletos.\n";
    } elsif (($before->{$publication[0]} % 2) || ($after->{$publication[0]} % 2)) {
        print $out "Publicacao: em andamento; comparacao indisponivel.\n";
        $publication_unsafe=1;
    } elsif ($after->{$publication[0]}<$before->{$publication[0]} ||
             $after->{$publication[1]}<$before->{$publication[1]}) {
        print $out "Publicacao: geracao/tempo regrediu; comparacao indisponivel.\n";
        $publication_unsafe=1;
    } elsif ($after->{$publication[0]}==$before->{$publication[0]}) {
        print $out "Publicacao: sem nova publicacao observada; geracao=$after->{$publication[0]}.\n";
    } else {
        print $out "Publicacao: geracao avancou $before->{$publication[0]} -> $after->{$publication[0]}.\n";
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
            print $out "$k leitura nativa indisponivel/incomparavel; consulte StatsValid/Index.\n";next;
        }
        if (!$both_complete) {print $out "$k observado=$after->{$k}; delta indisponivel por captura parcial\n";next;}
        if ($publication_unsafe) {print $out "$k observado=$after->{$k}; delta indisponivel por publicacao em andamento/regredida\n";next;}
        if (!$before || !exists $before->{$k}) {print $out "$k atual=$after->{$k}; anterior ausente\n";next;}
        if ($after->{$k}<$before->{$k}) {print $out "$k antes=$before->{$k} depois=$after->{$k}; reset/reiniciado, delta indisponivel\n";next;}
        print $out "$k antes=$before->{$k} depois=$after->{$k} delta=".($after->{$k}-$before->{$k})."\n";
    }
    print $out "Calib timing AU: nao instrumentado; RTWDiagCalib*=0 nao mede ausencia de atraso.\n" if ($r->{family}//"") =~ /AU$/;
}
close $out or die "close summary: $!";
PERL_RTW
then
    printf 'drivers/RTW-summary\tok\t0\tPerl: RTW snapshots por Registry ID\n' >> "$STATUS"
    SUCCESS=$((SUCCESS + 1))
else
    rc=$?
    printf 'drivers/RTW-summary\tfalhou\t%s\tPerl: RTW snapshots por Registry ID\n' "$rc" >> "$STATUS"
    FAILURE=$((FAILURE + 1))
fi

cat > "$RUN_DIR/README-DIAGNOSTICO.txt" <<EOM
TAHOEREALTEKWIFI — CAPTURA DE DIAGNÓSTICOS
Revisão do coletor: ${COLLECTOR_REVISION}
Data de execução: $(date '+%Y-%m-%d %H:%M:%S %z')
Intervalo de eventos: últimos ${MINUTES} minutos (logs existentes no macOS)
Prazo por comando: ${COMMAND_TIMEOUT}s; status124 indica timeout, não sucesso.
Modo: $(if [ "$RAW" -eq 1 ]; then echo 'RAW / NÃO ANONIMIZADO'; else echo 'anonimização básica de linhas sensíveis'; fi)
Administrador: $(if [ "$ADMIN_READY" -eq 1 ]; then echo 'somente comandos de leitura de diagnóstico'; else echo 'não'; fi)

LEIA PRIMEIRO:
  RTW-SUMMARY.txt - família/VID:PID/interface, erro USB e diferenças por adaptador.
  drivers/RTW-before/after.txt e .json - propriedades e contadores de cada controlador.
  USB-STARTUP.txt / drivers/USB-STARTUP.json - inicializacao, inclusive providers sem controlador.
  drivers/loaded-RTW-kext.txt / installed-app-Info.txt - versões realmente ativas.
  USB-IDENTIFICATION.txt  - todos os USB com VID:PID extraídos do IORegistry.
  USB-WIFI-CANDIDATES.txt - trechos suspeitos (inclusive dispositivos rebatizados).
  usb/SPUSBHostDataType.txt ou SPUSBDataType.txt - árvore USB e IDs por dispositivo.
  usb/ioreg_USB_tree.txt - topologia USB, fornecedor e provedores/drivers.
  usb/ioreg_USBHostInterface.txt - interfaces/interfaces driver.
  drivers/kmutil_showloaded.txt / systemextensionsctl.txt - drivers ativos.
  network/hardware_ports.txt / ifconfig_all.txt - interface e status.
  logs/USB_Realtek.log / WiFi_80211.log / Network_Ethernet.log - eventos.
  logs/unified-summary.txt - total de linhas mantidas e descartadas por limite.
  logs/unified-coverage.json - janela solicitada, rc e timestamps recebidos.
  logs/recent/ - consulta independente de 2min, somente apos timeout124 principal.
  STATUS.tsv - detalhes de comandos que falharam ou foram bloqueados.

LOGS: a consulta principal pede ${MINUTES}min. Se exceder o prazo, preserva a
saida parcial e tenta UMA consulta adicional de 2min, com o mesmo prazo de
${COMMAND_TIMEOUT}s por consulta. logs/recent/ nao completa o historico parcial.
Os timestamps primeiro/ultimo mostram somente eventos efetivamente recebidos;
quando indisponiveis, nao se conclui ausencia de eventos. Mesmo com dados
recebidos, cobertura ate o presente nao e garantida; examine os timestamps.
Gatekeeper desabilitado: resposta exata 'assessments disabled'/exit1 e estado
esperado; STATUS preserva rc1 sem contar falha. Nenhuma configuracao e alterada.
CRASHES: relatorios historicos selecionados por nome, sem filtro de idade,
ordem ou deduplicacao. A contagem de arquivos nao equivale a eventos novos
nem garante relacao com a janela solicitada para os logs unificados.

FRESCOR: snapshots do IORegistry sao os ultimos valores publicados pelo driver.
STATUS nao e forcado para os adaptadores; delta0 nao prova ausencia de erros
novos ou de trafego. A build54 publica geracao/tempo monotonico por controlador;
geracao impar indica publicacao em andamento. Sem marcadores, frescor desconhecido.
A leitura do IORegistry nao e atomica; os marcadores nao substituem trafego real.

IMPORTANTE: VID/PID, hardware revision e binding a driver permitem determinar
qual hardware o macOS DETECTA. A identificação definitiva do silício interno,
do firmware correto e de um driver operacional pode exigir inspeção física
ou leitura segura do chipset com uma ferramenta específica.

PRIVACIDADE:
O mascaramento básico pode deixar escapar outros identificadores, caminhos,
nomes de rede, IPs, tokens ou dados pessoais contidos nos logs; REVISE antes
de compartilhar. Os logs RAW não têm qualquer anonimização. Este programa
NÃO envia relatórios a servidores nem faz alterações de driver ou segurança.
EOM

# Mascara por padrão, preservando VID/PID, códigos de erro, endereços de
# registradores, versões, endpoints e parte do OUI do MAC. Não é garantia
# criptográfica de anonimato, especialmente em textos arbitrários do log.
echo '[7/7] Privacidade, integridade e ZIP'
if [ "$RAW" -eq 0 ]; then
    while IFS= read -r -d '' file; do
        case "$file" in
            *.txt|*.log|*.json|*.tsv|*.ips|*.crash|*.panic) ;;
            *) continue ;;
        esac
        if command -v perl >/dev/null 2>&1; then
            if perl -pe '
                s{((?:IOMACAddress|local-mac-address|MAC Address)"?\s*=\s*)<[^>]*>}{${1}<[REDACTED]>}gi;
                s{(/Users/)[^/\s"\x27]+}{${1}[USUARIO]}gi;
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
                echo "FALHA AO ANONIMIZAR: $file" >&2
                echo "FALHA AO ANONIMIZAR: $file" >> "$RUN_DIR/ANONYMIZATION-ERROR.txt"
            fi
        else
            echo 'Perl não encontrado. Não é seguro empacotar dados não anonimizados.' >&2
            echo 'Use --raw para permitir coleta sem anonimização, após avaliar a privacidade.' >&2
            exit 1
        fi
    done < <(find "$RUN_DIR" -type f -print0)
    if [ -f "$RUN_DIR/ANONYMIZATION-ERROR.txt" ]; then
        echo 'Uma ou mais tentativas de anonimização falharam. O ZIP NÃO será criado.' >&2
        exit 1
    fi
fi

cat > "$RUN_DIR/RESULTADO.txt" <<EOM
TAHOEREALTEKWIFI — RESULTADO DA COLETA
Revisão do coletor: ${COLLECTOR_REVISION}
Data: $(date '+%Y-%m-%d %H:%M:%S %z')
Janela de logs: ${MINUTES} minutos
Comandos aprovados: $SUCCESS
Comandos indisponíveis / negados / com erro: $FAILURE
Arquivos de crash relevantes: $CRASH_COUNT
Modalidade: $(if [ "$RAW" -eq 1 ]; then echo 'RAW (SENSÍVEL)'; else echo 'mascaramento básico'; fi)

Leitura do driver:
  RTW-SUMMARY.txt separa os dois adaptadores e registra quedas/resets.
  USB-STARTUP.txt registra fases/erros de providers e controladores USB.
  STATUS.tsv registra falhas, inclusive timeout124 e log sem permissão.
  Gatekeeper: estado_esperado/exit1 preserva assessments disabled sem contar erro.
  logs/unified-coverage.json registra janela, rc e timestamps recebidos.
  logs/recent/ e tentativa separada de 2min apos timeout; historico segue parcial.

Para descobrir o USB:
  1. USB-IDENTIFICATION.txt
  2. usb/SPUSBHostDataType.txt ou usb/SPUSBDataType.txt
  3. usb/ioreg_USB_tree.txt
  4. usb/ioreg_USBHostInterface.txt

OBS: ausência de mensagens não implica hardware compatível nem driver funcional.
Se um comando falhou, ver STATUS.tsv e a saída de erro do arquivo respectivo.
E-mail, SSID, senha, serial e MAC são parcialmente mascarados no modo padrão.
Examine os arquivos antes de compartilhar; o mascaramento não é infalível.
EOM

# Hashes DOS ARQUIVOS exportados, para detectar corrupção ao compartilhar.
if command -v shasum >/dev/null 2>&1; then
    (
        cd "$RUN_DIR" || exit 1
        find . -type f ! -name 'SHA256SUMS.txt' -print0 | while IFS= read -r -d '' f; do
            shasum -a 256 "$f"
        done
    ) > "$RUN_DIR/SHA256SUMS.txt"
fi
chmod -R go-rwx "$RUN_DIR" 2>/dev/null || true

# 'ditto' é nativo do macOS e preserva a pasta; fallback 'zip' se necessário.
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
    echo 'Não foi possível criar o ZIP; a pasta com todos os relatórios foi mantida:' >&2
    echo "$RUN_DIR" >&2
    exit 1
fi

printf '\n==============================================\n'
printf ' DIAGNÓSTICO CONCLUÍDO\n'
printf '==============================================\n'
printf 'ZIP PARA ANEXAR A ESTE CHAT:\n%s\n' "$ZIP_PATH"
printf '\nPasta para inspeção:\n%s\n' "$RUN_DIR"
printf '\nExamine o ZIP antes de compartilhar (logs podem conter dados privados).\n'
printf 'Nenhum driver, SIP ou ajuste de rede foi alterado.\n\n'
if [ "$NO_PAUSE" -eq 0 ] && [ -t 0 ]; then
    printf 'Pressione Enter para encerrar esta janela... '
    IFS= read -r ignored || true
fi
exit 0
