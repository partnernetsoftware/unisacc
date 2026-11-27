# Batch A 与 Friendly 候选单元调用次数与单次步数实测(LOCAL)

方法:在 `src/` 宿主入口处加 `#ifdef UNISACC_BATCH_A_COUNT` 保护的计数桩(默认关闭,零产品行为变更),以 `-DUNISACC_BATCH_A_COUNT` 链接本地探测产品 `.work/batch-a-count/unisacc_ba_cnt`,在 3 组标准夹具上各跑 3 次(均 `rc=0`,3 次之间偏差 `0.0%`,记录 `load1=2.01`,完整日志 `.work/batch-a-count/calls-13units-3x3.log`);各单元单次调用步数由 A2 runner(`.work/batch-a-count/tapemode_run` + `b9611e14c` `libtape_m1.so`,强制 `a2_used=1`)实测(`.work/batch-a-count/steps-a2.log`)。

派生耗时公式:`cost_us_per_run = calls_per_run * (10.68 + steps_per_call * 0.00177)`(其中 `10.68 us` 为 A2 + Cut 2b 单次调用基线 `a8ecc5d24`, `1.77 ns/step = 0.00177 us/step` 为 A2 步速)。

证伪红线:`calls_per_run > 200` 即不属于 Batch A。

## 13 单元实测总表(3 夹具 x 3 次重复,偏差 0.0%,load1=2.01)

| 单元 | 类别 | entries / live_B | F1 (-c 3行冷编译) calls | F2 (2单元链接) calls | F3 (stage e3流) calls | A2 steps/call (含155 CRT) | 单次 us | F1/F2/F3 cost_us/run | 判定与备注 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `m53` (`cxsa_valid`) | Batch A(已完成) | 1 / 20,723 | 0 (0/0/0) | 0 (0/0/0) | 1 (1/1/1) | 411 (`a2_used=1`) | 11.41 | 0 / 0 / 11.41 | PASS (`<= 200`),已完成 |
| `driveargs` | Batch A | 2 / - | 3 (3/3/3) | 4 (4/4/4) | 1 (1/1/1) | 2052 (`dv_word`, `a2_used=1`) | 14.31 | 42.94 / 57.25 / 14.31 | PASS (`<= 200`),`da_parse=1/1/0, da_word=2/3/1` |
| `modelnumber` | Batch A(证伪) | 4 / - | **4454** (4454x3) | **4325** (4325x3) | **4320** (4320x3) | 1032 (`cxmn_number`, `a2_used=1`) | 12.51 | 55705 / 54092 / 54029 | **FALSIFIED (> 200):不是 Batch A** (`mn_tag=1942, mn_num=2378..2512`) |
| `m25` (`rtsharedname`) | Batch A | 7 / - | 0 (0/0/0) | 10 (10/10/10) | 0 (0/0/0) | 323 (`rts_at`, `a2_used=1`) | 11.25 | 0 / 112.52 / 0 | PASS (`<= 200`),表类 |
| `bind` | Friendly A | 1 / 392 | 1 (1/1/1) | 1 (1/1/1) | 1 (1/1/1) | 206 (`bd_touch`, `a2_used=1`) | 11.04 | 11.04 / 11.04 / 11.04 | PASS (`<= 200`),管道证明 |
| `drivestream` | Friendly A | 1 / 725 | 0 (0/0/0) | 0 (0/0/0) | 1 (1/1/1) | `UNMEASURED` | `UNMEASURED` | 0 / 0 / `UNMEASURED` | PASS (`<= 200`),管道证明;steps 未测因 `dv_stream` 编排完整 stage 管道与 I/O |
| `m27` (`proc`) | Friendly A | 1 / 1,371 | 0 (0/0/0) | 0 (0/0/0) | 0 (0/0/0) | `UNMEASURED` | `UNMEASURED` | 0 / 0 / 0 | PASS (`<= 200`),默认路径 0 调用;steps 未测因 `sy_atspawn` 属 OS 进程钩子 |
| `deps` | Friendly A | 1 / 1,654 | 0 (0/0/0) | 0 (0/0/0) | 0 (0/0/0) | 871 (`cxd_default_target`, `a2_used=1`) | 12.22 | 0 / 0 / 0 | PASS (`<= 200`),管道证明(仅 `-M`/`-MD` 触发) |
| `m30` (`unitobj`) | Friendly B | 1 / 28,922 | 0 (0/0/0) | 0 (0/0/0) | 0 (0/0/0) | 14199 (`uo_path`, `a2_used=1`) | 35.81 | 0 / 0 / 0 | **高价值优选**:28,922 B/entry,默认路径 0 调用(仅 `UNISACC_RUN_SPLIT=1`) |
| `m31` (`objlink`) | Friendly B | 1 / 39,505 | 0 (0/0/0) | 0 (0/0/0) | 0 (0/0/0) | 313 (`ol_error`, `a2_used=1`;`ol_link` `UNMEASURED`) | 11.23+ | 0 / 0 / 0 | **高价值首选**:39,505 B/entry,默认路径 0 调用(仅 `UNISACC_RUN_SPLIT=1`) |
| `driveutil` | Friendly B | 4 / 3,302 | 4 (4/4/4) | 5 (5/5/5) | 1 (1/1/1) | 1306 (`dv_cx+dv_len`, `a2_used=1`) | 12.99 | 51.97 / 64.96 / 12.99 | PASS (`<= 200`),826 B/entry |
| `path` | Friendly B | 5 / 1,951 | 12 (12/12/12) | 12 (12/12/12) | 0 (0/0/0) | 651 (`cxp_object_path`, `a2_used=1`) | 11.83 | 141.99 / 141.99 / 0 | PASS (`<= 200`),390 B/entry |
| `driveface` | Friendly B | 3 / 54,233 | 0 (0/0/0) | 0 (0/0/0) | 0 (0/0/0) | `UNMEASURED` | `UNMEASURED` | 0 / 0 / 0 | **高价值优选**:18,078 B/entry (54,233 B),仅 `as`/`view`/`ar` 子命令调用;steps 未测因含子命令 I/O |

## 关键结论

(1) **`modelnumber` 被证伪踢出 Batch A**:三组标准夹具分别调用 `4,454`、`4,325`、`4,320` 次(`mn_tag=1942`, `mn_num=2378..2512`),远超 `200` 次红线;按 `12.51 us/call` 计算每次运行将增加约 `54-56 ms` 开销,不适合按单调用切片迁移。
(2) **其余 12 个单元全部满足 `calls_per_run <= 200`**:最高为 `path` (`12` 次,`142 us/run`)、`m25` (`10` 次,`113 us/run`)、`driveutil` (`5` 次,`65 us/run`)、`driveargs` (`4` 次,`57 us/run`)。
(3) **高价值/低入口优先队列**:`m31 objlink` (`39,505 B/entry`)、`m30 unitobj` (`28,922 B/entry`)、`driveface` (`18,078 B/entry`, 总 `54,233 B`) 三者在三组默认夹具下 `calls_per_run = 0` 且移出字节量最大,符合按单位入口价值优先原则;`bind`/`drivestream`/`m27`/`deps` 仅作管道证明。
