# Paper A heartbeat seal: refusal wording and partial-function clarification

**Date:** 2026-10-09  
**Scope:** research-only (Paper A abstracts + Definition 3 triad)  
**Branch:** `seal-refusal-partial-fn-20261009`

## Gap found (verified on tip main before edit)

| Surface | Refusal on out-of-domain / uncovered keys | No classic-compiler fallback |
|---|---|---|
| `research/unisacc-paper.md` (abstract, CN) | yes (`构造网络对域外或未覆盖键拒绝即报错`) | yes |
| `research/unisacc-paper.en.md` (abstract) | yes | yes |
| `research/arxiv-paper-a/abstract.txt` | **no** | yes only |
| `research/arxiv-paper-a/main.tex` (`\begin{abstract}`) | **no** | yes only |

Reviewer risk: reading arXiv/CN+EN triad without the explicit rejection clause could treat uncovered-key behaviour as failed generalisation rather than designed partial coverage.

Definition 3 (query discipline) was already aligned across CN, EN, and `main.tex`; this seal does not rewrite that definition body.

## Changes made

1. **Abstract triad:** Insert into arXiv `abstract.txt` and `main.tex` abstract the same refusal fact as EN markdown: *Rejection on out-of-domain or uncovered keys is an error; there is never a fallback to a classic compiler.* CN abstract unchanged (already equivalent).
2. **One sentence after Definition 3** (CN `unisacc-paper.md`, EN `unisacc-paper.en.md`, `arxiv-paper-a/main.tex` only): deployed `ask` is a partial function relative to a larger ambient key/observation space (exact on declared \(K_s\), error outside); design not holdout generalisation; leave-out / training generalisation belongs to preregistered A2, not Paper A product claim.

## Files touched

- `research/arxiv-paper-a/abstract.txt`
- `research/arxiv-paper-a/main.tex`
- `research/unisacc-paper.md` (Def 3 clarifier only)
- `research/unisacc-paper.en.md` (Def 3 clarifier only)
- `research/seal-refusal-partial-fn-20261009.md` (this note)
- `research/paper-notes-20261009.md` (one-line cross-ref)

## Explicit non-changes (bans held)

- Key-count identities **8509**, **8769**, **9174** and all table numeric cells: **unchanged**
- Product/code outside `research/` (and arxiv under `research/`): **unchanged**
- No gold remeasure, Table 4/5 audits, or platform receipt inventory re-run
- No arXiv submission or product version seal changes
- No new theorems or claims that construction beats training

## What this seal does not close

- Measurement identity alignment (abstract vs Table 1 op-74 / op-87)
- Tables 4–5 historical artifact bands
- Platform matrix / release receipts
- T2b/T3 or whole-compiler simulation claims
