# 全仓 `signal()` 与 `atexit()` 清理路径审计与 `fsx_scratch` 库层加固 (2026-11-23)

## 1. 背景与根因

`cx/sys/sig.cx` (L7-L10) 明确说明: 隐式 prelude 提供的 `signal()` / `sigaction()` 仅填写进程内表供 `raise()` 读取, **不会发起内核 `rt_sigaction` 系统调用**; 因此外部进程发来的 `SIGTERM` / `SIGINT` 不会触发任何经 prelude `signal()` 注册的处理函数。同理, 仅注册 `atexit()` / `sy_atexit()` 而不注册内核信号处理函数的工具, 在收到 `SIGTERM` / `SIGINT` 时会直接被内核默认终止, 不会执行清理回调。

## 2. 全仓清理类 `signal` / `atexit` 审计分类

基于全仓 `git grep` (`tools/`、`tests/`、`ext/`、`cx/`), 排除纯语法/单测夹具后, 共梳理出 20 处实际清理/中断位置:

### A 类: 高风险 (误用 prelude `signal()` 或管理共享树/长作业临时目录, 须在被杀时清理)

| 文件与行号 | 当前机制 | 管理对象与被杀风险 | 状态与建议 |
| --- | --- | --- | --- |
| `tools/prerun-drive.cx:1510-1511` | prelude `signal(SIGTERM/SIGINT, pd_lane_interrupt)` | **直接中招**: 预跑车道运行期间以为已接管 `SIGTERM`/`SIGINT` 做清理, 实际内核从未收到 handler, 被杀时 `pd_lane_interrupt` 不执行, 子作业与窗口状态可能残留 | 建议改用 `sys.sig` (`sy_sig_handle`), 归属预跑/发布车道排期 |
| `tools/precommit.cx:2993` | 仅 `atexit(pc_iso_cleanup)` | 隔离树 `${TMPDIR:-/tmp}/precommit-tree.XXXXXX` 被 `SIGTERM` 杀时不清理, 直接泄漏至 `/tmp` | `agy-flow` 整改中 |
| `tools/commit-own.cx:164` | 仅 `atexit(co_scratch_clean)` | 提交过程临时工作目录, 被 `SIGTERM` 杀时不清理 | 建议接入 `fsx_scratch` 或 `sys.sig` |
| `ext/fsx/fs.cx:191` (`fsx_scratch`) | 原仅 `sy_atexit(fsx_sc_atexit)` | 所有调用 `fsx_scratch` 的工具/检查 (`check-old-product-material`、`check-affected-oldproduct`、`check-opt-frame4-material`、`check-ext-fsx` 等) 原在 `SIGTERM`/`SIGINT` 下不清理且 `fork()` 子进程退出还会误删父进程目录 | **本次已单点根修**: 接入 `sy_sig_handle(15/2)` + `owner_pid` 隔离 + 启动时自动清除超龄 (>30min) 且无存活进程的同前缀旧目录 |

### B 类: 中/低风险 (仅用 `atexit` / `sy_atexit` 清理纯临时文件, 无仓内硬链接)

| 文件与行号 | 当前机制 | 风险说明 | 建议 |
| --- | --- | --- | --- |
| `tools/candidate-chain.cx:2698` | `atexit(watch_finish)` | 监视收尾钩子, 被 `SIGTERM` 杀时不跑 | 后续按需改 `sy_sig_handle` |
| `tools/ref-material.cx:102,168` | `atexit(cleanup)` | 参考材料临时目录, 被杀残留 `/tmp` | 后续迁 `fsx_scratch` |
| `tools/registry-gen.cx:560` | `atexit(rg_tree_cleanup)` | 临时树, 被杀残留 `/tmp` | 后续迁 `fsx_scratch` |
| `tools/slice-inventory.cx:92` | `atexit(cleanup)` | 临时目录, 被杀残留 `/tmp` | 后续迁 `fsx_scratch` |
| `tools/check-chain-ready-parse2.cx:53` | `sy_atexit(cleanup)` | 夹具临时目录 | 后续迁 `fsx_scratch` |
| `tools/check-image-contrib.cx:29` | `atexit(cleanup)` | 夹具临时目录 | 后续迁 `fsx_scratch` |
| `tools/check-ref-material.cx:56` | `atexit(cleanup)` | 夹具临时目录 | 后续迁 `fsx_scratch` |
| `tools/check-release-sweep.cx:86` | `atexit(cleanup_work)` | 夹具临时目录 | 后续迁 `fsx_scratch` |
| `tests/check-branch-hint.cx:63` | `atexit(clean)` | 测试临时目录 | 后续迁 `fsx_scratch` |
| `tests/check-werror-cli.cx:50` | `atexit(cleanup)` | 测试临时目录 | 后续迁 `fsx_scratch` |
| `tests/check-wt-gate.cx:92` | `atexit(cleanup_scratch)` | 测试临时目录 | 后续迁 `fsx_scratch` |
| `tests/coreapi.cx:480` | `sy_atexit(idx_cleanup)` | 索引临时目录 | 后续迁 `fsx_scratch` |
| `tools/chain-step.cx:157` | `sy_atexit(cs_memo_report)` | 仅打印统计, 无清理风险 | 保持 |

### C 类: 已安全 (真实内核信号处理)

- `tools/bound.c:413,455`: 宿主 C 编译 (`cc`), 走系统真实 `sigaction`/`signal`。
- `tools/devsvc-core.cx` (经 `cx/sys/sigwatch.cx:21`): 走 `sy_sig_handle` 真实 `rt_sigaction`。
- `tools/s9-shards.cx:707`: 走 `sy_sig_handle(SIGINT/SIGTERM, stop_requested)` 真实 `rt_sigaction`。
- `tools/check-old-product-material.cx:56-57`: 已走 `sy_sig_handle`, 且接入 `fsx_scratch` 库层保护。

## 3. `ext/fsx/fs.cx` (`fsx_scratch`) 单点修复与 SIGKILL 兜底

1. **`SIGTERM` / `SIGINT` 内核清理**: `fsx_scratch` 首次初始化时同时注册 `sy_atexit(fsx_sc_atexit)`、`sy_sig_handle(15, fsx_sc_on_sig)` 与 `sy_sig_handle(2, fsx_sc_on_sig)`。
2. **`fork()` 隔离 (`owner_pid`)**: 每个槽位记录创建者 `owner_pid = sy_pid()`, 退出/信号清理只移除当前进程拥有的槽位, 修复此前 `fork()` 子进程退出误删父进程 `fsx_scratch` 目录的缺陷。
3. **`SIGKILL` 不可捕获兜底 (启动时超龄回收)**:
   - 目录名编码创建者 PID (`<prefix>p<pid>-XXXXXX`), 目录内部保持 100% 空目录 (不破坏 `sy_rmdir` 合同)。
   - 每次 `fsx_scratch` 启动时扫描 `sy_tmpdir()` 下同前缀 `<prefix>p<pid>-*` 目录, 仅当同时满足: (a) 普通目录且属主为当前用户 `sy_euid()`, (b) `now - mtime > 1800` (超过 30 分钟), (c) `pid != sy_pid()` 且 `sy_signal(pid, 0) == -ESRCH` (进程已死), (d) 非本进程活跃槽位时, 调用 `fsx_rmtree` 自动清除。
4. **`build/pkg-gaps.tsv` 登记**: 已追加 `PRELUDE-SIGNAL-NO-KERNEL-HANDLER` 条目, 归属 `cx/sys 或 prelude`。
