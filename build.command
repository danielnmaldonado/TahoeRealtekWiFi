#!/bin/bash
# Build da kext, aplicativo de barra de menus e testes; NAO instala/carrega kext.
# macOS: duplo clique ou ./build.command [--all|--kext|--app|--pkg|--tests]
# Para nao acessar a rede: RTW_OFFLINE=1 ./build.command --kext
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
source "$ROOT/app/signing.sh"
rtw_require_adhoc_identity
RELEASE="$ROOT/Release"
SDK="$ROOT/MacKernelSDK"
SDK_REV="3f750085caa17ec3a7880f11c11bf4f48cd6a164"
PROJECT="$ROOT/TahoeRealtekWiFi.xcodeproj"
SCHEME="TahoeRealtekWiFi"
DERIVED="$RELEASE/.DerivedData"
PRODUCT="$DERIVED/Build/Products/Release/TahoeRealtekWiFi.kext"
OUTPUT="$RELEASE/TahoeRealtekWiFi.kext"
LOG_KEXT="$RELEASE/build-kext.log"
LOG_APP="$RELEASE/build-app.log"
LOG_TESTS="$RELEASE/build-and-test.log"
MODE="${1:---all}"

mkdir -p "$RELEASE"

usage() {
  cat <<'HELP'
TahoeRealtekWiFi - build de desenvolvimento (nao instala drivers)
  ./build.command           Compila kext, app e testes; gera ambos em Release/
  ./build.command --all     Idem
  ./build.command --kext    Compila somente a kext e copia para Release/
  ./build.command --app     Compila somente o app e copia para Release/
  ./build.command --pkg     Compila o app e gera o instalador .pkg (sem kext)
  ./build.command --tests   Compila nucleo/testes; no macOS verifica tambem app e auxiliar
  ./build.command --help    Esta ajuda

Requisitos da kext: Mac Intel, Xcode completo, git, Internet apenas se
MacKernelSDK ainda nao existir. O SDK e obtido do projeto Acidanthera em uma
revisao fixa, dentro de MacKernelSDK/ (ignorado pelo Git).
Use RTW_OFFLINE=1 para impedir a obtencao automatica da dependencia.

A kext gerada e experimental e assinada ad-hoc. O pacote visa Ventura 13,
Sonoma 14, Sequoia 15 e Tahoe 26 em Intel; requer validacao fisica por sistema.
Ad-hoc NAO substitui o certificado/consentimento exigido para carregar
kexts em instalacoes de macOS com politicas de seguranca padrao.
Este comando nao instala, nao carrega e nao altera SIP/AMFI.
A Apple nao suporta IONetworkingFamily + IOUSBHostFamily desde Big Sur.
HELP
}

fail() {
  printf '\n[ERRO] %s\n' "$*" >&2
  return 1
}

# Rodar inclusive o preflight pelo tee para que Release/build-kext.log
# exista caso o Mac nao tenha Xcode, o SDK falhe ou o build tenha erros.
run_logged() {
  local logfile="$1"
  shift
  set +e
  (set -euo pipefail; "$@") 2>&1 | tee "$logfile"
  local results=("${PIPESTATUS[@]}")
  local status="${results[0]}"
  if [[ "${results[1]}" -ne 0 ]]; then status="${results[1]}"; fi
  set -e
  if [[ "$status" -ne 0 ]]; then
    printf '\n[ERRO] A etapa falhou (codigo %s). Consulte: %s\n' "$status" "$logfile" >&2
  fi
  return "$status"
}

