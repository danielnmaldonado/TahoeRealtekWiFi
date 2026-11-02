# Validação — 0.14.6/build53 e app 0.3.4/build46

Projeto e adaptação macOS: MaLd0n. Estado: desenvolvimento.

## Evidência física de partida — build52

Após a troca de porta pelo usuário, AU 2357:0106 passou ao caminho direto
@14400000, 480 Mb/s, Registry 4294979718/en3, WPA2 autorizado no canal 6,
RSSI próximo de −44 dBm. EU não estava conectado. Continuou carregada a kext52
recompilada pelo usuário, UUID 7511954E-85FD-30FC-92E6-28942A938040; app/helper46.

Cinco pings ao modem, vinculados ao IP de en3, responderam sem perda:
2,479–5,490 ms. GET de 4 MiB e POST de 1 MiB de zeros aos endpoints públicos
Cloudflare, vinculados a en3/IPv4, completaram com HTTP200 e sem proxy. Médias
do curl: 8,73 Mb/s e 8,93 Mb/s. São amostras curtas de WAN, não capacidade máxima.
O download adicionou cerca de 5,05 MB de RX, 857 agregados, 53 truncated e
53 malformed; a maioria dos bytes HT estava em MCS15. Os valores publicados
mostraram zero recuperação USB. No upload, o snapshot imediato não mudou;
não se inferiu ausência de tráfego desse snapshot possivelmente antigo.

## RX compatível com a agregação DMA

O vendor identifica 0x2005 como threshold de 20 KiB. Linux rtw88 usa buffers
de 32768 bytes; o native usava 16 KiB. USBRXCapacity.h agora seleciona 32 KiB
para AU e mantém 16 KiB nos demais backends. Alocação, início, rearm e refill
usam a mesma capacidade e verificam o tamanho do buffer existente.

O teste real usa agregado válido de 20200 bytes/13 quadros. Dividi-lo em
16 KiB corta um MPDU após dez quadros, gera truncated e depois malformed no
read seguinte. Uma leitura de 32 KiB preserva os 13 quadros. O mecanismo é
compatível com os 53 casos físicos, mas a build52 não publicou os comprimentos
dessas completions; não se afirma identidade causal caso a caso.

Novos escalares: RTWDiagRXLastReadBytes, RTWDiagRXMaxReadBytes e
RTWDiagRXAggregateLastErrorReadBytes. O último se refere à falha estrutural
no parser, não ao comprimento de um erro USB. Não são exportados pacotes.
DMA, temporização e orçamento de recuperação permanecem iguais.

Revisão independente: 943 checks de capacidade, 2105 de recovery e 258 de
ordenação passaram; o mesmo teste continua RED contra a fonte52. O Controller
completo compilou com SDK nativo e limite de frame de stack de 2048 bytes.

## Autoridade da taxa térmica

sendFrame sobrescrevia a taxa efetiva C2H pela taxa inicial/fixa do descritor,
inclusive em gerenciamento, EAPOL e TX rejeitado. O primário atualiza o tracking
pela taxa C2H. A escrita por pacote foi removida, preservando descritores,
baseline do PHY e filtros de C2H. Os 55 checks Controller→PHY/thermal passaram;
o delta térmico reproduzido permanece 3→3, antes mudava 3→4.
C2H sem identificação de associação continua não distinguível apenas por
contador local; não foi inventada uma prova de epoch ausente no protocolo.

## TX AMPDU AU

A integração programa o limite de bytes do peer em 0x458 antes de aceitar BA.
Falha de I/O recusa admissão e bloqueia TX. O descritor consulta o acordo BA
vigente por TID, autorização, HT, QoS/ACK e fragmentos. MAX_AGG_NUM usa pares
de MPDUs e respeita metade da janela BA; janela 1 permanece individual.
Densidade, sequência do host e checksum foram verificados. Gerenciamento,
EAPOL e grupo não agregam. DELBA e reassociação retiram a elegibilidade.
A ausência de AGG_BK corresponde ao primário; bytes não agregados são preservados.
Batching de descritores por transferência USB continua fora desta correção.

Fixtures registram cinco hashes e máscaras primárias. Na revisão independente,
3817 checks TX/BA, 2025 codec, 2470 BA/CCMP e 178 lifecycle passaram. A negociação
importada foi exercitada nas duas bandas. Isso não valida agregação física no ar.

## Integração e entrega

Passaram 153/153 testes Release e 153/153 com ASan/UBSan no host. As revisões
independentes não encontraram novos bloqueadores. Resultados do source extraído,
produtos nativos, assinaturas e integridade ficam em VERIFICATION.json e
NATIVE-VERIFICATION.json junto à entrega, após congelar este source.
O app46 e seu protocolo continuam compatíveis.

A build53 ainda não foi carregada no hardware. Ganho de velocidade, negociação
BA e estabilidade precisam de medição após o usuário compilar/substituir a kext.
Não houve instalação, reinício, alteração de EFI, roteador, SIP/AMFI ou uso de
senhas pelo agente. Todos os arquivos permanecem na pasta do projeto.

Inventário e gates preservados: 270 IDs, 263 primários, 262 elegíveis a 13 backends,
mais descoberta restrita 13b1:0043. RTL8814BU depende de firmware/perfil qualificados;
DFS, WPA3 e modos não implementados não são anunciados. O canal real de 5 GHz do
roteador automático permanece desconhecido; esta sessão52 ficou em 2,4 GHz/canal6.
O lançamento completo ainda não está homologado.
