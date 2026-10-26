# 需求（强制）：`.cx` 逐步改造为无 `#include` 的模块 —— API 由 unisacc 提供，模块结构与内存安全参照 Rust

> 提出：2026-10-23（主人重申，强制要求，逐步实现）· 执行：cc · 依据：`docs/cx-spec-draft.md` v0.3、9df1417c（`.cx` 内置 prelude 已落地）

## 0. 一句话

`.cx` 是 POSIX C99 的**模块**：源码里不写 `#include`，所需 API 由 unisacc 按后缀内置提供（prelude，带版次）；模块间引用由 unisacc 解析（类 Rust `mod`/`use`），
内存安全由检查器而非新关键字保证。每一阶段都以**只降不升的棘轮闸**落地，不搞一次性大改。

## 1. 现状

2026-10-26：活树棘轮是 `tests/cxinclude.cx`（108 行 include，新文件必须为 0）、`tests/cxapis.cx`（strcpy/strcat/sprintf/gets 724 处，注释和字面量不计，只降不升）和 `tests/cxtrunc.cx`（被丢弃返回值、且长度可随输入变化的 snprintf/strncpy/memcpy；注释和字面量不计；新文件必须为 0，只降不升）。截断类首扫 4079。`examples/cx-lab/lex_table.cx` 的六处 `strncpy` 换成静态 `fit_field`（拷入上限并在终点写 NUL，与原先零填充数组上的 `strncpy` 字节相同）之后是 4073。同一输入仍打印 `lex_table: ok tokens=88 classes=9 rules=29`，token 文件字节不变。宿主 `gcc_cx`/`clang_cx` 用本仓库 `unisacc.com -cx-plan` 展开 `#pragma unisa use`，按每个单元自己的 include 列表交给系统 gcc/clang。产物仍是普通 C，不授予已检查身份。所有权检查仍是草案 §5，未实现。`examples/cx-lab/cxlab.h` 已改为同目录模块 `cxlab.cx`（enum 常量，导出 `cxlab_load_tsv` 与 `cxlab_timed_median`）；三个实验脚本改为 `use cxlab`。`cxlab_now` 没有搬过去：它依赖前奏尚未提供的 `sys/time.h`，而且没有调用者。`exec/`、`apps/csih`、`release/` 里剩余的 include 仍在原处。

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
