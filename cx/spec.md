# `.cx` 规格

> 对外条款。编号只增不改；废弃条款保留 ID 并标「废弃」。
> 当前状态与路线在 [`prd.md`](prd.md)；语言远景在 [`docs/cx-spec-draft.md`](../docs/cx-spec-draft.md)。未列入本文件的草案句子不在效力范围内。
> 本文件不替代仓库根 [`spec.md`](../spec.md)（那是流／网／编译器条款）。模块化与内存安全叙事归 Paper D。

RFC 2119：本文「必须 / 不得 / 应当 / 可以」= MUST / MUST NOT / SHOULD / MAY。

## 0. 效力与编号

| 前缀 | 域 |
|---|---|
| `CX-L` | 语言身份 |
| `CX-G` | pragma 文法 |
| `CX-M` | 模块 |
| `CX-U` | 导入 |
| `CX-N` | 命名空间 |
| `CX-I` | 接口文本 CXIF |
| `CX-P` | 包清单 |
| `CX-Y` | 分层 |
| `CX-A` | L0 合同 |
| `CX-O` | 运行期所有权 |
| `CX-R` | 棘轮 |
| `CX-W` | 未生效（保留，不在效力范围） |

**[CX-L-0]** 条款分两栏：正文是规范；「未生效」节不得被实现或测试写成已交付。运行期句柄检查不得称为编译期所有权。

### 术语

| 词 | 定义 |
|---|---|
| 模块 | 一个 `.cx` 文件，经 `#pragma unisa module` 声明，或作为使用方单元 |
| 公开符号 | 该文件非 `static` 的函数与类型名；`cxpack.tsv` 匹配行可再裁成子集 |
| L0 | 目录 `cx/sys/`、`cx/mem/`、`cx/text/`，唯一可对接 libc/POSIX |
| 上层 | `cx/` 其余目录及默认的 `src/`、`tools/`、`tests/`、`apps/` |
| 句柄 | 可复制的整型或结构值，由运行期核世代／边界；不是线性类型 |

---

## 1. 语言身份 [CX-L]

| ID | 条款 |
|---|---|
| **CX-L-1** | `.cx` 是 POSIX C99 的受检查子集与模块配置，不是新语言。不得增加关键字、运算符、声明符或表达式语法。 |
| **CX-L-2** | 模块、导入、公开集必须用 `#pragma unisa`、旁置清单或编译配置表达。宿主 gcc/clang 忽略未知 pragma；展开发生在驱动内，不得依赖宿主认识 `unisa`。 |
| **CX-L-3** | 普通 `.c` 的 C99 语义与六目标字节不得因 `.cx` 模块配置而改变。 |
| **CX-L-4** | 通过模块检查不等于内存安全资格。未证明部分必须降为运行期检查或拒绝，不得发「已证明」收据。 |

---

## 2. pragma 文法 [CX-G]

词法：`NAME = seg(.seg)*`，`seg = [a-z_][a-z0-9_]*`。`::` 与 `as` 只出现在 pragma 行内，不得进入 C 表达式。

| ID | 条款 |
|---|---|
| **CX-G-1** | 合法指令目前只有 `module` 与 `use`。未知指令必须拒绝，诊断含 `unknown unisa pragma`。 |
| **CX-G-2** | `#pragma unisa` 必须出现在该翻译单元任何声明之前，否则拒绝 `unisa pragma after first declaration`。 |
| **CX-G-3** | `#pragma unisa` 不得写在 `#if` 条件为真的分支内，否则拒绝 `unisa pragma inside conditional`。 |
| **CX-G-4** | 解析权威源是 `cx/cx0/plan.cx`（`CX0_COMPAT_BODY` 区）。投影 `src/cxmod.c` 必须与该区逐字一致；产品模块 `cx/compiler/cxmod.cx` 必须实现同一文法。改文法先改 `plan.cx`。 |

---

## 3. 模块 [CX-M]

```
#pragma unisa module NAME
```

| ID | 条款 |
|---|---|
| **CX-M-1** | `NAME` 必须等于文件名 stem（去掉目录与 `.cx`），否则拒绝 `module NAME is not the file stem STEM`。 |
| **CX-M-2** | 库模块不得定义 `main`。使用方单元可以没有 `module` 行。 |
| **CX-M-3** | 模块图必须无环。`a→b→a` 与自循环必须拒绝，诊断含 `module cycle`。 |
| **CX-M-4** | 默认私有：未公开的 `static` 名字不得出现在 CXIF。无匹配清单行时，兼容行为是导出全部非 `static` 函数。 |

---

## 4. 导入 [CX-U]

```
#pragma unisa use NAME
#pragma unisa use NAME::sym
#pragma unisa use NAME::sym as alias
```

