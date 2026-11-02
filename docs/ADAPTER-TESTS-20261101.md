# Auditoria das 14 placas — 01/11/2026

Registro histórico da build55, redigido na rodada60. Para o estado do source61 e pendências atuais, consulte [a reauditoria de02/11](RE-AUDIT-14-20261102.md).

Os 14 grupos foram fornecidos pelo usuário: 12 funcionam e dois falham. As capturas usam macOS12.7.6/21H1320, kext55 e app0.3.5/build48; não são validação física da build60. A versão atual continua direcionada ao Tahoe/Intel.

Nas 12 conexões há WPA2 autorizado, interface ativa com IPv4/rota e contadores de entrega Ethernet avançando. Isso comprova conectividade local e recepção durante a janela, sem demonstrar teste de internet, velocidade sustentada ou ausência de todos os descartes.

| Captura | USB VID:PID | Família | Modo observado | Erros/descartes na janela |
|---|---|---|---|---|
| 18:49:15 | 0BDA:B812 | RTL8822BU | HT20 / 2 fluxo(s); canal 157 | sem incremento de erro RX/TX/USB nos contadores disponíveis |
| 18:54:14 | 2357:010C | RTL8188EU | b/g, sem HT; canal 6 | replay CCMP +2; decrypt +2; RX net80211 +2; duplicados +7 |
| 18:58:06 | 2357:0107 | RTL8192EU | HT40 / 2 fluxo(s); canal 6 | duplicados +2; BA abaixo da janela +3 |
| 19:00:28 | 2357:0109 | RTL8192EU | HT40 / 2 fluxo(s); canal 6 | BA abaixo da janela +11 |
| 19:02:05 | 0BDA:0179 | RTL8188EU | b/g, sem HT; canal 6 | duplicados +7 |
| 19:03:36 | 0BDA:C811 | RTL8821CU | HT20 / 1 fluxo(s); canal 157 | sem incremento de erro RX/TX/USB nos contadores disponíveis |
| 19:04:55 | 0BDA:C811 | RTL8821CU | HT20 / 1 fluxo(s); canal 157 | BA abaixo da janela +2 |
| 19:06:32 | 0BDA:8179 | RTL8188EU | b/g, sem HT; canal 6 | sem incremento de erro RX/TX/USB nos contadores disponíveis |
| 19:08:00 | 20F4:808A | RTL8822BU | HT20 / 2 fluxo(s); canal 157 | BA abaixo da janela +2 |
| 19:09:42 | 0BDA:8178 | RTL8192CU | b/g, sem HT; canal 6 | duplicados +1 |
| 19:11:07 | 2357:0115 | RTL8822BU | HT20 / 2 fluxo(s); canal 157 | sem incremento de erro RX/TX/USB nos contadores disponíveis |
| 19:13:20 | 7392:A611 | RTL8723BU (perfil USB) | sem controlador | start USB rejeitado, 0xe00002c7 |
| 19:15:01 | 2357:0108 | RTL8192EU | HT40 / 2 fluxo(s); canal 6 | sem incremento de erro RX/TX/USB nos contadores disponíveis |
| 19:20:26 | 2357:0120 | RTL8821AU | sem canal/associação | canal/SCAN/RX/WPA sem avanço |

## Velocidade

Nenhuma captura contém teste de download/upload. IOLinkSpeed=0 e LinkSpeedKnown=0 significam taxa desconhecida. RXLastRate e histogramas incluem quadros de gerenciamento e amostras cumulativas; não são velocidade da conexão ou capacidade máxima. O JSON anexo conserva taxas/MCS/SGI/histogramas para cada grupo.

- RTL8192EU: as três amostras negociaram HT40 com dois fluxos.
- RTL8821CU/RTL8822BU: cinco amostras operaram HT20 em 5GHz; nenhuma negociou VHT. A implementação B/C da build55 capturada era HT20. O source61 acrescenta os modos indicados na reauditoria.
- RTL8188EU/RTL8192CU: quatro amostras estão no caminho b/g, sem HT, como definido no source55 capturado. O limite de implementação reduz a velocidade possível desses adaptadores N.
- RTL8821AU2357:0120: sem modo negociado, sem dados suficientes para avaliar velocidade.

## Todos os contadores e a interpretação

O CSV ADAPTER-ERRORS-20261101.csv lista 58 observações não nulas de erros/filtros: valores antes/depois/delta, categoria e significado. Não são 58 falhas distintas. Vários contadores refletem o mesmo descarte, e muitos são históricos sem incremento na captura. SignalForeignFrames/SignalMissingPHY filtram medição de sinal e não contam perda total de dados.

A 2357:010C teve replay CCMP+2, decrypt+2 e inputErrors+2, além de duplicados+7. A conexão permaneceu autorizada. A causa desses reenvios/descartes não foi registrada; não há fundamento para desabilitar proteção CCMP nem afirmar falha USB ou senha errada. Os demais grupos contêm duplicados/BA e alguns erros históricos sem avanço, conforme o CSV.

Nas 12 placas conectadas: erros USB/TX, recuperação RX e erros de agregação RX têm valores/deltas zero. Os totais RX nativos ficam zero mesmo com entrega Ethernet crescente; essa divergência de contagem permanece aberta, sem incrementos artificiais.

## Correções na build60

- Edimax7392:a611: o source rejeitava quatro OUT físicos, apesar do mapeamento Linux de até três filas lógicas. Corrigidos armazenamento, descoberta, inicialização e roteamento de filas, com limites e guardas Bluetooth preservados. O log registra seis endpoints e falha de descoberta; falta o descritor bruto para confirmar que quatro OUT são a causa física exata.
- AU8812/8821: firmware MCU residente antes do boot agora passa pelo reset limitado usado no Linux. Antes retornava EBUSY sem recuperação. Não é possível afirmar que a 2357:0120 estava nesse estado. A nova telemetria mostra estágio/erro original/reset e resultados de potência nos canais de referência, sem ignorar a calibração de fábrica.
- Caminho comum de recepção: corrigidos limites da cópia de cache IE e propriedade de memória em falta de alocação. Os dois panics históricos incluídos no ZIP são compatíveis com esse caminho; UUID/binário exato não foi localizado, portanto não há causalidade física comprovada.
- Coletor: mostra a falha no provider USB mesmo sem controlador, preserva estágios e erros AU e distingue resultados publicados de novas operações.

## Captura e validação

Os mesmos arquivos de panic foram repetidos nos 14 ZIPs; representam dois eventos anteriores, não 14 crashes novos. Há conflito entre cabeçalho e corpo quanto à versão do macOS. Cinco consultas de log terminaram por timeout e o comando Gatekeeper retornou1 em todos os grupos; os timeouts são falhas de coleta; o exit1 com assessments disabled descreve estado do Gatekeeper. Nenhum dos dois comprova falha de rádio.

O pacote inclui testes de software, comparação com Linux fixado e compilação nativa a partir do source. Edimax e2357:0120 precisam de novo teste físico na build60. Os 12 resultados anteriores também precisam de regressão de conexão/tráfego ao mudar o driver. Não há benchmark, teste RF ou homologação universal implícita.
