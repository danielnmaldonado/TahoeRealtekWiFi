# 0.14.14 / build61 — app0.3.11 / build53 — macOS13/14/15/26

- App, helper, testes e instalador passam de mínimo26.0 para13.0; os alvos
  são Ventura, Sonoma, Sequoia e Tahoe em Intel. Identificadores e preferências
  são preservados.
- Build trata uso desprotegido de API posterior ao mínimo como erro. O
  empacotador verifica também a versão mínima dos executáveis Mach-O.
- Kext61 e coletor revisão2 preservados. Matriz de compatibilidade distingue
  auditoria estática, build e teste físico; os quatro sistemas ainda exigem
  homologação de carregamento, WPA2 e tráfego desta entrega.

# 0.14.14 / build61 — app0.3.10 / build52 — coletor revisão 2

- Reabertos os 14logs originais e conferidos 784 hashes de arquivos, além de
 7.368 propriedades (ver relatório), causas e limitações por captura.
- O coletor preserva rc1/assessments disabled como estado esperado e faz uma
 tentativa recente separada de 2 min somente após timeout124 no histórico.
- Cobertura registra status e timestamps recebidos; histórico parcial continua
 parcial. Relatórios de crash são identificados explicitamente como históricos.
- Preservados21 testes anteriores e adicionados 9, com revisão independente.
- Kext/app mantêm o código da61. RX nativo zerado, causa CCMP+2, duas falhas
 físicas e throughput continuam abertos ou sem comprovação de resolução.

# Rodada61 — mudanças anteriores a esta revisão do coletor

- CU/88E: HT20/MCS/QoS/BA/SGI; CU com RA de firmware e limite de SMPS do peer;
 88E com AMRR host a partir de relatórios CCX reais.
- B/C: HT40/VHT20/40/80 em 5 GHz não DFS, com largura/PHY/RA/RX/coexistência/BA.
- AU: correção do layout de potência 80 MHz conforme referência Linux fixada.
- net80211: capacidades/negociação limitadas ao mapa do canal de destino.
- Native RX/TX: amostras de observação antes/depois da entrega, sem forçar contagem.

# 0.14.13 / build60 — app0.3.9 / build51

- RTL8723BU: quatro OUT físicos, três filas DMA; limites/rollback e Bluetooth
  preservados. TupleFF/FF/FF exata no perfil7392:a611.
- Descoberta USB: estágios/contagens permanecem no provider original retido,
  inclusive após revogação e falha anterior ao controlador.
- AU8812/8821: reset limitado de MCU residente conforme Linux; propagação de
  timeout/erro e telemetria de boot/reset/calibração sem relaxar guardas.
- CacheIE: região de cópia exata, alocação transacional/limpeza e limites de
  leitura preservando fluxo RSN. Testes somente de memória local.
- Coletor: startup USB/AU em resumo/JSON; análise individual dos 14logs e todos
  os contadores não nulos, com distinção entre taxa observada e throughput.
- App51: catálogo USB atualizado; preferência/reconexão mantêm o código anterior.

# 0.14.12 / build59 — app 0.3.8 / build50

- Publica `IOBuiltin = Yes` como OSBoolean na interface de rede de todos os
  adaptadores suportados, durante configureInterface e antes de registerService.
- Marcação de apresentação na interface; sem definir IOPrimaryInterface,
  alterar identidade/propriedades USB, nome BSD ou prioridade dos serviços.

# 0.14.11 / build58 — app 0.3.8 / build50

- Persiste a placa por identidade física e resolve RegistryIDs após reboot;
  mantém a preferência com enumeração atrasada e rejeita identidades ambíguas.
- Rede confirmada por placa, incluindo uso único sem guardar senha; migração
  conservadora de um único link já autorizado na atualização do app antigo.
- Epoch de reconexão cancela lookup/requests automáticos ainda na fila após
  desativar a opção. Preferência de rede e senha no Chaves continuam distintas.
- Busca para listar redes sem JOIN encerra ao fim da varredura completa,
  preservando cache/recepção e os retries de associação dos perfis JOIN.
  Callback opcional mantém o caminho net80211 existente quando não consumido.
- Remove timer de coexistência B/C antes de liberar o WorkLoop compartilhado,
  corrigindo retenção quando outra placa mantém o loop ativo.
- Reutiliza o formatador de timestamp com sincronização. Benchmark local de
  10 mil registros reduziu CPU de cerca de 1,1 s para 20–24 ms.
- Confere revisão/conteúdo do SDK; exclui diretórios gerados build-*/out-* do Git.

# 0.14.10 / build57 — app 0.3.7 / build49

- Busca real automática por adaptador selecionado quando ocioso e cache RF sem
  conclusão fresca. Coalescimento, geração/epoch, timeout30s e retries limitados.
- Placas conectadas mantêm consulta de cache; nenhum JOIN/senha novo por refresh.
- RX inicial: validação de quatro buffers, clear limitado e drain/proveniência
  ampliados aos 13 backends de estação, preservando a capacidade de cada família.
- Parada comum preserva erros tardios de RX/TX e gerações antes de reutilizar
  buffers; TX curto mantém Underrun. Callbacks após o preflight são verificados.
- Shutdown verificado e identidade próprios para recuperação das famílias;
  legado CU/8188EU/8192EU recebe power-off das fontes Linux fixadas.
- Diagnóstico publica capacidades ifnet/Skywalk para investigar RX nativo zero;
  nenhum contador nativo é incrementado manualmente.
- Checkup da build56 encontrou ambos autorizados e HTTPS separado funcional:
  RTL8814AU em 5 GHz/canal36 e RTL8192EU em2,4GHz/canal6. Não valida a build57.

