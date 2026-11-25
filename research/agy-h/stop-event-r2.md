# 0.0.107 事件记账: 第二轮 W0213 产品红停与停机事实

记录窗口: `agy-h` (仅事件账本与停机事实核实, 不动工作树、不清理链与预跑证据目录)
当前状态: `cc` 已定因 (`107 追加集 #1 的 daily 登记漏同步表`, 非人工 `--resume` 违例; 外层自动 `attempt=2` 已自然退出, 待 `B-t` 落车后人工 `--resume` 续 `W0213` 起余 `27` 窗; 路径落地与恢复耗时留 `UNMEASURED`)

---

## 1. W0213 产品红停事件与实测原行

1. `attempt=1` 停机实测事实 (`~/.cache/unisacc/prerun/v0.0.107-63354b4a2c/`):
   - 停机时刻: `2026-11-25 06:40:59 +0800` (`epoch=1795560059`, 对应 `strict-W0213.tsv` 首次红停写入 `mtime`; 距第二轮起链 `05:08:30` 经过 `92.5` 分钟, 距第二轮预跑起跑 `05:39:02` 经过 `62.0` 分钟)。
   - `attempt=1` 停机进度: `done=212/241` (`prebuild-self=1`, `heavy=68`, `long=132`, `normal=11`, `last=W0213`, `phase=normal`, 累计速率 `3.42` 窗/分; `INCOMPLETE=2` 为 `W0000`/`W0015`, `KEEP_GOING=3` 为 `W0016`/`W0017`/`W0093`)。
   - `rc124` 集合 (`9` 个): `W0000, W0001, W0014, W0015, W0017, W0088, W0091, W0093, W0185`。
   - `supervise.log` `attempt=1` `PRERUN_PRODUCT_RED_STOP` 与 `PRERUN_STOP` 原行:
     `PRERUN_PRODUCT_RED_STOP	batch=check-daily-compile-smoke	domain=product-qualification	status=FAIL	rc=1	retries=2	window=W0213	triage=inspect-strict-W0213.tsv	raw=check-daily-compile-smoke	product-qualification	FAIL	1	b773c7e221ab059e935276f41c0ac4070737f2ae6c548cd56ea8b6b0acc53c63	3165	7.24	0	-`
     `PRERUN_STOP	batch=check-daily-compile-smoke	retries=2	window=W0213	raw=check-daily-compile-smoke	product-qualification	FAIL	1	b773c7e221ab059e935276f41c0ac4070737f2ae6c548cd56ea8b6b0acc53c63	3165	7.24	0	-`
   - `strict-W0213.tsv` 失败原行 (`attempt=2` 单批隔离复核, 同批在同窗 `attempt=1 jobs=8` 其余 `7` 批 `tools-compile-133`, `facts-current`, `struct-member-capacity-s2040-run`, `tools-compile-077`, `struct-member-capacity-s2040-asm`, `optimization-corpus-7-o0-o1`, `unit-frontcache-budget` 均 `PASS rc=0`):
     `check-daily-compile-smoke	product-qualification	FAIL	1	b773c7e221ab059e935276f41c0ac4070737f2ae6c548cd56ea8b6b0acc53c63	3165	7.24	0	-`
     `RAW	BATCH_RUN	id=check-daily-compile-smoke	status=FAIL	rc=1	transport=0	marker=CHECK_DAILY_COMPILE_SMOKE	candidate=b773c7e221ab059e935276f41c0ac4070737f2ae6c548cd56ea8b6b0acc53c63	headers=product	scope=actual-one-batch-no-cached-result	assertions=6`
     `SUMMARY	batches=1	PASS=0	FAIL=1	UNKNOWN=0	INVALID=0	total_ms=4746	layout_ms=99	shadow=off	ape_sha256=b773c7e221ab059e935276f41c0ac4070737f2ae6c548cd56ea8b6b0acc53c63	version=unisacc 0.0.107`
