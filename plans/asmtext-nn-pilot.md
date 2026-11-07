# 试点：asmtext → NN（fable 首选；评审后才动产品）

替代 `plans/slice-shrink.md` 里的 netname 试点。不改产品、不碰 loader。

## 与包内 enc 网是否重复（核实）

| | `cx/compiler/asmtext.cx` | enc 网 `exec/enc/*` |
|---|---|---|
| 输入 | ELF 对象字节 / GNU 汇编文本 | lower 的 `target.text` |
| 输出 | GNU 汇编文本 / ELF 对象（含 `.unisa.tape`） | 机器码进镜像（elf 流） |
| 入口 | `at_dis` / `at_asm`（`src/driveface.cx`） | 七流 **elf** 阶段 |
| 合同 | `-S \| as == -c` 往返（asmtext.cx:25-37） | 编译器后端编码，不读 GNU 文本 |

**结论：不是同一张网。** 只共享「操作码知识」子集（助记符/编码）。ledger `2026-11-05T22:00 CORE-MIN-ASMTEXT TRIAGE` 同结论（督导，本页未重跑 enc 键）。

把 asmtext 整文件迁进现有 enc 网会混两种输入格式，拒绝。

共用操作码事实表（若做）：先 `--check` 且 **enc 网字节 `cmp` 不变**，再谈 asmtext 文本解析进新网。

## 输入输出字节流（提案）

- **dis**：ELF 对象字节 → GNU 汇编文本（现 `at_dis`）。
- **asm**：该文本 → ELF 对象字节（现 `at_asm`）。
- gold：冻结 `tools/asm-material.cx` 与现有 76 对象材料（路径若仍指归档 `src/asmtext.c` 则先修材料，否则 NOT_RUN）。
- 逐字节：`as(dis(obj)) == obj`；`tools/check-asmtext-tables.cx` 拒绝对比保持。

fable 数字（cc 转述，**未复跑**）：单片 `.text` 223,602B，gz 37,896B。不得写成已回收镜像字节。

## 风险

- 无界表：`AT_MAXT` 16MiB 文本、`AT_HASH` 等（asmtext.cx:7-10）；进网仍要无界数据态，等价只能抽样。
- 执行器 8×属性 / OOM panic（CORE-INPUT-MEMORY-OOM）未修前，大 ELF 输入危险。
- 不在加载网前提路径（`-S`/`as`/`objdump`），迁了也不自动缩小「执行器+host」切片核心。
- 收益归属：镜像 UNATTRIBUTED；`.text` 223KB 是对象/反汇编器体积口径。

## 不做

不把 enc 网当 asmtext 的实现。不实施直到 fable/cc 批准本页与材料路径可跑。
