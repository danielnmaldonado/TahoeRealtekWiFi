# Compilar e carregar pela Clover

Este projeto expõe o adaptador como interface Ethernet, com o net80211
responsável pela associação e pelo WPA2. O app da barra de menus controla a
interface USB selecionada. O carregamento da kext e o helper do app são etapas
separadas.

## Compilação

Na raiz do source, execute:

```sh
./build.command --kext
./app/build.command
```

Os produtos ficam em `Release/`. O source inclui a kext, o app, o helper,
os testes e os geradores. As assinaturas ad-hoc são aplicadas e verificadas
pelos scripts de compilação.

## EFI

Monte a EFI que efetivamente inicia o computador pela Clover. Identifique a
partição antes de copiar: o número `diskXs1` pode mudar entre reinícios.
Guarde o backup da kext anterior fora de `EFI/CLOVER/kexts/Other`, para evitar
a injeção simultânea de duas cópias com o mesmo identificador.

Copie o bundle inteiro `Release/TahoeRealtekWiFi.kext` para:

```text
EFI/CLOVER/kexts/Other/TahoeRealtekWiFi.kext
```

A configuração da Clover precisa estar habilitada para injetar essa kext.
O bundle novo será usado no próximo boot pela EFI correta. A cópia não troca
o driver já carregado em memória. Os scripts de build não reiniciam o Mac,
não alteram a configuração da Clover e não mudam SIP/AMFI.

Neste computador, `kmutil load` foi recusado por aprovação, enquanto a Clover
é o método de carregamento indicado pelo usuário. Abrir novamente os Ajustes
de Segurança não substitui a injeção pela EFI.

## App

O instalador do app coloca `Tahoe Wi-Fi USB.app` em `/Applications`.
O pacote instala o app e seu helper; a kext é copiada separadamente para a EFI.
A aprovação do helper, quando solicitada pelo macOS, fica em Itens de Início.
Isso não é a aprovação de uma extensão de kernel.

Escolha o adaptador no menu, atualize as redes e digite a senha no app.
A senha pode ser salva no Chaves do macOS. Nenhuma credencial pessoal é
necessária para compilar o projeto.

## Conferência após o boot

```sh
kmutil showloaded --list-only
ioreg -r -c RTWUSBController -l
```

Confira versão, build, família e interface do adaptador selecionado.
O registro da kext carregada e a existência de `enX` não demonstram, sozinhos,
WPA2, DHCP ou tráfego. A validação precisa conferir autorização da porta,
endereço IP, recepção/transmissão e o caminho de rede usado pelo teste.
Com Ethernet cabeada também conectada, um teste de velocidade pode usar a
interface cabeada.

## Estado desta árvore

A build57 (driver 0.14.10) e o app49 (0.3.7) estão em desenvolvimento.
Os 13 backends operacionais estão integrados, com 262 IDs elegíveis. Há
263 identidades verificadas. O RTL8814B possui HAL/USB e PHY próprios, mas
a estação aguarda firmware NIC/protocolo qualificados e validação física.
Sete registros permanecem não confirmados ou conflitantes.
A identificação numérica não comprova todas as revisões de placa.

O RTL8814AU conectado passou testes físicos de rádio e recepção nas duas
faixas não DFS de 5 GHz. Associação WPA2, DHCP e tráfego com a nova kext ainda
precisam de validação após seu carregamento. As outras famílias exigem os
adaptadores correspondentes para validar conexão, tráfego e coexistência.
Consulte README, matriz e relatório de validação antes de divulgar compatibilidade.

## DFS na build55

App e kext precisam ser recompilados: o app47 aceita os grupos DFS que a kext55
realmente registra, sem liberar canais em um driver antigo. Os sete backends
dual band incluem DFS HT20 em52–64/100–140, sujeitos a ROM/RFE, país e potência.
Use Refresh networks para uma busca real; a banda vista em cache não comprova
a banda da associação. Confira o canal atual nos detalhes. O canal 144 permanece
fechado. Troca de canal pelo AP interrompe TX e refaz associação/WPA2.

A build57 ainda exige carregamento físico posterior pelo usuário. Os testes de
software não demonstram o canal de5 GHz do roteador em automático nem validam
todas as placas/revisões. Detalhes em DFS-STATION.md e VALIDATION-0.14.10.md.

## Recuperação na build56

A build56 acrescenta recuperação manual limitada de falhas elegíveis no RTL8814AU,
com drenagem, desligamento verificado e reidentificação. Corrige também o registro
de energia pela família IOKit e preserva erros tardios de RX inicial. O app48 já
foi instalado localmente; a kext56 não foi carregada pelo agente. Não há prova de
estabilidade física desta nova kext. RTL8814AU permanece HT20, com até dois fluxos.

## Dois USB e refresh na build57/app49

App49 foi instalado em Aplicativos e consultou os dois USB reais com sucesso.
O kernel ainda usava a build56: AU em 5 GHz/canal 36 e EU em 2,4 GHz/canal 6, ambos
autorizados e com HTTPS funcional separado por interface. A nova build57 precisa
ser compilada/copiar o bundle inteiro na Clover e carregar no próximo boot.
O refresh automático do app49 é por adaptador selecionado e ocioso; uma conexão
autorizada mantém cache para preservar o link. Consulte VALIDATION-0.14.10.md.

## Preferência e recursos — build58/app50

A entrega atual usa driver0.14.11/build58 e app0.3.8/build50. O app restaura a
placa física por identidade, conserva a rede por placa e trata a enumeração
atrasada. As correções de recursos e busca comum exigem a kext nova; compilar
não troca o código carregado no kernel. Consulte VALIDATION-0.14.11.md e o
VERIFICATION.json da entrega.

## Auditoria de 14 placas — build60/app51

Use driver0.14.13/build60 e app0.3.9/build51 para a regressão desta entrega.
A kext deve ser compilada e copiada como bundle completo, mantendo backup.
O novo coletor exporta USB-STARTUP e etapas AU mesmo quando o controlador
não chega a publicar uma interface. Consulte ADAPTER-TESTS-20261101.md para
erros e limites de velocidade de cada captura e VALIDATION-0.14.13.md para
o alcance dos testes de software e o que ainda exige hardware.
