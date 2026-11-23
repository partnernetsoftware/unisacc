# 0.0.93 K3 先红夹具与两架构 `.kernel` SHA256 SAME 门设计

目标: `K3` 将 `exec/c/asm/layout.h` 与 `exec/c/asm/actions.h` 收口到单张事实表 `exec/facts/core-abi.tsv` 经 `tools/core-abi-gen.cx` 生成, 并在三处 `CoreModel` 定义(`exec/c/core.h`、`cx/sys/kernel.cx`、`cx/core/execute.cx`)末尾追加预留打包表指针 `const void *packed;` (`CM_PACKED=152`, `CM_SIZE: 152 -> 160`, 既有偏移 `0..144` 全不变), 硬门是**两架构 `.kernel` SHA256 逐字节 `SAME`**。

## 一、`core-abi-gen --check` 漂移与三处同步夹具(`tools/check-core-abi-gen.cx`)

1. 正例(`positive-sync-and-exact`): 事实表、派生双头与三处 `CoreModel` 一致 -> `rc=0`, `CORE_ABI_GEN OK`。
2. 漂移先红(`red-layout-drift` / `red-actions-drift`): 篡改 `layout.h` 或 `actions.h` 任一常量 -> `rc=1`, `CORE_ABI_DRIFT`。
3. 三处单改失步先红(`red-sync-core-h-only` / `red-sync-kernel-cx-only` / `red-sync-execute-cx-only`): 只改三处之一不改另两处 -> `rc=1`, `CORE_ABI_SYNC_MISMATCH` 且点出具体文件。
4. 变异全杀(`mut-kill-skip-core-h` / `mut-kill-skip-kernel-cx` / `mut-kill-skip-execute-cx`): 编译期关掉单处同步检查后, 对应失步夹具逃逸(`rc=0`), 证明三处检查均为真实有效断言。

## 二、两架构 `.kernel` SHA256 逐字节 `SAME` 门

- 来源: `tools/kernel-build.cx ROOT ABS_OUT link-x86_64` 与 `link-arm64` 产出的 `x86_64-kernel.sha256` 与 `arm64-kernel.sha256`(本机 `lnx/x86_64` 经产品自带交叉汇编器即可产出两架构静态字节)。
- 硬门: 基点(`f7bf2e106e`)与 K3 两轮 `kernel-build` 产出的 `x86_64-kernel.sha256` 相等 且 `arm64-kernel.sha256` 相等; 负控先红为临时篡改汇编引用的偏移(如 `CM_RET_OK 144 -> 148`)重跑 `link-*` 必报 `DIFF`。
- 口径区分: 两架构 `.kernel` 静态字节 `SAME` 均在本机实测; 运行时原生执行 `lnx/x86_64` 由 `UNISA_KERNEL` 实跑 `tools/check-core-run-diff.cx`, `arm64` 运行时原生执行标 `UNMEASURED`(待 CI 原生片实跑)。
