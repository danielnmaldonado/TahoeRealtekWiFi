# Validação — driver 0.14.11/build58 e app 0.3.8/build50

Projeto e adaptação macOS: MaLd0n. Entrega de desenvolvimento, com source
completo de app/helper/kext, testes, geradores e licenças. O arquivo
VERIFICATION.json do pacote registra os resultados exatos e hashes finais.

## Preferência de placa e rede

A causa original era seleção somente em memória por RegistryID e seleção
automática limitada a uma única placa. Com dois USB após reboot, nenhum era
selecionado. Agora a identidade física validada é persistida, o RegistryID é
resolvido novamente e cada placa conserva sua última rede confirmada.
Serial/MAC gravados precisam estar presentes e coincidir; porta é fallback
somente quando não existe identidade durável. Identificadores que aparecem
mais tarde fortalecem a preferência da mesma sessão. Uma placa diferente ou
ambiguidade não substitui silenciosamente a preferência.

A rede de uso único é lembrada, sem guardar sua senha. Reconexão automática
continua exigindo perfil salvo no Chaves e opção habilitada. Desativar/reativar
a opção invalida consultas automáticas antigas e JOIN ainda enfileirado.
A migração de versões antigas pode adotar um único link já autorizado, compatível
com a preferência de rede antiga; dois links exigem escolha. A seleção do app
e a ordem global de serviços do macOS são estados distintos.

Os testes executam os métodos reais de Main com boundaries de IOKit/Chaves
substituídos: IDs/ordem novos, enumeração/identidade atrasadas, troca de porta,
identidade ambígua, escolha manual, rede por placa, credencial sem interação,
pausa/opção desabilitada e cancelamento assíncrono. Ambos USB reais produziram
identidades válidas com serial/MAC/VID/PID/porta, sem registrar seus valores.
Isso não substitui repetir boot e associação na máquina com os produtos novos.

## CPU e memória

App49 instalado, menu fechado, 16 amostras em aproximadamente 30 segundos:
`top` registrou CPU 0–0,1%, memória MEM estável em 39 MB, quatro threads.
Auxiliar: 0%, 3.236 KB MEM, três threads. São observações desse intervalo,
não um teto de uso sob todas as redes/operações. RSS e MEM de top usam métricas
diferentes e não devem ser somados à memória da kext.

O formatador ISO8601 era criado/configurado duas vezes por exchange. O cache
sincronizado mantém o formato e evita essa construção repetida. Benchmark de
10.000 registros reais: cerca de 1,10–1,12 s de CPU antes, 20–24 ms depois.
Esse ganho se refere à rotina de logs; não significa que todo o app ficou
50 vezes mais rápido. O teste concorrente também valida conteúdo/timestamps,
FIFO/lock não bloqueantes, permissões e limite do arquivo privado.

A revisão da kext encontrou timer B/C retido no WorkLoop compartilhado após
releasePlatform soltar a referência do loop. A remoção agora ocorre antes.
O teste real de teardown mantém uma segunda placa simulada e repete 10.000
ciclos; mede propriedade/retenção, não apenas ausência de crash.

Os pedidos de payload DMA são fixos/reutilizados: 117.760 bytes por placa,
ou 183.296 no RTL8814AU. Isso exclui arredondamento por página, descritores,
mbufs, nós net80211, objetos IOKit e heaps próprios de cada família. Não é
memória total da kext. CPU/memória de kernel_task são agregados do kernel;
não foram atribuídos exclusivamente a este driver.

## Sistema real observado nesta rodada

macOS 26.7.2/25G332, kext compilada pelo usuário build57,
UUID 4D759E33-4832-38A0-9518-54607CC0D327; app49 no início.
T9UH/RTL8814AU: en3, canal36/5GHz, WPA2/CCMP autorizado, USBerror0.
RTL8192EU: en1, presente e procurando redes de 2,4GHz, sem link/IP naquele
momento, USBerror0. LIST/STATUS/MAP passaram nos dois. Não houve nova associação
forçada para mudar esse estado. A rota padrão usa en3; serviço en3 precede en1.
GET HTTPS vinculado a en3 retornou HTTP200/577 bytes em aproximadamente65 ms.
Isso verifica entrega de dados e não mede velocidade sustentada.

O EU ocioso repetia canais com a mesma geração de busca. A busca infinita vem
da política importada de procurar AP; a alteração desta entrega separa busca
para listar redes da busca que procura o perfil de JOIN. A validação nova do
rádio precisa da build58 carregada pelo usuário.

## Publicação e limites

Diretórios gerados build-*/out-*, SDK e Release estão excluídos do Git e do ZIP
de source. SDK local ou extraído sem Git é verificado por manifesto completo
Headers/Library e revisão quando existe metadata Git. Dependência divergente
é recusada e preservada. Firmware, licenças e notices de terceiros permanecem.

O source não contém dados de autenticação reais identificados pela varredura
realizada; testes usam somente fixtures. Esse exame tem limite de padrões e
não é prova matemática de ausência de toda informação privada.

Os limites documentados na matriz continuam: RTL8814AU HT20/até dois fluxos,
RTL8814BU estação fechada, canal144 fechado e sem WPA3/AX/modo AP. Não há
homologação física de todas as 262 identidades elegíveis/revisões. Contadores
RX nativos continuam zerados apesar de tráfego entregue; Skywalk está ativo.
A causa exata dessa divergência ainda não foi demonstrada. Não há incrementos
manuais ou alteração de capacidades para mascarar essa observação.
