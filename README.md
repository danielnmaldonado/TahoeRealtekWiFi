# Tahoe Wi-Fi USB

Projeto e adaptação para macOS: **MaLd0n**.

Source da kext e do aplicativo de barra de menus para macOS Tahoe em Intel.
A árvore 0.14.8/build55 e o app 0.3.5/build47 estão em desenvolvimento. Compilação e testes de software não
substituem testes de conexão e tráfego em cada adaptador e revisão de placa.

O driver expõe uma interface Ethernet `enX`, seguindo o modelo usado pelo
itlwm. O net80211 adaptado para macOS controla associação e WPA2-Personal/CCMP;
o app escolhe o USB, procura redes, conecta e mostra o estado. O protocolo de
controle não depende das classes privadas do AirPort.

## Modos implementados

Os limites descrevem código de estação. Identificação USB, ROM, revisão do
silício, caminhos RF, potência de fábrica e endpoints são verificados na
inicialização. Os 13 backends operacionais estão integrados ao controlador.

| Família | Bandas | Modo de estação |
|---|---|---|
| RTL8192CU | 2,4 GHz | 11b/g, 20 MHz |
| RTL8188EU | 2,4 GHz | 11b/g, 20 MHz; I-cut não suportado |
| RTL8192EU | 2,4 GHz | HT20/40, até 2 fluxos |
| RTL8192DU | 2,4 e 5 GHz | HT20; SMSP até 2 fluxos, DMDP 1 fluxo por MAC |
| RTL8192FU | 2,4 GHz | HT20/40, até 2 fluxos |
| RTL8710BU | 2,4 GHz | HT20/40, 1 fluxo; SMIC/UMC suportados |
| RTL8723BU | 2,4 GHz | HT20, 1 fluxo; controles de coexistência Bluetooth |
| RTL8812AU | 2,4 e 5 GHz | HT20/40 e VHT20/40/80, até 2 fluxos |
| RTL8814AU | 2,4 e 5 GHz | HT20, até 2 fluxos ativos; RFE0/1 |
| RTL8821AU | 2,4 e 5 GHz | HT20/40 e VHT20/40/80, 1 fluxo |
| RTL8821CU | 2,4 e 5 GHz | HT20, 1 fluxo |
| RTL8822BU/CU | 2,4 e 5 GHz | HT20, até 2 fluxos |
| RTL8814BU | Estação desabilitada | HAL/USB e PHY próprios; firmware NIC/protocolo pendentes |

Na implementação AU atual, RTL8812AU aceita RFE 0–3 e RTL8821AU aceita RFE 0,
com os padrões de ROM documentados. Placas RTL8812AU com Bluetooth ativo
continuam bloqueadas: o descritor primário escolhido não possui os callbacks
de coexistência necessários. Esses limites de placa valem mesmo quando o ID
USB está no catálogo.

Os modelos 04bb:0959, 04bb:095a e 0bda:2003 seguem a instalação OEM exclusiva
de 5 GHz: não anunciam nem aceitam transmissão em 2,4 GHz.

Os perfis usam canais 1–11 em 2,4 GHz e os canais não DFS 36/40/44/48 e
149/153/157/161/165 em 5 GHz, conforme o backend e a ROM. A build55 acrescenta
DFS HT20 em 52/56/60/64 e 100/104/108/112/116/120/124/128/132/136/140 aos sete
backends dual band. ROM/RFE, país de fábrica e limites de potência podem reduzir
esse conjunto. O driver carregado anuncia cada grupo separadamente; o catálogo
sozinho não habilita transmissão. O canal144 permanece fechado.

A busca DFS é passiva. Um beacon recente do BSSID/SSID/canal escolhido permite
autenticação dirigida; o driver respeita o teto de potência local e o anunciado
pelo AP. CSA interrompe TX, encerra o link e inicia nova busca/associação WPA2.
APs com canal mais largo usam o caminho DFS20 implementado. VHT40/80 existente
nos canais não DFS continua disponível nos AU correspondentes. Não há modo AP,
CAC/radar local, WPA3 ou Wi-Fi6/AX. Consulte [a implementação DFS](docs/DFS-STATION.md).

