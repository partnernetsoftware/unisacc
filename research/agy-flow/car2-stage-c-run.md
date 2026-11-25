# 第二车 0.0.107 阶段 C 封口与起链执行表(02:20 预备 -> 02:50/02:55 起链)

- 适用范围:第二车 `v0.0.107`(工作树 `~/repos/unisacc/.claude/worktrees/v0.0.107`)从门 5 缓存预备(`02:20 -> 02:35`)、阶段 C 四步封口(`02:35 -> 02:47`)、门 22 封口后预备(`02:47 -> 02:48`)到 `chain-ready` 全量过门与整链正式启动(`02:48 -> 02:52`,最迟 `02:55`)的执行序列。
- 最高裁定与时序纪律:
  - `(1)` 目标起链时刻:`2026-11-25 02:50 +0800`(含全量 `chain-ready` 串行 `4-5 分钟` 实测预算则最迟 `02:55`,不放宽任何门限)。`#14 v2`(`readback-prewarm-gate-v2`)与 `#8 v4`(`check-tool-material-macro`)、以及 `#15`(`release-body-from-notes`)在阶段 C 封口前经复核且 `precommit` 绿则入车;未绿即不入车,起链不等(链后走路径 B `R40 B-t` 非 SID 变体或按 `fable` 裁定顺延 `0.1.x`)。
  - `(2)` **`fable` 车序裁定**:`0.0.107` 公开后**直接起 `0.1.0` 封版车(零代码生成变化,只注入版本槽)**,`0.0.108+126`、`0.0.109`、`0.0.125` 全部顺延 `0.1.x`;唯一例外为 `F1` 转发桩批(`ac483247c7` 对应批 `fwd-cap` 与 `fwdstub-replay`,`0.0.126` 的唯一分支)若在 `0.0.107` 预跑中不绿,才在 `0.0.107` 后插一辆只含 `0.0.126` 的 `0.0.108` 再起 `0.1.0`。
  - `(3)` 共享根不升级:本机 Root 保持当前已热的 `0.0.105`(`934c7c0bc0`,已备 `.unisacc-root-backups`),Root 升级至 `0.0.106` 推迟到 `0.0.107` 起链之后。
  - `(4)` 门 5 与门 22 缓存键分置规则(`cdx` 源码核定):
    - **门 5(`5-candidate-chain`)在 `F` 之前预备**:`tools/cxlib/runneridentity.cx:60-99` 与 `tools/cxlib/runnerprep.cx:77-87` 按启动产品和源码闭包内容/路径成键,不含 `REV` 或 `HEAD`;`tools/chain-ready.cx:1719-1725` 在 `$TMPDIR/unisacc-cr-prep-cache` 下查找缓存。因此在最后一个会改源码闭包的补丁入车后、阶段 C 之前(`02:20 -> 02:35`),使用与 `chain-ready` 完全相同的 `cwd`(`$W`)与 `TMPDIR`(`$S/tmp`)按 `research/cdx/car2-chain-ready-gates.md` 正式语法串行跑两次 `--prepare "$S/tmp/unisacc-cr-prep-cache" tool` 与 `generator`,封口 `F`(只改冻结表、版本槽、派生表)后仍 100% 命中(`reused=1 reason=identity-hit`;注:命中预备只省去冷编工具与生成器,正式五分片执行热态仍需约 `51.5 秒`,计入全门预算)。
    - **门 22(`22-premerge-union`)与门 19(`19-chain-preflight`)必须在 `F` 之后、同 `F` 同 `TMPDIR` 预备**:
      - 门 22:`tools/premerge-union.cx:494-510,530,546-554` 缓存键含 `base`(`main`)与 `branch`(`v0.0.107`)两个完整提交 SHA;`F` 新提交一落,旧预合树立即失效。因此门 22 的 `--shard prep`(`cdx` 热测 `12.430 秒`、冷对照 `49.353 秒`)必须放在单笔封口 `F` 之后、同 `TMPDIR`(`$S/tmp`)下执行;`premerge-union.cx:672+` 在 stamp 命中时只省去预合树合成,全门再次跑 `prep` 时仍重查生成器(`2.653s` + 四片 `16.5s/16.2s/8.2s/14.2s`,合计 `57.8s`),不能算零。另 `chain-ready` 默认 `base` 取本地 `main`(`premerge-union.cx:451`),起链前由 `cc` 核保本地 `main` 与 `origin/main` 同 SHA。
      - 门 19:`cdx` 实测(`research/cdx/pre-chain-timings.md`)门 19 不预备直跑会触 `54 秒` 外限(`rc=124`),必须在 `F` 之后、同 `F` 同 `TMPDIR`(`$S/tmp`)顺序预备两项:`chain-preflight.cx -- --item chain-to-snap --root "$W" --repo "$W" --rev "$REV"`(热态 `0.481 秒`,绑定 `F=REV` 的快照)与 `sixtarget-precompile.cx -- --mode headers --deadline-s 50`(`10.764 秒`,`6/6`);预备后全门内门 19 耗时 `37.845 秒`(`nodes=9 ms=37661`,距 54 秒限余 `16.155 秒`)。
  - `(4)` 阶段 B 期间落 `B1` 与其余补丁时不逐笔跑全量 `derive --write`,只在阶段 C 第三步统一重算一次。