# 0.14.9 / build56 — app 0.3.6 / build48

- RTL8814AU: retry manual SCAN/JOIN após erro elegível, somente depois de
  drenagem, desligamento verificado e reidentificação do mesmo dispositivo.
  Orçamento limitado; erro original separado do resultado da limpeza.
- Corrige stop/start antes do fim dos callbacks e finaliza desligamento após
  a última conclusão, sem iniciar rádio ou contornar erro USB/perfil/identidade.
- RX inicial AU: NotResponding recebe limpeza limitada e EAGAIN, sem sucesso
  falso. Proveniência sobrevive a EBUSY e erros tardios distintos ficam fatais.
- IOKit: registro de energia no registerWithPolicyMaker, antes de attach,
  com pedido inicial de estado1 e PMstop antes de liberar recursos em falhas.
- Diagnósticos exportam core errno, estados reais e fases de inicialização AU.
- App48: CLI autenticada por ID exato para LIST/STATUS/MAP/SCAN e JSONL privado,
  sem credenciais. Logs não podem bloquear em FIFO ou lock de outro processo.
- Fixtures de diagnóstico usam home isolado e não poluem o log do app instalado.
- Teste físico na55 reproduziu SCAN recusado em0,765ms, USBerror0; código da
  primeira falha não era publicado. Validação física da56 continua necessária.

# 0.14.8 / build55 — app 0.3.5 / build47

- DFS HT20 nos sete backends dual band: RTL8192DU, RTL8812AU, RTL8814AU,
  RTL8821AU, RTL8821CU, RTL8822BU e RTL8822CU. Canais52–64/100–140;144 fechado.
- Índices de potência/calibração próprios, grupos OTP AU corrigidos, spur140
  e grupo térmico100–140 no8814AU; fallback20 em APs largos nos AU correspondentes.
- Busca passiva, autorização ligada a beacon recente do BSSID/SSID/canal e
  teto local de potência limitado por ROM/país, world, Country/PowerConstraint doAP.
  wireless-regdb primário fixado; nenhuma substituição de país por beacon/locale.
- CSA pausa todas as filas TX, inclusive MAC1 DU, drena USB e refaz busca/WPA2
  fora da chamada RX. Teardown/sleep/falhaUSB cancelam a retomada.
- App47 valida e descreve os grupos DFS anunciados, mantendo ABIv1/driver antigo.
- Consulta exata de canais registrados preserva faixas regionais parciais;
  mapa por adaptador/geração e fallback somente em driver antigo Unsupported.
- Catálogo USB inalterado; não habilita8814BU, WPA3/AX, AP ou CAC/radar local.
  Carregamento e validação física da nova build permanecem necessários.

# 0.14.7 / build54 — app 0.3.4 / build46

- Fila TX cheia preserva ownership e retorna stall/retry; conclusões retomam o
  output queue de forma assíncrona quando a admissão pode avançar.
- STATUS observa HT/BA em todos os backends, separado da política de rate control.
- Publicação por controlador ganha sequência ímpar/par e tempo monotônico;
  coletor distingue geração avançada, igual, incompleta ou em andamento.
- Expõe eventos existentes de BA/reorder e a fronteira de entrega RX/ifnet;
  não atribui os sete erros históricos a uma causa sem evidência.
- Remove metadado Ethernet54Mb/s inventado; taxa PHY incompleta fica desconhecida.
- Source inclui lista integral de identidades elegíveis e limites por família.
- A build53 foi observada com tráfego rápido e sem erros USB/truncamentos novos.
  A validação física da54 e a causa da contabilidade RX0 no BSD permanecem abertas.

# 0.14.6 / build53 — app 0.3.4 / build46

- RTL8814AU: buffers/leituras RX 32 KiB compatíveis com o threshold DMA 20 KiB;
  alocação, início, rearm e recuperação usam a mesma política por família.
- Regressão real reproduz corte 16 KiB de agregado válido 20.200 bytes; 32 KiB
  preserva todos os 13 quadros. Demais famílias mantêm a capacidade anterior.
- TX AMPDU AU completo: limite do peer, BA por TID, QoS/ACK/fragmentos, densidade,
  contagem em unidades primárias e checksum/seq; falha de I/O recusa BA.
- Tracking termal AU preserva taxa aceita por C2H; TX não sobrescreve com taxa
  inicial/fixa do descritor, inclusive gerenciamento/EAPOL/TX rejeitado.
- Novas métricas escalares de comprimento RX; mantém coletor logs2.
- App46 preservado. Validação física de BA, agregação no ar e velocidade da
  build53 permanece pendente; não há alteração de EFI/driver carregado.

## 2026-10-31 — coletor logs2 (mantém kext52/app46)

- Explicita que IORegistry contém últimos valores publicados, possivelmente
  antigos; delta0 não comprova ausência de tráfego ou de erros novos.
- Não força STATUS privilegiado nem altera driver, rede ou autenticação.

## 2026-10-31 — coletor logs1 (mantém kext52/app46)

- WiFi-Diagnostics.command salva em Biblioteca/Logs/Tahoe-WiFi-USB, fora do Desktop.
- Captura e compara os dois controladores por Registry ID, incluindo família,
  VID:PID, interface e contadores USB/RX/TX; distingue reset e remoção.
- Limita o tempo dos comandos, preserva saída parcial/status124 e continua o ZIP.
- Mantém coleta somente leitura e reforça mascaramento de MAC em formato de dados.
- Não altera o driver/app nem resolve por si só o erro USB 0xe00002ed.

# 0.14.5 / build52 — app 0.3.4 / build46