Tetos PHY não são resultados de download/upload. A velocidade depende do AP,
do sinal, da USB e das funções implementadas. CU/8188EU permanecem em 11b/g;
8814AU e B/C estão limitados a HT20.

## Catálogo e estado de lançamento

`data/legacy-usb-adapters.json` preserva os 250 registros originais do pacote
antigo e 20 extensões com evidência de origem. Há 270 IDs no inventário e 263
com correspondência numérica em tabela primária. Os sete restantes possuem
identidade não confirmada ou conflitante, sem habilitação estática. O ID
13b1:0043 possui descoberta separada e restrita: o chip é lido antes de
escolher B/C, com nova verificação antes de potência e firmware. Os outros
casos não são habilitados por semelhança de nome.
O RTL8814B possui agora HALMAC/USB, OTP e codecs próprios portados. O caminho
de estação continua bloqueado até qualificar firmware NIC licenciado e seu
protocolo, perfil de fábrica/regulatório e funções de tracking restantes,
além dos testes físicos. Veja [o estado detalhado](docs/RTL8814BU-STATUS.md).
Um nome parecido não permite usar o HAL de outro chipset.

Destes, 262 IDs são elegíveis aos 13 backends integrados. Consulte a
[matriz completa](docs/USB-WIFI-ADAPTERS.csv) e o
[relatório de validação](docs/VALIDATION-0.14.8.md). A elegibilidade de
um ID não comprova todas as revisões comerciais que o utilizam. Este source
não é uma homologação de lançamento para os 270 registros.

## Correções de busca e recepção desta build

A busca inicial dos backends de duas bandas conclui os canais passivos
habilitados antes de escolher o AP. O app mostra todas as bandas utilizáveis
observadas quando o mesmo SSID aparece em 2,4 e 5 GHz; a seleção do AP é
automática no driver.

A recepção USB trata transaction error sem bytes com recuperação limitada
do endpoint. A build50 corrige o encerramento prematuro quando a própria
limpeza do endpoint falha transitoriamente: repete a recuperação com intervalo
e orçamento finito, drena leituras antigas ou parcialmente rearmadas e só então
rearma os quatro buffers. Rejeições síncronas de RX entram nesse mesmo caminho.
O diagnóstico distingue limpeza, drenagem e rearmamento. Esse caminho é comum
às 13 famílias de estação; falhas persistentes e outros erros mantêm parada
segura. A build49 recuperou uma ocorrência física, mas falhou na seguinte.
A build50 foi encontrada estável no RTL8814AU, com WPA2 autorizado e sem
erros USB de recuperação no momento da captura. Isso não valida outras placas
ou todos os canais. A build53 foi carregada e examinada no checkup de 31/10; a build54 precisa de novo carregamento para validar suas correções.

## Busca explícita e sinal medido

O app47 solicita uma busca real ao clicar em Refresh networks, inclusive se
já conectado. Essa busca pode reassociar brevemente; as consultas de estado
em segundo plano continuam somente leitura. O cache não representa uma
nova varredura de rádio.

O menu, tooltip e acessibilidade mostram RSSI medido em dBm e o valor filtrado.
As barras mantêm seus limiares: o sinal observado entre −44 e −42 dBm corresponde
a três barras. As imagens nativas foram verificadas nos três níveis; não foi
encontrado um ícone preso no máximo. Leituras antigas ou ausentes são indicadas.

O caminho HT20 do RTL8814AU inclui o ajuste NBI/CSI de canal 153 presente no
Linux fixado. Contadores de scan indicam os canais visitados e as observações
PHY, separados do cache de redes. O canal de 5 GHz do roteador em automático ainda
não foi identificado; estas correções não comprovam conexão 5 GHz no hardware.

## Revisão das outras famílias

