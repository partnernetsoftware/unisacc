# Paper A — arXiv LaTeX build (draft, not yet submitted)

LaTeX conversion of `research/unisacc-paper.en.md` for arXiv (primary cs.PL), pinned to repo commit `3b971ea`.

- `main.tex` + `figures/` — arXiv source (pdflatex, TeX Live; tar these two for upload)
- `main.pdf` — local build (25 pages)
- `abstract.txt` — arXiv metadata abstract (1,483 chars, under the 1,920 limit)

Byline: Yunzuo Chen, PartnerNet Software Pty Ltd, Australia (wanjochan@partnernetsoftware.com).
References: 27 entries, each checked against Crossref / arXiv / original sources.

Known open items: §5.4–5.8 read like release notes (move to appendix or cut); the md source remains canonical, so port wording changes back to `unisacc-paper.en.md`. The v0.0.23 K2 paragraph in `main.tex` describes the development tree, while the release measurements remain pinned to the earlier source commit. The K2 audit values in `main.tex` are centralized in seven `\KTwo...` macros, measured from development tip `61c81afc`: 9 DSL operations (cap 12, violations 0), stage-specific seed Python files 0, and 51 graphhash entries (8 parse2 flag configurations). Python check and simulator tools remain outside the seed-file count. `main.pdf` has not been rebuilt after this edit. Status: awaiting cs.PL endorsement.
