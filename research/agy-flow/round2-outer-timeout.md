# 第二车 0.0.107 第二轮预跑外层 timeout(07:09:02)接续预案与预跑完 -> Step 1 X 四分钟序列

- 版本与状态:`v1 定稿 (2026-11-25 05:45 +0800)`
- 关联文档:
  - `research/agy-flow/round2-diffs.md`
  - `research/agy-flow/keepgoing-rerun-plan.md`
  - `research/ccogweco/prerun-done-to-x.md`
  - `research/agy-flow/car2-stage-c-run.md`
- 第二轮固定身份参数:
  - `V=0.0.107`
  - `REV=F4=63354b4a2cb707ecc923a89f0df6030149fa697f`(`63354b4a2c`)
  - `CHAIN2=$HOME/.cache/unisacc/chains/bg-b1/63354b4a2cb707ecc923a89f0df6030149fa697f`
  - `APE2=$CHAIN2/ape-final/unisacc-next.com`(`sha256=b773c7e221ab059e935276f41c0ac4070737f2ae6c548cd56ea8b6b0acc53c63`, `bytes=8991230`)
  - `SID2=37ff46cd02171c8ad2d3ba04dd084c296f5290a992077f8935e8b853e67d3238`
  - `PR2=$HOME/.cache/unisacc/prerun/v0.0.107-63354b4a2c`(`241 窗 / 517 批`)
  - `S2=$HOME/.cache/unisacc/release-v0107-state-63354b4a2c/pack`
  - `SRC_DIR=$HOME/.unisacc-release-evidence-backups/0.0.107/src-dir`
  - 预跑起跑时刻:`T_prerun2 = 05:39:02 +0800`(`epoch=1795556342`),`PID/PGID = 3163300`
  - 外层 `timeout -k 10s 5400s` 到期时刻:**`07:09:02 +0800`(`epoch=1795561742`)**,`SIGKILL` 兜底时刻 **`07:09:12 +0800`(`epoch=1795561752`)**

---

## (1) 外层 timeout 到时(07:09:02)保守档盯防与窗口边界接续序列

### Step T-1:`07:00` 起每 3 分钟(`07:00`、`07:03`、`07:05`)只读核 `done` 数与窗口边界

```sh
R=$HOME/repos/unisacc
V=0.0.107
W=$R/.claude/worktrees/v$V
REV=63354b4a2cb707ecc923a89f0df6030149fa697f
PR2=$HOME/.cache/unisacc/prerun/v$V-${REV:0:10}

# 1. 调 prerun-watch.cx 只读快照 + 统计去重已完成窗数、剩余窗数与当前在途双车道窗号
sh "$R/unisacc.com" "$R/tools/prerun-watch.cx" -- --out "$PR2" || true
DONE_W=$(ls "$PR2"/strict-W0*.tsv 2>/dev/null | xargs grep -l '^SUMMARY	' 2>/dev/null | wc -l)
REMAIN_W=$((241 - DONE_W))
HAS_DONE=$(grep -c '^PRERUN_RUN	status=DONE' "$PR2/supervise.log" 2>/dev/null || echo 0)
LAST_LANE=$(grep '^PRERUN_LANES	' "$PR2/supervise.log" | tail -n 1)
echo "CHECK_AT_$(date '+%H:%M:%S') DONE_W=$DONE_W/241 REMAIN_W=$REMAIN_W HAS_DONE=$HAS_DONE LAST_LANE=[$LAST_LANE]"
```

- **分流判据(`cc` 06:09 保守档裁定)**:
  - `(a)` 若 `HAS_DONE=1` 或 `REMAIN_W == 0`:预跑已在 `07:09:02` 前自然完成,**无需 resume**,立即执行 `/tmp/r2-post-prerun-rerun.sh` 进入下方第 `(2)` 节「预跑完 -> 单批串行补跑 -> merge -> Step 1 X 四分钟序列」。
  - `(b)` 若在 **`07:05`** 采样时仍有 **`REMAIN_W > 6`(超过 6 窗未完)**:
    - 在 **`07:08:30`** 前后盯住 `supervise.log` 的窗口边界(`PRERUN_LANE_WALL` 刚落地、下一对 `PRERUN_LANES` 刚起或将起之时);
    - 一旦当前双车道对(`PRERUN_LANE_WALL`)完成且距 `07:09:02` 不足一整窗耗时(或到 `07:08:30` 确认在途窗号后主动在窗口边界收束 `PGID=3163300`),立即在窗口边界停下旧守护并按 `Step T-2 -> Step T-3`(`sh /tmp/r2-resume.sh`)以同参同环境 + 外层 `timeout 5400` 重开 `--resume`,避免外层 `timeout` 杀在窗口执行正中间。

