# agy-flow 新会话接续交接页(2026-11-25 02:35 +0800 切净新会话专用)

- 交接目的:原会话已完成 `0.0.106` 公开发布与收尾、`0.0.107` 补丁独立复核(`#12`、`#3`、`#14 v1/v2`、`#15`)及四份执行手册落盘;按 `1:cc` 指令在 `02:40` 前切净新会话,由新会话直接读取本页与以下四份手册接管 `0.0.107` 起链(`02:50 -> 02:55`)、预跑(`S1-S4` + `P1-P6` + `F1` 盯批)与 Step 1-7 公开发布,随后直接衔接 `0.1.0` 封版车。
- 新会话启动后第一件事:读本页后立即发信封给 `1:cc`:
  `~/repos/moltbaby/bin/mux envelope "1:cc" "agy-flow 新会话就绪" "新会话就绪,已加载 handoff-for-fresh-session.md 与四份 107 执行表,随时接管 02:50/02:55 起链与 S1-S4/P1-P6/F1。"`

---

## (1) 当前固定事实状态(2026-11-25 02:35 +0800)

(1) **本窗口在途进程状态**:
- `agy-flow` 名下 **零在途后台进程、零在途 `bg-candidate` / `devsvc` 写锁**(`task-5863` 已完成并清理 `/tmp/ci-dryrun-107-priv`),切会话零丢失风险。

(2) **分支与车头状态**:
- 共享根 `~/repos/unisacc`:分支 `main`,保持已热的 `0.0.105` Root(`934c7c0bc0`),**起链前不升级共享根**(Root 升级推迟到 `0.0.107` 起链后)。
- 第二车工作树 `W=~/repos/unisacc/.claude/worktrees/v0.0.107`:
  - 当前车头(正式起链 `F=REV`):`28b6a518fe9fac6b117f31adeb25e1c8ab51701f`(`28b6a518fe`),`main` 在 `d71c225b7d`(冻结行 `main` 上 `07:30`、车树 `F` 上 `08:00`);
  - **整链已于 `2026-11-25 02:49:02 +0800` 正式启动(`T+0`)**:
    - `REV=28b6a518fe9fac6b117f31adeb25e1c8ab51701f`
    - `pid/pgid=1984486`, `deadline=5400s`
    - 输出目录:`CHAIN=~/.cache/unisacc/chains/bg-b1/28b6a518fe9fac6b117f31adeb25e1c8ab51701f`
    - 预计链止时刻:`03:17 -> 03:20 +0800`(链止 `T+1m` 即由 `agy-flow` 启动 `P1 -> P6` 并衔接预跑 `S1 -> S4` 与 `F1` 盯批)。

(3) **`fable` 路线总裁定**:
- **`0.0.107` 公开后直接起 `0.1.0` 封版车(零代码生成变化,只注入版本槽)**;原 `0.0.108+126`、`0.0.109`、`0.0.125` 四车序列不走,全部顺延 `0.1.x`。
- 唯一分支条件:`F1` 转发桩批(`ac483247c7` 对应批 `fwd-cap` 与 `fwdstub-replay`,`0.0.126` 的唯一分支)须在 `0.0.107` 预跑 `ids.txt` 内并由 `agy-flow` 专人盯其 `strict` 行;两批全绿则直起 `0.1.0`,不绿才插一辆只含 `0.0.126` 的 `0.0.108`。
- 目标时刻:`0.0.107` 公开基准 `04:50 -> 05:10`、保守 `06:13 -> 06:32`;`0.1.0` 公开基准 `~08:00`、保守 `~10:30`、最迟起链 `13:40`。

---

## (2) 四份已落盘执行手册索引(新会话按需直读,不重造)

1. **阶段 C 四步封口、五门预备与起链 / 预跑 S1-S4 合同**:`research/agy-flow/car2-stage-c-run.md`
2. **链止后 P1-P6 并行预做脚本合同与预跑完 4 分钟到 X 串行表**:`research/agy-flow/car2-parallel-prep.md`
3. **CI 起跑前五条核对(含 106 APE 十二进制预演实测)与红时四分类分诊表**:`research/agy-flow/car2-ci-dryrun.md`
4. **24 小时实况倒计时与 fable 裁定路线图**:`research/agy-flow/car2-live-clock.md`

---

## (3) 起链时点(02:46 -> 02:55)与核心命令速查

当 `cc` 完成阶段 C 第一至三步并于 `02:46` 提交单笔封口 `F=REV`(步骤 `4A`,`~5.4s`)后,必须固定同一个 `TMPDIR="$S/tmp"`(`/tmp/unisacc-session-car2-stagec/tmp`)按序执行 `4B` -> `4C`:

