# Validação — 0.14.5/build52 e app 0.3.4/build46

Projeto e adaptação macOS: DNMTechLabs. Estado: desenvolvimento.

## Revisão por família

Foram revisados os 13 backends de estação, divididos em legacy 2,4 GHz,
DU/AU e B/C. A revisão comparou implementação, descritores, RF/PHY, bandas,
associação, RX/TX, coexistência e ciclo de vida com as fontes primárias fixadas.
Testes anteriores podiam passar porque repetiam posições de campos incorretas;
as correções abaixo possuem novas referências independentes e RED→GREEN.

RTL8192EU usava máscaras gen1 em DW4 para HT40, mas seu descritor gen2 usa
DW5. O resultado podia ser BW20 com alteração de RTS/retry. O novo formato
separa DATA_BW, DATA_SC e DATA_SHORT/SGI, incluindo gestão/EAPOL 20 MHz em rádio
HT40. Setters textuais do cabeçalho Realtek fixado servem de referência; seu
hash/proveniência acompanha os testes. CU/8188EU rejeitam os novos campos HT.

RTL8812AU/RTL8821AU posicionavam PHY antes de SHIFT, divergindo do rtw88.
Foram corrigidos os casos 0–3 em ambas as famílias, com RSSI e comandos do
firmware/coexistência, fronteiras de agregados e ausência de PHY. Os codecs
operacionais RTL8814AU, DU e B/C já tinham a posição correta.

O codec auxiliar FamilyRX24 agora aplica SHIFT ao PHY somente nas famílias
rtw88. Para FU/8723BU, a fonte rtl8xxxu coloca PHY imediatamente após o
descritor e o padding antes do MPDU; essa disposição foi preservada. Não se
aplicou uma regra de outra geração a todos os chipsets.

RTL8192FU substituía a sequência de gestão/não-QoS do host por sequenciamento
de hardware. O port usa sequência 80211 de software; agora gestão, EAPOL,
QoS e demais dados preservam o valor e os fragmentos do cabeçalho recebido.
8710BU e 8723BU já preservavam esses valores e não foram alterados por isso.

A coexistência AU/B-C podia memorizar um nível LNA desejado e perder sua
requisição quando outra consulta MP estivesse pendente. Agora mantém somente
o último nível desejado por contexto e tenta admiti-lo após ACK ou timeout
da consulta anterior. O cache muda após admissão H2C/scoreboard bem-sucedida,
conforme a semântica primária; isso não confirma aplicação física pelo rádio.
Erros, stop e perda de firmware/propriedade/sessão descartam trabalho pendente.
Há testes de coalescência, cancelamento, isolamento e falha durante callbacks,
incluindo uma sequência pública real de BT-info/HID nos três B/C. Não equivale
a qualificação RF com Bluetooth no hardware.

## Conteúdo preservado da build51

O app46 solicita scan real somente na ação explícita de Refresh networks;
polls periódicos continuam somente leitura. Sinal em dBm medido/filtrado,
leitura antiga/ausente e acessibilidade são distinguidos. O ícone renderiza
os três níveis corretamente; −42/−44 dBm medidos na build50 são sinal forte.
Permanecem o ajuste NBI/CSI 153 e os contadores de scan/PHY/cache por banda.
Consulte VALIDATION-0.14.4.md para os detalhes dessas mudanças.

## Verificação e limites

O conjunto integrado passou em Release (150/150) e com ASan/UBSan no host
(150/150). Uma revisão independente não encontrou novos bloqueadores nas
correções. O app46 preserva as 15 suítes nativas verificadas na build51; o
conjunto integrado também recompila e verifica seus testes nativos. Compilação
nativa do ZIP, metadados, assinaturas e integridade ficam em VERIFICATION.json
e NATIVE-VERIFICATION.json junto aos arquivos da distribuição.
O inventário, as identidades e os gates anteriores são preservados: 270 IDs,
263 identidades primárias, 262 elegíveis aos 13 backends operacionais, mais
descoberta restrita 13b1:0043. Testes de código não homologam todas as revisões.

