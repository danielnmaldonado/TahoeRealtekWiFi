# DFS no modo cliente USB

O usuário pediu suporte DFS usando Linux, para todas as placas que têm essa
capacidade. Implementar diretamente na árvore atual, preservando a conexão
carregada e entregando source completo de kext e app. Não alterar EFI, carregar
kext, reiniciar, mudar roteador ou usar credenciais pessoais.

## Arquitetura escolhida

Portar o comportamento de estação gerenciada, sem implementar modo AP. Linux
cfg80211_chandef_dfs_required retorna zero para STATION: CAC/radar do master não
é requisito de um adaptador cliente. O cliente deve buscar passivamente, conectar
ao BSS observado e respeitar a saída de canal anunciada pelo AP. Não remover
globalmente NO_IR após receber beacon nem forçar país por locale/IP.

Expandir os sete backends de duas bandas: RTL8192DU, RTL8812AU, RTL8814AU,
RTL8821AU, RTL8821CU, RTL8822BU e RTL8822CU. Novos canais HT20: 52/56/60/64 e
100/104/108/112/116/120/124/128/132/136/140. Canal144 fica fechado nesta entrega:
faltam linhas de potência primária nos AU/DU. Não copiar valores do canal140.
Capacidades de placas exclusivas de2,4GHz e perfis OEM apenas5GHz permanecem.

A lista de canais no backend depende de validação de ROM/RFE, potência/calibração
e restrições existentes. Não basta adicionar números. AU8814 precisa do ajuste
primário de spur no140, cleanup ao sair e grupo térmico correto100–140. Os demais
PHYs exigem seus próprios índices OTP/limites. DFS começa em HT20; modos largos
existentes nos canais não DFS permanecem. Anúncio do AP mais largo deve negociar
20MHz, sem provocar falha na associação nem anunciar DFS40/80 não implementado.

## Estado comum de estação

Recepção passiva deve registrar beacon ESS válido por BSSID, SSID e canal,
com limite de memória e validade temporal. Autenticação DFS só pode usar BSS
observado recentemente no canal recebido e autorizado pelo backend. Frames
malformados, estrangeiros, associação antiga ou campos contraditórios não
concedem TX. Domínio/potência de fábrica não é relaxado por um beacon.

CSA de beacon/action do AP associado deve interromper TX conforme necessário,
baixar o link e iniciar recuperação passiva, sem enviar deauth no canal evacuado.
A primeira implementação pode refazer associação/WPA2 após rescan em vez de
trocar RF mantendo chaves. Retomada preserva perfil somente enquanto o dispositivo
e sessão continuam válidos. Teardown, sleep, falhaUSB e seleção de outro adaptador
cancelam tarefas; não executar stop/restart recursivamente dentro da análise RX.
Pause usa caminhos/registradores primários verificados para cada família.

## App e ABI

Manter layout v1 e bytes reservados. Acrescentar duas capabilities independentes:
DFS52–64 e DFS100–140, somente quando o mapa registrado cobre o grupo completo.
App47 aceita e descreve os grupos anunciados pelo driver. Drivers antigos não
passam a aceitar canais novos por nome do chipset. Não anunciar144 ou modoAP.

Revisão detectou países com apenas parte do grupo100–140 (AU/DZ/KP/MY/RU).
Adicionar consulta somente leitura do mapa exato de256 canais registrados,
com versão e reservados zero, sem alterar estruturas/seletores ABIv1 existentes.
App47 usa esse mapa na lista, seleção/reconexão e descrição; driver antigo retorna
Unsupported e mantém a política conservadora das capabilities completas.

## Domínio e teto de potência

Portar metadados do wireless-regdb oficial fixado e verificar hashes antes de
gerar. Usar país de fábrica conhecido, senão world; nunca locale/IP ou override
por beacon. Antes de AUTH/RFK, limite efetivo é min(world20dBm,domínioROM,
tripletCountryAPquandoexistente) menos PowerConstraint. Aplicar uma vez pelo SAR
absoluto de cada PHY, com arredondamento para baixo e sem segunda redução relativa.
Preservar headroom térmico de fábrica, sem margem extra por reduzir TXAGC.
Teto impossível não pode resultar em prontidão/TX; DU/B-C podem detectar depois
de I/O de canal pausado e devem manter falha fechada.

## Prova e entrega

Usar testes comportamentais RED na54 e GREEN na implementação: cada canal novo,
potência por RFE/ROM, transições de spur140, HT20 em AP largo, busca sem probes,
autenticação ligada ao beacon e CSA/teardown/ownership. Comparar RF/PHY com fontes
Linux/vendor fixadas e registrar hashes. Executar suítes Release/sanitizadores,
compilação SDK, testes nativos do app e build do ZIP entregue. Revisão independente
antes de congelar build55/app47. Testes físicos exigem carregamento posterior pelo
usuário; não declarar todas as revisões homologadas nem a causa da rede atual provada.
