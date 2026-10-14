# c43 十六进制立即数债｜第0步：拒绝点定位与最小修法（只是设计，2026-10-15）

## 0. 复核与更正（cc 裁定后，2026-10-15）

- **案 A 已裁定**（cc）：后段边界规范为十进制，不动生成表；C 后段不补 0o/0b（0o 非 C 语法，参考为准，由前段边界统一规范）。
- **HEAD 复核**：按 `tests/bound.py` 分步构建 HEAD `3b86d7ef` 的干净导出产品（`exec/c/buildcompiler.sh` 的 shared、各目标、pack-prep-1..3、pack-models、pack-driver、pack；每步 ≤58 秒，引导编译器为根目录的 unisacc.com，只作编译器不作被测物）。HEAD 产品 `unisacc-next.com` 的结果与第1节一致：`-S` 全部接受，`-o/-run` 对 `0x1f`、`-0x10`、`0o17`、`0b11111` 报 `reject: not covered: an operand that is not a register or an integer`。
- **更正**：第2节第2条把 `src/back_lower.c` 的 `bk_num` 写成“后段读取 tape 的边界”，**不对**。`bk_num`（`src/back_lower.c:365`）只用于 ABI 的寄存器与系统调用号（`:576`、`:626`），不解析 tape 操作数。tape 整数由 `src/asmtext.c:81–101` 的 `at_num` 解析（认符号与 0x，不认 0o/0b）。编码器拿到的是已解析的 `SK_IMM` 值（`src/back_encode.c:479`），不再有文本。
- **拒绝的真实位置**：`src/` 中搜不到拒绝文本；它是生成表 `exec/enc/x86-operand-result.tsv` 的 `@rej` 载荷（`exec/enc/gen-manifest.tsv:17`），由 `seed/gen.c:3761` 等解释生成，产品运行时按表的状态机对**操作数原始文本**做分类。十进制状态（`ITO.digit`，`:3`）之外没有接受 `x`、`o`、`b`、字母位的状态，所以 `0x1f` 进入 `@rej`。**未追到的一环**：原始文本从 tape 到该表分类器之间的调用点，需要一次插桩定位，本步未做。
- **约束（新增）**：`-S` 文本输出两侧都逐字保留原写法（参考与产品都输出 `imm r2, 0x1f`）。因此案 A 的规范化只能加在**编码路径**（`-c`、`-o`、`-run`），不能加在前段或 `-S` 文本输出上，否则 `-S` 与参考失配。

## 0.1 验收探针（已落地为研究脚本，不入产品树）

`research/c43-hex-immediate-probe.sh PRODUCT REF`：七个 tape（十进制、0x、负十六进制、两个 64 位边界、0o、0b），四模式（S、c、o、run）与参考比对；超 64 位的字面量必须在 `-o` 被两侧拒绝且 stdout 为空。对 HEAD 产品运行：**现状失败 8 项**（hex、neghex、hi64、lo64 的 `-o` 与 `-run`），符合预期；修复后应全部通过。oct、bin 在参考的 `-o` 也拒绝（`back end: malformed tape`），两侧 rc 相同，探针不比较字节；它们的预期（参考拒绝）由 cc 按裁定保留或改为负例。

## 0.2 可执行补丁草稿：暂缓

补丁的落点是 0.0 节所述的未追到的调用点（原始操作数文本交给表分类器之前）。未定位就写补丁，会改错层，故本步**不交补丁草稿**，只交探针与定位方法。下一步：用插桩（在 `@rej` 之前打印操作数文本的调用栈）定位调用点，再写补丁草稿。

---

**证据边界**：探针实跑；拒绝点与修法为读源码的设计结论，**未改实现、未跑验收**。产品目录与 cdx 在途文件未碰。

## 1. 探针（同一 tape，四种模式）

- 参考：C 参考 UA，由 HEAD `ec087423` 的干净导出以 `tests/build_ref.sh` 构建（原生二进制，直接运行）。
- 出货产品：主检出根目录的 `unisacc.com`（**未跟踪的 10-13 构建，不是 HEAD 构建**；只用于定位拒绝点，结论待 HEAD 产品复核）。
- 模式与 `tests/tapedomaincheck.cx` 的 MODES 相同：`-S -o -`、`-o out.bin`、`-run`。

| 立即数 | C 参考 -S | C 参考 -o / -run | 出货产品 -S | 出货产品 -o / -run |
|---|---|---|---|---|
| `31` | 0 | 0 | 0 | 0 |
| `0x1f` | 0 | 0 | 0 | **1：reject: not covered: an operand that is not a register or an integer** |
| `-0x10` | 0 | 0 | 0 | **1，同上** |
| `0o17` | 0 | **1：back end: malformed tape** | 0 | **1，同上** |
| `0b11111` | 0 | **1：back end: malformed tape** | 0 | **1，同上** |

