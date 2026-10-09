# Seal remeasure 2026-10-09 (tip gold key spaces)

Paper-A heartbeat evidence only. **Does not change** the labeled identities 8509 / 8769 / 9174 in the paper texts, and does **not** pick the submission identity.

## Question

On tip `873cc031`, which key-space identity does living `weights/gold/*.tsv` actually implement — the English/arXiv Table 1 label (op-74 / 8,509) or the Chinese Table 1 label (op-87 / 8,769)?

## Method

For each of the 18 Table 1 stages, read `#field` vocabulary lengths in `weights/gold/<stage>.tsv` on tip and take the Cartesian product. No network construction; **units were not remeasured** (still need `unisa acc` / constructed weights for the 517 vs 569 claim).

Aggregate SHA-256 pin: hash `===<stage>\n` + file bytes for stages in Table 1 paper order (pp…combo). A second pin hashes all 20 gold files in sorted name order (Table 1 18 + `lexcls` + `lexword`).

## Result (tip `873cc031`)

| Check | Value |
| --- | --- |
| 18-stage field-domain sum | **8769** |
| Matches CN Table 1 (op-87) | yes, every row |
| Matches EN Table 1 (op-74) | **no** — differs only on enc / isel / abi / combo (`op` 87 vs 74) |
| Mechanical gap 8769−8509 | 260 = 78+26+78+78 |
| SHA-256 pin (18 Table 1 stages) | `b8244bd8d9422dd32bb21af7053c38ef009d9d5a027c0924b1cac167fa454e70` |
| SHA-256 pin (20 gold files, sorted) | `1ef9608742cded03d0a7c1acc74572379efeae228a8f34e15be047bf6ff581b3` |
| Twenty-table domain sum (18+lexcls257+lexword15) | 9041 |

Machine-readable twin: [`seal-remeasure-tip.json`](seal-remeasure-tip.json).

## What this does **not** settle

- **Submission identity.** Tip gold is op-87 / 8769. EN/arXiv Table 1 remains the labeled historical op-74 / 8509 identity. Collapse only after 政委 chooses one sealed submission identity and the matching Table 1 + abstract are rewritten together (CN/EN/tex).
- **Units 517 / 569.** Not remeasured this round.
- **A2 9174.** Still no frozen gold on this tip that reproduces 9174 as a key count (9124 B in the texts is weight bytes, not keys).

## Self-check

Numbers in `unisacc-paper.md` / `.en.md` / `arxiv-paper-a` were **not** edited. Product code untouched.
