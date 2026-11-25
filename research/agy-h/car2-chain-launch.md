# 第二车 (0.0.107) 阶段 C 之后到起链与 2-lane 预跑命令页

编写窗口: `agy-h` (只读分析写作, 未执行重命令)
适用阶段: 工作树已过阶段 B/C (`tables-gen`/`derive`/`precommit`/`chain-ready -> READY=yes`), 准备起 `0.0.107` 候选链直至完成 `2-lane` 严格预跑归档。
对照基准: `0.0.106` 实况 (`chain_rev=2e8a19a342251910d53010f51002b14a4ca7f916`、候选 APE `sha=5f83ac93d201b252ea0b56098475be9e30045875cdf1ce692451cda095ceeb3c`、`SOURCES_SID=c6eeca9f10d66039ce49f5f74ee1ec63727bd85e552f342965d8445136bd812d`, 见 `release/evidence/0.0.106/INDEX.tsv:1-3`) 与 `~/.local/share/unisacc/research/cdx/car2-runbook.md`。

---

## 1. 会话基线: clean PATH、短 TMPDIR 与环境变量约束 (每窗必先执行)

在版本工作树根执行, 每次后台派发或重拉守护时必须在同一命令子壳中显式重设 `PATH` 与 `TMPDIR` (`car2-runbook.md:5-18,51-55`; `release/evidence/0.0.106/path-b-proof.md:38-40`; `tools/chain-ready.cx:31-34`; `prd_mempalace.md:64-66`):

```sh
R="$HOME/repos/unisacc"
V=0.0.107
W="$R/.claude/worktrees/v$V"
BASE="$HOME/.cache/unisacc/chains/bg-b1"
S="$(mktemp -d /tmp/unisacc-car2-XXXXXXXX)"
mkdir -p "$S/tmp"
cd "$W" || exit 2

export PATH="/usr/bin:/bin:$HOME/.local/bin"
test "$(command -v cc)" = /usr/bin/cc
command -v git
command -v gh

unset UNISACC_INTEGRATOR DVQUALIFY_NATIVE_RUN_BATCH UNISACC_RUN_CACHE UNISACC_UNIT_CACHE UNISACC_CACHE_DIR UNISACC_BOUND_DEADLINE_MONO UNISACC_E3CACHE
export TMPDIR=/tmp
test "${#TMPDIR}" -le 20
test -n "${IDENT_HELPER:-}" && test -x "$IDENT_HELPER" && export UNISACC_IDENT_HELPER="$IDENT_HELPER"
```

约束依据与期望原行:
- Clean PATH (`release/evidence/0.0.106/path-b-proof.md:38-40`, `car2-runbook.md:15-18`): `test "$(command -v cc)" = /usr/bin/cc` 必须返回 `0`, 防止 `moltbaby/bin/cc` 遮蔽系统 `/usr/bin/cc` (`W9998` 事故)。
- 短 TMPDIR (`cx/runtime/rcident.cx:278-280`, `release/evidence/0.0.106/path-b-proof.md:56`): 固定使用 `/tmp` (或同款短路径并全程从一而终), 避免超长路径触发 `sha_file` / `cx_fmt` 截断 (`W0150`) 或导致 `.plan` 缓存失效。
- 链前环境卫生 (`tools/chain-ready.cx:31-34`, `prd_mempalace.md:65`): `UNISACC_RUN_CACHE`、`UNISACC_CACHE_DIR` 与失效的 `UNISACC_BOUND_DEADLINE_MONO` 必须未设; `DVQUALIFY_NATIVE_RUN_BATCH` 必须保持未设(默认 `"0"`, `tools/dvqualify.cx:624`)。

---

## 2. 阶段 C 之后到起链与 2-lane 预跑逐步可照抄清单

### (1) 阶段 C 终门复核 (`chain-ready`)

