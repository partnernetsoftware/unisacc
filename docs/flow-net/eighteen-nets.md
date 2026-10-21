# 18 网表（决策 · `unisa.gold.STAGES`）

- tip `70f61992` · 观测 2026-10-21 AWST
- 真源：`unisa/gold.py` → `STAGES`；README stages 表；金表在 `weights/gold/*.tsv`
- **是否封存** = `tests/seals/` 有点名该网的叶封/夹具原件；可标 STALE风险。**有封 ≠ 已冻**。金表 tsv ≠ seal 夹具。

| 网名 | 功能 | 主要落流 | 是否封存 | 备注（出货/对照） |
|------|------|----------|----------|-------------------|
| **pp** | dir×defined → take/skip/pop/macro（指令条件） | **e2** | **有** · `tests/seals/exec-pp-literals/`（STALE风险：autoinc/头资源耦合） | 出货主脊；唯一有点名叶封的金网；tip 窗重封多为库存货身份，非 `pp.tsv` 语义行 |
| **lex** | c×peek → token 类 | **e1** | **无** | 出货主脊；earlystageseal 不验证 E1/lex |
| **parse** | nt×tok → 归约/动作类 | **e3**（嵌） | **无** | 出货；STAGE-IDENTITIES：无单独产物，集成 e3 |
| **type** | t1×op×t2 → 结果类型 | **e3**（嵌） | **无** | 出货；当前集成 e3 |
| **scope** | ctx×kind → 作用域动作 | **e3**（嵌） | **无** | 出货 |
| **irsel** | family×flavor → IR 形 | **e3** | **无** | 出货（ask 在 e3） |
| **enc** | op×os×arch → 编码类 | **elf**（主嵌）；lower 次要 ask | **无** | 出货；身份合同钉 elf；经典 lowering 仍问 form |
| **reloc** | kind×arch → 重定位类 | **lower** | **无** | 出货；决策在 lower，image 只落记录 |
| **tyinfo** | t → size/uns/narrow | **e3** | **无** | 出货（类型旁路） |
| **pfconv** | conv → 指针/函数转换类 | **e3** | **无** | 出货 |
| **regmap** | treg×arch → 机器寄存器 | **lower** | **无** | 出货 |
| **isel** | op×arch → form/symbol | （构造有；主脊不问） | **无** | **对照**：金表仍构造；lowering **不 ask**（form 由 enc 覆盖） |
| **abi** | op×os×arch → syscall/形/门… | **lower** | **无** | 出货；`abi.tsv` 是金表非封存夹具 |
| **combo** | isel∪abi 对照臂 | 对照 / 非出货主脊 | **无** | **不出货主脊**；`drive=="combo"` 实验路径；research `seal-combo-*` ≠ 金网夹具 |
| **binsel** | op×sign → ALU 味 | **e3** | **无** | 出货 |
| **prec** | op → 优先级 | **e3** | **无** | 出货（表达式） |
| **opinfo** | op → 优化用属性 | **e4** | **无** | 出货 |
| **peep** | a×b×rel → peephole 动作 | **e4** | **无** | 出货 |

**汇总**：有封存原件 **1/18**（仅 pp）。tip 窗 `weights/gold/*.tsv` / `unisa/gold.py` **无提交**——「18/18 在调」是未冻/未拆/耦合仪表盘，不是语义全线在改。
