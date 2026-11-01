# Validação — 0.14.8/build55 e app0.3.5/build47

Projeto e adaptação macOS: MaLd0n. Estado: desenvolvimento.

Esta entrega amplia a build54 com DFS HT20 para os sete backends dual band.
Preserva as correções53/54 de RX32KiB AU, TXAMPDU, trackingC2H, ownership das
filas TX e contabilidade/observação de RX. O catálogo permanece com 270 IDs,
263 identidades confirmadas e 262 elegíveis aos 13 backends. RTL8814BU e os sete
registros sem confirmação/conflitantes não ganham habilitação estática.

## Comportamentos verificados em software

Testes reproduzem rejeição dos canais novos na build54 e verificam geometria,
potência por ROM/RFE/caminho/taxa, índices OTP, spur140 e transições de saída,
grupo térmico100–140, falhas de I/O e limites absolutos SAR. DFS opera em20 MHz;
os caminhos AU aceitam APs mais largos por negociação20 sem anunciar DFS40/80.

O parser comum verifica bounds, SSID/BSSID/canal, frescor do beacon, Country,
PowerConstraint e CSA/ECSA. Testes dos métodos reais do Controller verificam
admissão antes de AUTH/RFK, bloqueio de filas, retenção de TXPAUSE nos dois MACs
DU, recuperação adiada, isolamento de adaptadores e cancelamento em teardown.
Teto local combinado é aplicado uma única vez. O país de fábrica e os limites
wireless-regdb são testados nos sete caminhos, incluindo domínio que proíbe100
e permite52; nenhuma regra é ampliada por beacon. O gerador recusa input alterado
antes de sobrescrever a saída.

App47 valida capabilities explícitas e descrições DFS, driver antigo e o mesmo
SSID observado em ambas as bandas. Layout ABIv1 e limites finitos de cache são
preservados. Consulta aditiva de mapa exato cobre faixas regionais parciais,
incluindo AU8/RU3, reservados, respostas inválidas, socket real e troca de
adaptador/geração. Busca/autenticação nativas8814 são testadas nos24 canais5 GHz;
CSA de AP associado continua ativa quando seu SSID é oculto ou muda, sem conceder
nova autorização. Planos de potência rejeitados preservam o teto térmico vigente;
SAR negativo mantém precisão e não pode ser convertido em índice positivo.
A execução final das suítes Release/sanitizadores, compilação nativa,
testes do app, revisão independente, extração do ZIP e hashes consta em
`VERIFICATION.json`/`NATIVE-VERIFICATION.json` junto da entrega. O ZIP inclui
source correspondente integral, helper, geradores, testes e licenças obrigatórias.

## Limites físicos

A build55 não foi carregada pelo agente. A evidência física anterior pertence à
build53 e está preservada em VALIDATION-0.14.7.md: associação2,4 GHz e transferências
curtas no RTL8814AU. Ela não valida DFS/CSA nem todas as placas/revisões. O canal
de5 GHz do roteador automático continua desconhecido. Não se afirma que DFS era
a causa observada ou que a nova build já resolveu a associação física.

Canal 144 permanece fechado, sem extrapolação de potência AU/DU. Não há modo AP,
CAC/radar local, WPA3/AX ou habilitação RTL8814BU. Limites de software não medem
ganho de antena/EIRP agregado no hardware. As restrições existentes de RFE, ROM,
Bluetooth e OEM 5 GHz permanecem. Veja DFS-STATION.md e USB-WIFI-SUPORTE.txt.

O agente não alterou EFI, kext carregada, app instalado, credenciais ou roteador.
Compilar/substituir o bundle inteiro e iniciar pela EFI correta continua sendo
a etapa física do usuário. Após o boot, conferir build55, canal atual, WPA2,
DHCP, tráfego pela interface USB e transição de canal do AP.
