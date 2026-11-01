# DFS em todas as famílias de duas bandas — Implementation Plan

> **For agentic workers:** Use superpowers:subagent-driven-development. Checkboxes record progress; root integrates shared files and delivery.

**Goal:** Implementar estação DFS HT20 nos sete backends de duas bandas, com app e source completos.
**Architecture:** PHY específico por família, estado comum de descoberta/CSA e capabilities v1 explícitas por grupo.
**Tech Stack:** C/C++ IOKit, net80211, fontes Linux/vendor fixadas, CMake/Python, AppKit.
**Spec:** docs/superpowers/specs/2026-10-31-dfs-station-design.md

## Global Constraints

- Trabalhar no source autorizado; nenhuma mudança no sistema/EFI/rede carregados.
- Preservar factory ROM/RFE/potência, WPA2/CCMP e fixes53/54; não adivinhar144.
- Não exigir CAC/radar deAP para estação; não remover restrições de busca passiva.
- Root: ABI/app/registration8814/CMake/versões/docs/entrega. Agentes: PHY8814;
  demaisPHYs/backends; estado comum/CSA no Controller. Blocos compartilhados distintos.

## Review Focus

- Probe/management/data não podem transmitir antes da permissão de estação DFS.
- Beacon/action estrangeiro ou inválido não pode conceder TX nem programar RF.
- CSA precisa silenciar e recuperar sem deauth no canal antigo nem reentrada RX.
- Potência/calibração pertencem à revisão/RFE/canal correto, incluindo saída140.
- App não pode autorizar DFS em driver antigo ou ocultar limites de largura/canal.

### Task1: PHY RTL8814AU

- [x] RED54 em novos canais e spur140/thermal100–140.
- [x] Portar índices/limites e funções primárias com backup/restore e gates existentes.
- [x] GREEN em24centros, ambosRFE e transições; comparar primários e compilarSDK.

### Task2: Demais seis backends de duas bandas

- [x] RED54 para DFS20 emDU/AU/B-C e manter casos inválidos/ROM/RFE fechados.
- [x] Corrigir grupos OTP/geometria/registro e fallback20 para APs largos.
- [x] GREEN em todos os canais/grupos/revisões implementados e compilaçãoSDK.

### Task3: Estação DFS comum e CSA

- [x] RED para passive-scan, beacon-freshness/identity e CSA.
- [x] Implementar parser/cache limitado, autorização BSS e stop/rescan/rejoin adiado.
- [x] GREEN para invalidez, TXbloqueado, sleep/teardown/múltiplosUSB e SDK.

### Task4: App, capabilities e entrega

- [x] RED54 em flags52–64/100–140 e agrupamento das bandas no app.
- [x] Implementar helper de canais, flags v1, descrição/validação no app47 e registro8814.
- [x] Integrar testes, rever primários independentemente e executar suítes completas.
- [x] Preparar build55/app47 e scripts de entrega; prova final do ZIP, assinaturas e hashes registrada em VERIFICATION.json externo ao source.