---

## (1) 02:20 -> 02:52(最迟 02:55)阶段 C 时点总表

| 步骤 | 时点窗口 | 预计分钟 | 负责窗口 | 核心目标 | 验收判据(期望原行) |
|---|---|---|---|---|---|
| 前置预备:门 5 缓存 | `02:20 -> 02:35` (闭包补丁落齐后) | `2m` | `agy-flow` / `agyneon` | 固定会话 `TMPDIR="$S/tmp"`,在 `$W` 串行跑 `check-candidate-chain.cx -- --prepare "$S/tmp/unisacc-cr-prep-cache" tool` 与 `generator` | 两个角色(`tool` 与 `generator`)均在 `$S/tmp/unisacc-cr-prep-cache` 下输出 `PREPARED`(再跑一次验证 `reused=1`) |
| 预备门:收口与清场 | `02:35 -> 02:37` | `2m` | `cc` + `agyneon` | 阶段 B 准入截断;清空全机后台编译/预检进程;核工作树常规启动器文件 | `devsvc status` `procs=0`;`unisacc.com` 与 `unisacc.com.build.json` 均为常规文件(`-f` 且非 `-L`) |
| 第一步:零落后 `main` | `02:37 -> 02:39` | `2m` | `agyneon` / `cc` | `v0.0.107` 合入最新 `origin/main`,核落后数为 `0` 与版本槽 `v0.0.107` | `git rev-list --count HEAD..origin/main` 输出 `0` |
| 第二步:写 `0.0.107` 冻结锚 | `02:39 -> 02:42` | `3m` | `cc` (`DEVSVC_OWNER=cc`) | 拿 `freeze` 锁;在 `main` 落 `0.0.107-FREEZE`(`deadline=1795559400=06:30`)并推 `origin/main`;核本地 `main==origin/main`;合入 `v0.0.107` | `test "$(git -C "$R" rev-parse main)" = "$(git -C "$R" rev-parse origin/main)"` 通过;`v0.0.107` 含 `0.0.107-FREEZE` 行 |
| 第三步:现取 `tlib` 与全量派生 | `02:42 -> 02:46` | `4m` (`实测 114.8s`) | `agyneon` / `cc` | 单跑 `tables-gen`、`batch-packages-gen`、`layer-api --bump-pending`、`iface-hash`、`derive` 并核零漂移与 pin | `TABLES_GEN stale=0 hard=0`;`BUMP_PENDING ok`;`DERIVE_SUMMARY ok=17 drift=0 refused=0`;`check-sourcepackage-pins` `rc=0` |
| 第四步 A:单笔封口 `F=REV` | `02:46 -> 02:47` | `1m` (`实测 5.4s + push`) | `cc` / `agyneon` | 单笔提交封口 `F=REV` 并推入 `origin/v0.0.107`(此后不再改树、不再合 `main`) | `git status --porcelain` 为空;`HEAD..origin/main` 为 `0` |
| 第四步 B:门 22 + 门 19 封口后预备 | `02:47 -> 02:48` | `1m` (`热 23.7s / 冷 60.6s`) | `cc` / `agy-flow` | 在同 `F=REV`、同 `TMPDIR="$S/tmp"` 顺序跑:(1) 门 22 `premerge-union --shard prep`(`12.4s/49.4s`);(2) 门 19 `chain-preflight --item chain-to-snap`(`0.48s`)与 `sixtarget-precompile --mode headers`(`10.8s`) | `PREMERGE_UNION shard=prep rc=0`;`chain-to-snap` `PASS`;`sixtarget-precompile` `6/6 PASS` |
| 第四步 C:`chain-ready` 全量与起链 | `02:48 -> 02:51` (最迟 `02:55`) | `3m` (`e2e 实测 179.4s`) | `cc` (`chain-ready`) + `agy-flow` (`bg-candidate`) | 同一 `TMPDIR="$S/tmp"` 跑全量 `chain-ready.cx`(门 5 `51.3s`、门 19 `47.4s`、门 22 五片 `32.4s`,全量墙钟 `179.365s`,`F` 到全量 `210.346s`);过门即起 4 路整链 | `READY yes failed=none`;`bg-candidate start --rev $REV` 启动且 `status` 见活跃 `owner-pid` |

