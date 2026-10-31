# cx — 结构化主线：pragma 扩展、底层/上层两棵树

> 本篇是 `cx/` 树的总索引。模块化与内存安全叙事归 **Paper D**（[`research/paper-d-intent.md`](../research/paper-d-intent.md)），不塞进 Paper A。现行产品阅读见 [`docs/cx/prd.md`](../docs/cx/prd.md)。目录归属总览见 [`ARCHITECTURE.md`](../ARCHITECTURE.md)。

## 一、`#pragma unisa` 扩展

`.cx` 不新增 C 关键字。模块声明、导入和符号级操作都压在 `#pragma unisa` 这一行内，宿主 gcc/clang 会忽略整行 pragma（这正是 `gcc_cx.com`/`clang_cx.com` 靠宿主编译器工作的前提）。解析器是同一份，三源一致（见 §1.4）。

### 1.1 两个指令

| 指令 | 语法 | 语义 |
|---|---|---|
| `module` | `#pragma unisa module NAME` | 声明本文件为库模块；`NAME` 必须等于文件名 stem，否则拒绝 |
| `use` | `#pragma unisa use NAME [::sym [as alias]]` | 导入模块；`NAME = seg(.seg)*`，点号=子目录，在使用者同目录解析 |

### 1.2 `use` 的三个粒度

1. **整模块导入**：`use text.text` — 导入该模块全部公开符号。
2. **符号级导入**：`use a::other` — 只导入 `other`，不导入 a 的其余符号。消解「只想用 a 的一个函数、却撞上 a 与 b 的同名符号」场景。
3. **符号级导入 + 别名**：`use a::clash as ca` — 导入 `clash` 但改名为 `ca`，把同名符号彻底分开（对齐 Rust `use a::b::c as d`）。

### 1.3 约束与校验

- `::`、`as` 只在 pragma 行内出现，不进入 C 表达式，不新增 C 关键字。
- `use` 必须在所有声明之前（否则 `unisa pragma after first declaration`），不能在 `#if` 条件内（`unisa pragma inside conditional`）。
- **命名空间冲突拒绝**：两个模块导出同名符号报 `clash exported by both a and b`，除非被符号级过滤或别名消解；模块循环（`a→b→a` 或自循环）硬拒绝 `module cycle`。锁定在 [`tests/cx-namespace.cx`](../tests/cx-namespace.cx)（8 用例）。
- **CXIF v0 生成**：direct use 给 full 接口，transitive use 只给 type part；符号级导入只输出匹配 sym 的原型。

### 1.4 三源一致性

解析逻辑单一权威源 `cx/cx0/plan.cx`（`CX0_COMPAT_BODY_BEGIN/END` 之间），经 `tools/cx0-extract.cx` 投影成 `src/cxmod.c`（gcc_cx/clang_cx 宿主链用），并独立移植到产品模块 `cx/compiler/cxmod.cx`（unisacc.com 链用）。`cx0-extract -- --check` 保证投影逐字一致（`EXACT`）。**改 use 语法先改 plan.cx，再重新投影；cxmod.cx 单独移植同一逻辑。** 详见 [`plans/cx-use-alias-reexport.md`](../plans/cx-use-alias-reexport.md)。

### 1.5 尚未实现

**重导出 `export`**（Rust `pub use`）：`a use b` + `a export b`，让使用方 `use a` 自动看到 b 的符号。已登记在方案第三部分，未落地。

## 二、底层 / 上层两棵树

`cx/` 按「离 libc 的距离」分两层，上层不得直接调 libc/POSIX，只能经底层 L0 接口。这条边界由 `tests/cxraw.cx`（裸 libc 调用）与 `tests/cxinclude.cx`（上层 `#include`）两个棘轮强制：**只降不升，新文件必须为 0**。

### 2.1 底层（L0 宿主对接，唯一可碰 libc/POSIX）

