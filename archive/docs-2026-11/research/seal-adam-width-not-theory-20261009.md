# Paper A heartbeat seal: §7.3 Adam rows are not a width-superiority theorem

**Date:** 2026-10-09  
**Scope:** research-only (Paper A abstract + §7.3 triad)  
**Branch:** `seal-adam-width-not-theory-20261009`  
**Tip SHA before edits:** `236362c5bd7d5c19c07e4c132260e53c7ebfdb7f`

## Reviewer-risk gap

After the refusal seal (PR #14) and the T1-vs-RQ2 locality seal (PR #16), a reader may still treat §7.3’s historical Adam table — especially 「原宽度下 type 降到 0.9514，须加宽」 / *the type stage drops to 0.9514 at the original width and must be widened*, merge stuck at **0.8143**, many-target collapse **0.4419** — as a **Paper A theorem** that construction is superior on **width** or **reachability**. That misread conflicts with `research/paper-notes-20261009.md` §「宽度作为一种复杂度」: $W^*$ versus training-width contests belong to preregistered **A2**; Paper A may only claim existence of constructed exact weights on the declared domain. If A2’s three falsification conditions (RQ1 width band) all fire, Paper A’s advantage language must shrink to **determinism, provability, and cost** — not “Adam failed to hit 1.000”.

## Audit table (tip `236362c5`, before this seal)

| Surface | Quoted sentence (Adam / width) | Judgment |
|---|---|---|
| CN abstract | 「§7.3 仅记录一次历史 Adam 示意对照，预注册研究 A2 负责系统性训练对照」 | OK |
| CN abstract | 「Adam 行在 §7.3 所述限制下与构造未对齐。」 | **overclaim-risk** (reads as product superiority) |
| EN abstract / `abstract.txt` / `main.tex` | `§7.3 records one illustrative historical Adam comparison only, and preregistered study A2 owns systematic training comparisons` | OK |
| EN abstract / tex | `the Adam rows did not align with construction under the limits stated in §7.3` | **overclaim-risk** |
| CN §7.3 | 表行含 0.9514 / 须加宽、0.8143、0.4419；开篇「历史示意」「A2…」；机理段「系统性失败，留给预注册研究 A2」 | **gap** (no explicit ban on width theorem) |
| EN / tex §7.3 | same table numerics; `illustrative history only`; `systematically fails … left to preregistered study A2` | **gap** |

T1/RQ2 opener sentence from PR #16 already present; this seal adds one **width / W\*** clarifier before the Adam protocol paragraph and softens the abstract “did not align” clause.

## Wording changes (before -> after)

### CN / EN / tex §7.3 (one sentence after T1/RQ2 opener, before Adam protocol)

- **CN before:** `…不是本文已证结论。我们在同一立方体架构上跑过一次 **Adam** 对照：`
- **CN after:** `…不是本文已证结论。表中 Adam 精度跌落与「须加宽」等行仅是在本节所述协议下的历史示意，不是构造在宽度或可到达性上优于训练的产品定理，也不否认训练在其它设定下可达到精确；$W^*$ 与训练宽度的竞赛及系统性构造—训练对照属 A2，若 A2 三条推翻条件均成立，Paper A 只保留确定性、可证明性与成本方面的主张，不得将「Adam 未达 1.000」升格为理论结论。我们在同一立方体架构上跑过一次 **Adam** 对照：`

- **EN before:** `…not a theorem proved here. We once ran an **Adam** control on the same cube architecture.`
- **EN after:** `…not a theorem proved here. The Adam accuracy drops and ``must widen'' figures in the table below are illustrative history under this section's stated protocol and limits—not a Paper A theorem that construction beats training on width or reachability, nor a denial that training could reach exactness under other settings; contests of $W^*$ versus training width and systematic construct-versus-train comparisons belong to A2, and if all three of A2's falsification conditions hold, Paper A's advantage language shrinks to determinism, provability, and cost—do not elevate ``Adam did not hit 1.000'' into a theory claim here. We once ran an **Adam** control on the same cube architecture.`

(`main.tex` §7.3: same EN text with LaTeX ``…'' quotes.)

### Abstract triad (half-clause on “did not align”)

- **CN before:** `Adam 行在 §7.3 所述限制下与构造未对齐。`
- **CN after:** `Adam 行在 §7.3 所述限制下与构造未对齐（仅为该节协议下的历史示意，非宽度或可到达性优势定理）。`

- **EN before:** `The Adam rows did not align with construction under the limits stated in §7.3.`
- **EN after:** `The Adam rows did not align with construction under the limits stated in §7.3 (illustrative history under that protocol, not a width- or reachability-superiority theorem).`

- **`abstract.txt`:** `section 7.3` instead of `§7.3` in the parenthetical (matches existing abstract style).

## Files touched

- `research/unisacc-paper.md` (abstract + §7.3)
- `research/unisacc-paper.en.md` (abstract + §7.3)
- `research/arxiv-paper-a/main.tex` (abstract + §7.3)
- `research/arxiv-paper-a/abstract.txt`
- `research/seal-adam-width-not-theory-20261009.md` (this note)
- `research/paper-notes-20261009.md` (one-line cross-ref)

## Explicit non-changes (bans held)

- Key-count identities **8509**, **8769**, **9174** and all table numeric cells (0.9514, 0.8143, 0.4419, 18.8 s, 3 s, θ counts): **unchanged**
- Product/code outside `research/`: **unchanged**
- No Table 4/5, platform receipts, or gold remeasure
- No arXiv submission or product version seals
- No claim that construction systematically beats training
- A2 preregistration four questions: **not rewritten**

## A2 cross-reference (read-only)

- Width / $W^*$ contests and three falsifiers: `archive/research/a2-preregistration.md` (RQ1); narrative: `research/paper-notes-20261009.md` §「宽度作为一种复杂度」
