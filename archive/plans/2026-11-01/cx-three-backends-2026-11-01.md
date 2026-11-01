# CX 三后端一致性与改进计划

## 目标

同一 `.cx` 源、同一模块图、同一诊断域，三个后端都成立：

- `unisacc`：产品默认运行 / `-run` / `-cx-plan`
- `gcc_cx`：宿主 gcc 后端
- `clang_cx`：宿主 clang 后端

三个后端可以有不同的编译实现和缓存，但不得有不同的模块语义。

## 当前事实

| 事实 | 证据 |
|---|---|
| 三源已合一 | `cx/cx0/plan.cx` 是权威，`src/cxmod.c` 投影，`cx/compiler/cxmod.cx` 同步；`cx0-extract --check=EXACT` |
| realpath 身份已统一 | 依赖一致则合并，依赖不同具名拒绝；host 与 plan 共享 |
| native 宿主 PCH 有收益 | gcc 75.4→59.1ms、clang 136.5→111.4ms（10 个唯一源冷路径） |
| APE 构建阻塞 | `tools/cxcc.cx` 非 root 调用 `cxproc_run`/`cxproc_run_raw` unknown；当前根 `.com` 是旧 APE，不含新 PCH |
| unisacc 冷路径 | 约 435ms；construction 74.5ms / model inflate ~115ms / inference ~204ms |
| PCH 不跨架构 | `.gch/.pch` 绑定编译器/架构；不能进入 APE 切片 |

## 工作流

### 1. 先修 APE 入口

- 查 `tools/cxcc.cx` 作为非 root 模块导入 `cxproc` 时为什么调用不可见；用最小模块复现。
- 修复应落在模块可见性/生成器，不得在 `gcc_cx.cx` 里复制进程实现。
- 重新跑 `tools/build-cxcc-ape.cx`，验证六目标 APE 头与五切片；在 APE 上跑 host suite。

验收：`file gcc_cx.com` 为 APE/PE；`gcc_cx.com tests/cxcc/suite.cx` 与 APE 入口 PASS。

### 2. 三后端合同矩阵

新增或扩展现有测试，只记录 MATCH/DIFF，不把差异记 PASS：

| 维度 | 用例 |
|---|---|
| plan | 无 use、单 use、多符号、整模块+符号、alias、注释/续行/条件、cxpack |
| identity | 双软链一致、依赖冲突、缺失入口 |
| diagnostics | unknown pragma、cycle、missing module、非公开调用 |
| execution | 返回码/stdout/stderr、`__FILE__`、参数透传 |

对 `unisacc` 使用当前候选 `552c5f96…`，不用旧已装结果冒充。

### 3. 后端缓存分层

- 前端合同：只由 `cx/cx0/plan.cx` 解析；宿主/产品共享。
- native PCH：只可作本机构建缓存；缺失/失效回退原 header。
- APE PCH：不能分发 PCH；若要做，APE 首次运行按本机编译器生成缓存，失败回退，且缓存不改变输出。
- unisacc 前线缓存（若做）：缓存解析/前端状态，不缓存运行结果；键绑定源、package、prelude、版本。

### 4. unisacc 改进实验（在宿主之外）

先做多单元 phase 测量：

- 1/2/8 单元分别记录 construction/model/inference；
- 若按单元重复，优先做单元级并行或前端状态缓存；
- 仅当 phase 数据证明前奏占比高，才做 trimmed prelude；不把宿主 PCH 方案直接搬入。

### 5. 验收

- 三后端合同矩阵：差异逐项有收据，未覆盖记 UNKNOWN。
- APE 入口实际运行；native PCH 收益单列，不冒充 APE。
- `ratchets` 7/7、`cx0-unit`、`cx-host-run`、host suite 全绿。
- 任何缓存优化必须保留冷/热两组数据和 fallback 负例。
