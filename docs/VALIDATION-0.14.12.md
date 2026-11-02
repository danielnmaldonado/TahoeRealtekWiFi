# Validação — 0.14.12/build59, app0.3.8/build50

Projeto/adaptação macOS: MaLd0n. Entrega de desenvolvimento.

A única alteração de comportamento desde a build58 é a propriedade booleana
IOBuiltin=Yes na IOEthernetInterface. O setter fica em configureInterface,
após validação do tipo/estatísticas e antes de ether_ifattach e publicação.
Não há escrita no provider USB nem em IOPrimaryInterface, nomes BSD ou rotas.
A instrução fica no caminho comum de todas as famílias que publicam interface.

O app50 e todos os caminhos de rádio, associação, WPA2, RX/TX e recuperação
mantêm o source anterior. As evidências dessa rodada estão no VERIFICATION.json
da entrega. Compilação nativa/assinatura validam o uso do setter do SDK;
IOBuiltin no hardware só pode ser conferido após carregar a build59.
A entrega não substitui a kext em memória nem reinicia o computador.

Para conferir após o boot, examine a IOEthernetInterface filha de cada
RTWUSBController no IORegistry: IOBuiltin deve ser um booleano Yes.
Os limites anteriores permanecem em VALIDATION-0.14.11.md e README.
