# USB RX recovery retry implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox syntax for tracking.

**Goal:** Avoid permanently disabling the adapter on the first transient failure of its endpoint recovery, and identify the exact failed stage.

**Architecture:** Preserve the serialized workloop, transfer ledger and clearStall(true). Add timed, bounded recovery attempts and route eligible synchronous RX rejection through completion ownership.

**Tech Stack:** C++ IOKit, actual-method host tests, ASan/UBSan, native Xcode kext and Objective-C++ app builds.

**Spec:** ../specs/2026-10-30-rx-clear-retry-design.md

## Constraints and review focus

- Work directly in the existing source; do not install, alter EFI, or reboot.
- Preserve app and driver source, license notices and existing hardware support gates.
- No host-only clear, TX replay, vendor register replay, unlimited retries or sleeping under the workloop gate.
- Maximum three transient recovery attempts and a two-second whole recovery deadline.
- Pending old and partially refilled reads must drain before a new epoch.
- Halted-pipe synchronous RX rejection retains exactly one completion ownership.
- Stop, sleep and removal during timed retry cancel all work.
- Other transport errors and partial-byte failures remain fatal.
- Scan/calibration work must not touch recovering hardware unexpectedly.

## Task 1: Recovery correctness

- [x] Extend tests/rtwn_rx_recovery_host/platform.hpp with halted-pipe semantics and scripted clear/refill outcomes.
- [x] Add failing actual-controller tests in tests/rtwn_rx_recovery_tests.cpp for transient clear failure, partial refill rejection, synchronous RX rejection, bounded exhaustion/deadline and cancellation.
- [x] Capture the failing run with scripts/test-rtwn-rx-recovery.py.
- [x] Correct kext/RTWUSBController.cpp/.hpp; retain ledger ownership, defer retry by timer and publish scalar stage/error counters.
- [x] Verify host recovery and related RX ordering/WPA suites; repair fixture declarations only where needed.
- [x] Obtain independent source review against Apple SDK and Linux references.

## Task 2: Deliver corresponding source

- [x] Bump kext to 0.14.3/build50, preserve compatible app0.3.3/build45, regenerate Xcode project and update README/CHANGELOG/Clover/validation.
- [x] Run full Release and sanitizer suites and app native tests.
- [ ] Snapshot corresponding source with manifest and extract ZIP into clean verification directory.
- [ ] Compile kext and app from extracted delivered source, verify metadata/signatures and package artifacts with checksums.
- [ ] Report concrete changes and that hardware validation on build50 remains pending.

## Revisão adicional

- [x] Guardar RTW_CONTROL_SCAN antes de ieee80211_begin_scan enquanto RX está pendente, com teste real do handler.
- [x] Adiar coex88TimerAction durante recuperação, testando pausa/retomada nas três famílias B/C e nos dois perfis de antena.

## Evidência e decisões

A revisão independente encontrou dois acessos auxiliares a EP0 durante a espera: busca direta e timer de coexistência B/C. Ambos receberam teste RED antes da correção e teste GREEN depois. O setter de taxa do AU apenas atualiza memória; TX permanece permitido, com seu tratamento fatal de erro original. O prazo de dois segundos é verificado antes/depois de chamadas síncronas da Apple; não há preempção dessas chamadas. Validação física continua pendente.
