# Paper A — arXiv LaTeX build (draft, not yet submitted)

LaTeX conversion of `research/unisacc-paper.en.md` for arXiv (primary **cs.AI**; optional cross-list **cs.PL**), pinned to repo commit `3b971ea`. Account default category was cs.PL; primary is now cs.AI per 政委 2026-10-16.

- `main.tex` + `figures/` — arXiv source (pdflatex, TeX Live; tar these two for upload)
- `main.pdf` — local build (25 pages)
- `abstract.txt` — arXiv metadata abstract (1,483 chars, under the 1,920 limit)

Byline: Yunzuo Chen, PartnerNet Software Pty Ltd, Australia (wanjochan@partnernetsoftware.com).
References: 27 entries, each checked against Crossref / arXiv / original sources.

Known open items: §5.4–5.8 read like release notes (move to appendix or cut); the md source remains canonical, so port wording changes back to `unisacc-paper.en.md`. `main.pdf` has not been rebuilt after the latest wording pass. Status: awaiting cs.AI endorsement (cs.PL endorsement no longer applies as primary).
