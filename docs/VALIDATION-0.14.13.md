# Validação — driver0.14.13/build60, app0.3.9/build51

Projeto/adaptação macOS: MaLd0n. Desenvolvimento.

Consulte ADAPTER-TESTS-20261101.md para a auditoria individual, CSVs de todos os
contadores e JSON de modo/taxas/histogramas. Os dados de entrada foram capturados
em outro sistema com kext55; não comprovam o comportamento físico desta entrega.
VERIFICATION.json registra resultados exatos dos testes e hashes da entrega.

A build60 trata o mapeamento8723BU de quatro OUT físicos para três filas, reset
MCU residente8812/8821, limites/propriedade de memória do cacheIE e observabilidade
do start USB/AU. IOBuiltin=Yes e as correções anteriores são preservados.
O app51 incorpora o catálogo USB atualizado; preferência e reconexão mantêm a
implementação anterior. O coletor inclui novos campos através de allowlists.

Os testes de memória usam buffers locais genéricos e falha de alocação; não
constroem quadros de rádio nem reproduzem panics em hardware. Os testes de USB
modelam os endpoints e falhas de transporte; não comprovam o descritor físico
não capturado da Edimax. A recuperação warmAU é demonstrada em código/modelo,
sem prova de que era a causa da falha da2357:0120. A omissão de2.4GHz nesse log
exige a nova evidência de potência/ROM; guardas de fábrica permanecem intactos.

As matrizes de suporte e limites de HT/VHT/ROM continuam no README. Contagem RX
nativa e medições sustentadas de velocidade permanecem sem resolução/medição.
Não houve nova instalação, carregamento da kext, alteração de EFI, roteador,
segurança do macOS, credenciais ou reboot nesta rodada de análise de arquivos.
