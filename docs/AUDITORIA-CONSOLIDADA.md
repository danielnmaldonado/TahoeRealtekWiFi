# Rodada de prioridades — 24/10/2026 — Kext 0.13.19 build 44 / App 0.2.13 build 39

**Status de Release: NAO VALIDADO.** Base imutavel: build43 ZIP SHA-256 `f0dfe8177e945c60e53a40b6a72a891de15d1e088250d471719bfeee98d694b4`; manifest original integro. Nao ha `.git` no pacote: impossivel conferir commits e branches. A integridade de ZIP e o diff sao verificaveis; isso nao implica conclusao da auditoria linha-a-linha.

## Duas causas raiz comprovadas e corrigidas

1. **F-P01 — entrada USB/RX sobrescrita por buffer de saida (`src/FamilyBackend.c`):** as funcoes `rtw_backend_usb_preflight` e `rtw_backend_rx_preflight` apagavam `out` antes de verificar aliasing com `config`/`transfer`. O teste original com mesma memoria falhou por assertiva; com a nova rotina `ranges_overlap` as entradas sao preservadas e a chamada retorna erro de argumento. Foram adicionados casos de sobreposicao exata e parcial para os dois caminhos. Impacto demonstrado: alteracao indevida da entrada quando o chamador fornece buffers sobrepostos; NAO ha evidencia de crash macOS ou vazamento em uso normal.
2. **F-P02 — selecao de adaptador invalidado (`app/Protocol.hpp`):** apos `invalidate()` o token anterior era rejeitado, mas obter `generation` atual podia passar em `matches()`, apesar da invalidacao, enquanto `select(same Registry ID)` nao reativava o adaptador. Adicionado estado `invalidated` com rejeicao em `matches()` e possibilidade de restaurar a selecao do mesmo Registry ID. Os testes da classe foram reproduzidos com falha anterior e sucesso posterior.

Alteracoes funcionais limitadas a esses dois caminhos. O incremento dos numeros de build e o gerador Xcode sao metadados para distinguir esta revisao. **Nenhum arquivo de firmware, vendor, RF, TX, UI visual, rede 802.11, assinatura ou configuracao de seguranca foi alterado**. O patch unico `docs/diffs/CORRECOES.patch` e relativo especificamente ao source original build43 (nao reescreve os patches historicos documentados).

## Testes reais (ambiente Linux, nao macOS)

- Build Debug CMake ASan/UBSan: concluiu.
- CTest 91/91 testes rapidos, mais USB matrix (270 perfis) 1/1, mais pacote app 1/1; **93/93 aprovados** em execucoes terminadas. A primeira tentativa do teste longo matrix foi interrompida por timeout e nao foi contada como aprovada.
- Clang C11 estrito em `src/FamilyBackend.c`, sintaxe dos scripts bash, parsers plist e gerador Xcode deterministico: aprovados.
- Diff unico aplicado em copia limpa do build43; os 9 arquivos alterados foram conferidos byte a byte. Nenhum arquivo original eliminado.
- Registros em `docs/AUDITORIA-EVIDENCIAS.txt`. A matriz `docs/AUDITORIA-COBERTURA.tsv` distingue indexes e validacao semantica.

## Pendencias priorizadas e evidencia insuficiente

| Prioridade | Item | Status real / proxima acao |
|---|---|---|
| P0 | 5GHz para sete familias dual-band | **Pendente.** Faltam backends RF/PHY por chip, firmware, tabelas de potencia, calibracao, HT40/VHT80, TX/RX, WPA2, limites regulatorios e provas de hardware. RTL8814AU preserva RX passivo, TX em 5GHz bloqueado. Nao liberar TX sem esses requisitos. |
| P0 | USB/IOKit hot-unplug | Leitura manual parcial de `RTWUSBController.cpp`: fluxo `message`/`quiesce`/`close`, drenagem de callbacks em `shutdownPlatform`/`deferredCleanup`. Testes portaveis passaram; hot-unplug fisico, busy/retain, sleep/wake, invalidacao de callbacks **nao testados no macOS**. |
| P0 | Xcode nativo | **Pendente.** Executar compilacao Release app/kext/helper com Xcode no Mac e inspecionar erros novos. |
| P1 | Assinatura ad-hoc | Politica existente preservada e exercitada pelo teste de assinatura; `codesign -s -` real e verificacao de binarios em Mac **nao executados**. |
| P1 | CPU/RAM/leaks/energia | Sanitizadores portaveis aprovados; Instruments/Leaks/Time Profiler/Energy Log e benchmark de WiFi **nao executados**. Nao e possivel afirmar otimizacao de desempenho. |
| P1 | Layout/tema/app | `Main.mm` parcialmente inspecionado, selecao de adaptadores corrigida. Dark/Light/HiDPI/VoiceOver em AppKit **nao verificados**. Nenhum tema ou layout modificado sem evidencia. |
| P1 | Auditoria 100% por funcao | Inventario/linhas/manifesto atualizados, inspeção semantica focada; milhares de funcoes nao foram individualmente analisadas; **NAO 100%**. |

## Checkpoint para a proxima rodada

1. Validar fisicamente no macOS o build 44, hotplug em 2 adaptadores, interrupcao de RX/TX e objetos IORegistry; coletar traces de `stop`, `close`, `busy/retain` e callbacks, antes de qualquer novo patch no teardown.
2. Continuar implementacao do **RF/PHY 5GHz por familia** sobre dispositivos de teste; nunca anunciar operacao sem TX/WPA/regulatorio comprovados.
3. Executar `codesign` ad-hoc e verifier no Mac e testes AppKit claro/escuro; medir CPU, RAM e vazamentos com Instruments; preencher matriz conforme linhas/funcoes efetivamente verificadas.
4. Prosseguir a inspeção direta da `RTWUSBController.cpp` nos pontos `shutdownPlatform`, `deferredCleanup`, `IOCommandGate`, e `app/Main.mm` em callback async/Keychain, com testes hardware associados.

**Integridade:** `SOURCE-MANIFEST.sha256` lista arquivos fonte mais documentos (exceto o proprio manifesto). O ZIP externo e seu `.sha256` sao verificados separadamente; assinatura digital real macOS permanece pendente.

---

# HOTFIX DE COMPILAÇÃO NATIVA — 24/10/2026 — kext build 41 / app build 38 (VERSÕES PRESERVADAS)

**Entrada:** log real de Xcode 26.6 (17F113), macOS SDK 26.5, do usuário. O `xcodebuild` compilou outros componentes e falhou em `kext/RTWUSBController.cpp` ao carregar `include/rtw/DiagnosticPublishPolicy.hpp:3`, com erro fatal `fatal error: 'cstdint' file not found`. A kext utiliza `-nostdinc`/headers de kernel; `cstdint` é da biblioteca padrão C++ e não está disponível nesse build. Os warnings adicionais de arquivos `vendor/` não são essa causa fatal e não foram alterados sem prova.

**Correção mínima:** substituído `#include <cstdint>` por `#include <stdint.h>` em `include/rtw/DiagnosticPublishPolicy.hpp`. A função `diagnosticCounterNeedsWrite` e seu contrato não foram alterados. O teste `tests/diagnostic_publish_policy_tests.cpp` usa agora headers C e é compilado com `-nostdinc++` (somente com compiladores que suportam essa opção), configurado em `CMakeLists.txt`. Isso evita regressão por dependência acidental da STL em código compartilhado com a kext.

**Reprodução e validação independente:** a compilação `clang++ -std=gnu++14 -nostdinc++ -Iinclude -fsyntax-only` sobre a política falhou no source original por ausência de `cstdint`, e passou após a correção. CMake Debug com ASan/UBSan recompilou; o teste de política passou com `-nostdinc++`; a suíte portátil foi dividida em 90 testes principais + 1 matriz USB + 1 pacote de aplicativo, todos aprovados (**92/92**). Essa evidência confirma a remoção da causa específica no source, **não** um `xcodebuild` nativo completo.

**Estado e escopo:** código-fonte completo, sem novos modelos USB, sem modificações no driver de rádio, firmware, layouts, protocolo, scripts de signing ou números de build. A exigência de assinatura ad-hoc está preservada, mas não foi executado `codesign` real em Linux. **Status de Release macOS: NÃO VALIDADO; nova compilação Xcode Release e assinatura efetiva ainda pendentes.** Sem `.git` no ZIP fornecido, não há validação de commits; o pacote de entrada e seus hashes são preservados como base.

**Checkpoint:** próximo teste no Mac: executar novamente `./build.command --kext` com Xcode 26.6/SDK 26.5. Caso outro erro apareça, usar o primeiro `error:` após esta correção para uma nova reprodução; após build, confirmar `codesign -dv --verbose=4 Release/TahoeRealtekWiFi.kext` e `codesign --verify --strict` antes da instalação. Não tratar warnings como crashes sem evidência.

---

# AUDITORIA CONSOLIDADA — TahoeRealtekWiFi

## Rodada adicional de 24/10/2026 — análise direta e correções comprovadas

**Árvore de referência imutável:** `TahoeRealtekWiFi-v0.13.19-KEXT41-APP38-AUDITORIA-CONSOLIDADA-ADHOC-FULL-SOURCE.zip`; SHA-256 `9be2b81fc3da5a2b0ce35ffc2c9debd569fedf0257773f76a66b999cdac5136f`; 547 arquivos originais. Confirmada a integridade do ZIP, e o manifesto de origem foi validado previamente. **Versão de source resultante:** kext 0.13.19 build 41 e app 0.2.13 build 38, com novas correções em fontes partilhados de processamento RX; os números de build não foram alterados sem compilação nativa de Release. **Sem metadados `.git`** no ZIP — não é possível assegurar commits/branches. Apenas o pacote recebido nesta conversa foi utilizado como base.

### Três defeitos demonstrados com regressão falhando antes da correção

| ID | Função real | Causa raiz | Correção localizada | Evidência |
|---|---|---|---|---|
| F-02 | `rtw_family_5g_scan_bulk`, `src/Family5GHzScan.c` | A chamada zerava `stats` antes de rejeitar a sobreposição entre saída e bytes USB ou perfil de identificação. Dados de entrada declarados imutáveis podiam ser modificados. | Rejeição precoce das faixas sobrepostas antes de `memset`, com teste de distâncias de ponteiros representados por `uintptr_t` e sem soma que exceda o endereço. | `repro_before.txt`: abort esperado, saída não compatível; `repro_fixed_family.txt`: teste passou; novos casos exatos e parcialmente sobrepostos. |
| F-03 | `rtw_r12au_passive_on_channel`, `src/RTL8812AUPassive.c` | Os contadores de RX podiam escrever sobre o agregado USB fornecido pelo chamador quando `stats` se sobrepunha a `bulk`. | Impedir aliasing do objeto de contadores com o buffer de RX antes de qualquer atualização. | `repro_second_before.txt`: abort esperado; `repro_fixed_passive.txt`: regressão passou; variantes parcial/exata. |
| F-04 | `rtw_rx_preflight_transfer`, `src/USBPackets.c` | O `memset` inicial do relatório de pré-validação podia apagar a transferência que deveria ser analisada se o mesmo armazenamento fosse passado a ambas as saídas/entradas. | Detectar e rejeitar a sobreposição antes de apagar `out`. | `repro_third_before.log`: abort esperado; `repro_third_fixed.log`: regressão passou; variantes parcial/exata. |

**Impacto comprovado:** a entrada é alterada indevidamente quando o contrato do chamador é violado mediante sobreposição; este resultado foi demonstrado em testes portáveis com os próprios fontes. **Não foi demonstrada exploração, crash macOS, vazamento de heap, ganho de velocidade ou falha no RF em uso normal**. Portanto essas afirmações não são feitas. O código de Kext RX partilhado mudou em `src/USBPackets.c`; arquivos em `kext/`, implementação do aplicativo, firmware, IDs USB, interface, tema e assinatura ficaram idênticos.

### Escopo real da inspeção semântica desta rodada

- **Leitura direta integral de unidades e principais caminhos de erro:** `src/Family5GHzScan.c` (incluindo formato de descritor, validação completa dos agregados e callbacks), `src/RTL8812AUPassive.c` (agregados, contadores e callbacks), `src/USBPackets.c` (RX, pré-validação, montagem de descritor TX), `src/USBControl.c`, `src/USBRegisterIO.c`, `src/FamilyRadioPolicy.c`, `src/StationPort.c`, `kext/RTWUserClient.cpp`, `app/LoginItem.mm` e as três unidades de testes corrigidas. Inspeções diretas adicionais em `app/Transport.cpp`, `app/DriverConnection.mm`, `app/PrivilegedService.mm`, `app/Helper.cpp`, `app/CredentialStore.mm`, `app/Model.hpp`, `app/Protocol.hpp`, `app/signing.sh` e módulos citados na matriz; nas unidades maiores, apenas trechos explícitos foram inspecionados.
- **Inspeção parcial:** `kext/RTWUSBController.cpp` (init/teardown/callbacks USB), `app/Main.mm` (fluxos assíncronos/UI), `src/EthernetBridge.c`, `src/USBDescriptors.c`, `src/RTL8814AUDataPath.c`, demais módulos visitados parcialmente. Isso não equivale a análise de cada função/linha.
- **Inventário automático e verificação por hash:** todos os arquivos presentes, mas **inventariar ou passar em CTest não equivale a inspeção manual**. O documento `docs/AUDITORIA-COBERTURA.tsv` registra status distintos para arquivos e funções heurísticas. Nem todos os ~2.834 candidatos a funções tiveram avaliação semântica: **cobertura integral NÃO atingida**.
- **Causas sem correção nesta rodada:** nenhuma outra hipótese obteve reprodução inequívoca e alteração segura. Em particular, o aviso do analisador Clang para `profile->family` em `src/Family5GHzScan.c` depende de supor que o validador retornaria `identity_verified=true` para `profile=nullptr`; `src/USBWiFiProfiles.c` demonstra explicitamente o retorno nulo/unverified. O aviso fica documentado, sem modificação especulativa.

### Rodadas reais, build, testes e assinatura

1. **R1 — base e source:** SHA-256 e manifesto do build 41/app 38, leitura direta dos caminhos de status, USB/RX, IPC, callback e assinatura ad-hoc; inventário dos riscos.
2. **R2 — reprodução:** três testes executados *antes* de cada correção, abortando como esperado; correção mínima em três fontes compartilhados; novos testes e variantes com sobreposição parcial.
3. **R3 — compilação/validação:** configuração portátil Debug e build com AddressSanitizer/UBSan; **92/92 grupos CTest concluíram sem falhas** em seis execuções disjuntas (36 + 1 + 32 + 10 + 1 + 12). Regressões de sobreposição passaram também após os últimos acréscimos. Uma execução ampla interrompida por limite de tempo e uma tentativa inconclusiva do teste de empacotamento não foram contabilizadas como sucesso; seu *retry* passou.
4. **R4 — análise independente:** `clang --analyze` aplicado aos três módulos C alterados. Duas unidades sem avisos; uma mensagem condicional da análise interunidade foi investigada, não tratada como prova de bug. A correção não adicionou dependências nem habilitou transmissão 5 GHz.
5. **R5 — verificação final e entrega:** o único patch cumulativo `docs/diffs/CORRECOES.patch` inclui as correções do app build 38 e os seis arquivos novos desta rodada, aplicável à base build 41/app 37; manifesto final e ZIP verificados. Hashes detalhados, saídas de testes, repros e checkpoints estão centralizados em `docs/AUDITORIA-EVIDENCIAS.txt`.

