# Gold-table mutation study (clone /tmp/s2a, 42 recipes of tools/cxlib/sourcepackage.cx)

Each mutation was followed by facts-gen --write (all), every check-decide decide-gen/abidispatch-gen --write, gen.exe for all 42 recipes, sha256 vs. the unmutated baseline, then revert.

| table | mutated key (old→new) | nets whose JSON changed | non-net consumers (files) | verdict |
|---|---|---|---|---|
| abi | write/win/x86_64 winimp WriteFile→ReadFile | win-x86_64-lower, object-lower-win-x86_64 (derived: exec/facts/lower-abi.tsv) | tools/abidispatch-gen.cx→weights/abi/dispatch.tsv, exec/lower/code-syscall-result.tsv, tools/check-stage-lower.cx, tests/stream-lower-domain.cx, tests/check-decide.cx | LIVE |
| binsel | +/s add→sub | shared-e3, warnparse, errorparse (via k2-gen2.tsv, k2-libraryenv.tsv) | seed/gold_binsel_label.c, tools/stream-e3-gold.tsv, tests/stream-e3-domain-matrix.tsv | LIVE |
| combo | write/win/x86_64 winimp WriteFile→ReadFile | none (no derived file changed) | none (no references) | DEAD |
| enc | write/lnx/x86_64 syscall→svc | lnx-x86_64-lower, object-lower-x86_64 (via lower-abi.tsv + lower-code.tsv) | tools/check-stage-lower.cx, tests/stream-lower-domain.cx, tests/stream-elf-domain-matrix.tsv | LIVE |
| irsel | alu/add add64→and64 | shared-e3, warnparse, errorparse (via k2-gen2.tsv, k2-libraryenv.tsv) | seed/gold_irsel_label.c, tests/stream-e3-domain-matrix.tsv | LIVE |
| isel | write/x86_64 symbol write→read | none (no derived file changed) | none | DEAD |
| lex | d/d num→bad | shared-e1, tokenlex, warnlex (via lex-gen.tsv) | seed/gold_lex_label.c, exec/lex/rules.md, tests/stream-e1-lex*.cx | LIVE |
| lexcls | 32 attws yes→no | shared-e1, tokenlex, warnlex (via lex-gen.tsv) | tools/{declared,declname,goldname,plain,rule}-gen.cx, exec/lex/rules.md, tests/stream-e1-lex.cx | LIVE |
| lexword | L pfxstr yes→no | shared-e1, tokenlex, warnlex (via lex-gen.tsv) | tools/{declared,declname,goldname,plain,rule}-gen.cx, tests/stream-e1-lex.cx | LIVE |
| opinfo | add64 bcls add64→other | shared-e4 (via opt-gen.tsv) | seed/gold_opinfo_label.c, tools/opinfo-cases-gen.cx, tests/coverage.tsv | LIVE |
| parse | stmt/while while→for | none (exec/facts/lex-gen.tsv DID change, but no net) | seed/gold_parse_label.c, exec/lex/rules.md, tests/stream-e1-lex.cx | CONTROL-ARM-ONLY (facts written, nets ignore) |
| peep | store64/load64/same_slot load_to_mov→keep | shared-e4 (via opt-gen.tsv + exec/opt/peepdecide-result.tsv) | seed/gold_peep_label.c, tools/peep-cases-gen.cx, exec/opt/peep-result.tsv, tests/check-decide.cx | LIVE |
| pfconv | x hex→oct | none (no derived file changed) | tools/stream-e3-gold.tsv, tests/stream-e3-domain-matrix.tsv | CONTROL-ARM-ONLY |
| pp | ifdef/1 take→skip | shared-e2, tokenpp, warnpp-shared (via pp-gen.tsv) | seed/gold_pp_label.c, tests/pp-gold-controls.cx, tests/net-controls.cx, tools/pp-gold-material.cx, tools/check-stage-lower.cx | LIVE |
| prec | + 9→8 | shared-e3, warnparse, errorparse (via k2-constexpr, k2-gen2(-tokens), k2-libraryenv, k2-membercontrol, k2-units-tokens, parse-tokens .tsv) | seed/gold_prec_label.c, tools/stream-e3-gold.tsv | LIVE |
| regmap (r0) | r0/x86_64 rax→rcx | all 6 x86_64 lower nets + 6 x86_64 enc nets (enc gen FAILED: "seed-gen: overlapping observation rules") (via enc-x86.tsv, lower-code.tsv) | tests/stream-lower-targets.cx, tests/stream-lower-domain.cx | LIVE |
| regmap (r7) | r7/arm64 x7→x6 | all 6 arm64 lower nets | (same) | LIVE |
| reloc | call/arm64 arm26→arm19 | all 6 arm64 lower nets (via lower-code.tsv) | seed/gold_reloc_label.c, tests/stream-lower-targets.cx | LIVE |
| scope | expr/id lookup→field | none (no derived file changed) | seed/gold_scope_label.c, seed/gold_arity_label.c, tools/stream-e3-scope-gen.cx, tests/coverage.tsv | CONTROL-ARM-ONLY |
| tyinfo | i32 narrow 1→0 | shared-e3, warnparse, errorparse (via k2-gen2, k2-libraryenv, k2-membercontrol) | tests/stream-e3-domain-matrix.tsv | LIVE |
| type | i32 + i64 i64→i32 | shared-e3, warnparse, errorparse (via k2-gen2.tsv, typedecide-result.tsv, typefloat-result.tsv) | seed/gold_type_label.c, seed/gold_arity_label.c, exec/parse2/typeresult-result.tsv, tests/check-decide.cx | LIVE |
| stmt/stmt | while while→for | shared-e3, warnparse, errorparse (via exec/parse2/stmtdecide-result.tsv) | exec/parse2/control-result.tsv, tests/check-decide.cx | LIVE |
| arity/arity | fixed/2/3 too-many-arguments→accept | shared-e3, warnparse, errorparse (via exec/parse2/aritydecide-result.tsv) | tools/arity-gen.cx, exec/parse2/arity-result.tsv, tests/arity-controls.cx | LIVE |

