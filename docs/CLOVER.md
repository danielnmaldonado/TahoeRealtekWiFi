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

A build55 (driver 0.14.8) e o app47 (0.3.5) estão em desenvolvimento.
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

A build55 ainda exige carregamento físico posterior pelo usuário. Os testes de
software não demonstram o canal de5 GHz do roteador em automático nem validam
todas as placas/revisões. Detalhes em DFS-STATION.md e VALIDATION-0.14.8.md.
