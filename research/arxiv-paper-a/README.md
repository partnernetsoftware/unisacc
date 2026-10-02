# Paper A — arXiv LaTeX build (draft, not yet submitted)

LaTeX conversion of `research/unisacc-paper.en.md` for arXiv (primary cs.PL), pinned to repo commit `3b971ea`.

- `main.tex` + `figures/` — arXiv source (pdflatex, TeX Live; tar these two for upload)
- `main.pdf` — local build (22 pages)
- `abstract.txt` — arXiv metadata abstract (1,483 chars, under the 1,920 limit)

Byline: Yunzuo Chen, PartnerNet Software Pty Ltd, Australia (wanjochan@partnernetsoftware.com).
References: 27 entries, each checked against Crossref / arXiv / original sources.

Known open items: Contribution 1 in the introduction still says "neural compilation";
§5.4–5.8 read like release notes (move to appendix or cut); the md source remains canonical,
so port wording changes back to `unisacc-paper.en.md`. Status: awaiting cs.PL endorsement.
