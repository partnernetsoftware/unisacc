# Seal Table 5 same-identity remeasurement (2026-10-09)

Paper-A §8.1 item 2 heartbeat: **read-only receipt integrity** for historical Table 5 (DENSE decision-network instance vs tcc/cc on osx/arm64). Does **not** run new benchmarks, does **not** change Table 4/5 cell values or key-count identities (8509 / 8769 / 9174) in manuscript files.

## Question

Can Paper A Table 5’s five historical rows be pinned to in-repo bench receipts with recorded tcc/cc versions and a product artifact identity, and what same-identity remeasure protocol closes §8.1 item 2 once 政委 picks a submission `unisacc.com` SHA-256?

## Method

1. Read CN Table 5 in [`unisacc-paper.md`](unisacc-paper.md) §7.2, §8.1 item 2, and Appendix A reproduction lines (and EN / `arxiv-paper-a/main.tex` for wording parity only).
2. Read [`tests/bench_vs.sh`](../tests/bench_vs.sh) as the cited driver (min-of-three wall time, flat export, byte-equal self-compile outputs).
3. Repo-wide search for printed cells (`1.81`, `569,384`, `490,840`, `683,008`, `677,154`, `1,056,930`, compile-column seconds, `5.4×`, `13.8×`) under `archive/research/` JSON and scripts.
4. For each row, judge whether a receipt pins: host, Apple `cc` identity, `tcc` path/version, min-of-three protocol, output equality, and any `unisacc.com` SHA-256.

Machine-readable twin: [`seal-table5-same-identity-20261009.json`](seal-table5-same-identity-20261009.json).

## Paper Table 5 (historical, as printed)

Workload (all rows): flat-exported compiler source; each row is **who built the unisacc binary** that then runs `unisacc -O2` on the same source to osx/arm64. **DENSE** classic path (decision-network instance), **not** Table 4’s seven-stage network-compiler pipeline. Measurement: **minimum of three** wall-clock samples (§7.2). Relative column is vs cc -O2 compile time (1.0× baseline).

| Row | Built by | Build time | Binary size | Compile unisacc.c (-O2) | vs cc -O2 |
| --- | --- | ---: | ---: | ---: | ---: |
| 1 | cc -O2 | 1.81 s | 569,384 B | 0.15 s | 1.0× |
| 2 | cc -O0 | 0.15 s | 490,840 B | 0.49 s | 3.3× |
| 3 | tcc | 0.02 s | 683,008 B | 0.55 s | 3.7× |
| 4 | unisacc -O2 (self) | 0.15 s | 677,154 B | 0.81 s | 5.4× |
| 5 | unisacc -O0 (self) | 0.10 s | 1,056,930 B | 2.05 s | 13.8× |

Appendix A cites `TCC=<tcc build dir> ./tests/bench_vs.sh` for Table 5. It does **not** name a bench JSON path (contrast Table 4 rows 1–2 → `archive/research/r9/r9-current-bench-20260928.json`).

## Verification — protocol script only

| Check | Result |
| --- | --- |
| Cited driver in tree | [`tests/bench_vs.sh`](../tests/bench_vs.sh) |
| Min-of-three | `best()` runs three `/usr/bin/time -p` samples, keeps minimum |
| Workload | `tests/sourceflat.py` → `SELF`; build `sr.c` = refshim + flat + reffoot; host target via `host_target()` |
| Self-compile job | Each built compiler: `-O2 "$SELF" -b $HT -o out`; reference `want` from `ua_self2` |
| Output equality | `cmp` against `want`; script requires `wrong=0` and `n>=3` |
| `bench_vs.sh` SHA-256 (tip audit) | `427800c7636333aee296e98ce55cefea32d8bb40e5048105a47b7cd5efafeae0` |
| `sourceflat.py` SHA-256 (tip audit) | `f74ca5894bc6c5d1442b954076e5ffe7c6844516a43fd7b41b305912e025c315` |
| Archived JSON with Table 5 timings/sizes | **none** |
| Printed integers in non-paper files | **none** (except partial narrative below) |
| Paper-claimed `unisacc.com` SHA-256 for Table 5 | **none** (Table 4 names mixed prefixes; Table 5 does not) |
| Recorded Apple `cc` version for historical run | **none** in repo |
| Recorded `tcc` version/path for historical run | **none** in repo |

**Judgment:** the **measurement procedure** is recoverable from `bench_vs.sh` and matches §7.2 wording. The **printed table cells are not pinned** to any `archive/research/**/*.json` receipt on tip.

## Per-row pin status

