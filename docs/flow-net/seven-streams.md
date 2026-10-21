# 7 流表（产品脊 · `image-stages.tsv`）

- tip `70f61992` · 观测 2026-10-21 AWST
- 真源：`exec/pipeline/image-stages.tsv`
- **是否封存** = `tests/seals/` 有点名该流的冻结夹具/叶封原件；可标 STALE风险。**有封 ≠ 已冻**。

| 流名 | 输入→输出 | 产品网 | 功能 | 是否封存 | 备注 |
|------|-----------|--------|------|----------|------|
| **e2** | `src.c` → `pp.text` | `e2.net` | 预处理：续行/注释/指令/宏/条件/include | **有** · `tests/seals/exec-pp-literals/`（measured leaf；STALE风险：头/资源闭包持续进封件） | 近 tip 常被库头→autoinc→叶封拖动；纯转发原型刀可守冻哨 |
| **e1** | `pp.text` → `tokens.typed` | `e1.net` | 词法：关键字/标识/算符/字面量最长匹配 | **无** | `stagecontract-e1-e3` README 写明不含 e1 网；earlystageseal 不封 e1 |
| **e3** | `tokens.typed` → `tape.text` | `e3.net` | 语法+类型等合成段 → tape（parse/type 等物理嵌此） | **有** · `stagecontract-e1-e3/` + `middleflow-step0/e3/` + `librarycallbacke3/`（librarytyped* 历史≠PASS→STALE风险） | STAGE-IDENTITIES：parse/type **未拆**仍嵌 e3 |
| **e4** | `tape.text` → `tape.text` | `e4.net` | opt：tape 上 peephole / 栈→寄存器类改写 | **无** | seals 无 e4/opt 点名封；tip 窗 `exec/opt` 基本静，仍算未冻 |
| **prune** | `tape.text` → `tape.text` | `prune.net` | 按根/导出剪不可达整函数（公开 tape 路径可跳） | **无** | 仅中段路由；无 `prune.net` 点名冻结夹具；**不消费** 18 金网 |
| **lower** | `tape.text` → `target.text` | `lower.net` | ABI/regmap/syscall/布局/reloc 降到目标 ops | **有** · `stagecontract-tape-lower/` + `middleflow-step0/lower/` + `librarytypedlower/` + `librarycallbacklower/`（历史≠PASS→STALE风险） | tip≈99-commit 窗 **0** 笔改 lower 封/构造；有封且近窗不拖 |
| **elf** | `target.text` → `image.bytes` | `elf.net` | 编码 + ELF/Mach-O/PE（或内存镜像）写出 | **无** | 无 elf/enc/image 点名封；STAGE-IDENTITIES：enc/image **集成于 elf** |

**汇总**：有封存原件 **3/7**（e2、e3、lower）。勿与「在调 6/7」混读——在调口径含未冻/耦合/未拆，见观测 `seal-count-7x18-1049awst.md`。
