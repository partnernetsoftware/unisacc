# `#pragma unisa use` 进化：符号级导入 + 别名 + 重导出（2026-10-31）

## 现状（archive/reference-c/src/cxmod.c 实测）

- `#pragma unisa use NAME`，`NAME = seg(.seg)*`，`seg = [a-z_][a-z0-9_]*`（模块名，不是符号名）。
- use 导入的是**整个模块的全部公开符号**，不能只导入一个符号，不能给符号起别名，不能重导出。
- 后果（本目标已实际撞上）：
  1. **命名空间冲突**：两个模块都导出 `tx_eqz`，use 两个就「clash exported by both」拒绝（tests/cx-namespace.cx）。
  2. **closure-once 纠缠**：`-funit` 把 use 闭包整份编进每只对象，符号重复。file-local static 是过渡（cli.cx 的 cxc_eqz），不是解。
  3. **不能重导出**：`a use b` 后，a 不能把 b 的符号再暴露给 a 的使用方，调用方被迫「直接 use b」（坑二）。

## Rust 的对应

- `use a::b::c`：路径导入单个符号。
- `use a::b::c as d`：别名。
- `pub use a::b::c`：重导出（re-export），把导入的符号再作为自己的公开面。

## 提案（对齐 Rust，不新增关键字，用 pragma 承载）

### 1. 符号级导入

```
#pragma unisa use text.text::tx_eqz
```

只把 `tx_eqz` 引入当前单元，不再导入 text.text 的其余符号。消解「两个模块同名符号冲突」：use `a::clash` 和 `b::clash` 仍冲突，但 use `a::clash` 和 `b::other` 不冲突。

### 2. 别名

```
#pragma unisa use a::clash as clash_a
#pragma unisa use b::clash as clash_b
```

把两个同名符号分别起别名 `clash_a`/`clash_b`，彻底消解冲突。这是 cli.cx 从 `static cxc_eqz` 过渡到 `tx_eqz` 的干净路径。

### 3. 重导出（module 侧）

```
#pragma unisa use mem.grow
#pragma unisa export mem.grow
```

`export` 把 `use` 进来的模块作为本模块公开面的一部分，使用方 `use 本模块` 就自动看到 mem.grow 的符号。消解「坑二：调用方被迫直接 use 底层模块」。

## 约束（关键）

- **不新增 C 关键字**：`as`、`::`、`export` 都只在 `#pragma unisa ...` 行内出现，C99 其余部分不受影响。gcc/clang 会忽略整行 pragma（这正是 gcc_cx/clang_cx 靠宿主编译器工作的前提）。
- **`::` 只在 pragma 内**：`NAME::sym` 不进入 C 表达式，只是 pragma 行内的路径分隔，spec §3「v1 不为冲突引入 `::` 语法」指的是 C 表达式内，pragma 内可用。
- **两个落点必须一致**：
  1. `archive/reference-c/src/cxmod.c`（编译进 unisacc 的模块解析器）；
  2. `tools/cxcc.cx` 链进的同一份 cxmod（gcc_cx/clang_cx 的展开）。
  两份用同一源，改一处即可，但重编 gcc_cx.com/clang_cx.com 要走 `tools/build-cxcc-ape.cx`。

## 验收

- unisacc：`tests/cx-namespace.cx` 加「别名消解冲突」用例（`use a::clash as ca` + `use b::clash as cb` → 不冲突，ca()/cb() 各返各值）。
- gcc_cx/clang_cx：同一 fixture 用宿主 gcc/clang 编过、运行结果一致。
- 棘轮六项不升；新用例不引入新 libc。

## 与 closure-once 的关系

符号级导入 + 别名**不直接解决** closure-once（那是「每模块只进一只对象」的架构改造），但让「use 同一模块的多个上层单元」可以各取所需、避免不必要的整模块内联，是 closure-once 落地后的自然配套。

## 实现进度（2026-10-31，bdy，在 gcc_cx/clang_cx 宿主链上）

已在三源（权威 `cx/cx0/plan.cx` 的 `CX0_COMPAT_BODY`、投影 `src/cxmod.c`、产品模块 `cx/compiler/cxmod.cx`）落地符号级导入 + 别名，端到端工作：
1. **解析**：`use NAME::sym [as alias]`（cxm_directive + cxm_adduse，新增 cxm_usym/cxm_ualias）。
2. **冲突消解**：cxm_collisions 函数分支加 cxm_imports（符号可见性）+ cxm_aliased（改名）检查，同名函数被别名或未被符号级导入时不冲突。
3. **CXIF 声明改名/过滤**：cxm_text(sym,alias) + cxm_on_rename（token 级改名）+ cxm_renpool + cxm_inc 负值编码；符号级导入时只输出匹配 sym 的原型（cxm_project 记录每个函数原型在 cxm_ift 的边界到 cxm_symb/syme）。
4. **函数体符号改名**：cxm_rewrite 生成 `#define sym alias` 头的改名源码，符号级导入时把未导入的导出函数改成 `ua_sym_<n>` 唯一名避免链接重复；cxm_planprint 输出改名源码路径（cxm_bodysrc）。

验收（gcc_cx 和 clang_cx 都过）：`use a::clash as clash_a` + `use b::clash as clash_b` 返回 3；`use aa::other` + `use bb::clash`（纯符号级导入，无别名）返回 9。`cx0-extract --check` 报 `EXACT`。`tests/cx-namespace.cx` 加 `alias-renames-away-clash` 与 `symbol-import-filters-unused` 两用例。

**已知边界**：
- 一个模块被多个单元用**不同**别名 use 时，cxm_bodysrc 只存一份改名源码（单版本假设），多版本模块编译尚未做。这是罕见场景，需要时再扩。
- 符号级导入的「未导入函数改 ua_sym_* 唯一名」只是让它们不参与链接冲突，函数体仍被编译（未做「只编译目标函数」的瘦身）；这是 closure-once 落地前的折中，等模块只进一只对象后再收紧。
- 传递 use 的符号级过滤不追踪（cxm_imports 对传递 reach 按全导入），即 `a use b::x` + `c use a` 时 c 仍看到 b 的其余符号；重导出（本方案第 3 部分）落地后再补。
- 产品侧 `unisacc.com` 尚未重编（33 对象 whole-o 链接的 `dlopen` 阻塞属 C44-ENTRY-HOSTIMPORT-01），故两个新用例在已装产品上暂 FAIL，重编后 PASS。
