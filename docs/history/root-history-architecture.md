# ARCHITECTURE.md 迁出的历史叙述(2026-11-13)

本文不是现行指引。以下是从根目录 ARCHITECTURE.md 迁出的历史段落,逐字保留,标注原行号。

## 原第 5-9 行(0.0.43 当时入口说明,2026-10-25)

**0.0.43当前入口（2026-10-25重规划）：** 旧测试树与旧门禁已移入
`archive/testing-c43-legacy/`。新套件由`tests/run.cx`直接默认-run；当前smoke
只有16项局部断言，完整验收仍明确NOT_READY。产品构造支持独立在`tools/`，
其中`tools/sourceflat.cx`已原生替代源码导出逻辑。下文涉及历史探针的条目仅描述来源，
不能据此恢复旧队列或借用旧通过结果。

## 原第 49-51 行(旧候选记录指针)

旧候选 6MB/171 项、Q 编码/177 项与各平台自举、失败及性能记录均保留在
[S-17 迁移档案](archive/s17-migration-log-20260928.md) 和
[原始最终证据](archive/research/20260928/s17-final-evidence.json)，不作为当前候选验收。

## 原第 94 行后半(2026-10-30 的 kernel 溯源注释现状)

仓内 `kernel/` 41 个文件中有 40 个的溯源注释仍写着 `python3 -m unisa emit-kernel`：这些已提交字节还没有用 `kernel-emit --write` 重写过。2026-10-30 对仓内 `kernel/` 跑 `kernel-emit --check`，41 个文件都报 DIFF（40 个首处差异在溯源注释；`unisa_headers.inc` 是 `carried-header-export` 写出的，与 `kernel-emit` 的输出不同）。同日 `facts-gen --check`、`gold-table --check`、`cx-flatten --check` 均通过。

## 原第 118 行后半(入口候选覆盖范围,截至 2026-10-30)

五个目标切片、APE 封装与安装的步骤见 candidate-to-product；该计划写明截至 2026-10-30 入口候选只覆盖 lnx/x86_64，尚未换成产品。

## 原第 157 行后半(棘轮现状,2026-10-30)

用法如 `sh ./unisacc.com -run tests/cxinclude.cx -- --check`；2026-10-30 它仍失败，29 个文件超基线。

## spec.md:229(原文,2026-09-26 审阅叙述;同行内改写,行号不变)

决策点表**不再手抄在这里**：原先这里的 14 行手写快照（合计 6,650 行、413 单元）自称“生成”，实际已经与真表不符。bdy-ds4flash 审阅（2026-09-26）指出了这一点。生成的表见 `README.md`、`prd.tree.md` 和 `prd.map.md`，由 `unisa/docgen.py` 写出，`tests/docs.sh` 检查。按 `unisa/gold.py` 的 `ALL`，共 **18 张，8,484 行，513 单元**；C 编译器实际询问其中 16 张（isel 和 combo 不在路径上）。