2. 外层 `attempt=2` 自动重试起止与结果 (`non-manual`, 预案内自动重试):
   - 性质说明: `attempt=2` (`PID=3615200`, `--resume`) 属于外层启动器 `sh` (`PID=3163302`) `for attempt in 1 2` 预案内自动重试, 不是人工 `--resume` 越界续跑 (`prerun-drive.cx` 退出非 `0` 且非 `2` 时由外层触发)。
   - 起止时间: `2026-11-25 06:40:59..06:43:26 +0800` (`epoch=1795560059..1795560206`, 历时约 `2.5` 分钟, 之后 `PGID=3163300` 自然退出)。
   - 实测执行结果:
     - `W0000` (`module-units-forward`, `warmup`): 材料已热后 `attempt=1 PASS rc=0` (`wall_ms=10607`, `sum_batch_ms=8520`, `PRERUN_WARMUP_READY`)。
     - `W0015` (`precommit-list-cap`, `long`): 两次贴 `55s` 上限 `rc=124` (`55501ms` 与 `55488ms`) 记 `PRERUN_KEEP_GOING`, 与 `slow-split` 同属 `55` 秒贴限类(补跑集不变, 历史 `PRERUN_INCOMPLETE` 行 `2` 条中 `W0000` 已清、`W0015` 转 `KG`, `PRERUN_KEEP_GOING` 累计行数为 `4`)。
     - `W0213` (`check-daily-compile-smoke`, `normal`): 再次触发同一缺键红停 (`sum_batch_ms=3467`, `load1=5.20`, `PRERUN_PRODUCT_RED_STOP` `rc=1` 与 `PRERUN_STOP`), `prerun-watch v2` 收敛至 `state=STOPPED done=214/241 last=W0213 rate_cum=3.32 rc=1` (余 `27` 个 `normal` 窗)。
3. 定因、处理方向与证据保留 (`cc` 06:43 定因):
   - 定因: `check-daily-compile-smoke FAIL` 属同一表缺键 (`107 追加集 #1 的 daily 登记漏同步表`), 非候选核心代码缺陷, 处理方向为走 `B-t` 落车后人工 `--resume` 续 `W0213` 起余 `27` 窗。
   - 证据保留: `~/.cache/unisacc/chains/bg-b1/63354b4a2cb707ecc923a89f0df6030149fa697f` 与 `~/.cache/unisacc/prerun/v0.0.107-63354b4a2c` 保持原样保留。

---

## 2. 第二轮耗时实测与后续路径窗口 (`UNMEASURED`)

| 阶段 / 事件 | 耗时口径 | 依据与起止区间 (`+0800`) | 口径类型 |
|---|---|---|---|
| 第二轮 `63354b4a2c` 起链 (`T1 -> T2`) | `26.4m` (`1585s`) | `05:08:30..05:34:55` (`rc=0`, `499/499`, `APE2=b773c7e2...`) | 实测 |
| 链止 -> 预跑起跑 (`T2 -> T_prerun2`) | `4.1m` (`247s`) | `05:34:55..05:39:02` (含 P1..P3 与双轮 `17/0` 独立 kit 检验) | 实测 |
| 第二轮预跑 `attempt=1` -> `W0213` 红停 | `62.0m` (`3717s`) | `05:39:02..06:40:59` (`done=212/241`, `W0021` `8/8 PASS`, `68` 个 `netregen` 全 `PASS`, `W0123 cx-booter-ape PASS`) | 实测 (红停) |
| 外层预案内自动 `attempt=2` (`--resume`) | `2.5m` (`147s`) | `06:40:59..06:43:26` (`W0000 PASS 10.6s`, `W0015 rc=124 -> KG`, 再停 `W0213`, `done=214/241`, `PGID 3163300` 自然退出) | 实测 (自然退出) |
| `B-t` 落车 + 人工 `--resume` 续余 `27` 窗 + 打包/CI/公开 | `UNMEASURED` | 待 `B-t` 落车并启动人工 `--resume` 后按实测补充 | UNMEASURED |

对准入第 `6` 条核对:
- `T2 -> X <= 45m` (`06:19:55`): 已于 `06:20:23` 实测超线, 在 [chain-to-public-r2.tsv](chain-to-public-r2.tsv) 记 `OVER_45M`。
- `T1 -> T4 <= 120m` (`07:08:30`): 当前暂记 `PENDING`, 待到点或公开完成按实测更新。