A build52 corrige campos de transmissão HT40/SGI/subcanal do RTL8192EU conforme
os setters primários Realtek, mantendo RTS e retransmissão separados. Quadros
20 MHz em rádio HT40 recebem o subcanal correto. CU/8188EU mantêm seus modos
implementados e rejeitam opções HT não portadas.

RTL8812AU/RTL8821AU corrigem a posição do PHY quando o descritor contém SHIFT,
incluindo medições de sinal e consumidores do firmware/coexistência. O codec
RX24 genérico distingue essa disposição rtw88 da disposição legacy rtl8xxxu.
RTL8192FU preserva a sequência 80211 do host em gestão, EAPOL e dados.
A coexistência AU/B-C preserva solicitações LNA quando outra consulta MP está
pendente, com limite, descarte de sessão antiga e estado por dispositivo.

A revisão cobriu os 13 backends operacionais. A ausência de outro defeito
confirmado numa revisão de código não substitui testes nas placas físicas.

## Extrair logs

Dê duplo clique em `WiFi-Diagnostics.command`. Ele usa somente ferramentas do
macOS, sem precisar compilar app ou kext. A pasta e o ZIP ficam em
`~/Library/Logs/Tahoe-WiFi-USB/Diagnostics/`; o Terminal mostra o caminho final.

`RTW-SUMMARY.txt` identifica cada controlador por Registry ID, família,
VID:PID e interface, e compara duas leituras dos contadores USB/RX/TX.
As propriedades originais e o JSON numérico ficam em `drivers/RTW-before/after`.
São valores publicados no IORegistry, que podem estar antigos quando o app
não consulta aquele adaptador. Delta zero indica valores publicados iguais;
não confirma ausência de novos erros ou tráfego durante a coleta. A build54 fornece geração de publicação e tempo monotônico por controlador. Geração ímpar indica publicação em andamento; a leitura do IORegistry não é atômica. Versões antigas não fornecem esses marcadores.
Falhas de leitura e timeout são registrados em `STATUS.tsv`, sem impedir o ZIP.
Nos AU, os contadores antigos `RTWDiagCalib*` não medem o tempo de calibração.

A coleta padrão não pede administrador, não executa teste de velocidade nem
altera rede/driver. Não lê senhas ou Chaves. Aplica o mascaramento básico já
existente, também a MAC em formato de dados; revise logs antes de compartilhar.
`--output-dir`, `--minutes`, `--command-timeout` e `--no-pause` estão na ajuda.

## RTL8814AU: desempenho na build53

A recepção AU usa agora buffers e leituras de 32 KiB, coerentes com o threshold
DMA primário de 20 KiB. O caminho antigo de 16 KiB podia cortar agregados válidos;
os demais backends mantêm 16 KiB. Novos escalares mostram comprimentos de leitura
e da leitura que falhou, sem exportar pacotes.

TX AMPDU inclui limite de bytes conforme o peer, BA por TID e campos do descritor
validados nas fontes fixadas. Gerenciamento, EAPOL, grupo e fragmentos permanecem
fora da agregação. A taxa efetiva reportada pelo firmware controla o tracking
termal, sem ser sobrescrita pela taxa inicial do descritor em cada envio.

A build52, na nova porta direta, completou transferências curtas em cerca de
8,7 Mb/s download e 8,9 Mb/s upload com zero erros de recuperação publicados,
mas acrescentou truncamentos/malformed RX. Esses são resultados da build52;
no checkup posterior, a build53 completou transferências curtas entre 48,3–73,9 Mb/s de download e 42,0–55,2 Mb/s de upload, sem novos erros USB ou truncamentos publicados. São amostras WAN curtas, não tetos PHY.

## Correções do checkup — build54

Fila TX cheia passa a preservar o pacote e solicitar pausa/retry ao output queue
IOKit. A retomada assíncrona ocorre quando há espaço e autorização; fila ausente
ou conexão encerrada mantém descarte seguro. Isso evita perdas de admissão por
pressão na fila, sem aumentar seus limites.