- Revisão dos 13 backends operacionais contra fontes primárias e testes específicos.
- RTL8192EU: corrige descritor gen2 HT40/SGI e subcanal 20 em 40 MHz; mantém os campos RTS/retry intactos e os gates gen1.
- RTL8812AU/RTL8821AU: corrige PHY após SHIFT; valida RSSI no codec e nos consumidores de firmware/coexistência.
- RX24 genérico: usa a disposição PHY correta por família; preserva rtl8xxxu legado.
- RTL8192FU: preserva sequência 80211 de software em gestão/EAPOL/não-QoS.
- Coexistência AU/B-C: não perde a solicitação LNA desejada quando a consulta MP está ocupada; trabalho pendente permanece por contexto e limitado.
- Preserva app46, scan real, sinal medido, ajuste 153 e telemetria da build51. Validação física de 5 GHz e outras placas/revisões continua necessária.

# 0.14.4 / build51 — app 0.3.4 / build46

- Busca explícita do app passa a solicitar scan real também quando conectado, mantendo as consultas periódicas somente leitura.
- Sinal: menu, tooltip e acessibilidade mostram dBm medido, filtrado e barras; estados antigos/ausentes permanecem distintos. A renderização dos três níveis foi verificada; −42/−44 dBm corresponde corretamente a sinal forte.
- RTL8814AU: ajuste NBI/CSI do Linux para canal 153 HT20, com verificação de registradores e transições para outros canais.
- Diagnóstico escalar separa visitas de scan, observações PHY e cache por banda, sem exportar identificadores de redes ou credenciais.
- Build50 observada com WPA2 autorizado e sem erros de recuperação USB. O canal 5 GHz do roteador automático continua desconhecido; validação física das mudanças permanece necessária.

# 0.14.3 / build50 — app 0.3.3 / build45

- Corrige falha prematura da recuperação RX quando a limpeza do endpoint retorna NotResponding transitório; novas tentativas são espaçadas e limitadas pelo orçamento e prazo total.
- Encaminha a rejeição síncrona de RX elegível para recuperação, preservando propriedade e ordem dos buffers.
- Drena também leituras parcialmente rearmadas antes de reiniciar uma tentativa; preserva clearStall(true) e a sincronização de toggles.
- Adia timers de scan/calibração/coexistência e rejeita busca direta enquanto o endpoint está em recuperação, evitando I/O de controle concorrente entre tentativas.
- Publica estágio e resultados específicos de recuperação; mantém o app45 compatível e seu source completo.
- Build49 confirmada no hardware: uma recuperação concluída e outra falha. A estabilidade da build50 ainda requer validação física.

# 0.14.2 / build49 — app 0.3.3 / build45

- Completa a fase passiva antes da seleção do AP nos sete backends de duas bandas.
- Recupera transaction error RX USB sem bytes com halt clear, drenagem e orçamento finito; mantém tratamento fatal para demais falhas.
- Mostra todas as bandas utilizáveis observadas em um SSID agrupado no app.
- Preserva catálogo, gates de placa, WPA2, canais autorizados e limites de firmware.
- A validação física da nova build permanece necessária.

## 2026-10-30 — 0.14.0 desenvolvimento (kext47, app42)

- Integrados 13 backends operacionais separados: CU, 88EU, EU, DU, FU,
  8710BU, 8723BU, 8812AU, 8814AU, 8821AU, 8821CU, 8822BU e 8822CU.
  São 230 IDs verificados elegíveis; preservados 270 registros e 39
  identidades não confirmadas ou conflitantes.
- Portados firmware, ROM, MAC/DMA, PHY/RFK, negociação, TX/RX e ciclo de
  vida próprios dos novos chipsets, a partir das fontes primárias fixadas.
- AU com HT20/40 e VHT20/40/80; DU, 8814AU e B/C com HT20; FU e 8710BU
  com HT20/40; 8723BU com HT20. Limites por placa permanecem explícitos.
- 8814AU: potência e RFK nos canais superiores 149/153/157/161/165;
  recepção física verificada no USB conectado, sem equivaler a WPA2/IP.
- Coexistência AU e B/C integrada com timers/C2H e SMPS confirmado por ACK;
  propriedade e prontidão do firmware verificadas nos estados, EDCA e
  encerramento, com autorização rápida sem I/O também nas placas sem BT.
- App anuncia separadamente os grupos inferiores/superiores de 5 GHz e
  descreve os modos implementados, conforme autorização do driver.
- Corrigidas assinaturas de firmware 8723BU e os pares SMIC/UMC 8710BU;
  diagnóstico físico genérico antigo desabilitado antes de qualquer I/O.
- Distribuição completa de source, firmwares inalterados, licenças,
  geradores e testes. A homologação física de associação, tráfego e
  coexistência continua pendente para famílias/revisões sem hardware validado.

- Confirmado ID RTL8814B B814 em fonte primária, mantendo o HAL desabilitado.

- Verificados aliases legados 056e:4010/AU e 20f4:648c/CU com fontes primárias fixadas.

- Tabelas IQK importadas tornadas privadas por módulo, corrigindo a colisão
  de símbolos no link conjunto de 8710BU e 8723BU.

## 2026-10-30 — validação nativa (kext build45, app build40)


## 0.14.0/build46 + app 0.3.0/build41 — em desenvolvimento

- RTL8814AU RFE0/1:5GHz36/40/44/48 a20MHz, potência ROM/PG/limites,
  RFK, térmica por banda, OFDM/SIFS/EDCA e HT20/MCS0–15 no net80211.