| ID | 条款 |
|---|---|
| **CX-U-1** | 整模块导入把该模块当前公开集引入本单元。 |
| **CX-U-2** | `NAME::sym` 只引入 `sym`，不得引入其余公开符号。 |
| **CX-U-3** | `as alias` 把引入的名字改成 `alias`。C 表达式里写 `alias`，不得写原名。 |
| **CX-U-4** | `NAME` 的点号是子目录。解析起点是使用方文件所在目录。跨目录消费靠该目录下的目录项（现为指向目标模块目录的符号链接）。 |
| **CX-U-5** | 间接可见不得给原型：A `use` mid、mid `use` text.fmt，A 调用 `tx_snprintf` 必须视为未声明。用到哪个模块就必须直接 `use` 那个模块。 |
| **CX-U-6** | 现行编译把被 `use` 的函数体编进当前 `-funit` 对象。每模块只进一只对象、消费者只留未定义引用，是已裁定未落地的构造合同，见 **CX-W-1**。 |

验收锁定：`tests/cx-namespace.cx`（含别名消解与符号级过滤）。

---

## 5. 命名空间 [CX-N]

| ID | 条款 |
|---|---|
| **CX-N-1** | 两个模块向同一单元导出同一函数名必须拒绝，诊断含 `exported by both`，除非符号级过滤或别名使可见名不再冲突。 |
| **CX-N-2** | 单元不得定义已被其 `use` 集合导出的函数名，诊断含 `is exported by`。 |
| **CX-N-3** | 两个模块向同一单元导出同名类型必须拒绝，诊断含 `type` 与两个模块名。 |

---

## 6. CXIF v0 [CX-I]

`-cx-interface FILE.cx` 的文本必须是：

1. 首行 `/* CXIF v0 module STEM src-sha256 HEX16 -- generated, do not edit */`
2. `#ifndef` / `#define` 守卫 `CXIF_STEM_HEX16`
3. 类型（`struct` / `union` / `enum` / `typedef`），再函数原型，各以 `;` 结束
4. 末行 `#endif`
5. 不得含 `static`，不得含函数体

| ID | 条款 |
|---|---|
| **CX-I-1** | 直接 `use` 给完整接口；传递 `use` 只给类型部分（banner 含 `types`）。符号级导入只输出匹配 `sym` 的原型，别名写在 banner。 |
| **CX-I-2** | v0 没有效果、布局、所有权字段。不得把 v0 文本称为结构化 ABI。 |
| **CX-I-3** | 缺 `#endif`、带函数体、带 `static`、版本不是 v0，检查必须退出非 0。 |

验收：`tests/cxif.cx -- --fixtures`。

---

## 7. 包清单 [CX-P]

一个目录一份 `cxpack.tsv`。列：`模块名<TAB>路径<TAB>公开函数`。

| ID | 条款 |
|---|---|
| **CX-P-1** | 路径必须等于模块名把 `.` 换成 `/` 再加 `.cx`。`std` / `std.cx` 是前奏，不进表。 |
| **CX-P-2** | 公开函数按名字排序，必须是该文件的非 `static` 定义。把 `static` 写入公开列必须拒绝。 |
| **CX-P-3** | 同目录有匹配行时，CXIF 只写出公开列；无匹配行保留全部非 `static` 的兼容行为。超限或读取失败必须拒绝，不得退回全部公开。 |

验收：`tests/cxpack.cx -- --fixtures`。

---

## 8. 分层 [CX-Y]

**层就是目录。**

| ID | 条款 |
|---|---|
| **CX-Y-1** | 只有 L0 与白名单精确例外可以 `#include` 对接 libc/POSIX。白名单：`tests/cxinclude-boundary.tsv`。上层必须用隐式前导 + `#pragma unisa use`。 |
| **CX-Y-2** | 上层不得直接调用 libc/POSIX（`printf`、`malloc`、`strcmp`、`open` 等）。必须走 L0 的 `sy_*`、`mm_*`、`tx_*`。谁可裸调：`tests/cxraw-layers.tsv`。 |
| **CX-Y-3** | 上层不得写平台 `#if`（`__linux__`、`_WIN32`、`__APPLE__`、`__x86_64__` 等），也不得写 `__syscallN`。平台差异只放 L0 的 `_os` 静态帮手或表。导出函数不得包在平台 `#if` 里。 |
| **CX-Y-4** | 跨模块错误码只走返回值。不得依赖全局 `errno` 在模块之间传递。L0 失败必须返回 `-errno`（或已文档化的负码）；空指针／空路径／负长度应当 `-EINVAL`；失败不得留下半开 fd。 |
| **CX-Y-5** | `cx/crypto/` 是上层：纯计算、调用方提供缓冲，不得 `#include` 宿主密码库。加速路径若需要系统接口，必须放进 `cx/sys/`，导出仍无条件编译。 |

`#include` 在模块 `.cx` 里目前是棘轮 lint，不是前端硬拒绝（硬拒绝见 **CX-W-2**）。

---

## 9. L0 合同 [CX-A]

L0 是产品的标准库边界，对应 Rust `std::fs` / `Vec` / `str` 的职责，而不是「允许随便写 C」。

