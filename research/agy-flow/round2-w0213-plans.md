# 第二车 0.0.107 W0213(check-daily-compile-smoke)停机后两种预案(B-t vs 路径 C)精确时间表与执行序列

- 版本与状态:`v2 B-t 裁定版 (2026-11-25 06:45 +0800, PGID 3163300/PID 3615200 已于 06:43:35 自然退净; cc 裁定路径 B-t 两个单文件提交落车后 --resume 续跑 W0213..W0240)`
- **0.0.108 候选改进登记(`cc` 06:43 裁定)**:
  - **问题**:外层启动包装中的 `for attempt in 1 2; do ... rc=$?; [ "$rc" -eq 0 ] && exit 0; [ "$rc" -eq 2 ] && exit 2; done` 在 `prerun-drive.cx` 触发 `PRERUN_PRODUCT_RED_STOP` 以 `rc=1` 退出时,会误把 `rc=1` 当作瞬态失败自动重跑第二轮 `attempt=2`(`--resume`),违背「产品红停绝不自动 resume」纪律。
  - **整改(`0.0.108` 候选 + 本次 `/tmp/r2-resume.sh` 已立即生效)**:外层启动包装一律改为单次 `exec sh ./unisacc.com tools/prerun-drive.cx -- run ...`(不包 `for attempt` 循环),或对 `rc=1` 显式记账直退、不自动重试。
- 当前基线事实:
  - 第二轮封口提交:`F4=63354b4a2cb707ecc923a89f0df6030149fa697f`,候选 `APE2=b773c7e221ab059e935276f41c0ac4070737f2ae6c548cd56ea8b6b0acc53c63`(`8991230 B`),`SID2=37ff46cd02171c8ad2d3ba04dd084c296f5290a992077f8935e8b853e67d3238`。
  - 预跑进度:`W0000..W0212` 共 213 个窗口中:
    - **`W0000`(`module-units-forward`)已在 3615200 自动二轮 warmup 中以 `8520 ms` `PASS rc=0` 转绿**!
    - 已 `PASS rc=0` 窗口:`208 / 241` 窗(`W0000..W0014`、`W0018..W0092`、`W0094..W0212`)。
    - 待单批补跑/处理的非产品红窗口仅剩 4 项:`W0015`(`precommit-list-cap`)、`W0016`(`ratchets-tool-full`,路径 B `check-ratchets-19`)、`W0017`(`tsv-lint-root`)、`W0093`(`slow-split`)。
    - `W0213`(8 批 `normal` 窗):前 7 批(`tools-compile-133`、`facts-current`、`struct-member-capacity-s2040-run`、`tools-compile-077`、`struct-member-capacity-s2040-asm`、`optimization-corpus-7-o0-o1`、`unit-frontcache-budget`)全部 `PASS rc=0`;第 8 批 `check-daily-compile-smoke` 在 `attempt=1` 与单批 `attempt=2`(`3165 ms`, `assertions=6`)报 `FAIL rc=1`。
    - 剩余未跑窗口:`W0213..W0240` 共 **28 个 `normal` 8 并发快窗**(每窗约 15-25 秒,28 窗合计仅需 **~10-12 分钟**)。

---

## 预案一:路径 B-t(判定 `check-daily-compile-smoke` 为检查夹具/测试表缺陷,不涉编译器/运行库产品代码,不重起链)

### 适用前提
- `cdx`/`ompgpt` 分诊确认 `check-daily-compile-smoke` 失败源于测试夹具(`tools/check-daily-compile-smoke.cx` 或测试表/固定输入)合同欠账,产品生成物 `APE2`(`b773c7e2...`)与 `SID2`(`37ff46cd...`)字节无需变动。

### 精确时间表(总耗时约 **48 分钟**,从 `T0` 分诊拍板到 `Step 7 Y2` 公开 `Latest`)

