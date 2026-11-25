# 第二车 (0.0.107) 预跑监控脚本合同页 (`tools/prerun-watch.cx`)

编写窗口: `agy-h` (只读查证与合同设计, 未新建或修改任何 `.cx` 源码)

---

## 1. 仓内现成工具核查结果

按照先核历史再定工具名的规则, 对仓内提交历史、工具索引与事件机制实查如下:

(1) `git log --all --oneline` 查未占用名称(实跑均无历史记录):
- `tools/prerun-watch.cx`: 无提交
- `tools/prerun-rate.cx`: 无提交
- `tools/prerun-status.cx`: 无提交
- `tools/prerun-monitor.cx`: 无提交

(2) `sh ./unisacc.com tools/devsvc.cx -- tool-index | grep -i 'prerun'` 与现有相邻工具逐一对照:

| 现成工具 / 机制 | 实际能力与出处 | 是否等价于"2-lane 预跑 10 分钟状态与速率一行输出" |
|---|---|---|
| `tools/prerun-inputs.cx` | 冻结生成 `ids.txt`、`times.tsv`、`priority.tsv` 与推荐命令 (`tools/prerun-inputs.cx:1-49`) | 否 (只负责预跑前准备输入) |
| `tools/prerun-drive.cx` | 支持 `plan`、`run`、`merge`、`archive`、`g1-dry`、`replay` (`tools/prerun-drive.cx:3020-3148`) | 否 (无只读 `status`/`watch`/`rate` 子命令; 但产出 `windows.tsv`、`supervise.log`、`done.Wxxxx` 与 `run.done`) |
| `tools/chain-watch.cx` | 候选整链看门狗 (`checkpoint.tsv` + `heartbeat.tsv`), 输出 `CHAIN_WATCH` (`tools/chain-watch.cx:1-18`) | 否 (针对整链相位, 不读预跑 `windows.tsv`、`supervise.log` 与 `prerun-rate.tsv`) |
| `tools/chain-pace.cx` | 整链各相位按基线比较 `OK/SLOW/STALLED/GAP` (`tools/chain-pace.cx:3-8`) | 否 (输入为整链 `checkpoint.tsv` 与 `heartbeat.tsv`) |
| `tools/speed-ledger.cx` | 每版 18 列离线全链速度账本 (`S5`) (`tools/speed-ledger.cx:1-12`) | 否 (版本级事后/汇总指标, 不输出运行中窗口数、增量速率、剩余估计与 `rc=124` 窗口列表) |
| `tools/devsvc-core.cx` 事件机制 | `events [--since N]`、`events --wait KIND [--since SEQ] [--timeout-ms N]`、`events emit KIND TEXT`、`jobs`、`wait JOB` (`tools/devsvc-core.cx:11-41`) | 仅通用进程/事件环基础设施: 预跑守护以独立 `nohup timeout` 跑 `prerun-drive.cx` 时默认不往 `devsvc` 写每窗事件, 也无 `windows.tsv`/`prerun-rate.tsv` 解析器 |

结论: 仓内尚缺等价工具。下面给出拟新增只读监控小工具 `tools/prerun-watch.cx` 的完整合同与配套检查批次 `prerun-watch` 设计 (本文仅设计, 不实现)。

---

## 2. `tools/prerun-watch.cx` 命令行与架构边界合同

### (1) 架构与运行时边界
- 新建 `.cx` 仅使用 `#pragma unisa use` 模块接口(不手写 `#include`), 本机 `tools/` 工具允许 `printf`/`fprintf`, 严禁往仓库 `out/` 写入任何文件。
- 全流程纯内存解析、无子进程构造、有界执行(`< 2s`, 受外层 `55s` 约束); 只读检查模式下绝不写入或改动 `windows.tsv`、`supervise.log`、`done.Wxxxx` 与 `run.done`。
- 有界容量上限(按 `exec/facts/limits.tsv` 口径, 超限具名拒绝 `PRERUN_WATCH\tREFUSE\treason=...-over-cap`, 禁止静默截断): 最大窗口 `2048` (`PD_MAX_WINS`), 日志读取上限 `32 MiB`, `prerun-rate.tsv` 最大采样行 `4096`。

### (2) 调用语法

```sh
sh ./unisacc.com tools/prerun-watch.cx -- \
  --windows PATH --log PATH \
  [--rate-tsv PATH] [--prerun-dir DIR] \
  [--append-rate] [--now EPOCH_S] [--chain-start EPOCH_S] [--prerun-start EPOCH_S]
```