### Integridade, organização e proteção de alterações anteriores

- **Arquivos de código/testes alterados em relação ao ZIP de entrada:** exatamente `src/Family5GHzScan.c`, `src/RTL8812AUPassive.c`, `src/USBPackets.c`, `tests/family_5ghz_scan_tests.cpp`, `tests/rtl8812au_passive_tests.cpp`, `tests/rx_preflight_tests.cpp`. Nenhum arquivo originalmente presente foi removido; o restante do código e firmware permaneceu byte a byte idêntico.
- **Somente um artefato por finalidade:** relatório de auditoria `docs/AUDITORIA-CONSOLIDADA.md`, evidências `docs/AUDITORIA-EVIDENCIAS.txt`, matriz `docs/AUDITORIA-COBERTURA.tsv`, patch `docs/diffs/CORRECOES.patch`. Outros README, arquitetura, manuais, registros de firmware/catálogo e testes são finalidades distintas e foram preservados.
- **Assinatura ad-hoc:** `RTW_SIGN_IDENTITY=-` continua obrigatório no script `app/signing.sh`; política testada estaticamente. **A assinatura nativa real com `codesign`, Xcode/IOKit, execução macOS e verificação física permanecem pendentes**.

### Status Release, riscos e checkpoint de retomada

**Status: NÃO VALIDADO para Release macOS.** Não foi possível provar ausência universal de bugs, vazamentos, problemas de CPU/RAM/energia, suporte funcional em 5 GHz para as sete famílias dual-band, nem ausência de regressão visual em modo claro/escuro. O RX de 5 GHz catalogado continua **passivo**, sem RF/TX operacionais certificados.

**Checkpoint exato:** partir do ZIP desta rodada, conferir SHA-256 do ZIP e `SOURCE-MANIFEST.sha256`; retomar a inspeção direta não concluída em `kext/RTWUSBController.cpp` por função (retenção/abort/callbacks/stop sob unplug), depois `app/Main.mm` (dispatch, Keychain, timers, estado de menu) e `app/AdapterDiagnostics.mm` (IORegistry e CF ownership), usando a matriz para registrar linhas e funções verdadeiramente examinadas. Em um Mac, rodar `xcodebuild` Debug/Release, `codesign` real ad-hoc, Instruments (leaks/CPU/energia), VoiceOver e regressões de desconexão USB e associação física 2,4/5 GHz. Jamais contar como coberto código apenas listado automaticamente.

---

# AUDITORIA CONSOLIDADA — TahoeRealtekWiFi

## Auditoria de 24/10/2026 — estado atual verificável

**Base auditada:** último ZIP efetivamente entregue nesta conversa, TahoeRealtekWiFi v0.13.19 / kext build 41 / app build 37, etapa 2 RX 5 GHz. SHA-256 do arquivo-base: `b6b500fdf82a6c6da7cded57bb1ef76d5d7baa76168404016656790852f5f1ff`. Teste de integridade ZIP e manifesto de origem: aprovados (555 entradas no manifesto, 556 arquivos na árvore-base). **O ZIP não contém `.git`**, portanto a auditoria não pode comprovar commits, branches ou uma árvore remota mais recente.

**Versão resultante:** kext 0.13.19 build 41 (inalterada); app 0.2.13 build 38 (incremento devido à correção do transporte). **Entrega de source, não de binários Release.**

### Rodadas com evidências — sem simular validação de produção

1. **Integridade e análise de arquitetura**: hashes do pacote-base conferidos, inventário de 556 arquivos; inspeção manual de partes de `app/Main.mm`, `app/DriverConnection.mm`, `app/Transport.cpp`, `app/Protocol.hpp`, `app/Devices.hpp`, `app/PrivilegedService.mm`, `app/Helper.cpp`, `app/CredentialStore.mm`, `app/LoginItem.mm`, `app/CodeIdentity.h`, `kext/RTWUSBController.cpp/.hpp`, `kext/RTWUserClient.cpp`, `kext/RTWNet80211Globals.cpp` e do RX passivo multifamília. Atenção específica a buffer, validação de mensagens, ciclos de vida, trabalho em fila, obtenção de credenciais, USB teardown e controlo de dispositivos. **As análises foram parciais nas unidades grandes e não equivalem a inspeção semântica de cada linha.**
2. **Reprodução independente de defeito**: o teste de socket adicionou duas requisições localmente inválidas que deveriam apagar saídas anteriores; **antes da alteração, a execução abortou em `assertErased`**. Causa raiz: `rtwui::exchange` retornava antes da chamada de limpeza se a requisição fosse rejeitada localmente. O caminho equivalente XPC possuía o mesmo problema. Correção mínima em `app/Transport.cpp` e `app/DriverConnection.mm` com limpeza *somente quando* há capacidade de saída válida conhecida, incluindo rejeição de comprimento acima de `UINT32_MAX`. Regressão portátil passou com **dez cenários** após correção. Teste nativo equivalente adicionado a `app/tests/transport-tests.cpp`, mas não executado sem macOS.
3. **Regressão de build portável**: configuração CMake Debug e compilação com `-fsanitize=address,undefined` aprovadas. **90/90 testes rápidos** e **2/2 testes demorados** aprovados separadamente após a alteração. A execução separada evita extrapolar sobre timeout de um único processo. Instrumentação ASan/UBSan cobre os componentes portáveis efetivamente compilados, não a kext macOS.
4. **Análise estática independente**: `clang --analyze` executado em **50 unidades `src/*.c` e `src/*.cpp`**, com zero erros de execução do analisador e zero arquivos com avisos. Análise estática não comprova ausência de bugs de concorrência, leaks de IOKit ou comportamento de rádio.
5. **Revisão de signing, ciclo de vida e documentação**: preservada a política `RTW_SIGN_IDENTITY=-`, regras de assinatura ad-hoc verificadas por leitura dos scripts e testes de política (não por `codesign` nativo). Histórico de auditorias reunido neste documento; matrizes históricas de releases anteriores são substituídas pela única matriz ativa `docs/AUDITORIA-COBERTURA.tsv`. Referências de README/CHANGELOG atualizadas para esta fonte de verdade.

### Checkpoint reprodutível e evidências persistentes

- **Registro único dos comandos e resultados desta rodada:** `docs/AUDITORIA-EVIDENCIAS.txt` (reprodução *antes* da correção, CTest 90/90, os dois CTest longos 1/1, análise Clang de 50 unidades e execução do transporte 10/10). Os hashes SHA-256 dos logs originais estão registrados no arquivo.
- **Um único diff textual:** `docs/diffs/CORRECOES.patch`; verificado com `patch -p1 --dry-run` sobre a árvore-base. Registra os sete arquivos originais modificados. A consolidação de documentação histórica, a matriz e os registros novos são artefatos de auditoria e não modificações de lógica de produção.
- **A integridade do source entregue** é verificável por `SOURCE-MANIFEST.sha256` (cada arquivo salvo, sem auto-referência), além do checksum SHA-256 externo do ZIP; a matriz registra hashes por arquivo e as funções apenas identificadas heuristicamente.
- **Checkpoint de retomada:** kext build 41, app build 38; revisão manual incompleta registrada na matriz; próxima inspeção precisa: iniciar por `kext/RTWUSBController.cpp` no teardown/assincronismo e `app/Main.mm` no ciclo UI/dispatch, reproduzir casos reais com instrumentos macOS antes de afirmar Release ou ausência de vazamentos.

### Defeito corrigido (F-01)

- **Gatilho:** chamada a `exchange` com frame ou input inválidos, buffer `out` com resultado antigo e tamanho válido.
- **Esperado:** erro de protocolo e `out` zerado, como ocorre para respostas malformadas e incompletas.
- **Observado antes da correção:** erro de protocolo com buffer antigo preservado (regressão falhou de forma determinística).
- **Implementações:** `app/Transport.cpp` (socket) e `app/DriverConnection.mm` (XPC). Saída com ponteiro nulo ou capacidade errada continua rejeitada sem escrita fora dos limites declarados.
- **Regressões:** `tests/socket_exchange_host_tests.cpp` e `app/tests/transport-tests.cpp`.
- **Escopo da mudança:** sem alteração da ABI, do firmware, dos perfis USB, do tema, da kext ou dos protocolos sem fio.

### Cobertura e matriz por arquivo/função

O inventário **`docs/AUDITORIA-COBERTURA.tsv`** distingue arquivos cuja **integridade** foi verificada, implementação parcialmente inspecionada e módulos efetivamente exercitados pelos testes portáveis. As linhas de símbolos descobertos por extração sintática são apenas candidatos: não provam inspeção manual. Nenhuma afirmação de auditoria manual de 100% das funções é feita.

### Assinatura ad-hoc obrigatória

O source segue impondo assinatura ad-hoc para app/helper/kext por meio de `app/signing.sh`. A suíte portátil inclui testes da política. **Não foi executado `codesign` real, `xcodebuild`, assinatura de artefato binário, instalação, Instruments, testes de layout em Dark/Light, hardware USB, 2,4 GHz ou 5 GHz neste ambiente Linux.** Consequentemente, não há comprovação de Release físico nem das sete famílias dual-band como estações 5 GHz operacionais.

### Pendências, riscos e próximo checkpoint

- **Risco P0 de entrega**: testes reais de unplug/replug, suspensão, hotplug e cliente IOKit sob carga no macOS Tahoe; verificar `busy/retain` no IORegistry e estabilidade de transferências assíncronas.
- **Risco P0 de compatibilidade**: backends RF/PHY, firmware, IQK/DPK, 20/40/80 MHz, WPA2 e TX/RX de 5 GHz ainda não estão completos para sete famílias; os leitores passivos de beacon **não** correspondem a Wi-Fi funcional nessa banda.
- **Risco P1 de qualidade**: validar AppKit no macOS com VoiceOver, tema claro/escuro, dimensões de menu, alta densidade de redes e atualização de estados assíncronos; perfis de CPU/RAM/energia com Instruments.
- **Risco P1 de processo**: obter metadados Git verificáveis (commit/hash/branch) do repositório original. O source compactado não inclui essa informação.
- **Próxima verificação exata**: compilar a árvore entregue com Xcode no macOS, executar os testes nativos `app/tests/transport-tests.cpp`, assinar ad-hoc e verificar `codesign -dv --verbose=4`; depois realizar testes físicos no macOS Tahoe com unplug/replug e tráfego de rede. Não sinalizar a etapa concluída sem evidência do hardware.

### Organização e histórico

Este é o **único relatório `.md` ativo de auditoria**. Para não perder informações anteriores, o conteúdo integral dos relatórios predecessores foi incorporado abaixo, com seus hashes. `README.md`, `CHANGELOG.md`, `ARCHITECTURE.md` e `SOURCES.md` continuam separados por terem finalidades próprias. As matrizes históricas `.tsv` estão substituídas pela matriz do source corrente; seus hashes de arquivo original são registrados aqui.

#### Checkpoint: matrizes históricas consolidadas

- `AUDITORIA-INVENTARIO-APP35.tsv`: SHA-256 `ff0d11ad99e1019d612fd4f9b2b631755ad7a88e37c76bbc7f827dfd2c0306b1`; matriz de release anterior, não utilizar como inventário do build atual.
- `AUDITORIA-INVENTARIO-APP36.tsv`: SHA-256 `d1a7fad386728f86e5e5f6593f72be67bc72976dcdf35cb39b9ab215043299bf`; matriz de release anterior, não utilizar como inventário do build atual.
- `AUDITORIA-INVENTARIO-BUILD40.tsv`: SHA-256 `80558caf7ef4f1fa00bd9ec599629b1083c63dea9c5c50d0ef4e7477bb2ec3c7`; matriz de release anterior, não utilizar como inventário do build atual.
- `INVENTARIO-SOURCE-BUILD41.tsv`: SHA-256 `9ea1aaf2bead7a1ade9a925553f9c333b3ab7d82424dee0f22ad604027e713cb`; matriz de release anterior, não utilizar como inventário do build atual.

---

# ANEXO — DOCUMENTAÇÃO HISTÓRICA (não constitui validação desta rodada)


## Histórico: `docs/AUDIT.md` (SHA-256 `69688bf02a11c82b5cd9661b786f9424671639bc766520135951c65cc751e84e`)

## 0.13.19 — guarded firmware-stage and optional USB laboratory tool

**Scope:** Added a shared legacy 8051 firmware *transfer and startup stage* for
RTL8192FU/RTL8723BU. RTL8710BU receives a validated firmware layout only,
with boot blocked because the SoC has a distinct power/reset sequence.
All other chipsets fail closed in this module. This is **not** a completed
PHY/RF or macOS station backend.

**Read-only stage:** `rtw_legacy_fw_prepare` requires a catalog-owned,
verified profile and descriptor-validated bulk endpoints. Known firmware
signature/header/size must match family. `rtw_legacy_fw_walk` plans at most
8 × 4096 bytes, using at most 254 bytes per vendor request without crossing
page boundaries; no USB writes and no allocations occur during planning.
Firmware format checking is not a cryptographic provenance guarantee.

**Experimental hardware stage:** `rtw_legacy_fw_boot_experimental` is
available only for 8192FU/8723BU and is deliberately outside kext attach.
It requires explicit caller permission, exclusive interface ownership and
board-specific power initialization to have been completed externally.
Its bounded 8051 sequence rejects stale MCU checksum state, aborts on short
writes or timeout and attempts to disable downloads on failure.
An interrupted transfer may leave the adapter requiring a USB replug.
The optional `rtw-legacy-fwdiag` tool defaults to read-only USB descriptor
inspection and dry-run transfer planning. Writes require two explicit
flags; it neither detaches the kernel driver nor offers Wi-Fi connectivity.
It claims only the descriptor-validated interface, not a hard-coded USB 0.

**Validated:** GCC CMake build, **85/85 CTest groups with UBSan**;
firmware-stage synthetic USB tests for family/signature mismatch, size/page
boundaries, stale checksums, short USB transfers, error cleanup and deadlines.
Clang strict C compilation plus analyzer passed the new translation unit;
Clang C++ syntax checked the optional lab tool against a temporary libusb
API declaration stub. No real libusb compile/link, macOS Xcode,
actual `codesign`, IQK, RF transmission, AP join, WPA2 handshake or real
adapter use was performed.

