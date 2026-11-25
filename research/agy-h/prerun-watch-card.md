# 0.0.107 预跑值守一页命令卡 (每 10 分钟照抄)

变量(替换 `REV` 为 10 位候选短 sha): `PR="$HOME/.cache/unisacc/prerun/v0.0.107-$REV"`

## 1. 每 10 分钟三条命令 (只读, 不得加 `--append-rate`)

```sh
sh ./unisacc.com research/agy-h/prerun-watch.cx -- --prerun-dir "$PR" --rate-tsv "$PR/prerun-rate.tsv"
tail -n 3 "$PR/supervise.log"
tail -n 2 "$PR/prerun-rate.tsv"
```
(注: 首次采样尚无 `prerun-rate.tsv` 时改传 `--prerun-start <EPOCH_S>` 并去掉 `--rate-tsv`。)

## 2. 输出字段解读 (`PRERUN_WATCH` 行)

| 字段 | 含义 | 口径要点 |
|---|---|---|
| `state` | `RUNNING` / `STOPPED` / `DONE` / `INCOMPLETE` | `rc=0` 正常或完工; `rc=1` 停机或未满窗假完工; `rc=2` 拒跑 |
| `done` | `已完成/总窗数` | 按 `rc=0` 与 `PRERUN_KEEP_GOING` 唯一窗去重 (`--resume` 不重复加) |
| `rate_inc` | 近 10 分钟增量速率 (`窗/分`) | 相对 `rate.tsv` 末行计算; `DONE` 且末行满窗按末次采样冻结 |
| `rate_cum` | 累计平均速率 (`窗/分`) | 无历史或缺 `--prerun-start` 时打印 `UNMEASURED`, 不猜数 |
| `eta_min` | 预计剩余分钟 | 优先用 `rate_inc` 外推; `DONE` 满窗为 `0`, 无正速率打印 `UNMEASURED` |
| `rc124` | 超时 (`58s` 限) 唯一窗列表 | 按首次出现顺序列出窗号 (如 `W0080`), 无则 `none` |
| `lanes` | 实测并发车道数 | 取 `PRERUN_LANES` (未并道回退 `windows.tsv` 头); `107` 须为 `2` |

## 3. 动作阈值与出处

| 观测条件 | 立即动作 (出处) |
|---|---|
| 第 10 分钟 `done < 14` | 复核 `lanes=2`, 不加 lane (`chain-day-roster.md:24`) |
| 第 30 分钟 `done < 110` | 改按保守档执行, 重核 `13:40` 最迟起链线, 不续用基准档 (`decision-24h-fable.md:41`) |
| 速率落后基准 `> 20%` | 查 `supervise.log` `outer_overhead_ms` (防误设 `DVQUALIFY_NATIVE_RUN_BATCH`) 与 `PATH` (`command -v cc == /usr/bin/cc`), 不改窗数 (`chain-day-roster.md:26`) |
| 新现 `rc124` 窗或 `state=STOPPED` | `2` 分钟内核 `pgrep -af 'prerun-drive\.cx'` 已退、留原行读对应 `strict-Wxxxx.tsv`, 再以同 `REV`/`APE`/`$PR` 带 `--window-lanes 2 --resume --keep-going` 续跑 (`chain-day-roster.md:25`) |
| `PRERUN_PRODUCT_RED_STOP` 或连停 `2` 次 | 立即停手保留日志与现场报 `cc`, 不换说法重试 (`chain-day-roster.md:50`) |
| `state=DONE` 且 `done` 满窗 | 核 `eta_min=0`, 转入 `prerun-drive` `merge` / `archive` 与 `release-exec` (`chain-day-roster.md:27`) |