简化入口(只传预跑目录时自动展开默认路径):
```sh
sh ./unisacc.com tools/prerun-watch.cx -- --prerun-dir "$PR" [--rate-tsv "$RATE_TSV"] [--append-rate]
```
- 展开规则: 当传 `--prerun-dir DIR` 且未显式覆盖 `--windows` / `--log` 时, 取 `DIR/windows.tsv` 与 `DIR/supervise.log`; `--prerun-dir` 同时用于可选交叉核对 `DIR/done.Wxxxx` 与 `DIR/run.done`。

### (3) 与 `devsvc` 事件订阅的协同边界
- 常规模式(与 `0.0.106` / `0.0.107` 独立 `nohup` 守护兼容): 每 10 分钟定时调用一次 `prerun-watch.cx` 只读解析三文件。
- 可选 `devsvc` 唤醒模式: 若后续在每窗或外层监督循环接入 `devsvc`, 可由监督脚本在状态推进或停机时执行 `sh ./unisacc.com tools/devsvc.cx -- events emit prerun.tick "win=... rc=..."` (`tools/devsvc-core.cx:12`), 监控方先跑 `sh ./unisacc.com tools/devsvc.cx -- events --wait prerun.tick --since "$SEQ" --timeout-ms 50000` (`tools/devsvc-core.cx:41`) 等新事件唤醒后再调用 `prerun-watch.cx` 读取真实日志并出数(不得以事件字符串替代 `windows.tsv`/`supervise.log` 真相)。

---

## 3. 三件输入文件结构与精确解析口径

### (1) 输入 1: `windows.tsv` (对照 `tools/prerun-drive.cx:857-859,1754-1766` 与 `0.0.106` 实测表头)

实际格式 (`~/.cache/unisacc/prerun/v0.0.106-2e8a19a342/windows.tsv:1-6`):
```text
# prerun-drive windows
# timing=wave-v1 jobs=8 budget_ms=22000
# window-lanes=3
window	jobs	phase	count	ids
W0000	1	warmup	1	prepush-mut1
```
解析规则:
- 读取注释行中的 `# window-lanes=<K>` (`K` in `1..8`, 记为 `plan_lanes`; 缺失则 `plan_lanes=UNMEASURED`)。
- 解析数据行(非 `#`、非 `window\t` 表头且至少 5 列): 累加 `total_wins`, 建立每窗口映射 `wid -> (phase, jobs)`, 统计计划内各 `phase` 窗口总数 (`plan_by_phase`)。若 `total_wins == 0` 拒绝 (`rc=2`)。

### (2) 输入 2: `supervise.log` (对照 `release/evidence/0.0.106/prerun-supervise.log:1-25,145-175`)

识别五类关键原行 (`tools/prerun-drive.cx:1174,1227-1242,1878,2174`):
1. `PRERUN_WINDOW\twindow=<W>\tphase=<P>\tattempt=<A>\tjobs=<J>\trc=<R>\twall_ms=<MS>...`:
   - `done_wins`: 按 `window=<W>` 去重统计出现 `rc=0` 的唯一窗口数(即使经 `--resume` 跨轮重拉也不重复计数); 维护 `done_by_phase` 与最后一次 `rc=0` 的 `last_window` / `last_phase`。
   - `rc124_wins`: 收集所有 `rc=124` 的唯一窗口名(保持出现序); 另单独记录哪些 `rc=124` 窗口尚未在后续日志出现 `rc=0` (`rc124_open`)。
2. `PRERUN_LANES\tfirst=<W>\twindows=<K>`:
   - 记录日志实测并发车道数 `obs_lanes = max(obs_lanes, K)` (`tools/prerun-drive.cx:1878`)。
3. `PRERUN_PRODUCT_RED_STOP` / `PRERUN_STOP`:
   - 若末条 `PRERUN_STOP` (或 `PRERUN_PRODUCT_RED_STOP`) 出现在最后一条 `PRERUN_WINDOW ... rc=0` 之后, 状态判为 `STOPPED` (`rc=1`, 并提取停机批名/窗口号); 若后续已继续跑出新 `PRERUN_WINDOW` 且未见 `PRERUN_RUN status=DONE`, 状态回到 `RUNNING`。
4. `PRERUN_RUN\tstatus=DONE\t...` (`tools/prerun-drive.cx:2174`):
   - 当存在且 `done_wins == total_wins` 时状态判为 `DONE` (`eta_min=0`); 若日志见 `status=DONE` 但 `done_wins < total_wins` 或反之, 拒称 `DONE` 改标 `INCOMPLETE` 或检查截断。