门 5 与门 22 起链前预备口径(以 `~/.local/share/unisacc/research/cdx/car2-chain-ready-gates.md:14,17,33-35` 为准):
- 门 5 (`5-candidate-chain`) 仅接受以下两条串行准备(不使用错误的 `-- OUT CACHE --prepare` 写法), 并在同 `TMPDIR` 下核两条 `PREPARED candidate-check` (`reused=1 reason=identity-hit`):
  `sh ./unisacc.com tools/check-candidate-chain.cx -- --prepare "$TMPDIR/unisacc-cr-prep-cache" tool`
  `sh ./unisacc.com tools/check-candidate-chain.cx -- --prepare "$TMPDIR/unisacc-cr-prep-cache" generator`
- 门 22 (`22-premerge-union`) 的 `stamp` 命中(`reused=1`)只省去预合树生成 (`tools/premerge-union.cx:659-661`), 第 `:672` 行起的 `--shard prep` 阶段仍重跑七个生成器 `--check`, 不得在全门估算里把重复 `prep` 算零。

命令原行 (`car2-runbook.md:57`; `tools/chain-ready.cx:3`):
```sh
REV="$(git rev-parse HEAD)"
OUT="$BASE/$REV"
DEVSVC_OWNER=cc sh ./unisacc.com tools/chain-ready.cx -- --rev "$REV" --version "$V" --chain-base "$BASE"
```
出处与期望原行 (`tools/chain-ready.cx:6-8,2280-2287`):
- 各门均输出 `<item>\tPASS\t...` (仅设计人工项 `7-recipe`、`9-ledger` 为 `UNMEASURED`, `:2281-2282`)。
- 末行严格为 `READY\tyes\tfailed=none\t<ms>` (`:2283`), 退出码 `rc=0`。
- 按 `9-ledger` (`:2282`) 起链前在 `prd_mempalace.md` 登记一行: 输入提交 `REV`、真实 `PID`/`PGID`、截止时间、输出目录 `OUT`。

### (2) 确认无同输入/活链占用并登记 `REV`

命令原行 (`tools/bg-candidate.cx:646-658,804`; `car2-runbook.md:56`):
```sh
printf 'REV=%s\nOUT=%s\n' "$REV" "$OUT"
sh ./unisacc.com tools/bg-candidate.cx -- status --base "$BASE"
```
出处与期望原行 (`tools/bg-candidate.cx:657`):
- `BG_STATUS\tcandidates=<N>\tbase=<BASE>` 且 `rc=0`, 不存在任何 `state=RUNNING` 的在途候选 (`bg-candidate.cx:566` 否则会拒 `another-candidate-running` 或 `live-owner`)。
- 启动前核 `ROOT/unisacc.com` 与 `ROOT/unisacc.com.build.json` 为普通文件而非符号链接 (`tools/chain-ready.cx:42-46`), 且 `16-disk-free` 可用空间 `>= 8G` (`:2272`)。

### (3) 启动候选链 (`bg-candidate start` -> `tools/chain-run.cx`)

命令原行 (`car2-runbook.md:58`; `tools/bg-candidate.cx:538-620`; `tools/chain-run.cx:1-20`):
```sh
DEVSVC_OWNER=cc sh ./unisacc.com tools/bg-candidate.cx -- start --rev "$REV" --base "$BASE" --deadline-s 5400 -- --parallel 4
```
出处与执行链 (`tools/bg-candidate.cx:550-620`):
- `bg-candidate start` 内部先串联 `start_preflight("12", 0)`、`11`、`18`、`16-disk-free` (`:550-551`) 与 `start_chain_preflight(rev)` (运行 `tools/chain-preflight.cx --strict`, `:513-516`), 全绿后写 `owner.tsv` 并拉起 `tools/chain-run.cx -- --rev <REV> --out <OUT> --deadline-s 5400 --parallel 4`。
- 期望原行 (`tools/bg-candidate.cx:619`):
  `BG_CANDIDATE\tSTARTED\trev=<REV>\tguard=<PID>\tpid=<PID>\tpgid=<PGID>\tdeadline_s=5400\thard_s=<N>\tout=<OUT>`