### Step T-2:07:09:12 后确认 `PGID=3163300` 退净并识别被杀在途窗

```sh
# 在 07:09:12 +0800 (epoch >= 1795561752) 执行:
ps -o pid,pgid,stat,etime,cmd -g 3163300 2>/dev/null || true
if ps -g 3163300 >/dev/null 2>&1; then
  kill -TERM -- -3163300 2>/dev/null || true
  sleep 2
  kill -KILL -- -3163300 2>/dev/null || true
fi
ps -g 3163300 >/dev/null 2>&1 && { echo "PGID_3163300_STILL_ALIVE=FAIL"; exit 1; } || echo "PGID_3163300_CLEAN=PASS"

# 核对被外层 timeout 打断时正在跑的窗口(strict-WNNNN.tsv 尚无 SUMMARY 行的半截文件):
for f in "$PR2"/strict-W0*.tsv; do
  [ -f "$f" ] || continue
  wbase=$(basename "$f")
  [ "$wbase" = "strict-W0000.tsv" ] && continue
  if ! grep -q '^SUMMARY	' "$f" 2>/dev/null; then
    echo "INTERRUPTED_WINDOW_FILE=$f (expected to be re-run by --resume)"
  fi
done
tail -n 15 "$PR2/supervise.log"
```

- **机制说明**:`tools/prerun-drive.cx -- run ... --resume` 扫描 `windows.tsv` 时,凡 `strict-WNNNN.tsv` 已含完整 `SUMMARY` 且 `FAIL=0 UNKNOWN=0 INVALID=0` 的已完成窗口均自动跳过(`SKIP`);外层 `timeout` 到点时被中断的在途双车道窗口(最多 2 个窗)因无 `SUMMARY` 或未写完,在 `--resume` 重开时会被自动重跑。
- **红停排除铁律**:重开前必须确认 `tail -n 20 "$PR2/supervise.log"` 中**无 `PRERUN_PRODUCT_RED_STOP`**(若有产品红停则严禁 `--resume`,立即上报 `1:cc`)。

### Step T-3:07:09:15 用同参同环境 + 外层 `timeout 5400` 重开 `--resume`

```sh
R=$HOME/repos/unisacc
V=0.0.107
W=$R/.claude/worktrees/v$V
REV=63354b4a2cb707ecc923a89f0df6030149fa697f
CHAIN2=$HOME/.cache/unisacc/chains/bg-b1/$REV
APE2=$CHAIN2/ape-final/unisacc-next.com
PR2=$HOME/.cache/unisacc/prerun/v$V-${REV:0:10}

grep -q 'PRERUN_PRODUCT_RED_STOP' "$PR2/supervise.log" && { echo "REFUSE_RESUME_ON_PRODUCT_RED_STOP"; exit 2; }

cd "$W"
setsid nohup timeout -k 10s 5400s sh -c '
  export PATH="/usr/bin:/bin:$HOME/.local/bin"
  export TMPDIR=/tmp
  unset UNISACC_INTEGRATOR DVQUALIFY_NATIVE_RUN_BATCH UNISACC_RUN_CACHE UNISACC_UNIT_CACHE UNISACC_CACHE_DIR UNISACC_BOUND_DEADLINE_MONO UNISACC_E3CACHE
  export UNISACC_BOOTER_CHAIN_OUT="$4"
  sh ./unisacc.com tools/prerun-drive.cx -- run \
    --root "$1" \
    --candidate "$2" \
    --out "$3" \
    --window-lanes 2 \
    --resume \
    --max-jobs 8 \
    --keep-going \
    --min-free 2147483648
' sh "$W" "$APE2" "$PR2" "$CHAIN2" >> "$PR2/prerun-daemon.out" 2>&1 < /dev/null &
RESUME_PID=$!
echo "$RESUME_PID" > "$PR2/prerun-daemon-resume.pid"
echo "R2_PRERUN_RESUMED ts='$(date '+%Y-%m-%d %H:%M:%S %z')' epoch=$(date +%s) pid=$RESUME_PID pgid=$RESUME_PID"
```

- 重开后立即发信封告知 `1:cc` 与 `1:agy-h` 新的 `RESUME_PID/PGID` 与剩余窗数。

