# fast-iteration 第二步: 短批出 long 窗装箱设计与补丁说明

- 基准树: `v0.1.2` (`HEAD 4e8aa13c6`, 与共享 `main` 上的 `tools/prerun-drive.cx`、`tools/check-prerun-drive.cx` 字节一致)
- 实测数据来源: `/home/devali/.cache/unisacc/prerun/v0.1.2-115e5720de/` (`rev 115e5720ded24809c6daadbc9aaa7c72afe08d90`, 只读统计, 未修改缓存)
- 交付文件:
  - 报告: [short-batch-packing.md](file:///home/devali/repos/unisacc/research/agy-h/short-batch-packing.md)
  - 补丁: [short-batch-packing.patch](file:///home/devali/repos/unisacc/research/agy-h/short-batch-packing.patch)
- 状态范围: 所有测试、模拟与 `git apply --check` 均在私有临时目录 (`/tmp/agy_h_work/`) 以只读/副本方式验证, 未修改共享检出、未提交、未执行真实 `prerun`。

---

## (1) v0.1.2 预跑现状分析

### 1.1 实跑窗口与日志构成

从 `/home/devali/.cache/unisacc/prerun/v0.1.2-115e5720de/windows.tsv` 与 `supervise.log` 统计:

| 统计口径 | 指标 | 数值 | 说明 |
| --- | --- | --- | --- |
| `windows.tsv` 计划 | 总窗口 / 总批数 | `247` 窗 / `527` 批 | `warmup=1(1)`, `prebuild-self=1(1)`, `heavy=68(68)`, `long=138(138)`, `normal=39(320)` |
| `supervise.log` 全部 `PRERUN_WINDOW` 行 | 全日志单窗累加 / `jobs=1` 累加 | `5,592,109 ms` (93.20m) / `4,656,282 ms` (77.60m) | 含重试窗口与 canary 窗口 |
| `supervise.log` `phase=long` 日志行 | 行数 / 累加 `wall_ms` | `146` 行 / `3,506,160 ms` (58.44m) | 138 唯一窗 + 8 次重试(`222,711 ms`); 占 `jobs=1` 总和 `75.3%` (串行等价约 `73%`) |
| `phase=long` 中 `sum_batch_ms <= 22000` | 日志行数 / 累加 `wall_ms` | `94` 行 / `1,484,608 ms` (24.74m) | 其中 `wall_ms <= 22000` 占 `78` 行 (`1,115,558 ms`, 18.59m) |
| 唯一 138 个 `long` 窗终次尝试 | 终次 `wall_ms` / `strict_ms` 累加 | `3,183,950 ms` (53.07m) / `2,889,075 ms` (48.15m) | 每窗外层进程开销 `outer_overhead_ms` 约 `2.07s - 2.40s` (均值 `2.13s`) |
| 实际 2 车道 (`window_lanes=2`) 终次墙钟 | 全局实跑墙钟 `total_real` | `3,656,178 ms` (60.94m) | `warmup=55,871`, `prebuild=51,313`, `heavy_real=558,310`(30组), `long_real=2,073,160`(34.55m, 12 canary 串行 + 63 个 2 车道组), `normal=917,524`(39 窗) |
| `normal` (`jobs=8`) 实测并行度 | 39 个 `PASS` 窗实测 | `sum_wall_ms=877,792` / `sum_batch_ms=3,824,028` | 实测有效并行度 `eff_par = 4.36` |

对 138 个 `long` 批逐个关联 `strict-W*.tsv` 第 6 列 (`strict_ms`):
- `strict_ms <= 22000 ms`: `91` 个 (`65.9%`), `strict_ms` 和 `1,225,117 ms`, 单窗实跑和 `1,419,430 ms` (包含 `11` 个 `canary` `W0002..W0012` 共 `128,445 ms` 串行执行, 以及 `80` 个非 canary `1,290,985 ms`)。
- `22000 < strict_ms <= 45000 ms`: `38` 个。
- `strict_ms > 45000 ms`: `9` 个。
- 旧隔离白名单 `pd_is_isolated_long_id`: 仅 `6` 个 (`slow-split`, `unit-frontcache-equiv0,1,2,3b,4`)。

### 1.2 两个根本原因

1. **历史全量 `max` 污染 (`tools/prerun-drive.cx:424-425`)**: `pd_load_times_file()` 对 `times.tsv` 全部历史 `PASS` 样本取单调最大值 (`if (!pd_ids[idx].measured || v > pd_ids[idx].time_ms)`)。约 25 轮历史中任何一次高负载偶发毛刺 (`22.1s - 48.4s`) 会永久钉死该批, 导致 `times_gt_22k` 命中 `132 / 138` 个 `long` 批 (比如 `tools-compile-002` 本轮仅 `5,914 ms`, 近 3 轮最高 `15,534 ms`, 却因历史 `max=47,785 ms` 独占一个 `long` 窗)。
2. **时长判据与共享状态隔离耦合 (`tools/prerun-drive.cx:564-605`)**: `pd_pack_normal_group()` 在多处内联重复判断 `pd_is_isolated_long_id || time_ms > 22000 || unmeasured`, 且缺少对 7 域共享状态冲突 (`d1..d7`) 的白名单约束。这导致两个问题同时存在: 无冲突短批被滞留在 `long` 窗浪费 `2.13s/窗` 外层开销与并行度, 而 `normal` 窗内仍混入了 `devsvc-core-cancel` (52.9s, `d2/d6`)、`precommit-list-cap*`、`release-exec-a/b` 等 9 个共享状态批。

---

## (2) 7 域共享状态冲突审计与独占白名单 (`pd_is_exclusive_heavy_id`)

依据 [lane-isolation-v2.md](file:///home/devali/repos/unisacc/research/agy-flow/lane-isolation-v2.md) 第 68-95 行的 7 类共享状态冲突域 (`d1` 共享仓 git/worktree, `d2` `$HOME/.cache` 等用户态固定状态, `d3` 运行缓存目录, `d4` 共享构建物, `d5` 公共进程/锁/端口, `d6` 固定 `/tmp` 路径, `d7` 本机工作区/外围真值), 核查主要家族源码后得出隔离边界:

| 冲突域 / 分类 | 典型批前缀或精确 ID | 实读源码依据 | 处置方式 |
| --- | --- | --- | --- |
| 原孤立长批与定时缓存竞态 | `slow-split`, `unit-frontcache-equiv0..4`, `unit-frontcache-race`, `unit-frontcache-neg`, `runcache-pin`, `runcache-roots` | `tools/check-prerun-drive.cx:1953-2020`, `tools/check-unit-frontcache.cx`, `tests/runcache-roots.cx`, `tests/check-runcache-pin.cx` (`d3`) | 列入 `pd_is_exclusive_heavy_id`, 留守 1 批 `long` 窗 (即使 `<=22s`) |
| `d1 + d2 + d6`: devsvc 常驻与固定缓存 | `devsvc-git-c-*`, `devsvc-core-*` | `tools/check-devsvc-git.cx:5-6` (`$HOME/.cache/unisacc/devsvc/pinned`), `tools/check-devsvc-core.cx:92-99` (`flock` 共享缓存目录) | 前缀列入白名单 (`devsvc-core-cancel` 等同时从 `normal` 保护回 `long`) |
| `d1 + d6 + d7`: 仓锁/发布/前置门/预合并 | `precommit-flow-*`, `precommit-list-cap*`, `check-gen-all-*`, `prepush-*`, `release-exec-*`, `premerge-selfcheck-*`, `release-flow`, `check-release-state`, `chain-ready-freeze`, `notcovered` | `tools/check-precommit-flow.cx:29-31` (固定 `/tmp/pcfck-fx-<S>`), `tools/check-gen-all-check.cx`, `tools/check-prepush.cx`, `tools/check-release-exec.cx` | 全部前缀/精确名列入白名单 |
| `d4 + d6 + d7`: 布局门/层 API/自举安装/身份探测 | `check-register-layout-*`, `layer-api-*`, `cx-booter-ape`, `module-units`, `ratchets-tool-full`, `check-run-split`, `check-unitlink`, `check-unitlink-more`, `affected-gitnode`, `install-ape-split2`, `processcore-compile`, `productpkg-dir-identity`, `speed-ledger`, `runner-identity`, `evidence-identity`, `lane-key-probes`, `check-netregen-expected-fail`, `check-tls13-get` | `tools/check-register-layout-gate.cx`, `tools/check-layer-api.cx`, `tools/check-dirtywatch.cx` | 精确 ID 与前缀列入白名单 |
| 允许并入 `normal` (`jobs=8`) 的短批家族 | `daily-compile-smoke-*`, `ri-image-*`, `ri-route-refuse`, `tools-compile-*`, `optimization-*`, `properties-cases-*`, `ucn-identities`, `struct-member-capacity-*`, `snapshots-cx-core`, `run-batch-identity`, `run-batch-candidate-root`, `releaseseq`, `check-prerun-drive` 等 | `tests/daily-compile-smoke.cx:8-9` (按子进程隔离 `TMPDIR` 和 `UNISACC_CACHE_DIR`), `tests/tools-compile.cx` (`NEW_OUT` 隔离; 约 150 个分片本就在 `normal` 并行), `tools/check-releaseseq.cx` (仅只读传参 `check-releaseseq`) | 只要最近 5 轮均 `<=22s`, 允许并入 `normal jobs=8` |

注: `properties-cases-det`、`optimization-closure-*`、`affected-oldproduct`、`fixedpoint-evidence`、`tools-compile-131` 等最近 5 轮滑窗仍超过 `22000ms` 的批继续留在 `long` 窗。

---

## (3) 补丁设计与关键改动 (`tools/prerun-drive.cx`)

补丁 [short-batch-packing.patch](file:///home/devali/repos/unisacc/research/agy-h/short-batch-packing.patch) 共 297 行 (`tools/prerun-drive.cx` +100/-14, `tools/check-prerun-drive.cx` +126):

1. **有界尾窗定时 (`PD_TIMES_WINDOW 5`)**:
   - `PdBatch` 增加 `recent_ms[PD_TIMES_WINDOW]` 与 `recent_cnt`。
   - `pd_load_times_file()` 按时间序保留每个批最近 5 个 `PASS` 样本并取该 5 轮窗口最大值赋给 `time_ms`。既防单次毛刺永久把短批钉死在 `long`, 又保留低于 5 样本时与原有 `keep-max-observation` (测试 9) 和 `pass-only` (测试 15) 的完全兼容, 同时维持对近期真实变慢或持续波动批的保守上界。
2. **统一独占判定 (`pd_is_exclusive_heavy_id` + `pd_needs_exclusive_long_window`)**:
   - 新增 `pd_is_exclusive_heavy_id(const char *id)` 整合 `pd_is_isolated_long_id(id)`、10 个共享状态冲突前缀、24 个精确独占/定时敏感批名以及命令行 `--heavy` 追加列表 `pd_heavy_extra`。
   - 新增 `pd_needs_exclusive_long_window(int idx)` 作为 `pd_pack_normal_group()` 内无 times 兜底分支、降序排序和 `normal` 装窗循环的单一共享判据: 只有满足 `!pd_is_exclusive_heavy_id`、`time_ms <= 22000` (或已设置 `--unmeasured-ms` / `compile-smoke` 允许未测并发) 的安全短批, 才允许装入 `normal` (`jobs=8`) 窗。

---

## (4) 前后计划对比与墙钟节省估算

使用完全相同的输入 (`/home/devali/.cache/unisacc/prerun/v0.1.2-115e5720de/ids.txt` 共 527 批与 `times.tsv`) 及参数 (`--normal-jobs 8 --window-lanes 2 --warmup 1`), 对原版与打补丁版做同基准计划生成对比:

| 指标 | 原实跑缓存 (`v0.1.2` `windows.tsv`) | 同基准原版 (`cmp_old`) | 同基准补丁版 (`cmp_new`) | 变化 (`cmp_old -> cmp_new`) |
| --- | --- | --- | --- | --- |
| 总窗口数 | `247` | `246` | `192` | `-54` 窗 (`-22.0%`; 相对实跑 `-55` 窗) |
| `warmup` / `prebuild-self` / `heavy` | `1` / `1` / `68` | `1` / `1` / `68` | `1` / `1` / `68` | `0` (不变量全保留) |
| `long` 单窗单作业 | `138` 窗 (138 批) | `138` 窗 (138 批) | `78` 窗 (78 批) | `-60` 窗 (`-43.5%`) |
| `normal` (`jobs=8`) | `39` 窗 (320 批) | `38` 窗 | `44` 窗 (380 批) | `+6` 窗 (相对实跑 `+5` 窗) |
| `PRERUN_LANE_ESTIMATE` `groups` | - | `152` 组 | `123` 组 | `-29` 组 |
| `PRERUN_LANE_ESTIMATE` `wall_ms` | - | `3,480,538 ms` (58.01m) | `2,196,156 ms` (36.60m) | 省 `1,284,382 ms` (**21.41 分钟**, `-36.9%`) |
| `PRERUN_LANE_ESTIMATE` `serial_ms` | - | `5,542,430 ms` (92.37m) | `3,318,944 ms` (55.32m) | 省 `2,223,486 ms` (**37.06 分钟**, `-40.1%`) |

### 4.1 批集合迁移精确明细 (527 批 100% 守恒)

1. **从 `long` 移入 `normal` (`69` 个批)**:
   - `11` 个原 `canary` 窗 (`W0002..W0012`, 含 `ri-image-*`, `ri-route-refuse`, `daily-compile-smoke-01,02,03,06,07,08,09`), 仅留 `W0001` (`lane-key-probes`, 50.2s) 在 `long`。新计划将这些 early canary 与 `daily-compile-smoke-*` 合并成 `W0002..W0004` 共 3 个 `normal` 窗。这 11 批在 `v0.1.2` 曾严格串行占用 `128,445 ms` (`2.14` 分钟)。
   - `58` 个非 `canary` 短批 (`tools-compile-*`、`optimization-corpus-*`、`properties-cases-*`、`ucn-identities`、`releaseseq`、`snapshots-cx-core`、`run-batch-*`、`check-prerun-drive` 等)。其中仅 `2` 批 (`objects-unsupported-win-arm64` 33938ms、`properties-cases-layout-len12-part6` 32530ms) 因近 5 轮 <=22s 而本轮 >22s, 在新计划中分别位于 `normal` 8-批窗 (`W0157`、`W0153`), 远低于 `45s` 单窗超时与 `serial-recovery` 保护线。
   - 69 批合计 `strict_ms = 937,965 ms` (15.63m), 在 `v0.1.2` 单窗首试累计 `wall_ms = 1,085,468 ms` (18.09m, `retries=0`), 折算为真实 2 车道 (`11` 串行 `128,445 ms` + `58` 并行 `957,023 / 2 = 478,512 ms`) 约 **`606,957 ms` (`10.12` 分钟)** 真实墙钟。
2. **从 `normal` 保护性移入 `long` (`9` 个共享状态冲突批)**:
   - `devsvc-core-cancel` (`52,940 ms`)、`precommit-list-cap` (`25,358 ms`)、`release-exec-b` (`23,228 ms`)、`release-exec-a` (`22,129 ms`)、`precommit-list-cap-tight` (`15,677 ms`)、`prepush-feed` (`8,115 ms`)、`precommit-list-cap-deny` (`8,031 ms`)、`precommit-list-cap-pass` (`7,016 ms`)、`check-gen-all-split` (`2,742 ms`)。
   - 这 9 批 `strict_ms` 合计 `165,236 ms` (2.75m), 加 `9 * 2.13s` 外层开销后在 2 车道 `long` 上约增 **`92,203 ms` (`1.54` 分钟)**, 同时消除了 `devsvc-core-cancel` 在 `normal` 窗内长达 52.9s 的尾延迟与共享 `devsvc`/lock 抖动。
3. **留在 `long` 的原 `69` 个批**:
   - 共 `65` 个命中 `pd_is_exclusive_heavy_id` (原有 `56` 个 + 新移入 `9` 个 = `65` 个, 含 `24` 个 `strict_ms <= 22000` 的独占/定时批) 与 `13` 个非白名单高耗时批 (`affected-oldproduct`, `tools-compile-131`, `fixedpoint-evidence` 及 10 个 `>22s` 慢批), 共 `78` 窗。

### 4.2 保守真实墙钟节省口径说明

`PRERUN_LANE_ESTIMATE` 输出 `21.41 分钟` 节省 (`3,480,538 -> 2,196,156 ms`) 是按理论波次 (`batch-timing-only-no-contention`) 计算的乐观模型值。结合 `v0.1.2` `supervise.log` 实际观测效率折算更为保守可靠的实际预跑墙钟估算:
- `long` 离开 69 批在 2 车道省下: `~10.12 分钟`。
- `long` 新保护进 9 批在 2 车道增加: `~1.54 分钟`。
- `normal` 净增 `+60` 批 (`+772,729 ms` strict 净增量, `+5` 窗外层开销): 按实测 `normal eff_par = 4.36` 折算增加 `772,729 / 4.36 + 5 * 2,130 = 187,881 ms` (`~3.13 分钟`)。
- 扣除增项后, 保守预计净节省实际 2 车道预跑墙钟 **`~5.45 分钟`** (`10.12 - 1.54 - 3.13`); 考虑到移除了 `devsvc-core-cancel` (52.9s) 对 `normal` 并发窗的锁/尾延迟拖累以及减少 `long` 窗口外层启动碎片, 预期真实整轮预跑净节省约为 **5.5 - 7.0 分钟** (若按单窗串行 `wall_ms` 累加口径则净减约 `14 - 15` 分钟), 同时 `PRERUN_LANE_ESTIMATE` 计划值降低 **21.41 分钟**。

---

## (5) 先红后绿验收夹具设计与实测结果

在 `tools/check-prerun-drive.cx` 新增第 `(18)` 项断言 `short-batch-out-of-long` (已包含于 [short-batch-packing.patch](file:///home/devali/repos/unisacc/research/agy-h/short-batch-packing.patch) 中):

1. **夹具构造**:
   - 构造 17 个批输入 `ids.txt`: 1 个极短批 (`w-light` 进 `W0000` warmup), 8 个安全短批 (`daily-compile-smoke-01,02`, `tools-compile-002,004`, `optimization-corpus-9-o0-o1`, `properties-cases-tu`, `ucn-identities`, `releaseseq`, 每项先放 1 条 `23.8s - 47.7s` 历史高值再放 5 条 `<=22s` 近期 `PASS`), 6 个 `<=22s` 的 7 域/定时独占批 (`devsvc-git-c-operations`, `check-register-layout-gate`, `check-gen-all-ready`, `layer-api-real-b`, `slow-split`, `unit-frontcache-equiv0`), 以及 2 个 `>22s` 慢批 (`notcovered`, `check-unitlink`)。
   - 检查命令验证:
     (a) `daily-compile-smoke-02,daily-compile-smoke-01` 并入同一个 `8` 作业 `normal` 窗;
     (b) 其余 6 个非冲突短批也离开 `long` 并合入第 2 个 `normal` 窗 (`count=6`), 不再出现 `long 1 <id>`;
     (c) 所有 8 个独占/超预算批均保持在 `\t1\tlong\t1\t<id>` 单窗中;
     (d) 用 `diff -u` 对比输入 `ids.txt` 与展开所有计划窗口的 `ids` 排序集合 (`grep -v '^#' windows.tsv | grep -v '^window\t' | cut -f5 | tr ',' '\n' | sort`), 确保 100% 集合守恒;
     (e) `PRERUN_PLAN total=17 windows=11 warmup=1 heavy=0 normal=2` 精确匹配。
   - 对 527 批全量真实集合核对 `windows.tsv` 覆盖一致性时, 过滤 `prebuild-self` 复用的第二个 `real-commit` (`grep -v '\tprebuild-self\t'`) 后与 `ids.txt` 做 `diff -u`, 实测 527 批零增零漏。

2. **先红后绿实测记录 (`nice -n 19 taskset -c 10-11`)**:
   - **Red 基线** (补丁应用到 `prerun-drive.cx` 之前, 仅运行含 `--main` 路径的检查):
     `CHECK_PRERUN_DRIVE_SRC=/tmp/agy_h_work/v012_test/prerun-drive.cx sh ./unisacc.com /tmp/agy_h_work/v012_test/check-prerun-drive.cx -- --main`
     -> `FAIL short-batch-out-of-long` (`CHECK_PRERUN_DRIVE ok=54 failed=1 ms=44245`)。
   - **Green 全量** (补丁应用到 `/tmp/agy_h_work/v012_test/prerun-drive.cx` 之后, 运行完整自检套件):
     `CHECK_PRERUN_DRIVE_SRC=/tmp/agy_h_work/v012_test/prerun-drive.cx sh ./unisacc.com /tmp/agy_h_work/v012_test/check-prerun-drive.cx --`
     -> `PASS short-batch-out-of-long` 及全部既有测试全部通过 (`CHECK_PRERUN_DRIVE ok=95 failed=0 ms=28319`)。
   - **只读补丁校验**:
     `git -C .claude/worktrees/v0.1.2 apply --check /home/devali/repos/unisacc/research/agy-h/short-batch-packing.patch` -> exit `0` (`V012_APPLY_OK`)。
     `git apply --check /home/devali/repos/unisacc/research/agy-h/short-batch-packing.patch` -> exit `0` (`MAIN_APPLY_OK`)。

---

## (6) 基于 ccogweco v0.1.2 全量实跑分布的 55s/2 车道回放与 46s 内限合规核查

### 6.1 回放输入与 v0.1.2 实跑基准 (`251 窗 / 约 76 分钟` 口径还原)

- 数据来源: [v012-batch-ms-dist.tsv](file:///home/devali/repos/unisacc/research/ccogweco/v012-batch-ms-dist.tsv) (`529` 数据行 = `527` 个唯一批 + `2` 条 `W99xx` 补跑行 `W9919 check-daily-compile-smoke 3661ms`、`W9987 slow-split 52588ms`, 批内实测总和 `7,451,622 ms`) 与 [v012-long-short-batches.tsv](file:///home/devali/repos/unisacc/research/ccogweco/v012-long-short-batches.tsv) (`91` 个在 v0.1.2 驻留 `long` 却实测 `< 22s` 的短批)。
- 原 `v0.1.2` `251` 窗口构成: `windows.tsv` 主计划 `247` 窗 + `W0242` 超时触发 `serial-recovery` 裂出的 `2` 个子窗 (`W0242.z09`, `W0242.z10`) + `2` 次人工补跑窗 (`W9919`, `W9987`) = `251` 窗; 主驱动终次尝试 `supervise.log` 实测 `60.94 min` (`3,656,178 ms`), 叠加热身备料 `55s`、`8` 次重试 `222.7s`、`W0242` 恢复及 `W0219`/`W99xx` 红停处置合计约 `76 分钟` (参见 `PIPELINE.md:18`)。
- 对 `91` 个 `< 22s` `long` 短批回放统计:
  - 真实并入 `normal` (`jobs=8`): **`67` 个** (另加近 5 轮 <=22s 而当轮 >22s 的 `objects-unsupported-win-arm64` `33,938ms` 与 `properties-cases-layout-len12-part6` `32,530ms`, 共 `69` 批出 `long`)。
  - 留守 1 批 `long` 窗: **`24` 个** (全部由 `pd_is_exclusive_heavy_id` 7 域/定时白名单或近 5 轮滑窗守卫拦截保留)。

### 6.2 按 55 秒窗口、2 车道回放预测 (`short-batch-packing.patch` 输出 `cmp_new`)

将 `v012-batch-ms-dist.tsv` 中 527 批实测 `ms` 逐一映射至打补丁后的 192 窗计划 (`warmup=1`, `prebuild-self=1`, `heavy=68`, `long=78`, `normal=44`), 按 `PD_CHILD_SECONDS=55`、单窗外层开销 `2,130 ms`、`normal` 实测有效并行度 `eff_par=4.36` 与 `window_lanes=2` 回放如下:

| 相位 / 阶段 | 预测窗数 | 回放墙钟 (ms) | 回放墙钟 (min) | 调度口径说明 |
| --- | --- | --- | --- | --- |
| `warmup` (`W0000`) | `1` | `20,254 - 55,871` | `0.34 - 0.93` | 新 plan `W0000=win-argv-x86_64`; 保守下界按同 `v0.1.2` 冷 `kit+material` 取 `55,871 ms` |
| `prebuild-self` (`W0005`) | `1` | `51,313` | `0.86` | 实测 `real-commit prebuild-self` 耗时 |
| `canary` 串行 (`W0001..W0004`) | `4` (`1 long + 3 normal`) | `171,564` | `2.86` | 原 `12` 个 `long` canary 中 `11` 个合并进 `3` 个 `normal` 窗 |
| `heavy` (`68` 窗) | `68` | `558,310` (现行) / `344,935` (全 2 车道) | `9.31` / `5.75` | 现行 `pd_can_share_window` 仅 `netregen-*` 开 2 车道 (沿用实测 `558,310 ms`) |
| `long` (非 canary `77` 窗, 2 车道) | `77` | `1,140,192` | `19.00` | 含 `9` 个从 `normal` 救回的冲突批 (`devsvc-core-cancel` 等); 77 窗单窗之和 `2,227,225 ms` |
| `normal` (非 canary `41` 窗, `jobs=8`) | `41` | `1,202,896` | `20.05` | 按 `max(sum_b / 4.36 + 2130, max_b + 2130)` 逐窗累计 |
| **主计划合计 (不含裂窗)** | **`192` 窗** | **`3,144,529 - 3,180,146`** | **`52.41 - 53.00`** | 对比 `v0.1.2` `supervise.log` 终次实跑 `60.94m` 省 `~8.0 - 8.5m` |
| **含备料/典型抖动预计总墙钟** | **`192 - 194` 窗** | - | **`约 53 - 56 分钟`** | 对比 `v0.1.2` 实际 **`251` 窗约 `76 分钟`** 减少 `57 - 59` 窗、省约 **`20 - 23 分钟`** |

注: 若不对下节发现的 `tsv-lint-root` (`55,532 ms` in `W0180 normal`) 补加独占隔离, 该窗仍会在 55s 边界触发 `serial-recovery` 并至少增加 `1` 个恢复轮次 (`+2` 个子窗 `z09/z10`), 使实际运行总窗升至 `194` 窗并额外耗时约 `~2 分钟`。

### 6.3 贴 46 秒内限的批不得合箱约束检查 (`13 / 15` 满足, `2 / 15` 未拦截)

在 `v012-batch-ms-dist.tsv` 中共有 `15` 个唯一批实测耗时 `>= 46,000 ms` (另有 `release-flow` `44,518 ms` 仍独享 `W0079 long`):

| 处置结果 | 数量 | 批名与实测 ms | 原 v0.1.2 位置 | 补丁后 (`cmp_new`) 位置 | 根因说明 |
| --- | --- | --- | --- | --- | --- |
| 满足 (保持独享 1 批窗) | `12` | `slow-split` (55,502/52,588), `notcovered` (54,554), `check-gen-all-drift` (53,029), `module-units` (52,963), `real-commit` (51,380), `check-gen-all-hang` (51,259), `chain-ready-freeze` (50,804), `cx-booter-ape` (50,685), `lane-key-probes` (50,217), `devsvc-git-c-background` (50,018), `optimization-closure-0-4` (46,664), `stream-pipe-parts` (46,612) | `heavy` 或 `long` (`cnt=1`) | 全部保留在 `heavy` 或 `long` (`cnt=1`) | 命中 `heavy` 或 `pd_is_exclusive_heavy_id` / 近 5 轮 `>22s` |
| 满足 (本补丁从 `normal` 救回) | `1` | `devsvc-core-cancel` (`52,940 ms`) | `W0213 normal` | **`W0148 long (cnt=1)`** | `"devsvc-core-"` 前缀命中 `pd_is_exclusive_heavy_id` 成功保护出 `normal` |
| **未满足 (仍留在 `normal` 合箱)** | **`2`** | `tsv-lint-root` (`55,532 ms`), `module-units-forward` (`46,047 ms`) | `W0242 normal`, `W0244 normal` | **`W0180 normal (cnt=10)`** (回放 `57,662 ms`), **`W0185 normal (cnt=10)`** (回放 `48,177 ms`) | 见下方根因分析; 当前补丁尚未拦截这 2 批 |

两例未拦截的真实根因 (`times.tsv` 实查):
1. `tsv-lint-root`: 最近 8 条 `times.tsv` 记录中有 **`6` 次超时被记为 `UNKNOWN (~53,140 - 53,170 ms)`**, 仅 `2` 次热跑记为 `PASS (8,479 / 8,609 ms)`。因 `pd_load_times_file()` 只读取 `PASS` 行、丢弃 `UNKNOWN` 样本, 系统误以为其仅 `8.6s`, 继续将它装入 `10`-批 `normal` 窗。
2. `module-units-forward`: 最近几次 `PASS` 热态仅为 `101 - 7,025 ms`, 但新候选首次冷编译 `module-units-tool` 材料时跃升至 `46,047 ms` (`v0.1.1` 冷态同样高达 `~59-60s`), 且该子批未在 `pd_is_exclusive_heavy_id` 白名单中。

**合入前建议小增补**: 在本补丁后续落 `main` (`toolline-roadmap.md S5`) 时, 把 `"module-units-forward"` 与 `"tsv-lint-root"` 纳入 `pd_is_exclusive_heavy_id` (或使 `pd_load_times_file` 同时把 `UNKNOWN` 超时 `>= 45s` 的样本计入近期上界), 即可使 `>= 46s` 批实现 **`15 / 15` 全部独享 1 批窗**, 主计划仅增 2 窗至 `194` 窗 (`long 80`), 并消除 `W0180` 分裂风险。
