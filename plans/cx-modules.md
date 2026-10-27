# 需求（强制）：`.cx` 逐步改造为无 `#include` 的模块 —— API 由 unisacc 提供，模块结构与内存安全参照 Rust

> 提出：2026-10-23（主人重申，强制要求，逐步实现）· 执行：cc · 依据：`docs/cx-spec-draft.md` v0.3、9df1417c（`.cx` 内置 prelude 已落地）

## 0. 一句话

`.cx` 是 POSIX C99 的**模块**：源码里不写 `#include`，所需 API 由 unisacc 按后缀内置提供（prelude，带版次）；模块间引用由 unisacc 解析（类 Rust `mod`/`use`），
内存安全由检查器而非新关键字保证。每一阶段都以**只降不升的棘轮闸**落地，不搞一次性大改。

## 1. 现状

2026-10-26：活树棘轮是 `tests/cxinclude.cx`（108 行 include，新文件必须为 0）、`tests/cxapis.cx`（strcpy/strcat/sprintf/gets 724 处，注释和字面量不计，只降不升）和 `tests/cxtrunc.cx`（被丢弃返回值、且长度可随输入变化的 snprintf/strncpy/memcpy；注释和字面量不计；新文件必须为 0，只降不升）。截断类首扫 4079。`examples/cx-lab/lex_table.cx` 的六处 `strncpy` 换成静态 `fit_field`（拷入上限并在终点写 NUL，与原先零填充数组上的 `strncpy` 字节相同）之后是 4073。同一输入仍打印 `lex_table: ok tokens=88 classes=9 rules=29`，token 文件字节不变。宿主 `gcc_cx`/`clang_cx` 用本仓库 `unisacc.com -cx-plan` 展开 `#pragma unisa use`，按每个单元自己的 include 列表交给系统 gcc/clang。产物仍是普通 C，不授予已检查身份。所有权检查仍是草案 §5，未实现。`examples/cx-lab/cxlab.h` 已改为同目录模块 `cxlab.cx`（enum 常量，导出 `cxlab_load_tsv` 与 `cxlab_timed_median`）；三个实验脚本改为 `use cxlab`。`cxlab_now` 没有搬过去：它依赖前奏尚未提供的 `sys/time.h`，而且没有调用者。`exec/`、`apps/csih`、`release/` 里剩余的 include 仍在原处。其后新增的 `tests/phase-timing.cx` 没有基线行，却带了 2 处 `strcpy` 和 4 处被丢弃的 `snprintf`，把无界闸和截断闸顶到 726 与 4077。这两处路径拷贝改为 `copy_fit`，收据文件名改为检查返回值的 `fit_print`，超长则具名拒绝。同一条 `ROOT PRODUCT ABS_OUT` 实测仍是 5 项断言、0 失败。基线文件当时没有改写，无界闸回到 724。截断回看原先只跳过空白，注释或标号后面的语句被漏计：`ujs/product/build-six.cx` 实际 4 处，旧基线记 3。修正后整树 4155 处（442 个文件，42 个文件上升，0 个下降），`tests/cxtrunc.tsv` 据此重写了一次。三元运算符的冒号仍然不算语句边界。在 `/* note */` 后面加一条真正的 `snprintf` 会让 `--check` 退出 1。include 仍是 108。2026-10-27：无界类再量是 724。`examples/cx-lab/lex_table.cx` 里给每个字节先写入 `OTHER` 的那处 `strcpy` 改为 `fit_field`（5 个字母写入 8 字节槽，和写入已清零数组的 `strcpy` 字节相同），活计数是 723。同一输入的 token 文件 sha256 仍是 `e43da7b4e70d7dc26ec0b44d2e1d66bd990b480be82642212e8e3dff28c393b2`，仍打印 `lex_table: ok tokens=88 classes=9 rules=29`。在该文件加回一条 `strcpy` 会让 `--check` 退出 1。截断闸仍是 4155。2026-10-27 模块边界：`tests/cxhost_mod.cx` 的导出函数调用一个文件内 `static` 助手。`unisacc.com -cx-interface` 只写出 `int cxhost_add(int a, int b);`，接口文本里没有这个助手。消费者 `tests/cxhost_main.cx` 在 `unisacc.com -run` 与宿主 `gcc_cx` 下都退出 42。只写一个不存在的模块名时，unisacc 按该名字拒绝，没有生成 gcc 二进制。gcc 产物仍是普通 C，不授予已检查身份。无界类本步改调用前是 461 个文件、757 处（基线 723，19 个文件高于基线）。没有把这次上升写进基线。`lex_table.cx` 里写入标识符种类 `ID` 的那处 `strcpy` 改为已有的 `fit_field`。同一输入的 token 文件 sha256 仍是 `e43da7b4e70d7dc26ec0b44d2e1d66bd990b480be82642212e8e3dff28c393b2`，仍打印 `lex_table: ok tokens=88 classes=9 rules=29`。同一时刻已经高于基线的其它 `strcpy`，以及被丢弃的截断调用，改成带长度的拷贝或检查返回值，使它们回到各自的基线行。无界基线据此只把 `lex_table.cx` 从 12 改到 11，整树 722。在该文件再加一条 `strcpy` 会让 `--check` 退出 1。截断类改调用前是 4210（基线 4155，17 个文件高于基线），收回之后仍是 4155，`tests/cxtrunc.tsv` 没有重写。include 仍是 108。2026-10-27 目录模块：`tests/cxdir_main.cx` 用 `#pragma unisa use cxdir.part`。现有旁边文件解析器把点号换成目录，读到 `tests/cxdir/part.cx`（模块名等于文件名 `part`）。`std` 与 `std.cx` 仍是前奏，没有第二条解析。提供方导出 `cxdir_fold`，它调用文件内 `static` 助手 `cxdir_pack`。`unisacc.com -cx-interface` 只写出 `int cxdir_fold(int a, int b);`，接口里没有这个助手。两次 `unisacc.com -run` 与两次宿主 `gcc_cx` 都退出 35，两次 gcc 编译退出 0。模块名 `cxdir.no_such_leaf` 在宿主编译之前被拒绝，stderr 含该名字和 `no file`，没有写出二进制。Node 与 Bun 的 `node_modules`、`package.json` 的 `exports`/`main` 和安装缓存仍不在这条解析器里。gcc 与 clang 的产物仍是普通 C，不授予已检查身份。无界类本步之前是 462 个文件、722 处；加上这两个 0 调用的新文件并改掉一处调用之后是 464 个文件、721 处（0 个高于基线）。`lex_table.cx` 写入种类 `NUM` 的 `strcpy` 改为已有的 `fit_field`。同一输入的 token 文件 sha256 仍是 `e43da7b4e70d7dc26ec0b44d2e1d66bd990b480be82642212e8e3dff28c393b2`，仍打印 `lex_table: ok tokens=88 classes=9 rules=29`。无界基线只把该行从 11 改到 10。截断类仍是 4155，`tests/cxtrunc.cx` 与 `tests/cxtrunc.tsv` 没有改。include 仍是 108。2026-10-27 两层 use：`tests/cxnest_main.cx` 只写 `#pragma unisa use cxnest.gate`，源码里没有内层名字。旁边文件解析器读到 `tests/cxnest/gate.cx`。外层模块自己 `#pragma unisa use leaf`，闭包把 `tests/cxnest/leaf.cx` 放进 `-cx-plan`。`std` 与 `std.cx` 仍是前奏。`-cx-interface` 对 gate 只写出 `int cxnest_fold(int a, int b);`，对 leaf 只写出 `int cxnest_leaf(int a, int b);`。文件内助手 `cxnest_wrap` 与 `cxnest_pack` 都不在接口里。外层助手调用内层导出。两次 `unisacc.com -run` 与两次 `gcc_cx` 都退出 36，两次 gcc 编译退出 0。模块名 `cxnest.no_such_leaf` 在宿主编译之前被拒绝，stderr 含该名字和 `no file`，没有写出二进制。Node 与 Bun 的 `node_modules`、`package.json` 的 `exports`/`main` 和安装缓存仍不在这条解析器里。gcc 与 clang 的产物仍是普通 C，不授予已检查身份。本步开始时无界类是 468 个文件、750 处（基线 721，4 个文件没有基线行），截断类是 4173（基线 4155，同样 4 个文件）。没有把这次上升写进基线。这 4 个文件收回之后，`lex_table.cx` 写入种类 `CHAR` 的 `strcpy` 改为已有的 `fit_field`。同一输入的 token 文件 sha256 仍是 `e43da7b4e70d7dc26ec0b44d2e1d66bd990b480be82642212e8e3dff28c393b2`，仍打印 `lex_table: ok tokens=88 classes=9 rules=29`。无界基线只把该行从 10 改到 9，整树 720。截断类仍是 4155，`tests/cxtrunc.cx` 与 `tests/cxtrunc.tsv` 没有改。include 仍是 108。