---

## (2) 分步命令原行、负责窗口与失败处置

### 前置预备:门 5 缓存预备(02:20 -> 02:35 最后一个改闭包补丁入车后执行,预计 2 分钟,负责窗口:agy-flow / agyneon)

- **目标**:创建固定会话目录 `$S`(后续门 22 预备、`chain-ready` 与起链全程复用同一个 `TMPDIR="$S/tmp"`!),在 `$W` 下按正式入口串行准备 `tool` 与 `generator` 两个角色到 `$S/tmp/unisacc-cr-prep-cache`,并复跑一次验证 `reused=1`。
- **命令原行**:

```sh
R=$HOME/repos/unisacc
V=0.0.107
W=$R/.claude/worktrees/v$V
BASE=$HOME/.cache/unisacc/chains/bg-b1
S=/tmp/unisacc-session-car2-stagec
mkdir -p "$S/tmp"
export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:$HOME/.local/bin"
export TMPDIR="$S/tmp"
test "$(command -v cc)" = /usr/bin/cc

cd "$W"
sh ./unisacc.com tools/check-candidate-chain.cx -- --prepare "$S/tmp/unisacc-cr-prep-cache" tool
sh ./unisacc.com tools/check-candidate-chain.cx -- --prepare "$S/tmp/unisacc-cr-prep-cache" generator
sh ./unisacc.com tools/check-candidate-chain.cx -- --prepare "$S/tmp/unisacc-cr-prep-cache" tool
sh ./unisacc.com tools/check-candidate-chain.cx -- --prepare "$S/tmp/unisacc-cr-prep-cache" generator
```

- **失败处置**:
  - 若复跑未报 `reused=1`:检查 `$W/unisacc.com` 是否为常规文件、以及是否有后续补丁又改动了 `tools/` 下被 `check-candidate-chain.cx` 或生成器依赖的 `tools/cxlib/*` 闭包;若有改动,在该改动落定后重跑上述两条 `--prepare`。

---

### 预备门:阶段 B 收口截断与清场(02:35 -> 02:37,预计 2 分钟,负责窗口:cc + agyneon)

- **目标**:对未落补丁做准入截断,确认全机无在途重编译/测试进程,核对工作树启动器为常规文件。
- **命令原行**:

```sh
cd "$W"
DEVSVC_OWNER=cc sh ./unisacc.com tools/devsvc.cx -- status
test -f "$W/unisacc.com" && test ! -L "$W/unisacc.com"
test -f "$W/unisacc.com.build.json" && test ! -L "$W/unisacc.com.build.json"
```

- **失败处置**:
  - 若 `unisacc.com` 或 `unisacc.com.build.json` 为符号链接(`-L`):立即替换为 `$R/unisacc.com` 与 `$R/unisacc.com.build.json` 的真实文件副本,并移走 `~/.cache/unisacc/chains/snap/` 下旧快照(`prd_mempalace.md:66`),随后重跑一次门 5 `--prepare`。
  - 若 `devsvc status` 有活跃测试/编译子进程:等待收尾或按真实 `PID/PGID` 清理后再进第一步。

---

### 第一步:零落后 main(02:37 -> 02:39,预计 2 分钟,负责窗口:agyneon / cc)

- **目标**:将 `origin/main` 最新头合入 `v0.0.107`,确认 `HEAD..origin/main` 落后提交数为 `0`,且 `release/candidate.tsv` 版本槽保持 `v0.0.107`。
- **命令原行**:

