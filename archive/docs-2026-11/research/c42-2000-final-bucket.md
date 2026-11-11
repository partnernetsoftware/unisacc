# c42 定点闸 /tmp/c42-2000 FINAL 分桶（2026-10-12 SGT，只读分桶）

性质：分桶记账，不是验收通过。acceptance FAILED，release NOT ready。不改验收措辞。

## 1. 运行身份

| 项 | 值 |
|---|---|
| GATE | `/tmp/c42-2000/cand.queue`（fresh start，未 resume，未改 env-pin） |
| 候选 | `93ec722e…`，由含 `src/host_dl.h` 修复（commit `62e0b811`）的树构建 |
| comboot | stage1、stage2、stage3、fixedpoint 全部 rc=0，fixed point 成立 |
| 结证 | 721/721 completed；`queue.sh` final rc=1；无进程残留 |
| 参照 | `/tmp/c42-1640`（旧候选 `e221516a`，721/721，final rc=1，史证） |

## 2. rc 分布

| rc | 数量 | 1640 参照 | 记账 |
|---:|---:|---:|---|
| 0 | 627 | 627 | PASS |
| 1 | 47 | 48 | 见第 3 节 |
| 2 | 7 | 7 | RESOURCE_UNKNOWN（缺内存证据） |
| 77 | 6 | 6 | UNVERIFIED_HOST（宿主不适用，覆盖义务保留） |
| 142 | 34 | 33 | HOST_TIMEOUT，超时记账，不当产品红 |

`exittable.py`（只读，`--h1 archive/plans/v0.0.38.md`）：PASS 627；BLOCKED 27；HOST_TIMEOUT 12；NEEDS_RULING 11；RESOURCE_UNKNOWN 7；RULED_BASELINE 31；UNVERIFIED_HOST 6。

## 3. 真红栏（只留 lib-lifecycle）

- **lib-lifecycle（rc=1）**：`resident-legacy`，AssertionError。唯一的产品真红。
  - 驻留量震荡：量度是整个进程的 `/proc/self/statm` RSS，运行时自身分配计数与 live 均为 0。把 glibc 的 trim 设为环境变量后，原生探针可以通过，说明震荡来自分配器归还堆顶，不是运行时增长。
  - 之后被掩盖的第二项：进程内检查 `test_guest_count==1` 实际为 0，需要确认 guest 映射应在 run 之后保留还是释放，这需要裁定。
  - 本记录不改验收断言。

## 4. 显式标注：com-* 为绿

- `com-multi`：rc=0，`multi-unit ok`。
- `com-forward-multi`：rc=0，`forwardmulti ok 3 wrong 0`。
- 二者都是 `src/host_dl.h` 修复（`62e0b811`）所针对的项。修复之前是 rc=1（静态镜像拒绝 `-run` 宿主转发）。

## 5. 非产品红（分栏记账，不计入真红）

- **forward（rc=1）**：`ok 0 wrong 0 skipped 1`。`tests/forward.sh` 头部注释写明 Linux 不跑 `-run` 宿主转发，按名字跳过。rc=1 来自“skip 即失败”的记账规则，不是输出不一致。
- **rowcov-parse2 连锁（24）+ rowcov-parse2-union + rowcov-enc-union**：前驱 `rowcov-parse2-build` 超时的级联，`predecessor … failed`，耗时 0.00s，未执行。
- **元套件（4）**：`queuestart-launch`（另一 queue.sh 在跑）、`queuetimeout`（`$2` 未绑定）、`queuetimer`、`observation`。撞车或测试脚本问题，非产品。
- **独立 rc1，待分诊（其余）**：`windows-resolver-host`（缺 zig）、`syscall6`、`seedmemory`（FAILED failures=2）、`realprog`、`elfobj`/`com-elfobj`（宿主 ld 无 aarch64elf 仿真，疑似环境）、`exec-formats-f1`（参照二进制构建失败）、`freezecheck`（closure commits without fix: 3）、`ledgercheck`（carry into v0.0.42.md mismatches 1）。
- **rc142 超时（34）**：按超时记账。`seedgen-ppconsumer-1` 为 NEG 夹具停止记录。

## 6. rc2 与 rc77

- rc2（7）：`seedparse2-1/-2`、`seedgen`、`seedgen-2..4`、`com-seedgen`，缺内存证据，需单 owner 单项测量。
- rc77（6，UNVERIFIED）：`exec-bootstrap-osxarm`、`exec-bootstrap-osxx86`、`lib-stack-arm`、`lib-stack-x86`、`lib-stack-x86-hostabi`、`lib-windows-gp-native`。宿主不适用，覆盖义务保留，不是 PASS。

## 7. 本拍边界

- 本回执只在本地提交，不推送。本地 `main` 领先 origin 的两个提交（`62e0b811`、`b3bdaa6e`）与合流要一起处理，等 origin 合流后再推。
- 禁止：resume 旧壳、seal、Latest、签名、公开。Draft 仍缓（acceptance 未过）。
- lifecycle 两条线（驻留震荡、guest 映射计数）继续取证，不用 com-* 的绿去推定 lifecycle。
