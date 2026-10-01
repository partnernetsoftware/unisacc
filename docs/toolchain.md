# 编译工具链：从单文件映像到可链接的目标文件（R16-7 设计，2026-10-01）

本文是 0.0.16–0.0.18 工具链工作的设计基线：先如实记录后端今天的形状（全部来自对 `src/back_*.c`、`unisa/image/`、`exec/enc/elfimage.py` 的阅读与对 `unisacc.com` 的实测），再给出 ELF `.o` 的规格、与系统工具的互操作矩阵、三版递进和验证方式。盘点表（已有/部分/缺/不做）在 [archive/plans/v0.0.16.md](../archive/plans/v0.0.16.md) H 节，本文只写设计。

## 1. 后端今天的形状（事实，不是愿望）

| 事实 | 出处 | 对 `.o` 的含义 |
|---|---|---|
| `-c` 与 `-S` 都写出 **tape 文本**（编译器的汇编级 IR），不是目标文件或汇编 | `src/main.c` 的用法行、README 限制表、2026-10-01 对 `unisacc.com` 实测 | `-c` 的语义要翻转，但 86 处套件把 `-c` 当 `-S` 的同义词，翻转分两步（§4） |
| 映像是**固定地址**的两段静态文件：ELF 文本在 `0x400000+176`，数据紧随其后按页对齐；Mach-O、PE 同理各有基址 | `src/back_encode.c` `bk_assemble`、`src/back_image.c` `bk_elf` | `.o` 里文本从 0 起、数据从 0 起，地址全部变成重定位 |
| 代码引用数据只有两种编码：x86-64 一律 **RIP 相对**（`x_rip`，`mov/lea r64,[rip+d32]`，位移是指令最后 4 字节）；arm64 一律 **`adrp` + `add`**（`a_adrp_add`） | `src/back_encode.c` 的全部 `x_rip`/`a_adrp_add` 调用点 | x86-64 只需 `R_X86_64_PC32`；arm64 只需 `R_AARCH64_ADR_PREL_PG_HI21` + `R_AARCH64_ADD_ABS_LO12_NC` |
| 代码取**函数地址**（`.lea 标号`）也走同样两种编码；分支与调用（`jmp/call`、`b/bl`）是节内 PC 相对 | `bk_leaaddr`、`a_relf`、`x_rel` | x86-64 节内 RIP 相对差值与链接地址无关，不需要重定位；arm64 `adrp` 的页差取决于节基址的页内偏移，**文本目标也要重定位** |
| 数据区**没有地址值单元**：`tape.relocs` 只被初始化和复制，从不追加；全局指针（含字符串字面量初始化）由 `__init` 里的代码在启动时写入 | `unisa/tape.py:101`、`unisa/lower.py:207`、`unisa/prune.py:186`、`src/front_parse.c` 的 `__init` | **不需要 `.rela.data`**，没有绝对地址重定位 |
| 零数据不存储：lower 把零块排在数据末尾（`zero_last`），映像只写到最后一个非零字节，其余由加载器清零；暂存单元（`bk_scr0`…）、`argc/argv` 单元、Windows 的 tape 栈都在这段零尾里 | `src/back_lower.c` `bk_lower`/`bk_repack`、`bk_nzlen` | `.data` = 非零前缀，`.bss` = 零尾；对数据的每个引用按偏移选择节符号 |
| **调用约定不是 SysV / AAPCS64**：参数经 tape 栈压入弹出（`TO_PUSH`/`TO_POP`、`@call.frame`），返回值在 r0；x86-64 与 Linux arm64 的 tape 栈就是进程栈（r7 = sp），Windows arm64 自带栈 | `src/back_lower.c` 775–830、927–933 | 与 cc 编译的函数**不能直接互调**；需要适配序（thunk），见 §3 矩阵 |
| 入口是 `_start`：先 `__init`（全部单元的全局初始化），再把 argc/argv 写进自己的单元，再 `call main`；不依赖 crt | `src/front_parse.c:5673` | `.o` 导出全局 `_start`，`ld` 不需要 crt1；被 crt 启动（`main` 由 C 运行时调用）要等调用约定适配 |
| `extern int x;` 没有定义时被**当作定义**（静默分配）；未定义函数报错 `undefined function` | 2026-10-01 用参考编译器实测 | 跨目标文件引用数据符号需要把 extern 变成 `UND` 符号，这是前端改动，排 0.0.17 |
| 后端有**三份实现**必须逐字节一致：Python（`unisa/image/`，构造源头）、C 参考（`src/back_image.c`）、产品 δ（`exec/enc/elfimage.py` 生成 `elf.net`）；`tests/closure.sh` 对映像逐字节对拍 | ARCHITECTURE.md 后端行、`exec/pipeline/image-stages.tsv` | `.o` 写出先在 C 参考落地（本版），产品 δ 与 Python 写出跟进（0.0.17），之后 closure 把 `.o` 也纳入逐字节对拍 |
| 随带 C 库以源码形式跟着编译器，按需裁剪进程序（`-ftrim-libc` 默认） | README、R11-3 | `.o` 自带它用到的库体，**不链接系统 libc**；与 glibc 共存要等符号前缀与调用约定问题一并解决 |