```sh
cd "$W"
git fetch --quiet origin
git status --short
git log -5 --oneline origin/main
DEVSVC_OWNER=cc sh ./unisacc.com tools/devsvc.cx -- git merge origin/main
test "$(git rev-list --count HEAD..origin/main)" -eq 0
head -n 2 "$W/release/candidate.tsv"
```

- **失败处置**:
  - 若派生表(`build/iface-hash.tsv`、`build/links.tsv`、`build/tlib-packages.tsv`、`build/table-index.tsv`)出现合并冲突:取版本分支侧后在第三步由生成器统一重算覆盖。
  - 若 `tests/batch-product-inputs.tsv` 出现两侧追加冲突:取两侧追加行的无重并集。

---

### 第二步:写入 0.0.107 冻结锚并同步本地 main 与车树(02:39 -> 02:42,预计 3 分钟,负责窗口:cc)

- **目标**:`cc` 获取 `freeze` 锁(`ttl=7200`),在共享根 `main` 写入 `0.0.107-FREEZE`(`deadline=1795559400`,即 `06:30`)并提交推送到 `origin/main`,**核实共享根本地 `main` 与 `origin/main` 同 SHA**(供 `chain-ready` 门 22 默认 `base=main` 使用),随后合入 `v0.0.107`。
- **命令原行**:

```sh
cd "$R"
git fetch --quiet origin
MAIN_BASE=$(git rev-parse origin/main)
DEADLINE=1795559400
DEVSVC_OWNER=cc sh ./unisacc.com tools/devsvc.cx -- lock acquire freeze --owner cc --ttl 7200

cat <<EOF > "$R/build/freeze.tsv"
# candidate_sid	base_commit	owner	deadline_epoch_s	paths
0.0.107-FREEZE	$MAIN_BASE	cc	$DEADLINE	src cx ext exec include seed kernel tests release
EOF

DEVSVC_OWNER=cc sh ./unisacc.com tools/devsvc.cx -- git commit \
  -m "freeze: arm 0.0.107-FREEZE on main (base=${MAIN_BASE:0:10}, deadline=06:30)" \
  --whole build/freeze.tsv --push

git fetch --quiet origin
test "$(git -C "$R" rev-parse main)" = "$(git -C "$R" rev-parse origin/main)"

cd "$W"
git fetch --quiet origin
DEVSVC_OWNER=cc sh ./unisacc.com tools/devsvc.cx -- git merge origin/main
test "$(git rev-list --count HEAD..origin/main)" -eq 0
cat "$W/build/freeze.tsv"
```

- **失败处置**:
  - 若 `git -C "$R" rev-parse main` 与 `origin/main` 不等:由 `cc` 在共享根确认本地 `main` 已对齐 `origin/main`,确保门 22 默认 `--base main` 读到含 `0.0.107-FREEZE` 的同一提交。

---

### 第三步:现取 tlib 号与全量派生重算(02:42 -> 02:46,预计 4 分钟,负责窗口:agyneon / cc)

- **目标**:在 `0.0.107-FREEZE` 已在车树且零落后 `origin/main` 的前提下,一次性跑 `tables-gen`、单跑 `batch-packages-gen`(避开 `derive` 30 秒并发界)、暂存后跑 `layer-api --bump-pending` 与 `iface-hash --write`,再跑 `derive --write` 与五项 `--check`。
- **命令原行**:

```sh
cd "$W"
sh ./unisacc.com tools/tables-gen.cx -- --write
sh ./unisacc.com tools/batch-packages-gen.cx -- --write
git add -A
DEVSVC_OWNER=cc sh ./unisacc.com tools/layer-api.cx -- --bump-pending
sh ./unisacc.com tools/iface-hash.cx -- --write --table ./build/iface-hash.tsv
sh ./unisacc.com tools/derive.cx -- --write

sh ./unisacc.com tools/tables-gen.cx -- --check
sh ./unisacc.com tools/layer-api.cx -- --check
sh ./unisacc.com tools/iface-hash.cx -- --check --table ./build/iface-hash.tsv
sh ./unisacc.com tools/derive.cx -- --check
sh ./unisacc.com tools/check-sourcepackage-pins.cx -- .
```

- **失败处置**:
  - 若 `layer-api.cx -- --bump-pending` 报 `dirty-package-path`:先 `git add -A` 再重跑 `layer-api.cx -- --bump-pending`。
  - 若 `third-step` 修改了 `tools/cxlib/*` 导致门 5 闭包变化:在第四步 A 提交 `F=REV` 后补跑一次门 5 `--prepare`(`~40s`)。