2026-10-27 互相 use：12:38 与 12:41 两次写入 `tools/hostseed-step-nativefirst.cx`、`tests/hostseed-chain-nativefirst.cx`、`tests/hostseed-independent-ident.cx`、`tests/hostseed-source-bind.cx` 的是本 cx 线。安全补丁停在 `8068f201e2`。`e3076abd21` 不回退。工具内容前缀仍是 `101ffc`。这四个路径的单写者窗口已经交给主线。`tests/cxcycle/ping.cx` 与 `tests/cxcycle/pong.cx` 互相 `#pragma unisa use`，没有 `#include`。两次 `unisacc.com -run` 与两次 `gcc_cx` 都在宿主编译之前拒绝，stderr 是 `module cycle: ping -> pong -> ping`，没有写出二进制。旁边文件解析器仍没有 `node_modules`、`package.json` 的 `exports` 和 Bun 安装缓存。gcc 与 clang 的产物仍是普通 C，不授予已检查身份。本步开始时无界类是 471 个文件、720 处，截断类是 4155。`lex_table.cx` 写入种类 `STR` 的 `strcpy` 改为已有的 `fit_field`。同一输入的 token 文件 sha256 仍是 `e43da7b4e70d7dc26ec0b44d2e1d66bd990b480be82642212e8e3dff28c393b2`，仍打印 `lex_table: ok tokens=88 classes=9 rules=29`。无界基线只把该行从 9 改到 8，加上两个 0 调用的新文件后整树 473 个文件、719 处。截断类仍是 4155，`tests/cxtrunc.cx` 与 `tests/cxtrunc.tsv` 没有改。include 仍是 108。