---

## (2) 预跑完 -> 单批补跑(merge 前) -> merge/archive -> pack -> warm -> dry -> Step 1 X 四分钟序列

> 依据 `research/ccogweco/prerun-done-to-x.md` 与 `research/agy-flow/keepgoing-rerun-plan.md`:当 `supervise.log` 出现 `PRERUN_RUN status=DONE` 且 `prerun-drive.cx` 进程退出后,**先在 `merge` 之前完成整机空闲单批补跑(`W0000` `module-units-forward` 及任何其他 `KEEP_GOING` 窗)**,再执行 `merge/archive -> pack -> warm -> dry -> Step 1 X`。

### Phase M0 (`T_done + 0s -> +50s`):确认预跑退出 + `merge` 前执行单批补跑(`W0000` `module-units-forward` 等)

```sh
R=$HOME/repos/unisacc
V=0.0.107
W=$R/.claude/worktrees/v$V
REV=63354b4a2cb707ecc923a89f0df6030149fa697f
CHAIN2=$HOME/.cache/unisacc/chains/bg-b1/$REV
APE2=$CHAIN2/ape-final/unisacc-next.com
SID2=37ff46cd02171c8ad2d3ba04dd084c296f5290a992077f8935e8b853e67d3238
PR2=$HOME/.cache/unisacc/prerun/v$V-${REV:0:10}
FROZEN_ROOT=$PR2/frozen-root
S2=$HOME/.cache/unisacc/release-v0107-state-63354b4a2c/pack
SRC_DIR=$HOME/.unisacc-release-evidence-backups/0.0.107/src-dir

export PATH="/usr/bin:/bin:$HOME/.local/bin"
export TMPDIR=/tmp
unset UNISACC_INTEGRATOR DVQUALIFY_NATIVE_RUN_BATCH UNISACC_RUN_CACHE UNISACC_UNIT_CACHE UNISACC_CACHE_DIR UNISACC_BOUND_DEADLINE_MONO UNISACC_E3CACHE
export UNISACC_BOOTER_CHAIN_OUT="$CHAIN2"
test "$(command -v cc)" = /usr/bin/cc

# 1. 确认预跑已 status=DONE 且无在途 dvqualify/prerun-drive 抢核
grep '^PRERUN_RUN	status=DONE' "$PR2/supervise.log"
grep -E 'PRERUN_INCOMPLETE|PRERUN_KEEP_GOING' "$PR2/supervise.log" || true

# 2. 在 frozen-root 上对补跑集(W0000 module-units-forward、W0015 precommit-list-cap、W0016 ratchets-tool-full[待 check-ratchets.cx 补丁落车后]、W0017 tsv-lint-root)逐批串行:
#    先独立预备该批材料(--materials-dump --ids <id> --prepare-materials -j 1),再以 jobs=1 新 --out 单批真跑
cd "$FROZEN_ROOT"
for pair in "W9900:module-units-forward" "W9915:precommit-list-cap" "W9916:ratchets-tool-full" "W9917:tsv-lint-root"; do
  wtag="${pair%%:*}"
  bid="${pair#*:}"
  sh "$APE2" tools/affected.cx -- --materials-dump --ids "$bid" --prepare-materials -j 1
  out_f="$PR2/strict-${wtag}-${bid}.tsv"
  rm -f "$out_f" "$out_f.stderr"
  timeout -k 5s 60s sh ./unisacc.com tools/dvqualify.cx -- . \
    --candidate "$APE2" \
    --ids "$bid" \
    --out "$out_f" \
    --jobs 1 \
    --segment-batches 1 \
    2> "$out_f.stderr"
  grep -E "^${bid}\t|^RAW\tBATCH_RUN\tid=${bid}\t|^SUMMARY\t" "$out_f"
done

# 3. W0093 (slow-split) 先备材料再以 jobs=1 单批补跑至 $PR2/strict-W9993-slow-split.tsv
sh "$APE2" tools/affected.cx -- --materials-dump --ids slow-split --prepare-materials -j 1
OUT_W93="$PR2/strict-W9993-slow-split.tsv"
rm -f "$OUT_W93" "$OUT_W93.stderr"
timeout -k 5s 60s sh ./unisacc.com tools/dvqualify.cx -- . \
  --candidate "$APE2" \
  --ids slow-split \
  --out "$OUT_W93" \
  --jobs 1 \
  --segment-batches 1 \
  2> "$OUT_W93.stderr" || true
grep -E '^slow-split\t|^RAW\tBATCH_RUN\tid=slow-split\t|^SUMMARY\t' "$OUT_W93" || true
```