- App:capacidade5GHz por adaptador, seleção recuperada e revalidação noJOIN.
- Ports independentes AU/B/C/DU:ROM/boot/firmware/MAC/tabelas PHY/USB;
  integração e calibrações restantes continuam em desenvolvimento.
- B/C:port0/filtros/CCMP por software, EDCA, H2C com cursor comum e RA HT20;
  este módulo não publica interface de rede nem autoriza a porta WPA2.
- Corrigida a identidade CCNC0bda:2102 paraRTL8822BU, com interfaceff/ff/ff.
- Assertivas dos testes de rádio permanecem ativas também em Release.
- Sem certificação de WPA2/IP/tráfego para o pacote completo.

- Usar a KPI pública thread_call_allocate_with_options na limpeza adiada.
- Recuperar o token do adaptador selecionado após retorno do mesmo Registry ID.
- Diagnóstico RTL8192EU nativo IOKit e aceitação de múltiplas interfaces compatíveis.
- Regressões nativas e testes físicos documentados em docs/NATIVE-20261030.md.

## 2026-10-24 — USB preflight e recuperação de seleção (kext build 44, app build 39)

- `src/FamilyBackend.c`: evitar apagar descritores USB e agregados RX quando
  saída e entrada compartilham memória. Rejeitar antes da escrita.
- `app/Protocol.hpp`: após invalidação por desconexão, rejeitar tokens
  antigos e permitir reseleção do mesmo Registry ID.
- Regressões reproduzidas na versão anterior e aprovadas após correção.
- Assinatura ad-hoc, firmware, RF/TX, aparência e protocolos preservados.
- 5 GHz operacional e Release macOS ainda não comprovados.

## 2026-10-24 — auditoria de memória/aliasing (kext build 43, app build 38)

- Rejeitar sobreposição de memória entre saída e entradas somente nos fluxos
  `rtw_family_5g_join_prepare`, `rtw_family_radio_prepare_profile` e
  `rtw_r12au_scan_parse`, antes de limpar buffers do chamador.
- Adicionar regressões que falham no source build 42 e passam no build 43;
  não alterar protocolo, RF/PHY, firmware, hardware TX ou interface AppKit.
- Testes host ASan/UBSan: 93 grupos portáveis aprovados em execuções concluídas.
  Compilação macOS, assinatura ad-hoc nativa, GUI e 5 GHz ativo não verificados.
- Continuar documentando revisão semântica parcial; não declarar Release.

## 2026-10-24 — RTL8814AU 5 GHz RX passivo (kext build 42)

- Implementado chaveamento RF real em modo de recepção passiva de 20 MHz,
  canais 36/40/44/48, com RFE 0/1, seleção de banda, AGC e os quatro caminhos RF.
- Integrado aos canais passivos do scanner net80211; o caminho de retorno a
  2,4 GHz exige recalibração antes de permitir transmissão normal.
- 5 GHz permanece **sem autorização de TX, autenticação ou associação** neste
  backend; não representa 5 GHz operacional nem suporte pleno HT/VHT.
- Mantida a assinatura ad-hoc obrigatória; aplicativo permanece build 38.

## 2026-10-24 — auditoria consolidada: app build 38; kext permanece build 41

- Corrigir os transportes socket e XPC do aplicativo para limpar a resposta
  anterior quando uma requisição local inválida é rejeitada (inclusive
  comprimentos maiores que `UINT32_MAX` quando o buffer é válido).
- Acrescentar regressões das funções reais do socket e casos correspondentes
  ao teste nativo, reproduzindo a falha anterior à correção.
- Consolidar relatórios antigos no único documento
  `docs/AUDITORIA-CONSOLIDADA.md`, atualizar o inventário verificável
  `docs/AUDITORIA-COBERTURA.tsv` e guardar um diff em
  `docs/diffs/CORRECOES.patch`.
- Preservar kext, TX/RX de rádio, catálogo, firmwares, tema e política
  obrigatória de assinatura ad-hoc. Este pacote contém **somente source**;
  testes reais de macOS, `codesign`, benchmark e 5 GHz ativo pendentes.

## 2026-10-23 — radio capability matrix: driver build 41, app build 37

- Centralize silicon-family RF/band/PHY ceilings separately from modes actually
  integrated into the macOS station driver. Mark unknown RTL8814BU hardware
  unknown instead of inferring a similar chipset. All 270 immutable USB IDs
  are audited via per-profile API, CLI JSON and generated CSV/JSON inventory.
- Extend **passive-only**, fail-closed channel geometry to 13 known chipset
  families. This does not enable real 5-GHz tuning, DFS, TX, firmware, HT/VHT
  or additional USB personalities in the macOS kernel extension.
- Correct the app RTL8192EU text to HT20/HT40 and expose the reference-vs-driver
  limitations in the read-only USB diagnostics and catalog.
- Tests for each family and every USB profile; signing policy, USB data paths,
  network associations, firmware, encryption and the existing UI styling stay
  unchanged. **Full-speed/multi-band operation is not implemented by this patch.**

## 2026-10-23 — app build 36 (0.2.13), driver remains build 40 (0.13.19)

- Fix `app/Transport.cpp` legacy local socket exchange: clear stale/partially read
  output on receive EOF, truncated replies, semantic payload validation errors,
  remote error frames and write failures. Preserve shared in/out aliasing until
  the request has been transmitted; the wire format remains unchanged.
- Reproduce the original bug on build 35, then exercise the production socket
  functions extracted verbatim against real Unix socketpairs (seven scenarios,
  host ASan/UBSan), plus native source regression scenarios and a Python
  source-contract test. Add one CTest group (88 total).