**Coverage claim:** 14 chipset families are catalogued, but only the four
preexisting station families have eligible kext personalities (122 IDs).
105 confirmed profiles need functional backends and 43 remain unverified.
This release adds **zero** usable/certified station adapters.

## 0.13.18 — cross-family RX / firmware / passive-radio review

**Scope:** RTL8192FU, RTL8723BU, RTL8821CU, RTL8822BU, RTL8822CU receive
decoders; RTL8192DU / RTL8192FU / RTL8723BU / RTL8710BU firmware header
inspection; six unfinished families' passive channel geometry. USB
preflight validates the existing 270 IDs; 122 remain eligible for the
already integrated station code. These are source-level capabilities only.

**Safety:** Both 24-byte RX wire variants validate bounded USB aggregates
before publishing any result, including CRC/ICV, rates, sizes, frame
limits and descriptor alignment. Family RX helpers never access physical
USB, never allocate memory and do not authorize TX. Firmware checks are
signature/size checks only, not cryptographic authentication. The
channel plan is passive-only and does not assert regional permissions.

**Testing:** CMake portable build; 84/84 CTest cases with UBSan and the
system Python; 60,000 RX plus 35,000 firmware/channel randomized inputs;
four modified C units compile strictly and pass Clang static analysis.
`plutil -lint` validates the Xcode project; the kext plist retains
122 personalities. Native Xcode kext/app compilation, real codesign,
IQK, USB hardware operation, AP association and physical RSSI validation
were not performed on this Linux test host.

**Remaining blockers:** RTL8192DU RX/TX requires a checked silicon-specific
wire format; RTL8710BU/RTL8814BU still lack verified station hardware
implementations. All new RX-only families still need firmware upload,
PHY/RF calibration, TX, WPA2, macOS kernel interface integration and
physical regression. Do not interpret a passed decoder test as a
functioning radio.

## 0.13.17 — Cross-family backend boundary (portable stage)

The **14-family** registry and USB preflight are implemented for every
catalogued chipset. The source checks actual class/subclass/protocol,
configuration value, interface number, optional bcdDevice and a safe paired
bulk endpoint mapping. Unverified ID matches are rejected. No adapter is
claimed or modified by this portable code.

The available RX wire-format codecs are **RTL8192CU, RTL8188EU, RTL8192EU,
RTL8814AU, RTL8812AU, RTL8821AU**, with all-frame atomic offline decoding.
Eight other families still have **no chip-specific RX/TX codec**. Shared
preflight is **not** a substitute for a station backend: no hardware
transmission is authorized by `FamilyBackend.c`.

The kext's runtime discovery retains its four existing station families
and now cross-checks the backend capability bit before IOEthernet attach.
This check was compiled only as portable C and contract-tested; no native
IOKit ABI, RF calibration, association, WPA2 exchange, kext loading,
codesign, or hardware interoperability was verified here.

USB catalog retained unchanged: 122 eligible personalities in the four
existing families, 105 verified IDs pending full backends and 43 unverified
or conflicting IDs. The requested full 14-family hardware support remains
**unfinished**. The next engineering work is porting firmware boot / PHY
tables, calibrations, RX/TX and station attachment for each missing silicon
generation, then verifying those functions on the actual USB devices.

# Compatibility and protocol review — driver 0.13.16 / app 0.2.12

Review date: 2026-10-23. Input: source release 0.13.15 / app 0.2.12.
This is the single current audit. `validation.json` contains executed evidence;
`source-inventory.csv` inventories the shipped source/configuration files.

## Changes audited in 0.13.16

Input: driver 0.13.15 / app 0.2.12. Corrected `FirmwareBoot.c` to clear and
verify the write-one-to-clear MCUFWDL checksum status before each firmware
download. The USB mock models W1C rather than treating MCUFWDL as normal RAM.
Regression cases cover a stale status during a failed new boot and an
unclearable status that must abort before uploading firmware data.

`Efuse.c` and `RFCalibration.c` now reject saturated six-bit thermal code
0x3F as an eFuse baseline. This only avoids a false RF preflight result;
no actual IQK, LCK or power tracking is run by the RTL8812AU preflight.

`RTL8812AUStation.c` parses all advertised RSN pairwise/AKM suites and
accepts the WPA2-PSK/CCMP subset of transition networks with optional PMF.
It rejects SAE-only, TKIP-only, mandatory PMF and truncated/malformed
suite lists. This parser operates offline and cannot grant carrier.

Portable UBSan build: **80/80 CTest groups passed**, 0 failed, using the
system Python. Four changed C modules passed Clang -Werror strict compilation
and Clang static analysis without diagnostics. No native macOS Xcode,
actual codesign, USB hardware boot, memory-leak instrumentation or physical
802.11 association tests were completed. Enabled USB identities remain 122,
and 148 other catalog identities remain disabled.

## Changes audited in 0.13.15

Source input: 0.13.14 / app 0.2.12. The station kext's scan previously
treated the available pairwise CCMP indication as enough to advertise a
WPA2-PSK/CCMP connection. The new `rtw_network_security_classify` also
requires RSN, PSK, **group CCMP** and absence of mandatory PMF; the
group field comes from net80211 `ni_rsngroupcipher`, not the negotiated
pairwise `ni_rsncipher`. The model remains conservative: a displayed AP
is only *protocol-eligible*, not proven able to associate over the radio.

`src/RTL8812AUStation.c` now validates association-response ESS/privacy
capabilities and advertised rates/IE structure before reporting even
offline association. These helpers are not wired into a full real station
backend for RTL8812AU/RTL8821AU, cannot transmit or negotiate WPA2 keys,
and do not increase the kext's enabled ID set.

Portable Linux Debug/UBSan CTest: **80 of 80** pass with `/usr/bin/python3`
and four-worker scheduling. Includes 64 security combinations, malformed
association-response cases, and a source contract checking the native-only
macOS scan's actual group-cipher field. Clang strict C/C++ syntax
compilation and Clang static analysis of the modified portable station
module passed. USB matching generation and catalog checks passed.
**Native Xcode/kext compile, real codesign, hardware RF/link tests,
ASan/LSan and macOS Instruments were not performed.**
The baseline 122 mapped/enabled USB profiles are not hardware-certified,
and the remaining 148 catalog profiles stay disabled.

## Changes audited in 0.13.14

`src/RFCalibration.c` / `include/rtw/RFCalibration.h` extend the RTL8812AU
B-cut/TSMC/USB/RFE3 channel-6 passive *laboratory diagnostic*, not the
operational kext station attach path. Preflight requires explicit opt-in,
positive prior path-A RF channel readback, prepared USB chip identity and
six matching MAC/BB monitor registers. It reads two path-A thermal values
from RF[0x42] via Jaguar HSSI (PI or SI mode), comparing their 6-bit sensor
codes and an optional factory eFuse code at logical offset 0xBA. Missing,
unstable or untrusted values do not silently become a valid calibration.
A delta of 8 or more code units is only an advisory for a future IQK
implementation. Code units are not degrees Celsius.

After a complete register snapshot, the preflight temporarily adjusts the
CCA guard and RF serial selector. It attempts selector/CCA restoration on
every path after a write and verifies both saved values; restore failures
override sampling failures. It does not program analog RF, start a temperature
conversion, run IQK/LCK/DPK, change TX power, transmit 802.11 frames,
authenticate or advertise a station link. A cached RF register value may
look consistent without being fresh, so successful observations cannot prove
functional calibration. `rtw-hwdiag` invokes the preflight only in the
already authorized lab mode and attempts MAC RX quiescence on malformed
USB transfer lengths as well as normal/other failure exits.

Portable Linux Debug build configured with UBSan (`-fsanitize=undefined`):
**78/78 CTest groups passed** using `/usr/bin/python3` with `-j4`;
the new synthetic USB calibration test passed **96 checks**. Separate RF
readback tests passed 119 checks. HAL assertions passed 504 checks, with
12,000 adversarial eFuse/RX test inputs. Strict Clang warnings-as-errors
syntax of the new C module passed. No Apple `codesign`, macOS/Xcode,
real libusb link, device RF/thermal readout, power calibration or association
was performed. Previously eligible 122 profiles and pending 148 profiles
are unchanged. The UI is still English and mandatory ad-hoc signing policy
remains in place but was not verified by native codesign in this host.

## Changes audited in 0.13.13

This increment adds `RFReadback.h` / `RFReadback.c` to diagnose the
hardware RF channel of the previously opt-in RTL8812AU B-cut/TSMC/USB/RFE3
channel-6 monitor. `rtw_rf_readback_channel6_8812au` first re-verifies the
six MAC/BB registers and the monitor authorization flag; if this read-only
guard fails, no additional register writes occur.

After saving registers 0x838 (CCA) and 0x8B0 (HSSI), the RF probe temporarily
sets the secondary-CCA guard and selects RF register 0x18 in HSSI. It reads
path A and B via the appropriate PI or SI readback register, twice each.
Path A must encode channel 6 with 2.4-GHz band bits. Path B is diagnostic:
zero/invalid/stale B readback is explicitly inconclusive, never evidence
of working dual-chain RF. Selector and CCA original values are restored
on all post-write exits and verified by reading them again. A failed restore
takes precedence over the original I/O error and blocks capture.

`rtw_monitor_quiesce_rx_channel6` reads CR (0x100), clears only MACRXEN,
RXDMA_EN and HCI_RXDMA_EN, and verifies their cleared state. The libusb
diagnostic calls it on normal scan exit, after RF/MAC verification failure
and on partial RX configuration failures. It is **not** an RF power-down
or full hardware reset. It does not alter the station kext's attach path.

Host validation: Linux debug build with `-fsanitize=undefined` throughout,
**77/77 portable CTest groups passed** using `/usr/bin/python3` in 34.91 s.
The new mock RF test covers 2T2R PI/SI selection, path-B zero values,
two path-A samples, invalid channel/band, report authorization, USB
read/write error injection, RF settling delay failures, BB restore readback
failures and MAC RX quiescence. Both changed C modules pass Clang strict
`-Wall -Wextra -Wconversion -Wshadow -Wpedantic -Werror` syntax checks
and `clang --analyze` with no emitted diagnostics.

`tools/rtw-hwdiag.cpp` passes Clang C++ syntax checking against temporary
test declarations for the `libusb` API. This is **not** equivalent to
compiling and linking against the real `libusb` SDK. No physical adapter
or macOS/Xcode build was available; no real RF register readings, IQK/LO
calibration, TX power verification, association, WPA2 or codesign was tested.
The 122 previously enabled profiles are unchanged; the RTL8812AU/RTL8821AU
station IDs are still disabled.

## Changes audited in 0.13.12

This increment connects the **opt-in libusb passive laboratory RX path**
to `rtw_r12au_rx_next` (specific 8812AU aggregate layout) and
`rtw_r12au_scan_parse` (beacon/probe/RSN with FCS). The two-pass aggregate
preflight prevents a truncated tail from publishing an earlier AP from the
same USB transfer. The receiver uses caller-owned buffers, no heap allocation,
a 65,536-byte transfer bound and up to 64 frames; callbacks do not store
pointers. Unsupported security can appear in diagnostics with an explicit
unsupported indication, never as a usable station backend.

`rtw_monitor_verify_channel6` makes exactly six read-only vendor-register
requests on success, comparing the 0xF0 SYS_CFG, 0x100 MAC RX enable,
0x608 RX filter and selected BB gate bits at 0x800, 0x808 and 0x834.
A mismatch stops the tool before bulk capture. It does **not** read RF
path 3-wire channel register 0x18, prove IQ calibration, guarantee reception
or authorize station TX. The earlier opt-in profile/table write sequence
is unchanged; its risks and supported board restriction remain.

Portable evidence: full non-instrumented CMake/CTest suite 76/76 passing in
31.58 s (`-j4`, `/usr/bin/python3`). Four affected tests were separately
built with `RTW_ENABLE_SANITIZERS=ON` and
`RTW_HOST_SANITIZERS=undefined`; all 4/4 passed. New RX tests ran
20,000 deterministic hostile-transfer iterations. Clang strict
`-Wall -Wextra -Wconversion -Wshadow -Werror` and `clang --analyze`
completed for the two modified C translation units without diagnostics.
Xcode project plist syntax passed `plutil -lint`. The optional libusb tool
compiled **only in syntax mode against temporary test declarations**, not
against the real libusb library. No macOS Xcode/kext build, real codesign,
ASan/LSan, Apple Instruments or physical hardware capture was performed.
The 270-profile matrix, 122 enabled / 148 disabled split, app UI strings,
firmware blobs and signing policy were not modified.

## Changes audited in 0.13.11

Implemented `RTL8812AURadio.{h,c}` (pure channel/frequency geometry) and
`RTL8812AUStation.{h,c}` (beacon/probe-response and WPA2-RSN inspection,
fixed-size selected AP state, open-system authentication and association
frame serialization and reply state transitions). Both modules are portable,
make no USB calls, allocate no heap memory, and grant no TX authorization.

The channel planner handles 2.4 GHz channels 1-11 at 20 MHz and selected
5 GHz standards-based groupings at 20/40/80 MHz. It rejects inconsistent
primary/center pairings, unsupported widths, channel 14 and 2.4 GHz 40 MHz,
and marks every plan passive-only. DFS is a flag requiring full regulatory
handling and hardware CAC; no passive-only plan may be used to transmit.
Real RFE and per-board transmit-power calibration remains unimplemented
for the new families.

The station parser requires the PHY receiver's externally known tuned
channel and rejects DS/HT primary mismatches, duplicate SSID/RSN/rate IEs,
truncated fields, conflicting or invalid RSN suites, hidden SSIDs, bad MAC
addresses, ICV/CRC descriptor errors and invalid FCS when available.
It permits only open networks or WPA2-PSK with CCMP group/pairwise suites;
WEP, WPA1, SAE, 802.1X, TKIP, PMF-required and mixed-mode RSN are not
negotiated. CRC32 is not authentication and cannot prevent forged beacons.

The join state machine checks BSSID/source/destination and response status,
tracks timeout/disconnect/reject states, emits a restricted RSN IE for
WPA2 and refuses invalid management transitions. Successful management
responses do not create a trusted station link: OPEN remains UNVERIFIED;
WPA2 remains KEYS_REQUIRED, with no four-way handshake or CCMP TX.
No code connects these new functions to the active kext station path and no
USB IDs were changed.

Validated in a Linux portable CMake build with UBSan and all 75 CTest
groups passing (31.1 s final run, -j5), 100,495 targeted RF/join checks including
50,000 deterministic parser fuzz inputs, 11 CLI integration scenarios, and
Clang --analyze without warnings for the two new implementation files.
Existing C/RTL8192EU warnings and previously reported instrumentation
limits remain; there was no native macOS build, codesign invocation,
Apple Instruments leak measurement, radio transmission or physical testing.

