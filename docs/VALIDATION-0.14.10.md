# Validação — 0.14.10/build57 e app 0.3.7/build49

Projeto e adaptação macOS: MaLd0n. Desenvolvimento.

## Sistema real verificado

Em 01/11/2026, o usuário já havia carregado sua build56. UUID carregado:
3B3FC579-9C48-3D8F-B104-89C9D314100D. App instalado no começo: 0.3.6/build48.
IDs atuais AU 4294968519/en3 e EU 4294968559/en1 substituem os do boot anterior.

Duas consultas reais LIST/STATUS/MAP por placa passaram, aproximadamente 5,11 s
entre amostras. Ambas estavam presentes, habilitadas, com rádio/RUN e WPA2/CCMP
PTKDONE/porta autorizada/IP. AU: canal 36/−46 dBm; EU: canal 6/−44 dBm. PM do driver
estava no estado 1. USB alta velocidade 480 Mb/s; AU direto, EU no hub USB2.
As rotas padrão IPv4/IPv6 usavam AU/en3; Ethernet en0 estava inativa.
Não cresceram erros USB/TX/RX ou falhas de recuperação na amostragem.

Dois GET HTTPS limitados, vinculados separadamente a en1 e en3 e sem proxy,
retornaram HTTP 200 e 577 bytes cada, cerca de 60 ms. Assim há entrega de dados em
ambas, além da associação/IP. Não é teste de velocidade, persistência ou WPA2
a partir de uma nova tentativa; o agente não alterou senha, AP ou associação.

## Contagem RX nativa

O driver ofereceu e esvaziou filas Ethernet, mas ifnet_stat/netstat continuaram
com RX cumulativo zero nas duas interfaces. Os dois testes HTTPS funcionaram.
A revisão de Apple IONetworkingFamily/XNU não comprovou um defeito do driver;
há caminhos de contagem distintos, incluindo Skywalk. Uma leitura posterior de
ifconfig -m confirmou options/capabilities0x400 (CHANNEL_IO) nas duas interfaces,
correspondente ao IFCAP_SKYWALK do SDK. Isso não prova sozinho onde a contagem
foi omitida. A build57 publica capacidades suportadas/ativas e SkywalkEnabled
para investigar. Não há incremento manual artificial, desativação de recursos
ou substituição do handoff IOKit por tentativa. A divergência continua aberta.

## App e recuperação comum

O app49 troca leitura de cache por uma busca real quando o USB selecionado está
ocioso e sem conclusão RF fresca. Validade de 30 s, timeout de 30 s, retries de 4/8 s e pausa de 300 s
após três falhas. Estado por RegistryID, geração e work epoch. Link autorizado,
JOIN, credenciais, erro USB e serviço/sessão indisponíveis bloqueiam autoSCAN.
RADIO ausente em interface ainda não iniciada não bloqueia a primeira busca.
A UI conserva o contrato de menuNeedsUpdate; o modelo é renovado na conclusão.

PM e recuperação RX em operação já eram comuns. A validação do RX inicial agora
atinge todos os 13 backends; quatro leituras válidas são exigidas antes de sucesso,
com orçamento de clearStall, drain por geração e preservação de erro tardio.
A parada comum também rastreia a geração dos RX/TX pendentes: erro USB tardio
não desaparece ao cancelar e TX curto não vira sucesso. Metadados são verificados
antes de liberar o slot, inclusive quando o callback chega após o preflight.
A recuperação de bootstrap usa shutdown verificado e reidentificação por família;
a implementação e as revisões/testes exatos são fornecidos com a entrega.

## Limites da entrega

A build57 ainda depende de carregamento e validação no hardware. Não equivale
a certificação de todas as 262 identidades/revisões. RTL8814AU continua HT20 e até
dois fluxos, sem VHT/AC. Canal 144 e estação RTL8814BU continuam fechados.
As contagens de software, produtos nativos, payload, hashes e revisões fechadas
constam de VERIFICATION.json e REVIEWS junto ao pack. Source completo fornecido.
Nenhum reboot, EFI, roteador, Keychain ou proteção do macOS foi alterado.
