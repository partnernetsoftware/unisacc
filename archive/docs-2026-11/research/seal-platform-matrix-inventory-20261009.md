# Seal platform-matrix inventory 2026-10-09 (receipt-only)

Paper-A heartbeat evidence for **§8.1 item 3** (platform execution matrix). **No new benchmarks.** Numbers in Table 4 / Table 5 and Table 1 key counts (8509 / 8769 / 9174) were **not** edited.

## Question

For each of the six product targets (`osx/arm64`, `osx/x86_64`, `lnx/arm64`, `lnx/x86_64`, `win/arm64`, `win/x86_64`), what do existing **release receipts** already prove about **how** binaries are built and executed—and what do they still fail to prove?

## Method

- Read-only pass over cited release acceptance JSON and the paper’s §8.1 / §6.2 statements.
- Classify each target into one primary **evidence class** (when receipts support it): `native-run`, `cross-compile-only`, `hosted-runner demo`, `VM self-rebuild`, or `missing`.
- Prefer the **latest published** receipt (`research/r36-release-acceptance.json`, v0.0.36) for recurring patterns; use **paper-cited** `archive/research/r19-release-acceptance.json` (v0.0.19) and `archive/research/r9/r9-release-acceptance.json` (v0.0.9 `platform_smoke.profiles`) where they add per-target mechanism detail receipts do not repeat.
- Do **not** treat “six runner demo matrix green” as a per-target full native execution matrix (paper §6.2 line on r19; §8.1 item 3).

## Inventory table

| Target | Evidence class | Source receipt + version | What is proved | What is NOT proved |
| --- | --- | --- | --- | --- |
| `osx/arm64` | `hosted-runner demo` (+ local `native-run` on arm64 dev host) | `research/r36-release-acceptance.json` v0.0.36 (`release_check` success, `post_release_smoke` success); `archive/research/r19-release-acceptance.json` v0.0.19 (`release_check`: “six candidate runners”); `archive/research/r9/r9-release-acceptance.json` v0.0.9 (`platform_smoke.profiles.osx/arm64`: native macOS arm64 model driver and generated arm64 images) | Sealed public candidate is exercised on a **hosted** macOS arm64 runner as part of release-check / post-release smoke; v0.0.9 smoke probes passed on **native** macOS arm64 for model vs generated-native equality (scope: hello/fib/convert-style smoke, not full suites—see r9 `platform_smoke.scope`). | One **submission-grade** row for this target under a **single frozen identity**, separating host metal vs VM vs runner and listing what ran natively vs cross-only (gap named in `research/unisacc-paper.md` §6.2 / §8.1 item 3). |
| `osx/x86_64` | `native-run` (Rosetta on arm64 host) + `hosted-runner demo` | r9 `platform_smoke.profiles.osx/x86_64`: “macOS x86_64 through **Rosetta** on arm64 host”; `research/r24-release-acceptance.json` v0.0.24 (`crossnative.result` includes `osx/x86_64` 9/9); r36 `release_check` / `post_release_smoke` (six-runner pattern, no per-target split in receipt) | Generated x86_64 macOS images run under **Rosetta** on an arm64 host in smoke; crossnative examples pass for `osx/x86_64`; hosted six-runner release-check succeeds for the sealed candidate. | Native execution on **Intel macOS hardware** with full local gate suites as the receipted primary path; explicit §8.1 matrix row that labels Rosetta/emulation vs bare metal. |
| `lnx/arm64` | `native-run` (Lima guest, native arm64) + `hosted-runner demo` | r19 `linux_must_run`: “lnx/arm64 Lima: compiler reds 0”; `research/r35-release-acceptance.json` v0.0.35 (`linux.vm`: “lima default (native arm64)”, full suite guest-known only); r9 profile: “Lima Linux arm64 guest”; r36 `release_check` success | Full **local** Linux gate suite on Lima **native arm64** guest (with documented guest-known warn reds); hosted arm64 Linux runner in six-runner release-check. | Same-identity table cell that maps “dev host / Lima VM / GitHub runner” for **one** submission artifact; proof that receipts for v0.0.19 identity cover v0.0.36 without re-listing per §8.1. |
| `lnx/x86_64` | `hosted-runner demo` (+ emulated local smoke; **not** full local x86_64 suites) | `archive/research/r21-release-acceptance.json` v0.0.21 (`linux_must_run`: product on **ubuntu-latest real runner**; “source suites **NOT** run on x86_64”; emulated Lima run “**UNVERIFIED** on x86_64 by name”; `six_candidate_runners`: “lnx/x86_64 on ubuntu-latest real hardware”); r9 profile: “Lima Linux x86_64 guest through **emulation** on arm64 host”; `research/r22-release-acceptance.json` v0.0.22 (same `linux_must_run` pattern) | **Hosted** real x86_64 Linux runner proves the **product** in release-check; crossnative 9/9 on `lnx/x86_64` (e.g. r24); local smoke via **emulated** Lima guest in r9. | Local **full source suite** on x86_64 Linux (explicitly absent / UNVERIFIED in r21–r22); §8.1-style admission that x86_64 Linux evidence is **runner demo + crossnative**, not a complete native matrix on one machine. |
| `win/arm64` | `VM self-rebuild` + `hosted-runner demo` | r19 `windows_must_run`: “crossnative …; win/arm64 … rebuilt themselves **byte for byte**”; `research/r25-release-acceptance.json` v0.0.25 (`release_check.includes`: “winsuite on windows-latest and **windows-11-arm**”); `research/r36-release-acceptance.json` v0.0.36 (`unverified`: “**Windows -run/winposix: CI runner only**”) | Local **UTM** path: crossnative strict pass + **nativeboot** self-rebuild byte-identical (r19–r24 family); **hosted** Windows ARM runner runs winsuite/comdemo in release-check (r25+). | Local obligation for **Windows -run / winposix** treated as **CI-only** in r33–r36 `unverified` (not re-run locally); §8.1 unified matrix for win/arm64 under one submission identity. |
| `win/x86_64` | `VM self-rebuild` + `hosted-runner demo` | Same r19 `windows_must_run` for `win/x86_64`; r25 `release_check` winsuite on **windows-latest**; r36 `unverified` “Windows -run/winposix: **CI runner only**”; r9 profile: Windows x86_64 images under guest **x86_64 emulation** for APE driver path | Same as win/arm64 pattern: VM crossnative + byte-identical **self-rebuild**; hosted x86_64 Windows runner in release-check. | Local -run/winposix proof outside hosted runners; native Windows x86_64 **metal** full gate as receipted primary evidence. |

