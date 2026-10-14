# c43 十六进制立即数｜第1步：A′（intcanon 有限 δ 阶段）与 B（扩 x86-operand 表）比较与推荐（只是设计，2026-10-15）

**证据边界**：以下事实来自读源码与既有读集（`exec/enc/gen-manifest.tsv`、`exec/c/compilerpack.py`、`exec/c/core.c`、`exec/c/core.h`、`exec/pipeline/stages.tsv`、派生读集）。表规模为**估计**，未生成验证。未改产品树，未派实现；2c1 合流前不派。

## 1. 已确认的事实

- **阶段接线**：`exec/pipeline/stages.tsv` 只列前段（E2、E1、E3）。镜像路线的阶段表在 `exec/c/compilerpack.py`：image 路线的校验要求阶段名恰为 `['e2','e1','e3','e4','prune','lower','elf']`（`compilerpack.py:228`），`tapecheck` 等由 `:176` 等处声明。插入新阶段必须同时改这些声明与校验。
- **拒绝的表**：`exec/enc/x86-operand-result.tsv` 共 52 个 scan 行，动作集闭合（CMPI、CMP、LDI、COPYW、REGN、LDX、INTERN、C64U、ALUI、A64、ADV、MARK）。十进制累加靠 `C64U`，状态只认十进制位。
- **执行器算术**：`exec/c/core.c:209–221` 的 `alu32` 只有 32 位运算（加、减、乘、除、模、与、或、异或、左移、右移）。**64 位的十六进制累加（移位加或）在现有 ALU 中不存在**，两案都需要通用执行器的 64 位原语。
- **读集影响**：`enc-elf`、`enc-macho`、`enc-pe`、`obj-enc-x` 四条生成路线读取 `exec/enc/x86-operand-*.tsv`（`tests/seedmemory-reads.tsv` 的派生读集）。

## 2. 两案比较

| 项 | A′：intcanon 有限 δ 阶段 | B：扩 x86-operand 表接受 0x |
|---|---|---|
| 形态 | 新阶段挂在镜像路线 `lower` 之后、`elf`/`macho`/`pe` 之前；输入 lowered 文本，输出规范十进制文本；`-S` 路线不挂。 | 在既有 x86-operand 扫描表中增加 `0x` 状态，直接把十六进制值累加进操作数 `av`，编码器照常消费。 |
| 表规模（估计） | 约 20–25 个状态、约 60 行：头行与 `@` 跳过、记号起点、符号、`0`、`x`、十六进制位累加、记号结束、十进制输出（约 20 位的除十取余循环）。 | 约 +12 状态、约 +30 行：`0x` 前缀与十六进制位（0–9a–fA–F），累加入 `av`。比 A′ 小，因为不需要十进制输出。 |
| 执行器新增（通用） | 64 位 shl、or，以及 64 位 div、mod（十进制输出需要）。 | 64 位 shl、or（十六进制累加）。少 div、mod。 |
| 改动文件域 | 新增 `exec/intcanon/`（gen-manifest、生成结果、Python 参考）；`exec/c/compilerpack.py` 的路线与校验（**cdx 在途，2c1 合流后再动**）；`exec/c/core.c` 的 64 位 ALU（通用，需单独评审）；探针 `tests/tapedomaincheck.cx`。 | `exec/enc/gen-manifest.tsv`；生成器 `exec/facts/export.py`；`exec/c/core.c` 的 64 位 ALU；派生重生成：`exec/enc/x86-operand-*`、`exec/facts/enc-x86*`、`kernel/weight.enc.inc`、`kernel/dense.enc.inc`、权重；探针。 |
| 派生与测量影响 | 新表只被新阶段读；**不改**既有四条生成路线的读集，不动其内存键与 RSS。新阶段需自己的 RSS 收据。 | 四条生成路线的读集变化：内存键必须重钥，RSS 须对 enc-elf、enc-macho、enc-pe、obj-enc-x 重测；kernel 与权重整链重生成（同 9645c6db 那一次的派生代价）。 |
| 与 `stages.tsv` 接线 | `stages.tsv` 不含镜像阶段；需在 `compilerpack.py` 接线（同上）。 | 无阶段变化，接线不动。 |
| 与“显式模型流、执行器通用、不手写解析器”的契合 | 高：变换是显式阶段，表是数据，执行器只加通用算术。 | 中：变换隐在扫描表里，语义与编码器耦合，难以单独对拍。 |

## 3. 推荐：A′

理由：
1. **边界清楚**：变换是显式阶段，可以用 Python 参考对拍（cc 要求的形式），也能单独证明“只规范 0x、其余逐字节不变”。
2. **不扩散**：不触发 kernel、权重、四条生成路线的内存键与 RSS 重测。B 的代价是派生整链，与“先收窄派生面”的方向相反。
3. **执行器改动可单独评审**：两案都必须加 64 位原语；A′ 多出 div、mod，但它们是通用算术，可在独立的一笔里以自身探针验收。

前置与顺序（建议）：
- 先：通用执行器 64 位 ALU 原语的单独一笔（`exec/c/core.c`，带探针：移位、或、除、模在 64 位上的边界）。
- 再：`exec/intcanon/` 表与 Python 参考，对拍只规范 `0x`/`-0x`。
- 最后：`compilerpack.py` 挂接（**等 cdx 2c1 合流**）与探针组。

## 4. 验收探针草案

沿用 `research/c43-hex-immediate-probe.sh`：七个 tape，四模式；新增两条：`-S` 文本逐字节不变（证明不挂 `-S`）；`0o`、`0b` 两侧同拒且具名。预算：每组一次 `tests/bound 58`。

## 5. 待 cc 定

1. 认可推荐 A′，并接受“64 位 ALU 原语先行、单独一笔”。
2. `compilerpack.py` 的挂接在 cdx 2c1 合流后再动。