prepare_sdk() {
  local header="$SDK/Headers/IOKit/usb/IOUSBHostInterface.h"
  local ethernet="$SDK/Headers/IOKit/network/IOEthernetController.h"
  local kmod="$SDK/Library/x86_64/libkmod.a"
  if [[ -f "$header" && -f "$ethernet" && -f "$kmod" ]]; then
    /usr/bin/python3 "$ROOT/scripts/check-mackernelsdk.py" "$SDK" \
      "$ROOT/scripts/MacKernelSDK-SHA256.json" "$SDK_REV" || {
      fail 'MacKernelSDK nao corresponde a revisao/conteudo fixados. Seu diretorio foi preservado; forneca o SDK correto antes de compilar.'
      return 1
    }
    printf '[SDK] MacKernelSDK validado: %s\n' "$SDK"
    return 0
  fi

  if [[ -e "$SDK" ]]; then
    fail "MacKernelSDK existe, mas esta incompleto. Esperados: $header, $ethernet e $kmod. Nao alterei seu diretorio." || return 1
  fi
  if [[ "${RTW_OFFLINE:-0}" == "1" ]]; then
    fail 'MacKernelSDK ausente e RTW_OFFLINE=1. Obtenha https://github.com/acidanthera/MacKernelSDK na pasta MacKernelSDK antes de compilar.' || return 1
  fi
  if ! command -v git >/dev/null 2>&1; then
    fail 'git indisponivel: instale o Xcode completo / Command Line Tools para obter o MacKernelSDK.' || return 1
  fi

  printf '[SDK] Baixando revisao fixa do MacKernelSDK (somente dependencias de compilacao).\n'
  printf '[SDK] Repositorio: https://github.com/acidanthera/MacKernelSDK\n'
  printf '[SDK] Commit: %s\n' "$SDK_REV"
  local temp_sdk
  temp_sdk="$(mktemp -d "$ROOT/.MacKernelSDK-download.XXXXXXXX")" || return 1
  if ! (git -C "$temp_sdk" init -q &&
        git -C "$temp_sdk" remote add origin https://github.com/acidanthera/MacKernelSDK.git &&
        git -C "$temp_sdk" -c advice.detachedHead=false fetch --depth 1 origin "$SDK_REV" &&
        git -C "$temp_sdk" -c advice.detachedHead=false checkout -q --detach FETCH_HEAD); then
    rm -rf "$temp_sdk"
    fail 'Nao foi possivel obter o MacKernelSDK. Verifique conexao HTTPS/GitHub ou coloque a dependencia manualmente em MacKernelSDK/.' || return 1
  fi
  if [[ "$(git -C "$temp_sdk" rev-parse HEAD 2>/dev/null || true)" != "$SDK_REV" ]]; then
    rm -rf "$temp_sdk"
    fail 'A revisao obtida do MacKernelSDK nao corresponde ao commit fixo exigido.' || return 1
  fi
  if [[ ! -f "$temp_sdk/Headers/IOKit/usb/IOUSBHostInterface.h" ||
        ! -f "$temp_sdk/Headers/IOKit/network/IOEthernetController.h" ||
        ! -f "$temp_sdk/Library/x86_64/libkmod.a" ]]; then
    rm -rf "$temp_sdk"
    fail 'A revisao baixada do MacKernelSDK nao contem todos os headers e libkmod exigidos.' || return 1
  fi
  if ! /usr/bin/python3 "$ROOT/scripts/check-mackernelsdk.py" "$temp_sdk" \
      "$ROOT/scripts/MacKernelSDK-SHA256.json" "$SDK_REV"; then
    rm -rf "$temp_sdk"
    fail 'O conteudo do MacKernelSDK baixado nao corresponde ao manifesto fixado.' || return 1
  fi
  if ! mv "$temp_sdk" "$SDK"; then
    rm -rf "$temp_sdk"
    fail 'Nao foi possivel preparar MacKernelSDK/.' || return 1
  fi
  printf '[SDK] Preparado com sucesso, sem instalar nada no sistema.\n'
}