### Phase M1 (`T_done + 50s -> +90s`):`prerun-drive merge` 与 `archive` + 家目录路径脱敏 + `INDEX.tsv` 齐套

```sh
cd "$W"

# 1. 将所有补跑 strict-W99*.tsv 备份进 release/evidence/0.0.107/ 并脱敏
for f in "$PR2"/strict-W99*.tsv; do
  [ -f "$f" ] || continue
  base=$(basename "$f")
  cp -f "$f" "$W/release/evidence/0.0.107/$base"
  sh ./unisacc.com tools/scrub-home-paths.cx -- --write "$W/release/evidence/0.0.107/$base"
done

# 2. 执行 merge 与 archive,并将 strict-final.tsv 拷入 release/evidence/0.0.107/strict.tsv
rm -f "$W/release/evidence/0.0.107/prerun-windows.tsv" "$W/release/evidence/0.0.107/prerun-supervise.log"
sh ./unisacc.com tools/prerun-drive.cx -- merge --in-dir "$PR2" --out "$PR2" --version "$V"
sh ./unisacc.com tools/prerun-drive.cx -- archive --in-dir "$PR2" --repo "$W" --version "$V"
cp -f "$PR2/strict-final.tsv" "$W/release/evidence/0.0.107/strict.tsv"
sh ./unisacc.com tools/scrub-home-paths.cx -- --write \
  "$W/release/evidence/0.0.107/strict.tsv" \
  "$W/release/evidence/0.0.107/prerun-supervise.log" \
  "$W/release/evidence/0.0.107/prerun-windows.tsv"

# 3. 核验 SUMMARY 行 PASS=517 FAIL=0 UNKNOWN=0 INVALID=0 与 expected-unmeasured.tsv / deferred-batches.tsv / INDEX.tsv
tail -n 5 "$W/release/evidence/0.0.107/strict.tsv"
grep -rnE '/home/[a-zA-Z0-9_-]+/|/Users/[a-zA-Z0-9_-]+/' "$W/release/evidence/0.0.107/" && exit 1 || echo "EVIDENCE_HOME_PATHS=0"
```

### Phase M2 (`T_done + 90s -> +150s`):`release-run --pack` -> `release-exec --warm` -> `release-exec --dry` -> `precommit` 预热

```sh
cd "$W"
mkdir -p "$(dirname "$S2")"

# 1. release-run.cx -- --pack (产出一套锁定在 $S2 的打包收据与 release/candidate.tsv)
sh ./unisacc.com tools/release-run.cx -- --pack \
  --version "$V" \
  --candidate "$APE2" \
  --src-dir "$SRC_DIR" \
  --state-dir "$S2" \
  --source-sha "$REV" \
  --source-id "$SID2"

# 2. release-exec.cx -- --warm 与 --dry (确认 7 步 0..6 全部 DRY OK 且 G1..G10 全绿)
sh ./unisacc.com tools/release-exec.cx -- --warm --version "$V" --state-dir "$S2"
sh ./unisacc.com tools/release-exec.cx -- --dry --version "$V" --state-dir "$S2"

# 3. 预热 precommit.cx (防止 Step 1 X 内 git commit 钩子冷编译超 30s)
sh ./unisacc.com tools/precommit.cx -- release/candidate.tsv
```

### Phase M3 (`T_done + 150s -> +240s`):Step 1 `X` 封口提交并推远端(`v0.0.107` 车树 + 对齐 `main`)

```sh
cd "$W"
# 执行 Step 1 X (生成提交 "release: seal 0.0.107 candidate and local qualification evidence" 并推送)
UNISACC_INTEGRATOR=1 sh ./unisacc.com tools/release-exec.cx -- --step commit-x --version "$V" --state-dir "$S2"

X_SHA=$(git -C "$W" rev-parse HEAD)
echo "STEP1_X_SEALED X_SHA=$X_SHA"
```

- Step 1 `X` 完成后立即将 `X_SHA` 发信封给 `1:cc`、`1:cdx`、`1:agy-h`、`1:ccogweco`,并直推 Step 2 (`draft`) -> Check 5 (`slice-win-arm64` + `pkg.bin` 下载字节核验) -> Step 3 (`ci` 6 目标)。