RTL8814BU possui componentes HAL/USB e PHY próprios, mas a estação permanece
desabilitada sem firmware NIC/protocolo e perfil de fábrica qualificados.
Identidades ainda sem prova não são habilitadas por semelhança de nome.
Modos e canais não implementados, inclusive DFS, não são anunciados. O canal
real de 5 GHz do roteador automático permanece desconhecido.

Na preparação original da build52, a build50 estava carregada no RTL8814AU,
com WPA2 autorizado, zero erros RX de recuperação e última amostra de −37 dBm. Naquele momento, build52/app46 ainda
precisavam ser carregados para validar scan 5 GHz, tráfego e os demais adaptadores.
Não houve instalação, reinício, alteração de EFI/roteador ou uso de credenciais
pelo agente nesta revisão. O source continua na pasta do projeto, sem saídas
no Desktop.

## Atualização logs1 — 2026-10-31

O usuário recompilou/carregou a kext52 (UUID
7511954E-85FD-30FC-92E6-28942A938040). A captura às 10:59 registrou
RTL8814AU 2357:0106/en3 sem porta WPA2 autorizada e com erro USB 0xe00002ed:
25 erros, 24 recuperações concluídas e um esgotamento. RTL8192EU
2357:0108/en1 estava autorizado, com rota padrão e zero erros de recuperação
USB. A captura não mede throughput: o teste mostrado falhou por HTTP500 e
nenhum teste de velocidade foi iniciado pelo coletor.

WiFi-Diagnostics.command agora gera o ZIP em Biblioteca/Logs/Tahoe-WiFi-USB,
com duas leituras textuais/JSON, resumo de deltas por Registry ID e status de
cada comando. Falha de leitura é distinta de remoção; deltas de captura
incompleta ficam indisponíveis. Há limite de execução por comando, incluindo
saída parcial, e somente os filhos desse comando são encerrados ao excedê-lo.
Dez regressões offline passaram; uma coleta real produziu ZIP íntegro.

logs1 altera o coletor, sua documentação e integração dos testes. Os sources
nativos da kext/app, firmware, versões52/46 e produtos compilados permanecem
iguais aos da entrega52 original. O novo ZIP de source é separado e não
reescreve a entrega anterior. O erro USB e a lentidão do AU continuam em
investigação; criar um coletor não equivale a corrigir a comunicação USB.

## Análise do relatório 11:17 e logs2

O ZIP do usuário contém logs de10:47:34 a11:17:11. Como os erros do AU já
existiam às10:43, a primeira falha ficou fora dessa janela. A consulta local
somente leitura desde o boot recuperou25 erros de RX0x81 do AU@14130000 entre
10:41:59.912 e10:42:06.261, seguidos pelo abort final de três leituras.
Também houve11 erros EP0, coerentes com as11 falhas de clear/retry contabilizadas;
atribuir cada uma ao CLEAR_FEATURE continua inferência sem setup packet.

O mesmo controlador XHCI registra erros do teclado em outro hub, inclusive
antes da enumeração atual do AU. Isso não prova causa comum nem defeito físico.
Os dados localizam a falha no transporte RX, antes de WPA2/DHCP; não determinam
sua origem. Não foi alterado orçamento de recuperação, buffer ou timeout USB.

A publicação geral RTWDiag acontece em status(). O app consulta o adaptador
selecionado; no outro, as propriedades podem estar antigas. Scan e alguns
campos HT/RA possuem publicação separada. O coletor logs2 documenta essa
limitação: delta0 significa somente valores publicados iguais. A kext52 não
fornece timestamp/geração de publicação geral e o coletor não contorna o controle
de acesso para forçar STATUS. Sources nativos continuam iguais à entrega52.
