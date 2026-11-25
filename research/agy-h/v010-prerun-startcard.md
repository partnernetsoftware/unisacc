# 0.1.0 预跑起跑单与值守操作卡 (`v0.1.0`, 基于 F=a11d28fc0a)

编写窗口: `agy-h` | 适用分支: `.claude/worktrees/v0.1.0` (`F=a11d28fc0a`) | 依据: `0.0.107` 第二轮实测复盘、`v010-priority-newbatches.txt`、`prerun-watch` v3 (`12/0 PASS`)

---

## 1. 环境隔离与 `prerun-inputs` / `plan` / `prerun-drive` 起跑命令行

1. 强制环境清理(防 `outer_overhead_ms` 膨胀与缓存污染, `UNISACC_BOOTER_CHAIN_OUT` 保 `cx-booter-ape` 一次过):
```sh
export PATH="/usr/bin:/bin:$HOME/.local/bin"
export TMPDIR=/tmp
unset UNISACC_INTEGRATOR DVQUALIFY_NATIVE_RUN_BATCH UNISACC_RUN_CACHE UNISACC_UNIT_CACHE UNISACC_CACHE_DIR UNISACC_BOUND_DEADLINE_MONO UNISACC_E3CACHE
export UNISACC_BOOTER_CHAIN_OUT="$CHAIN_DIR"
```
2. 输入准备与规划(`inputs` + `plan`, 严格对齐 `tools/prerun-inputs.cx:1-6` 与 `tools/prerun-drive.cx:1-6` 头部签名, 全串首行加 `set -e` 禁静默跳过):
```sh
set -e
sh ./unisacc.com tools/prerun-inputs.cx -- \
  --rev "$REV" --version "$VER" --prev "$PREV_VER" \
  --ape "$CHAIN_DIR/ape-final/unisacc-next.com" --dir "$PR" \
  --repo "$WT" --priority-src "$PRIORITY_SRC" --add-ids "$PR/add-ids.txt" \
  --prev-rev "$PREV_REV" --lanes 2 --jobs 8
sh ./unisacc.com tools/prerun-drive.cx -- plan \
  --ids-file "$PR/ids.txt" --out "$PR" \
  --times-file "$PR/times.tsv" --normal-jobs 8 \
  --priority-file "$PR/priority.tsv" --window-lanes 2 \
  | tee "$PR/plan.summary"
```
3. 起跑前自检三行(必过此三行与 `--keep-going 必带` 校验才允许进第 `4` 步起 `run`, 缺 `windows.tsv`、`W0000` 为重批或漏传 `--keep-going` 立即停手):
```sh
set -e
ls -la "$PR/ids.txt" "$PR/times.tsv" "$PR/priority.tsv" "$PR/windows.tsv" "$PR/plan.summary"
grep -E '^(PRERUN_PLAN|W0000[[:space:]])' "$PR/plan.summary" "$PR/windows.tsv"
RUN_OPTS="--resume --max-jobs 8 --window-lanes 2 --keep-going --min-free 2147483648"
printf ' %s ' "$RUN_OPTS" | grep -qF -- ' --keep-going '  # --keep-going 必带(缺则 set -e 立即中断, 防贴限批 rc=124 硬停)
```
4. 正式起跑(`run`: 禁止带 `--normal-jobs`, `prerun-drive.cx:3067` 拒收; 必带 `$RUN_OPTS` 中的 `--keep-going`; 单次 `exec`, 禁止外层 `for attempt in 1 2` 防止产品红停 `rc=1` 自动重试; 外层 `timeout -k 10s 5400s`, 内层 `set -e`):
```sh
set -e
test -s "$PR/windows.tsv"
printf ' %s ' "$RUN_OPTS" | grep -qF -- ' --keep-going '  # --keep-going 必带
timeout -k 10s 5400s sh -c '
  set -e
  exec sh ./unisacc.com tools/prerun-drive.cx -- run \
    --root "$1" --candidate "$2" --out "$3" \
    $4
' sh "$WT" "$CHAIN_DIR/ape-final/unisacc-next.com" "$PR" "$RUN_OPTS" > "$PR/supervise.log" 2>&1 &
PRERUN_PID=$!
printf '%s\n' "$PRERUN_PID" > "$PR/prerun-drive.pid"
```
说明: 若因非产品问题人工 `--resume` 并输出至 `$PR/prerun-daemon.out`, 同步更新 `$PR/prerun-drive.pid` 为新守护 `PID=PGID`。

