# Seal Table 4 same-identity remeasurement (2026-10-09)

Paper-A §8.1 item 1 heartbeat: **read-only receipt integrity** for historical Table 4. Does **not** run new benchmarks, does **not** change Table 4/5 cell values or key-count identities (8509 / 8769 / 9174) in manuscript files.

## Question

Can Paper A Table 4’s four rows be pinned to archived bench receipts under the identities the paper claims (`c4993fd0…` rows 1–2; `c94cf5fe…` rows 3–4), and what protocol closes same-identity remeasurement once 政委 picks a submission artifact?

## Method

1. Read CN paper Table 4 labels and rounding in [`unisacc-paper.md`](unisacc-paper.md) §7.2 and Appendix A data-source lines.
2. Match rows 1–2 against [`archive/research/r9/r9-current-bench-20260928.json`](../archive/research/r9/r9-current-bench-20260928.json) (`artifact_sha256`, `median_seconds`, `host`).
3. Repo-wide search for `c94cf5fe` and for paper-printed classical cells `0.0797`, `0.790`, `10.15` outside drafts.
4. Compare nearest in-repo fib/self receipt [`archive/research/20260928/candidate-bench-20260928.json`](../archive/research/20260928/candidate-bench-20260928.json) without treating it as Table 4 rows 3–4 provenance.

Machine-readable twin: [`seal-table4-same-identity-20261009.json`](seal-table4-same-identity-20261009.json).

## Paper Table 4 (historical, as printed)

| Row | Workload (CN) | Classic | Model | Ratio | Claimed product identity |
| --- | --- | ---: | ---: | ---: | --- |
| 1 | `calc.c`：编译并在内存中运行 | 0.0108 s | 0.179 s | 16.6× | `c4993fd0…` |
| 2 | 最小程序：启动 | 0.0032 s | 0.029 s | 9.2× | `c4993fd0…` |
| 3 | `fib.c`：编译到镜像 | 0.0797 s | 0.312 s | 3.9× | `c94cf5fe…` |
| 4 | 编译器自身源码：编译到镜像 | 0.790 s | 10.15 s | 12.8× | `c94cf5fe…` |

Appendix A cites a JSON path for rows 1–2 only; rows 3–4 name `c94cf5fe…` with **no** in-repo bench JSON path.

## Verification — rows 1–2 (`c4993fd0…`)

Receipt: `archive/research/r9/r9-current-bench-20260928.json`

| Field | Receipt value | Paper (rounded) | Match |
| --- | --- | --- | --- |
| `artifact_sha256` | `c4993fd0c7a812a7b986e5d6ede0ba6aa162b1688d53fb847df187608d468ff8` | `c4993fd0…` | yes |
| `host` | `macOS-26.5.1-arm64-arm-64bit-Mach-O` | same-machine medians (§7.2) | consistent |
| `scope` | warm fresh-process `-run`, five alternating samples, median | §7.2 wording | consistent |
| calc `median_seconds.classic` | `0.01084070885553956` | 0.0108 s | yes |
| calc `median_seconds.model` | `0.17940804222598672` | 0.179 s | yes |
| startup `median_seconds.classic` | `0.003171415999531746` | 0.0032 s | yes |
| startup `median_seconds.model` | `0.029107333160936832` | 0.029 s | yes |
| calc ratio (medians) | ≈16.56× | 16.6× | yes (paper rounding) |
| startup ratio (medians) | ≈9.18× | 9.2× | yes (paper rounding) |

**Judgment:** rows 1–2 are receipt-OK under identity `c4993fd0…` on the archived macOS arm64 host.

## Gap — rows 3–4 (`c94cf5fe…`)

| Check | Result |
| --- | --- |
| `c94cf5fe` in any `archive/**` bench JSON | **no** (only narrative in [`archive/s17-migration-log-20260928.md`](../archive/s17-migration-log-20260928.md)) |
| `0.0797`, `0.790`, `10.15` outside paper drafts | **no** (CN/EN/tex/notes only) |
| Appendix A JSON for rows 3–4 | **missing** — identity string only |

**Nearest archived fib/self file (not claimed as `c94cf5fe…`):** `candidate-bench-20260928.json`