- Keep all kext sources, chipset families, firmware, helper XPC behavior,
  existing UI themes and visual assets, privileges, and fail-closed mandatory
  ad-hoc signing exactly as in build 35.
- Offline build/test: 88/88 portable CTest groups passed in three batches
  (35+30+23). macOS AppKit/IOKit compilation, actual ad-hoc signature,
  physical USB hot-unplug, Instruments and live UI tests remain pending.
- Release source details: `docs/AUDITORIA-CONSOLIDADA.md`.

## 2026-10-23 — app build 35 (0.2.13), driver stays at build 40 (0.13.19)

- App: ignore delayed automatic Keychain password lookups if automatic reconnect
  has since been disabled or paused; preserve explicit manual connections.
- App: reject malformed successful XPC payloads by clearing the output buffer
  and invalidating the untrusted helper connection. Correct framing alone is
  not proof that the payload contents are valid.
- Extend reconnect-policy combinations and AppKit/XPC source-contract tests.
- Preserve the build-40 kext, USB unplug fix, firmware, network protocols,
  app layout, themes, default settings, and fail-closed mandatory ad-hoc
  signing policy byte-for-byte except for the app sources listed above.
- Offline CI: 87/87 portable test groups passed (84 together; 3 long groups
  individually); no native AppKit/IOKit compilation, signing or UI tests
  performed. Audit details: `docs/AUDITORIA-CONSOLIDADA.md`.

## 2026-10-23 — build 40 (driver 0.13.19), app build 34 (0.2.13)

- Audited all source-package file paths and contents with a reproducible
  per-file inventory, Python/Bash/plist checks, regenerated deterministic
  Xcode metadata and sanitized host regression tests.
- Driver: skip publishing unchanged `OSNumber` diagnostics in status and
  calibration paths; keep identical keys, values and 64-bit types.
- App: refresh the live menu-item states immediately for auto-reconnect and
  macOS-managed login startup; no new UI assets or theme overrides.
- CMake: correct Python interpreter discovery order; all registered test
  groups now execute. Host-only extraction shims updated, with new diagnostic
  de-duplication regressions.
- Build metadata only: kext build 39 -> 40, app build 33 -> 34; functionality
  versions and ABI remain 0.13.19, 0.2.13 and unchanged respectively.
- Keep fail-closed mandatory ad-hoc signing, all USB profiles, firmware,
  passwords, installation choices and existing build39 unplug fix untouched.
- macOS native app/kext build, actual codesign and physical hot-unplug tests
  still need to be validated on a macOS Tahoe machine.

## Driver 0.13.19 build 39 — USB hot-unplug termination (2026-10-23)

- On `kIOMessageServiceIsTerminated` / `kIOMessageServiceIsRequestingClose`,
  quiesce the transfer engine, block new commands, then close the opened
  `IOUSBHostInterface` outside the `IOCommandGate`. IOKit can now enter
  `stop()` instead of waiting on an open provider indefinitely.
- Claim the provider-open state under `lifecycleLock_` before closing it.
  A synchronous/reentrant `stop()` cannot cause a duplicate `close()`.
  Hold the controller and provider references across the call.
- Retain the original deferred DMA/completion cleanup mechanism; do not
  release memory belonging to transfers whose callbacks are outstanding.
- Add a host regression test exercising the actual `message()` and
  `closeUSBProvider()` functions, synchronous stop/reentrancy, concurrent
  close attempts, and 10,000 lifecycle iterations.
- Driver kext build changed from 38 to 39; **app stays at 0.2.13 build 33**.
  USB matching profiles, firmware, network stack, security and app are unchanged.
- Portable host test groups: 86/86 passed in split runs with UBSan for
  the CMake build. Native Xcode build, code signing, and physical unplug
  on macOS Tahoe have not been performed on this Linux host.

See `docs/AUDITORIA-CONSOLIDADA.md` for scope, evidence and validation steps.

## Driver 0.13.19 — guarded legacy firmware upload stage (2026-10-23)

- Added `LegacyFirmwareStage` with validated firmware format, bounded
  4096-byte pages/254-byte vendor transfers and 8-page capacity. Planning
  supports RTL8192FU, RTL8723BU and RTL8710BU; other families fail closed.
- Implemented opt-in **experimental, userspace-only** 8051 firmware download
  for RTL8192FU/RTL8723BU: power-state preconditions, bounded polling,
  stale checksum detection, USB failure propagation and best-effort cleanup.
  RTL8710BU stays planning-only because SoC reset/power control differs.
- Added optional `rtw-legacy-fwdiag` (libusb) with read-only default,
  no driver detachment and explicit hardware-write confirmation; corrected
  USB interface claim to use the validated profile's interface number.
- Extended `rtw-backends` JSON with firmware planning and experimental boot
  flags, independent of `kext_station_integrated`.
- Added portable firmware staging tests and regression cases; **85/85**
  host tests passed with UBSan. New C module passed Clang static analysis;
  optional USB tool only passed syntax validation against test declarations.
- Preserved all 270 catalog profiles, 122 existing kext station-eligible
  profiles, English app 0.2.13, ad-hoc signing policy and retained firmware
  and upstream license notices. No new families were attached to macOS.

**Not completed:** native Xcode compilation, actual signing, live USB
firmware startup, RF calibration, association, WPA2 and physical validation.

## Driver 0.13.18 — all-family backend iteration (2026-10-23)

- Added allocation-free 24-byte RX descriptor/aggregate validation for RTL8192FU,
  RTL8723BU, RTL8821CU, RTL8822BU and RTL8822CU (offline; no station TX).