5. 模板参数轻量实测留证(私有目录 `/tmp/v010-startcard-smoke`, 不起真实预跑): `prerun-inputs.cx` 全参入 `--dir /tmp/...` 仅在 `:198` 拒 `dir-in-tmp` (`rc=2`, 不报 `unknown-option`); `prerun-drive.cx plan` 全参配合修订后 `v010-priority-newbatches.txt` 在双行夹具(`module-units-limits`,`module-units-forward`) 输出 `PRERUN_PLAN total=2 windows=2 warmup=1 normal=1` (`rc=0`, 且实证锁定 `W0000=module-units-forward`、`W0001=module-units-limits`); `prerun-drive.cx run` 全参在空目录仅在 `:1752` 拒 `read-windows-tsv` (`rc=2`, 不报 `unknown-run-arg`)。

---

## 2. `W0000` 轻窗口锁与 `priority` 文件用法

1. 构造 `--priority-src`: 基于 [v010-priority-newbatches.txt](v010-priority-newbatches.txt), 将第 `1`/`2` 节(`releaseseq`、`tools-compile-193`、冷态重批及仅在暖态测得短耗时的 `module-units-limits`) 全部保持 `status!=PASS` (`priority=2`, `nonpass=1`), 仅将 `0.0.107` round 2 实证冷态首窗通过的 `module-units-forward` (`8520ms PASS`) 放在第 `3` 节 (`status=PASS`), 禁止按暖态 `strict` 耗时手选批充当 `W0000` (见 [car2-108-candidates.md](car2-108-candidates.md) 第 `(16)` 项)。
2. 解析保障(`tools/prerun-drive.cx:466,498-501,660`): 因表含 `PASS` 行(`has_pass_rows=1`), 所有新增/高危/暖态未证批(`!= PASS`) 自动获得 `priority=2` 与 `nonpass=1` 置顶至 `W0001..W0032`, 且 `:660` (`!pd_ids[i].nonpass`) 将其排除出 `W0000` 冷首窗, `W0000` 仅从第 `3` 节实证冷态轻批选取。
3. `plan` 后起跑前必核两句:
   - `grep -n '^W0000\t' "$PR/windows.tsv"` (确认 `W0000` 为实证冷态安全批 `module-units-forward`, 严禁 `module-units-limits` 等暖态手选批占首窗);
   - `head -n 35 "$PR/windows.tsv"` (核 `releaseseq`, `tools-compile-193` 等新增与重点批全部落入前 `32` 窗)。

---

## 3. 四批贴限/特殊批单跑先后与稳态布局规程 (预跑 `DONE` 后执行)

1. 稳态前置原则: 严禁每跑一批改一次 `frozen-root`; 所有串行补跑必须在同一稳定 `frozen-root` 与既有 `tree-NNNN` 布局下进行, 且外层脚本必须设 `timeout >= 75s` (禁止 `60s` 外限在 `dvqualify` 写表前截断), 严禁 `set -e` 接 `grep` 返回 `1` 误中断循环。
2. 建议单跑先后(`jobs=1`, 逐批顺序落 `strict-W99NN.tsv`, 并拷入 `release/evidence/0.1.0/`):
   - 第 `(1)` 批 `tsv-lint-root` (`headers=tree`, 有外部材料 `tsv-lint-tool:path`): 若初跑新布局触发冷构造工具超时 (`~57.6s UNKNOWN rc=3`), 待工具落 `prep-cache` 后在同布局立刻再跑一次 (`107` 实测二跑 `11285ms PASS`)。
   - 第 `(2)` 批 `ratchets-tool-full` (`107` 单批实测 `42680ms PASS`, `assertions=4`)。
   - 第 `(3)` 批 `slow-split` (`107` 初跑 `55449ms` -> 低载复跑 `51144ms PASS`, `assertions=5`, 贴 `55s` 限, 必须在整机空闲下独跑)。
   - 第 `(4)` 批 `precommit-list-cap` (`headers=tree`, 无外部材料, 内含 `4` 次 `precommit.cx`, `107` 同布局四连测 `55486..55557ms` 稳定触 `run-batch.cx:264` `53s` 限): 在 `load1 < 2.0` 下隔离复核一次留 `RAW transport=-1` 严格证据, 按 `expected-unmeasured.tsv` (`reason=window-timeout`) 与既有 `G1` 门处理, 不放宽 `55s`。