---

### 第四步(A/B/C):单笔封口 F=REV -> 门 22 与门 19 预备 -> chain-ready 全量与起链(02:46 -> 02:52,最迟 02:55,负责窗口:cc + agy-flow)

- **目标与 `pre-chain-timings.md` 实测预算(`4A -> 4C` 合计 `296.1 秒`(热)至 `342.7 秒`(冷),即 `4.9 -> 5.7 分钟`,起链目标 `02:51 -> 02:55` 以内)**:
  - **4A(`02:46 -> 02:47`,本地实测 `5.353s`:`precommit 4.345s + commit 1.008s`,加 `push` 约 `10-15s`)**:单笔提交封口 `F=REV` 并推入 `origin/v0.0.107`;
  - **4B(`02:47 -> 02:48`,同 `F=REV`、同 `TMPDIR="$S/tmp"` 顺序跑门 22 + 门 19 共三条预备,热态合计 `23.675s` / 冷态 `60.598s`)**:
    - `(1)` 门 22 预合树预备:`premerge-union.cx -- --repo "$W" --shard prep --branch "v$V"`(热态 `12.430s`,冷对照 `49.353s`);
    - `(2)` 门 19 快照预备:`chain-preflight.cx -- --item chain-to-snap --root "$W" --repo "$W" --rev "$REV"`(热态 `0.481s`,绑定最终 `F=REV` 生成 `snap`);
    - `(3)` 门 19 六目标头预备:`sixtarget-precompile.cx -- --mode headers --deadline-s 50`(`10.764s`,`6/6 PASS`);
  - **4C(`02:48 -> 02:51`,端到端实测全量 `179.365 秒` = `2 分 59 秒`,从 `F` 到全量 `READY` 合计 `210.346 秒` = `3 分 30 秒`,见 `research/cdx/car2-chain-ready-e2e.md`)**:使用同一 `TMPDIR="$S/tmp"` 跑全量 `chain-ready.cx`(实测 25 PASS、7/9 人工 UNMEASURED:门 5 `51.285s` `139/0`、门 19 `47.437s` 九节点全绿、门 22 `32.368s` 五片全绿,末行 `READY yes failed=none 179033`),拿到 `READY yes failed=none` 后立即在 `02:51 -> 02:52`(远早于 `02:55`)启动 4 路后台整链。
- **命令原行**:

```sh
cd "$W"
git status --short
DEVSVC_OWNER=cc sh ./unisacc.com tools/devsvc.cx -- git commit \
  -m "v0.0.107 Stage C seal: tlib bump, tables-gen, derive zero-drift for chain start" \
  --whole $(git diff --name-only HEAD) --push

REV=$(git rev-parse HEAD)
OUT=$BASE/$REV
test "$(git rev-list --count HEAD..origin/main)" -eq 0
test "$(git -C "$R" rev-parse main)" = "$(git -C "$R" rev-parse origin/main)"
test -z "$(git status --porcelain)"
echo "SEALED_F_REV=$REV"

unset UNISACC_INTEGRATOR DVQUALIFY_NATIVE_RUN_BATCH UNISACC_RUN_CACHE UNISACC_UNIT_CACHE UNISACC_CACHE_DIR UNISACC_BOUND_DEADLINE_MONO UNISACC_E3CACHE
export TMPDIR="$S/tmp"

# 4B-1: 门 22 封口后预合树预备(绑定 main 与 F=REV 双 SHA, 热 12.4s / 冷 49.4s)
sh ./unisacc.com tools/premerge-union.cx -- --repo "$W" --shard prep --branch "v$V"

# 4B-2: 门 19 封口后两条预备(必须同 F=REV、同 TMPDIR="$S/tmp", 合计 ~11.3s, 使门 19 全门稳定在 37.8s)
sh ./unisacc.com tools/chain-preflight.cx -- --item chain-to-snap --root "$W" --repo "$W" --rev "$REV"
sh ./unisacc.com tools/sixtarget-precompile.cx -- --mode headers --deadline-s 50

# 4C: 全量 chain-ready (~267.0s = 4m27s) 与 4 路后台起链
sh ./unisacc.com tools/bg-candidate.cx -- status --base "$BASE"
DEVSVC_OWNER=cc sh ./unisacc.com tools/chain-ready.cx -- --repo "$W" --root "$W" --rev "$REV" --version "$V" --chain-base "$BASE"

DEVSVC_OWNER=cc sh ./unisacc.com tools/bg-candidate.cx -- start --rev "$REV" --base "$BASE" --deadline-s 5400 -- --parallel 4
sh ./unisacc.com tools/bg-candidate.cx -- status "$REV" --base "$BASE"
```

