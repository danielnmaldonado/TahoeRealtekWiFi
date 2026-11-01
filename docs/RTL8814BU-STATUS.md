# RTL8814BU — estado do source

Projeto e adaptação macOS: MaLd0n. O backend de estação continua desabilitado.

## Componentes presentes

O HALMAC/USB é próprio da geração B14: identificação positiva SYS_CFG2=0x11,
OTP de 1024 bytes, descritores TX de corpo de 16 bytes com IEs e RX de 24 bytes, fluxo de firmware
USB/reserved-page/DDMA/MCU e verificação de propriedade do contexto. Não usa
HAL, descritores ou tabelas RF do RTL8814AU.

O PHY/RFK importa 159 funções da revisão fixada em
`src/RTW8814BPHY-Upstream.json`: tabelas BB/AGC/RFK-init, quatro caminhos RF e
dois sintetizadores, DACK, RTC/LCK, TXGAPK, IQK e DPK. Os bypasses de PA externo
seguem a fonte primária. Os contextos de calibração pertencem a cada objeto.
Uma lease privada do HAL limita o PHY à sessão de firmware atual; transições
de potência, falhas e fechamento invalidam essa relação sem USB I/O.

As fontes, cabeçalhos, geradores e notices estão no source. Os manifestos
registram o commit primário e as adaptações locais; licenças são preservadas.

## O que ainda impede habilitar estação

- Firmware USB NIC licenciado e protocolo de estação qualificados. O parser
  aceitar cabeçalho/ABI11 não prova compatibilidade operacional. Imagens de
  roteador ABI15 e de AU são distintas; nenhum firmware proprietário extraído
  de driver Windows é distribuído neste pacote.
- Perfil USB real de fábrica/regulatório, incluindo produção dos metadados
  RFE, pacote, front-end e cristal a partir do OTP. O plano de potência recebido
  pelo PHY é validado quanto à representação; não é derivado de um perfil
  USB de fábrica/regulatório já qualificado.
- A leitura dos dados de fábrica TSSI/thermal existe. Programação TSSI DE/DCK,
  runtime genérico TSSI/thermal swing e CFO tracking não foram portados.
  `track` executa atualmente apenas o tracking DPK da fonte primária.
- Integração de estação e validação física: associação/WPA2, DHCP, tráfego,
  reconexão, suspensão, remoção USB e diferentes revisões de placa.

A geometria do componente é limitada a HT20 não DFS: canais 1–11, 36/40/44/48
e 149/153/157/161/165. A implementação deste componente não habilita uma
personalidade de estação B14 nem altera o catálogo para anunciar conexão.

## Verificação

Os testes de host exercitam os algoritmos importados com DSP/registradores
modelados, falhas de I/O e restauração de estado, com ASan/UBSan. O HAL real
possui testes separados de firmware, OTP, codecs e ciclo de vida da lease.
Esses resultados não são medições RF nem validação de throughput em hardware.
A compilação nativa usa os cabeçalhos reais do kernel e checa frames de pilha.
