# Validação — 0.14.3/build50 e app 0.3.3/build45

Projeto e adaptação macOS: MaLd0n. Estado: desenvolvimento.

## Falha observada na build49

A kext49 foi confirmada por kmutil, UUID 88C75B61-85DE-39D2-AEA1-972C0546A64A;
o app instalado era 0.3.3/build45. No RTL8814AU 2357:0106, o IORegistry registrou
duas tentativas RX, uma concluída, uma falha e zero esgotamentos. Não se trata
de uma cópia antiga carregada nem de limite de tentativas já atingido.
O RTL8192EU 2357:0108 continuava com WPA2 autorizado.

Às 23:01:31.118 e 23:01:31.994, o log USB registrou transaction error 0xe00002ed,
zero bytes, no endpoint RX 0x81 do AU. Às 23:01:31.995, houve erro também no
endpoint de controle 0x00. Os contadores antigos não distinguiam falha em
clearStall de falha no refill. A proximidade do erro de controle é compatível
com falha de clearStall, mas o log não informa a identidade da requisição.

A conexão AU estava a 480 Mbps através de hub USB 2.0. O mesmo log apresenta erros
USB no outro adaptador e no teclado. Isso não determina a origem física ou de
firmware; testar uma porta direta separa parte dessas variáveis.

## Correção da recuperação

A build49 tornava fatal a primeira falha da limpeza ou do rearmamento, mesmo
com orçamento disponível. Além disso, uma rejeição síncrona de uma submissão RX
podia reter usbError antes que a conclusão correspondente entrasse na recuperação.

A build50 preserva clearStall(true), cancela/drena as gerações anteriores e
admite novas tentativas espaçadas apenas para NotResponding transitório durante
essa recuperação. O limite é de três tentativas de recuperação do endpoint,
com intervalo mínimo de 100 ms e prazo total de dois segundos. O prazo é
verificado antes e depois das chamadas síncronas da Apple; uma chamada
bloqueada não pode ser interrompida pelo watchdog deste driver. Leituras de uma
tentativa parcialmente rearmada precisam ser canceladas e drenadas antes da
seguinte. Continuam valendo o orçamento de erros RX consecutivos, o reset
somente após RX útil e os controles de stop/sleep/unplug.

A busca direta solicitada pelo app retorna Busy durante a recuperação, antes
de alterar canal ou estado. Timers de scan, calibração e coexistência B/C
adiam seu trabalho até a recuperação terminar. JOIN e LEAVE continuam podendo
cancelar a sessão deliberadamente.

Requisições de registradores e transmissões não são reproduzidas por essa
política. Erros diferentes, transferência parcial ou falha persistente seguem
o caminho fatal. Diagnósticos escalares distinguem limpeza, drenagem e refill. No IORegistry,
RTWDiagRXRecoveryStage usa 0=Idle, 1=Clear, 2=RetryWait, 3=Drain, 4=Refill,
5=Complete, 6=Failed e 7=Cancelled. FailureStage preserva o estágio terminal;
ClearResult e RefillResult identificam os resultados individuais. ClearAttempts,
ClearFailures, RefillFailures, TransportRetries e TransportExhausted mostram o
orçamento efetivamente usado, sem SSID ou material de chave.

## Referências primárias

O contrato Apple recomenda sincronização do toggle com withRequest=true e
explica que erros bulk não timeout deixam o pipe halted:
[Apple ClearStall](https://developer.apple.com/documentation/usbdriverkit/iousbhostpipe/clearstall).
A adaptação foi confrontada também com IOUSBHostPipe.h do SDK 26.5 real.

O Linux vendor reenvia RX após EPROTO e contabiliza erros consecutivos, mas o
host controller Linux faz sua própria recuperação. Isso não equivale a trocar
clearStall(true) por false no macOS:
[RTL8814AU RX](https://github.com/aircrack-ng/rtl8814au/blob/892641430c24308e7ae193d746b25b600ef227d8/os_dep/linux/usb_ops_linux.c#L806),
[Linux xHCI](https://github.com/torvalds/linux/blob/2a500a04d6f969318c05f46aedb6e73fc1963a00/drivers/usb/host/xhci-ring.c#L2558).

## Verificação e limites

Os testes executam métodos reais do controlador, com fronteiras USB modeladas,
incluindo pipe halted, rejeição síncrona, erro transitório ao limpar, refill
parcial, prazo, orçamento, ordenação, cancelamento e erros fatais. A verificação de software passou 149/149 testes Release e 149/149 com
ASan/UBSan no host, além das 15 suítes nativas do app. A recuperação executou
2.105 verificações; controle/WPA 1.050; ordenação 258; coexistência verificou
pausa/retomada nos seis perfis família/antena. Os resultados detalhados e a
compilação nativa a partir do ZIP entregue ficam no
VERIFICATION.json da entrega. Esses testes não comprovam estabilidade RF/USB real.

As correções de busca passiva e bandas observadas da build49 são preservadas.
Canais 5 GHz continuam limitados a 36/40/44/48 e 149/153/157/161/165; DFS não está
implementado. Inventário 270, 263 identidades primárias, 262 IDs elegíveis aos 13
backends de estação, mais descoberta restrita separada 13b1:0043. O RTL8814BU e
as identidades sem prova permanecem nos limites documentados.

A kext50 precisa ser carregada pelo usuário para verificar a queda no Tahoe.
O source inclui app45 e kext50; a compilação não instala, altera EFI ou reinicia.