## Changes audited in 0.13.10

This increment implements a portable RTL8812AU/RTL8821AU wire-format layer
without enabling any new USB device, kext attach path, firmware radio sequence
or operating station. The unchanged 270-profile matrix contains 122
backend-eligible entries, 105 waiting for a station backend and 43 with
conflicting or inadequate identity evidence. Of the 105 waiting, 34 are verified
RTL8812AU IDs and 22 verified RTL8821AU IDs. No new adapter is claimed working.

Descriptor memory is caller-owned: no allocations, threads or retained pointers
are created. Bulk input length is bounded at 65,536 bytes, each RX frame at
4,096 bytes, and a transfer at 64 frames. Truncated, oversized and malformed
descriptors return error statuses without advancing the successful-frame cursor.
PHY metadata has the correct position before a separate alignment shift.
The 40-byte TX serializer validates candidate chain count, link-mode options,
rates, EAPOL, multicast and QoS, then computes and checks the 16-bit XOR
checksum over the descriptor prefix. Non-QoS frames request hardware sequence
assignment, while QoS frames carry a caller-assigned sequence.

Tests confirmed the module builds in the Linux CMake project and is listed in
the native Xcode kext target (the latter was not compiled). The new unit test
exercised 149,935 assertions, including 50,000 deterministic fuzz iterations.
The same isolated unit test passed under Clang UndefinedBehaviorSanitizer.
The read-only inspector decoded a synthetic RX transfer for both families.
The complete 73-group portable CTest suite then passed in 33.31 seconds,
with the system Python interpreter and `RTW_HOST_SANITIZERS=undefined`
configured in CMake. The matrix test also passed four of four unittest
cases when run separately. Earlier attempts with the original environment
timed out during interpreter startup or failed to allocate ASan shadow memory;
those attempts are not counted as passes. No macOS / on-device testing occurred.

USB adapter identity, English application strings, signing policy and firmware
from 0.13.9 were preserved. Ad-hoc signing remains mandatory in the build
scripts, but no actual macOS signature was produced. The actual 8812/8821
PHY calibration, chipset/RFE variants, WPA2 and station operations remain to
be implemented and validated before their 56 verified IDs may be enabled.

## Baseline compatibility review from 0.13.9

The following 20-alias compatibility review was performed in the previous
release and is preserved as historical evidence, not a claim that the complete
suite ran again in 0.13.10.

## Result and scope

The implementation enables **20 additional USB identities** through existing
station backends: 19 C/CU and one RTL8814AU. The complete catalog has 270 entries,
with 122 enabled: C/CU 86, 8188EU 13, 8192EU 9, 8814AU 14.

This is not an implementation of all 250 legacy profiles or all possible Realtek
families. The original 148 disabled entries remain disabled: 105 have confirmed
numeric family evidence but no integrated station backend, and 43 still have
unverified/conflicting identity evidence. No new radio HAL or firmware was added.
The RTL8188FU/FTV path was investigated but remains incomplete and is not enabled.

The review focused on catalog evidence, shared matching, kernel discovery,
generators and app diagnostics. It is not a fresh line-by-line audit of every
vendor source. Previous signal, lifecycle and resource fixes are preserved;
their historical changes are recorded in CHANGELOG.md.

## Implementation changes

All 20 extension entries are represented in the source catalog, generated C
table, generated IOKit personalities, kernel full-descriptor lookup, app reports
and the single CSV matrix. They route to real existing implementations, not
dummy success paths. Existing endpoint, silicon/ROM, firmware, board/RFE and
connection-state checks are retained.

The 250 original matching dictionaries are preserved byte-value-for-byte-value
and protected by a canonical SHA-256 identity pin. New records have their own
origin and matching policy, not invented original plist provenance. The schema
requires exact numeric source evidence, an explicit revision/release, supported
backend membership, unique identities, bounded descriptor values and nonempty
NUL-free display names.

The kernel now passes class/subclass/protocol fields from the real USB interface
descriptor. Shared C matching checks explicit mask bits. The older five-argument
API fails closed for profiles that need the missing class descriptors. The app
reads the same descriptor fields and distinguishes missing data from mismatch.
A Bluetooth interface cannot qualify as a vendor-specific Wi-Fi interface just
because its containing USB device has a known VID/PID.

New C/CU aliases use configuration 1/interface 0/class 0xff as conservative
project policy. Verified AU profiles also require subclass/protocol 0xff/0xff
from the reviewed primary driver. A different genuine hardware layout is still
excluded; this is not a claim of unrestricted revision/interface coverage.
Legacy revision/configuration/interface constraints were not weakened.

C lookup now uses bounded binary search over the generated sorted table.
No radio polling, allocation rate, USB transfer size or calibration constant was
increased. CPU or throughput gains were not measured.

## New identities

C/CU:
04bb:0950, 050d:11f2, 0846:9042, 0846:9043, 0bda:17c0, 0bda:8170,
0bda:817c, 0bda:817d, 0bda:817e, 0bda:8186, 0bda:8191, 0bda:819a,
0bda:8754, 0eb0:9071, 1058:0631, 13d3:3358, 13d3:3359, 2357:0100,
9846:9041.

RTL8814AU: 20f4:809b.

The correct new identities 2357:0100 and 0eb0:9071 do not silently relabel the
different legacy entries 038b:0100 and 0e0b:9071. Those remain disabled.
All source locations are consolidated in SOURCES.md and per-profile metadata.

## Validation

The portable build and full 72-group CTest suite passed with explicitly selected
UndefinedBehaviorSanitizer. Assertions remain enabled. Tests include all 270
catalog entries, dispatch accounting, deterministic generation, preserved legacy
constraints, extension provenance mutation, unknown/wrong USB identities,
class/subclass/protocol byte domains, absent/overflowing registry descriptors,
composite-interface isolation, original radio/stack harnesses and mandatory
ad-hoc signing/package policy fixtures.

Signing tests use a controlled replacement for codesign; they do not produce
or verify a real Apple signature. Native app/kext builds and radio hardware
were not available. AddressSanitizer, LeakSanitizer and Instruments were not
executed for this release; UBSan is not a substitute for leak detection.

Static analysis results, exact commands and captured test output are recorded
in validation.json. File hashes prove archive consistency, not runtime safety.
No claim is made that all bugs/leaks are eliminated, that all enabled boards
work physically, or that performance improved by a measured percentage.

## Native acceptance still required

Build the app/helper/kext with Xcode on the target Intel Mac; verify mandatory
ad-hoc signatures and publication failure on broken signatures. The app installer
does not install the kext. Confirm actual USB descriptors and board revision
for every newly claimed adapter before physical certification.

Exercise scan, association, WPA2/CCMP, DHCP, Ethernet traffic, reconnect,
unplug/replug, sleep/wake, multiple adapters and sustained load. Check preserved
RSSI behavior, packet/error counters and runtime initialization diagnostics.
Profile CPU and memory with Instruments on the Mac. Capture light/dark/high
contrast and VoiceOver behavior for the updated diagnostic reports.

## Source delivery and documents

The source archive contains the complete app, helper, kext, portable core,
vendor stack, firmware, generators and tests, except the separately fetched
MacKernelSDK dependency already required by the baseline. It contains no
newly compiled macOS executable, installer or kext.

Documentation remains one file per purpose. The old fixed-size CSV filename
was replaced by USB-WIFI-ADAPTERS.csv, not duplicated. The existing README,
changelog, architecture, audit, provenance guide, validation record, source
inventory and integrity manifest were updated in place. Distinct third-party
license texts and component hash records are preserved unchanged.


---

## Histórico: `docs/AUDITORIA-APP35.md` (SHA-256 `d9ef19c4841328011e328f7e9d6d13f421781b102bf80be68745b1a6b56e2dcf`)

# Auditoria de seguimento — TahoeRealtekWiFi / App build 35

**Data:** 23/10/2026  
**Fonte exata:** `TahoeRealtekWiFi-v0.13.19-build40-APP-build34-ADHOC-AUDITED-FULL-SOURCE.zip`  
**Entregável:** kext `0.13.19` **build 40** (intocada); aplicativo `0.2.13` **build 35**. O protocolo/ABI não foi modificado.

## Controle de origem, cobertura e limitações

- O ZIP-base passou em `unzip -t`, conferência SHA-256 do pacote e verificação de todas as **536 entradas** de `SOURCE-MANIFEST.sha256`; **537 arquivos** no source de origem. Não foi utilizada versão antiga.
- `docs/AUDITORIA-INVENTARIO-APP35.tsv` lista cada arquivo regular do pacote atualizado, exceto ele mesmo e o manifesto auto-referencial, com tamanho, linhas, SHA-256 e estado comparado ao build 40. O manifesto atualizado protege todos os arquivos exceto o próprio manifesto.
- A conferência de todos os caminhos/bytes e as verificações automatizadas não equivalem à inspeção humana semântica de cada linha das aproximadamente 140 mil linhas. As revisões direcionadas cobriram fontes de AppKit, reconexão, Keychain, XPC, empacotamento, scripts, política de assinatura e driver USB. O código de terceiros e o firmware foram preservados.
- Plataforma disponível: Linux com compilador Clang, CMake e testes portáveis. **Não** foi possível compilar ou executar código AppKit/IOKit no macOS Tahoe, observar UI real, produzir códigos assinados ad-hoc ou medir consumo real de CPU/RAM.

## Correções efetivas — escopo restrito

1. **Conexão automática tardia após desabilitação:** `app/Main.mm` verificava a identidade da interface e uma geração de JOIN após a leitura assíncrona do Keychain, mas não verificava se o usuário havia desligado/pausado a reconexão automática nesse intervalo. Acrescentada checagem no retorno à thread principal **antes** de `startJoin`; o resultado atrasado é descartado e a senha temporária é apagada. A conexão manual continua permitida. A decisão pura está em `app/ReconnectPolicy.hpp` com teste combinatório em `app/tests/reconnect-policy-tests.cpp`.
2. **Resposta XPC malformada:** `app/DriverConnection.mm` aceitava uma resposta com framing válido que continha um payload semanticamente inválido; após `memcpy` a função devolvia `protocolError`, deixando conteúdo não confiável no buffer do chamador, sem invalidar a conexão. O caminho de erro agora zera o buffer e fecha a conexão XPC. `tests/app_source_contract_tests.py` adiciona teste estático de contrato; não substitui um teste XPC nativo.
3. **Identificação rastreável:** incremento isolado do `CFBundleVersion` do app (34 → 35), com `CHANGELOG.md`, `README.md` e este relatório. `CFBundleShortVersionString` permanece `0.2.13`, e a kext continua build 40.

Nenhum arquivo foi removido; nenhum source `kext/`, `src/` ou `vendor/`, perfil USB, firmware, ícone, tema, privilégio, configuração de usuário ou script de assinatura foi alterado. A correção hot-unplug preexistente do build 39 continua incorporada no build 40.

## Matriz de revisão e evidências

| Subsistema | Verificação | Resultado / ressalva |
|---|---|---|
| Arquitetura e árvore | Inventário e SHA por arquivo, comparação com fonte verificada | Cobertura de arquivos 100%; não é prova de revisão semântica linha a linha |
| App e estados assíncronos | `Main.mm`, `ReconnectPolicy.hpp`, testes de cenários e contrato | Reconexão tardia corrigida; callback real do Keychain não exercitado no macOS |
| IPC / helper | Framing, conteúdo de resposta, tratamento de erros em `DriverConnection.mm` | Buffer e conexão saneados no caminho de payload inválido; XPC real pendente |
| Kext / USB lifecycle | Caminhos de stop/close e testes portáveis preexistentes | Kext intocada; descarte físico e retenções reais pendentes |
| CPU, memória, leaks, threads | Compilação com ASan/UBSan, CTest e análise estática de 49 arquivos `src` | Sem falhas nos cenários percorridos; não prova ausência global de leak/race |
| Wi-Fi, WPA, RF, firmware e USB | Suite de matrizes, WPA/net80211 e backend | 270 perfis de catálogo testados; firmware e protocolo intocados |
| Interface/tema/fluidez | Leitura dos estados de menu e preservação AppKit/SF Symbols | Nenhuma alteração visual; exige inspeção em modo claro/escuro no macOS |
| Assinatura ad-hoc/instalação | `app/signing.sh` e testes de política, rotinas do .pkg | Obrigatoriedade intacta; sem `codesign` real neste ambiente |
| Dependências de terceiros | Arquivos hash-comparados ao build 40 | Sem alterações; não garante auditoria exaustiva de código upstream |

## Testes e análise realizados

- Compilação CMake Debug com `RTW_ENABLE_SANITIZERS=ON`, `RTW_HOST_SANITIZERS=address,undefined` — concluída sem erro.
- CTest: **87/87 grupos portáveis aprovados**, com **84** grupos em uma execução e **3** testes mais demorados executados isoladamente (`rtw-app-pkg-tests`, `rtw-net80211-pae-tests`, `rtw-usb-wifi-matrix-tests`). Sanitizers estavam habilitados no build portátil; alguns testes Python possuem seus próprios subprocessos.
- Regressões direcionadas: `rtw-menu-reconnect-policy-tests`, `rtw-menu-source-contract-tests` e `rtw-signing-policy-tests`, aprovados.
- Clang Static Analyzer (`core`, `unix`, `deadcode`, `security`) em 49 unidades portáveis `src/`: nenhum diagnóstico de erro; 96 avisos genéricos `memset`/`memcpy`/`memmove` da regra de funções de buffer consideradas inseguras. Esses avisos não foram tratados como vazamentos confirmados nem corrigidos indiscriminadamente.
- Sintaxe de Bash/Python, parse de plists, manifesto e hashes do ZIP verificados durante o empacotamento. Alguns testes dependentes de Mac só podem ser executados em Darwin.

## Assinatura ad-hoc obrigatória

`app/signing.sh` rejeita `RTW_SIGN_IDENTITY` diferente de `-`, rejeita perfil de notarização neste caminho, assina com `codesign --sign -` e confere a assinatura em helper, binário, bundle do aplicativo e kext. Esta auditoria **não flexibiliza** a política e **não desativa SIP, AMFI ou qualquer mecanismo do macOS**. O material distribuído é **código-fonte**, sem binários macOS ad-hoc previamente assinados. A assinatura será produzida e conferida pelo fluxo existente em um Mac; ela por si só não contorna os requisitos de segurança do macOS para carregar kexts.

## Validação macOS ainda obrigatória

