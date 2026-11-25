# 0.0.107 预跑值守使用说明 (`prerun-watch` 只读模式, 不加 `--append-rate`)

编写窗口: `agy-h`
生效范围: `0.0.107` 严格预跑 (`2-lane`) 值守期间每 `10` 分钟采样(以及异常/完工复核)。
版本与生效边界(`cdx` 二审 + `cc` 裁定 -> `v2` 纯只读版):
- `v2` 已按二审与 `cc` 决定完成并同步更新 `../../tools/prerun-watch.cx`、`../../tools/check-prerun-watch.cx` 及 `research/agy-h/prerun-watch.patch`: 移除 `--append-rate` 改纯只读(传入 `--append-rate` 返回 `rc=2 reason=unknown-arg`), 校验全部 `15` 列 `prerun-rate.tsv` 表头(`rate-header-mismatch`), `state=DONE` 且末行已含 `done==total` 时自动取末次采样时间与倒数第二行算 `rate_inc`/`rate_cum`, `status=DONE` 但窗口未满改判 `INCOMPLETE` (`rc=1`), `cx-lint findings=0`, 离线 `8/0` 夹具通过且 `0.0.106` 实跑得 `done=238/238 rate_inc=3.33 rate_cum=1.94`。
- `107` 预跑期间按"不触冻结 `tests/`"原则直接只读运行 `../../tools/prerun-watch.cx`, 速率采样继续由值守窗口人工或单独命令写入 `$PR` 目录自己的速率日志; 补丁 `research/agy-h/prerun-watch.patch` 待解冻后并入 `0.0.108` 工具线并登记批次。

---

## 1. 标准只读值守命令 (`107` 推荐写法)

从版本工作树根(`.claude/worktrees/v0.0.107`)或仓根执行, 只读无副作用:

```sh
PR="$HOME/.cache/unisacc/prerun/v0.0.107-${REV%"${REV#??????????}"}"

# 方式 A: 指定预跑目录 (自动读取 $PR/windows.tsv 与 $PR/supervise.log)
sh ./unisacc.com research/agy-h/prerun-watch.cx -- \
  --prerun-dir "$PR" \
  [--prerun-start <PRERUN_START_EPOCH_S>]

# 方式 B: 带已有历史速率文件读取增量速率与剩余分钟估计 (不加 --append-rate)
sh ./unisacc.com research/agy-h/prerun-watch.cx -- \
  --windows "$PR/windows.tsv" \
  --log "$PR/supervise.log" \
  --rate-tsv "$PR/prerun-rate.tsv" \
  [--prerun-start <PRERUN_START_EPOCH_S>] \
  [--now <EPOCH_S>]
```

强制禁忌:
- 不得加 `--append-rate`。
- 不得把本工具放到 `tests/` 登记批次或改动冻结文件。
- 当未提供含历史有效正速率的 `--rate-tsv` 或未提供 `--prerun-start` 时, 工具如实打印 `rate_inc=UNMEASURED` / `rate_cum=UNMEASURED` / `eta_min=UNMEASURED`, 不猜数。

---

## 2. 一行输出字段、速率口径与退出码

标准 `stdout` 仅输出一行 TSV:
```text
PRERUN_WATCH	state=<RUNNING|STOPPED|DONE|INCOMPLETE>	done=<done>/<total>	rate_inc=<win/min|UNMEASURED>	rate_cum=<win/min|UNMEASURED>	eta_min=<min|0|UNMEASURED>	rc124=<W...|none>	lanes=<N|UNMEASURED>	last=<Wxxxx|none>	phase=<phase|none>
```

关键字段口径与注意项:
- `done=<done>/<total>`: 对照 `windows.tsv` 总窗数, 按 `window=` 去重统计 `supervise.log` 中 `rc=0` 及 `--keep-going` 跳过闭合(`PRERUN_KEEP_GOING`)的唯一窗口数(`--resume` 续跑不重复加计)。
- `lanes=<N>`: 优先取 `supervise.log` 的 `PRERUN_LANES\twindows=<K>` 实测并发数, 仅在日志尚未进入并发窗时回退 `windows.tsv` 表头的 `# window-lanes=`。
- `rc124=<...>`: 按首次出现顺序汇总日志中所有触发 `rc=124` 的唯一窗口名, 无则 `none`。
- 完工态口径(`v2`):
  - 若 `state=DONE` 且 `--rate-tsv` 末行已记 `done_windows == total_windows`(如 `0.0.106` `1795536495`), `v2` 自动以末次采样时间(`prev_epoch_s`)冻结计算 `rate_inc`(取倒数第二至末次采样区间, 如 `3.33`)与 `rate_cum`(如 `1.94`), 不再因事后墙钟 `--now` 归 `0.00` 或摊薄。
  - 若日志见 `PRERUN_RUN status=DONE` 但 `done_windows < total_windows`, `v2` 拒绝判 `DONE`, 改输出 `state=INCOMPLETE` 与非零 `eta_min` 并返回 `rc=1`。

退出码语义:
- `0`: `state=RUNNING` 或 `state=DONE` (正常在跑或已完工)。
- `1`: `state=STOPPED` (检测到 `PRERUN_STOP` / `PRERUN_PRODUCT_RED_STOP`) 或 `state=INCOMPLETE` (`status=DONE` 但窗口未满)。
- `2`: 用法错(含传入已弃用的 `--append-rate`)、文件缺失/无权读(`ENOENT`/`EACCES`/`ENOTDIR`)、格式或 `15` 列表头名错(`rate-header-mismatch`)或超限 (`stderr` 打印 `PRERUN_WATCH\tREFUSE\treason=...\tpath=...`)。
