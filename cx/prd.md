# cx — 结构化主线：pragma 扩展、底层/上层两棵树

> 本篇是 `cx/` 树的总索引，不是第二份规格。对外条款在 [`spec.md`](spec.md)（编号只增不改）。pragma 扩展说明在 §2.1，分层功能树在 §13.1；这两节是导航。语言与模块是 §1–§7（`CX-L`…`CX-P`）；目录架构是 §13（`CX-S-1`…`CX-S-10`）；分层许可、L0 形状、运行期所有权、棘轮是 §8–§11（`CX-Y` `CX-A` `CX-O` `CX-R`）。未落地的不在效力范围（§12 `CX-W`）。模块化与内存安全叙事归 **Paper D**（[`research/paper-d-intent.md`](../research/paper-d-intent.md)），不塞进 Paper A。现行产品阅读见 [`docs/cx/prd.md`](../docs/cx/prd.md)。目录归属总览见 [`ARCHITECTURE.md`](../ARCHITECTURE.md)。

## 一、`#pragma unisa` 扩展

说明与语法树在 [`spec.md` §2.1](spec.md)。条款是 **CX-L-1** **CX-L-2**、**CX-G-1**…**CX-G-4**、**CX-M-***、**CX-U-***、**CX-N-***。重导出 `export` 未生效（**CX-W-3**）。过程记录在 [`plans/cx-use-alias-reexport.md`](../plans/cx-use-alias-reexport.md)。

## 二、目录架构

规范在 [`spec.md` §13](spec.md)。功能树在 [`§13.1`](spec.md)。下表只作导航，冲突时以条款为准。

| 层 | 路径 | 条款 | 索引 |
|---|---|---|---|
| L0 | [`cx/sys/`](sys/README.md) [`cx/mem/`](mem/README.md) [`cx/text/`](text/README.md) | **CX-S-1** **CX-A-1**…**CX-A-5** **CX-Y-4** | 文件/进程/时间/环境/映射/装载；带长度的堆与字节；带界文本。模块名单在各 README，不钉进条款 |
| L1 | [`cx/core/`](core/memory.md) [`cx/runtime/`](runtime/README.md) | **CX-S-2** **CX-O-1**…**CX-O-6** | 执行器、包装载/解码、路由、流输出 |
| L2 | [`cx/driver/`](driver) [`cx/compiler/`](compiler) `src/`（引导例外除外） | **CX-S-3** **CX-S-7** **CX-S-8** | CLI、tape、绑定、`cxmod`；`winprocess` 留在 compiler；名表只经生成器 |
| L3 | `tools/` `tests/` `apps/` | **CX-S-4** **CX-S-10** | 构造与验证。`apps/csih/`、`ujs/` 不由本树条款授权改写 |
| 解析 / 包图 / 密码 | [`cx/cx0/`](cx0/README.md) [`cx/workspace/`](workspace/README.md) [`cx/crypto/`](crypto) | **CX-S-6** **CX-S-7** **CX-Y-5** **CX-G-4** | cx0 不是 L0；crypto 是上层纯计算 |

许可不在目录散文里开：裸 libc 只看 `tests/cxraw-layers.tsv`，`#include` 只看 `tests/cxinclude-boundary.tsv`（**CX-S-5** **CX-Y-1** **CX-Y-2**）。扫描面见 **CX-S-9**。棘轮 ID 见 **CX-R-1**…**CX-R-7**，一次跑 `tools/ratchets.cx`。

分层铁律（全文在 spec，这里只列 ID）：

| 铁律 | 条款 |
|---|---|
| 只有 L0 与白名单可以 `#include` | **CX-Y-1** **CX-S-5** |
| 上层不直调 libc/POSIX，走 `sy_*` / `mm_*` / `tx_*` | **CX-Y-2** |
| 上层不写平台 `#if`，不写 `__syscallN` | **CX-Y-3** |
| 跨模块错误只走返回值 `-errno` | **CX-Y-4** |
| 间接可见函数无原型 | **CX-U-5** |
| 名表手改无效 | **CX-S-8** |

设计过程与已落地记录在 [`plans/cx-layers.md`](../plans/cx-layers.md)，不是条款。

## 三、当前状态与边界

- 已装产品仍用原驱动，`cx` 结构化入口（`cxmod`/CXIF v0）是迁移桥；编译期所有权（Rust 式 borrow checker）尚未实现，运行期句柄检查 ≠ 编译期所有权。
- `src/cxmod.c` 是活的生成兼容输入（投影），`exec/c` 执行核仍服务新链；旧 C 只在生产/构造/测试全不读它后才归档。
- 名表手改无效，合同是 **CX-S-8**。`elfname` / `x86*` / `k*` / `l*` 等在生成器覆盖范围内。

## 四、扩展约定（分析，不是条款）

2026-10-31。不改 [`spec.md`](spec.md)。下面不是 **CX-L** / **CX-G** 的改写，也不把「能跟 Rust、Bun、Node 竞争」写成已生效目标。

用 `#pragma` 扩 C、而且做成了的，是规范，不是某个单仓语言。OpenMP（`#pragma omp`）和 OpenACC（`#pragma acc`）是存在证明：C 标准要求不认识的 pragma 忽略，所以宿主不用改。Cilk Plus 的 `#pragma simd` 已死，能力进了 OpenMP。GitHub 上最近的安全扩 C 是 [checkedc/checkedc](https://github.com/checkedc/checkedc)（2026-10-30 仍在动）；它用 `_Ptr`、`_Array_ptr`，不走 pragma，要改 clang。没找到另一个还在维护、用宿主忽略的 pragma 做模块加所有权的公开语言。

能跟宿主共存的约定只有三条：

| 约定 | 宿主行为 | 适合挂什么 | 本树 |
|---|---|---|---|
| `#pragma` / `_Pragma` | 不认识就忽略 | 一行一个指令：`module`、`use` | 现行路。契约在文件里，`gcc_cx` / `clang_cx` 不改宿主 |
| C23 `[[unisa::...]]`，以及 gcc `__attribute__` | 不认识的属性忽略 | 贴在声明上：所有权、借用、生命周期 | 唯一值得以后加的第二条。pragma 贴不住单个声明。现在不加 |
| 旁路文件 | 完全看不见 | 层号、生成表、不属于语法的事实 | 已在用：本索引、`spec.md`、tsv。不能代替文件里的 `use`，两份会漂 |

不能当本树约定的：

- 新关键字：宿主直接编译失败，除非改编译器或先剥掉再交 gcc。`gcc_cx` / `clang_cx` 的前提就没了。
- 宏库：宿主能编，加不出模块、所有权和链接合同。
- 注释里的契约：宿主忽略，但注释不进语法，工具和人都会漏。

决定：语言扩展继续只走 pragma。声明级事实将来可以加属性，现在不加，也不为此改 spec。旁路只放层号、表和条款，不放 `use`。选哪条约定，都不构成跟 Rust、Bun、Node 竞争的资格。
