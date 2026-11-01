# Validação — 0.14.7/build54 e app 0.3.4/build46

Projeto e adaptação macOS: MaLd0n. Estado: desenvolvimento.

## Checkup físico da build53

Em 31/10/2026 foi encontrada carregada a build53 recompilada pelo usuário,
UUID F700B442-AB96-3173-A65B-595D3F1B44CE. Tahoe 26.7.2/25G332, RTL8814AU
2357:0106 na porta direta 14400000 a 480 Mb/s, Registry 4294968504/en3. WPA2/CCMP
autorizado, IPv4/IPv6 e rota padrão na USB. Canal 6 / 2,4 GHz, sinal entre −46 e −42 dBm.
App 0.3.4/build46 e helper ativo, íntegros por assinatura ad hoc; helper com uma
execução e sem encerramento registrado. Nenhum crash/panic atual de app/helper
foi encontrado nos diretórios DiagnosticReports consultados.

Cinco downloads válidos de 4 MiB e uploads de 1 MiB / 4 MiB usaram curl IPv4 vinculado
a en3, TLS verificado, sem proxy e HTTP 200. Amostras curtas: 48,3–73,9 Mb/s de
download e 42,0–55,2 Mb/s de upload. Um pedido separado de 16 MiB recebeu HTTP 403
do servidor e foi excluído da medida. Não são tetos de velocidade, nem o mesmo
servidor/metodologia da captura do usuário, 84,9/32,7 Mb/s.

90/90 pings ao gateway responderam, sem perda. Durante as transferências, média
7,838 ms e máximo 190,015 ms; em seguida, sem transferência gerada pelo agente,
média 4,526 ms e máximo 12,267 ms. O pico sob carga permanece observado; não foi
atribuído ao rádio ou modem sem evidência adicional.

Snapshots gerais tiveram nove valores RX distintos durante 50 s e permaneceram
autorizados no mesmo controlador. No intervalo total, RX publicou +38.773.035 bytes,
+6.365 agregados e +30.912 pacotes Ethernet enfileirados. USB/recovery, erros de
agregado/truncado/malformed e CCMP MIC/replay permaneceram 0. O maior read real
foi 21498 bytes: acima dos 16 KiB antigos, sem corte do agregado. Esses valores são
últimas publicações, não contadores automaticamente vivos nem captura atômica.

TXQueueDrops 12132 e TXLinkDrops 5 não aumentaram. InputErrors 5→12, sem aumento
nas causas exportadas pela build53. Contabilidade interna de inputPackets avançou
pelos mesmos 30912 pacotes, enquanto netstat e getifaddrs retornaram BSD RX 0.
Não há prova de que os sete erros sejam falha USB, CCMP ou uma causa BA específica.

Nos logs do boot houve um aborto inicial de quatro requests RX e erros iniciais
de frameworks Apple; não houve repetição correspondente no recorte consultado.
O diagnóstico logd de escrita em disco não foi atribuído ao driver. Não se afirma
que todos os logs estejam sem erros. Registros de scan/cache incluem 5 GHz;
a associação física atual permaneceu 2,4 GHz. Não houve scan forçado, troca de AP,
alteração de roteador, reinstalação, reinício ou alteração de EFI pelo agente.

## Correções da build54

Fila interna cheia agora retorna stall ao IOGatedOutputQueue e preserva o mbuf
para retry. O wrapper não libera o pacote nem soma erro nessa pressão temporária.
Após conclusão/escoamento, a fila superior recebe serviço assíncrono somente
quando autorização e capacidade permitem. Fila ausente continua falha real.
Teardown, reset, concorrência e isolamento por adaptador preservam ownership.
A fila superior é finita; sobrecarga sustentada ainda pode esgotá-la.

O retorno de inputPacket/flushInputQueue não é mais ignorado. Falta de interface
nativa ou diferença na quantidade de submissões retorna erro, sem tocar novamente
em mbufs transferidos. Oito contadores por interface observam a fronteira; KPI
ifnet_stat e índice nativo permitem comparar a etapa BSD. Isso corrige sucesso
falso da entrega, mas não comprova a causa nem o reparo do RX 0 observado na build53.
Nenhum contador nativo foi incrementado manualmente ou zerado para mascarar falha.

STATUS publica HT/BA para os 13 backends, separado da política AMRR. Geração ímpar
indica publicação em andamento e a próxima par conclui o snapshot geral; tempo
monotônico pertence àquele controlador. Timers podem atualizar subconjuntos entre
consultas e uma leitura do IORegistry não é atômica. O coletor mantém essa ressalva,
suprime comparações de publicação em andamento/regredida e respeita validade/índice
das estatísticas nativas. Versões antigas continuam legíveis, com frescor desconhecido.

São publicados eventos BA/reorder existentes, sem inventar contador de fragmentação
ou decomposição um-a-um dos erros. no_buf inclui buffer ausente e slot duplicado.
Metadado Ethernet fixo 54 Mb/s foi substituído por desconhecido 0: não existe snapshot
completo e recente de taxa TX, largura, GI e associação em todos os backends. MCS RX,
capacidade negociada e taxa térmica C2H permanecem separados. Não foi alterada RF,
criptografia, firmware, ABI, capacidade RX 32 KiB AU ou TXAMPDU da build53.

## Verificação e entrega

Testes novos reproduziram os defeitos na fonte53 antes da implementação. Testes
dos métodos reais verificam 77 casos TX/wake, 24 de completion e 36 de entrega RX; Controller
e _mbuf compilaram no SDK nativo com limite de stack de 2048 bytes. A publicação real
cobre todas as 13 famílias, estados de associação, KPI inválida e leitura sem mudança
de política RF. O coletor tem 15 testes comportamentais offline com Bash/Perl/ditto.

Passaram 155/155 testes Release e 155/155 testes com ASan/UBSan no host.
A revisão independente, compilação do ZIP extraído, produtos,
assinaturas e hashes são registrados em VERIFICATION.json/NATIVE-VERIFICATION.json
na pasta da entrega. O source contém app46, helper, kext, testes e scripts completos.
A build54 ainda precisa ser carregada para medir seu comportamento físico.

## Suporte

270 identidades inventariadas, 263 confirmadas em tabela primária, 262 elegíveis a 13
backends e descoberta restrita 13b1:0043. Lista completa em USB-WIFI-SUPORTE.txt e
USB-WIFI-ADAPTERS.csv. RTL8814BU continua sem estação habilitada por firmware,
protocolo/perfil e tracking pendentes. DFS/WPA3/AX não foram habilitados.
Validação física de todas as famílias/revisões continua necessária.