## Anomalies
- combo.tsv and isel.tsv: nothing references them (git grep empty in tools/seed/exec/kernel/tests), and facts and nets ignore them. They are dead copies that duplicate abi+enc (combo = form/symbol + abi columns).
- parse.tsv: mutating it rewrites exec/facts/lex-gen.tsv, but no net hash changes. The parse2 nets get their dispatch from stmt.tsv/decide results instead, so the parse.tsv rows in lex-gen.tsv are not read.
- pfconv.tsv and scope.tsv are read only by gold/control tooling (tools/stream-e3-gold.tsv, tools/stream-e3-scope-gen.cx, seed label C). No facts and no nets.
- enc.tsv feeds both lower-abi.tsv and lower-code.tsv: the form is duplicated across the two fact files.
- regmap: arm64 r7 (x7→x6) changes only the arm64 lower nets. x86_64 r0 (rax→rcx) changes lower nets, and makes every x86_64 enc gen fail with "overlapping observation rules" (the enc-x86.tsv facts collide on a duplicate register). So regmap is live but not a free label: changing a value can break uniqueness.
- Mutating abi.tsv does not change weights/abi/dispatch.tsv after abidispatch-gen --write. Only exec/facts/lower-abi.tsv changed, and that is what moved the win lower nets.
- warnparse once showed as changed in the first abi comparison. A recheck found an identical hash, and gen.exe parse2 is deterministic (3 identical runs). Treated as a transient compare artifact.
- The arm64/x86 enc nets (lnx-*-elf, object-enc-*) changed only in the regmap-r0 failure case. No gold table moved them otherwise.

Wall time of whole study: ~1590 s (1576 s measured from the post-build start mark, plus ~10 s clone/build).


## 督导裁决单（2026-11-05，依据上表与 kernel/ / built.json 核查）

| 表 | 变异结果 | 非编译网消费者 | 裁决 |
|---|---|---|---|
| combo | 42 网全不变 | kernel/dense.combo.inc、weight.combo.inc（kernel-emit 由 weights/built.json 生成）、tests/nets.cx、table-mask、check-cx-gold/netrt/plain | **不删**：编译器管线不读，但仍是 18 gold 经典网的一员（built.json 在用）。标"仅决策网对照臂"，prd 与论文注释说明 |
| isel | 42 网全不变 | kernel/dense|weight.isel.inc、unisa_model.inc、tests/nets.cx、table-mask、check-cx-gold | 同上：**仅对照臂** |
| parse | lex-gen.tsv 变但 0 网变 | seed/gold_parse_label.c、tests/stream-e1-lex.cx | **仅对照臂**；lex-gen.tsv 里 parse 行是死写——列入 NN-NF-F 正规形 lint 的"有表无读" |
| pfconv | 0 网变 | tools/stream-e3-gold.tsv、tests/stream-e3-domain-matrix.tsv | **仅对照臂** |
| scope | 0 网变 | seed/gold_scope_label.c、tools/stream-e3-scope-gen.cx | **仅对照臂** |
| 其余 17 张（abi binsel enc irsel lex lexcls lexword opinfo peep pp prec regmap reloc tyinfo type stmt arity） | 改一键即改对应网 | — | **LIVE** |

另三条须处理的异常：
1. **regmap r0 x86_64 rax→rcx 使 6 张 x86_64 enc 网生成失败**（`seed-gen: overlapping observation rules`）：enc-x86.tsv 由 regmap 派生时出现重复观测键——说明 enc 生成对 regmap 的单射性有隐含假设。立项修生成器（拒绝时报具名冲突键，或在 facts 层断言 regmap 单射）。
2. **enc.tsv 同时写进 lower-abi.tsv 与 lower-code.tsv**：同一 form 两份事实，属 AUDIT-DUP/NN-NF 的"一个事实一处"。
3. **abi.tsv 变异后 weights/abi/dispatch.tsv 不变**（abidispatch-gen 不读 abi.tsv），只有 lower-abi.tsv 变——第五例的决策表与 gold abi 表不是同一来源，要么让 dispatch.tsv 由 abi.tsv 派生，要么在 prd 写明两者分工。
