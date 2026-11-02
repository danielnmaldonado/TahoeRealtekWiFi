# Validation — driver0.14.13/build60, app0.3.9/build51

Project/macOS adaptation: MaLd0n. Development.

See ADAPTER-TESTS-20261101.md for the individual audit, CSVs of all
counters and JSON of mode/rates/histograms. Input data were captured
on another system with kext55; they do not prove this delivery's physical behavior.
VERIFICATION.json records exact test results and delivery hashes.

Build60 addresses the 8723BU mapping of four physical OUT endpoints to three queues, resident
MCU reset 8812/8821, IE cache memory bounds/ownership and USB/AU start
observability. IOBuiltin=Yes and previous fixes are preserved.
App51 incorporates the updated USB catalog; preference and reconnection retain the
previous implementation. The collector includes new fields through allowlists.

Memory tests use generic local buffers and allocation failure; they do not
construct radio frames or reproduce panics on hardware. USB tests
model endpoints and transport failures; they do not prove the uncaptured
physical Edimax descriptor. WarmAU recovery is demonstrated in code/model,
without proof that it caused the 2357:0120 failure. The omission of 2.4GHz in that log
requires new power/ROM evidence; factory guards remain intact.

Support matrices and HT/VHT/ROM limits remain in the README. Native RX
accounting and sustained speed measurements remain unresolved/unmeasured.
There was no new installation, kext loading, EFI, router,
macOS security, credential change or reboot during this file-analysis round.