## 2. 格式范围与三版递进

| 版本 | 主线 | 本文覆盖的产物 |
|---|---|---|
| 0.0.16 | 收口 + 可观测的工具链第一步 | 本文；C 参考后端写 **ELF 可重定位目标文件**（lnx/x86_64、lnx/arm64），`ld` 能把它链成可运行程序；`tests/elfobj.sh` 门禁 |
| 0.0.17 | 自带静态链接器与调试行号 | Mach-O `.o`、COFF `.o`；产品 δ 与 Python 的 `.o` 写出并入 closure；自带静态链接器（只链自家 `.o`）；`ar/nm/objdump` 子集；`-g` 行号表；extern → `UND` 符号；函数级互调的适配序设计稿 |
| 0.0.18 | 汇编器子集与 `-S` 真汇编 | 汇编器子集与内联汇编；`-S` 输出真汇编并能被 `as` 与自带汇编器接受；`-shared`/`-fPIC`/`dlopen`；完整调试信息 |

顺延规则同计划：同一项顺延两次必须降级到 0.1.x 或砍掉。

## 3. 与系统工具的互操作矩阵

| 场景 | 需要什么 | 版本 | 验收 |
|---|---|---|---|
| 系统 `ld`（GNU ld / lld）把我们的 `.o` 链成自洽程序 | ELF `.o` 的节、符号、重定位正确；`_start` 全局 | **0.0.16** | 两架构各一例：链接后运行，输出与 `-b` 映像路线逐字节相同 |
| `llvm-readelf`/`llvm-objdump`/`nm` 能读我们的 `.o` | 规范的节头、符号表、`.rela.text` | **0.0.16** | 门禁用 `llvm-readelf -S -r -s` 解析并断言 |
| 我们的 `.o` 与 cc 的 `.o` 共链，**读 cc 定义的数据符号** | extern 未定义数据 → `UND` 符号 + 对未定义符号的重定位 | 0.0.17 | cc 的 `.o` 提供 `const char banner[]`，我们打印它 |
| 我们的代码**调用** cc 编译的函数 | 调用适配序：把 tape 栈上的参数搬进 SysV/AAPCS64 寄存器、对齐栈、取回返回值；原型信息来自前端 | 0.0.17 设计 / 0.0.18 实现 | 调用 cc 的 `int add(int,int)` |
| cc 编译的代码**调用**我们的函数 | 被调适配序（反向 thunk）；导出名不加前缀 | 0.0.18 | cc 的 `main` 调我们的函数 |
| 被系统 crt 启动（`cc a.o b.o` 直接可用） | 上一行 + `main` 走 C 调用约定 + 与 libc 的符号共存 | 0.0.18 之后，按需 | — |
| 链接系统 libc 代替随带库 | 符号前缀策略、调用约定、errno/stdio 状态归属 | 远期，不承诺 | — |
| `-shared` / `-fPIC` | 全部引用已是 PC 相对；缺动态节、GOT、PLT 与导出表 | 0.0.18 | `dlopen` 自家 `.so` |

## 4. ELF 可重定位目标文件规格（0.0.16）

**驱动语义。** `-c -b lnx/x86_64` 与 `-c -b lnx/arm64` 写出 `.o`（`-o` 给名，缺省 `a.o`）。**裸 `-c`（不带 `-b`）仍写 tape**，与 `-S` 同义——86 处套件和 README 的 ccparity 条目依赖这一点；等 Mach-O 与 COFF 的 `.o` 也能写（0.0.17 末）再把裸 `-c` 翻成“宿主目标文件”，并在 README 与 `ccparity` 同一提交里改口。`-c -b osx/*`、`-c -b win/*` 在本版报错并指向本文。产品（`unisacc.com`）在本版对 `-c -b lnx/*` 仍写 tape，盘点表如实标“参考侧有，产品 0.0.17”。

**布局**（全部小端，`ET_REL`，`e_machine` 62 / 183）：