2026-10-27 前奏名：`tests/cxstd_main.cx` 写 `#pragma unisa use std` 和 `#pragma unisa use cxstd.mark`。`std` 是已有前奏，`-cx-plan` 列出 `tests/cxstd/mark.cx`，不为 `std` 列出文件。提供方导出 `cxstd_mark`，它调用文件内 `static` 助手 `cxstd_pack`。`-cx-interface` 只写出 `int cxstd_mark(int n);`，接口里没有 `cxstd_pack`。两次 `unisacc.com -run` 与两次 `gcc_cx` 都退出 11，两次 gcc 编译退出 0。`tests/cxstd_unknown.cx` 写 `std.absent`。unisacc 与 `gcc_cx` 都在宿主编译之前拒绝，stderr 含 `std.absent` 和 `unknown std module`，没有写出二进制。旁边文件解析器仍没有 `node_modules`、`package.json` 的 `exports` 和 Bun 安装缓存。gcc 与 clang 的产物仍是普通 C，不授予已检查身份。本步开始时无界类是 480 个文件、720 处（基线 719，`tests/pending/properties.cx` 没有基线行），截断类是 480 个文件、4172 处（基线 4155，`tests/pending/properties.cx`、`tools/p3-member-consume.cx`、`tools/p3-nn-asm-provenance.cx` 没有基线行）。没有把这次上升写进基线。这三处收回到 0 之后，`lex_table.cx` 回退路径上的 `strcpy(action, "bad")` 改为已有的 `fit_field(action, "bad", 23)`。同一输入的 token 文件 sha256 仍是 `e43da7b4e70d7dc26ec0b44d2e1d66bd990b480be82642212e8e3dff28c393b2`，仍打印 `lex_table: ok tokens=88 classes=9 rules=29`。无界基线只把该行从 8 改到 7，基线合计仍是 718，没有改写。干净树是 480 个文件：include 108 行；无界活计数 710（基线合计 718，0 个文件高于基线）；截断活计数 4138（基线合计 4155，0 个文件高于基线）。合计里的差额来自尚未进入该树的 `tests/hostseed-ancestry-controls.cx` 与 `tools/identity-material.cx` 两行。`tests/cxtrunc.cx` 与 `tests/cxtrunc.tsv` 没有改。`tests/pending/properties.cx` 与 `tools/p3-nn-asm-provenance.cx` 收回到 0 后进入提交，干净树不再有无基线行。`tools/p3-member-consume.cx` 有三行 `#include`，不能抬 include 基线，本步没有提交它。

