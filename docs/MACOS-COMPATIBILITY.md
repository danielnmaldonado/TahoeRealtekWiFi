# Compatibilidade do pacote com macOS

Escopo: kext 0.14.14/build61, app 0.3.11/build53 e coletor revisão2, Intel
x86_64. O pacote é de desenvolvimento. Esta mudança amplia os sistemas alvo
do app/helper/instalador; não altera o rádio nem comprova compatibilidade física.

| Sistema | Darwin | Alvo do pacote | Carregamento, WPA2 e tráfego nesta versão |
| --- | --- | --- | --- |
| Ventura 13 | 22 | Sim, mínimo13.0 | Pendente em hardware |
| Sonoma 14 | 23 | Sim | Pendente em hardware |
| Sequoia 15 | 24 | Sim | Pendente em hardware |
| Tahoe 26 | 25 | Sim | Pendente para esta entrega |

Os14logs históricos foram capturados no macOS12.7.6 com kext55 e app48.
Eles não homologam este pacote nos quatro sistemas da tabela. As limitações
por chipset, revisão, OTP, potência e canais continuam descritas na
[lista de suporte](USB-WIFI-SUPORTE.txt) e na [reauditoria14](RE-AUDIT-14-20261102.md).
As duas placas sem funcionamento comprovado, RX nativo zerado e os eventos
CCMP da captura antiga continuam pendentes; mudar o mínimo do sistema não
resolve essas questões.

## O que foi preparado

- AppKit e helper compilados com `-mmacosx-version-min=13.0`, incluindo as
  compilações de testes. `LSMinimumSystemVersion` e o mínimo do `.pkg` são13.0.
- Erros de disponibilidade habilitados na compilação para detectar chamadas
  sem proteção a APIs posteriores ao Ventura. `SMAppService`, a aprovação do
  serviço e o início da sessão continuam usando as APIs já presentes no projeto.
- O empacotador inspeciona os comandos de versão dos executáveis Mach-O do
  app e helper. Alterar somente o plist não permite empacotar um binário que
  exija macOS26 como se fosse compatível com13.
- O código da kext61 permanece igual. O projeto mantém deployment11.0 e os
  mínimos existentes de `OSBundleLibraries`. A kext continua publicando Ethernet
  e usa net80211 próprio; não precisa de uma variante de IO80211/AirPort para
  cada sistema. Deployment11.0 é configuração de compilação: o Mach-O da kext
  não contém um comando de versão mínima equivalente ao dos aplicativos.
- Nome, bundle IDs, protocolo de controle, preferências por adaptador/rede e
  itens do Chaves são preservados. Nenhum dado do usuário precisa ser migrado
  por esta mudança de mínimo do sistema.

## Evidência e limites

O ambiente disponível para compilação é macOS26.7.2, Xcode26.6 e SDK26.5.
O mínimo da aplicação é13.0; isso permite gerar um binário para Ventura usando
um SDK mais novo. Não é uma execução no Ventura. Builds com Xcode/SDK antigos
continuam sem teste próprio. Os resultados exatos de build, testes, assinaturas
e manifestos ficam no `VERIFICATION.json` ao lado do ZIP de entrega.

Na auditoria estática, os419imports de KPI declarados usados pela kext61
aparecem nas listas oficiais de exportação da Apple nos XNU correspondentes a
Darwin22/23/24/25. Os demais imports são das famílias USB/Ethernet. Presença de
símbolo não comprova layout de classe C++, comportamento do kernel, instalação
ou estabilidade de USB em cada versão; essas etapas precisam de teste físico.

A Apple declara sem suporte a combinação de KPIs USB e IONetworkingFamily
desde Big Sur. Esta entrega continua no modelo de kext do projeto; a auditoria
não o transforma em DriverKit nem em driver aprovado para instalação padrão.
Fontes primárias: [extensões de kernel e alternativas](https://developer.apple.com/support/kernel-extensions/),
[SMAppService](https://developer.apple.com/documentation/servicemanagement/smappservice).

## Verificação e teste físico

Para compilar a kext, o app e os testes a partir do source completo:

```sh
bash ./build.command --all
bash ./app/test.command
bash ./app/package.command --no-build
python3 ./app/check-macos-compatibility.py --json \
  "Release/Tahoe Wi-Fi USB.app/Contents/MacOS/TahoeUSBWiFi" \
  "Release/Tahoe Wi-Fi USB.app/Contents/Helpers/TahoeWiFiHelper"
```

Em cada sistema da tabela ainda é necessário verificar o carregamento real,
scan/refresh, seleção entre dois USB, WPA2/DHCP, tráfego sustentado em2,4/5GHz
quando aplicável, reboot, suspensão e remoção/reconexão do USB. Executar o
coletor revisão2 antes e depois permite relacionar erros e contadores à mesma
sessão. Uma medição de throughput exige teste de rede real; taxa de link ou
compilação aprovada não a substituem.

Esta entrega não instala o app, não troca a kext carregada e não modifica EFI,
políticas de segurança ou configurações do roteador. Publicação estável depende
da validação acima e das pendências da reauditoria14.
