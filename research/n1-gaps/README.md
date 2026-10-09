# N1 产品缺口最小例（cc → cdx，10-08）

来源：用 unisacc.com 编 seed 工具与 zlib 源（N1 自构造）。实测：com = 根目录 unisacc.com（0.0.35 发布版），ref = 当前树参考编译器；`-run` 或两文件编译后运行。

| 例 | 文件 | com | ref | 条款 |
|---|---|---|---|---|
| g1 块内 register | g1_register.c | 拒：`this is not the start of an expression` | rc=0 | 6.7.1（ledger 标 covered，按实测应改 partial） |
| g2 预定义宏 | g2_stdc.c | 打印 `missing` | 打印 `missing` | 6.10.8：`__STDC__`/`__STDC_VERSION__`/`__STDC_HOSTED__` 两路线都未预定义；须两侧同规则同落 |
| g3 返回结构体的调用作 return 表达式 | g3_structret.c | 拒：`struct return expression outside local lvalue` | rc=0 | 6.8.6.4 |
| g4 无原型声明调用 >6 实参 | g4_a.c + g4_b.c | 拒：`not covered: more than 6 arguments` | rc=0 | 6.5.2.2p6 |
| g5 zlib trees.c `static const static_tree_desc static_l_desc = {...}` | 未最小化 | `-DSTDC` 下报 `not covered: incomplete struct`；同轮另见 `sys/_exit.h:13 incomplete array declaration before another definition`（多单元含 zlib 源） | 未测 | 待最小化 |

g4 是 zlib 1.2.12 在缺 `__STDC__` 时走 K&R `OF()` 无原型路径的直接后果；g2 修好后 zlib 不再触发，但 g4 本身仍是缺口。

g4 定位（cc 10-09，云机 Linux x86_64，只读，未改产品）：公开 0.0.36 unisacc.com（baf296dd）复现：`sh unisacc.com research/n1-gaps/g4_a.c -o g4` 在编译期即拒（g4_a.c 只有 `int f8();` 声明与调用、无定义，不是完整程序，只用来显示拒点；完整程序须 g4_a.c + g4_b.c）；参考（main 666bb0bc 同源 build_ref）`ua g4_a.c g4_b.c -o g4 && ./g4` rc=0。以上是公开 0.0.36 的旧行为，不代表当前源码（cdx 5a632535 已改表，待候选验收）。同一调用写成原型 `int f8(int,...×8)` 产品 `-run` rc=0；单文件 `int f8();` + 调用 + 其后带原型定义：参考 `-run` rc=0，公开 0.0.36 仍拒（K&R 定义参考本身报 expected '{'，不作反例）。拒点在 exec/parse2/callcontrol-result.tsv part17：`CL.done1` 比较 `na` 与 6，>6 进 `CL.directmany`，只在 `vsp[VS_TOP]` 非 0（有原型/参数类型栈）时走 `CL.vdone`，否则落 `DEAD.na` → `not covered: more than 6 arguments`。即产品对 >6 实参的栈传参只接在有原型路径上，无原型路径缺默认实参提升后的同一出口（6.5.2.2p6）。该表由 exec/parse2/callcontrol.py 生成（exec/rules.md 第 7 行：cdx 暂存区中，待迁），修片须在生成器侧做，不手改 result.tsv。