- 若被拒: 打印 `BG_CANDIDATE\tREFUSE\treason=<why>...` (`:491,503,520,532,566,578,616`), 确定性报错禁止盲重试, 必须先修原因或清干净残余进程。

### (4) 链期监控周期与健康判断 (每 10 分钟有界巡检一次)

命令原行 (`car2-runbook.md:65`; `research/cdx/s1/r32-monitor-review.md:1-5`; `tools/bg-candidate.cx:301,646-658`):
```sh
sh ./unisacc.com tools/bg-candidate.cx -- status "$REV" --base "$BASE"
cat "$OUT/owner.tsv"
tail -n 12 "$OUT/chain-run.tsv"
ps -eo pid,ppid,pgid,stat,etimes,args | awk -v g="<PGID>" '$3==g {print}'
DEVSVC_CHAIN_LOCK_FILE="$OUT/chain-supervise.lock" DEVSVC_OWNER=cc sh ./unisacc.com tools/devsvc.cx -- status
```
出处与期望原行:
- `tools/bg-candidate.cx:301,657`:
  `BG_CANDIDATE\t<REV>\tstate=<RUNNING|DONE>\tphases=<done>/<total>\tlast=<phase>\tpid=<PID>\tpgid=<PGID>\trc=<rc>\tape_sha=<sha>`
  `BG_STATUS\tcandidates=1\tbase=<BASE>`
- 监控周期与规则 (`car2-runbook.md:65,69`; `r32-monitor-review.md:3-5`):
  - 每 `10` 分钟 (`600` 秒) 巡检一次 (参照 `0.0.106` `499/499` 约 `28` 分钟, `0.0.107` 预计 `28-45` 分钟, 最长外限 `5400` 秒); 不得用无退出上限的 `while` 干等。
  - 以 `owner.tsv` 的真实 `pid`/`pgid` 与 `ps` 按 `PGID` 列核对进程存活, 不单凭 `pgrep` (`chain-run.cx` 在原生/supervisor 进程名下可能不出现)。
  - `chain-supervise.lock` 租约固定 `监督开始 + 2100 秒` (`r32-monitor-review.md:5`), 租约到期不等价于链停; 连续两次 10 分钟 `chain-run.tsv` (`ATTEMPT`/`SEGMENT`/`BLOCKER`) 无增量再深入查 supervisor 日志。
  - 链在途期禁止变异门、预检整表、全批或大构造。

### (5) 链止封口检查与提取候选 `sha`、`SOURCES_SID`

命令原行 (`tools/bg-candidate.cx:677-757`; `car2-runbook.md:67,73,131`; `release/evidence/0.0.106/INDEX.tsv:1-3`):
```sh
cat "$OUT/exit.tsv"
sh ./unisacc.com tools/bg-candidate.cx -- promote --check --rev "$REV" --base "$BASE"
APE="$OUT/ape-final/unisacc-next.com"
sha256sum "$APE"
SOURCES_SID=$(awk -F'\t' '$1=="source"{sid=$2} END{print sid}' "$OUT/ctx/context.tsv")
PREVREV="2e8a19a342251910d53010f51002b14a4ca7f916"
printf 'APE=%s\nSOURCES_SID=%s\nPREVREV=%s\n' "$APE" "$SOURCES_SID" "$PREVREV"
```
出处与期望原行 (`tools/bg-candidate.cx:744-746`):
- `PROMOTE_CHECK\tOK\trev=<REV>\tphases=<total>/<total>\tinputs=<N>\tnot=PUBLISHED\tnot=PASS`
- `NEED\tape\t<OUT>/ape-final/unisacc-next.com\tsha=<64hex>\tbytes=<N>`
- `NEED\tsource_sid\t<64hex>`
- (`tools/bg-candidate.cx:739-742` 从 `$OUT/ctx/context.tsv` 末尾 `source\t<64hex>` 取得 `SOURCES_SID`, 打包阶段可直接传 `--sid "$SOURCES_SID"` 给 `tools/pack-srcdir.cx` (`car2-runbook.md:131`); `0.0.106` 实测对照为 `APE sha=5f83ac93d201b252ea0b56098475be9e30045875cdf1ce692451cda095ceeb3c`, `SOURCES_SID=c6eeca9f10d66039ce49f5f74ee1ec63727bd85e552f342965d8445136bd812d`, 见 `release/evidence/0.0.106/INDEX.tsv:2-3`)。

