# Correções do checkup da build53

O usuário pediu corrigir os problemas encontrados, diretamente no source. A conexão
atual deve permanecer utilizável; não instalar, recarregar ou reiniciar o driver.

## Evidência

Build53 carregada, RTL8814AU2357:0106/en3, WPA2 autorizado no canal6.
Leituras reais de21498bytes, sem erro USB ou agregado truncado. No checkup,
90/90 pings responderam; transferências curtas passaram pela interfaceUSB.
TXQueueDrops12132 e TXLinkDrops5 não aumentaram, mas a fila cheia descarta
pacotes em vez de sinalizar pressão ao output queue. Netstat relata RX0 apesar
de tráfego recebido. Sete erros de entrada novos não têm causa exportada.
Diagnósticos HT/BA não são publicados no AU por retorno antecipado da calibração.

## Abordagem

Corrigir a admissão TX com o contrato IOKit de stall/retry e ownership, verificar
a contabilidade no ponto de entrega Ethernet e completar os diagnósticos sob o
gate do controlador. Rejeitados: aumentar filas para esconder perdas; zerar
contadores; interpretar metadados Ethernet54Mb/s como taxaRF; classificar os sete
erros sem evidência. A causa RX será reproduzida antes de mudar o código.

Preservar ABI de controle, buffers32KiB AU, C2H termal, políticasRF, chaves,
WPA2/CCMP, recuperaçãoUSB e demais famílias. Publicar somente escalares numéricos;
nenhum dado de pacote ou chave. Sem atribuir sete descartes a falha específica
sem contador que a demonstre. O meio Ethernet deve declarar velocidade apenas
quando houver taxaPHY válida do backend; não inventar capacidade por throughput.

## Verificação e entrega

Cada correção terá teste comportamental falhando na53 e passando na implementação,
com ownership, filascheias, término, múltiplos adaptadores e publicação coerente.
Executar suíte Release/sanitizadores, compilaçãoSDK e app nativo. Revisão independente
antes de congelar. Entregar build54 com source integral de app e kext, manifesto e
compilação do próprioZIP. Validação física da54 exige carregamento pelo usuário.
