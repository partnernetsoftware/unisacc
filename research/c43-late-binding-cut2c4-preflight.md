# 2c4 callback（kind=4）落地前核对（历史：先于 b83cb995 落地写就，2026-10-22 补归档）

只读 `research/c43-late-binding-cut2c4-step0.md` §2/§3，对照 tip `22e46738` 的 `exec/c/librarylatebind.h`、`librarynative.h`、`libunisacc.c`。未改产品代码，未提交。行号为本次读取。

## 1. `US_LATE_CLASSES` 实际末值

宏本身（`librarylatebind.h:273–276`）只展开 kind 0（函数，owner 1）和 kind 1（数据，owner 4）。静态表在宏展开之后另写了 kind 2（typed）和 kind 3（callsite），都是 owner 1（`:281–283`）。`US_LATE_EXTRA_CLASSES` 默认为空。表上已占用的最大 kind 是 **3**，**kind 4 空着**。落地应沿 2/3 的写法在宏外追加一行，不要把 4 写进宏、也不要按宏的末值 1 去取下一个号。

## 2. verify 函数指针的接入点

`UsLateClass.verify` 的类型是 `UsLateVerify`（`:267`）。唯一调用点是 `us_late_provider`（`:338`）：`d->verify(...)`，返回值不是 2 就直接返回，返回 2 才落到后面的 `dlsym` 兜底。kind 2/3 的 verify 已是 `us_late_typed_provider` / `us_late_callsite_provider`，prepare 是同一行上的另一列。callback 的图校验应做成 kind 4 这一行的 verify，从这里进入；不要写进 owner1 的通用分支，也不要改 `libraryrelocate.h`。

## 3. 闭包释放统一到哪一处

**没有统一到 `us_late_release`。** 该函数（`:53`）只释放 contract、selection 和 phase 缓冲；`us_compile` 在 `:702` 调用它，闭包不在其中。typed 帧由 `us_late_typed_release`（`:134`，内部 `us_ffi_closure_drop`）释放；callsite 的 `us_late_callsite_release`（`:205`）只是它的别名。两者都挂在 `plan->release_attachment` 上，由 `us_native_plan_free`（`librarynative.h:29`）经 `us_native_plans_clear` 调用，`us_compile` 在 `:704` 清的是 plan 表。`librarycallables.h:375` 的 plan 闭包和 `libraryexports.h` 的导出闭包仍各自 `ffi_closure_free`。kind 4 若要和 typed/callsite 同一归属，应挂同一个 `release_attachment`，而不是改 `us_late_release`。§3 写的“经 `us_late_release` 释放”与当前代码不符。
