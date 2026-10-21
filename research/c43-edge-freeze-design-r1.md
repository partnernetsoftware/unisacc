# 边级冻结原件：设计轮收据（r1）

按 CLAUDE.md 脱敏流程：父会话先采证，写自包含抽象简报，fable 设计子代理只读简报、只写报告（不碰仓库、不跑命令）。报告全文标注「只是设计」；以下是对真实数据的核对与采纳结论。

## 简报给的硬约束

确定性逐字节判等；每闸 job ≤58s；叶=数据目录+参数化检查器；负例必红；优先不改生产实现；原件不得退化成比对闸。

## 报告要点（6 思路，推荐 2）

- 诊断：最有内容的定义是**规范化键答映射**（排序去重的 key→answer）为判等主体；完整序列只对参考端稳定且冻的是实现细节；生产端计数只配做注释。证明力闭环靠组合推理：边原件 × 问过比对（stages/stages-back）× 缓存≡表（oracle-cache）× 表级冻结（net-*-seal）。
- 推荐一（最稳，已采纳）：参考端规范化键答映射叶 + 既有 seal 形态收口；manifest 三重钉扎（探针 sha、金表 sha、叶体 sha），金表演进时主动红。
- 推荐二（最有内容，**未采纳本拍**）：抽样消融慢闸（每边抽 2–3 键变异金表重跑生产探针，断言输出变化）——把边原件升为生产端因果证据；与 58s 冲突，须抽样+分批，留作后续刀。
- 点名放弃：冻完整序列（记忆化实现细节入原件）、给生产端加 trace（撞产品闭包+需开/关等价新证明）、污点表（破逐字节等价）、全量消融（~2.3 万问级不可能）。

## 真实数据核对（tip 6bcd8cd5，lnx/x86_64 -O2 全程参考路线）

- 每边 distinct 键仅 3–84（reloc 3、peep/regmap/scope 8、…、type 84）；总问数 ~124k/探针。→ 键答映射叶尺寸=几十行，推荐一可行性成立。
- 逐问全序列双跑逐字节相同 → refcount（参考端问次）稳定，升入判等字段（报告原设计留为可选，实测后升格）。
- 相位归边干净：optimise 相位只问 opinfo/peep；compile_file/optimise/lower 三相位包裹即可归边。
- **pfconv 空洞**：fib.c、hello.c、浮点 printf 探针、整型 printf 探针（%d %s %x %c %u）两侧均 0 ask（C 侧 ask 点 `src/front_parse.c` printf 展开路径未达；Python 侧 `unisa/` 无 ask("pfconv") 调用点）。e3→pfconv 无见证探针，stages 存在性比对对它空真。已记入 tests/SEAL-MODULES.md「边级已知空洞」。

## 落地

首叶 `tests/seals/edge-e4-peep/`（8 键），检查器 `tests/edgeseal.py`，闸 `edge-e4-peep-seal`；边表 `tests/seal-edges.tsv` 计数 1/17。负例：闸内三类变异 + 落盘改一字节，均实测红。

简报与报告原文：会话 scratchpad（edge-freeze-brief.md / edge-freeze-report.md），要点已尽录于此。