### (6) 冻结严格预跑输入 (`prerun-inputs`, 显式指定 `--lanes 2`)

命令原行 (`tools/prerun-inputs.cx:3-6,376-380`; `car2-runbook.md:74,76`):
```sh
PR="$HOME/.cache/unisacc/prerun/v$V-${REV%"${REV#??????????}"}"
mkdir -p "$PR"
sh ./unisacc.com tools/prerun-inputs.cx -- --rev "$REV" --version "$V" --prev 0.0.106 --prev-rev "$PREVREV" --ape "$APE" --dir "$PR" --lanes 2 --jobs 8
```
出处与期望原行 (`tools/prerun-inputs.cx:376,378,380`):
- `PRERUN_CMD\tplan\tsh ./unisacc.com tools/prerun-drive.cx -- plan --ids-file "<PR>/ids.txt" --times-file "<PR>/times.tsv" --priority-file "<PR>/priority.tsv" --out "<PR>" --normal-jobs 8 --window-lanes 2`
- `PRERUN_CMD\trun\tnohup timeout -k 10s 3600s sh -c 'for attempt in 1 2; do sh ./unisacc.com tools/prerun-drive.cx -- run --root . --candidate "$1" --out "$2" --window-lanes 2 --normal-jobs 8 --resume --max-jobs 8 --keep-going --min-free 2147483648; rc=$?; [ "$rc" -eq 0 ] && exit 0; [ "$rc" -eq 2 ] && exit 2; done; exit 1' sh "<APE>" "<PR>" >> "<PR>/supervise.log" 2>&1 &`
- 末行(`:380`): `PRERUN_INPUTS ape=<APE> ids_sha=<64hex> times_sha=<64hex> priority=<...> dir=<PR> added=<N> excluded=<M> status=READY` (`rc=0`; 若输出 `status=MISSING:...` 则先补齐)。

### (7) 生成 `2-lane` 窗口计划 (`prerun-drive plan`)

命令原行 (`tools/prerun-inputs.cx:376`; `tools/prerun-drive.cx:2965`; `car2-runbook.md:77`):
```sh
sh ./unisacc.com tools/prerun-drive.cx -- plan --ids-file "$PR/ids.txt" --times-file "$PR/times.tsv" --priority-file "$PR/priority.tsv" --out "$PR" --normal-jobs 8 --window-lanes 2
```
期望原行 (`tools/prerun-drive.cx:2965`):
- `PRERUN_PREPARE\tversion=0.0.107\trev=<REV>\tape_sha256=<64hex>\tids=<N>\tdvqualify_list=<N>\texcluded_planned=<N>\texcluded_foreign_platform=<N>\texcluded_stale=<N>\twindows=<W>\tout=<PR>`

### (8) 预跑前候选 APE 与材料预热 (按 `car2-prewarm-list.md` 第 5 节联合判据)

命令原行 (`car2-runbook.md:85-104`; `tools/affected.cx:1506-1513,2005-2043`):
```sh
unset DVQUALIFY_NATIVE_RUN_BATCH UNISACC_RUN_CACHE
# 1. 同工作树/私有布局 QROOT 上以 CANDIDATE 预热 affected.cx 与 tests/run-batch.cx (用法拒绝 rc=2 为预期预热)
for unit in tools/affected.cx tests/run-batch.cx; do
  rc=0
  timeout -k 5s 50s sh "$APE" "$unit" -- --help || rc=$?
  printf 'RUN_CACHE_WARM unit=%s rc=%s\n' "$unit" "$rc"
  test "$rc" -eq 2
done
# 2. 对本次 windows 交集中第八列非空的材料批做有界材料准备 (如 PREP_ID=<实际批ID>)
sh "$APE" tools/affected.cx -- --materials-dump --ids "$PREP_ID" --prepare-materials -j 1
```
出处与期望原行 (`tools/affected.cx:1436-1439,2027`; 详见 `research/agy-h/car2-prewarm-list.md` 第 5 节):
- 必须同时核: `MATERIAL\t<BATCH>\t<NAME>\t<REAL_ABS_PATH>` (无 `-` 或 `REFUSED=`), `PREPARED\t<NAME>\tid=<16hex>\tsha256=<64hex>\tcache=<hit|built>\tms=<N>` (无 `PREP_FAILED`), 以及 `PREBUILD\tmaterials=<N>\thit=<H>\tbuilt=<B>\tfailed=0\tdeferred=0` (`N>0` 且 `H+B==N`)。
- `PREPARED`/`MATERIAL` 不计 `PASS`, 材料批仍随严格窗口实跑。