2026-10-27 库成员批：`tests/p3-library-members.cx` 在 `5fd85c0130` 进入树，截断基线没有它的行。干净树因此失败：5 处被丢弃的调用（一处变长 `memcpy`，四处带增长格式的 `snprintf`）。这五处改为检查返回值的 `snprintf`，或按字节拷贝已有长度。无参数运行仍以 `usage ROOT PACK_CTX NEW_ABS_OUT` 拒绝，退出 2。基线没有改写。干净树是 481 个文件：include 108；无界活计数 710（基线合计 718，0 个高于基线）；截断活计数 4138（基线合计 4155，0 个高于基线）。gcc 与 clang 的产物仍是普通 C。

2026-10-27 前奏重跑：`tests/cxstd/mark.cx` 的 `-cx-interface` 只写出 `int cxstd_mark(int n);`，文件内助手 `cxstd_pack` 不在接口里。`tests/cxstd_main.cx` 的 `-cx-plan` 列出 `tests/cxstd/mark.cx`，不为前奏 `std` 列出文件。两次 `unisacc.com -run` 与两次 `gcc_cx` 都退出 11，两次 gcc 编译退出 0。`tests/cxstd_unknown.cx` 的 `std.absent` 在宿主编译之前被拒绝，stderr 含 `unknown std module`，没有写出二进制。旁边文件解析器仍没有 `node_modules`、`package.json` 的 `exports` 和 Bun 安装缓存。gcc 与 clang 的产物仍是普通 C。

2026-10-27 前奏重跑的棘轮：干净树 `48b5be2a4c` 上 include、无界、截断各退出 0，没有文件高于自己的行。481 个 `.cx` 文件，include 108 行（基线 108）；无界活计数 710（基线合计 718）；截断活计数 4138（基线合计 4155）。基线没有改写。旁边文件解析器仍没有 `node_modules`、`package.json` 的 `exports` 和 Bun 安装缓存。gcc 与 clang 的产物仍是普通 C。

2026-10-27 负例两文件：`tests/negative-diagnostics.cx` 与 `tests/negative-model-loader.cx` 在 `ebe891507c` 进入树，截断基线没有它们的行。诊断文件有 9 处被丢弃、格式会增长的 `snprintf`（27、28、66 行两处、70、76、79、81、103）。装载文件有 9 处同样的 `snprintf`（30、31、60、66 行两处、70、73、81、146）和一处变长 `memcpy(b, ape, apen)`。每处 `snprintf` 改为留下返回值，长度不够则 `die`。变长拷贝改为按已检查的 `apen` 逐字节写入，再异或一个字节。`path` 里已经检查过的 `snprintf`，以及装载文件里赋给 `n` 的那次 `snprintf`，没有改。夹具源字符串里的 `#include` 与 `strcpy` 没有改。基线没有改写，也没有为这两个文件加行。干净树在 `7d26c0a9f7` 上、只含这两处改动，include、无界、截断各跑两次，都退出 0，没有文件高于自己的行。483 个 `.cx` 文件，include 108 行（基线 108）；无界活计数 710（基线合计 718）；截断活计数 4138（基线合计 4155）。旁边文件解析器仍没有 `node_modules`、`package.json` 的 `exports` 和 Bun 安装缓存。gcc 与 clang 的产物仍是普通 C。