- Added family-specific legacy 8051 firmware header preflight for RTL8192DU,
  RTL8192FU, RTL8723BU and RTL8710BU. Prevents accepting cross-family
  signatures and truncation; does not authenticate or load firmware.
- Added passive-only channel geometry preflight for six unfinished families,
  with conservative band, width and DFS checks.
- Added explicit separate capabilities in `rtw-backends --json`, maintained
  all 270 catalog entries and the existing 122 kext station personalities.
- Added firmware/radio negative tests, bounded RX fuzzing and CLI capability
  contracts. No unsupported USB aliases were newly enabled.
- App remains version 0.2.13 with English UI and mandatory ad-hoc policy.
  Native macOS/Xcode, real codesign and hardware testing are pending.

## 0.13.17 / app 0.2.13 — multi-family transport and RX boundary

- Added a common, allocation-free family backend registry for all 14 catalogued
  chipsets. A USB endpoint preflight now verifies real configuration/interface
  descriptors and independently reports incomplete station implementations.
- Added shared offline USB receive-aggregate preflight for the six chipsets
  already having verified wire-format decoders (RTL8192CU, RTL8188EU,
  RTL8192EU, RTL8814AU, RTL8812AU and RTL8821AU). Malformed/CRC/ICV frames
  fail without authorizing any USB transmit.
- Kept all 122 previously eligible station IDs unchanged and left the other 148
  disabled (105 verified identities awaiting station backends; 43 ambiguous IDs).
  The eight other chipset families have descriptor preflight only; this release
  does NOT complete their firmware, PHY/RF, RX/TX or WPA2 station code.
- Added the `rtw-backends` diagnostic tool, 270-profile compatibility tests and
  20,000 randomized parser-input checks. The kernel attach path now checks
  explicit backend capabilities instead of inferring station readiness from a
  recognized vendor/product ID.
- Expanded English capability explanations for each pending chipset family.
  Preserved ad-hoc signing requirements and existing documentation structure.
- Portable Linux UBSan build and 81/81 test groups passed. No native macOS
  Xcode/kext build, physical card verification, or real codesign was performed.

# Changelog

## Driver 0.13.16 / app 0.2.12 — 2026-10-23

Driver build 35; app remains 0.2.12 (build 32), with English UI and
mandatory ad-hoc signing scripts. Complete source only.

Prevent stale RTL8812AU firmware checksum reports from authorizing a new
download: reset the MCUFWDL W1C checksum bit and confirm it is clear before
writing pages. Add simulated stale-status and stuck-clear USB cases.

Reject saturated six-bit eFuse temperature code 0x3F as a factory reference
in both identity extraction and thermal preflight. This is diagnostic only;
it does not execute IQK or grant TX authorization.

Accept WPA2-compatible suite selections in transition-mode RSN beacons:
group CCMP, an offered pairwise CCMP suite and PSK AKM, without mandatory
PMF. The offline association request still selects only PSK/CCMP. WPA3-SAE
key exchange and physical RTL8812AU/RTL8821AU station operation are not
implemented; no additional USB identities are enabled.

Full portable build with UBSan and **80/80 CTest groups passed**;
strict Clang build and static analysis of four C modules passed.
Hardware RF/USB operation, native macOS build and actual codesign were
not executed.

## Driver 0.13.15 / app 0.2.12 — 2026-10-23

Driver build 34; the English app remains at 0.2.12 (build 32).
**Complete source only; existing 122 mapped USB identities preserved.
No additional chipsets activated or physical functionality certified.**

Fix station-scan WPA2 classification in `kext/RTWUSBController.cpp`:
require RSN, PSK, pairwise CCMP, **group CCMP** (`ni_rsngroupcipher`),
and no mandatory PMF. An AP with TKIP group encryption, WEP or required
PMF is no longer presented as connectable through the app. Add portable
`NetworkSecurity.h`, exhaustive six-boolean combination regression
and a native-only kernel source contract.

Harden the *offline, non-transmitting* RTL8812AU/RTL8821AU association
response parser: reject non-ESS/IBSS, security capability changes, malformed
or duplicate Supported/Extended Rates IEs and rates not advertised by
the selected AP before changing association state. This does not provide
WPA2 key exchange, on-air association or carrier.

Linux host Debug build with UBSan: **80/80 CTest groups passed**;
strict Clang compile and analyzer passed for the modified C code; USB
profile/personality/matrix generator checks passed. The macOS Xcode/kext
build, actual ad-hoc codesign, libusb hardware diagnostic link and USB/RF
tests on each adapter have NOT been executed. All 148 previously disabled
catalog identities remain disabled pending the appropriate backend or
USB identity evidence.


## Driver 0.13.14 / app 0.2.12 — 2026-10-22

Driver build 33; app unchanged at 0.2.12 (build 32).
**Source only; no additional USB ID enabled or new station backend activated.**

Add `RFCalibration.h`/`RFCalibration.c`, a strictly gated RTL8812AU B-cut,
USB/RFE3, channel-6 passive **thermal preflight**. The laboratory helper checks
the current read-only MAC/BB receive state and prior RF channel verification,
then snapshots CCA and HSSI selections, observes two path-A RF[0x42] samples
through PI/SI serial readback, restores register state and verifies restoration.
It rejects missing/unstable codes, parses the factory eFuse reference at `0xBA`,
and reports an eight-code-unit drift advisory for future IQK investigation.
A readout may be stale; these are not Celsius measurements. **No IQK/LCK/DPK,
RF write, calibration correction, transmission or association is performed.**

