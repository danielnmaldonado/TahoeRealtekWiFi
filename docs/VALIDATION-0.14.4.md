# Validação — 0.14.4/build51 e app 0.3.4/build46

Projeto e adaptação macOS: MaLd0n. Estado: desenvolvimento.

## Evidência na build50

kmutil confirmou a build50 carregada, UUID 2A9BFFFC-E0B2-34BC-BA34-EEF2B306D7EE.
O RTL8814AU 2357:0106 estava com WPA2 autorizado, link ativo e zero erros ou
tentativas de recuperação USB publicados. O usuário relatou estabilidade.
Isso é uma observação dessa sessão; não é certificação de tráfego sustentado.

O PHY mediu valores entre −44 e −42 dBm, com amostras recentes e milhares de
leituras válidas. Nesses níveis, três barras são o resultado esperado dos
limiares existentes. Testes nativos AppKit distinguiram as imagens de 1/2/3
barras e verificaram retorno correto às imagens em cache. Não se constatou
defeito de renderização ou necessidade de modificar a calibração RSSI.

O roteador foi descrito como Wi-Fi 6 com canal/bandas automáticos e mesmo SSID.
O canal real de 5 GHz não foi informado e não pôde ser obtido pela interface
de controle sem autenticação administrativa. Não houve pedido de senha,
alteração de roteador, troca de USB, instalação ou reinício pelo agente.

## Mudanças no app

Tooltip, acessibilidade e linha de sinal mostram a leitura dBm medida e o
valor filtrado usado pelas barras. Leituras retidas são marcadas como antigas;
expiração, desconexão e indisponibilidade não produzem 0 dBm fictício. A largura
do ícone, os limiares e a filtragem não foram alterados.

Refresh networks, quando conectado, antes apenas solicitava nova leitura do
cache. Agora a ação explícita solicita uma busca real. O driver pode precisar
reassociar durante esse scan foreground; o perfil permanece no mesmo caminho
de controle. Polls periódicos continuam somente leitura, sem buscas automáticas
disruptivas. Testes cobrem duplicidade, adaptador, resultado e preferências.

## Rádio e diagnóstico

A revisão comparou o RTL8814AU com o Linux fixado no manifesto de origem.
O caminho HT20 omitia o ajuste NBI/CSI de canal 153; foi portado e verificado
contra os registradores finais do código primário, incluindo retorno a 149/157.
Essa omissão não demonstra que o AP do usuário utiliza 153 nem explica, por si,
a ausência de todas as redes 5 GHz.

O scan foreground já percorre os nove canais 5 GHz implementados antes de
escolher BSS. A nova telemetria distingue transições/visitas, quadros de
beacon/probe response por canal de software e por canal PHY conhecido, além
de divergências entre ambos e contagem do cache por banda. Ausência de canal
PHY não vira uma suposta medição física. Nenhuma alteração é feita na entrega
de quadros ou nos controles de USB para produzir esses diagnósticos.

Somente contadores escalares são publicados, sem SSID, BSSID, endereço MAC,
senha, PMK, dados de tráfego ou conteúdo de pacotes. A publicação ocorre nas
fronteiras de estado/consulta; não há mutações de IORegistry a cada quadro RX.

Os canais autorizados continuam 1–11 e, em 5 GHz, 36/40/44/48 e 149/153/157/161/165,
conforme backend/ROM. DFS continua fora da implementação. O nome Wi-Fi 6 do
roteador não identifica seu canal ou suas opções de segurança. Não houve
desbloqueio de canais sem implementação nem mudanças em WPA2/CCMP.

## Leitura dos contadores de scan

RTWDiagScanState usa 0=Idle, 1=Active, 2=Complete, 3=Cancelled e 4=Failed.
Complete identifica a saída de scan para AUTH; não comprova autorização WPA2
ou acesso à rede. RTWDiagScanVisited0–3 são palavras de 64 bits: para o canal C,
consulte a palavra C/64 e o bit C%64. Visita indica programação aceita pelo
backend, não recepção RF comprovada. LastError é o erro de transição.

RTWDiagScanSoftware5Frames conta quadros rotulados pelo canal de software;
RTWDiagScanPHY5Frames usa somente canal PHY decodificado. UnknownPHYFrames e
PHYChannelMismatches deixam explícita a ausência ou divergência dessa informação.
RTWDiagScanCacheBSS5 conta registros do cache, não quadros nem redes distintas
por SSID. CurrentChannel é o canal de estado atual. Não confundir esses valores
com a banda que o roteador utiliza fora dos canais observados.

## Verificação e entrega

A suíte passou 150/150 em Release e 150/150 com ASan/UBSan no host.
As 15 suítes nativas do app também passaram. O PHY executou 35.657 verificações,
incluindo os 22 registros de referência e 472 posições de falha do canal 153;
a telemetria portátil passou 34 e o controlador/WPA 1.186 verificações.
Os resultados detalhados e a compilação nativa do ZIP entregue são registrados
em VERIFICATION.json. Testes de modelos, imagens ou
registradores não substituem a observação de redes e tráfego no hardware.
O source contém app46 e kext51; o script de build não instala ou reinicia.

O inventário e os limites de hardware anteriores são preservados: 270 IDs,
263 identidades primárias, 262 IDs elegíveis aos 13 backends de estação mais
descoberta restrita separada 13b1:0043. RTL8814BU e identidades sem prova
continuam nos estados documentados.