- **失败处置**:
  - 若 `devsvc git commit --push` 报 `PUSH_REPLAY_REFUSE reason=unowned-pending`:提交已落地,立即执行 `DEVSVC_OWNER=cc sh ./unisacc.com tools/devsvc.cx -- git push $(git rev-list --reverse origin/v0.0.107..HEAD)` 按第一父序推入 `origin/v0.0.107`。
  - 若 `chain-ready` 中门 22(`22-premerge-union*`)在 `prep` 或 `derive-b` 超 55 秒限:核对 `premerge-union --shard prep` 与 `chain-ready.cx` 的 `TMPDIR` 是否同为 `"$S/tmp"`、且本地 `main` 与 `HEAD`(`F=REV`)未移动;若 `derive-b` 内层 `batch-packages-gen` 首次冷跑触 30 秒限(`car2-chain-ready-gates.md:29`),保持同一预合目录空闲重跑一次即可命中热态(`14.935 秒`)。
  - 若 `chain-ready` 中门 5(`5-candidate-chain*`)未命中预备缓存:说明第三步或最后合入的补丁动了 `tools/` 闭包,在同一 `TMPDIR="$S/tmp"` 下按正式语法补跑 `sh ./unisacc.com tools/check-candidate-chain.cx -- --prepare "$S/tmp/unisacc-cr-prep-cache" tool` 与 `generator` 各一次,核 `reused=1` 后重跑 `chain-ready.cx`。

---

## (3) 起链后衔接:预跑提速 S1-S4 合同与 F1(`fwd-cap`)专人盯批规程(fable 裁定)

整链在第四步 C 启动后(`02:50/02:55`),立即按以下五条规程衔接链跑期预热与严格预跑(不改源码,配合 `research/agy-flow/car2-parallel-prep.md` 的 `P1..P6` 并行预做):

### (S1) 值守 2 分钟内 `--resume` 与已审豁免红不停机

- 预跑启动固定带 `--keep-going product-red`;已审豁免红窗不在预跑中途停机等人,`strict.tsv` 仍如实记录原始 `FAIL` 行。
- 若任一窗口因双次 `58s rc=124` 触发 `PRERUN_STOP`,值守窗口(`agy-flow`)必须在 **2 分钟内** 执行同命令续跑(已写 `done.W*` 的窗口自动跳过):

```sh
export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:$HOME/.local/bin"
export TMPDIR=/tmp
cd "$W"
sh ./unisacc.com tools/prerun-supervise.cx -- run . \
  --candidate "$CHAIN/ape-final/unisacc-next.com" \
  --out "$PRERUN_OUT" --jobs 8 --keep-going product-red
```

### (S2) 起预跑前(整链尾段)预热 `affected`、`run-batch` 与 `QROOT`(消除 W0000/W0001 冷重跑)

- 在整链进入 `fixedpoint` 尾段(或链止后起 `prerun-supervise.cx -- run` 前的 60 秒内),先导出冻结源目录 `QROOT="$PRERUN_OUT/frozen-root"` 并预热 `tools/affected.cx`、`tests/run-batch.cx` 与 `opt-frame4-material`,把 `0.0.106` 中 `W0000`(`58s rc=124`)与 `W0001`(`58s rc=124`)的冷编译挪到链跑期/起跑前:

```sh
PRERUN_OUT=$HOME/.cache/unisacc/prerun/v0.0.107-${REV:0:10}
QROOT=$PRERUN_OUT/frozen-root
mkdir -p "$QROOT" "$PRERUN_OUT/warmup-tmp"
git -C "$W" archive "$REV" | tar -xf - -C "$QROOT"
cp -f "$W/unisacc.com" "$QROOT/unisacc.com"
cp -f "$W/unisacc.com.build.json" "$QROOT/unisacc.com.build.json"

cd "$QROOT"
TMPDIR=/tmp sh ./unisacc.com tools/affected.cx -- --files README.md
TMPDIR=/tmp sh ./unisacc.com tests/run-batch.cx -- "$QROOT" "$PRERUN_OUT/warmup-tmp" prepush-mut1
```

