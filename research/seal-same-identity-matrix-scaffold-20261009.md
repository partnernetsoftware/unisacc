# Seal same-identity platform-matrix scaffold 2026-10-09

Paper-A heartbeat evidence for **§8.1 item 3**. **Research-only:** no new benchmarks, no product/kernel/weights/facts edits, no invented pass counts or timings.

## Question and method

**Question:** Once 政委 picks a **single frozen submission identity**, what is the **minimal table shape** Paper A must fill so §8.1 item 3 can close—and do current receipts already satisfy it without that run?

**Method:** Read-only synthesis from [`seal-platform-matrix-inventory-20261009.md`](seal-platform-matrix-inventory-20261009.md) (+ JSON), [`seal-remeasure-20261009.md`](seal-remeasure-20261009.md) / [`seal-remeasure-tip.json`](seal-remeasure-tip.json), `research/unisacc-paper.md` §6.2 / §8.1 (and EN / arXiv tex mirrors), and `release/RELEASE-PIPELINE.md` / `.github/workflows/release-check.yml` runner labels. **No receipt inventory rerun** this beat.

## Candidate identities (not chosen here)

| Label | Role in repo | Pinned facts (candidates only) |
| --- | --- | --- |
| Paper freeze **v0.0.19** | CN/EN §8.1: “本文冻结的发布身份为 v0.0.19”；§6.2 cites [r19 receipt](../archive/research/r19-release-acceptance.json) | Source `d95176a`; public `unisacc.com` SHA-256 `ad1d87e9…` (Appendix A) |
| Latest published **v0.0.36** | `research/r36-release-acceptance.json` (`latest: true`) | RC `29728cc6`; public `unisacc.com` SHA-256 `baf296dd…` |
| Tip gold remeasure **873cc031** | [`seal-remeasure-tip.json`](seal-remeasure-tip.json) pins living `weights/gold` for key-space checks only | 18-stage domain pin `b8244bd8…`; **not** a submission product identity |
| Historical **v0.0.9** smoke | `archive/research/r9/r9-release-acceptance.json` `platform_smoke` (per-target profiles) | Used in inventory for mechanism labels only; must not mix into a submission matrix row |

**Submission identity:** still **not** chosen by 政委. This scaffold does not pick among the rows above.

## Minimum fill rule (same identity)

- Publish **one** frozen artifact label on **every** row of the submission matrix: the same release tag **or** the same `unisacc.com` SHA-256 **or** the same sealed `release/candidate.json` digest set used for release-check.
- **Forbidden:** one published table row that cites r9 mechanisms for `lnx/x86_64`, r19 for Windows VM, and r36 `unverified` lines without all cells referring to the **same** candidate bytes.
- Receipt archaeology may inform **column semantics** (inventory); the **filled matrix** must be re-run or re-exported under the chosen identity.

## Scaffold table (six targets)

Columns describe what Paper A §8.1 item 3 asks for; `fill_status` is procedural only (no measured results).