### (9) 启动 `2-lane` 严格预跑守护 (`--window-lanes 2 --resume --keep-going`)

命令原行 (`tools/prerun-inputs.cx:378`; `tools/prerun-drive.cx:31,89,1226-1244,2996,3041-3049`; `car2-runbook.md:108-112`):
```sh
nohup timeout -k 10s 3600s sh -c '
  export PATH="/usr/bin:/bin:$HOME/.local/bin"
  export TMPDIR=/tmp
  unset DVQUALIFY_NATIVE_RUN_BATCH UNISACC_RUN_CACHE UNISACC_CACHE_DIR
  for attempt in 1 2; do
    sh ./unisacc.com tools/prerun-drive.cx -- run --root . --candidate "$1" --out "$2" \
      --window-lanes 2 --normal-jobs 8 --resume --max-jobs 8 --keep-going --min-free 2147483648
    rc=$?
    [ "$rc" -eq 0 ] && exit 0
    [ "$rc" -eq 2 ] && exit 2
  done
  exit 1
' sh "$APE" "$PR" >> "$PR/supervise.log" 2>&1 &
```

关键参数语义 (`tools/prerun-drive.cx`):
- `--window-lanes 2`: 强制 2 车道并行 (`0.0.105`/`0.0.106` 验证 `heavy` 窗 `3-lane` 不可行; 单窗仍保持 `PD_CHILD_SECONDS=58`, `:31`)。
- `--normal-jobs 8 --max-jobs 8`: 限制 `normal` 窗最多 8 并发; `heavy`/`long` 窗自动单窗 `jobs=1`。
- `--resume` (`:3041`): 读取已完成窗的 `done.Wxxxx` 与 `strict-Wxxxx.tsv`, 断线或修红重拉时跳过已完窗口, 仅续跑未完成或串行恢复 `zNN` 分片 (`:1213-1216`)。
- `--keep-going` (`:89,2996,3064`): 将内部默认值 `pd_keep_going=1` 提升为 `2`, 遇 `UNKNOWN` 或非 `product-qualification FAIL` 时输出 `PRERUN_KEEP_GOING` / `PRERUN_INCOMPLETE` (`:1237,1242`) 并继续推进剩余窗口; 但一旦遇到 `product-qualification FAIL` (`:1226-1231`), 仍强制打印 `PRERUN_PRODUCT_RED_STOP` + `PRERUN_STOP` 并返回 `rc=2`, 由外层 `[ "$rc" -eq 2 ] && exit 2` 立即终止守护交由人工分诊。
- `--min-free 2147483648` (`:3049`): 起跑及续窗时检查磁盘余量至少 `2 GiB`。

期望原行 (`tools/prerun-drive.cx:2174-2177`; `release/evidence/0.0.106/prerun-supervise.log:1,161`):
- `PRERUN_FROZEN_SOURCE\trev=<REV>\troot=<PR>/frozen-root`
- 逐窗记录 `PRERUN_WINDOW\twindow=Wxxxx\tphase=<phase>\tattempt=1\tjobs=<J>\trc=0\twall_ms=<W>\tsum_batch_ms=<S>\touter_overhead_ms=<O>`
- 完成末行: `PRERUN_RUN\tstatus=DONE\tran=<N>\tskipped=<M>\tout=<PR>\twindow_wall_ms=<W>\tsum_batch_ms=<S>\touter_overhead_ms=<O>`, 并在 `$PR/run.done` 写入 `status\tDONE` (`:2166`)。

