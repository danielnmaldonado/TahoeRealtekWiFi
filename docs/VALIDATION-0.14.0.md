# Validação de desenvolvimento — 0.14.0/build47, app 0.3.0/build42

Este relatório acompanha o source e não constitui homologação de todos os
adaptadores ou revisões do catálogo. Estado atual: desenvolvimento.

## Escopo

- 270 registros: 250 do pacote original e 20 extensões com origem preservada.
- 231 identidades numéricas verificadas; 230 elegíveis aos 13 backends integrados.
- RTL8814B B814 possui identificação primária, mas não possui HAL operacional.
- 39 identidades não confirmadas/conflitantes permanecem desabilitadas.
- Associação de estação WPA2-Personal/CCMP, com integridade, replay e porta
  controlada no net80211 adaptado para macOS.

Os limites por família, largura e banda constam no README e na matriz USB.
Identidade numérica, interface existente e compilação não comprovam tráfego.

## Evidência física disponível

No RTL8814AU 2357:0106 conectado, a rotina explícita de rádio/recepção verificou
os canais 149/153/157/161/165: 40 MPDUs/beacons, zero erros de formato/CRC/ICV e
encerramento limpo. Há registro anterior de recepção nos canais 36–48.
Esses testes não são associação WPA2, DHCP ou medição de download/upload com
a kext build47 carregada.

A consulta atual não encontrou kext TahoeRealtekWiFi nem serviço RTWUSBController
carregados. A validação da nova kext exige a Clover no próximo boot
e os adaptadores/revisões correspondentes. A compilação não reinicia o Mac
nem modifica SIP/AMFI.

## Verificação de software

O catálogo, o leitor C, a geração de personalidades, a matriz e a identificação
no app passaram juntos em nove testes após confirmar B814 e os dois aliases legados. Os testes
usam todas as 270 identidades e suas restrições de interface/configuração.

Os componentes novos possuem testes de registradores, imagens reais de
firmware, falhas de transporte antes/depois do efeito, métodos reais extraídos
do controlador e negociação pelo net80211 importado. Contagens de asserções
não representam número de placas homologadas.

O app 0.3.0/build42 passou os 14 executáveis de teste nativos, incluindo menu
AppKit, identificação, transporte e políticas de credenciais. O app e o helper
foram compilados e suas assinaturas ad-hoc passaram a verificação estrita.
Os testes de credenciais e login usam substitutos e não alteram os ajustes
ou o Chaves do usuário.

A kext 0.14.0/build47 foi compilada e ligada com as 109 unidades do projeto,
230 personalidades e assinatura ad-hoc verificada. Não há referências externas
às classes privadas Apple80211/IO80211/AppleBCMWLAN no binário. As tabelas IQK
8710BU/8723BU têm vínculo privado por módulo, também nos geradores.

A bateria final passou os 143 testes CTest em Release (50,20 s) e os mesmos
143 com ASan/UBSan nos alvos de host (59,93 s). O kernel não recebe essa
instrumentação. O fixture de bootstrap B/C observa a captura da autoridade
apenas após a identificação completa, mantendo os caminhos reais de
Boot/FW/OTP/features e 8.611 falhas de I/O.

A revisão independente de AU e B/C terminou sem falhas reproduzíveis pendentes
no escopo de software. Estados MLME, encerramento e EDCA verificam propriedade
e firmware antes de comandos; a autorização rápida de TX/taxa/BA usa somente
memória, inclusive sem Bluetooth. Os testes incluem alterações de transporte,
placa, ROM e pipes, firmware inválido, erros antes/depois do efeito e limpeza
local de chaves/porta. A matriz AU contém 988 casos de propriedade, 252 de
firmware e 1.600 falhas de inicialização; a matriz B/C contém 2.268 casos de
estados/EDCA com e sem BT. Contagens não equivalem a placas homologadas.

A consulta de lista HID do watchdog 8822C foi recuperada da fonte primária;
a regressão nas três famílias confirma H2C0x73 somente no 8822C. Os testes
CCX verificam ACK real, tokens e expiração; conclusão USB não é tratada como
confirmação pelo rádio.

As fontes C importadas ainda emitem avisos de conversão. O build completo
passou; os testes separados de Controller/unidades novas também verificaram
quadros de pilha limitados a 2.048 bytes. Não se declara ausência de avisos
em toda a árvore importada.

O empacotamento confere CRC, hashes de todos os arquivos e permissões. A
validação da entrega extrai o ZIP para outra pasta, compila testes, app,
kext e instalador, verifica as assinaturas e compara o payload app/helper do
instalador com o bundle compilado. O SDK de kernel continua uma dependência
externa fixada; não é incorporado ao ZIP do source.

## Critérios ainda necessários para lançamento

Conexão, reconexão, suspensão, remoção USB, DHCP, tráfego sustentado e coexistência
Bluetooth precisam ser exercitados em hardware para cada família/revisão.
Identidades conflitantes exigem evidência adicional; o RTL8814B exige HAL,
firmware e PHY próprios. O catálogo completo não deve ser anunciado como
conjunto de 270 adaptadores fisicamente homologados.