```sh
R=$HOME/repos/unisacc
V=0.0.107
W=$R/.claude/worktrees/v$V
BASE=$HOME/.cache/unisacc/chains/bg-b1
S=/tmp/unisacc-session-car2-stagec
mkdir -p "$S/tmp"
export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:$HOME/.local/bin"
export TMPDIR="$S/tmp"
unset UNISACC_INTEGRATOR DVQUALIFY_NATIVE_RUN_BATCH UNISACC_RUN_CACHE UNISACC_UNIT_CACHE UNISACC_CACHE_DIR UNISACC_BOUND_DEADLINE_MONO UNISACC_E3CACHE

cd "$W"
REV=$(git rev-parse HEAD)

# [F 前若未跑或闭包有变才补跑门 5 预备: tool 与 generator 各一次]
# sh ./unisacc.com tools/check-candidate-chain.cx -- --prepare "$S/tmp/unisacc-cr-prep-cache" tool
# sh ./unisacc.com tools/check-candidate-chain.cx -- --prepare "$S/tmp/unisacc-cr-prep-cache" generator

# [4B-1: F 后门 22 预合树预备, 绑定本地 main 与 F=REV 双 SHA, 热 12.4s / 冷 49.4s]
sh ./unisacc.com tools/premerge-union.cx -- --repo "$W" --shard prep --branch "v$V"

# [4B-2: F 后门 19 两条预备, 同 F 同 TMPDIR, 合计 ~11.3s (0.481s + 10.764s), 使门 19 全门稳定在 37.8s]
sh ./unisacc.com tools/chain-preflight.cx -- --item chain-to-snap --root "$W" --repo "$W" --rev "$REV"
sh ./unisacc.com tools/sixtarget-precompile.cx -- --mode headers --deadline-s 50

# [4C: 全量 chain-ready (cdx e2e 实测 179.365s = 2m59s, F 到全量 210.346s = 3m30s, 末行 READY yes failed=none) -> 4 路起链]
DEVSVC_OWNER=cc sh ./unisacc.com tools/chain-ready.cx -- --repo "$W" --root "$W" --rev "$REV" --version "$V" --chain-base "$BASE"
DEVSVC_OWNER=cc sh ./unisacc.com tools/bg-candidate.cx -- start --rev "$REV" --base "$BASE" --deadline-s 5400 -- --parallel 4
sh ./unisacc.com tools/bg-candidate.cx -- status "$REV" --base "$BASE"
```

---

## (4) 读回预热门(#14 v2)、CI 挡板与 P1-P6 状态速查

(1) **读回预热门(`#14 v2` 已入车 `48f9e054ad`,`tools/release-exec.cx`)**:
- `#14 v2` 在 Step 5 `publish` 公开前新增硬门 `GATE 5 READBACK_PREWARM`:会检查 `warm-<key>.tsv` 收据并调 `release-ape.cx -- --smoke` 要求 `lookup_hit >= 1` 且 `elapsed_ms <= 5000`(首次若未热会自动跑第二次确认);同时在 `--from-step 6/7` 时让 Step 5 `VERIFY` 免重跑 live prewarm、Step 6 回滚自动写 `exec-6-rollback`。
- 衔接动作:
  - 在预跑尾段 `P6`(`car2-parallel-prep.md` 第 3 节)先对 `$CHAIN/ape-final/unisacc-next.com` 跑两次 `TMPDIR=/tmp sh ./unisacc.com tools/release-ape.cx -- --smoke "$APE" "$W"`;
  - 在预跑完第 2 分钟 `--pack` 后立即执行一次:
    `sh ./unisacc.com tools/release-exec.cx -- --warm release/candidate.tsv --layout "$S/pack/layout" --out "$S/exec"`
    产出 `$S/exec/warm-<key>.tsv` 与脱敏 `release/evidence/0.0.107/warm-<key>.tsv`(`WARM RESULT READY targets=5`),Step 5 即可秒过 `GATE 5 READBACK_PREWARM PASS`。

(2) **CI 必过挡板(`car2-ci-dryrun.md` 核对一至五)**:
- **核对三(10 个二进制交叉编译预演)**:已在 `P6` 预跑尾段用候选 APE 跑完(拆为两批 `<= 45s`,两个 `candidate-qualify-osx-*` 并行 `wait`);
- **核对五(`loss-guards-107.md` 损耗一必过挡板,防 `fetch` 瞬态 404)**:在 Step 2 草稿创建上传后、**进 Step 3 派 CI 之前**,必须独立下载 `slice-win-arm64` 与 `pkg.bin` 校验 SHA256 相等(`GUARD_107_DRAFT_SLICE_READY=PASS`),严禁在草稿上传后盲派 CI。

(3) **`P1 -> P6` 当前状态**:
- 脚本合同已写妥在 `research/agy-flow/car2-parallel-prep.md`,并用 `0.0.106` 备份产物完成 `P1-P3` 预演(`161 ms`);
- **尚未对 `0.0.107` 触发**(因 `0.0.107` 尚未起链,严禁在链止前预写任何 `release/evidence/0.0.107/` 文件);待 `0.0.107` 整链 `CHAIN_STOP` 后按 `T+1m..T+50m` 依次触发。

---

## (5) 绝对不要重复做的事(防浪费上下文与时间)

1. **不要重新审查** `#12`、`#3`、`#14 v1/v2`、`#15`——均已审毕且 `#12`(`1122ace564`)、`#3`(`f0f07a315f`)、`#14 v2`(`48f9e054ad`)已入 `v0.0.107`。
2. **不要重新跑** `0.0.106` APE 的核对三十二进制预演或 `0.0.106` 备份的 `P1-P3` 预演——回执已写入 `car2-ci-dryrun.md` 与 `car2-parallel-prep.md`。
3. **不要在起链前升级共享根 `~/repos/unisacc/unisacc.com`**(`0.0.105`)——`cdx` 与 `cc` 已定保持共享根不升、车树用 `0.0.106` 正规文件。
4. **不要用错门 5 `--prepare` 语法**——唯一正确语法是 `sh ./unisacc.com tools/check-candidate-chain.cx -- --prepare "$S/tmp/unisacc-cr-prep-cache" tool` 与 `generator`(不是旧手册的 `OUT CACHE --prepare`)。
5. **不要再按 `108 -> 109 -> 125 -> 0.1.0` 四车序列排程**——`fable` 已裁定 `0.0.107` 公开后直起 `0.1.0`(仅盯 `F1` `fwd-cap` + `fwdstub-replay`)。
