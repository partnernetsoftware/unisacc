# Seal remeasure 2026-10-09 (tip gold key spaces)

Paper-A heartbeat evidence only. **Does not change** the labeled identities 8509 / 8769 / 9174 in the paper texts, and does **not** pick the submission identity.

## Question

On tip `873cc031`, which key-space identity does living `weights/gold/*.tsv` actually implement — the English/arXiv Table 1 label (op-74 / 8,509) or the Chinese Table 1 label (op-87 / 8,769)?

## Method

For each of the 18 Table 1 stages, read `#field` vocabulary lengths in `weights/gold/<stage>.tsv` on tip and take the Cartesian product (key counts). Hidden units use the same tip identity: `python3 -m unisa acc` on shipped **constructed** weights (`weights/built.json`, `IntNet.nunits()` = hidden width `H`).

Aggregate SHA-256 pin: hash `===<stage>\n` + file bytes for stages in Table 1 paper order (pp…combo). A second pin hashes all 20 gold files in sorted name order (Table 1 18 + `lexcls` + `lexword`).

## Result — key counts (tip `873cc031` gold pin; `built.json` unchanged on `7026c648`)

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

## Result — hidden units (tip `7026c648`, same gold / `built.json` as above)

| Check | Value |
| --- | --- |
| Command | `python3 -m unisa acc` (constructed weights; full gold enumeration) |
| `weights/built.json` SHA-256 | `5e45bf5e018c9e1f0417fdbeb17cbf3b87463c761542f676686a7c5fc03d665a` (44,094 B) |
| 18-stage hidden-unit sum (`H` / `nunits()`) | **569** |
| Matches CN Table 1「单元」列行加总 | **yes** (`python3 -m unisa docs --check` exit 0 on `research/unisacc-paper.md`) |
| EN/arXiv Table 1「Units」列行加总 | **517** (manuscript op-74 identity; **not** current `built.json`) |
| Mechanical gap 569−517 | 52 = 14 (`isel`) + 17 (`abi`) + 21 (`combo`) |
| Same four stages as key gap? | **keys:** enc / isel / abi / combo (`op` 87 vs 74). **units:** only isel / abi / combo — `enc` stays 5 on both identities |

### Where 517 and 569 come from (read-only trace)

| Total | Source chain |
| --- | --- |
| **569** | Tip `weights/gold/*.tsv` -> `unisa build-weights` / committed `built.json` -> per-stage greedy cube cover -> `H` -> summed by `unisa acc` and by `unisa/docgen.py` `table_zh()` into CN Table 1. |
| **517** | **Not** a live sum over tip `built.json`. It is the **labeled** EN/arXiv Table 1 op-74 identity: add the printed per-stage Units column (6+11+…+100). That table assumes smaller `op` vocabularies on enc/isel/abi/combo, so keys total 8509 and three stages need fewer cubes. |
| **8769 / 8509** | Key side: field-domain products from gold TSV `#field` lines (prior section). Unit side does **not** follow key products; construction width is cover size on the actual gold rows. |

Per-stage receipt: [`seal-remeasure-tip.json`](seal-remeasure-tip.json) field `hidden_units`.

## What this does **not** settle

- **Submission identity.** Tip gold + constructed weights are op-87 / 8769 keys / **569** units. EN/arXiv Table 1 remains the labeled historical op-74 / 8509 / **517** identity. Collapse only after 政委 chooses one sealed submission identity and the matching Table 1 + abstract are rewritten together (CN/EN/tex).
- **A2 9174.** Still no frozen gold on this tip that reproduces 9174 as a key count (9124 B in the texts is weight bytes, not keys).
- **§8.1 platform matrix.** Per-target execution evidence from release receipts only is inventoried in [`seal-platform-matrix-inventory-20261009.md`](seal-platform-matrix-inventory-20261009.md); that does not replace a same-identity native matrix for submission.

## Self-check

Numbers in `unisacc-paper.md` / `.en.md` / `arxiv-paper-a` were **not** edited. Product code untouched.