```
ELF 头 (64)
.text        机器码，与映像完全相同的字节（除被重定位覆盖的字段）
.data        数据非零前缀（bk_nzlen），对齐 16
.rela.text   Elf64_Rela 表
.symtab      Elf64_Sym 表
.strtab      符号名
.shstrtab    节名
节头表       [0] null, [1] .text, [2] .data, [3] .bss, [4] .rela.text, [5] .symtab, [6] .strtab, [7] .shstrtab
```

`.data` 取到**最后一个非零数据块的末尾**（repack 后零块全在其后），`.bss` 是其余部分（含暂存单元与 `argc/argv` 单元）——不能取到“最后一个非零字节”，否则一个初值尾部为零的符号会跨 `.data`/`.bss` 两节，而链接器可以把两节分开放（0.0.17 修正）。Mach-O 同此；arm64 的重定位必须是外部的，所以每节有一个局部锚点符号 ltmp0..2，非零加数放在前置的 `ARM64_RELOC_ADDEND`；x86_64 的 `X86_64_RELOC_SIGNED` 把加数放在位移字段里。`.text`、`.data`、`.bss` 的 `sh_addralign` 都是 16。

**地址如何变成重定位。** 装配时对象模式把文本基址设为 0，把数据基址设为 2^32（`bk_shift = 2^32 − 256`），于是编码器看到的每个地址天然分成两类：< 2^32 是文本偏移，≥ 2^32 是数据偏移。编码器在两个且只有两个地方记录重定位：

| 编码点 | 目标 | x86-64 | arm64 |
|---|---|---|---|
| `x_rip`（位移在指令末 4 字节，P = 指令末 − 4） | 数据 | `R_X86_64_PC32`，S = `.data`/`.bss`，A = 偏移 − 4 | — |
| `x_rip` | 文本 | 不记录：节内 RIP 相对差值与链接地址无关 | — |
| `a_adrp_add` | 数据或文本 | — | `adrp` 处 `R_AARCH64_ADR_PREL_PG_HI21`，`add` 处 `R_AARCH64_ADD_ABS_LO12_NC`，S = 目标节，A = 偏移 |

数据偏移 ≥ 非零前缀长度的，S 取 `.bss`，A 减去前缀长度。分支/调用（`jmp/call/jcc`、`b/bl/cbz`）全部节内、不记录。没有 `.rela.data`（§1：数据区无地址值单元）。

**符号表。** `[0]` 空；`[1..3]` 三个节符号（`STT_SECTION`）；随后每个代码标号与数据符号各一个 **局部** `STT_NOTYPE` 符号（名字来自 tape，供 `objdump`/`nm` 读；数据符号的值按 `.data`/`.bss` 换算）；最后 `_start` 为 **全局** `STT_FUNC`。`sh_info` 指向第一个全局符号。本版不导出其他全局符号：没有 extern 语义之前，导出只会制造与 cc `.o` 的重名冲突。

**不变量。** 同一 tape 在对象模式与映像模式下，`.text` 字节除重定位字段外逐字节相同，`.data` 逐字节相同——门禁对此断言，这把 `.o` 写出钉在已有的 closure 证据上。

## 5. 验证（`tests/elfobj.sh`，门禁 `elfobj`）

1. 对一组探针（`examples/hello.c`、`fact.c`、`fib.c`、`ptr.c`、`struct.c`、`switch.c`、`indirect6.c` 以及 `tests/c/` 中带全局指针初始化的一例），两架构各写 `.o`；
2. 用 `llvm-readelf -h -S -r -s`（本机 llvm 已装；没有时用随附的最小 Python 解析）断言：`ET_REL`、`e_machine`、八个节、`.rela.text` 条目数 > 0、每条重定位的类型只在 §4 的集合里、符号 `_start` 全局；
3. `ld.lld -m elf_x86_64|aarch64elf -o prog prog.o`（本机 lld@21），再在 Lima 里运行 arm64 程序，输出与 `-b lnx/arm64` 映像的运行输出逐字节相同；x86_64 的运行走 `tests/linux.sh` 的 x86_64 虚拟机（宿主上是模拟，看门狗放大 10 倍）；没有链接器或虚拟机时点名跳过，`STRICT=1` 让跳过成为失败；
4. 不变量检查（§4 末）：`.text` 去掉重定位字段后与映像文本相同，`.data` 相同。

## 6. 不做与已知限制

- 不链接系统 libc，不走 crt 启动（见矩阵）；
- 不生成 DWARF/行号（0.0.17 `-g`）；
- `.o` 里的局部符号名是 tape 名字，不是源码里的 C 名字加前缀规则之外的任何修饰；
- 产品 δ 本版不写 `.o`，README 不得写成“产品支持 `-c`”。