Add USB mock fault-injection tests for authorization, selector/CCA preservation,
sampling/settling errors, sensor codes, factory eFuse, missing or unstable
readings and restore failures. Fix the `rtw-hwdiag` invalid-length return path
to attempt MAC RX quiescence. Integrate source in CMake and Xcode.
The full Linux portable Debug/UBSan CTest suite passed **78/78** tests;
new thermal mock test passed **96 assertions**. Strict Clang compilation passed.
Actual `libusb` headers, Mac Xcode/codesign and physical device were unavailable;
the helper's syntax was checked only against temporary mock API declarations.
The 122 existing eligible profiles remain unchanged.

## Driver 0.13.13 / app 0.2.12 — 2026-10-22

Driver build 32; the app is unchanged at 0.2.12 (build 32).
**Source only. No additional USB adapter ID is enabled in the kext.**

Add guarded RF[0x18] channel readback for the sole supported RTL8812AU
B-cut/TSMC/USB/RFE3 channel-6 passive laboratory profile. Query both
Jaguar LSSI/HSSI readback paths, supporting per-path PI and SI modes,
with bounded 10-microsecond settling waits. Read the A and B paths twice,
validate channel 6 on path A before passive capture, and mark path B as
inconclusive when its known unreliable readback cannot be confirmed.

Save and restore the exact 0x838 CCA and 0x8B0 HSSI values, including on
partial USB errors. Verify the restoration by readback and refuse capture
if any required state cannot be restored. Add `rtw_monitor_quiesce_rx_channel6`
to clear MACRXEN/RXDMA/HCI_RXDMA without altering other MAC CR bits,
attempted on successful capture exit and capture/verification failures.
The radio is not fully powered down by this operation.

Extend the optional `rtw-hwdiag` with English RF/cleanup diagnostics.
Add a model USB regression suite including PI/SI selection, transient RF
values, absent chain B, permission gating, forced USB/readback/delay errors,
restoration failures and RX quiescence. Linux host build and 77/77 CTest
groups pass with UBSan. Strict Clang syntax and static analyzer run
without diagnostics for the two changed C modules; `rtw-hwdiag` passes
syntax checking only against temporary API declarations, not linked
against actual libusb. No macOS/Xcode build, codesign or RF hardware
validation was performed. RF calibration, association and WPA2 remain
absent for RTL8812AU/RTL8821AU.

## Driver 0.13.12 / app 0.2.12 — 2026-10-22

Driver build 31; the app is unchanged at 0.2.12 (build 32).
Source-only RTL8812AU B-cut/RFE3 laboratory receive integration. **No new
adapter is bound to the kext and no station/radio readiness is claimed.**

Introduce `RTL8812AUPassive.{h,c}`: a bounded, allocation-free physical USB
bulk-RX bridge connecting the 24-byte RTL8812AU descriptors to the strict
802.11 beacon/probe and RSN parser. Validate each complete USB aggregate
before delivering any AP callbacks; reject malformed/truncated tails,
discard C2H, CRC and ICV errors, verify on-air FCS, and compare advertised
channels to the independently known channel-6 lab profile. Count reception
failures by category; include unsupported security APs in diagnostics without
claiming association.

Update the explicit opt-in `rtw-hwdiag --passive-scan-6` libusb path to use
the RTL8812AU-specific decoder instead of the earlier generic aggregate parser.
Add a six-register **read-only** post-init monitor check for device identity,
MAC RX enable/filter and BB receive gates before entering the receive loop.
That check cannot prove RF-chain calibration, transmitted power, real reception,
WPA2, security or any connection to an AP.

Add host regression tests for USB aggregates, malformed buffers, callback
abort, channel mismatch, FCS/CRC/ICV errors and 20,000 adversarial inputs.
Run full portable suite (76/76 passing using system Python) and four relevant
tests rebuilt with UBSan. Clang strict compilation and static analysis of
the changed C files reported no diagnostics. The libusb tool passed syntax
checking against test-only API declarations, but the actual libusb headers
were unavailable here. Native Xcode build, Apple codesign, and physical
USB/radio reception remain unverified. The 122 enabled/148 pending profile
counts, English UI and mandatory ad-hoc signing are unchanged.

## Driver 0.13.11 / app 0.2.12 — 2026-10-22

Driver build 30, app build 32. Source-only development increment for
RTL8812AU and RTL8821AU. **No additional adapter is enabled.**

Add independently tested `RTL8812AURadio.c` channel geometry for supported
2.4/5-GHz standard 20/40/80-MHz channel groups, recognizing DFS but never
granting regulatory or TX permission. Add `RTL8812AUStation.c` for bounded,
allocation-free parsing of 802.11 beacon/probe-response IEs and RSN suites,
optional FCS validation, strictly matched primary channels, and offline
open-system authentication/association frame construction. Only open or
WPA2-PSK/CCMP networks without required PMF can be candidates. Parsing
an association response cannot authorize an Ethernet link; WPA2 remains
blocked until an unimplemented secure key exchange/CCMP datapath exists.

Extend the capture inspector with explicit `--channel` / `--fcs` options
and hexadecimal SSIDs, avoiding terminal-control injection. Integrate
new sources in CMake and Xcode, add adversarial RX/IE/state-machine tests,
50,000 randomized malformed management frames and 11 end-to-end capture
CLI regressions. Linux UBSan CMake build and all 75 CTest groups passed.
Clang static analysis passed for both new C files. macOS/Xcode compilation,
native ad-hoc `codesign`, USB RF calibration, management transmission and
physical link tests remain pending.

Preserve English UI, ad-hoc signing requirements, third-party licenses,
the 270-profile adapter catalog, 122 existing eligible profiles and 148
disabled entries. Audit, changelog, inventory and validation records remain
consolidated to one file per function.


