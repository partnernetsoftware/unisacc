# prerun-watch v3 设计与验收说明 (`resume` 期间以 `done.WNNNN` 与进程存活判活)

交付窗口: `agy-h` | 代码路径: [prerun-watch.cx](prerun-watch.cx) (`633` 行), [check-prerun-watch.cx](check-prerun-watch.cx) (`427` 行)
适用基线: `main c67b497670` (仅写于 `research/agy-h/`, 不动共享树与 `tests/` 登记表, 待工具线解冻后入 `tools/` 并 `register.cx` 登记)

---

## 1. 解决的 `0.0.107` 实测痛点

1. `--resume` 期间日志滞后误报 `STOPPED`: `tools/prerun-drive.cx` 仅写 stdout, `0.0.107` `resume-r1` 重定向至 `prerun-daemon.out` 后, 既盘 `done.W0213..done.W0219` 已落地且 `PGID=3661284` 存活, 但 `v2` 只读旧 `supervise.log` 仍报 `state=STOPPED done=214/241`。
2. 串行恢复子窗导致收尾误报 `INCOMPLETE`: `W0220` 经 `PRERUN_RECOVER` 转由 `W0220.z06` 恢复后虽写出 `done.W0220` 与 `PRERUN_RUN status=DONE`, 但 `supervise.log` 无 `window=W0220 rc=0` 行, `v2` 误报 `state=INCOMPLETE done=240/241`。

---

## 2. v3 新增接口与只读判活状态机

1. 新增可选 CLI 参数(全向后兼容 `v2` `8` 项接口与 `15` 列速率表严格校验):
   - `--daemon-log D.log`: 在主 `--log` (`supervise.log`) 之后连续递增行号续读增量日志; 在 `--prerun-dir DIR` 模式下若 `DIR/prerun-daemon.out` 存在(`sy_exists == 1`)则自动接入。
   - `--done-dir DIR`: 通过 `sys.posix` 目录遍历(`sy_dir_open`/`sy_dir_next`/`sy_dir_close`, 上限 `16384` 项) 扫描 `done.<WID>` 文件; `--prerun-dir DIR` 模式下自动设为 `DIR`。仅匹配 `windows.tsv` 中的主窗 `WID` 补记 `done_wins` 与 `extra_dir`, `W0220.z06` 等子窗只计入 `done_files` 而不虚增主窗总完成数。
   - `--pid PID` / `--pid-file FILE`: 通过 `sys.posix` 只读系统调用 `sy_getpgid((int)pid)` 判活 (`>= 0` 或 `-EPERM` -> 存活 `1`, `-ESRCH` -> 已退 `0`); `--prerun-dir DIR` 下若 `DIR/prerun-drive.pid` 存在则自动读取。
2. 状态机与 `--resume` 判活双向规则:
   - 第一行输出严格保留 `v2` `10` 列格式 (`PRERUN_WATCH\tstate=...\tphase=...\n`) 以不破坏现有脚本; 启用任一 `v3` 参数或检测到增量文件时额外输出第二行:
     `PRERUN_WATCH_V3\tlog_done=N\tdone_files=N\tdir_wins=N\textra_dir=N\tpid_alive=yes|no|UNMEASURED\tlogs=supervise|supervise+daemon`
   - 若见 `PRERUN_RUN status=DONE` 且合并 `done.WNNNN` 后 `done_wins == total` -> `DONE` (`rc=0`), 否则 `INCOMPLETE` (`rc=1`)。
   - 若日志末事件为 `PRERUN_STOP`/`PRERUN_PRODUCT_RED_STOP`: 当且仅当 `pid_alive == 1` 或 `extra_dir > 0` (`done.WNNNN` 计数领先于日志) 时判定被压制为 `RUNNING` (`rc=0`); 否则保持 `STOPPED` (`rc=1`)。
   - 若日志末无 `STOP` 也未 `DONE`, 但显式给出且实测 `pid_alive == 0` 且 `done_wins < total` -> 判 `STOPPED` (`rc=1`, 捕捉静默死进程)。

---

## 3. 实测验收证据 (本轮已全部实跑通过)

1. `cx-lint`:
   `sh ./unisacc.com tools/cx-lint.cx -- research/agy-h/prerun-watch.cx research/agy-h/check-prerun-watch.cx`
   实测: `CX_LINT files=2 findings=0 elapsed_ms=11`, `LINT_RC=0` (零 `#include`, 全用 `sys.fs`/`sys.posix`/`text.text`/`tsvx.read`/`mem.bytes`)。
2. `check-prerun-watch.cx` (`12` 个合成离线夹具, 覆盖 `v2` `8` 例及 `v3` `4` 例):
   - 新增 Case `9`: `v3-stale-stop-suppressed-by-done-files-and-live-pid` (验证旧 `PRERUN_STOP` 下由 `done.W0004..5` + 活 PID 压制判 `RUNNING 6/10` 且子窗 `done.W0003.z01` 不虚增主计数);
   - 新增 Case `10`: `v3-daemon-log-and-subrecover-done` (验证 `supervise.log` + `prerun-daemon.out` + `done.W0003` 子恢复收尾判 `DONE 10/10`);
   - 新增 Case `11`: `v3-dead-pid-marks-stopped` (无 `PRERUN_STOP` 但 `--pid` 已退判 `STOPPED rc=1`);
   - 新增 Case `12`: `v3-refuse-bad-pid-and-missing-done-dir` (坏 PID 与不存在目录严格拒绝 `rc=2`)。
   实测热态耗时: `CHECK_PRERUN_WATCH passed=12 failed=0`, `WARM_MS=4307`, `RC=0`。
3. `0.0.107` 第二轮真实目录回放(`~/.cache/unisacc/prerun/v0.0.107-63354b4a2c` + `prerun-rate-107-r2.tsv`):
   - 命令: `sh ./unisacc.com research/agy-h/prerun-watch.cx -- --prerun-dir "$PR2" --rate-tsv research/agy-h/prerun-rate-107-r2.tsv --prerun-start 1795556342 --now 1795561194`
   - 实测(修正原 `v2` `INCOMPLETE 240/241 rc=1` 误报):
```text
PRERUN_WATCH	state=DONE	done=241/241	rate_inc=2.86	rate_cum=2.98	eta_min=0	rc124=W0000,W0001,W0014,W0015,W0017,W0088,W0091,W0093,W0185,W0220	lanes=2	last=W0240	phase=normal
PRERUN_WATCH_V3	log_done=240	done_files=243	dir_wins=241	extra_dir=1	pid_alive=UNMEASURED	logs=supervise+daemon
R2_V3_RC=0
```
