# Validação — 0.14.9/build56 e app0.3.6/build48

Projeto e adaptação macOS: MaLd0n. Desenvolvimento.

## Evidência real

O ZIP de 01/11/2026 mostra a build55 e app47: RTL8814AU 2357:0106/en3 com
publicação fresca, mas scan/RX/perfil em zero; RTL8192EU 2357:0108/en1 conectado.
O erro USB acompanhado de desconexão pertence ao EU. Os 20 arquivos IPS são
abortos históricos de testes em userspace, sem prova de panic desta kext.

App48 foi instalado e seu executável comparado por SHA256 ao produto compilado.
A CLI executou LIST/STATUS/MAP com sucesso no ID 4294968492 e SCAN retornou
0xe00002d8 em 0,765 ms. STATUS mostrava USBerror em zero e nenhum rádio/busca iniciado.
As consultas são metadados; não demonstram saúde física do transporte USB.
A build55 não exportava o core errno nem a fase de bootstrap, então o erro inicial
específico permanece desconhecido. Nenhuma causa de RFK/firmware foi inventada.

## Correções e verificação

Testes dos métodos reais reproduziram e corrigiram: latch permanente após
falha elegível; poweroff antes de drain; bypass por startup/watchdog; perda
de erro misto; falta de finalizador após callback; e proveniência RX perdida
entre EBUSY e conclusão tardia. O erro de limpeza não apaga o original.

A família IONetworkController chamava o hook padrão Unsupported e encerrava PM
antes do registro tardio do driver. A nova implementação registra os estados
pelo policy maker, pede estado 1 e encerra PM fora do gate antes de liberar
recursos. A ausência de PM no snapshot combina com esse defeito, mas não prova
que ele causou a falha original da AU.

A CLI do app foi validada por parser estrito, fronteiras XPC reais/mocadas e log
privado limitado. FIFO e flock ocupado não bloqueiam comandos. CodeIdentity e
autenticação do helper foram preservados. Nenhum arquivo de senha foi criado.

Os fixtures de pedido inválido estavam gravando erros simulados no log do usuário.
Agora CTest e app/test.command isolam o home do processo antes de iniciar esses
binários. A verificação encontrou os registros no home de teste e nenhum PID
desses testes no log normal. STATUS real do app instalado continuou válido.

As contagens finais Release/sanitizadores/ZIP, compilação nativa, assinaturas,
payload do instalador e hashes constam em VERIFICATION.json junto da entrega.
As revisões independentes e sua delimitação estão em REVIEWS/.

## Limites

Não foi carregada a kext56 pelo agente. Não há validação física de recuperação,
sleep/wake, WPA2/DHCP/tráfego ou estabilidade de todas as placas/revisões desta
build. Nenhuma alteração de EFI, reboot, roteador ou proteções do macOS foi feita.
RTL8814AU permanece HT20, com até dois fluxos; VHT/AC não foi habilitado. Canal 144 e estação
RTL8814BU continuam fechados. Source completo de app/helper/kext é fornecido.
