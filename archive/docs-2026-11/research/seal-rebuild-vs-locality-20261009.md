# Paper A heartbeat seal: exact rebuild (T1) vs edit locality (A2 RQ2)

**Date:** 2026-10-09  
**Scope:** research-only (Paper A abstract + §7.3 triad)  
**Branch:** `seal-rebuild-vs-locality-20261009`  
**Tip SHA before edits:** `24606778f68c3f27a0c222727e23c31e267b049f`

## Reviewer-risk gap

After the refusal/partial-function seal (PR #14), a reader may treat the abstract clause “construction rebuilds exactly when rules on a table change” / 「构造在表上增删规则时可精确重建」 as a **locality theorem**: edit one rule -> only local weight change and zero error on unrelated keys. That hypothesis is **A2 preregistration RQ2** (`archive/research/a2-preregistration.md`), not a Paper A product claim. Paper A’s intended reading is **T1**: rerun the constructor on the **updated full table** and recover a network exact on every key of the declared domain.

A secondary risk is reading §7.3’s Adam-vs-construction table as **systematic** proof that construction beats training; §7.3 is one historical Adam illustration; **A2 owns systematic training comparisons** (already stated in abstract and §7.3 opener).

## Audit table (tip `24606778`, before this seal)

| Surface | Quoted sentence (rebuild / superiority) | Judgment |
|---|---|---|
| CN abstract (`unisacc-paper.md`) | 「构造在表上增删规则时可精确重建」 | **overclaim-risk** (locality misread) |
| CN abstract | 「§7.3 仅记录一次历史 Adam 示意对照，预注册研究 A2 负责系统性训练对照」 | OK |
| EN abstract (`unisacc-paper.en.md`) | `Construction rebuilds exactly when rules on a table change` | **overclaim-risk** |
| EN abstract | `§7.3 records one illustrative historical Adam comparison only, and preregistered study A2 owns systematic training comparisons` | OK |
| arXiv `abstract.txt` | same EN rebuild sentence | **overclaim-risk** |
| arXiv `main.tex` abstract | same EN rebuild sentence | **overclaim-risk** |
| CN §7.3 | 开篇「下列仅为历史示意」「配套工作 A2…」；机理「构造按规则逐条展开」；「系统性失败，留给预注册研究 A2」 | OK (illustrative framing present) |
| EN §7.3 | `illustrative history only`; `systematically fails … left to preregistered study A2` | OK |
| tex §7.3 | same as EN §7.3 | OK |

No change required to §7.3 Adam table numerics or mechanism paragraph beyond a one-sentence T1 vs RQ2 cross-pointer in the opener (aligned with abstract).

## Wording changes (before -> after)

### CN abstract

- **Before:** `构造在表上增删规则时可精确重建；`
- **After:** `表上增删规则后，对更新后的全表做一次确定性全量重建，仍得到与表逐键一致的网络（T1）；这不主张单条规则编辑下的权重局部性或无关键零错——该问题留给预注册 A2 的 RQ2，而非本文产品主张。`

### EN abstract (+ arXiv abstract.txt + `main.tex` abstract)

- **Before:** `Construction rebuilds exactly when rules on a table change;`
- **After:** `After rules on a table change, a full deterministic reconstruction from the updated tables yields networks exact on every key again (T1)—not a proved locality theorem for single-rule edits or zero error on unrelated keys (preregistered A2 RQ2).`

(`main.tex` uses `---` em-dash; `abstract.txt` uses Unicode em dash.)

### CN / EN / tex §7.3 (one sentence after A2 disclaimer)

- **CN added:** `摘要「精确重建」指自更新表全量重跑构造器后仍全域精确（T1）；单条规则编辑的权重局部性与无关键零错属 A2/RQ2，不是本文已证结论。`
- **EN/tex added:** `In the abstract, "exact rebuild" means rerunning the constructor on the updated full table and remaining exact on the whole domain (T1); locality of single-rule weight edits and zero error on unrelated keys is A2/RQ2, not a theorem proved here.`

## Files touched

- `research/unisacc-paper.md` (abstract + §7.3)
- `research/unisacc-paper.en.md` (abstract + §7.3)
- `research/arxiv-paper-a/main.tex` (abstract + §7.3)
- `research/arxiv-paper-a/abstract.txt`
- `research/seal-rebuild-vs-locality-20261009.md` (this note)
- `research/paper-notes-20261009.md` (one-line cross-ref)

## Explicit non-changes (bans held)

- Key-count identities **8509**, **8769**, **9174** and all table numeric cells: **unchanged**
- Product/code outside `research/` (arxiv under `research/` only): **unchanged**
- No gold remeasure, Table 4/5, or platform receipt re-runs
- No arXiv submission or version seals
- No new theorems; no claim that construction systematically beats training
- A2 preregistration four questions: **not rewritten** (cross-reference only)

## A2 cross-reference (read-only)

- RQ2 locality hypothesis and falsification: `archive/research/a2-preregistration.md` §「RQ2 规则局部性」
- Paper notes locality bullet: `research/paper-notes-20261009.md` §「局部性」