## Driver 0.13.10 / app 0.2.11 — 2026-10-22

Driver build 29, app build 31. Source-only, fail-closed RTL8812AU /
RTL8821AU development increment. **No additional USB adapter was enabled.**

Introduce an allocation-free C USB RX descriptor decoder and restricted 40-byte
TX descriptor serializer for 1- and 2-chain 802.11ac chipsets. Parse C2H
notifications separately from data frames, reject truncated and malformed
transfers, bound MPDU and bulk lengths, and validate rates against chain count.
TX requires consistent QoS, multicast, EAPOL, PHY bandwidth, retry limits and
protection options, and emits the hardware USB XOR checksum. Non-QoS TX uses
hardware sequence assignment; QoS TX uses the supplied 12-bit sequence.

Add a read-only CLI for inspecting saved bulk-RX captures, portable descriptor
and adversarial parser tests, and source integration into the CMake library and
native Xcode project. Firmware, PHY/RF calibration, AP scans, 802.11 station
association and WPA2/CCMP for these new families are **not** implemented.
The decoder/serializer is not called by the active kernel station path.
RTL8812AU (34 verified candidate IDs) and RTL8821AU (22 verified IDs) remain
disabled; 122 existing profiles and 148 disabled profiles are unchanged.

Preserve English app localization and the mandatory ad-hoc signing policy.
The actual macOS build, codesign execution and physical USB/Wi-Fi tests remain
pending. Existing license notices and one-file-per-document-purpose structure
are retained.

## Driver 0.13.9 / app 0.2.10 — 2026-10-22

Driver build 28, app build 30. Source-only compatibility expansion.

Add 20 independently evidenced USB aliases: 19 use the existing C/CU backend
and one uses the existing RTL8814AU backend. Enabled profiles increase from
102 to 122; catalog size increases from 250 to 270. Preserve the original
250 identity dictionaries and pin their canonical digest. Preserve all 148
disabled entries; this release does not implement a new radio family.

Introduce full interface-class/subclass/protocol matching in the shared C API,
kernel discovery, generated personalities and app diagnostics. The older
descriptor-incomplete API fails closed for class-constrained profiles.
New aliases require configuration 1/interface 0/vendor-specific class. All
verified RTL8814AU entries also enforce the upstream 0xff/0xff/0xff interface.
Missing registry descriptors are reported as unavailable, not fabricated.

Use binary search over the immutable sorted profile table. Reject malformed,
duplicate, unpinned, mismatched or unsupported extension metadata. Add
provenance/matching regressions and exhaustive descriptor-byte tests. Rename
the existing support matrix to USB-WIFI-ADAPTERS.csv rather than keeping a
second matrix. Keep English UI, mandatory ad-hoc signing, firmware, PHY tables,
signal fixes and existing runtime resource policies.

## Driver 0.13.8 / app 0.2.9 — 2026-10-22

This is a source maintenance release. Driver build metadata advances to 27 and app
metadata to 29. The adapter catalog, firmware and radio calibration are unchanged.

### Runtime and resource ownership

Cancel queued or stale UI work using an atomic work epoch across adapter changes,
suspension, session changes, disconnect and termination. Keep sleep and session
activity independent. Stop polling while suspended. Do not synchronously wait for
an XPC queue on AppKit's main thread during termination.

Use per-work-item autorelease queues for driver and credential work. Close the
previous listener and retire its temporary endpoint before reusing a transport
session. Preserve a credential-list refresh requested while another is in flight.
Deduplicate saved SSIDs with a set. Coalesce repeated pending control commands.

### Performance and presentation

Adapt scheduled polling to activity: 2 seconds with the menu open or a join in
progress, 4 seconds while connected with the menu closed, 8 seconds without an
adapter and 15 seconds without the service. Permit timer coalescing with tolerance.
Cache interface enumeration longer while the menu is closed. These are scheduling
changes, not measured CPU or battery savings.

Prefer a supported access point when duplicate SSIDs include a stronger
unsupported-band access point. Preserve complete data separately from bounded,
single-line menu labels. Add accessible names and native semantic colors.
Constrain diagnostic windows to available screen space. Show invalid RSSI as
unavailable instead of printing a misleading numeric reading.

### Build, signing and distribution

Centralize mandatory ad-hoc signing and strict verification. Sign nested code
inside-out, verify app/helper separately, reject certificate/notarization modes,
verify the staged installer payload and verify again before postinstall launch.
Add the same policy to the generated Xcode kext project. A check-only app build
now compiles, signs and verifies a disposable full bundle.

Fail on logging/build/signature errors and reject unexpected destination links.
Use the installer as the only application installation path instead of maintaining
a second privileged copy implementation. Bound portable build parallelism.

### Verification and maintenance

Add concurrent cancellation, poll policy, duplicate-network, bounded-history and
signing-policy regressions. Extend actual production-function harnesses to Linux
with test-only compatibility headers. Add native lifecycle, menu and repeated
socket-session tests, which still require macOS execution.
Consolidate historical instructions, audits and run logs into one file per purpose.
Preserve upstream legal notices, firmware and machine-readable source pins.

## Driver 0.13.7 / app 0.2.8 — input release

The supplied previous source release introduced English application strings,
signal/adapter diagnostic reports, median/hysteresis display filtering and
RSSI-validity handling. Its CCK calculation restricted the special weak-signal
cutoff to the applicable LNA case; missing PHY samples no longer replaced a
valid AP sample with zero. That source reported 102 enabled profiles.

Earlier experimental backend development remains represented by source,
per-file legal notices and upstream metadata. Historical test claims are not
carried forward as evidence that this release passed native or hardware tests.