| ID | 条款 |
|---|---|
| **CX-A-1** | `cx/sys` 对接文件、进程、环境、时间、网络、映射、装载。公开接口无平台分支。 |
| **CX-A-2** | `cx/mem`：`heap` 是带长度的分配；`bytes` 带界 copy/move/fill/compare；`arena` 分组释放；`grow` 是 `Vec<u8>`；`slice` 是 `&[u8]` / `Option<&[u8]>`。 |
| **CX-A-3** | `cx/text`：带界 z-string／span、`text.build`（`String`）、deflate、格式化、数字。新上层 API 应当优先 `(ptr,len)` 或 `ms_slice`，而不是无界 `char*`。 |
| **CX-A-4** | 前导 `cx_*`（`cx_read`、`cx_write`、`cx_run*` 等）原地认定为 L0 接口，棘轮不计。新代码应当优先 `sy_*` / `mm_*` / `tx_*`。 |
| **CX-A-5** | 逐函数表在各目录 README。本规格管合同形状，不把每个 `sy_*` 钉成条款。 |

---

## 10. 运行期所有权 [CX-O]

本节是运行期合同。**不是** Rust 编译期借用检查。

| ID | 条款 |
|---|---|
| **CX-O-1** | `mg_buf` 的调用方拥有结构体；数据块来自 `mem.heap`。失败时缓冲必须不变。 |
| **CX-O-2** | `mg_free` 释放后清零。对已清零缓冲再 `mg_free` 必须是空操作。 |
| **CX-O-3** | `mg_move(dst, src)` 是 Rust 赋值：先释放 `dst` 上仍拥有的块（若有），再把块从 `src` 转到 `dst`，`src` 清零。`dst == src` 是空操作。之后 `mg_free(src)` 必须是空操作。 |
| **CX-O-4** | `mg_take` 把块交给调用方，缓冲清零。调用方用 `mm_free` 释放。 |
| **CX-O-5** | `ms_of` / `ms_mut_of` 借用 `mg_buf` 的当前内容，不拥有。切片不得在所有者 `mg_free` / `mg_move` / `mg_take` 之后使用——现行实现不在编译期强制这一点。 |
| **CX-O-6** | `cx/core/memory.cx` 的 `cm_*` 句柄：世代、共享／独占借用、越界拒绝。句柄可伪造、可复制；这是过期句柄隔离，不是线性类型或机密性。 |
| **CX-O-7** | `cx/owned.cx` 是冻结的 i64 原型，不得扩展为通用所有权系统。 |

验收：`tools/check-cx-grow.cx`、`tools/check-cx-slice.cx`、`cx/check_memory.cx`。

---

## 11. 棘轮 [CX-R]

只降不升；新文件计数必须为 0。不得靠删断言、缩覆盖或放宽时限造绿。

| ID | 闸 | 计什么 |
|---|---|---|
| **CX-R-1** | `tests/cxinclude.cx` | 模块 `.cx` 的 `#include` 行 |
| **CX-R-2** | `tests/cxraw.cx` | 上层直接 libc/POSIX 调用 |
| **CX-R-3** | `tests/cxapis.cx` | `strcpy` / `strcat` / `sprintf` / `gets` |
| **CX-R-4** | `tests/cxtrunc.cx` | 丢弃返回值且长度可随输入变化的截断写入 |
| **CX-R-5** | `tests/cxlibc.cx` | 上层 libc+host 调用（全名单） |
| **CX-R-6** | `tests/cxplatform.cx` | 上层平台 `#if` / `__syscallN` |
| **CX-R-7** | `tests/srcshape.cx` | `src/*.cx` 行数、最长函数、if 数、总行只降不升 |

七闸一次跑：`tools/ratchets.cx`。

---

## 12. 未生效 [CX-W]

下列 ID 已分配，**不在效力范围**。实现不得把它们标 PASS。

| ID | 内容 | 状态 |
|---|---|---|
| **CX-W-1** | `use` 只留引用；每个模块只编一只对象，链接共享。不定 `STB_WEAK`。 | 已裁定，未落地 |
| **CX-W-2** | `.cx` 中 `#include` 为前端具名拒绝（CX3） | 未落地；现为棘轮 |
| **CX-W-3** | `export`（Rust `pub use`） | 未落地；现禁止间接原型已足够 |
| **CX-W-4** | 编译期所有权／借用检查（草案 §5） | 未落地 |
| **CX-W-5** | 层感知前导：L0 才含 libc 声明，上层前导只有 L0 接口 | 未落地 |
| **CX-W-6** | 包根解析 `use`，取消跨目录软链 | 未落地 |
| **CX-W-7** | `gcc_cx` / `clang_cx` 的 `-run` | 未落地 |
| **CX-W-8** | CXIF 效果／布局／所有权字段（v1 schema） | 未落地 |

宿主驱动 `gcc_cx.com` / `clang_cx.com` 的产物是普通 C，不授予已检查身份。