### Cross-cutting receipts (all six targets, not a per-host matrix)

| Mechanism | Evidence class | Source | Proved | Not proved |
| --- | --- | --- | --- | --- |
| Six-runner sealed candidate + demo programs | `hosted-runner demo` | r19 `release_check`; r34 `ci_demo_matrix` (“6 targets × 19 programs, 114 cells, no gap”); r24/r25 `post_release_smoke` (“six cells success” listing all six targets); r36 `post_release_smoke` success | Deterministic demo programs match across six **hosted** runners for a given sealed candidate. | Which layers are native vs emulated **per cell**; equivalence to full local gate / full native suites per target (§8.1 item 3). |
| ABI machine-model 6/6 folding | `native-run` (model driver, in-process) | `research/unisacc-paper.md` §6.2 (cites r19): six-target ABI machine-model folding + fault injection 6/6→4/6 | Six lowered images agree under ABI machine models tied to r19-era evidence. | Not re-exported as a per-target “host vs VM native run” table in any receipt; not re-bound to v0.0.36 in this inventory pass. |
| v0.0.9 platform smoke (3 probes × 6 targets) | mixed (`native-run` / emulation per profile) | r9 `platform_smoke` (18 cells, profiles + per-target records) | hello/fib/convert smoke: model run matches generated-native output per target under stated profile (incl. Lima emulation, Rosetta, Windows guest emulation). | Full native suites; Windows bootstrap; scope disclaimer in r9 `platform_smoke.scope`. |

## Judgment (§8.1 item 3)

**Cannot be closed from receipts alone.** Receipts prove recurring patterns (six hosted runners, Lima arm64 full suite, Windows VM self-rebuild, lnx/x86_64 product on real ubuntu-latest runner with **local x86_64 suites explicitly not run / UNVERIFIED**, Windows -run **CI-only** in latest `unverified` lines). The paper already states that v0.0.19 does not publish the full “host / Linux VM / Windows VM per-target native run” matrix (`unisacc-paper.md` §6.2; §8.1 item 3).

**Still needed for submission:** one **same-identity** platform matrix table on the **frozen submission artifact** (identity not chosen in this heartbeat—§8.1.1 still names v0.0.19 for paper freeze while v0.0.36 is latest published), with explicit rows for emulation-only paths (lnx/x86_64 local suites, Rosetta, Windows -run CI-only) as the paper admits.

## Self-check

- `research/unisacc-paper.md` / `.en.md` Table 1 cells and Table 4 / Table 5 numbers: **not edited** in this change.
- Product / kernel / weights / facts code: **not edited**.
- No timings or pass counts invented; table cells cite receipt fields or paper lines only.

Machine-readable twin: [`seal-platform-matrix-inventory.json`](seal-platform-matrix-inventory.json).
