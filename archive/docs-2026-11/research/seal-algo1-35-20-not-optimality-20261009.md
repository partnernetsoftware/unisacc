# Seal: §3.2 35→20 is not an Algorithm 1 optimality theorem (Paper A)

Paper-A research heartbeat — **research-only wording**. **Key counts and product unchanged** (8509 / 8769 / 9174, 517 / 569; no kernel/weights/facts edits).

## Question

§3.2 names two different measurements in adjacent sentences: (1) greedy cover lines 3–5 reducing parameters 796,490 → 71,828 across the 18 stages; (2) exact minimum within the cube representation on five small tables, reported only as aggregate 35 → 20 units. The existing clause that 35→20 is not minimum width over arbitrary networks does not stop readers from conflating them—treating Algorithm 1 (or its greedy path) as near-optimal/proven optimal, or reading 35→20 as certifying the 796k→71k greedy reduction.

## Conflation risk

| Misread | Why it fails |
|---|---|
| 35→20 proves greedy cover optimal | Exact enumeration on five small tables only; separate control |
| 35→20 certifies 796,490→71,828 | Parameter reduction is full 18-stage greedy cover; different object |
| Paper A has an optimality theorem | Neither figure is elevated; complexity/optimality-gap study remains open |

## What was pinned (one sentence after 35→20 + arbitrary-network disclaimer)

| Locale | Location |
|---|---|
| CN | `research/unisacc-paper.md` §3.2 |
| EN | `research/unisacc-paper.en.md` §3.2 |
| arXiv | `research/arxiv-paper-a/main.tex` §3.2 |

### Exact sentence (EN)

That aggregate is exact only within the cube representation over those five tables (no per-table breakdown here); it is a separate exact-enumeration control—not a certificate that Algorithm 1's greedy cover is optimal or for the 796,490 → 71,828 reduction on the full 18-stage suite—and Paper A does not elevate either figure into an optimality theorem (greedy runtime and optimality-gap analysis remain open; see `research/paper-a-notes.md`).

### Exact sentence (CN)

该 35→20 合计仅在立方体表示类内、且仅对上述 5 个小表为精确枚举最小值（此处不声称逐表分解）；它是与贪心覆盖并列的精确枚举对照，不能证明算法 1 的贪心覆盖最优，也不能为全套 18 阶段上 796,490→71,828 的缩参作证书；Paper A 不把上述任一数升格为最优性定理（贪心运行时间与最优性差距分析仍为开放项，见 `research/paper-a-notes.md`）。

## Files touched

- `research/unisacc-paper.md`
- `research/unisacc-paper.en.md`
- `research/arxiv-paper-a/main.tex`
- `research/seal-algo1-35-20-not-optimality-20261009.md` (this file)
- `research/paper-a-notes.md` (Algorithm 1 bullet cross-ref)
- `research/paper-notes-20261009.md` (seal list one-liner)

`research/arxiv-paper-a/abstract.txt` — not changed (no §3.2 claim).

## Explicit unchanged

- Product / kernel / weights / facts: **unchanged**
- Key counts 8509 / 8769 / 9174 and units 517 / 569; Table 4/5, receipts, gold remeasure: **not touched**
- Per-table greedy-vs-exact and greedy runtime: **still open** (see `research/paper-a-notes.md`); this seal is wording hygiene only
