# Multi-adapter Refresh and Recovery Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans; current user explicitly requests direct execution.

**Goal:** Atualizar as redes do USB selecionado e expandir correções verificadas sem misturar famílias ou controladores.
**Architecture:** Estado de refresh por ID/epoch no app, transporte RX comum e recuperação de bootstrap com operações próprias de cada família.
**Tech Stack:** Objective-C++/AppKit/XPC, C/C++/IOKit/net80211, CMake, clang, testes ASan/UBSan.
**Spec:** docs/superpowers/specs/2026-11-01-multi-adapter-design.md

## Global Constraints

- Source na pasta Documents existente; releases anteriores imutáveis; logs autorizados no Desktop.
- Sem credenciais, nova associação, reboot ou EFI pelo agente nesta rodada.
- Protocolo público e autenticação app/helper preservados; nenhuma habilitação de chipset sem HAL.
- Testes de diagnóstico com home isolado; nenhum reset/clean do repositório sujo.

## Review Focus

- Troca A→B→A durante SCAN: callback antigo não altera B nem dá origem fresca ao cache.
- Adaptador conectado: refresh automático não rompe associação/tráfego.
- USB rejeita o RX inicial: sem anúncio falso de sucesso nem retry infinito.
- Callback tardio/erro misto: nenhuma liberação/reinicialização antes de drenar nem perda do primeiro erro.
- Identidade/ROM alterada, stop falho ou USB fatal: recuperação bloqueada, sem limpar o erro.

## Task 1: Verificação física somente leitura

- [x] Descobrir IDs atuais e capturar LIST/STATUS/MAP de ambos com app instalado.
- [x] Conferir build carregada, USB, WPA2/IP/rota e deltas sem misturar adaptadores.
- [x] Salvar relatório local com limites; não converter metadados em prova de rádio/tráfego.

## Task 2: Refresh do app

Files: app/Main.mm, políticas pequenas de estado se necessárias, app/tests/*.
- [x] Provar que troca para USB ocioso atualmente só lê cache.
- [x] Testar SCAN automático por necessidade e conclusão, sem associação automática nova.
- [x] Implementar estado por ID, tempo de conclusão, invalidação e retry/timeout limitados.
- [x] Testar troca rápida, hotplug, wake, cache vazio, busca em andamento e link ativo.

## Task 3: RX inicial comum

Files: kext/RTWUSBController.cpp/.hpp, tests/rtwn_rx_capacity_tests.cpp.
- [x] Executar RED para rejeição em cada família de estação.
- [x] Ampliar os guards existentes ao registro de backends de estação.
- [x] Testar capacidade própria, quatro rejeições, abort diferido/erro fatal, orçamento e cancelamento.
- [x] Executar capacidade, runtime RX e ordenação com sanitizadores.

## Task 4: Bootstrap/stop das famílias

Files: RTWFamilyRecovery.inc/Members, backends tipados, APIs stop_checked e testes reais.
- [x] Reproduzir latch e stop antes do drain; reter provas RED.
- [x] Implementar stop_checked preservando o erro original e verificando limpeza separadamente.
- [x] Guardar identidade/ROM/provider e retry explícito limitado em SCAN/JOIN.
- [x] Finalizar somente teardown após drain; impedir bypass por watchdog/initTask.
- [x] No legado, exigir poweroff primário antes de retry persistente; testar separadamente.

## Task 5: Integração e entrega

- [x] Revisão independente dos deltas; corrigir achados e repetir checks afetados.
- [x] Bump driver57/app49, documentação e tabela de cobertura factual.
- [ ] CTest Release/sanitizadores, testes AppKit, SDK e compilação de app/kext do ZIP final.
- [ ] Conferir assinaturas, payload do instalador, source manifest e hashes do pack.
- [ ] Instalar o app autorizado; manter kext/EFI para compilação/carregamento pelo usuário.

A fotografia deste plano precede a geração do ZIP. Os resultados finais da
compilação/testes/instalador são registrados no VERIFICATION.json da entrega.