STATUS publica HT/BA para os backends que retornavam antes da calibração legada,
sem executar AMRR ou I/O de rádio. Inclui causas existentes de reorder/BA e
marcadores de publicação por controlador. A velocidade Ethernet fixa de 54 Mb/s
foi removida: sem uma taxa PHY efetiva completa e recente, o valor é desconhecido.

A contabilidade RX interna avançou junto dos pacotes entregues, enquanto o BSD
reportou zero no checkup53. A build54 verifica o retorno da entrega/flush e
expõe a fronteira de entrada e a leitura nativa do ifnet, sem duplicar contadores.
Isso ainda não comprova correção do zero BSD, nem explica retroativamente os sete
erros de entrada observados. A validação física desses pontos exige carregar54.

A [lista completa por chipset e VID:PID](docs/USB-WIFI-SUPORTE.txt) explica as
262 identidades elegíveis, restrições e revisões; não é homologação física de
todas as placas. O catálogo continua igual ao53.

## Compilar

Requisitos: Mac Intel, Tahoe, Xcode completo com SDK macOS 26, CMake,
Python 3.10+ e ferramentas C/C++. Node é usado por uma verificação
criptográfica independente. O script obtém o MacKernelSDK em revisão fixa;
o SDK não vem no ZIP. `RTW_OFFLINE=1` impede downloads e exige SDK local.

Na raiz do source extraído:

```sh
bash ./build.command --all
```

`--all` compila kext, app e testes. `--kext`, `--app` e `--tests` selecionam
as etapas. Os produtos ficam em `Release/`. Os sources e packs de entrega ficam em
`releases/`, na pasta principal do projeto; não são gravados no Desktop. Os scripts verificam assinaturas
ad-hoc; não instalam nem carregam a kext.

Para os testes nativos do app, AppKit, Chaves e transporte:

```sh
bash ./app/test.command
```

Para gerar o instalador depois de compilar o app:

```sh
bash ./app/package.command --no-build
```

O instalador coloca `Tahoe Wi-Fi USB.app` em `/Applications` e instala o
helper. A kext vai separadamente para a EFI. O app permite guardar a senha
no Chaves do macOS. Nenhuma senha pessoal é necessária para compilar.

## Carregamento pela Clover

Identifique a EFI que realmente inicia o computador. Faça backup da kext
anterior fora da pasta de injeção e copie o bundle inteiro:

```text
Release/TahoeRealtekWiFi.kext
  → EFI/CLOVER/kexts/Other/TahoeRealtekWiFi.kext
```

A cópia será usada no próximo boot pela EFI correta. O driver já carregado
continua em memória até então. Os scripts não reiniciam o computador e não
mudam SIP, AMFI ou a configuração da Clover. Consulte [Clover](docs/CLOVER.md).

## Conferir o resultado

Confira versão/build, adaptador selecionado, autorização WPA2, endereço IP e
tráfego pela interface USB correspondente. A existência de `enX` ou da kext
no IORegistry não comprova conexão. Com Ethernet cabeada conectada, confirme
qual interface é usada pelo teste de velocidade.

`rtw-backends --json` e `--adapters-json` mostram capacidades do source;
não acessam o hardware. Os campos de firmware do diagnóstico legado descrevem
somente `rtw-legacy-fwdiag`. O carregador real da kext usa seu próprio backend.
O boot físico genérico antigo está desabilitado.

## Origem e licenças

O source inclui app, helper, kext, net80211, criptografia, firmwares
redistribuíveis, testes, geradores e avisos de licença. Consulte
[origens](docs/SOURCES.md), os arquivos `*-Upstream.json`, os registros
`firmware/*-provenance.json` e as licenças junto de cada componente.
Firmwares são imagens binárias inalteradas incorporadas ao source; não são
código-fonte do firmware Realtek.

Os geradores validam as entradas primárias fixadas antes de escrever arquivos.
A entrega inclui `SOURCE-MANIFEST.sha256` para conferir a integridade do source.
