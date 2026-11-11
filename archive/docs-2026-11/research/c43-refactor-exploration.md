# 抽象与复用：重构探索（只是设计）

2026-10-22，主人提醒「迟绑定、流与网的范式乃至模块，应思考抽象与复用」后，派强模型子代理只读探索（Fable 额度用尽，改 Opus 跑）。只读源码、grep/wc/git，未构建、未跑套件；下文为其报告的整理，未经实现验证。起因见 [sizeof 行与 (**p) 声明符案](c43-array-pointer-member-miscompile.md)。

## 诊断

- **表柱——寄存器无声明**：`exec/c/tbl.py:72-73` `regs.setdefault(n, len(regs))` 给任何新名（含拼错）静默分配 0 初值槽。E3「值」是散落的 vt(32 个 tsv)/vb(36)/rkok·rk·vid(7–10)/szrow(5)。部分保存配方常见且无标注：`gen2-actions-manifest.tsv` 的 `recipe_retseqs_stack4` 存 rkok/vid/rk 不存 szrow*；`recipe_updseqs_stack0` 不存 rkok/vid/rk。`@stack4` 按节编号，四个文件各指一物；`return-stack.tsv` 的栈条目已被配方取代却仍哈希进 facts（`exec/facts/export.py:1158`）。
- **类型编码表达不了 C 派生**：Python 参考有递归声明符（`unisa/front/parse.py:354`）产出类型树（`unisa/front/sema.py:45`）；模型与经典都压扁成「指针深度 + 基类型 + ≤8 维 + 标志」，写不出 P·P·A3、P·A3·A2。按路径补丁（`fp_more`、`declfpp`、经典 `cursize` 78 处）是今天两个缺陷的根因；`szrow` 靠「之后没发码」反推，是旁证不是值种类。
- **流合同柱**：e3 边界三套夹具形状（`exec-e3/cell.tsv`、`stagecontract-e1-e3/boundary.tsv`、`middleflow-step0`）；判定侧已由 streamseal 合一，产出侧没有——8 份 `tests/reseal-*.sh` 共 659 行近副本。
- **发布链**：`derived-map.tsv` + `derivedcheck` 只答「该跑哪个套件」，不管谁产出、顺序、是否需本树重建产品（`derived-map.tsv:58` 明确排除 stage-contract）；每刀落地链只存在于私有脚本——本日 abi-manifest 漏刷（f8f45cb6 补）即此。
- **晚绑定柱**：LB-8 要求类型/extent 字段比较，但三实现间无共享类型描述编码。

## 候选

|编号|名称|统一什么|不得合并|成本/风险|
|---|---|---|---|---|
|R1|状态模式表|每阶段 `regs.tsv`（记录/生命周期/复位点）+ `regcheck`：保存完整性、产出值转移是否全写、死文件；按现首次使用序声明，模型字节不变|阶段语义与拒绝|低–中|
|R2|显式值种类 `vk`|{scalar,ptr,row,fn} + 有限 op×vk 表，取代 szrow/rkok 旁证|—|中；须在 R1 后|
|R3|声明符模块 + TDESC|规范派生串 `P/A<n>/F<sig>`+基类型；各实现内唯一声明符；TDESC 兼作晚绑定类型/签名/extent 唯一来源；三实现只共享格式与探针域，不共享代码|拒绝文本与归属留调用方|高|
|R4|派生图 + 落地驱动|derived-map 加 producer/needs(本树产品/干净导出)/顺序；`--plan` 只读、`--check`；producer 不写 PASS、不由 commitgate 自动执行|—|低|
|R5|reseal 收进 cell.tsv|产出侧字段，8 脚本收成 `reseal LEAF`；网格/流格/pp 分界不变|网格与流格|低|
|R6|stagecontract 并为 cell 一种 kind|一套夹具模式，保留 MISSING/ABSENT/EMPTY 语义|S 合同语义|中|
|R7|配方/标签语义命名、删死文件|并入 R1；须保持状态序，否则 .tbl/.net 字节变|—|低|

见证实验（各 <1 天）：R1 仓外对 `e3.json` 跑 regcheck，预测报出 `retseqs_stack4`/`updseqs_stack0`，删 szcomma 保存的变异必红；R3 枚举 `{*,[N],(int)}` 深 ≤3 的约 39 种派生 × 4 上下文生成 sizeof/解引用探针，四方（gcc/经典/Python/产品）对拍，生成器本身可成该类缺陷之闸；R4 对 59bf6258..HEAD 每提交跑 `--plan` 看能否预测事后实刷集合（含 abi-manifest）；R5 新旧路径各重封，identity.tsv 与 case 字节逐字节同。迁移：先只检查的叠加或旁生流（同 LB-1 UNISYM1 做法），再逐消费者迁，每步 647 文件全差分对干净 HEAD 产品。

## 推荐

1. **R1 先做**：三缺陷的共同前提，第一刀对产品字节零影响，是 R2/R3 迁移的安全网。第一刀：仓外 `gen.py parse2`（≤58s），写 `regcheck.py` 与 VALUE 记录的 `regs-e3.tsv` 草案，列出全部部分保存配方逐项人工裁定，疑似缺陷写成 `tests/c/candidates/` 探针；szcomma 变异作负例；不改 `tbl.py`。
2. **R3 次之**：唯一能让丢级/错维在编码层不可表达、并给晚绑定描述符来源的候选。先做探针生成器，再出 TDESC 旁生流与 Python 参考对拍，最后迁消费者。

R4（R5 为其一部分）是纯工具，可并行。本日已落两块工具缺口：`derivedcheck` 导出用 tar.umask=022（384f84c6）；`refresh_gatedeps.py --index` 让改动与 reviewed-tree 戳同一 commitgate 提交（ec654a09）。

## 与硬约束冲突的诱人做法

形状/维度进金网（无界整数，金表无法穷举，且牵动全部 kernel/gold 封）；三实现共用声明符代码（参考对拍变空真）；DSL 重生成全部 `*-result.tsv`（状态序变致 .tbl/.net 全变、全部重封、构造撞 60s）；commitgate 自动 reseal（自封自证、陈旧根产品）；未声明寄存器默认 0 或 TDESC 缺失回落旧寄存器（隐式回退）；统一拒绝文本（stderr 字节变、归属模糊）；合并 netseal/streamseal（SEAL-MODULES 明说不是第三形态）；推广「无发射」旁证（不发码却改值种类的新运算会静默失效）；启用窄指纹（本版已裁暂不启用）。

## 不确定性

R1 在带 PUSH/RET 下推机上做全路径数据流的难度未评估，第一刀只承诺局部保存完整性；部分保存配方中真缺陷比例未知（预测多数合法）；R3 的状态数增长与构造时长未测；Python 参考在 `(**p)[N]` 类形状上是否全对未核实。
