# Paper A heartbeat seal: §7.3 trapdoor is mechanism illustration, not an unreachability theorem

**Date:** 2026-10-09  
**Scope:** research-only (Paper A §7.3 mechanism paragraph triad)  
**Branch:** `seal-trapdoor-not-unreachability-20261009`  
**Tip SHA before edits:** `b88a1da98c78f7f67bb007fd326a3e9d34fa247f`

## Reviewer-risk gap

After the Adam-width seal ([`seal-adam-width-not-theory-20261009.md`](seal-adam-width-not-theory-20261009.md)), a reader may still treat §7.3's **trapdoor phenomenon** / 「活板门现象」—the joint discrete jump in $(W_1,b_1)$ that small $W_1$ gradient steps cannot coordinate—as a **Paper A theorem** that gradient methods are unreachable on this cube construction, or as a new unreachability result proved in this paper. The paragraph already says *机理示意* / *Mechanism illustration* and defers systematic Adam/SGD failure to preregistered A2, but the named phenomenon can be elevated into a product-level gradient impossibility claim unless explicitly fenced.

## Audit table (tip `b88a1da9`, before this seal)

| Surface | Quoted sentence (trapdoor / mechanism) | Judgment |
|---|---|---|
| CN §7.3 | 开篇「机理示意」；活板门括号；「这与 §9…一致」「系统性失败，留给预注册研究 A2」 | **gap** (no explicit ban on trapdoor-as-theorem) |
| EN §7.3 / `main.tex` | `Mechanism illustration`; `trapdoor phenomenon`; aligns with §9; systematic failure → A2 | **gap** |
| CN abstract / Adam-width clarifier | 宽度/可到达性非定理（PR 已封口） | OK (unchanged by this seal) |

## Wording changes (before -> after)

### CN / EN / tex §7.3 mechanism paragraph only (one added sentence)

- **CN before:** `…却无法协调地完成这一跳变（活板门现象）。构造按规则逐条展开。`
- **CN after:** `…却无法协调地完成这一跳变（活板门现象）。此处「活板门」仅为本节立方体表示下的非形式机理示意，不是 Paper A 的不可达性定理，也不证明 Adam/SGD 在编译器规模合并与扩表上无法达到精确。构造按规则逐条展开。`

- **EN before:** `…(we call this the trapdoor phenomenon). Construction unfolds rule by rule.`
- **EN after:** `…(we call this the trapdoor phenomenon). The trapdoor label here names only an informal mechanism sketch under this section's cube representation—not a Paper A unreachability theorem and not a proof that Adam or SGD cannot reach exactness on compiler-scale table merges and extensions. Construction unfolds rule by rule.`

(`main.tex`: same EN text with LaTeX `---` em-dash and no curly-quote changes elsewhere.)

## Files touched

- `research/unisacc-paper.md` (§7.3 mechanism paragraph)
- `research/unisacc-paper.en.md` (§7.3 mechanism paragraph)
- `research/arxiv-paper-a/main.tex` (§7.3 mechanism paragraph)
- `research/seal-trapdoor-not-unreachability-20261009.md` (this note)
- `research/paper-notes-20261009.md` (one-line cross-ref in 封口 list)

## Explicit non-changes (bans held)

- Key-count identities **8509**, **8769**, **9174** and all table numeric cells (0.9514, 0.8143, 0.4419, 18.8 s, 3 s, θ counts): **unchanged**
- Adam-width clarifier sentence in §7.3 opener and abstract: **unchanged**
- Product/code outside `research/`: **unchanged**
- No Table 4/5, platform receipts, or gold remeasure
- No arXiv submission
- Algorithm 1 optimality wording: **unchanged**
- A2 preregistration: **not rewritten**

## A2 / §9 cross-reference (read-only)

- Systematic construct-versus-train and unreachability contests: `archive/research/a2-preregistration.md`; narrative: `research/paper-notes-20261009.md`
- §9 gradient-unreachability citations remain the literature anchor; this seal does not add a new Paper A theorem in that family.