3. 拆批门附加核对(`premerge-selfcheck`): 若 `premerge-selfcheck-main` 与 `premerge-selfcheck-rd` 不在 `batch-product-inputs.tsv` 主清单中, 于 `merge` 前补单窗 `jobs=2` 运行 `--ids premerge-selfcheck-main,premerge-selfcheck-rd` 写入 `strict-W9994-premerge-selfcheck-shards.tsv`, 防 `--dry` 停在 `GATE 1 STRICT_SPLIT_INCOMPLETE`。

---

## 4. 负载上限与资源纪律

1. 主预跑与补跑期间, 除值守外所有协同窗口遵守静默令, 禁止并发启动编译、生成器、`check` 全批等重命令。
2. 主预跑严格限 `window-lanes=2`, `max-jobs=8`, `min-free=2147483648` (`2 GiB`); 磁盘剩余低于 `2 GiB` 或 `outer_overhead_ms > 5000ms` 立即查 `TMPDIR` / `PATH` / 泄漏子进程。
3. 串行贴限补跑(`slow-split`, `precommit-list-cap`, `tsv-lint-root`) 启动前提: 整机 `load1 < 2.0` (至多 `< 2.5`) 且前一单批子进程树退净后再起下一批。

---

## 5. `done.WNNNN` 与进程存活判活 (`prerun-watch` v3 值守规范)

1. 每次(`+0m` 起每 `10` 分钟) 只读执行 [prerun-watch.cx](../../tools/prerun-watch.cx) v3:
```sh
sh ./unisacc.com tools/prerun-watch.cx -- \
  --prerun-dir "$PR" --rate-tsv "$RATE_TSV" --prerun-start "$T_PRERUN" --now "$(date +%s)"
```
(若未落 `prerun-drive.pid`, 显式追加 `--pid "$PRERUN_PID"`; 若使用自定义增量日志文件名, 追加 `--daemon-log FILE`)。
2. `v3` 判活与防误判规则(`prerun-watch-v3.md`):
   - 目录内 `done.WNNNN` 文件(仅匹配 `windows.tsv` 主窗; `.zNN` 子恢复片只计 `done_files` 不虚增) 自动补齐日志滞后(`--resume` 期间)或子恢复(`W0220.z06` 类)窗口计数, 确保收官后输出 `state=DONE done=T/T rc=0`;
   - `PRERUN_WATCH_V3` 实测 `pid_alive=yes` 或 `extra_dir > 0` 时压制 `supervise.log` 陈旧 `PRERUN_STOP`, 输出 `state=RUNNING rc=0`;
   - 若日志无 `STOP`/`DONE`, 但 `pid_alive=no` 且 `done < total` -> 立即判 `state=STOPPED rc=1` 揭露静默死进程。
3. 红停处置线: 一旦 `PRERUN_WATCH` 输出 `state=STOPPED` 且 `supervise.log` / `prerun-daemon.out` 含 `PRERUN_PRODUCT_RED_STOP`, 严禁自行 `--resume`, 立刻取 `strict-Wxxxx.tsv` 报 `cc` 与 `cdx` 定因。