| Row | Receipt pin | Notes |
| --- | --- | --- |
| 1 cc -O2 | **unpinned** | No JSON/log with 1.81 s / 569,384 B / 0.15 s |
| 2 cc -O0 | **unpinned** | No JSON/log with 490,840 B / 0.49 s |
| 3 tcc | **unpinned** | No JSON/log with 683,008 B / 0.55 s; §8.1 item 2 explicitly needs tcc version |
| 4 unisacc -O2 (self) | **unpinned** | No timing receipt; **677,154 B** osx/arm64 -O2 image size appears in [`archive/prd-history-20260929.md`](../archive/prd-history-20260929.md) (peephole narrative, 2026-09-25) — **not** a bench row receipt |
| 5 unisacc -O0 (self) | **unpinned** | No JSON/log with 1,056,930 B / 2.05 s |

Narrative-only speed fragments (e.g. self-compile **0.81 s** after branch-prediction change in the same prd-history file) are **not** Table 5 row receipts and omit build/binary columns.

## Same-identity remeasure protocol (no execution until submission identity chosen)

**Scope:** Table 5 = **DENSE decision-network instance** via cc-built flat-export compilers. **Not** Table 4 network-compiler pipeline timings ([`seal-table4-same-identity-20261009.md`](seal-table4-same-identity-20261009.md)).

**Precondition:** 政委 freezes one submission `unisacc.com` SHA-256 (full 64 hex). Record matching `source_commit` / release receipt from Appendix A family. **Do not run** until that identity is chosen. Do not mix v0.0.19 / v0.0.36 / tip candidates across a published remeasure table.

**Host:** macOS **osx/arm64** metal (or explicitly label if host changes). Historical Table 5 is osx/arm64-only. Cloud Linux remeasure is **not** a drop-in replacement without relabeling the paper table.

**Compilers to record (closes §8.1 item 2 version gap):**

- Apple `cc`: capture `cc --version` (or `clang --version`) and full path used for `ua_cc2` / `ua_cc0`.
- `tcc`: capture `TCC` directory, `"$TCC/tcc" -v` (or equivalent), and build label if vendored.

**Build and measure (align with current script unless deliberately upgraded):**

1. Checkout tree at `source_commit` tied to frozen `submission_identity_sha256` (from release acceptance JSON).
2. `python3 tests/sourceflat.py` → pin flat byte size and SHA-256 of `SELF`.
3. `TCC=<dir> ./tests/bench_vs.sh` on osx/arm64 host (or a wrapper that logs the same steps with receipts).
4. **Min-of-three** wall times for build and compile columns (current `bench_vs.sh`). If 政委 aligns Table 4 and Table 5 statistics, document an explicit switch to **median-of-five** in the receipt; do not silently mix policies in one table.
5. For **unisacc rows only**: receipt must include `submission_identity_sha256` and SHA-256 of built `ua_self2` / `ua_self0` binaries (DENSE path products of that tree). Optional cross-check: same `-O2` output bytes as frozen `.com` on the self-compile workload if product route is claimed equivalent on that host.
6. Assert **byte-equal** outputs across all compared compilers (`bench_vs` `cmp` policy).

**Outputs:**

- Write `research/seal-table5-remeasure-<submission_sha256_prefix16>.json` with: `schema`, `submission_identity_sha256`, `source_commit`, `host`, `cc_version`, `tcc_version`, `flat_source_sha256`, `measurement_script_sha256`, `policy` (`min_of_3` or documented alternative), per-row build seconds, binary bytes, compile seconds, ratios, `output_equal` flags, three raw samples per timed step if retained.
- Optional archive copy under `archive/research/` after review.

**Paper update rule (after remeasure only):** replace Table 5 from one receipt or keep historical table and cite remeasure separately — 政委 + author choice. This note does **not** edit Table 5 cells. **§8.1 item 2 is not closed** by inventory alone.

## Blockers

1. **Submission identity** not frozen ([`seal-remeasure-20261009.md`](seal-remeasure-20261009.md), [`seal-same-identity-matrix-scaffold-20261009.md`](seal-same-identity-matrix-scaffold-20261009.md)).
2. **Historical Table 5 receipt missing** — no in-repo JSON; cannot retroactively seal tcc/cc versions without off-repo logs or remeasure.
3. **Host** — historical numbers are Mac osx/arm64; remeasure must declare host parity or relabel.
4. **Table 5 vs Table 4** — remeasure must not conflate DENSE instance (this table) with pipeline rows (Table 4).

## Self-check

- `unisacc-paper.md` / `.en.md` / `arxiv-paper-a` Table 4 and Table 5 cells: **not edited**.
- Key counts 8509 / 8769 / 9174 in papers: **not edited**.
- `kernel/`, `weights/`, product sources, `facts/`: **not edited**.
- No new benchmarks run; `bench_vs.sh` not executed this heartbeat.
- This heartbeat: `research/seal-table5-same-identity-20261009.{md,json}` only.