5. 可选交叉核对 `--prerun-dir`:
   - 若存在 `DIR/done.Wxxxx` 文件或 `DIR/run.done`, 允许与日志统计比对差异并附在 `note`/诊断中, 缺日志行不靠猜。

### (3) 输入 3: `prerun-rate.tsv` (对照 `research/agy-h/prerun-rate.tsv:1-11`, 15 列 TSV)

15 列严格表头 (`research/agy-h/prerun-rate.tsv:1`):
`sample_ts`, `epoch_s`, `elapsed_since_chain_start_min`, `elapsed_since_prerun_ids_min`, `done_windows`, `total_windows`, `last_window`, `current_phase`, `done_by_phase`, `rem_by_phase`, `rate_win_per_min`, `eta_prerun_rem_min`, `est_prerun_end_ts`, `est_start_to_pub_min`, `note`。

解析与速率/剩余估计口径:
- 校验首行表头固定 15 列; 若给定文件存在但非空且列数 `!= 15`, 返回 `rc=2` (`rate-schema-mismatch`), 不悄悄跳过损坏行。
- 取最后一行有效样本的 `prev_epoch_s` 与 `prev_done`:
  - 增量速率 `rate_inc`: 当 `now_s > prev_epoch_s` 且 `done_wins >= prev_done` 时, `rate_inc = (done_wins - prev_done) / ((now_s - prev_epoch_s) / 60.0)` (`%.2f`); 否则 `UNMEASURED` (首次采样或时间戳未推进时不除零、不虚报 `0.00`)。
  - 累计速率 `rate_cum`: 当提供有效 `--prerun-start` (或取 `rate.tsv` 首行与当前 `now_s` 之差 > 0, 且已对齐起跑锚点) 时计算 `done_wins / ((now_s - start_s) / 60.0)`; 缺乏起跑锚点时输出 `UNMEASURED`。
  - 剩余估计 `eta_min`:
    - 当 `done_wins == total_wins` 且 `total_wins > 0`: 输出 `eta_min=0`。
    - 当 `rem = total_wins - done_wins > 0` 且存在正速率(`rate_inc > 0`, 若停机/本段未动但有正 `rate_cum > 0` 则按 `rate_cum`): `eta_min = rem / rate` (`%.1f`); 若刚开跑无正速率样本(`rate_inc` 与 `rate_cum` 均为 `UNMEASURED` 或 `0`), 严格输出 `eta_min=UNMEASURED`。
- 可选 `--append-rate`: 仅显式传入时按上述 15 列格式原子追加当前采样行(文件不存在时先写 15 列标准表头)。

---

## 4. 一行输出合同与退出码语义

### (1) 标准单行输出 (`stdout`)

固定以 `PRERUN_WATCH\t` 开头, Tab 分隔键值对(严格包含: 已完成窗口数、速率、剩余估计、`rc=124` 窗口名、`lanes` 数):

```text
PRERUN_WATCH	state=<RUNNING|STOPPED|DONE>	done=<done>/<total>	rate_inc=<%.2f|UNMEASURED>	rate_cum=<%.2f|UNMEASURED>	eta_min=<%.1f|0|UNMEASURED>	rc124=<W0000,W0001|none>	lanes=<N|UNMEASURED>	last=<Wxxxx|none>	phase=<phase|none>
```

0.0.106 实测回放示例(对应 `research/agy-h/prerun-rate.tsv:3` 停机时刻、`0.0.106` `supervise.log:157-159`):
```text
PRERUN_WATCH	state=STOPPED	done=80/238	rate_inc=1.77	rate_cum=1.55	eta_min=101.9	rc124=W0000,W0001,W0080	lanes=2	last=W0079	phase=heavy
```
收尾示例(对应 `prerun-rate.tsv:11` 与 `PRERUN_RUN status=DONE`):
```text
PRERUN_WATCH	state=DONE	done=238/238	rate_inc=3.33	rate_cum=1.94	eta_min=0	rc124=W0000,W0001,W0080,W0087	lanes=2	last=W0237	phase=normal
```

### (2) `lanes` 取值规则 (防 3-lane 表头误导)
- 优先取 `supervise.log` 观测到的真实并发窗口数 `obs_lanes` (来自 `PRERUN_LANES\twindows=<K>`);
- 若日志尚未经历可并道窗(如仍处于 `warmup` 或全程 `heavy` 且未遇 `PRERUN_LANES`), 则回退 `windows.tsv` 表头的 `# window-lanes=<K>`; 两者皆无时输出 `lanes=UNMEASURED`。
- 当实测 `obs_lanes != plan_lanes` (例如计划头写 `3` 而命令行覆写为 `--window-lanes 2`) 时, 以实际并发数 `obs_lanes` 填入 `lanes=` 字段, 并在可选 `note` 列标注 `plan_lanes!=obs_lanes`。