### (10) 预跑期间监控周期、停机续跑与归档合并

监控命令 (每 `10` 分钟采集一次速率与尾行, 见 `car2-runbook.md:112`):
```sh
tail -n 20 "$PR/supervise.log"
grep -cE '^PRERUN_WINDOW.+rc=0' "$PR/supervise.log"
pgrep -af 'prerun-drive\.cx'
```
停机续跑判据 (`tools/prerun-drive.cx:1226-1231,2157-2160`):
- 若日志出现 `PRERUN_STOP` (如 `W0080` `rc=124 x2` 或 `W0087` `PRERUN_PRODUCT_RED_STOP`), 先通过 `pgrep -af 'prerun-drive\.cx'` 确认旧进程组完全退出, 读取对应 `strict-Wxxxx.tsv` 分诊; 若属已知非阻断项或已修复重起, 使用完全相同的 `REV`、`APE`、`$PR` 与 `--resume` 重执行第 (9) 步。

结束后合并与归档命令 (`car2-runbook.md:124-125`; `tools/prerun-drive.cx:2266,2414`):
```sh
sh ./unisacc.com tools/prerun-drive.cx -- merge --in-dir "$PR" --out "$PR" --version "$V"
sh ./unisacc.com tools/prerun-drive.cx -- archive --in-dir "$PR" --repo "$W" --version "$V"
```
期望原行:
- `PRERUN_MERGE\twindows=<W>\tbatches=<B>\tPASS=<P>\tFAIL=<F>\tUNKNOWN=<U>\tSUPERSEDED=<S>\tout=<PR>` (`:2414`)
- `PRERUN_ARCHIVE\tversion=0.0.107\twindows_bytes=<N>\tlog_bytes=<N>\thome_paths_redacted=<N>` (`:2266`)

---

## 3. 0.0.106 踩坑对应的 0.0.107 强制预防条目

