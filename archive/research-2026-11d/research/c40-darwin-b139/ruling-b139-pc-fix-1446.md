# 代裁：Darwin B139 PC 锚点 → 窄修（机房主任 1446）

墙钟：2026-10-11 14:51 CST。授源：突破（m4pro 同形 B=139 + FINDINGS）+「paste cc 主刀 / 证据够则代裁窄修」。续 1349/1354。

## 结论（根因闭合到具名域）
1. **崩溃锚** `0x10000b2d0` / `str x0,[x1]`：x1=**−9**，x0=0x810000。
2. **−9 来源**：同函数稍早 `bl` 到 `__mmap` 门（svc `0x20000c5`）；Darwin 失败惯例 `b.lo` 后 `neg x0` → **−errno**；errno=**9 EBADF**。
3. **EBADF 原因**：B 镜像 `__mmap` 参数 flags=`imm 34`（**0x22** = Linux `MAP_PRIVATE|MAP_ANON`）。Darwin 上 0x22 **不含** `MAP_ANON(0x1000)`，fd=−1 → EBADF。U 对照同语义点 flags=`**0x1002**`（Darwin `MAP_PRIVATE|MAP_ANON`）→ 成功。
4. **为何 B 带 Linux flags**：bootstrap `branch_b` 调 `"$UA" "$SELF" -c` **未传 `-t $HOST_TARGET`**。`src/main.c` 默认 `t="lnx/x86_64"` → `predef()` 装 `__linux__` 不装 `__APPLE__` → `include/sys/mman.h` 取 `MAP_ANON 0x20` → tape 大量 `imm r0, 34`（本机 B.tape 计 129；`4098` 计 0）。from-tape 按 osx 降 svc 号，但 **立即数已烤死**。U 走 Python `predefines(osx)` → 正确 0x1002。
5. **store-indirect 非发射 bug**：`str [x1]` 忠实执行 `*mmap_ret = …`；坏的是 **指针局部（mmap 返回值）**。修域不在 `emit_arm`/`a_mem`。

## 具名最小修域（代裁授权并落地）
| 文件 | 改动 | 为何够窄 |
|---|---|---|
| **`tests/bootstrap.sh`** | 三处裸 `-c` 改为 `-t "$HOST_TARGET" -c`：UA→B.tape、bs_B→C.tape、bs_U→UT.tape | 只修 bootstrap 目标传递；不改验收谓词、不放宽、不碰 emit/from-tape 内核 |

**明确不修（本刀）**：`src/main.c` 默认 target（产品「裸 tape=lnx」契约保留）；`acceptance.sh` 10-* 分片（Darwin 上同源问题，另刀）；禁 bump/Draft/公开/seal。

## 旁核要求
- **cdx**：核验因果链 PC→−9→EBADF→flags 0x22 vs U 0x1002→UA 缺 `-t`；确认修域仅 bootstrap 三行。
- **cdx2**：对照 1354 建议；本刀已闭合「缺 PC」缺口；intel x86_64 同形可用同一缺 `-t` 解释（另核）。

## CI
- tip diag run `38118763767` @ `fc2c016b`（workflow_dispatch）采样时 **in_progress**（linux×2 + macos-14/15/intel suites）；继续盯；本窄修 **未** 含在该 tip，绿不以该 run 证明本修。

## 本地复测（m4pro 已做；非 CI）
- 对照：旧 bs_B（裸 `-c` tape）`bs_B $SELF -c` → **rc=139**（复现）。
- 证明：`UA $SELF -t osx/arm64 -c` → tape `imm r0, 4098`×4（对照旧 tape ×0）；from-tape 镜像 `mov #0x1002`×4。
- 新 bs_B `$SELF -t osx/arm64 -c` → **rc=0**，C.tape 7730128 B；裸 `-c` 跑新镜像亦 **rc=0**（flags 已烤进镜像）。
- 未在本刀跑完整 `bootstrap.sh` 定点比较（B=C=U）；卡点转交 **CI Darwin** 验绿。`acceptance.sh` 10-* 仍裸 `-c`，另刀。

## 硬禁（不破）
禁 bump / Draft / 公开 / Latest / seal / 放宽验收 / 以 U 代 B / 把 m4pro 绿写成 CI 绿。