要点：`-S` 全部接受，所以 Tape 验域（前段）接受；产品后段在 `-o/-run` 才拒绝。C 参考接受十六进制与负十六进制，拒绝 0o/0b。Python 参考 `unisa/tape.py` 的 `_tape_integer` 对 0x/0o/0b 与负号全部接受（已实测：`0o17`→15，`0b11111`→31，`-0x10`→-16）。

## 2. 拒绝点（读源码，file:line）

1. **Tape 整数语法（前段，接受全部进制）**：`unisa/tape.py:288–302` `_tape_integer`，`prefixed` 识别 0x/0o/0b，`alphabet` 按基数限定。
2. **C 参考后段（只认十进制、0x、符号）**：`src/back_lower.c:365–377` `bk_num`，注释 `int(tok, 0)`，实际只识别 `+/-` 与 `0x`。`0o`、`0b` 在 `o`、`b` 处 `break`，使 token 残缺，经 `src/back_lower.c:380–381` `bk_tape_bad` 报 `back end: malformed tape`。
3. **出货产品的操作数扫描表（拒绝来源）**：`exec/enc/gen-manifest.tsv:17`，`x86-operand` 的 `register_reject=@rej:not covered: an operand that is not a register or an integer`。生成结果 `exec/enc/x86-operand-result.tsv:2–3`：`AN.guard` 以 `CMPI cls=C_ITOA` 进入 `ITO.digit`，之后 `:5` `LDI limit=2147483647` 做十进制位累加与上界。这张表只有十进制数字状态，没有 `x` 或 `o`、`b` 与十六进制字母状态，所以 `0x1f` 的 `x` 不属于任何接受状态，落到 `@rej`。
4. 类别来源：`exec/facts/export.py:1462` 的 `C_ITOA` 类，`:1475` 映射为 `itoa`。

未追到的一环：执行 `@rej` 的解释器（生成表到运行时的解释代码）本步未追，结论以生成表为准。

## 3. 最小修法（两案）

**案 A（推荐）：验域之后把整数规范为十进制再交后段。** 在后段读取 tape 的边界（`src/back_lower.c` 的 tape 解析，`bk_num` 之后、操作数交给扫描表之前）把整数操作数重写为十进制文本。扫描表与参考的 `int(tok,0)` 语义保持不变，表不动。
- 代价：边界处增加一次“值→十进制”的规范化；需要保证 64 位范围与负值（`bk_num` 现在不检查溢出，需先补界检查，使超界与 Python 的 `_tape_integer` 一致地拒绝）。
- 不动生成表 → 不触发派生链（kernel、weights、gen 键、RSS 路线）的重生成。

**案 B：扩表。** 在 `exec/enc/gen-manifest.tsv` 增加 `0x`、`0o`、`0b`、符号与十六进制字母状态，经 `export.py` 重生成 `x86-operand-result.tsv` 及其派生。
- 代价大：状态数显著增加；生成表进入 kernel、weights 与 gen 的读集，按规则要重钥、重测 RSS 路线（e4、o1 等）。
- 不推荐。

**附带决定（须 cc 定）**：C 参考后段 `bk_num` 不认 0o/0b，Python 认。两案之外需要定其一：(i) 让 C 后段也接受 0o/0b（改 `src/back_lower.c`，并验证与 Python 逐字节一致）；或 (ii) 让 Python 验域也拒绝 0o/0b（改 `unisa/tape.py`）。推荐 (i)，使两参考在语法上一致。

## 4. 验收探针草案（只是设计）

在 `tests/tapedomaincheck.cx` 增加正例与负例，四模式（S、c、o、run）逐字节比对参考：
- 正例：`imm r2, 0x1f`；`imm r2, -0x10`；`imm r2, 0x7fffffffffffffff`（上界）；`imm r2, -0x8000000000000000`（下界）。四模式输出与 C 参考字节一致。
- 负例：`imm r2, 0x10000000000000000`（超 64 位，前段与后段都拒绝，且拒绝措辞一致）。
- 0o/0b：按 (i)/(ii) 的决定成为正例（i）或负例（ii）。
- 预算：每个探针组一次 `tests/bound 58`，单步不跨越 58 秒。

## 5. 不做什么

- 不改 `exec/c`、`exec/c/libunisacc*`、`librarylatebind`、`relocate`（cdx 在途）。
- 不改生成表（案 B 不选）。
