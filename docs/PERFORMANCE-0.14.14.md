# Desempenho e diagnóstico — build61/app52

Esta rodada continua a auditoria das 14 capturas da build55. Os resultados
históricos permanecem em ADAPTER-TESTS-20261101.md e nos CSVs associados. A nova
implementação ainda não foi carregada nem validada fisicamente pelo agente.

| Famílias | Mudança no source | Limites mantidos |
|---|---|---|
| RTL8192CU | HT20/MCS e SGI20, máscara RA/H2C de firmware, QoS, RX e acordos BA | Até 2 fluxos conforme a placa; sem HT40/5 GHz |
| RTL8188EU | HT20/MCS e SGI20, adaptação AMRR por relatórios CCX, descritores, QoS, RX e acordos BA | 1 fluxo; sem HT40/5 GHz; I-cut não suportado |
| RTL8821CU | HT40 e VHT20/40/80 em 5 GHz não DFS; largura/calibração/RA/RX/coexistência coerentes | 1 fluxo; 2,4 GHz e DFS em HT20; canal 165 em 20 MHz |
| RTL8822BU/CU | Mesmos modos B/C, com máscaras limitadas às cadeias e ao peer | Até 2 fluxos conforme eFuse/coexistência; mesmos limites de banda/largura |
| RTL8812AU/RTL8821AU | Correção do offset e dos bits de calibração OTP para potência VHT80 | ROM ausente/inválida continua recusada; não usa valores inventados |

Os canais largos B/C ficam nos blocos não DFS 36–48 e 149–161. Cada componente
de 20 MHz precisa ser permitido pelo perfil e pela política regulatória. VHT20
não anuncia MCS9 para 1/2 fluxos. Máscaras de transmissão seguem as capacidades
de recepção do AP; descritores e relatórios C2H usam a largura negociada.
Calibração ou falha de RA/media encerra a inicialização do link.

Na RTL8188EU o firmware não é marcado como controlador de taxa. A adaptação
usa uma lista limitada às taxas negociadas, com feedback real CCX; respostas
USB não são tratadas como sucesso RF. O wrapper AMRR anterior mantém sua API.

A correção AU usa os tipos e a função de potência da revisão Linux já fixada
no projeto. Os campos de 80 MHz ficam no nibble alto dos bytes 38/39 do bloco de
potência de 42 bytes, depois do campo OFDM de dois bytes. O gerador de goldens
compila a referência primária e deriva o layout, sem copiar a fórmula defeituosa.
Os vetores cobrem valores positivos/negativos, diferenças entre80/160MHz,
1/2 fluxos e caminhos RF. Isso não prova a causa física da falha 2357:0120 nem
resolve por inferência a ausência de 2,4 GHz daquela captura.

## Capacidades e negociação no canal escolhido

As requisições de associação e busca usam as capacidades locais permitidas
no canal de destino. O caminho comum não anuncia HT40/SGI40 em canais registrados
somente para20MHz, nem VHT em2,4GHz ou em canais sem VHT registrado. A negociação
também respeita o lado do canal secundário e o limite de largura registrado,
incluindo DFS20 e canal165. O catálogo global de canais/capacidades permanece
intacto para a próxima busca ou troca de banda. Os testes executam os geradores
reais de requisição e as funções reais de negociação, sem transmissão no rádio.

## Contadores nativos

A fronteira de entrega ao IONetworkInterface registra duas leituras nativas
na primeira entrega não vazia e, depois, a cada 1024 lotes. São observações de
pacotes/bytes/erros RX/TX, índice e capacidades, com validade separada. Não
exporta ponteiros, endereços ou conteúdo de quadros; não altera contadores,
capabilities, processamento ou propriedade de pacotes. O coletor trata essas
amostras como valores de um lote, sem calcular deltas entre amostras diferentes.

O RX nativo zerado continua sem causa confirmada. As capturas mostram entrega
Ethernet positiva; a análise do binário instalado confirma o caminho de entrada,
mas não comprova uma opção suportada para corrigir a contagem em macOS. A
instrumentação serve para separar entrega, identidade e contagem adiada.

## Validação e desempenho físico

VERIFICATION.json na entrega registra os testes executados, builds nativos,
revisões e hashes exatos. Testes de software verificam lógica, descritores,
negociação, calibração e falhas modeladas; não medem download/upload. A revisão
por outra pessoa/agente também é limitada ao código e aos modelos utilizados.

Ainda é preciso medir conexão, tráfego sustentado, recuperação e taxas reais
nos adaptadores e revisões correspondentes. Isso não impede as correções de
source desta rodada; impede afirmar que o pack já foi certificado em todas
as placas. RTL8814BU continua sem estação habilitada; RTL8814AU permanece HT20.
