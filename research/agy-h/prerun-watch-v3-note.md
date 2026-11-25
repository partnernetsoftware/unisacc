# prerun-watch v3 补充说明: --resume 期间日志读数陈旧问题与处置

记录窗口: `agy-h` (仅设计分析与值守口径补充, 不改 `0.0.107` 源码与 `tests/` 登记表)
生成时机: `2026-11-25 06:53 +0800` (`0.0.107` 第二轮 `B-t --resume` 期间实测暴露)
关联文件: [car2-108-candidates.md](car2-108-candidates.md) 第 `(12)` 项; [roadmap-0.1.x.md](../ccogweco/roadmap-0.1.x.md) 工具线 `T12`

---

## 1. 现象 (`0.0.107` 第二轮 `resume-r1` 实测)

1. `2026-11-25 06:48:28 +0800` (`epoch=1795560508`), 第二轮 `B-t` 人工 `--resume` (`PID/PGID=3661284`) 启动并重置 `strict-W0213.tsv`。
2. 至 `06:52:25` (`+4.0m`), 实地核查观测到:
   - `strict-W0213.tsv` 已落盘 `SUMMARY batches=8 PASS=8 FAIL=0` (`check-daily-compile-smoke PASS rc=0 5073ms assertions=8`);
   - `prerun-daemon.out` 已连续推进 `W0213..W0219` 共 `7` 窗全绿(`done=221/241`), 并进入 `W0220` `PRERUN_RECOVER` (`from_jobs=8 to_jobs=1`), `done.WNNNN` 标记文件数升至 `220`, `PGID=3661284` 进程存活(`etimes=244s`)。
3. 但同期运行 `sh ./unisacc.com research/agy-h/prerun-watch.cx -- --prerun-dir ... --now ...` 仍持续输出 `PRERUN_WATCH state=STOPPED done=214/241 last=W0213` (`PW_RC=1`), 读数完全停留在 `06:43:26` 的上一轮红停历史状态。

---

## 2. 根因 (源码与启动重定向双因素)

1. `tools/prerun-drive.cx` 仅将过程日志行打印至标准输出(`tools/prerun-drive.cx:1174,1782,2158,2174` 等 `printf` 输出 `PRERUN_WINDOW`、`PRERUN_FROZEN_SOURCE`、`PRERUN_RUN` 等), 自身不直接按窗追加 `<out>/supervise.log`。
2. 初次全量启动时外层脚本把标准输出重定向至 `<out>/supervise.log`; 而 `--resume` 重启守护为保留停机现场或分离重启输出, 将标准输出改向 `<out>/prerun-daemon.out`, 仅计划在运行结束收尾时再合并回 `supervise.log`。
3. `research/agy-h/prerun-watch.cx:410-413` 在使用 `--prerun-dir` 时只拼接读取 `%s/supervise.log`, 且 `pw_load_log` (`prerun-watch.cx:255-263`) 按文件内最后一个 `PRERUN_STOP`/`PRERUN_PRODUCT_RED_STOP` 与最后一个 `PRERUN_WINDOW` 行序裁定 `state=STOPPED`。因此只要 `--resume` 实时输出仍停留在 `prerun-daemon.out` 中、尚未并回 `supervise.log`, `prerun-watch` 就必然滞后并误报陈旧的 `STOPPED`。

---

## 3. 建议改法 (`0.0.108` / 工具线 `T12`, 随 `T6` 合并落地)

1. 接收端 (`prerun-watch v3` 兼容升级, 优先低风险):
   - 当传入 `--prerun-dir DIR` 且 `DIR/prerun-daemon.out` 存在并有内容时, 按 `supervise.log` 先读、再续读 `prerun-daemon.out` (或支持显式 `--daemon-log FILE` / 重复 `--log` 依次扫描), 确保 `last_win_idx` 与 `done_wins` 反映 `--resume` 最新进度; 对跨段重复出现的窗口号保持去重计数或只统计增量新窗。
   - 在 `--prerun-dir` 模式下交叉核对 `done.WNNNN` 文件或最新 `PRERUN_FROZEN_SOURCE`, 当日志计数与实盘 `done.WNNNN` 存在明显偏差时在扩展诊断/note 提示中标明来源文件(`supervise+daemon`)。
2. 驱动/启动端配合 (工具一致性):
   - 规范外层 `--resume` 启动命令优先以追加模式(`>> supervise.log`)实时写回 `supervise.log`, 或在 `prerun-drive.cx` 内提供明确参数在保留原日志的同时逐窗行级刷新到 `<out>/supervise.log`; `prerun-watch v3` 对 `prerun-daemon.out` 的兼容读作为双保险保留。
3. 验收要求: 落地至 `tools/prerun-watch.cx` 时补充 `check-prerun-watch.cx` 至少 `2` 条正/负夹具 (`supervise.log` 含旧 `PRERUN_STOP` + `prerun-daemon.out` 含新 `PRERUN_WINDOW` 判 `RUNNING`; 两份合并后含 `PRERUN_RUN status=DONE` 判 `DONE`), 与 `T6` 一并 `register.cx` 登记 `interfaces` 域。

---

## 4. 对 `0.0.108` 与当前 `0.0.107` 值守的影响

1. 不进 `0.0.107`: 本项仅涉及监控/驱动工具一致性与体验, 触 `tools/` 与 `tests/` 登记表, 严格遵守 `0.0.107` 冻结规则, 不在本车修改任何 `.cx` 源码, 顺延至 `0.0.108` / `0.1.x` 工具线(见 `T12`)。
2. 当前 `0.0.107` `--resume` 期间值守口径:
   - 实时进度以 `<out>/done.WNNNN` 文件计数 + `<out>/prerun-daemon.out` 新增 `PRERUN_WINDOW` 行 + `ps` 核验守护 `PGID` 存活为准;
   - `prerun-watch.cx --prerun-dir` 在 `prerun-daemon.out` 尚未并回 `supervise.log` 前只用于核验静态基线(`windows.tsv` 与历史 `rc124`), 不直接以其 `state=STOPPED` 误判在途 `--resume` 停机;
   - 待运行自然收尾并把 `prerun-daemon.out` 追加回 `supervise.log` 后, 再以 `prerun-watch.cx --prerun-dir` 核实最终 `PRERUN_RUN status=DONE`。