| 阶段 | 相对时刻 | 预计钟点(以 `T0=06:50` 计) | 动作与产出 | 责任窗 |
|---|---|---|---|---|
| **B1. 确认旧守护退净 + 落夹具补丁** | `T0 + 0m -> +4m` | `06:50 -> 06:54` | 确认 `PGID 3163300` / `PID 3615200` 已完全退净;在 `v0.0.107` 车树(`$W`)一同落入 `check-ratchets-19` 与 `check-daily-compile-smoke` 夹具补丁并提交(二次夹具提交 `F4b`,不改 `SID2`)。 | `agyneon` / `cc` |
| **B2. 同步夹具至 `frozen-root` 并失效旧 layout 收据** | `T0 + 4m -> +5m` | `06:54 -> 06:55` | 将新夹具文件同步至 `$PR2/frozen-root`,移除 `$PR2/strict-W0213.tsv` 与 `/tmp/unisacc-qualify-b773c7e2.../ready` + `$PR2/.lane-tmp-{0,1}/unisacc-qualify-b773c7e2.../ready`(使 `dvqualify` 用新夹具重建 `tree-*`)。 | `agy-flow` |
| **B3. `--resume` 续跑剩余 28 个快窗(`W0213..W0240`)** | `T0 + 5m -> +16m` | `06:55 -> 07:06` | 以同参同环境(不带 `for attempt` 循环,单次 `timeout -k 10s 5400s` + `--window-lanes 2 --resume --max-jobs 8 --keep-going`)从 `W0213` 续跑至 `W0240` 出 `PRERUN_RUN status=DONE`。 | `agy-flow` |
| **B4. 整机空闲串行补跑剩余 4 窗(`W9915/W9916/W9917/W9993`)** | `T0 + 16m -> +23m` | `07:06 -> 07:13` | 因 `W0000` 已在 `strict-W0000.tsv` 转 `PASS 0 (8520ms)`,仅需串行补跑 `W9915`(`precommit-list-cap`)、`W9916`(`ratchets-tool-full`)、`W9917`(`tsv-lint-root`)、`W9993`(`slow-split`),每批先备材料再 `jobs=1` 单批跑;同刻在 `$FROZEN_ROOT` 产 `ratchets.txt`(`19/0`)。 | `agy-flow` |
| **B5. `merge/archive` -> `--pack` -> `--warm` -> `--dry` -> Step 1 `X`** | `T0 + 23m -> +27m` | `07:13 -> 07:17` | 4 分钟标准序列:`prerun-drive merge` & `archive`(`PASS=517 FAIL=0 UNKNOWN=0`) -> `scrub-home-paths` -> `release-run --pack` -> `release-exec --warm` & `--dry` -> `Step 1 commit-x`。 | `agy-flow` |
| **B6. Step 2 `draft` + Check 5 + Step 3 `ci` (6 目标)** | `T0 + 27m -> +43m` | `07:17 -> 07:33` | 上传私有草稿 release + 独立下载核验 `slice-win-arm64` & `pkg.bin`(`GUARD_107_DRAFT_SLICE_READY=PASS`) + GitHub Actions 六目标 `release.yml` 全绿。 | `agy-flow` |
| **B7. Step 4 `Y1` -> Step 5 `publish` -> Step 6 `readback` -> Step 7 `Y2` -> `R33`** | `T0 + 43m -> +48m` | `07:33 -> 07:38` | 封存 CI 证据 `Y1` -> 公开 `0.0.107 Latest` -> 外部资产回读核 SHA + `Y2` -> `release-post --execute` -> `old-product.cx` 建 `0700` 缓存。 | `agy-flow` |

### 预案一(B-t)关键执行命令(B2 -> B3)

```sh
R="$HOME/repos/unisacc"
V="0.0.107"
W="$R/.claude/worktrees/v$V"
REV="63354b4a2cb707ecc923a89f0df6030149fa697f"
CHAIN2="$HOME/.cache/unisacc/chains/bg-b1/$REV"
APE2="$CHAIN2/ape-final/unisacc-next.com"
PR2="$HOME/.cache/unisacc/prerun/v$V-63354b4a2c"
FROZEN_ROOT="$PR2/frozen-root"

# 1. 确认旧守护已退净
ps -g 3163300 >/dev/null 2>&1 && { echo "REFUSE: PGID 3163300 still alive"; exit 2; }

# 2. 同步车树 $W 上已提交的夹具补丁至 $FROZEN_ROOT 并清理 layout 缓存 ready 收据及 strict-W0213.tsv
cp -f "$W/tools/check-ratchets.cx" "$FROZEN_ROOT/tools/check-ratchets.cx"
cp -f "$W/tools/check-daily-compile-smoke.cx" "$FROZEN_ROOT/tools/check-daily-compile-smoke.cx"
rm -f "$PR2/strict-W0213.tsv" "$PR2/strict-W0213.tsv.stderr"
rm -rf /tmp/unisacc-qualify-b773c7e2* "$PR2"/.lane-tmp-0/unisacc-qualify-b773c7e2* "$PR2"/.lane-tmp-1/unisacc-qualify-b773c7e2*

# 3. 单次(无外层 for 循环!)重开 --resume 续跑 W0213..W0240 (28 窗)
cd "$W"
setsid nohup timeout -k 10s 5400s sh -c '
  export PATH="/usr/bin:/bin:$HOME/.local/bin"
  export TMPDIR=/tmp
  unset UNISACC_INTEGRATOR DVQUALIFY_NATIVE_RUN_BATCH UNISACC_RUN_CACHE UNISACC_UNIT_CACHE UNISACC_CACHE_DIR UNISACC_BOUND_DEADLINE_MONO UNISACC_E3CACHE
  export UNISACC_BOOTER_CHAIN_OUT="$4"
  exec sh ./unisacc.com tools/prerun-drive.cx -- run \
    --root "$1" \
    --candidate "$2" \
    --out "$3" \
    --window-lanes 2 \
    --resume \
    --max-jobs 8 \
    --keep-going \
    --min-free 2147483648
' sh "$W" "$APE2" "$PR2" "$CHAIN2" >> "$PR2/prerun-daemon.out" 2>&1 < /dev/null &
```

