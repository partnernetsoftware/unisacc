# Seal Table 5 same-identity remeasurement (2026-10-09)

Paper-A §8.1 item 2 heartbeat: **read-only receipt integrity** for historical Table 5. Does **not** run `tests/bench_vs.sh`, does **not** change Table 4/5 cell values or key-count identities (8509 / 8769 / 9174) in manuscript files.

## Question

Can Paper A Table 5’s five rows (DENSE decision-network instance vs cc/tcc on osx/arm64, minimum of three) be pinned to in-repo bench receipts that also record **tcc and cc versions**, and what protocol closes same-identity remeasurement once 政委 picks a submission artifact?

## Method

1. Read CN/EN/arXiv Table 5 labels and §8.1 item 2 in [`unisacc-paper.md`](unisacc-paper.md) §7.2 / Appendix A, [`unisacc-paper.en.md`](unisacc-paper.en.md), [`arxiv-paper-a/main.tex`](arxiv-paper-a/main.tex).
2. Read the reproduction script [`tests/bench_vs.sh`](../tests/bench_vs.sh) (best-of-3 wall clock; optional `$TCC`; byte-compare of `-O2` self-compile outputs).
3. Repo-wide search for every printed Table 5 cell (`1.81`, `569,384`, `0.15`, `490,840`, `0.02`, `683,008`, `0.55`, `677,154`, `0.81`, `1,056,930`, `2.05`, `5.4×`, `13.8×`) outside drafts.
4. Check Appendix A “数据来源 / Data sources” for any Table 5 JSON path (Table 4 has explicit pins; Table 5 only lists the reproduction command).

Machine-readable twin: [`seal-table5-same-identity-20261009.json`](seal-table5-same-identity-20261009.json).

## Paper Table 5 (historical, as printed)

Host claim: **osx/arm64**. Protocol claim: **minimum of three**. Instance: **decision-network (DENSE lookup)**, **not** the seven-stage network-compiler pipeline of Table 4. Workload: every compiler builds / runs the same job — compile unisacc itself at `-O2` — with byte-equal outputs.

| Row | Builder | Build time | Binary size | That binary compiles `unisacc.c` | vs cc -O2 |
| --- | --- | ---: | ---: | ---: | ---: |
| 1 | cc -O2 | 1.81 s | 569,384 B | 0.15 s | 1.0× |
| 2 | cc -O0 | 0.15 s | 490,840 B | 0.49 s | 3.3× |
| 3 | tcc | 0.02 s | 683,008 B | 0.55 s | 3.7× |
| 4 | unisacc -O2 (itself) | 0.15 s | 677,154 B | 0.81 s | 5.4× |
| 5 | unisacc -O0 (itself) | 0.10 s | 1,056,930 B | 2.05 s | 13.8× |

Narrative claim retained in §7.2: self-built compiler ≈ **1.5×** the tcc-built one on the “compile unisacc.c” column (0.81 / 0.55).

## Verification — printed cells vs repo

| Check | Result |
| --- | --- |
| Printed cells in CN / EN / arXiv drafts | **yes** (three manuscripts only for most cells) |
| Sealed JSON under `archive/**` or `research/**` with these timings/sizes as a Table 5 receipt | **no** |
| Appendix A JSON path for Table 5 | **missing** — Appendix A pins Table 4 rows 1–2 to `r9-current-bench`; Table 5 is only named under **复现** as `TCC=<…> ./tests/bench_vs.sh` |
| `tests/bench_vs.sh` documents protocol | **yes** — `best()` = min of 3 `/usr/bin/time -p` wall times; builds flat self via `sourceflat.py`; compares `-O2` outputs with `cmp` |
| tcc / Apple cc **version strings** recorded next to Table 5 | **no** (exactly the gap §8.1 item 2 names) |
| Product / unisacc SHA-256 for the historical Table 5 unisacc rows | **no** sealed receipt |

**Nearest non-receipt hit:** `archive/prd-history-20260929.md` narrates an osx/arm64 `-O2` compiler image size **677,154 B** after a peep change (2026-09-25). That matches Table 5 row 4’s **size** cell as a historical product figure, but it is **not** a `bench_vs` timing receipt and does **not** pin build time, compile-self time, ratios, host, or compiler versions.