1. Em macOS Tahoe Intel com Xcode e MacKernelSDK: `./build.command --all` e `./build.command --tests`; conferir logs antes da instalação.
2. Confirmar identidades com `codesign -dv --verbose=4` e verificar cada bundle/binário `codesign --verify --strict`.
3. Testar Keychain com leitura deliberadamente atrasada enquanto se desabilita a reconexão; observar que não surge novo JOIN automático.
4. Testar respostas XPC truncadas/malformadas no ambiente nativo e confirmar zeroing, invalidação da conexão e recuperação.
5. Instrumentar CPU/heap com Instruments, comparar antes/depois; verificar energia/ociosidade, menu/tema claro e escuro, resolução HiDPI, sleep/wake, dois adaptadores Realtek, desconexão de USB e a ausência de serviços residuais no IORegistry.

**Resultado:** dois defeitos de confiabilidade corrigidos na camada do aplicativo. Não é tecnicamente justificável declarar ausência de todos os bugs, leaks, regressões de layout ou estabilidade no hardware sem os testes nativos acima.


---

## Histórico: `docs/AUDITORIA-APP36.md` (SHA-256 `f74913c0136afe93240e20b836dc095b40799bcc4bcdb6ebe351526ddeabb888`)

# Auditoria técnica geral — TahoeRealtekWiFi, aplicativo build 36

**Data:** 23/10/2026. **Base verificada:**
`TahoeRealtekWiFi-v0.13.19-KEXT-build40-APP-build35-ADHOC-AUDITED-FULL-SOURCE.zip`.
**Entrega:** kext 0.13.19 **build 40 (inalterada)** e aplicativo 0.2.13
**build 36**. Source integral; sem binários macOS nem assinatura aplicada.

## Método de auditoria e limites

- O ZIP de entrada passou na conferência SHA-256 de seu artefato e no teste de
  integridade. A árvore completa foi inventariada e cada arquivo foi comparado
  por hash contra essa versão, incluindo fontes, firmwares, recursos e código
  de terceiros. Não foram removidos arquivos da versão de origem.
- Código portátil compilado em Debug com sanitizadores AddressSanitizer e
  UndefinedBehaviorSanitizer. Os **88 grupos** registrados no CTest passaram
  nos lotes **35/35 + 30/30 + 23/23**, incluindo lifecycle USB, políticas WPA,
  matrizes de adaptadores, empacotamento, contratos do aplicativo e política
  ad-hoc. Os testes podem variar de cobertura e não inspecionam todas as
  trajetórias possíveis em execução.
- Revisão dirigida de contratos do app, fluxo de dados de IPC, caminho de
  cancelamento, operações de socket e falhas de leitura, ciclo de vida USB,
  scripts de build/empacotamento e assinatura. Conferência de árvore completa
  e hashes **não equivale** a auditoria semântica humana linha a linha de todo
  código de terceiros e do driver, nem garante ausência de defeitos futuros.
- Ambiente de execução: Linux/Clang. Não há compilação nativa AppKit/IOKit,
  medição com Instruments, teste de hardware Realtek ou validação gráfica
  dos modos claro/escuro nesta entrega. Portanto, não se afirma 100% de
  ausência de bugs, vazamentos, travamentos, uso excessivo de CPU ou falhas visuais.

## Defeito confirmado e correção implementada

**Arquivo:** `app/Transport.cpp` — `rtwui::exchange`, transporte legado por socket.

**Antes (build 35):** um cabeçalho de resposta válido seguido de EOF após
parte do payload fazia `readExact` retornar falso e `exchange` devolver
`transportError`, mas deixava no buffer de saída tanto bytes previamente
existentes quanto dados parcialmente lidos e não confiáveis. Respostas com
versão inválida ou listas de dispositivos inválidas também não higienizavam
seu buffer antes de retornar `protocolError`.

**Agora (app build 36):** após enviar a solicitação, o output é limpo antes
qualquer leitura da resposta; é novamente limpo em erros após leitura parcial
ou validação do payload. Falha de envio também limpa o output. A ordem
preserva possibilidade de compartilhamento entre os buffers de entrada/saída
até finalizar o envio. Framing, ABI, protocolo, selectors e status não mudaram.
O componente XPC ativo não foi alterado: esta correção protege especificamente
o caminho legado da troca por socket.

**Reprodução causal:** o teste com a implementação extraída **do build 35**
aborta em `assertErased()` após uma resposta truncada (buffer contendo bytes
não zerados). Com a implementação atual **extraída sem modificações** do source,
sete cenários reais de `socketpair` passaram: payload válido, resposta
truncada, versão inválida, erro reportado pelo helper, registry ID errado,
cabeçalho incompleto e lista de dispositivos malformada. O executável de
regressão é compilado com sanitizadores. Foram adicionados casos equivalentes
em `app/tests/transport-tests.cpp` para execução nativa posterior e um
contrato em `tests/app_source_contract_tests.py`.

## Matriz de cobertura

| Subsistema | Evidência desta revisão | Condição/pendência |
|---|---|---|
| Pacote / provenance | Inventário, hashes, comparação do ZIP base; nenhum original removido | Integridade verificável, mas não prova a correção global |
| Transporte app / IPC | Bug reproduzido no legado; sete cenários de socket e contrato de source | XPC principal não alterado; execução XPC nativa pendente |
| Reconexão / Keychain | Regressões e lógica do build 35 mantidas | Teste de latência real do Keychain pendente |
| Driver IOKit / hot-unplug | Código idêntico ao build 40; testes portáveis de lifecycle | Desconexão USB real, retenções IORegistry pendentes |
| WPA / RF / firmware | Testes portáveis incluídos e artefatos preservados | Rádio e associação em hardware não verificados |
| CPU, memória, threads | Debug + ASan/UBSan, correção comprovada no buffer | Sem medição de CPU/heap real; inexistência global de leaks não garantida |
| AppKit / layout / tema | Assets e código visual preservados, contratos portáveis verificados | QA visual macOS, animações e HiDPI pendentes |
| Ad-hoc / helper / kext | Política de `app/signing.sh` mantida; teste de política passou | `codesign`/instalação não realizados no Linux |
| Pacote final | Manifesto SHA-256 e ZIP byte a byte verificados | Origem/alterações rastreáveis no inventário |

## Assinatura ad-hoc (obrigatória)

Em `app/signing.sh`, `rtw_require_adhoc_identity` rejeita identidades diferentes
`-` e perfis de notarização; `rtw_sign_app` e `rtw_sign_kext` usam
`codesign --sign -` e verificam a indicação **`Signature=adhoc`** para
componentes aplicáveis. Scripts e política permaneceram **byte a byte
inalterados**. O arquivo distribuído é source, portanto não contém aplicativo,
helper nem kext já assinados. A assinatura deverá ser emitida e verificada
num Mac; não substitui permissões de carregamento de kext exigidas pelo macOS.

## Mudanças rastreadas

- `app/Transport.cpp`: limpeza segura do buffer em falhas de recepção,
  envio e payload inválido.
- `app/tests/transport-tests.cpp`: três novas regressões macOS (não executadas).
- `tests/app_source_contract_tests.py`: novo contrato de sanitização.
- `tests/socket_exchange_host_tests.cpp` + `scripts/test-app-socket-exchange.py`:
  teste host baseado nas funções de produção extraídas literalmente, 7 cenários.
- `CMakeLists.txt`: registra o novo grupo de teste.
- `app/Info.plist`: somente build 35 → 36 (mantém `0.2.13`).
- `README.md`, `CHANGELOG.md`, `docs/AUDITORIA-APP36.md`,
  `docs/AUDITORIA-INVENTARIO-APP36.tsv`, `SOURCE-MANIFEST.sha256`: documentação,
  trilha de hashes e histórico. Nenhum arquivo `kext/`, `src/`, `vendor/`,
  `include/`, firmware, asset visual, protocolo ou script de assinatura
  foi alterado em relação ao pacote verificado.

## Validação no macOS ainda necessária

1. Compilar no macOS Tahoe Intel com os SDKs requeridos usando o fluxo do
   projeto (`./build.command --all` e `./build.command --tests`).
2. Conferir `codesign -dv --verbose=4` com `Signature=adhoc` em cada
   componente, mais `codesign --verify --strict` após o empacotamento.
3. Exercitar `app/tests/transport-tests.cpp` nativamente; forçar
   respostas interrompidas/invalidas e observar recuperação sem dados parciais.
4. Medir CPU, heap, wakeups e bateria com Instruments; verificar ícones,
   layout, fluidez, login item e modo claro/escuro no AppKit real.
5. Testar USB plug/unplug, sleep/wake, múltiplos adaptadores e retenções
   IORegistry, comparando `RTWUSBController` e USBHost antes e depois.

**Resultado desta entrega:** uma falha real reproduzida e corrigida, com
regressões introduzidas e preservação integral dos demais subsistemas.
**Não há comprovação de que todos os bugs/leaks estejam eliminados.**


---

## Histórico: `docs/AUDITORIA-GERAL-BUILD40.md` (SHA-256 `ffd3e38ba1a0f58808f4e56fd08d1fca041d76c53fce00b8b06694a0631121e0`)

# Auditoria técnica geral — TahoeRealtekWiFi build 40

**Data:** 2026-10-23  
**Base auditada:** ZIP integral `TahoeRealtekWiFi-v0.13.19-build39-USB-HOTUNPLUG-FULL-SOURCE.zip` (533 arquivos), sem reaproveitar uma versão anterior.  
**Entrega:** kext versão funcional 0.13.19, **build 40**; aplicativo versão funcional 0.2.13, **build 34**. ABI e perfis USB não alterados.

## Precisão do escopo e limites

Todos os arquivos presentes no ZIP-base foram inventariados, confrontados byte a byte com a entrega e abrangidos por verificações automatizadas de conteúdo/sintaxe conforme o formato. A lista de arquivos com tamanho, linhas, hash e classificação está em `docs/AUDITORIA-INVENTARIO-BUILD40.tsv`. Isso **não equivale a revisão semântica manual de cada uma das mais de 100 mil linhas de código**. A revisão humana foi concentrada no driver, lifecycle, publicação de diagnósticos, interface AppKit, empacotamento, assinatura e testes; o código-fonte de terceiros e firmwares foram preservados byte a byte. Não se declara erradicação de todos os bugs.

A execução foi efetuada em **Linux**, não em macOS Tahoe. Sem Xcode, IOKit, AppKit, `codesign` nativo ou adaptadores USB reais, não é possível afirmar desempenho, consumo real, fluidez gráfica, ausência de vazamentos no kernel, compatibilidade de carregamento da kext ou assinatura efetivamente produzida. Nenhum ajuste de SIP, AMFI, Gatekeeper, configurações do usuário ou serviços foi realizado.

## Matriz de cobertura

| Componente | Análise e evidência | Status / limite |
|---|---|---|
| Inventário da árvore e dependências | Todos os arquivos do source comparados ao build 39; nenhum original excluído | Cobertura integral do inventário, não da semântica de cada linha |
| Kext / ciclo de vida USB | Conferidos `start`, `stop`, provider close, retenções e testes do build 39 | Fix anterior preservado; unplug físico pendente |
| USB ID, famílias e firmware | Suite de perfis e catálogos, recursos e upstream preservados | Perfis não modificados; validação de hardware pendente |
| Diagnósticos / CPU e heap | Eliminação de escrita de `OSNumber` em valores inalterados durante status e calibração | Testes host aprovados; economia em produção não medida |
| WPA2, pacotes, RF, net80211 | Testes de protocolo/isolamento e sanitizadores de memória host | Caminhos portáveis testados; kernel real não instrumentado |
| Menu/AppKit | Opções de auto-reconexão e inicialização com estado visual imediato sem refazer o menu em tracking; recursos nativos mantidos | Testes AppKit apenas analisados no source; aparência real não fotografada |
| Threads/estado assíncrono | Testes de lifecycle, state machine, timeout e epoch existentes | Testes host aprovados; race detection no macOS pendente |
| App/helper, IPC e credenciais | Código e contratos existentes preservados; verificação por testes de regressão host | Fluxo XPC/Keychain real pendente |
| Instalação/empacotamento | Testes de política de `.pkg`, sintaxe de shell e paths fail-closed | Criar `.pkg` e instalar em macOS pendentes |
| Assinatura ad-hoc obrigatória | Inspeção de `app/signing.sh`, `app/package.command`, `app/build.command`, `build.command` e testes de política | Exigência preservada; verificação `codesign` real pendente |
| Build e infraestrutura | Python descoberto antes dos dois primeiros testes; Xcode project generator determinístico | Testes CMake executados em host Linux |

## Problemas confirmados e correções

1. **Testes silenciosamente não executados**: `find_package(Python3 ...)` era chamado após duas entradas CTest, que ficavam sem comando. Movido para o começo do bloco `BUILD_TESTING`.
2. **Gravações repetidas de diagnósticos no IORegistry**: publishers de status e `chooseRateControl` criavam novos objetos `OSNumber` para valores 64-bit idênticos. Adicionada `publishDiagnosticCounter` e a decisão pura testável `diagnosticCounterNeedsWrite`; todos os valores, tipos e nomes publicados permanecem inalterados. O contador só se escreve quando ausente, com largura errada ou valor diferente.
3. **Menu visualmente desatualizado durante o clique**: alternadores de auto-reconexão e login startup agora atualizam o `NSMenuItem.state` imediatamente. O login reflete **estado confirmado** por macOS, inclusive aprovação pendente, e não somente a solicitação.
4. **Simuladores de teste fora de sincronia com o driver**: três ambientes host que extraem o método real não tinham a nova API de publicação. Foram atualizados sem reduzir as verificações existentes.
5. **Log de versão desatualizado**: atualizada a mensagem textual de 0.13.14 para 0.13.19, sem modificar o protocolo.

## Testes executados

- CMake Debug com `-DRTW_ENABLE_SANITIZERS=ON`, `RTW_HOST_SANITIZERS=address,undefined` (AddressSanitizer / UBSan).
- Suíte **87/87 grupos CTest aprovados**, incluindo assinatura ad-hoc (testada com utilitário simulado), WPA, USB, firmware, menus/estado portátil, teardown e matriz de IDs. Não substitui instrumentação do processo Mac e do kernel.
- Regressão do publisher: 100.000 passos de counters e teste integrado que confirma 0 novas gravações em recalibração estável e exatamente uma gravação quando uma métrica muda.
- AST Python, `bash -n` e parsing de Property Lists de todos os respectivos arquivos do pacote, sem erros; Xcode project generator determinístico.
- Todos os registros do novo manifesto SHA-256 devem conferir; o próprio manifesto é excluído de sua lista, por impossibilidade de auto-hash.

## Assinatura ad-hoc (obrigatória)

`rtw_require_adhoc_identity` rejeita identidades diferentes de `-` e perfil de notarização. `rtw_sign_app` assina primeiro o helper e depois o bundle, conferindo helper, executável e app; `rtw_sign_kext` assina e verifica a extensão. Scripts de build e packaging abortam em falhas. Os testes de política verificam a ordem, `--sign -`, falhas de assinatura e rejeição de assinaturas não ad-hoc.