---

## 预案二:路径 C(判定 `check-daily-compile-smoke` 暴露产品/构建支持代码缺陷,需改 `SID` 并起第三轮整链)

### 适用前提
- 分诊确认修复涉及 `SID` 覆盖的产品源码或编译链行为,必须重新生成候选 `APE3` 与 `SID3`。

### 精确时间表(总耗时约 **122 分钟 / 2 小时 02 分**,从 `T0` 封口 `F5` 到 `Step 7 Y2` 公开 `Latest`)

| 阶段 | 相对时刻 | 预计钟点(以 `T0=06:55` 计) | 动作与产出 | 责任窗 |
|---|---|---|---|---|
| **C1. 修复落车 + 封口 `F5` + 封存第二轮证据** | `T0 + 0m -> +5m` | `06:55 -> 07:00` | 一同落入产品修复 + `check-ratchets-19` 夹具补丁,在 `v0.0.107` 与 `main` 同步封口 `F5`(延冻结 deadline);将第二轮 `release/evidence/0.0.107` 与 `src-dir` 封存至 `~/.unisacc-release-evidence-backups/0.0.107/round2-63354b4a2c/`。 | `ompgpt`/`agyneon`/`cc`/`agy-flow` |
| **C2. 第三轮整链(`REV=F5`)** | `T0 + 5m -> +32m` | `07:00 -> 07:27` | `bg-candidate.cx` 起第三轮整链(前两轮实测均为 `26.2 - 26.4 分钟`);期间更新 `priority-newbatches-107.txt` 把 `check-daily-compile-smoke` 加入置顶集并从置顶集移除冷材料重批 `module-units-forward` / `precommit-list-cap` / `tsv-lint-root`(避免它们抢在 `lane-key-probes` 前冷超时)。 | `cc` / `agy-flow` |
| **C3. 链止 `P1 -> P3` + `cdx` 对账 + `ompgpt` 双轮 kit (17/0)** | `T0 + 32m -> +35m` | `07:27 -> 07:30` | `P1 -> P3` 生成 `fixedpoint-6targets.tsv`、`ape-receipt.txt`、`src-dir`(`GUARD6` 零 `F1/F4` 残留);`ompgpt` 双轮 `tlib-kit` 17/0 放行。 | `agy-flow`/`cdx`/`ompgpt` |
| **C4. 第三轮预跑(`241` 窗,2 车道)** | `T0 + 35m -> +95m` | `07:30 -> 08:30` | `prerun-inputs` -> `prerun-drive plan`(核 `W0000` 恢复为 `lane-key-probes`,置顶快批 `W0001..W0020` 含 `check-daily-compile-smoke` 与 `c99-array-param-typedef`) -> 启动单次 `prerun-drive run`(无外层 `for attempt` 循环! 导出 `UNISACC_BOOTER_CHAIN_OUT=$CHAIN3`, `timeout 5400`)。 | `agy-flow` |
| **C5. 空闲补跑(`slow-split` 若触发) -> `merge/archive` -> `--pack` -> `--dry` -> Step 1 `X`** | `T0 + 95m -> +101m` | `08:30 -> 08:36` | 若 `slow-split` 贴限则单批补跑 -> `merge/archive` -> `release-run --pack` -> `release-exec --warm` & `--dry` -> `Step 1 commit-x`。 | `agy-flow` |
| **C6. Step 2 `draft` + Check 5 + Step 3 `ci` (6 目标)** | `T0 + 101m -> +117m` | `08:36 -> 08:52` | 草稿上传 + Check 5 切片下载核验 + 六目标 CI 全绿。 | `agy-flow` |
| **C7. Step 4 `Y1` -> Step 5 `publish` -> Step 6 `readback` -> Step 7 `Y2` -> `R33`** | `T0 + 117m -> +122m` | `08:52 -> 08:57` | `0.0.107` 正式公开 `Latest`,仍比 `0.1.0` 封口起链最晚红线(`13:40 +0800`)提前 **4 小时 40 分钟**! | `agy-flow` |