2026-10-27 负例回跑：`28a2912821` 之后，`tests/cxstd/mark.cx` 的 `-cx-interface` 仍只写出 `int cxstd_mark(int n);`，文件内助手 `cxstd_pack` 不在接口里。`tests/cxstd_main.cx` 的 `-cx-plan` 列出 `tests/cxstd_main.cx` 与 `tests/cxstd/mark.cx`，不为前奏 `std` 列出文件。两次 `unisacc.com -run` 与两次 `gcc_cx` 都退出 11，两次 gcc 编译退出 0。`tests/cxstd_unknown.cx` 的 `std.absent` 在宿主编译之前被拒绝，stderr 含 `unknown std module`，没有写出二进制。同一候选 `7a0cf28bba67f2394abaf783b886baeafe0687f4590c392dbd8611212a0f5f77` 上，`negative-diagnostics` 打印 `assertions=193 failed=0 known_gaps=3 archived_cases=17`，`negative-model-loader` 的 core 打印 `assertions=99 failed=0`。两批 `BATCH_RUN` 都是 PASS，rc 0，stderr 为空。基线没有改写。旁边文件解析器仍没有 `node_modules`、`package.json` 的 `exports` 和 Bun 安装缓存。gcc 与 clang 的产物仍是普通 C。

2026-10-27 调度回退：干净树 `6d9d5315a4` 的起点是 483 个 `.cx` 文件，include 108，无界活计数 710（基线合计 718），截断活计数 4138（基线合计 4155）。三闸退出 0，没有文件高于自己的行。`examples/cx-lab/lex_table.cx` 回退路径上的 `strcpy(next, "DISPATCH")` 改为已有的 `fit_field(next, "DISPATCH", 15)`。同一输入的 token 文件 sha256 仍是 `e43da7b4e70d7dc26ec0b44d2e1d66bd990b480be82642212e8e3dff28c393b2`，777 字节，仍打印 `lex_table: ok tokens=88 classes=9 rules=29`。无界活计数因此是 709，该文件低于自己的基线行。截断仍是 4138。两次复测都退出 0，没有文件高于自己的行。基线没有改写，`tests/hostseed-ancestry-controls.cx` 与 `tools/identity-material.cx` 的旧行还在。`tests/cxstd_main.cx` 写 `#pragma unisa use std` 和 `#pragma unisa use cxstd.mark`。`-cx-interface` 只写出 `int cxstd_mark(int n);`，助手 `cxstd_pack` 不在接口里。`-cx-plan` 列出 `tests/cxstd/mark.cx`，不为前奏 `std` 列出文件。两次 `unisacc.com -run` 与两次 `gcc_cx` 都退出 11，两次 gcc 编译退出 0。`std.absent` 在宿主编译之前被拒绝，stderr 含 `unknown std module`，没有写出二进制。旁边文件解析器仍没有 `node_modules`、`package.json` 的 `exports` / `main` / `imports`、`bun` 条件和 Bun 安装缓存。gcc 与 clang 的产物仍是普通 C。

2026-10-23 实测：

- 已有：`.cx` 单元自动前置 `cx.h`（产品 `exec/c/compiler.c` 与经典参考 `src/front_parse.c` 均已实现；转发单元不带 prelude）。
- 仓内 77 个 `.cx`：`#include <cx.h>` 64 处（已冗余）；系统头 unistd/stdint/sys/stat/fcntl/dirent/stdio/string/stdlib/errno/limits/ctype/sys/types 共 ~70 处；
  本地头 `cxlab.h`×3、`snapbase.h`×2、`../exec/c/codec.h`×2、`../exec/c/bind.h`×1；`.cx` 文本包含 `.cx`×1（`librarycallbacklowercheck.cx`）。
- flownet W0 实测（`research/flownet-w0-feasibility.md`）：多单元 `-run A.cx B.cx` 可作模块引用；**未定义的外部对象被当作全零读取而不拒绝**（未定义函数则在首调用处具名拒绝 rc127）——这是内存安全缺口。

## 2. 阶段（每阶段独立 commit、独立棘轮）

