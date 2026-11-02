# DFS de estação — build55/app47

Projeto e adaptação macOS: MaLd0n.

## Canais e famílias

RTL8192DU, RTL8812AU, RTL8814AU, RTL8821AU, RTL8821CU, RTL8822BU e RTL8822CU
recebem DFS HT20 em 52/56/60/64 e
100/104/108/112/116/120/124/128/132/136/140. Os canais não DFS existentes continuam.
O mapa registrado depende de ROM/RFE, domínio de fábrica, calibração e potência.
As restrições OEM exclusivamente 5 GHz são preservadas. Canal 144 permanece fechado:
não se extrapolam tabelas primárias AU/DU ausentes a partir do canal 140. Não se anuncia
DFS40/80. O caminho AU negocia20 MHz com APs mais largos; seus modos não DFS não
são reduzidos por esta mudança. As famílias exclusivamente2,4 GHz não ganham DFS.

O app47 aceita dois novos bits de capacidade, mantendo o layout ABIv1.
Cada bit requer o grupo completo no mapa do controlador; um driver antigo ou
um catálogo de chipsets não concede autorização para canais novos. Agrupamento
por SSID continua preservando as bandas efetivamente observadas e utilizáveis.

O seletor somente leitura 5 acrescenta um mapa exato de 256 canais registrados,
com resposta de 48 bytes, versão e reservados zero. Todas as estruturas e os
seletores anteriores mantêm tamanho e significado. Esse mapa evita esconder
canais válidos nos países com faixas parciais (AU/DZ/KP/MY/RU), sem anunciar os
que ficaram proibidos. App vincula mapa ao adaptador e à geração selecionados;
erro/ausência invalida os canais, e somente Unsupported de um driver antigo
permite fallback para os bits de grupos completos. Modelo, reconexão e descrição
usam o mesmo mapa. A consulta não sintoniza rádio nem concede uma licença de TX.

## Descoberta, associação e saída de canal

O cliente busca passivamente. Beacon ESS completo deve corresponder ao SSID
desejado, BSSID/TA, canal realmente sintonizado e canal PHY quando disponível.
O cache é limitado a 32 observações; a permissão de AUTH expira em 30 s. Campos
contraditórios, frames incompletos/estrangeiros e registros antigos não concedem
TX. A permissão é ligada ao BSS, sem liberar probes ou remover NO_IR globalmente.
ASSOC/RUN permanecem ligados ao BSSID/canal admitido. WPA2/CCMP usa o net80211.
Beacons do AP já vinculado atualizam potência mesmo com SSID oculto; o relógio
dessa atualização não renova a permissão de AUTH visível. Isso vale durante
AUTH/ASSOC já vinculados, antes de RUN. Perfis salvos ocultos preservam descoberta
dirigida em 2,4 GHz quando o mapa atual permite essa faixa. Mapa inválido, sem
canais ativos de 2,4 GHz ou placa apenas 5 GHz não libera esse fallback.

CSA/ECSA válido do AP associado interrompe imediatamente transmissão de host e
hardware, baixa o link e fecha a porta controlada. A retenção de TX também cobre
escritas posteriores em TXPAUSE, incluindo endereço MAC1 do 8192DU. O watchdog
executa stop/drain completo antes de liberar a retenção e tentar busca passiva,
associação e WPA2 novamente. O destino anunciado não programa RF diretamente.
Não se envia deauth no canal evacuado nem se encerra recursivamente dentro do RX.
Remoção, sleep, perfil apagado e falhaUSB impedem retomada indevida.

## País e potência

O país conhecido vem dos campos de fábrica já decodificados por família.
DU não reutiliza posições OTP de outro chipset e usa a base world. País ausente
ou desconhecido também usa world; locale, IP e Country IE não substituem o ROM.
`DFSRegulatoryTables.inc` contém183 domínios derivados do wireless-regdb oficial,
com cobertura completa de20 MHz e teto mBm por canal. Uma borda ou lacuna de regra
não é estendida. O gerador verifica SHA256 e gitblob dos três inputs antes de
executar o parser primário ou escrever a tabela.

Antes de AUTH/RFK, o teto DFS é o menor entre world20 dBm, país de fábrica e o
limite do triplet Country do AP quando presente, subtraindo PowerConstraint.
O valor é arredondado para baixo na unidade da PHY. O limite absoluto usa a
conversão SAR por caminho/seção de taxa e base ROM/PG das fontes primárias; a
compensação térmica não ganha margem extra devido ao teto reduzido. Um limite
impossível recusa prontidão/TX: AU faz preflight do plano; DU/B-C podem executar
I/O de canal ainda pausado antes de detectar e manter a falha bloqueada.
Não há uma segunda redução relativa sobre a taxa já limitada. Limite do AP mais
restritivo durante RUN interrompe TX e exige nova admissão. Esses controles de
software não substituem medição de EIRP, ganho de antena e certificação da placa.

## Fontes primárias e reprodução

- Linux fixado em `0e135910deec43ffe97eca0b6af1f8f847285b59`:
  [STATION não exige CAC local](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/net/wireless/chan.c#L1015),
  [CSA de estação](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/net/mac80211/spectmgmt.c#L220),
  [regras rtw88](https://github.com/torvalds/linux/blob/0e135910deec43ffe97eca0b6af1f8f847285b59/drivers/net/wireless/realtek/rtw88/regd.c#L336).
- [wireless-regdb oficial](https://git.kernel.org/pub/scm/linux/kernel/git/wens/wireless-regdb.git),
  revisão `40095bd16e53d5c5682fd6e7e3dbbbfaf0fe5410`.
  Inputs ISC intactos em `vendor/wireless-regdb/`; hashes e origem em
  `src/DFSRegulatory-Upstream.json`.
- PHYs específicos mantêm seus arquivos `*-Upstream.json`, geradores,
  golden fixtures e avisos de licença. Não se copiam tabelas de outra família.

```sh
python3 scripts/generate-dfs-regulatory.py --check
cmake -S . -B build-dfs -DCMAKE_BUILD_TYPE=Release
cmake --build build-dfs --parallel 4
ctest --test-dir build-dfs --output-on-failure --parallel 4
bash ./build.command --kext
bash ./app/build.command
```

É modo cliente, sem AP/hotspot, CAC/radar local, WPA3 ou AX. O canal automático
da rede do usuário ainda não foi identificado e esta entrega não prova que DFS
era a causa da ausência daquela rede. A nova kext deve ser carregada para validar
associação, DHCP, tráfego e CSA no hardware. RTL8814BU continua sem estação.