preflight_kext() {
  [[ "$(uname -s)" == "Darwin" ]] || { fail 'Esta kext x86_64 deve ser compilada em um Mac com Xcode.'; return 1; }
  command -v xcodebuild >/dev/null 2>&1 || { fail 'xcodebuild nao encontrado. Instale o Xcode completo e selecione-o com xcode-select.'; return 1; }
  command -v xcrun >/dev/null 2>&1 || { fail 'xcrun nao encontrado (requer ferramentas do Xcode).'; return 1; }
  command -v lipo >/dev/null 2>&1 || { fail 'lipo nao encontrado.'; return 1; }
  command -v ditto >/dev/null 2>&1 || { fail 'ditto nao encontrado.'; return 1; }
  command -v plutil >/dev/null 2>&1 || { fail 'plutil nao encontrado.'; return 1; }
  command -v codesign >/dev/null 2>&1 || { fail 'codesign nao encontrado. Assinatura ad-hoc obrigatoria.'; return 1; }
  if ! xcodebuild -version; then
    fail 'xcodebuild indisponivel: confira se o Xcode completo esta selecionado (sudo xcode-select -s /Applications/Xcode.app/Contents/Developer).' || return 1
  fi
  if ! xcrun --sdk macosx --show-sdk-path; then
    fail 'SDK macOS nao encontrado no Xcode selecionado.' || return 1
  fi
  [[ -d "$PROJECT" ]] || { fail "Projeto Xcode ausente: $PROJECT"; return 1; }
  prepare_sdk
}

compile_kext() {
  printf '%s\n' '=================================================='
  printf '%s\n' 'TahoeRealtekWiFi - compilacao da kext x86_64'
  printf 'Data: %s\n' "$(date '+%Y-%m-%d %H:%M:%S %z')"
  printf 'Source: %s\n' "$ROOT"
  printf '%s\n' 'ATENCAO: binario experimental ad-hoc; radio/conexao fisica pendentes de validacao.'
  printf '%s\n' '=================================================='
  preflight_kext

  # New core sources must be present in both CMake and the Xcode product.
  /usr/bin/python3 "$ROOT/scripts/generate-kext-project.py"

  printf '\n[Xcode] Compilando target %s (Release, x86_64)...\n' "$SCHEME"
  # MacKernelSDK fornece tanto headers legados quanto libkmod.a,
  # vinculado explicitamente no projeto Xcode. Sem instalacao/carregamento.
  xcodebuild -project "$PROJECT" -scheme "$SCHEME" \
    -configuration Release -derivedDataPath "$DERIVED" \
    ARCHS=x86_64 ONLY_ACTIVE_ARCH=NO CODE_SIGNING_ALLOWED=NO \
    build

  printf '\n[Validacao] Verificando binario Mach-O da kext...\n'
  local executable="$PRODUCT/Contents/MacOS/TahoeRealtekWiFi"
  local info="$PRODUCT/Contents/Info.plist"
  [[ -d "$PRODUCT" ]] || { fail "Xcode retornou sucesso, mas nao gerou: $PRODUCT"; return 1; }
  [[ -s "$executable" ]] || { fail "Kext sem executavel (nao e binario compilado): $executable"; return 1; }
  [[ -s "$info" ]] || { fail "Kext sem Info.plist: $info"; return 1; }
  plutil -lint "$info"
  /usr/bin/python3 "$ROOT/scripts/check-kext-linkage.py" "$executable"
  if ! lipo -archs "$executable" | tr ' ' '\n' | grep -qx x86_64; then
    fail 'O executavel nao possui arquitetura x86_64. Nao sera copiado para Release/.' || return 1
  fi

  # O destino anterior nao pode ser alterado ate que o novo bundle esteja
  # completo, assinado ad-hoc e aprovado pela verificacao de integridade.
  local temporary="$RELEASE/.TahoeRealtekWiFi.kext-new-$$"
  local previous="$RELEASE/.TahoeRealtekWiFi.kext-previous-$$"
  [[ ! -e "$temporary" && ! -L "$temporary" && ! -e "$previous" && ! -L "$previous" ]] || {
    fail 'Ja existe staging de compilacao desta sessao; nada foi sobrescrito.'; return 1;
  }
  if ! ditto "$PRODUCT" "$temporary"; then
    rm -rf "$temporary"
    fail 'Erro ao copiar a kext para Release/.' || return 1
  fi
  if [[ ! -s "$temporary/Contents/MacOS/TahoeRealtekWiFi" ||
        ! -s "$temporary/Contents/Info.plist" ]]; then
    rm -rf "$temporary"
    fail 'A copia da kext esta incompleta; preservei o build anterior.' || return 1
  fi
  printf '\n[Assinatura] Assinando kext ad-hoc antes de publicar...\n'
  if ! rtw_sign_kext "$temporary"; then
    rm -rf "$temporary"
    fail 'Falha na assinatura ou verificacao ad-hoc da kext; build anterior preservado.' || return 1
  fi
  printf '[Assinatura] Dados do codigo assinado:\n'
  codesign -dv --verbose=2 "$temporary" 2>&1
  if [[ -e "$OUTPUT" || -L "$OUTPUT" ]]; then
    [[ -d "$OUTPUT" && ! -L "$OUTPUT" ]] || {
      rm -rf "$temporary"
      fail 'O destino kext existe e nao e um bundle regular. Nada foi sobrescrito.'; return 1
    }
    if ! mv "$OUTPUT" "$previous"; then
      rm -rf "$temporary";fail 'Falha ao preservar a kext anterior.';return 1
    fi
  fi
  if ! mv "$temporary" "$OUTPUT"; then
    if [[ -d "$previous" && ! -e "$OUTPUT" ]]; then mv "$previous" "$OUTPUT"; fi
    rm -rf "$temporary"
    fail 'Falha na publicacao; kext anterior restaurada.' || return 1
  fi
  if [[ -d "$previous" ]]; then rm -rf "$previous"; fi
  printf '\n[OK] KEXT GERADA: %s\n' "$OUTPUT"
  printf '[OK] Assinatura: ad-hoc (kext)\n'
  printf '[OK] Executavel: %s\n' "$OUTPUT/Contents/MacOS/TahoeRealtekWiFi"
  if command -v shasum >/dev/null 2>&1; then
    shasum -a 256 "$OUTPUT/Contents/MacOS/TahoeRealtekWiFi" "$OUTPUT/Contents/Info.plist"
  fi
  printf '\nIMPORTANTE: build/assinatura ad-hoc nao garante carregamento da kext no Tahoe.\n'
  printf 'Nao foi instalada, carregada nem testada no adaptador USB.\n'

}

