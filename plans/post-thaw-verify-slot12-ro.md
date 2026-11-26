# v0.1.6 安全子集解冻后运行验收计划 (M1X-A slot-12 + `sy_protect_ro` v2)

## (1) 范围与当前状态

- 覆盖对象:
  - `M1X-A slot-12`: `w-agyneon` 提交 `fe0858c1d` (`tools/tapepack.cx`, `tools/check-tapepack.cx`)。
  - `sy_protect_ro` v2: `cx/sys/mem.cx`、`cx/driver/tape_soload.cx`、`tests/core-interface.tsv`、`tools/std-surface-v2.cx`。
- 当前状态: `PLAN_ONLY` (冻结期未运行任何编译、冷测、生成器或批次命令; 所有运行项均为 `UNMEASURED`)。

## (2) 有序执行命令与源码级预期输出

1. **Step 1 (`M1X-A` 运行验证)**:
   - 命令: `sh ./unisacc.com tools/check-tapepack.cx --`
   - 预期输出 (源自 `tools/check-tapepack.cx:954`):
     - `CHECK_TAPEPACK\tpassed=21\tfailed=0` (`rc=0`, 含 `raw-syscall-static-and-negatives` 与 `raw-syscall-tapemode-run` 通过)。
2. **Step 2 (`M1X-A` 替代体击杀验证)**:
   - 命令: 在临时补丁中依次注入 3 个替代体并运行 `sh ./unisacc.com tools/check-tapepack.cx --`:
     - (a) 保留旧 `syscall -> slot 12` 映射;
     - (b) 放过额外尾部寄存器;
     - (c) 降级不清 `target_kind` 保留局部标签地址求解。
   - 预期输出 (源自 `tools/check-tapepack.cx:954`):
     - 每个替代体均在 `raw-syscall-static-and-negatives` 变红 (`failed>=1`, `rc=1`)。
3. **Step 3 (`sy_protect_ro` L0 原语与 RW vs RX 负控)**:
   - 命令: `sh ./unisacc.com tools/check-cx-sys.cx --` (解冻后落地 `sy_protect_ro` 及其隔离测试子进程负控: 读回一致、写触发 `SIGSEGV`、`/proc/self/maps` `r--p` + 执行探针 `SIGSEGV`; Windows 有效 `-ENOSYS` / 无效 `-EINVAL`)。
   - 预期输出 (源自 `tools/check-cx-sys.cx`):
     - 全项 `ok`, `failed=0`, `rc=0`; 分别注入误接 `RW` 与误接 `RX` 替代体时同一入口变红 (`rc=1`)。
4. **Step 4 (Loader 页合同验证)**:
   - 命令: `sh ./unisacc.com tools/check-tape-soload.cx --` (解冻后扩展 `PF_R` -> `sy_protect_ro`、`PF_W|PF_X` 拒绝、`flags==0` 拒绝、跨权限共享 `4096B` 页拒绝)。
   - 预期输出 (源自 `tools/check-tape-soload.cx`):
     - 全项通过, `failed=0`, `rc=0`。
5. **Step 5 (候选五态与 `stream e2` 集成验证)**:
   - 命令: `sh ./unisacc.com tools/check-slice-nn-m53-s1.cx -- --g5-candidate` (绑定已装候选 APE + `UNISACC_APE_SELF` + `stream e2` 五态实跑, 核查 `sy_protect_ro` 分支命中与影子对拍输出/回退原文)。
   - 预期输出 (源自 `tools/check-slice-nn-m53-s1.cx:668`):
     - `CHECK_SLICE_NN_M53_S1\tPASS\tvectors=28\tok=<N>\tfail=0\tmutations=3\tcandidate_ape_sha=<sha>\ttape_container_sha=<sha>` (`rc=0`)。
6. **Step 6 (全仓棘轮回归)**:
   - 命令: `sh ./unisacc.com tools/ratchets.cx --`
   - 预期输出 (源自 `tools/ratchets.cx:560`):
     - `RATCHETS\tok=13\tfail=0\telapsed_ms=<T>` (`rc=0`)。
7. **Step 7 (`macOS` 跨平台验证)**:
   - 命令: 在 macOS 节点/CI 运行 `sh ./unisacc.com tools/check-cx-sys.cx --` (验证 `sy_protect_ro` 在 macOS `__mprotect(..., PROT_READ)` 生效且写/执行受拒)。
   - 预期输出:
     - 全项 `ok`, `failed=0`, `rc=0`。

## (3) `cdx UNCOVERED` 项 -> 命令映射表

| `cdx UNCOVERED` 项 | 对应验收步骤与命令 |
| --- | --- |
| `runtime run` | Step 1: `sh ./unisacc.com tools/check-tapepack.cx --` |
| `substitute bodies` | Step 2 (`M1X-A` 3 替代体) + Step 3 (`sy_protect_ro` `RW`/`RX` 2 替代体) |
| `loader page contract` | Step 4: `sh ./unisacc.com tools/check-tape-soload.cx --` |
| `integration` | Step 5 (`check-slice-nn-m53-s1.cx -- --g5-candidate`) + Step 6 (`ratchets.cx --`) |
| `macOS` | Step 7: macOS 节点/CI `sh ./unisacc.com tools/check-cx-sys.cx --` |