| Target | `evidence_class_from_inventory` | `required_native_cell` | `native_vs_emulation_label_required` | `suite_scope_required` (paper / receipt language only) | `fill_status` |
| --- | --- | --- | --- | --- | --- |
| `osx/arm64` | `hosted-runner demo` (+ dev-host native in r9 smoke scope) | Host metal (arm64 Mac dev host) **and** hosted runner `macos-15` per release-check matrix | Label whether execution is bare metal vs in-process ABI model (§6.2 folding) vs hosted demo only | §8.1 item 3: per-target host / VM / runner matrix; release-check: sealed candidate **comdemo** smoke on six runners (`release-check.yml` candidate job), not full `./tests/gate.sh --com` per receipt | `blocked-on-identity` |
| `osx/x86_64` | `native-run` (Rosetta on arm64 host) + `hosted-runner demo` | Host: Rosetta on arm64 **or** hosted `macos-15-intel` | Must state Rosetta / emulation vs Intel-native metal (inventory: r9 profile text) | Same as above: §8.1 unified matrix; six-runner comdemo for sealed candidate | `blocked-on-identity` |
| `lnx/arm64` | `native-run` (Lima guest) + `hosted-runner demo` | Lima VM (`default` / native arm64 guest) **and** hosted `ubuntu-24.04-arm` | Label Lima guest native arm64 vs hosted runner; note guest-known reds per r19 `linux_must_run` wording | §8.1 item 3; r19 family: “Linux 本机必跑项” / local gate on Lima; release-check comdemo on arm64 Linux runner | `blocked-on-identity` |
| `lnx/x86_64` | `hosted-runner demo` (+ emulated Lima smoke only in receipts) | Hosted `ubuntu-latest` for product; **not** full local x86_64 source suites (r21–r22 `linux_must_run` pattern in inventory) | Must mark cross-compile-only / emulator-only local paths (§8.1 example: lnx/x86_64 under emulator) | §8.1 item 3 explicitly; inventory: product on real runner vs “source suites NOT run on x86_64” | `blocked-on-identity` |
| `win/arm64` | `VM self-rebuild` + `hosted-runner demo` | UTM VM (local must-run / nativeboot path per r19 `windows_must_run`) **and** hosted `windows-11-arm` | Label VM guest emulation vs hosted runner; r36 `unverified`: “Windows -run/winposix: CI runner only” | §8.1 matrix; r19 byte-identical self-rebuild wording; release-check winsuite/comdemo on ARM Windows runner (r25+ `release_check.includes` pattern in inventory) | `blocked-on-identity` |
| `win/x86_64` | `VM self-rebuild` + `hosted-runner demo` | UTM VM **and** hosted `windows-latest` | Same CI-only -run/winposix caveat as win/arm64 in latest receipts | Same as win/arm64 | `blocked-on-identity` |

After identity pick: each row moves to `fillable-after-identity-run` when the sealed candidate for **that** identity has been exercised and the native/emulation/suite columns are filled from **new** logs (still no mixing identities).

## rowcov / `UNISACC_EDGE_LOG` (orthogonal)

`prd.md` tip (rowcov / `UNISACC_EDGE_LOG`) targets **per-edge native observation** inside a fixed model run. That is **not** the §8.1 **platform execution matrix** (host vs Lima vs UTM vs GitHub runner for six product targets). Filling rowcov does **not** close §8.1 item 3.

## Does current paper prose imply receipts already close §8.1 item 3?

**No change recommended this beat.** CN/EN §6.2 already states the v0.0.19 receipt “未给出 / does not publish” the full host / Linux VM / Windows VM per-target matrix and points to §8.1 item 3. §8.1 lists item 3 under “投稿前尚未满足的条件” with “登记不等于完成”. Inventory judgment: [`seal-platform-matrix-inventory-20261009.md`](seal-platform-matrix-inventory-20261009.md) — **cannot close from receipts alone**.

Optional reader confusion: §5.6 / Appendix rows praise “六个 runner” success alongside §8.1; the explicit §6.2 negation is the controlling sentence. No CN/EN/tex edit unless a later beat wants a one-line cross-pointer in §8.1 item 3 to §6.2 (not done here).

## Judgment

| Enables next beat (after 政委 identity) | Still blocked on 政委 |
| --- | --- |
| Run release-check + local must-run paths **once** on the chosen tag/SHA; export one six-row table with emulation labels and suite scope columns above | Which of v0.0.19 vs v0.0.36 (or other) is the submission artifact |
| Align Table 4 same-identity work with the **same** tag (separate scaffold: `seal-table4-same-identity-20261009.md`) | Rewriting Table 1 key identity (8509 vs 8769); tip `873cc031` gold is op-87 / 8769 per remeasure pins only |

**Do not redo next beat:** receipt-only platform inventory (`seal-platform-matrix-inventory-*`); gold domain remeasure on tip unless gold files change.

Machine-readable twin: [`seal-same-identity-matrix-scaffold.json`](seal-same-identity-matrix-scaffold.json).