### (S3) 维持 2 车道不变(`--window-lanes 2`)

- `0.0.107` 预跑维持 `2` 车道不变(避免 3 车道在 `heavy`/`long` 段争抢 CPU 引发 `batch-packages-gen` 30 秒界抖动);`3` 车道留待 `0.1.0` 封板车在 `long` 段单试。

### (S4) 重窗先行 `--priority-file`、首 20 窗外耗核验与 30 分钟证伪点

- 创建优先级文件 `$PRERUN_OUT/priority-batches.txt`,将 `F1` 转发桩批(`fwd-cap`、`fwdstub-replay`)与 `0.0.106` 曾慢/曾红批(`check-split-parents`、`check-gen-all-drift`、`check-ln-noderef`、`release-exec-c`、`devsvc-git-c-operations`、`env-semantics`)排在最前:

```sh
cat <<'EOF' > "$PRERUN_OUT/priority-batches.txt"
fwd-cap
fwdstub-replay
c99-tentative-array-incomplete
c99-array-param-typedef
check-split-parents
check-gen-all-drift
check-ln-noderef
release-exec-c
devsvc-git-c-operations
env-semantics
EOF
```

- **首 20 窗 `outer_overhead_ms` 中位数判定(`#3` 选择器复用收益核验)**:
  - 当 `done.W0019` 出现时,执行:
    `awk -F'\t' '/^PRERUN_WINDOW/ && /outer_overhead_ms=/ {for(i=1;i<=NF;i++) if($i~/^outer_overhead_ms=/) {sub(/outer_overhead_ms=/,"",$i); if($i+0>0) print $i+0}}' "$PRERUN_OUT/prerun-supervise.log" | head -n 20 | sort -n | awk '{a[NR]=$1} END {if(NR>0) print "MEDIAN_OUTER_OVERHEAD_MS=" a[int((NR+1)/2)]}'`
  - 判据:中位数 **`<= 3000 ms`** 为证实 `#3` 选择器复用生效(收益上限约 `10 分钟`);中位数 **`>= 20000 ms`** 为证伪。
- **第 30 分钟完成窗数证伪点**:
  - 在 `prerun-supervise.cx -- run` 启动满 **30 分钟** 时,执行:
    `DONE_30M=$(ls "$PRERUN_OUT"/done.W* 2>/dev/null | wc -l); echo "PRERUN_30M_WINDOWS=$DONE_30M"`
  - 判据:若 `DONE_30M < 110`,基准档(`04:50 -> 05:10` 公开)即刻证伪,切换按保守档(`06:13 -> 06:32` 公开)排程报时。

### (F1) `ac483247c7` 转发桩批(`fwd-cap` + `fwdstub-replay`,`0.0.126` 唯一分支)专人盯批合同

- **责任人**:`agy-flow`(主盯) + `cdx`(独立复核)。
- **三步盯批命令**:

```sh
# 1. plan 生成后立即核对 fwd-cap 与 fwdstub-replay 均在 ids.txt 与 prerun-windows.tsv 内
grep -nE '^(fwd-cap|fwdstub-replay)$' "$PRERUN_OUT/ids.txt"
grep -nE 'fwd-cap|fwdstub-replay' "$PRERUN_OUT/prerun-windows.tsv"

# 2. 对应窗口跑完后立即提取 strict 行
grep -E '^(fwd-cap|fwdstub-replay)\t' "$PRERUN_OUT"/strict-W*.tsv

# 3. merge 后在最终 strict.tsv 中终核
grep -E '^(fwd-cap|fwdstub-replay)\t' "$W/release/evidence/0.0.107/strict.tsv"
```

- **期望原行与分支裁定**:
  - 两批均输出 `PASS` 且 `rc=0`(分别对应 `rc0:FWD_CAP` 与 `rc0:CHECK_FWDSTUB_REPLAY`) -> `0.0.126` 随 `0.0.107` 正式完成,**`0.0.107` 公开后直接起 `0.1.0` 封版车**;
  - 若 `fwd-cap` 或 `fwdstub-replay` 非绿 -> 立即保留原始 `strict` 行上报 `cc` 与 `cdx`,在 `0.0.107` 公开后**插跑一辆只含 `0.0.126` 修复的 `0.0.108`**,再起 `0.1.0`。