| 目录 | 模块 | 对接内容 | 对应 Rust/Node |
|---|---|---|---|
| [`cx/sys/`](sys/README.md) | `sys.env` `sys.fs` `sys.proc` `sys.time` `sys.net` `sys.posix` `sys.mem` `sys.dl` `sys.sha` `sys.guard` `sys.ffi` `sys.io` `sys.fd` `sys.poll` `sys.tty` `sys.sig` | 文件/目录、进程、时间、环境、映射内存、动态装载、摘要、信号、poll/tty；一律返回 `-errno` | `std::fs` `std::process` `std::env` |
| [`cx/mem/`](mem/README.md) | `mem.heap` `mem.bytes` `mem.arena` `mem.grow` `mem.slice` | malloc/calloc 显式长度封装、带界 copy/move/fill/compare、句柄式分组分配、可增长缓冲区、借用切片 | `Vec<u8>` `&[u8]` `Bump` |
| [`cx/text/`](text/README.md) | `text.text` `text.build` `text.deflate` `text.fmt` `text.num` | 带界 z-string/span、字符串构造器、deflate 压缩、格式化、数字 | `str` `String` `Buffer` `zlib` |

### 2.2 上层（L1/L2，经 L0 消费，不碰 libc）

| 目录 | 职责 | 索引 |
|---|---|---|
| [`cx/core/`](core/memory.md) | 执行器：ALU、栈、稀疏表、过渡、执行核、输出；按实例拥有的缓冲区、move/drop | `core/memory.md` |
| [`cx/compiler/`](compiler) | 编译器各阶段：`cxmod`（模块解析）、`tapegate`/`tapelink`（tape）、`bindstage`/`bindgate` 等（绑定）、`fwd*`（前向）、`streamname`/`netname`（七流十八网名表） | — |
| [`cx/runtime/`](runtime/README.md) | 运行时：包装载/解码/生命周期、模型装载、路由（`routestep`/`routespan`/`routetime`）、进程（`process`/`winprocess`）、流输出 | `runtime/README.md` |
| [`cx/driver/`](driver) | 驱动：`cli`/`args`/`input`/`output`/`path`/`env`/`compat`/`route`/`archive`/`deps` | — |
| [`cx/cx0/`](cx0/README.md) | 权威 CX 解析器 + 经典 C 兼容投影 + 不可变单元束 | `cx0/README.md` |
| [`cx/workspace/`](workspace/README.md) | 包图、构建、严格续构、声明资源目录闭包 | `workspace/README.md` |

### 2.3 分层铁律

- **只有底层能 `#include`** 对接 libc/POSIX 头；上层用隐式前导 + `#pragma unisa use`。名单在 `tests/cxinclude-boundary.tsv`。
- **上层不直调 libc**：printf/malloc/strcmp/open 等一律改走 L0 的 `sy_*`、`mm_*`、`tx_*`。
- **上层不写平台 `#if`**（`__linux__`/`_WIN32`/`__APPLE__` 等），也不写 `__syscallN`；平台差异只放 L0 的 `_os` 静态帮手或表里。
- **errno 每编译单元一份**：跨模块错误码只走返回值（`-errno`），不依赖全局 errno。
- **间接可见函数无原型**：A use mid、mid use text.fmt，A 调 tx_snprintf 会错；约定用到哪个模块的函数就直接 use 那个模块。

详见 [`plans/cx-layers.md`](../plans/cx-layers.md)（分层设计与 17 条已落地记录）。

## 三、当前状态与边界

- 已装产品仍用原驱动，`cx` 结构化入口（`cxmod`/CXIF v0）是迁移桥；编译期所有权（Rust 式 borrow checker）尚未实现，运行期句柄检查 ≠ 编译期所有权。
- `src/cxmod.c` 是活的生成兼容输入（投影），`exec/c` 执行核仍服务新链；旧 C 只在生产/构造/测试全不读它后才归档。
- `cx/compiler/`、`cx/runtime/` 下有大量由 `tools/nametab-gen.cx` 从 `tools/nametabs.tsv` 生成的 `*name.cx`/`*.cx` 名表（`elfname`/`x86*`/`k*`/`l*` 等），手改会被生成器覆盖；改它们要走「事实表 → 生成器（带 `--check`）→ 表」。