**Aviso de segurança:** assinatura ad-hoc é distinta de Developer ID e de notarização. Ela **não garante** o carregamento de uma kext em configurações padrão de segurança no macOS. O ZIP fornecido é **source**, não binários macOS assinados. Não foi alterada qualquer configuração de segurança para contornar as restrições do sistema.

## Mudanças e preservação

Sem alterações a firmwares, dados binários, diretório `vendor/`, catálogos USB, credenciais, rede/transmissão, arquitetura IPC, recursos gráficos ou preferências do usuário. Os únicos números de build alterados são kext 39 → 40 e app 33 → 34; as versões semânticas foram preservadas.

## Validação obrigatória restante no macOS Tahoe

1. Compilar kext/app no Xcode e verificar assinatura ad-hoc com `codesign -dv --verbose=4` e `codesign --verify --strict`.
2. Medir CPU, heap e memória física com Instruments (Time Profiler, Allocations/Leaks) em idle, scan, associação e tráfego sustentado.
3. Validar tema claro/escuro, menu aberto, monitor HiDPI e estados de aprovação de login usando AppKit real.
4. Testar remoção/conexão dos dois adaptadores, estado `ioreg`, transferências USB pendentes, sleep/wake e troca de porta USB; testar repetição para investigar retenções.
5. Validar `.pkg` instalado em máquina de teste, instalador/serviço e compatibilidade com políticas de segurança definidas pelo proprietário.

**Não foi comprovada ausência global de bugs, leaks ou regressões não exercitadas.** O source corrigido, testes e inventário tornam o escopo das mudanças rastreável.


---

## Histórico: `docs/AUDITORIA-RADIO-BUILD41.md` (SHA-256 `5946fd9a6df3d53b0a35f1a164b2e6ac109cd389dac51fc914f1f5ef3dafbe8a`)

# Relatório técnico — bandas, velocidades e matriz USB (build 41 / app 37)

**Data:** 23/10/2026. **Base:** source completo TahoeRealtekWiFi v0.13.19 (kext build 40) / app v0.2.13 (build 36). **Resultado deste ciclo:** kext build 41, app build 37; nenhuma nova estação Wi-Fi operacional habilitada.

## Resultado técnico, sem extrapolações

Este pacote **NÃO entrega 5 GHz funcional para todas as placas, nem velocidade máxima em todas as famílias**. Fazer isso exige implementar e validar inicialização USB, firmware, rádio RF/PHY, eFuse, calibração de potência, regras regulatórias, canais, DFS, MIMO/NSS, HT/VHT, associação, segurança e RX/TX em cada backend. Nenhuma dessas sequências pode ser inferida de um ID USB ou de uma tabela de taxas. **Somente capacidades nominais de família, inspeção passiva de geometria e metadados do aplicativo** foram ampliados com segurança nesta revisão.

Tampouco é possível habilitar 5 GHz em hardware exclusivamente 2,4 GHz, como RTL8192EU e RTL8188EU. A taxa indicada como *PHY ceiling* não é vazão real de TCP/UDP, nem prova que uma placa de determinada marca possua todas as cadeias de RF da família.

- 270 USB VID:PID listados, sem novos IDs nem exclusões.
- 227 perfis com identidade USB catalogada como verificada; **43 sem verificação** continuam inelegíveis a vínculo novo.
- 122 perfis associados aos quatro backends de estação existentes (RTL8192CU/8188EU/8192EU/8814AU), sujeitos a checagem adicional em execução.
- **0** perfis com 5 GHz operacional comprovado pelo backend macOS. Nenhum modo HT/VHT foi desbloqueado arbitrariamente.
- 13 famílias possuem descrição nominal suficiente para pré-verificação **passiva** de canais. RTL8814BU permanece desconhecida/bloqueada.

## Matriz auditada por família

`VID:PID` significa um par no catálogo, **não** uma placa individualmente testada. O limite PHY da família pressupõe design de referência e condições ideais; `0` na coluna de driver significa que o backend de estação ainda não existe.

| Família | IDs / verificados | IDs com estação | Bandas nominais da família | Estação no macOS | Teto PHY da família / driver |
|---|---:|---:|---|---|---|
| RTL8192CU | 92 / 86 | 86 | 2,4 GHz | 2,4 GHz | 300 / 54 Mbps |
| RTL8188EU | 18 / 13 | 13 | 2,4 GHz | 2,4 GHz | 150 / 54 Mbps |
| RTL8192EU | 10 / 9 | 9 | 2,4 GHz | 2,4 GHz | 300 / 300 Mbps |
| RTL8192DU | 5 / 5 | 0 | 2,4 e 5 GHz | não implementada | 300 / 0 Mbps |
| RTL8192FU | 4 / 4 | 0 | 2,4 GHz | não implementada | 300 / 0 Mbps |
| RTL8710BU | 2 / 2 | 0 | 2,4 GHz | não implementada | 150 / 0 Mbps |
| RTL8723BU | 3 / 2 | 0 | 2,4 GHz | não implementada | 150 / 0 Mbps |
| RTL8812AU | 37 / 34 | 0 | 2,4 e 5 GHz | não implementada | 867 / 0 Mbps |
| RTL8814AU | 17 / 14 | 14 | 2,4 e 5 GHz | 2,4 GHz | 1300 / 54 Mbps |
| RTL8814BU | 1 / 0 | 0 | indeterminada | não implementada | indeterminado / 0 |
| RTL8821AU | 36 / 22 | 0 | 2,4 e 5 GHz | não implementada | 433 / 0 Mbps |
| RTL8821CU | 13 / 5 | 0 | 2,4 e 5 GHz | não implementada | 433 / 0 Mbps |
| RTL8822BU | 30 / 29 | 0 | 2,4 e 5 GHz | não implementada | 867 / 0 Mbps |
| RTL8822CU | 2 / 2 | 0 | 2,4 e 5 GHz | não implementada | 867 / 0 Mbps |

Observação: RTL8192EU tem um caminho HT20/HT40 com até dois fluxos espaciais **quando negociado** (300 Mbps PHY é teto teórico); RTL8192CU/8188EU e RTL8814AU ainda têm limite de estação legado de 54 Mbps PHY neste source. O RTL8814AU é nominalmente dual-band 802.11ac, porém seu caminho de estação ainda é apenas 2,4 GHz legado. Nada nesta revisão altera tais limites.

## Arquivos e correções específicas

1. `include/rtw/USBWiFiProfiles.h` e `src/USBWiFiProfiles.c`: tipo e funções compartilhadas de capacidade da família versus capacidade implementada; verificação da identidade **por ponteiro do catálogo imutável** antes de informar capacidade de estação. Declaração explícita compatível com C e C++ para não quebrar compiladores com `-Werror`.
2. `src/FamilyRadioPolicy.c` e `include/rtw/FamilyRadioPolicy.h`: validação **sem escrever no hardware**, rejeitando famílias, bandas, largura e canais não previstos; nenhum estado de conexão ou transmissão foi habilitado. Conserva a política de falhar de maneira fechada.
3. `src/FamilyBackend.c` e `tests/family_backend_tests.cpp`: alinham os indicadores de pré-validação passiva das 13 famílias ao mesmo catálogo de rádio. Essas flags não ativam backends ou placas USB.
4. `tools/rtw-backends.cpp`: novo `--adapters-json` lista os 270 IDs, identidade verificada, band/width/NSS e teto PHY nominal e de estação. `--json` reflete o mesmo contraste por família.
5. `app/AdapterDiagnostics.mm` e `app/AdapterSupport.hpp`: relatórios de catálogo/diagnóstico mais claros, distinguindo nominal versus implementado; RTL8192EU corretamente descrito com HT20/HT40. Nenhuma alteração em ícones, aparência, cores ou comportamento de conexão.
6. `tests/radio_capabilities_tests.cpp` e `tests/radio_cli_contract_tests.py`: validação de todas as 14 famílias, todos os 270 IDs e garantia de que nenhuma capacidade nominal de 5 GHz seja incorretamente exposta como operacional no macOS.
7. `docs/USB-PHY-CAPABILITIES.csv` e `.json`: matriz completa por VID:PID. `README.md` e `CHANGELOG.md` atualizados. `CMakeLists.txt` atualizado para registrar os dois testes novos.
8. `kext/Info.plist` e `app/Info.plist`: **somente** números de build atualizados, mantendo versões funcionais e bundle IDs. Projeto Xcode regenerado a partir do script existente.

## O que falta para cumprir integralmente o requisito

**Famílias 802.11n apenas 2,4 GHz:** RTL8192CU, RTL8188EU, RTL8192EU, RTL8192FU, RTL8710BU e RTL8723BU. Só devem operar nessa banda; para alcançar seu teto é necessário completar/validar HT20/40, rate control, agregação e calibração eFuse, respeitando quantas cadeias RF existem em cada placa.

**Família 802.11n dual-band:** RTL8192DU. Requer backend de estação macOS, firmware, validação 5 GHz/DFS e HT20/40 real.

**Famílias 802.11ac dual-band:** RTL8812AU, RTL8814AU, RTL8821AU, RTL8821CU, RTL8822BU e RTL8822CU. Requerem backends com inicialização de RF 5 GHz, parâmetros por placa, eFuse, calibração, 80 MHz/VHT, negociação NSS/SGI, WPA e transporte USB robusto. A RTL8814AU já possui um backend de estação **parcial**; as demais ainda não.

**RTL8814BU:** catálogo com identidade não verificada; não habilitar ou anunciar banda/PHY até confirmar o silício específico e os endpoints reais.

A seguir, para cada família, implementar por etapas isoladas e testáveis: (a) identificação + firmware + boot controlado; (b) calibração RF/eFuse e canais permitidos; (c) scan e associação WPA; (d) TX/RX + HT/VHT e calibração regulatória; (e) medições reais em 2,4/5 GHz com largura, MCS, NSS, throughput e hot-unplug sob macOS Tahoe. Isso demanda logs e adaptadores físicos representativos; não deve ser declarado pronto com testes offline.

## Testes e política de assinatura

- Portáveis com AddressSanitizer/UndefinedBehaviorSanitizer: 90 grupos CTest, executados em três blocos. Um erro de compilação C/C++ (`-Wmissing-field-initializers`, terceiro bloco) foi identificado e corrigido, antes da repetição completa. **90/90 testes passaram na árvore final, em três blocos de 30**, com sanitizadores habilitados.
- Matrizes de todas as identidades USB e cenários passivos de canal: passaram nos testes locais. Nenhum backend 5 GHz novo habilitado.
- Assinatura **ad-hoc obrigatória, fail-closed**: scripts originais mantidos. A assinatura real e a compilação AppKit/IOKit exigem o ambiente macOS/Xcode; não são verificáveis neste host Linux.
- **Não foram feitos** testes com dongles reais, roteadores 5 GHz, resultados de velocidade, DFS, RF nem certificação de potência. O pacote não contém binários macOS assinados, somente código-fonte.

## Verificação local pelo desenvolvedor macOS

```
./build.command --kext
kmutil showloaded | grep -i TahoeRealtekWiFi
```

O build `41` carregado confirma apenas a versão, não o suporte de rádio. Antes de considerar um novo chipset operacional, são necessários resultados físicos de scan/associação, largura HT/VHT, MCS/NSS, throughput medido e desvinculação USB limpa. A emissão de `codesign` deve ser checada sobre cada bundle compilado pelo procedimento do projeto.

## Referências técnicas públicas

- Realtek, RTL8192EU — especificação oficial, 802.11b/g/n 2,4 GHz: https://www.realtek.com/Product/Index?cate_id=194&id=601&menu_id=291
- Realtek, RTL8814AU — especificação oficial, 802.11ac/abgn, três fluxos espaciais e USB 3.0: https://www.realtek.com/Product/Index?cate_id=194&id=574
- Linux Wireless, matriz de drivers USB/rtw88/rtl8xxxu: https://wireless.docs.kernel.org/en/latest/en/users/drivers/rtl819x.html
- Kconfig do kernel Linux para rtl8xxxu, com restrições documentadas: https://kernel.googlesource.com/pub/scm/linux/kernel/git/rt/linux-rt-devel/+/master/drivers/net/wireless/realtek/rtl8xxxu/Kconfig


---

## Histórico: `docs/5GHZ-RX-IMPLEMENTATION-STATUS.md` (SHA-256 `16d480662cd43947983ca715e4118398520698f3fc140cafbcf8040aecb7ea5c`)

# 5 GHz — implementação incremental e cobertura real

**Data:** 2026-10-24  
**Base verificada:** TahoeRealtekWiFi v0.13.19, kext build 41 / app v0.2.13 build 37.  
**Versões não alteradas:** esta revisão modifica apenas componentes portáveis de RX/scan offline e de validação passiva; não inclui novo binário, firmware ou integração de transmissão na kext.

## Conclusão operacional

**Não há 5 GHz operacional novo nesta entrega.** É um avanço real de validação de dados/scan em 5 GHz, NÃO uma implementação completa do RF/PHY, associação e TX/RX em cada família. Nenhuma placa deve ser anunciada como dual-band funcional apenas por constar do catálogo. Todas as verificações de canais continuam `passive_only` e não representam autorização regulatória ou de transmissão.

## Famílias de hardware dual-band presentes (7)

| Família | Hardware 5 GHz (referência) | Kext: associação/TX 5 GHz | RX/scan desenvolvido aqui |
|---|---|---|---|
| RTL8192DU | Sim | **Não** | Preflight de identidade e geometria, passivo |
| RTL8812AU | Sim | **Não** | Decoder offline Jaguar, beacons HT/VHT, FCS |
| RTL8814AU | Sim | **Não** | Preflight passivo; backend de estação atual continua 2,4 GHz |
| RTL8821AU | Sim | **Não** | Decoder offline Jaguar, beacons HT/VHT, FCS |
| RTL8821CU | Sim | **Não** | Preflight de identidade e geometria, passivo |
| RTL8822BU | Sim | **Não** | Preflight de identidade e geometria, passivo |
| RTL8822CU | Sim | **Não** | Preflight de identidade e geometria, passivo |

RTL8814BU possui identificação pendente e não recebe capacidade inferida. Famílias exclusivamente 2,4 GHz não devem e não passam a transmitir em 5 GHz.

## Código novo e alterado

