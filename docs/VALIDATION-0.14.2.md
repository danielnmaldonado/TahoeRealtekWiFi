# Validação — 0.14.2/build49 e app 0.3.3/build45

Projeto e adaptação macOS: MaLd0n. Estado: desenvolvimento.

## Evidência física que motivou a correção

A build48 foi encontrada carregada. O RTL8192EU 2357:0108 apresentava porta
WPA2 autorizada. O RTL8814AU 2357:0106, RFE1/cut1, inicializava e recebia quadros;
após reconexão USB associou em 2,4 GHz segundo o usuário, mas voltou a cair.
O log do host USB registrou em duas sessões o endpoint de entrada0x81
retornando0xe00002ed (NotResponding / transaction error), com zero bytes.
Esse evento era transformado imediatamente em erro fatal retido pelo driver.
O log identifica o gatilho; não determina sua origem elétrica ou de firmware.

## Recepção USB

A recuperação admite apenas NotResponding com zero bytes, em estação ativa.
Limpa halt com clearStall(true), bloqueia novos RX enquanto drena as quatro
gerações antigas e só então rearma os buffers. Callbacks de abort não consomem
o orçamento. Três tentativas consecutivas e dois segundos limitam a operação;
falha em halt clear, drenagem, refill ou orçamento retorna ao caminho fatal.
RX útil zera a sequência. TX, demais erros, stop, sleep e unplug mantêm seus
controles. O diagnóstico publica tentativas, conclusões, falhas e endpoint.

A política foi confrontada com o tratamento de erros do
[driver Linux RTL8814AU](https://github.com/aircrack-ng/rtl8814au/blob/892641430c24308e7ae193d746b25b600ef227d8/os_dep/linux/usb_ops_linux.c#L735)
e com o contrato de halt/abort de IOUSBHostPipe nos headers reais do SDK Apple.
A implementação e os testes de host percorrem o registro das 13 famílias
operacionais; isso não homologa fisicamente todas as placas.

## Busca em duas bandas

O código podia escolher o SSID em 2,4 GHz ao terminar a rodada ativa sem
visitar os canais passivos de 5 GHz. Agora a busca foreground com SCANALLBAND
consome os canais passivos habilitados antes da escolha e preserva o cache.
DU, 8812AU/8821AU, 8814AU e 8821CU/8822BU/8822CU anunciam essa capacidade.
O teste executa funções reais de scan/seleção e os blocos reais de registro
dos sete backends, em cinco máscaras/perfis de bandas.

Continuam autorizados somente os canais e placas implementados. DFS permanece
fora do escopo; em 5 GHz os grupos são36/40/44/48 e149/153/157/161/165.
O app mostra bandas utilizáveis observadas; o AP é escolhido automaticamente.
Uma rede 5 GHz em canal fora desses grupos requer trabalho adicional de driver.

## Verificação e entrega

Os testes operacionais RX e scan usam métodos reais extraídos, com fronteiras
USB/registradores modeladas e ASan/UBSan. Os resultados completos de integração,
compilação nativa, source extraído e payload do instalador são registrados no
VERIFICATION.json da entrega. Modelos não substituem RF, WPA2, DHCP e tráfego
sustentado nos adaptadores/revisões físicos.

O inventário permanece em270 IDs:263 identidades primárias,262 elegíveis e
uma descoberta separada restrita para13b1:0043. Cinco IDs sem prova e dois
conflitantes não recebem habilitação estática. O RTL8814BU permanece sem estação
habilitada; seus limites estão em RTL8814BU-STATUS.md.

A build49 precisa ser carregada para validar as correções no Tahoe. O source,
app e kext são entregues juntos em releases do projeto, com manifesto e SHA-256.
Não há reinício automático ou alteração de SIP/AMFI durante a compilação.
