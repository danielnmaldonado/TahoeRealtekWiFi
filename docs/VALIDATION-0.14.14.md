# Validação — driver0.14.14/build61, app0.3.10/build52

Projeto/adaptação macOS: MaLd0n. Desenvolvimento.

A entrega contém o source correspondente da kext, app, auxiliar, testes,
geradores, referências e licenças. PERFORMANCE-0.14.14.md descreve os modos
implementados e seus limites. ADAPTER-TESTS-20261101.md mantém a análise dos
14 logs históricos, capturados com a build55 em macOS12.7.6.

VERIFICATION.json registra os resultados exatos dos testes host e sanitizers,
reconstrução do ZIP, suites nativas do app, coletor e produtos nativos. As
assinaturas ad-hoc validam a integridade local dos bundles; não equivalem a
notarização ou certificação de funcionamento universal.

Não houve instalação nova do app, carregamento da kext, alteração de EFI,
roteador, segurança do macOS, credenciais ou reboot nesta rodada. Não há
benchmark físico de velocidade desta build. Os novos modos HT/VHT e a correção
de potência exigem regressão física nos respectivos adaptadores/revisões.

A contagem RX nativa zerada permanece em investigação. A causa exata da falha
2357:0120 e da omissão de 2,4 GHz não foi demonstrada pelo arquivo antigo; os
novos campos de boot/potência continuam disponíveis para a próxima captura.
A Edimax 7392:a611 precisa confirmar a descoberta na sua topologia real com a
correção anterior. Não se declara os 14 casos ou todos os modos certificados.