1. `src/RTL8812AUStation.c` e `include/rtw/RTL8812AUStation.h`: parsing offline de HT Operation e VHT Operation (20/40/80 MHz), canal primário/central, incompatibilidades de geometria, estados inválidos e FCS, sem ativar TX. Novo resultado `operating_width_mhz` e `center_channel` reflete **anúncio do AP**, não negociação de velocidade.
2. `src/RTL8812AUPassive.c` e `include/rtw/RTL8812AUPassive.h`: `rtw_r12au_passive_on_channel` aceita canal fisicamente conhecido e os formatos RTL8812AU/RTL8821AU, valida input e dois passes em agregados USB; wrapper de canal 6 mantido para compatibilidade.
3. `src/FamilyRadioPolicy.c` e `include/rtw/FamilyRadioPolicy.h`: `rtw_family_radio_prepare_profile` exige ponteiro real e identidade verificada do catálogo antes de retornar geometria **somente passiva**, cobrindo os 270 IDs sem autorizar nenhum rádio inexistente.
4. `tools/rtw-acusb-decode.cpp`: acrescenta `advertised_width`, `center`, `ht`, `vht` **após** todos os campos antigos, sem quebrar o contrato de saída.
5. `tests/radio_5ghz_rx_tests.cpp` (novo) e `CMakeLists.txt`: regressões por família e por ID USB, HT40/VHT80, divergência de canal, DFS em inspeção passiva, FCS, buffers USB malformados, chips de um/dois fluxos.
6. `SOURCE-MANIFEST.sha256`: manifesto atualizado para a árvore final (excluindo o próprio arquivo do manifesto).

**Nenhuma** mudança em firmware, `kext/`, `vendor/`, `app/`, signing, hardware power/tuning ou instalação. A correção USB e a assinatura ad-hoc obrigatória permanecem preservadas.

## Validação efetuada

- ZIP de entrada build 41: SHA-256 do distribuidor validado, teste de integridade sem erro.
- Linux, `-fsanitize=undefined`: **89/89** testes da regressão rápida + **1/1** matriz de 270 perfis + **1/1** empacotamento; 91/91 grupos aprovados em três execuções.
- Linux, `-fsanitize=address,undefined`: **3/3** grupos de recepção/scan (incluindo o novo teste) aprovados.
- CLI anterior e parser de canal 6: testes de compatibilidade aprovados.
- Não executados: build Xcode nativo, assinatura efetiva dos binários, 5 GHz real com adaptadores/roteador, TX power, DFS/CAC, firmware, associação WPA2, MCS/VHT em hardware, throughput ou hot-unplug em 5 GHz.

**Para atingir o requisito original** ainda é necessário implementar e validar (em cada família) driver USB + firmware, board/eFuse, RF 5 GHz e calibração de potência/RFE, canais permitidos pela região, DFS quando aplicável, RX/TX real, negociação HT/VHT/MCS/NSS e segurança. Em especial, RTL8814AU ainda limita PHY a canais 1–11/20 MHz e RTL8812AU/RTL8821AU não possuem backend de estação integrado à kext. Isso não pode ser substituído por anúncio de canais ou por testes sem hardware.

## Verificação no macOS

Use o fluxo do projeto para compilar de forma nativa e verificar assinaturas ad-hoc, sem instalar binários de terceiros não auditados. O decodificador trabalha apenas sobre captura previamente obtida de forma autorizada e passiva:

```sh
rtw-acusb-decode rtl8812au /caminho/captura-usb.bin --channel 36 --fcs
```

`--channel 36` exige evidência da sintonia física efetiva; nunca deve ser inferido do conteúdo do beacon. Este comando **não habilita** a placa em 5 GHz.


---

## Histórico: `docs/5GHZ-MULTIFAMILY-RX-STAGE2.md` (SHA-256 `0d5890d5f1967875f409f4aebd9d95154641c184d3b60a29462968ac4e7dfc2d`)

# 5 GHz — estágio 2 de recepção entre famílias (NÃO é estação Wi-Fi funcional)

**Data:** 2026-10-24  
**Base conferida:** TahoeRealtekWiFi v0.13.19, kext build 41, app v0.2.13 build 37, pacote `5GHZ-RX-STAGE`.  
**Política mantida:** assinatura ad-hoc obrigatória e fail-closed para app, helper e kext; nenhuma habilitação de TX/região/DFS nova.

## Advertência operacional

**NENHUMA das sete famílias nominalmente dual-band recebeu suporte ativo de estação 5 GHz nesta revisão.** Reconhecer beacons ou decodificar transferências USB em software não equivale a ligar o rádio, selecionar canal, autenticar, associar, transmitir nem conseguir throughput 802.11ac. Não foi executada compilação Xcode macOS nem teste físico; não há base para prometer 5 GHz funcional em nenhum dos 270 IDs USB.

Este estágio acrescenta um pipeline REAL, porém **offline/passivo**, de agregados USB RX para seis famílias cujos formatos de descritor já constavam do source. O sétimo formato (RTL8192DU) não existe de modo verificado e, por segurança, continua indisponível.

## Mudanças de source

- `src/Family5GHzScan.c`, `include/rtw/Family5GHzScan.h`: função `rtw_family_5g_scan_bulk()` que identifica perfis imutáveis/verificados do catálogo, exige canal sintonizado externamente e metadata explícita de FCS, mapeia descritores RTL8812AU/8821AU (Jaguar), RTL8814AU (8814A) e RTL8821CU/8822BU/8822CU (rtw88), e valida o agregado inteiro antes do primeiro callback. Distingue C2H, CRC, ICV, beacons incompatíveis e canal incorreto. Não contém comandos USB, `IOMalloc`, alterações de firmware nem operações de TX.
- `tests/family_5ghz_scan_tests.cpp`: regressões de recepção para seis famílias, beacon HT/VHT80, canal incorreto, FCS alterado, CRC e descarte atômico de agregado corrompido. Verifica os 270 perfis, inclusive identidades não verificadas e perfil fabricado. **934 assertions aprovadas** em execução local.
- `docs/5GHZ-USB-RX-COVERAGE.csv`: status por VID:PID, distinguindo suporte nominal a 5 GHz de leitura **offline** do descritor e de suporte operacional da kext.
- `CMakeLists.txt`, `TahoeRealtekWiFi.xcodeproj/project.pbxproj`: fonte nova adicionada a todas as configurações de compilação relevantes; Xcode regenerado pelo script original determinístico `scripts/generate-kext-project.py` para impedir omissão silenciosa.
- `README.md`, `SOURCE-MANIFEST.sha256` e este relatório atualizados.

## Cobertura dos 270 IDs USB

| Família nominalmente dual-band | IDs catalogados | IDs com identidade verificada + parser offline de 5 GHz | Estação 5 GHz na kext |
|---|---:|---:|---:|
| RTL8192DU | 5 | 0 | NÃO |
| RTL8812AU | 37 | 34 | NÃO |
| RTL8814AU | 17 | 14 | NÃO |
| RTL8821AU | 36 | 22 | NÃO |
| RTL8821CU | 13 | 5 | NÃO |
| RTL8822BU | 30 | 29 | NÃO |
| RTL8822CU | 2 | 2 | NÃO |
| **Total dual-band nominal** | **140** | **106** | **0** |

Os outros 130 IDs do catálogo não têm capacidade nominal de 5 GHz comprovada nesta matriz. `RTL8814BU` continua sem identificação verificada, sem extrapolação de silício. Os 106 perfis não representam 106 adaptadores testados em hardware; somente a disponibilidade de decodificação **sem acesso ao rádio**.

## Verificações concluídas

- ZIP original e respectivo SHA-256 conferidos antes da alteração.
- 90 testes rápidos + teste do pacote do app + matriz USB: **92/92 grupos de teste portátil aprovados** (Linux, `-fsanitize=undefined`, execução do conjunto rápido e dois testes longos separados).
- Testes novos e preexistentes de recepção com `-fsanitize=address,undefined`: **3/3 aprovados**.
- Sintaxe C nova com Clang `-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`: aprovada.
- Teste de cobertura Xcode e linkage da kext: aprovado após inclusão do novo arquivo C no projeto.
- `kext/`, `vendor/`, `app/`, firmwares e scripts de assinatura originais permanecem byte a byte idênticos à base (exceto a lista de compilação do projeto Xcode).

## Pendências obrigatórias para entrega de 5 GHz FUNCIONAL

1. **RTL8192DU** — decodificador RX correto, power-on/eFuse, firmware, RF 5 GHz, estação, USB TX/RX e validação 802.11n.
2. **RTL8812AU e RTL8821AU** — completar boot/RF com perfis de placa certificados, associação macOS, WPA2, TX USB, 20/40/80 MHz com VHT e validação do sinal.
3. **RTL8814AU** — implementar RF 5 GHz, tabelas de potência específicas de região/placa, calibração 5 GHz e caminho de taxa HT/VHT/80 MHz; neste source `set8814Channel` ainda rejeita canais acima de 11 e o PHY ativo permanece 2,4 GHz / 20 MHz.
4. **RTL8821CU, RTL8822BU, RTL8822CU** — portar/integrar seus drivers RF/PHY/firmware e estação de forma específica a cada silício, sem tratar descritores iguais como prova de calibração equivalente.
5. **Todos** — regras regulatórias locais, DFS/CAC quando aplicável, WPA, estabilidade, desconexão USB, teste em Mac real com cada chipset e medição de largura/MCS/NSS/throughput real.

**Não é seguro editar somente uma tabela de canais, a string de capacidade ou o limite PHY para fingir essa implementação.** Por isso, nenhuma família ou ID foi reclassificado como 5 GHz funcional.


---

## Histórico: `docs/USB-TEARDOWN-FIX.md` (SHA-256 `68c8f7af46b3c47cbd9235b753252492912d635d937ac586127e3f30d1afa231`)

# USB hot-unplug lifecycle correction — driver 0.13.19 build 39

## Reproduced evidence (user-reported; build 38)

- `kmutil showloaded` reports `org.research.TahoeRealtekWiFi (38)`.
- With both USB adapters unplugged, three `RTWUSBController` objects remain
  `!registered`, `!matched`, `inactive`, `busy 1`, with retain counts 9/6/9.
- Two `802.11n NIC` USB nodes remain `inactive`, `busy 2`.
- The preceding `dmesg` query had no matching driver lines, and the unified
  log showed temporary kext history lookup errors but no disconnect traceback.
- Apple IOKit `IOService::terminate` semantics defer calling a client's
  `stop()` until that client closes an opened provider. The original code
  opened `usb_` in `start()`, but only closed it in `releasePlatform()` after
  `stop()`. This is a real circular lifecycle dependency. Source:
  https://github.com/apple/darwin-xnu/blob/main/iokit/IOKit/IOService.h

The exact contribution of any outstanding asynchronous USB callback to the
observed retain counts **cannot be determined** from these logs alone.

## Narrow source change

- `kext/RTWUSBController.cpp`: the `Terminated` gate operation now sets
  `closing_` and `present_` appropriately **before** quiescing and aborting
  transfer submissions. Both termination and request-to-close events now
  safely close the opened provider **after the gate is released**.
- `closeUSBProvider()` atomically claims `usbOpened_` under `lifecycleLock_`
  and retains the provider across `close(this)`. In `message()`, an explicit
  controller retain protects against immediate `stop()` / teardown.
- `releasePlatform()` continues to own the fallback close for early start
  failures; `usbOpened_` is consumed only once. Pending DMA buffers are still
  protected by the existing `retainedForPending_` deferred cleanup.
- Source build is identifiable as **39**, not 38. The app remains **0.2.13
  build 33**. No USB catalog, driver personalities, firmware, security,
  networking policies, or app behavior were changed.

## Executed portable validation (Linux)

- Original 528-file source manifest: **528/528 valid before edits**.
- CMake Debug portable build with UBSan: **built successfully**.
- **86/86** portable CTest groups passed across complementary runs; three
  longer tests (`rtw-app-pkg-tests`, `rtw-usb-wifi-catalog-tests` and
  `rtw-usb-wifi-matrix-tests`) were re-run and passed individually after
  concurrent test runs exceeded the environment's execution time limit.
- `rtw-usb-termination-tests`: compiles the actual `message()` and
  `closeUSBProvider()` methods using host IOKit doubles; validates
  synchronous `stop()` during `close()`, once-only provider close, no
  close under a held command gate, failure handling, concurrent close
  attempts, and 10,000 iterations.
- `rtw-deferred-cleanup-tests`: 10,000 iterations of safe deferred teardown
  with pending USB callbacks.

**Not executed:** native Xcode/kernel SDK build, actual kext signature,
installation, live USB hot-unplug/replug, RF or network hardware tests.
The release contains the **full source**, not a prebuilt kext.

## Mac verification (requires installing a separately built/signed build 39)

1. Build natively on the intended Mac with `./build.command --kext`.
   Follow the existing signing/installation policy; no SIP/AMFI changes are
   performed by the source bundle. Confirm build and signing results.
2. Reboot to clear prior build 38 kernel objects before beginning a clean
   hot-unplug test; do not mistake old retained objects for build 39 failures.
3. Check the loaded version with
   `kmutil showloaded | grep -i 'org.research.TahoeRealtekWiFi'`;
   the result must show `(39)`.
4. With each dongle attached, record `ioreg -p IOService -r -c
   RTWUSBController -w0 -l`; physically unplug each device (no software
   eject required), then run the same command after the USB stack settles.
   No `RTWUSBController` instances for removed adapters should remain.
5. If an inactive/busy object persists, preserve the full `ioreg` output,
   an exact unplug timestamp, and kernel logs. Do not force-release an
   outstanding DMA transfer without proving its USB callback is complete.

The physical outcome is deliberately **unverified** until performed on
macOS with this source compiled, signed and loaded.


---

Arquivo de inventário legado `docs/source-inventory.csv`: SHA-256 `d778f88e41fd94d9801102d8f99f55ade5125ac906705b17dc6425d153fbb8c3`; substituído pela matriz vigente.

---

## 2026-10-24 — Implementation checkpoint: 5 GHz management across seven families

**Input:** `TahoeRealtekWiFi-Source-Fixed.zip`, SHA-256
`d901747ce73a9f3e466b9d3a52975b5dda7185130fe7f0069df05aeccc8b13a5`.
This is the latest complete source package supplied in the conversation; no
`.git` history is included, so commits/branches cannot be verified here.

**New, implementation-level work:**

- `include/rtw/Family5GHzStation.h` and `src/Family5GHzStation.c` introduce a
  shared IEEE 802.11 management join session, using the existing
  `rtw_r12au_join_*` authentication and association builders. The new API
  performs a binding between immutable, verified USB profiles and observed
  5-GHz APs. It accepts all seven hardware dual-band families individually.
- The validator rejects copied/fabricated/unverified USB identities; rejects
  2.4-GHz-only chipsets, mismatch between tuned channel and AP, bad 20/40/80
  MHz primary/center geometry, unsupported 80-MHz requests on RTL8192DU,
  contradictory HT/VHT announcements, invalid security and PMF-required APs.
- `src/FamilyBackend.c` now advertises **offline management support**, not
  operational station support, for the five additional dual-band families.
  `RTW_BACKEND_CAP_KEXT_STATION`, station 5-GHz bands and TX authorization are
  **unchanged**. RTL8192DU has no trusted USB RX descriptor; only offline
  IEEE 802.11 management preparation is available for that chip.