compile_app() {
  bash "$ROOT/app/build.command" "$RELEASE"
}

compile_tests() {
  printf '%s\n' 'TahoeRealtekWiFi - suites CMake (sem kext)'
  bash "$ROOT/scripts/build-and-test.sh"
}

case "$MODE" in
  --pkg|pkg)
    bash "$ROOT/app/package.command"
    ;;
  --help|-h|help) usage ;;
  --tests|tests)
    run_logged "$LOG_TESTS" compile_tests
    ;;
  --kext|kext)
    run_logged "$LOG_KEXT" compile_kext
    ;;
  --app|app)
    compile_app
    ;;
  --all|all)
    # Mesmo se a kext falhar, tenta gerar o app independente do MacKernelSDK.
    local_kext_status=0
    local_app_status=0
    # Fresh shell processes preserve errexit inside each stage. Calling a
    # function on the left of "||" disables errexit in its nested functions.
    set +e
    bash "$ROOT/build.command" --kext
    local_kext_status=$?
    bash "$ROOT/build.command" --app
    local_app_status=$?
    set -e
    if [[ "$local_kext_status" -ne 0 || "$local_app_status" -ne 0 ]]; then
      printf '\n[ERRO] Falha(s): kext=%s app=%s. Verifique os logs em %s\n' \
        "$local_kext_status" "$local_app_status" "$RELEASE" >&2
      exit 1
    fi
    run_logged "$LOG_TESTS" compile_tests
    printf '\n[OK] Kext em: %s\n' "$OUTPUT"
    printf '[OK] App em: %s\n' "$RELEASE/Tahoe Wi-Fi USB.app"
    printf '[OK] Logs: %s, %s e %s\n' "$LOG_KEXT" "$LOG_APP" "$LOG_TESTS"
    ;;
  *)
    usage >&2
    exit 2
    ;;
esac