| 阶段 | 内容 | 闸 | 动产品字节? |
|---|---|---|---|
| CX0 | 删除冗余 `#include <cx.h>` 及 prelude 已提供的系统头；新增 `tests/cxinclude.cx` + `tests/cxinclude.tsv`（逐文件 include 计数，只降不升；新 `.cx` 必须 0） | `cxinclude` 棘轮 | 否 |
| CX1 | prelude 补齐 `.cx` 实际所需 POSIX API（stdint/fcntl/errno/limits/ctype/sys/types 等），版次化（`cx edition`）；随后删除对应系统头 | 棘轮下降 + 产品重建 + 28/28 不退 | 是（`include/cx.h`） |
| CX2 | 模块引用：`#pragma unisa use <mod>`（spec §2）由 unisacc 解析——加入该模块单元、按其非 static 定义生成接口（CXIF）注入原型；取代本地头与 `.cx` 文本包含 | 本地头 include 归零；去掉 use 后必须具名红 | 是（前端） |
| CX3 | 严格模式：`.cx` 中 `#include` 为具名拒绝；未定义外部对象具名拒绝（修 W0 缺口） | 负例闸 | 是 |
| CX4 | 内存安全：检查器闸——禁无界 API（strcpy/strcat/sprintf/gets…）、prelude 提供带长度的切片/缓冲 API；所有权/借用规则按 spec §5 逐条以棘轮引入 | 违规计数棘轮 | 视规则 |

CX2/CX3 设计已采纳（cc 2026-10-23）：`research/cx2-module-design.md`——`#pragma unisa use NAME` 只在使用者所在目录解析（点号=子目录，`std.*` 保留），
CXIF 由单趟扫描导出非 static 函数原型与类型、按哈希写临时头并作为该单元的强制 include（`src/cxmod.c` 共享），`-cx-plan`/`-cx-interface` 两个旗标驱动系统 cc 对拍；
CX3 分三步：`.cx` 中 `#include` 拒绝 → `extern` 拒绝（prelude 接管 `environ`，以单文件规则封住「未定义外部对象读零」）→ 调用未声明函数拒绝（设计期实测：跨单元隐式声明调用目前被接受）。
CX2a = 共享 `src/cxmod.c` + 两端前端接线 + 两旗标 + `tests/flownetw0.cx` 改 `use`，闸 11 负例 + 9 正例（含 `.c` 输出逐字节不变）。

顺序：CX0 → CX2a → CX1 → CX2 迁移 → CX3；CX4 的 lint 棘轮可与 CX1 起并行。CX1 起的产品改动与 0.0.43 收口（ruling R10）冲突时，归 0.0.44；CX0 与 CX4 闸本身不动字节，0.0.43 内即可落。

### CX4 首批规则（实例驱动，2026-10-23）

1. **静默截断**：`snprintf`/`strncpy`/`memcpy` 写入固定大小数组且长度取决于输入，而返回值未检查（或截断后继续使用）——flownet 适配器 `emit_wat` 的 `char t[512]` 把 ≥500 位小数的字面量截成坏 WAT（W2fix 验证者实测），属此类。规则：`.cx` 里此类写入必须用 prelude 的可增长缓冲，或检查返回值并具名拒绝。活闸 `tests/cxtrunc.cx`（2026-10-26）：只计返回值被丢弃的调用；`snprintf`/`strncpy` 还要求长度参数是十进制常量或 `sizeof`，且来源可随输入变长；`memcpy` 在长度不是这类常量时计入，包括恰好拷满的调用。首扫 4079，`lex_table.cx` 六处改为 `fit_field` 后 4073。
2. **无界 API**：`strcpy`/`strcat`/`sprintf`/`gets` 计数只降不升。
3. **未定义外部对象读零**（CX3b 的 `extern` 拒绝封住）。

## 3. 红线

1. 不为迁就 `.cx` 改 C99 语义：`.c` 单元行为逐字节不变。
2. 每步 ≤60s；棘轮只降不升；新增 `.cx` 一律 0 include。
3. 系统 cc 仍是差分 oracle：`cc -x c -include include/cx.h` 路径须保持可用。