- `tests/family_5ghz_station_tests.cpp` executes all seven family management
  transitions, produces actual 802.11 auth and association request bytes and
  validates synthetic AP responses through to `WPA2_KEYS_REQUIRED`. A second
  test starts from the existing USB bulk-decoder callback for six families.
  It also tests all 270 catalogued USB identities; **1,100 assertions**.
- Added the portable C source to CMake and to the x86_64 Xcode kext source
  phase using `scripts/generate-kext-project.py`, without editing signing
  scripts or the existing interface/UI.

**Real validation:** CMake portable build succeeded; 91/91 non-long CTest
cases and 2/2 long cases passed; the four focused tests ran with
ASan+UBSan; final 5-GHz management integration test passed with
ASan+UBSan. New C implementation passed Clang with
`-std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`.
Actual macOS Xcode compilation, ad-hoc `codesign`, physical radio tests,
iperf and TX-power calibration **were not performed**.

**Explicit non-claims:** No RTL8192DU/RTL8812AU/RTL8814AU/RTL8821AU/
RTL8821CU/RTL8822BU/RTL8822CU adapter is demonstrated to transmit, associate
or connect on 5 GHz in this build. Negotiation state is not proof of keys
installed or an authorized RF configuration. DFS channels remain tagged
passive-only; no DFS-CAC or regulatory transmit permission is implemented.

**Checkpoint / exact next implementation point:**

1. `kext/RTW8814Backend.inc`: map verified 5-GHz `ieee80211_channel`
   selection to RF/PHY. Current `set8814Channel()` still uses 20-MHz/2.4-GHz;
   do not remove this gate before RFE board, power tables and IQK support.
2. `src/RTL8814AUPHY.c` and `src/RTL8814AUAlgorithms.inc`: port and trace
   verified per-board 5-GHz RF/PHY configuration, power regulatory tables,
   correct HT/VHT TX rates and calibration, then test with physical hardware.
3. `src/RTL8812AUStation.c` / `src/RTL8812AURadio.c`: integrate management
   frames with runtime USB TX/RX and WPA2 key installation for 8812AU/8821AU.
4. RTL8821CU/8822BU/8822CU: implement individual runtime firmware/PHY and
   TX/RX, never assuming common RX descriptors imply identical RF hardware.
5. RTL8192DU: establish the USB RX codec and the unique PHY/firmware path.
6. All: use macOS regulatory/channel control (including DFS-CAC where needed),
   verify ad-hoc signing, test link/throughput/disconnect on each chip in 5 GHz.

**Release status:** source-only development checkpoint. Not a 5-GHz-ready
or Release-certified driver. No additional unrelated functionality was changed.


---

## 2026-10-24 — Etapa 4: sintonia física 5 GHz RX-only no RTL8814AU (kext build 42)

**Base confirmada:** Stage 3, SHA-256
`2d1c3fb10274e4328cb5cb005108b2aae0451f27c69f88f77975f46a60b6097f`,
550 arquivos; sem diretório `.git` para confirmar commits. A árvore desta
entrega parte da base imutável, sem substituir firmwares ou código de terceiros.

### Inspeção e causa

`src/RTL8814AUPHY.c::rtw_8814au_phy_set_channel` aceita apenas 1–11/20 MHz e
aplica tabelas de potência **somente de 2,4 GHz**. O scanner do backend
`kext/RTW8814Backend.inc` não incluía canais de 5 GHz. Foram examinados
esses caminhos, as funções auxiliares de RF em `RTL8814AUAlgorithms.inc`,
os registos `RTL8814AURegisters.inc`, as transições de net80211, a interface
de calibração e o esquema de RFE 0/1. O chaveamento de banda/canal foi
contrastado com `rtw8814a.c` BSD-3-Clause do driver Linux `rtw88`, commit
documentado no projeto `0e135910deec43ffe97eca0b6af1f8f847285b59`.

### Implementação realizada — não confundir com associação funcional

- `rtw_8814au_phy_set_passive_5g_channel` programa os quatro caminhos RF,
  pinmux específico RFE0/RFE1, banda CCK/OFDM, CLKTRK, AGC e canal, para
  **36/40/44/48 a 20 MHz**, com as transferências reais de registradores
  realizadas pelos callbacks I/O do PHY existente.
- `REG_TXPAUSE=0x3f` é aplicado antes de reprogramar RF e permanece aplicado;
  `phy_initialized=0` e `channel_set=false` bloqueiam calibração de estação e
  TX em 5 GHz. Não há qualquer escrita nas tabelas 2G de potência de TX.
- O controlador registra apenas canais `IEEE80211_CHAN_PASSIVE` de 5 GHz;
  escaneamento ativo pula esses canais. `state8814` chama o novo caminho
  exclusivamente no estado SCAN e retorna `ENOTSUP` para AUTH/ASSOC/RUN.
- Ao voltar a um canal de 2,4 GHz, exige `set8814Channel` e calibração real
  antes de voltar ao escaneamento/estação normal.
- Kext `CFBundleVersion`/projeto Xcode avançados de 41 para **42**, aplicativo
  permanece **0.2.13 build 38**. Nenhuma alteração na política ad-hoc.

### Regressões executadas

- Source PHY + fake USB register bus, sob ASan/UBSan: RFE 0/1, quatro caminhos,
  4 canais permitidos, TXPAUSE invariável, ausência de índices TX 2G,
  rejeição de canais DFS e larguras 40/80 MHz, falha injetada de I/O,
  recuperação e recalibração de 2,4 GHz: **aprovado**.
- Máquina de estados real extraída de `RTW8814Backend.inc`, com fronteiras
  isoladas, sob ASan/UBSan: entrada SCAN 5 GHz, AUTH recusado e retorno a
  2,4 GHz com calibração: **aprovado**.
- Compilação CMake portátil: **aprovada**.
- CTest: **91/91** testes rápidos concluídos e **2/2** longos concluídos
  separadamente (matriz USB e pacote do app), total **93/93 grupos aprovados**.
  Uma execução conjunta excedeu o tempo disponível durante o teste longo;
  ele foi repetido e terminou aprovado, não foi contado pela interrupção.
- Script original regenerou o projeto Xcode com **82 unidades** e a única
  diferença gerada no `.pbxproj` foi `MODULE_VERSION=41 -> 42`.
- **Não executados:** build nativo macOS Tahoe, codesign real de app/helper/kext,
  varredura 5 GHz em hardware, radio calibration/DFS/Tx power/iperf.

### Status de Release e próximo checkpoint

**Não pronto para Release como driver 5 GHz.** Trata-se de implementação
**RX-only 5 GHz**, ainda não validada fisicamente. A recepção de beacons pode
falhar por insuficiência de calibração RX/RF em placas reais. Não foram
habilitados TX/association, largura HT40/VHT80 ou 5 GHz nas outras seis
famílias. A API comum Stage3 para sete famílias segue preservada, mas não
constitui conexão operacional.

Próximo ponto exato: `src/RTL8814AUPHY.c::power_tables` ainda rejeita
explicitamente todas as linhas `r->band != 0` e só calcula limites de 2 GHz;
`RTW8814Backend.inc::set8814Channel` continua bloqueando o TX 5 GHz. É
necessário implementar e validar por RFE os limites 5 GHz, tabela de TX,
IQK/LOK, seleção primário/centro 40/80 MHz, H2C/TX/RX, WPA2 e regulação,
com adaptador físico. As outras seis famílias precisam de backends próprios
(RTL8192DU, RTL8812AU, RTL8821AU, RTL8821CU, RTL8822BU, RTL8822CU).

A análise desta etapa foi **direta, mas parcial**, concentrada nas funções
acima; a matriz não declara inspeção integral de todos os arquivos/linhas.


---

## 24/10/2026 — Auditoria complementar: aliasing 5 GHz, kext build 43 / app build 38

**Referência verificada:** ZIP de entrada `TahoeRealtekWiFi-Source-Full-Build42.zip`, SHA-256 `62f8037364eb66163db9f0fabdd8cd6b447d2544cf561cb7b6347c6e239635b8`; 550 arquivos no pacote de entrada, manifesto original verificado sem divergências. Não existe `.git` no pacote, portanto não são conhecidos commits/branches. O pacote original permaneceu imutável. Nova compilação macOS não está disponível neste ambiente.

### R1 — Inspeção semântica direta e diagnóstico

Foi examinada a lógica concreta de `src/Family5GHzStation.c`, `src/FamilyRadioPolicy.c` e `src/RTL8812AUStation.c`: validação de argumentos, identidade USB, estado de AP, geometria de canais, escrita em objetos de saída e entradas emprestadas. Foram examinados parcialmente `kext/RTWUSBController.cpp` (teardown/refs/callbacks), `kext/RTW8814Backend.inc` (scan passivo, transições SCAN/AUTH/ASSOC/RUN), `src/RTL8814AUPHY.c` (TXPAUSE, retorno à banda 2,4 GHz), `app/DriverConnection.mm` e `app/Transport.cpp` (respostas assíncronas/XPC/IPC), `build.command`, `app/signing.sh`, regras CMake e scripts de geração Xcode. Leitura não equivale a validação de cada ramificação. Não foram encontrados defeitos adicionais reproduzidos nesses trechos de teardown/GUI durante esta rodada.

### R2 — Três problemas reproduzidos antes de corrigir

| Defeito | Código e causa raiz | Efeito comprovado com entrada sobreposta | Correção mínima |
|---|---|---|---|
| S5-01 | `rtw_family_5g_join_prepare`, `src/Family5GHzStation.c`: `rtw_family_5g_join_reset(out)` em erro antes de verificar sobreposição com `profile`, `ap`, `station_mac`. | Destruição indevida de bytes de entrada declarada somente-leitura quando o chamador reutiliza a mesma região. | Rejeitar sobreposição antes de qualquer escrita; preservar o comportamento anterior em todos os casos válidos e inválidos sem alias. |
| S5-02 | `rtw_family_radio_prepare_profile`, `src/FamilyRadioPolicy.c`: `memset(out)` antes de validar alias com a identidade USB. | Cópia sobreposta do perfil forjado era apagada ao retornar `UNSUPPORTED`. | Rejeição de alias perfil/saída antes de reset do plano; identidade USB continua exigindo ponteiro do catálogo. |
| S5-03 | `rtw_r12au_scan_parse`, `src/RTL8812AUStation.c`: saída zerada antes de verificar overlap com descritor RX ou payload. | Bytes do beacon ou descritor podiam ser modificados pelo parser quando saída e entrada se sobrepunham. | Rejeição explícita de overlap antes do `memset`; demais regras 802.11, RSN e canal preservadas. |

Reprodução: árvore de source 42 sem correções, com os três testes de regressão novos; `ctest` retornou código 8, **3/3 falhas esperadas**. Árvore corrigida: três testes passaram com ASan/UBSan. As verificações de sobreposição convertem ponteiros para `uintptr_t` e comparam distâncias, sem somar comprimentos a endereços, evitando wraparound nesse cálculo. Não há mudança de API pública ou no layout das estruturas.

### R3 — Compilação real e testes de regressão

- CMake Debug portátil construído com `RTW_HOST_SANITIZERS=address,undefined` e `RTW_ENABLE_LIBUSB=OFF`: **build concluído**.
- **93/93 grupos CTest aprovados em três execuções concluídas**: 91 rápidos, 1 matriz de adaptadores (270 IDs) e 1 empacotamento do app. Os logs estão resumidos no arquivo único de evidências.
- Três fontes alterados passaram em Clang C11 com `-Wall -Wextra -Wconversion -Wshadow -Wpedantic -Werror` (nenhum diagnóstico de compilação desses três arquivos). `clang --analyze` apontou uma possibilidade condicional preexistente em `FamilyRadioPolicy.c: profile->family`; a implementação verifica `caps.identity_verified` antes do acesso, retornando `UNSUPPORTED` para `profile=nullptr`. Esse aviso **não foi classificado como crash comprovado** nem corrigido especulativamente.
- Após atualização de build, cinco testes direcionados (3 de código, assinatura e personalidade kext) passaram novamente.
- **Patch único** `docs/diffs/CORRECOES.patch` aplica-se especificamente ao ZIP build 42 informado acima; sua aplicação foi testada em cópia original e os 10 arquivos corrigidos/atualizados reproduziram a árvore resultante byte a byte. O manifesto do ZIP entregue é regenerado separadamente e cobre inclusive o patch.

### Integridade e alterações

- Alterados 3 arquivos de implementação (todos relacionados à entrada/saída de 5 GHz), 3 arquivos de testes, `kext/Info.plist`, projeto Xcode gerado, `README.md`, `CHANGELOG.md`, e os quatro artefatos de auditoria/manifesto mantidos em seus caminhos únicos. Nenhum arquivo original foi removido, e `vendor/`, firmwares, `app/` e lógica TX/RF permanecem idênticos ao build 42.
- A kext passou de build **42 para 43** para distinguir a compilação potencial dos fontes alterados; app continua build **38**. A atualização do `.pbxproj` corresponde à versão do plist, com **82 unidades de compilação**.
- Código de assinatura ad-hoc obrigatória (`app/signing.sh`, scripts de build/installer) preservado; o teste da política passou, mas `codesign` efetivo não foi executado.
- A matriz `docs/AUDITORIA-COBERTURA.tsv` mantém linhas por arquivo/função e diferencia expressamente leitura, inspeção e validação; hashes de arquivos modificados são atualizados, e autorreferências não recebem hash fictício.

### O que NÃO foi comprovado — status Release e próximo checkpoint

**Cobertura semântica integral não atingida; Release macOS não validado.** Não foram executados build nativo em Xcode/macOS Tahoe, assinatura ad-hoc efetiva, testes visuais AppKit claro/escuro, testes físicos USB/hotplug, Instruments para leaks/memória/CPU, benchmark real, RF 5 GHz ativo, calibração e DFS. Os testes portáveis não substituem essas verificações. Nenhuma das sete famílias dual-band possui estação 5 GHz funcional comprovada; RTL8814AU mantém exclusivamente sintonia 5 GHz RX passiva experimental (36/40/44/48, 20 MHz, TX bloqueado).

**Checkpoint exato para a próxima auditoria:** `kext/RTWUSBController.cpp` — fechamento de provider após terminação, `shutdownPlatform()`, `deferredCleanup()`, races com `IOCommandGate`, referências mantidas por callbacks e testes com adaptadores reais; depois `app/Main.mm` — geração de requests, cancelamento Keychain, UI/tema e medições macOS. Para 5 GHz ativo, implementar, por hardware/board, RF/PHY, tabelas de potência, calibração, HT/VHT, firmware, WPA2/TX/RX e política regional antes de liberar transmissão. Não declarar conclusão com base em simulação ou leitura de beacons.