**Judgment:** **all five Table 5 rows are unpinned** as a sealed same-identity receipt. The table is appendix-strength historical prose plus a living script; it cannot close §8.1 item 2 without remeasure (or an explicit downgrade that keeps appendix-only status).

## Same-identity remeasure protocol (no execution until submission identity chosen)

**Precondition:** 政委 freezes one submission decision-network / product identity appropriate for Table 5 (DENSE path). Because Table 5 times the **decision-network instance**, do **not** substitute a seven-stage network-compiler `unisacc.com` timing and call it Table 5. If the submission artifact is only the network-compiler package, either (a) rebuild the decision-network instance from the same frozen source/weights identity and pin both, or (b) downgrade Table 5 in the manuscripts and leave this protocol unused. Do **not** mix tip gold with a released tag across rows.

**Host:** osx/arm64 (paper claim). Record `sw_vers`, `uname -m`, and `cc --version` / `xcodebuild -version` (or equivalent) in the receipt. Cloud Linux alone is **not** a drop-in replacement without explicit host-change labeling.

**Toolchain pins (required by §8.1 item 2):**

- Apple `cc` full `--version` string (and SDK if printed).
- `tcc` version string **and** the build-dir path used as `$TCC` (script requires `$TCC/tcc`).
- Flat self source: hash of the `sourceflat.py` output used for the run.
- Decision-network / builder identity: SHA-256 of each measured unisacc binary that appears in a row (cc-built and self-built).

**Measurement (align with living script unless upgraded and documented):**

| Step | Action |
| --- | --- |
| 1 | `TCC=<tcc build dir> ./tests/bench_vs.sh` on the pinned host |
| 2 | Keep **min-of-three** wall times (script `best()`), or document an upgrade to median-of-five if aligning with Table 4 protocol — do not silently change |
| 3 | Require `outputs differ 0` (script already fails otherwise) |
| 4 | Capture full stdout of the script plus the version pins above |

**Outputs:**

- Write `research/seal-table5-remeasure-<identity_prefix16>.json` with: `schema`, `submission_identity`, `host`, `cc_version`, `tcc_version`, `tcc_path`, `flat_self_sha256`, per-row build time / size / compile-self time / ratio, `byte_equal` flags, script path + script content hash, `source_commit` if built from tree.
- Optional archive copy under `archive/research/` after review.

**Paper update rule (after remeasure only):** replace Table 5 from **one** receipt, or keep historical table and cite remeasure separately — 政委 + author choice. This note does **not** edit Table 5 cells. CN / EN / arXiv must stay linked if cells change later.

## Relation to other §8.1 seals

| Item | Seal note | Status |
| --- | --- | --- |
| 1 Table 4 | [`seal-table4-same-identity-20261009.md`](seal-table4-same-identity-20261009.md) | rows 1–2 pin; 3–4 unpinned; remeasure blocked on identity |
| 2 Table 5 | **this note** | **all rows unpinned**; remeasure blocked on identity + version pins |
| 3 platform matrix | [`seal-same-identity-matrix-scaffold-20261009.md`](seal-same-identity-matrix-scaffold-20261009.md) | scaffold only; fill blocked on identity |

## Blockers

1. **Submission identity** not frozen (same family of blockers as Table 4 / matrix / gold remeasure notes).
2. **No in-repo sealed JSON** for the printed Table 5 cells; cannot retroactively seal without off-repo artifacts or remeasure.
3. **tcc / cc versions** absent from Appendix A and from any receipt next to the table.
4. **Instance mismatch risk:** remeasure must stay on the DENSE decision-network path; network-compiler pipeline timings belong to Table 4, not Table 5.

## Self-check

- `unisacc-paper.md` / `.en.md` / `arxiv-paper-a` Table 4 and Table 5 cells: **not edited**.
- Key counts 8509 / 8769 / 9174 in papers: **not edited**.
- `kernel/`, `weights/`, product sources, `facts/`: **not edited**.
- `tests/bench_vs.sh`: **not executed** this heartbeat.
- This heartbeat: research markdown + JSON only.
