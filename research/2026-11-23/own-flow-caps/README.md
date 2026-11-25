# OWN-FLOW 容量上限:实测为具名拒绝,不是静默漏判

日期 2026-11-23。工作树 v0.0.98,产品 0.0.93(c561e7e0)。

## 结论先行

`build/pkg-gaps.tsv:142` 的 `OWN-FLOW-CAPS-SILENT-MISS` 描述**已经过时**。当前源里四条上限全部是
**具名拒绝**,不是「静默漏判」。实测原行见下。

那行的处理列自己也写了「0.0.78 把上限具名写进 plan.cx 头注释与 KNOWN-LIMIT,超限改报具名拒绝而不是漏判」——
也就是 0.0.78 已经做了,但状态列还挂 OPEN。

## 实测原行(产品 0.0.93,`sh ./unisacc.com F.cx -o OUT`)

| 夹具 | 原行 |
| --- | --- |
| `ok-64-names.cx`(64 个 malloc 名) | 无输出,产出 `m64m.bin` 12288 字节 |
| `refuse-65-names.cx`(65 个) | `cx: /tmp/owflow/m65m.cx:2: unisa safe: more than 64 released names for the flow check in main` |
| `ok-32-goto-labels.cx`(32 个标签) | 无输出,产出 `g32.bin` 8192 字节 |
| `refuse-33-goto-labels.cx`(33 个) | `cx: /tmp/owflow/g33.cx:2: unisa safe: more than 32 goto labels for the flow check in f` |
| `ok-64-nested-loops.cx`(64 层) | 无输出,产出 `n64.bin` 8192 字节 |
| `refuse-65-nested-loops.cx`(65 层) | `cx: /tmp/owflow/n65.cx:2: unisa safe: loops or switches nested deeper than 64 for the flow check in main` |

三条都是**带函数名与位置**的拒绝,且边界正好在 64/32/64,与 `cx/cx0/plan.cx:674-681` 的
`CXF_V 64` / `CXF_L 32` / `CXF_D 64` 一致。

## 与 pkg-gaps 原文的逐项对照

| pkg-gaps 原文 | 实测 |
| --- | --- |
| 第 65 个变量起不跟踪 | 否,拒绝: `more than 64 released names` |
| 第 33 个 goto 标签起忽略汇合 | 否,拒绝: `more than 32 goto labels` |
| goto 只跑 2 遍非不动点 | 否,源里是 `CXF_GP 64`,文案 `goto labels not stable after 64 walks`(本轮未造不稳定夹具,只核常量与文案) |
| 循环嵌套超 64 层跳过循环体 | 否,拒绝: `loops or switches nested deeper than 64` |

## 需要 cc 决定的事

1. `build/pkg-gaps.tsv:142` 应改状态。它写的四条里三条已实测为具名拒绝,第四条(2 遍非不动点)源里是 64 遍。
   建议改 `FIXED-IN-0.0.78`,或至少把描述改成「已具名拒绝;2 遍非不动点说法与源不符」。**我不改这行**,等 cc 定。
2. 这四条上限本身仍值得进水位表(它们是**正确性**上限而不是容量上限:超了会拒绝用户的合法程序)。
   与 `CXM_MAXU`/`CXM_SYMS` 的区别是:那两个是产品自身的静态数组容量,这四个是用户代码的形状上限。

## 夹具位置

本目录六个 `.cx`,直接 `sh ./unisacc.com F.cx -o OUT` 可复跑。
