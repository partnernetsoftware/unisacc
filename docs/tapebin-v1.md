# tapebin v1：tape 的二进制封装（R14-7 落点 A）

目标是把**现有、已预处理的整个程序 tape** 编码成可检查的记录，供同一后端 `-run` 或落成六目标镜像。`target=0` 只声明指令集不含机器寄存器/字节码；它**不**撤销 C 前端已按 `_WIN32` 等宏选定的分支。跨平台语义等价仍由现有测试裁判，格式不充当证明或沙箱。

## 文件与记录

所有固定字段为小端；所有偏移从文件起点算，节起点按 8 字节对齐，填充必须为零。64 字节头：`UTAPEBIN`（8 字节）、`major=1`/`minor=0`/`opset=1`/`flags`（各 u16）、`target=0`（tape ISA 中立）/`section_count`/`header_bytes=64`（各 u32）、后续目录及节内容的 SHA-256（32 字节）、`origin_target:u32`。`origin_target` 固定枚举：0=未知或手写，1=lnx/x86_64，2=lnx/arm64，3=osx/x86_64，4=osx/arm64，5=win/x86_64，6=win/arm64；从 C 前端直接写包时按实际目标设置，从无来源标记的手写 `.tape` 转包默认 0，可显式指定。未知 major、opset、flags 位或 target 值必须拒绝。紧接 `section_count` 个 24 字节目录项：`kind:u16, flags:u16, offset:u64, length:u64, count:u32`；`kind` 固定为 1=`NAMES`、2=`CONSTS`、3=`RECORDS`、4=`SOURCE_SHA`、5=`TEXT_EXACT`，节 flags 仅 bit0=`required` 可置位。要求长度/计数有界、偏移不溢出、不重叠、文件无尾随字节。未知**可选**节跳过，未知必需节拒绝。

| 节 | v1 内容 |
|---|---|
| `NAMES`（必需） | ULEB128 个数；每项 ULEB128 字节长 + 原样名字字节。标签与数据符号共用索引，定义的种类由记录标明；同名定义的先后次序不丢。 |
| `CONSTS`（必需） | ULEB128 个数；每项 ULEB128 长度 + 原样字节。`.str` 引用池索引；`.bss` 只记长度，不展开零字节。池可去重，但定义记录顺序不变。 |
| `RECORDS`（必需） | ULEB128 条数；依原 tape 顺序写记录种类 byte：0=`label`、1=`.str`、2=`.bss`、3=指令。定义记录携名字索引及池索引/长度；指令携 1 字节 opcode，随后按冻结的 operand shape 写值。 |
| `SOURCE_SHA`（可选） | 32 字节原 C 输入摘要；没有原 C 输入时省略，绝不由 tape 猜造。 |
| `TEXT_EXACT`（可选） | 仅供手写 tape 在确有需要时保存注释、空白与数字拼写；执行前必须核实它重新解析所得记录与 `RECORDS` 一致，避免两个答案。规范编码与验收均不依赖此节；编译器 `-S` 所产包不携带此节。 |

opcode 0–70 按下面**固定次序**赋值（不是运行时字典迭代或 `src/back_lower.c` 的内部编号），`opset=1` 永不重排；`unisa/tape.py:SHAPE` 是唯一声明，导出 `docs/tapebin-v1.shape.tsv`，C 参考、Python、`exec/c` 从该文件生成或启动时逐项断言，门禁 `tapebin-shape` 比对三方：

```
0–10   imm mov add64 sub64 mul64 xor64 and64 or64 shl64 shr64 lshr64
11–20  .div .mod .udiv .umod slt64 sle64 ult64 ule64 eq ne
21–30  load64 store64 .ld .st .lea .zero jump jumpz call callr
31–46  callm .hostcall .librarycall .libraryaddr .hostaddr ret .frame .arg .print .write .exit .sys .sys6 .argc .argv nop
47–59  fadd64 fsub64 fmul64 fdiv64 flt64 fle64 feq64 fadd32 fsub32 fmul32 fdiv32 flt32 fle32
60–70  feq32 cvtid cvtud cvtis cvtus cvtdi cvtdu cvtsd cvtds fsqrt64 fsqrt32
```

operand shape 固定为上述 71 项快照，只能在新 opset 末尾追加。每条指令的寄存器按出现次序两个合一字节（低/高 nibble，值 0–7；奇数个时高 nibble 必须为 `0xf`）。`i` 用种类 byte（0=有符号、1=无符号）加**最短** SLEB128/ULEB128 表示 64 位值；计数和索引用最短 ULEB128（至多 10 字节）。`L`/`s` 用种类 byte（0=数值、1=`NAMES` 索引）；引用越界、数值溢出、非法寄存器、未知 opcode 或不符 shape 均拒绝。名字按原样字节存储，但含 NUL/空白/冒号而无法由当前 tape 文本语法表达的名字拒绝。行号不是执行语义；若手写 tape 以 `TEXT_EXACT` 带原文则保留物理行，除此不伪造源位置。

## 接入与兼容

`to_canonical_text` 是确定性的规范打印器：同一记录序列给出同一文本，不还原原始 `-S` 的空白与写法。`Tape.records` 按原行顺序保留定义、标签和指令；旧 `to_text` 暂留给既有种子构建路径，不作为二进制往返的依据。

验收用 `examples/` 与 `tests/c/` 的实际 tape，断言生成包不含 `TEXT_EXACT`：① 对同一输入，`encode(parse(t))` 两次逐字节相同，记录保持原行顺序；② `decode(encode(t))` 与 `parse(t)` 的记录逐项相等；③ 对所有生成的包，`encode(decode(b)) == b` 逐字节成立。这三项保证内容哈希稳定，不以文本空白为语义。文本与 tapebin 入口的 `-run` 输出及退出码相同，`-b <target>` 的六目标镜像逐字节相同。`-b <target> x.tapebin` 若 `origin_target` 非 0 且与目标不同，默认拒绝；显式 `--force-origin` 才允许跨目标试编，并在 README 限制表提示目标宏风险。C 参考与 Python 种子各有编解码器；模型产品的 `exec/c/` 只做格式校验和记录到既有 `tape.text` 的适配，把编译决策继续交给 E4/prune/lower/镜像网络。v1 不承诺省掉模型当前的文本解析，也不预先承诺包更小；记录实测尺寸和装载时间。加载前做结构校验；跳转可达性、栈平衡、权限与宿主导入安全仍属 FX-1 的独立验证器义务。

v1 内只可增加**可选节**和在新的 minor/opset 下追加 opcode；既有字段、编号、shape 与语义不得改写。旧读器遇到未知必需节或 opcode 明确拒绝，已接受的 v1 文件在后续 v1 读器上解释不变。任何不兼容改动升 major。
