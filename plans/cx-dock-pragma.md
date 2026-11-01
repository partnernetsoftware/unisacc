# CX `dock` pragma：把「谁能对接 libc/POSIX/#include」从旁置白名单改成模块显式声明

## 问题（为什么现在的设计要改）

现在的分层规则——「上层 cx 模块不直接调 libc/POSIX，底层 L0 才对接」——是靠**三个旁置 TSV 白名单**在 lint 层事后拦截的：

- `tests/cxinclude-boundary.tsv`：谁能写 `#include`（27 行：`include/`、`tools/cxcc-hostinc/`、`cx/sys/` + 一堆 csih probe 精确条目）。
- `tests/cxraw-layers.tsv`：谁裸调 libc/POSIX 不计数（`cx/sys/`、`cx/mem/`、`cx/text/` + `src/host.cx` 等精确文件）。

缺陷：
1. **规则在旁置表里，不在代码里**。一个模块「能不能对接 libc」由「它在不在某个 TSV 前缀下」决定，改目录、加文件、加例外都要去翻两张表，容易改乱（2026-11-01 已经发生一次：为解工具脚本的 printf 误伤，把 `tools/`/`tests/` 按目录豁免，结果把 `tools/cxcc.cx`、`tools/cxproc.cx` 两个真上层模块也一起豁免了）。
2. **lint 靠「目录前缀 + 精确例外」匹配**，不是「读模块自己声明了什么」。目录一变、文件一挪，规则就漂移。
3. **`#pragma unisa` 明明只有 `module` 和 `use` 两种**（`cx/spec.md` CX-G-1），「对接 libc」这个语义**根本没有在语言层表达**，只能靠外部表补。

## 方案：加一个 `dock` 指令

在 `#pragma unisa` 里加第三种指令 `dock`，让「本单元对接宿主（允许 `#include` + 直接调 libc/POSIX）」变成**模块文件头自己的显式声明**。

语法（`#pragma unisa dock [libc|include]`，可带参数也可裸写）：

```c
// cx/sys/fs.cx  —— L0 对接层，声明自己可以对接宿主
#pragma unisa module fs
#pragma unisa dock

// 只声明允许 #include（但不裸调 libc 符号）——极少数场景，如头面探针
#pragma unisa dock include

// 只声明允许裸调 libc/POSIX 符号（但不写 #include）——如 host 引导层
#pragma unisa dock libc
```

- 裸 `dock` = 同时允许 `#include` 和裸 libc/POSIX。
- `dock include` = 只允许 `#include`。
- `dock libc` = 只允许直接调 libc/POSIX 符号（不 `#include`，走隐式前导里的原型）。

## 规则（lint 从「查白名单」改成「查 pragma + 符号」，纯机械）

1. **`#include` 出现在无 `dock`（含 `dock include`）声明的单元 → 拒绝**，诊断 `unisa pragma dock required for #include`。
2. **裸 libc/POSIX 符号调用出现在无 `dock`（含 `dock libc`）声明的单元 → 拒绝**，诊断 `dock required for <name>`。
3. **上层模块（无 dock）`use` 一个 dock 模块 → 允许**（对接层就是给上层用的）。但**无 dock 模块不得再 `use` 另一个无 dock 模块之外的东西，也不得自己偷偷调 libc**——由 1、2 两条保证。
4. **`dock` 声明只允许出现在对接层语义的模块里**：产品路径的 `src/`、`cx/compiler`、`cx/runtime`、`cx/core`、`cx/driver` 若出现 `dock`，等于声明「这一块退回宿主实现」，要人工审（见「迁移」——它们现在大多数本来就有裸 libc 债，正该逐个审：哪些是真对接层该 dock，哪些是债该清）。

## 落点（改哪些）

| 层 | 改什么 |
|---|---|
| 语言 spec | `cx/spec.md`：CX-G-1 从「只有 module/use」改成「module/use/dock」；新增 CX-DOCK-1…4 条款（语法、语义、拒绝诊断、dock 模块可被 use） |
| 解析器（三源一致） | `cx/cx0/plan.cx`（权威）、`src/cxmod.c`（投影）、`cx/compiler/cxmod.cx`（产品模块）——都加 `dock` 指令识别；`tools/cx0-extract.cx` 若投影 pragma 也同步 |
| lint：include | `tests/cxinclude.cx` 不再读 `cxinclude-boundary.tsv`，改成「扫 `#include` 时先看该文件有没有 `dock`/`dock include`」；白名单文件废弃 |
| lint：libc 调用 | `tests/cxlibc.cx`、`tests/cxraw.cx` 不再读 `cxraw-layers.tsv` 的「目录豁免」，改成「扫裸 libc 调用时先看该文件有没有 `dock`/`dock libc`」；白名单文件废弃 |
| 迁移 | 逐个给「真对接层」加 `dock` 声明：`cx/sys/*`、`cx/mem/*`、`cx/text/*`、`src/host.cx`、`src/hostnative.cx`、`tools/cxcc.cx`（它是被 use 的上层模块，需裁定是「该清债」还是「该 dock」）、`tools/cxproc.cx`、csih 头面探针（它们 `dock include`）。其余上层模块一律不加 dock，反而把现有裸 libc 债照常清 |

## 一次解决的两个历史纠缠

1. **工具/测试脚本（tools/tests）的 printf 误伤**：工具脚本要输出，本来就用 printf。它们若裸调 libc，就在文件头加一行 `dock`（或明确「工具脚本不算上层模块、不必 dock」——这是要 omp-go 定的一条，见「待定」），不再靠「目录豁免」这种一刀切。
2. **「只降不升」机械门禁**：dock 落地后，lint 变成「有无声明」的机械拒绝，不再需要「逐文件基线数只降不升」这个 cc 加塞的门禁。门禁可以退化成「上层模块不得 dock 之外再裸调 libc」（由规则 1/2 直接保证），基线表可废。

## 待 omp-go 定的一条

`tools/`、`tests/` 里的**纯工具脚本**（非 `#pragma unisa module`、不被别的模块 use 的）：
- 方案 A：它们不是「模块」，不参与 dock 规则，printf/fprintf 自由（本机跑、不跨目标）。
- 方案 B：它们也算「上层单元」，要输出就 `dock`，否则也禁 libc。

建议 A——「模块」和「脚本」是两个概念，脚本不该背模块的分层规则。但这条影响 `#pragma unisa module` 的边界语义，由 omp-go 裁。

## 验收

1. `#pragma unisa dock` 三源解析一致，`cx0-extract --check` 报 EXACT。
2. 一个无 dock 的上层模块写 `#include` → 编译期就报 `dock required for #include`（不再是 lint 事后拦）。
3. 一个无 dock 的上层模块裸调 `strcmp` → 报 `dock required for strcmp`。
4. `cx/sys/*` 等对接层加了 dock 后照常编过、行为不变、两代逐字节相同。
5. `tests/cxinclude-boundary.tsv`、`tests/cxraw-layers.tsv`（目录豁免部分）废弃，`cxinclude.cx`/`cxlibc.cx`/`cxraw.cx` 改成读 pragma 不读目录。
6. 七棘轮语义改为「无 dock 却对接 → 报」，不再「基线数只降不升」。
