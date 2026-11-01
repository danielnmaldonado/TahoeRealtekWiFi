# Validação de desenvolvimento — 0.14.1/build48, app 0.3.2/build44

Projeto e adaptação para macOS: DNMTechLabs. Estado: desenvolvimento.
Este relatório separa evidência de código da validação física de estação.

## Catálogo

Há 270 registros, preservando os 250 do pacote original e 20 extensões.
263 possuem identidade primária verificada; 262 são elegíveis aos 13 backends
de estação integrados. As 32 novas confirmações usam evidência preservada no
registro de fontes, com restrições OEM e de interface.

Cinco IDs continuam sem correspondência primária suficiente: 038b:0100,
058c:ff20, 0bda:8011, 0e0b:9071 e 1668:8105. Os IDs 13b1:0043 e 0bda:8192
possuem fontes conflitantes. A identidade de uma unidade observada não altera
silenciosamente o catálogo nem autoriza outra revisão.

Os perfis 04bb:0959, 04bb:095a e 0bda:2003 são exclusivos de 5 GHz segundo a
instalação OEM. O controlador rejeita 2,4 GHz antes de I/O; o app interpreta
as bandas explícitas publicadas pelo driver, preservando compatibilidade
com o protocolo anterior.

## RTL8814BU

O source inclui HALMAC/USB, OTP e codecs próprios, importados de uma revisão
primária fixada. A compilação desse componente, a reprodução do gerador e a
revisão independente passaram com falhas de transporte antes e depois do
efeito, verificação de propriedade e limites reais de pilha do kernel.
O vínculo de ciclo de vida entre PHY e HAL recebeu revisão estática separada
e testes locais de lease; nenhuma revisão de modelo é homologação física.

O PHY/RFK próprio está integrado como componente bloqueado para estação. O caminho
de estação permanece desabilitado: ainda exige firmware NIC licenciado e
qualificação do protocolo de estação, perfil USB de fábrica/regulatório e
funções de tracking restantes, além do hardware correspondente. Consulte
[os limites do componente](RTL8814BU-STATUS.md).
Uma assinatura de firmware semelhante à de AU não é prova de compatibilidade.

## Verificação de software

A bateria integrada com o PHY e a descoberta de silício passou 147/147
testes CTest em Release e os mesmos 147 com ASan/UBSan nos alvos de host
(62,15 s no último passe instrumentado). Os testes de bandas incluem os métodos
reais do controlador e rejeição de transmissão em banda bloqueada, sem USB I/O.
O app 0.3.2/build44 passou os 15 executáveis nativos, incluindo a compilação
AppKit. O fluxo de entrega extrai o ZIP em outra pasta, repete os testes e
compila app, kext e instalador; confere assinaturas, hashes e o payload
app/helper do instalador contra os produtos dessa mesma árvore extraída.
Os geradores reproduziram 72 arquivos HALMAC e 16 artefatos PHY, com 159
funções reais e nove arrays primários. O PHY passou compilação KernelSDK
com limite de frame de pilha de 2.048 bytes.

Os testes usam modelos, fixtures de firmware e métodos reais extraídos. Eles
não equivalem a um conjunto de placas fisicamente homologadas. Instrumentação
ASan/UBSan pertence aos alvos de host, não ao kernel.

## Sistema físico e lançamento

Foram observados dois adaptadores: RTL8814AU 2357:0106 e RTL8192EU 2357:0108.
A auditoria inicial desta sessão não encontrou a kext carregada nem um serviço
RTWUSBController. Os resultados históricos de recepção no AU não comprovam
WPA2, DHCP ou throughput com a nova build. Tráfego da Ethernet integrada não
é considerado tráfego dos adaptadores USB.

A validação de lançamento requer carregamento real, WPA2, DHCP, reconexão,
remoção USB, suspensão e tráfego sustentado nas famílias/revisões pertinentes.
Compilar não altera SIP/AMFI nem reinicia o computador. Não se declara suporte
completo ou homologação dos 270 registros enquanto faltarem essas evidências.

## Entrega

Os pacotes e fontes para compilação ficam na pasta `releases` deste projeto.
O Desktop não é usado como destino. Cada pacote novo deve conter instruções
de compilação, manifesto de arquivos e SHA-256; licenças obrigatórias e
proveniência das dependências permanecem junto do source.