| Case | Compiler `sha256` (prefix) | vs paper row 3–4 identity |
| --- | --- | --- |
| `bench-fib` | `a4de871a…` | **not** `c94cf5fe…` |
| `bench-self` | `a4de871a…` | **not** `c94cf5fe…` |

Printed classical column vs that candidate receipt (modelbench-style reference compile, single `elapsed_seconds` on reference; model side five-sample median):

| Row | Paper classic | Candidate receipt classic ref | Paper model | Candidate model median |
| --- | ---: | ---: | ---: | ---: |
| 3 fib | 0.0797 s | 0.0215 s (`bench-fib.reference.elapsed_seconds`) | 0.312 s | 0.306 s (sample 1 = 0.3126 s) |
| 4 self | 0.790 s | 0.195 s (`bench-self.reference.elapsed_seconds`) | 10.15 s | 9.87 s |

**Judgment:** rows 3–4 **cannot** be pinned to an in-repo bench receipt at identity `c94cf5fe…`. The migration log records the printed fib/self classics and model medians narratively; there is no sealed JSON to audit. `candidate-bench` is a different compiler artifact and does **not** reproduce the printed classical column for fib/self.

## Same-identity remeasure protocol (no execution until submission identity chosen)

**Precondition:** 政委 freezes one submission `unisacc.com` SHA-256 (full 64 hex). All four Table 4 rows use **that same** artifact. Do not mix `c4993fd0…` / `c94cf5fe…` / tip candidates across rows. Do not run until that identity is chosen.

**Host:** state `platform.platform()` (or equivalent) in the receipt. Historical Table 4 rows 1–2 were **macOS arm64**. Cloud Linux remeasure alone is **not** a drop-in replacement for the historical Mac table without explicit host-change labeling in the paper.

**Classic reference:** one host-built private native seed binary; record `reference_sha256`. Same reference for all four rows on that host (r9 used `/tmp/unisacc-r9-parent-ref`; remeasure may use an equivalent path but must pin SHA-256).

**Workloads (labels must match Table 4):**

| Row | Workload | Measurement style |
| --- | --- | --- |
| 1 | `examples/apps/calc.c`, compile + run in memory | warm fresh-process `unisacc.com -run` vs reference `-run`; five alternating samples; **median** wall time; assert stdout/stderr/rc equality each sample (see r9 `scope`) |
| 2 | minimal empty `main` program, startup | same as row 1 |
| 3 | `examples/fib.c`, compile to osx/arm64 image | modelbench-style: `-O2`, `-b osx/arm64`, `-o` temp image; five model samples median; reference compile timed once (or same five-sample policy if protocol is tightened — document choice in receipt) |
| 4 | `unisacc.c` self compile to image | same as row 3 |

**Outputs:**

- Write `research/seal-table4-remeasure-<artifact_sha256_prefix16>.json` (or full hash if preferred; prefix must match frozen identity).
- Include: `schema`, `submission_identity_sha256`, `reference_sha256`, `host`, `source_commit` (if built from tree), per-case commands, samples, medians, output hashes, `equal_output` flags, measurement script hash if embedded.
- Optional: archive copy under `archive/research/` after review; heartbeat path in `research/` is enough for seal.

**Paper update rule (after remeasure only):** replace Table 4 with four rows from **one** receipt, or keep historical table and cite remeasure separately — 政委 + author choice. This note does not edit Table 4 cells.

## Blockers

1. **Submission identity** not frozen (same blocker as [`seal-remeasure-20261009.md`](seal-remeasure-20261009.md)).
2. **Rows 3–4 historical provenance** — no in-repo JSON for `c94cf5fe…`; cannot retroactively seal without finding off-repo artifacts or remeasuring.
3. **Host** — remeasure should declare macOS arm64 parity if claiming continuity with historical rows 1–2; cross-host numbers need explicit labeling.

## Self-check

- `unisacc-paper.md` / `.en.md` / `arxiv-paper-a` Table 4 and Table 5 cells: **not edited**.
- Key counts 8509 / 8769 / 9174 in papers: **not edited**.
- `kernel/`, `weights/`, product sources, `facts/`: **not edited**.
- This heartbeat: research markdown + JSON only.