| 序号 | 0.0.106 实况与根因 | 0.0.107 必须执行的预防条目与验收原行 | 出处文件与行号 |
|---|---|---|---|
| (1) `chain pin` 失配/过期 | `0.0.106` 早期起链在 `final-net` 阶段因 `kernelsha` 字面量 pin 失配退出 `rc=2` (`car2-runbook.md:61`); 后于提交 `2e8a19a342` 合入 `kernelsha pin` 修复 `r2` 并在 `chain-preflight` 新增第 9 节点 `sourcepackage-pins` (`timeout_s=15`) 同步校验全组消费者。 | 起链前必跑 `sh ./unisacc.com tools/check-sourcepackage-pins.cx -- .` 与 `sh ./unisacc.com tools/check-pins.cx -- --pins-only`, 核 `SOURCEPACKAGE_PINS ... failed=0` 与 `CHECK_PINS ... failed=0`, 且确认 `bg-candidate start` 自动通过 `chain-preflight.cx --strict` 第 9 节点 (`sourcepackage-pins`) `PASS`; 改动 `kernelsha.cx` 时须一次性同步全部 6 个消费者与 `tools/pins.tsv`。 | `tools/flows/chain-preflight.tsv:16`, `tools/check-sourcepackage-pins.cx:1-18`, `tools/check-pins.cx:1-27`, `tools/pins.tsv:268,306,322,328,339,387-394`, `2e8a19a342` |
| (2) `heavy` 窗 `3-lane` 触发 `rc=124` | `tools/freeze-day.cx:822-823`、`tools/release-chain.cx:90,94`、`tools/prerun-inputs.cx:33` 默认输出 `--window-lanes 3`。在 `0.0.105` `3-lane` 跑 `heavy` 窗三路齐超 `58` 秒限全报 `rc=124` (`W0016-W0018`), 降为 `2-lane` 后 `W0019-W0020` 在 `31.4s` 双过; `0.0.106` 的 `W0080 (check-split-parents)` 亦两次 `rc=124` 后单窗 `57.02s` 过。 | `prerun-inputs.cx` 与 `prerun-drive.cx plan/run` 全程手动覆写并固定 `--window-lanes 2`, 严禁直接套用 `freeze-day`/`release-chain` 打印的 `--window-lanes 3` 或放宽 `PD_CHILD_SECONDS=58`。`heavy` 窗若再触 `rc=124` 只降并发或单窗 `--resume` 分诊。 | `archive/research-2026-11-25/ccogweco/mempalace-decisions.patch:19`, `car2-runbook.md:112`, `tools/prerun-drive.cx:31`, `tools/freeze-day.cx:822-823`, `tools/release-chain.cx:90,94`, `release/evidence/0.0.106/prerun-supervise.log:157-161` |
| (3) 误设 `DVQUALIFY_NATIVE_RUN_BATCH` | 该开关为 `opt-in` 默认 `"0"` (`sh` 运行器)。`0.0.106` 预跑前半误设为 `1`, 导致 `QUALIFY_SELECTOR_PREP ms=20048 reused=0` 常驻触发, 使 `W0000-W0079` 窗口外耗 `outer_overhead_ms` 高达 `23.6s - 28.5s` 并把多窗压超时; 取消该变量后外耗立即降回 `2.02s - 2.32s`。 | 在终端会话、第 (1) 步环境初始化与第 (9) 步 `nohup` 子壳内双处显式 `unset DVQUALIFY_NATIVE_RUN_BATCH`, 启动前断言 `test "${DVQUALIFY_NATIVE_RUN_BATCH:-0}" != "1"`。除非跨双布局 `reused=1` 专项过门 (`car2-runbook.md:122`), 不得在预跑开启。 | `tools/dvqualify.cx:624`, `tools/dvledger.cx:71,167`, `archive/research-2026-11-25/ccogweco/mempalace-decisions.patch:26`, `car2-runbook.md:120-122`, `release/evidence/0.0.106/prerun-supervise.log:4-18,146-163` |
| (4) 预热只看 `PREBUILD failed=0` (`cdx` 补录) | `affected.cx --materials-dump --prepare-materials` 在材料被拒时仍可返回 `rc=0`, 且 `PREBUILD` 单独只看 `failed=0` 会放过 `deferred>0`、`materials=0` 或输出占位于 `MATERIAL ... - REFUSED=<why>` 的失败项。 | 预热验收必须三件齐核: (a) 每个 `MATERIAL\t<BATCH>\t<NAME>\t<REAL_ABS_PATH>` 第 4 列为实有路径(非 `-`, 无 `REFUSED=`); (b) 对应 `PREPARED\t<NAME>\tid=...\tsha256=...` 齐备(无 `PREP_FAILED`); (c) `PREBUILD` 满足 `materials>0`、`hit+built==materials`、`failed=0`、`deferred=0`。 | `tools/affected.cx:1388-1441,1506-1511,2005-2043`, `car1-unstage/material.log:1-3`, `c1-devsvc-a-material.log:1-5`, `research/agy-h/car2-prewarm-list.md` 第 5 节 |
| (5) 脏 `PATH` 遮蔽 `cc` 与长 `TMPDIR` 截断 | `0.0.106` `W9998` 因 `moltbaby/bin/cc` 排在 `/usr/bin/cc` 之前导致 `cc -std=c99` 报错退出; `W0150` 因长 `TMPDIR` 截断摘要路径 (`path-b-proof.md:56`)。 | 每次后台派发守护内层重新执行 `export PATH="/usr/bin:/bin:$HOME/.local/bin"`, 并留存 `test "$(command -v cc)" = /usr/bin/cc`; 全局固定短 `export TMPDIR=/tmp` (`<=20` 字节)。 | `release/evidence/0.0.106/path-b-proof.md:36-40,56`, `car2-runbook.md:15-18,118` |
