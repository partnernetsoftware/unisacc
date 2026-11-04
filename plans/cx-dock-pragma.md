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

## 已定（omp-go，2026-11-04）：脚本不是模块

条文：**只有 `#pragma unisa module` 声明的单元或被他模块 `use` 的单元才算「模块」，才受
dock 规则约束。** `tools/`、`tests/` 里的纯工具脚本（二者皆非）不是模块，`printf`/`fprintf`
等输出调用自由，不需要 `dock`。

判据（可机械实现，不看目录）：
- 受约束 ⇔ 该文件有 `#pragma unisa module`，或该文件被任何其它单元的 `use` 引用；
- 不受约束 ⇔ 两者皆无（本机跑、不跨目标的脚本）。

采方案 A 而非 B 的理由：
1. dock 的语义是「对接层与上层的边界」。脚本不是上层模块，也不是对接层，把它纳入 dock
   规则会让「模块」与「可执行脚本」两个概念混成一个，`#pragma unisa module` 的边界语义
   随之失真。
2. 方案 B 会让每个新写的 `.cx` 工具都先加一行 `dock` 才能 `printf` —— 工具脚本的输出是
   给人看的，不是跨目标 ABI 的一部分，这层手续没有对应风险。
3. 目录豁免确实是一刀切（`tools/` 下也有被 `use` 的模块，如 `tools/cxlib/*`、`tools/cxcc.cx`），
   所以本裁定废的是「按目录豁免」，改成「按是否模块」——这正是要消除的那种粗糙。

后果：
- `tools/cxlib/*.cx`、`tools/cxcc.cx`、`tools/cxproc.cx` 等被 `use` 的单元仍按模块处理
  （该清债清债，该 dock 的对接口 dock），不因本裁定豁免。
- 纯脚本无需任何声明；`tests/cxraw-layers.tsv` 的「目录豁免」部分可以按模块判据替换。

## 验收

1. `#pragma unisa dock` 三源解析一致，`cx0-extract --check` 报 EXACT。
2. 一个无 dock 的上层模块写 `#include` → 编译期就报 `dock required for #include`（不再是 lint 事后拦）。
3. 一个无 dock 的上层模块裸调 `strcmp` → 报 `dock required for strcmp`。
4. `cx/sys/*` 等对接层加了 dock 后照常编过、行为不变、两代逐字节相同。
5. `tests/cxinclude-boundary.tsv`、`tests/cxraw-layers.tsv`（目录豁免部分）废弃，`cxinclude.cx`/`cxlibc.cx`/`cxraw.cx` 改成读 pragma 不读目录。
6. 七棘轮语义改为「无 dock 却对接 → 报」，不再「基线数只降不升」。