### (3) 退出码与拒绝合同
- `rc = 0`: 输入合法且 `state=RUNNING` 或 `state=DONE`。
- `rc = 1`: 输入合法但 `state=STOPPED` (检测到 `PRERUN_STOP` / `PRERUN_PRODUCT_RED_STOP`, 方便值守脚本分支停等或报警)。
- `rc = 2`: 参数缺失/非法、输入文件不存在/无权读(`ENOENT`/`EACCES`/`ENOTDIR` 区分输出 `errno`)、表格格式错误、超上限; `stderr` 打印 `PRERUN_WATCH\tREFUSE\treason=<why>\tpath=<path>`, 不输出伪 `PRERUN_WATCH state=...` 成功行。

---

## 5. 检查批次与接入手续设计 (`tools/check-prerun-watch.cx` + 批次 `prerun-watch`, 不实现)

### (1) 独立离线检查 `tools/check-prerun-watch.cx`
在 `$TMPDIR/unisacc-cpw-<pid>` 内自造全离线合成夹具(`windows.tsv`、`supervise.log`、`prerun-rate.tsv`), 零触发真实预跑或产品构造, 目标总耗时 `< 5s` (批次预算 `55s`):

| 用例编号 | 夹具场景与输入 | 期望断言与退出码 |
|---|---|---|
| `cpw.normal-2lane` | 10 窗 `windows.tsv` (`# window-lanes=2`), 6 窗 `rc=0` + `PRERUN_LANES\twindows=2`, 前行 `rate.tsv` `done=2` 距 `--now` `300s` | `rc=0`, `state=RUNNING`, `done=6/10`, `rate_inc=0.80`, `eta_min=5.0`, `rc124=none`, `lanes=2`, `last=W0005` |
| `cpw.rc124-dedup-stopped` | `W0003` 先出现两次 `rc=124` 后打 `PRERUN_STOP`; 随后夹具再追加 `--resume` 跑出 `W0003 rc=0` | 停机态: `rc=1`, `state=STOPPED`, `done=3/10`, `rc124=W0003`; 续跑后: `rc=0`, `state=RUNNING`, `done=4/10` (不重复加计), `rc124=W0003` 保留 |
| `cpw.lanes-obs-overrides-plan` | `windows.tsv` 头写 `# window-lanes=3`, 日志 `PRERUN_LANES\twindows=2` | `lanes=2` (不盲从旧头 `3`) |
| `cpw.cold-or-same-ts-unmeasured` | 不传 `--rate-tsv` 或 `now_s == prev_epoch_s` | `rate_inc=UNMEASURED`, `eta_min=UNMEASURED` (不除零、不填 `0`) |
| `cpw.done-zero-eta` | `10/10` 窗全 `rc=0` 且末段含 `PRERUN_RUN\tstatus=DONE` | `rc=0`, `state=DONE`, `done=10/10`, `eta_min=0` |
| `cpw.append-rate-15cols` | 传入 `--append-rate` 向临时 `prerun-rate.tsv` 追加一行 | 新增 1 行严格 `15` 列且原有行逐字节保留 |
| `cpw.refuse-missing-or-bad-schema` | 缺 `--log` 文件(`ENOENT`)、坏 14 列 `prerun-rate.tsv`、`windows.tsv` 零数据行 | `rc=2`, `stderr` 含 `PRERUN_WATCH\tREFUSE` |

结束行标记: `CHECK_PRERUN_WATCH\tpassed=<N>\tfailed=0` (`rc=0`)。

### (2) 批次登记与仓内文档收口步骤(实现时按序执行)
1. 运行 `sh ./unisacc.com tools/register.cx` 登记批次: `id=prerun-watch`, `domain=interfaces`, `implementation=IMPLEMENTED`, `check_tool=tools/check-prerun-watch.cx`, `budget_s=55`, `marker=rc0:CHECK_PRERUN_WATCH`。
2. 保持与 `tools/prerun-drive.cx` 单向解耦: `prerun-watch.cx` 只读消费 `prerun-drive` 产物, 不被 `prerun-drive` 反向依赖, 不进入产品 `SOURCES_SID`。
3. 在 `prd.md` 能力速查表补一行 `tools/prerun-watch.cx`, 并运行 `tools/tables-gen.cx -- --check`、`tools/derive.cx -- --check`、`tools/ratchets.cx` 与受影响检查收口。
