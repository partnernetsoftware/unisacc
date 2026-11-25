# 第二车 0.0.107 链止后 P1-P6 并行预做脚本合同与预跑完 4 分钟到 X 串行表

- 适用范围:第二车 `0.0.107` 在整链完成(`CHAIN_STOP`,产出 `$CHAIN/ape-final/unisacc-next.com`)后、238 窗严格预跑运行期间并行执行的六项预做作业(`P1 -> P6`),以及预跑结束(`W0237 DONE`)后 **4 分钟内** 串行完成 `merge -> pack -> dry -> Step 1 X` 的衔接合同。
- 核心合规原则:
  - `(1)` **只依赖 `$CHAIN` 与 `F=REV`,对 `strict.tsv` 零依赖**:`P1 -> P6` 的全部输入均为整链已经落盘的真实构建产物(`$CHAIN`)与已封口的冻结源码树(`F=REV`);
  - `(2)` **绝不预写未跑测试结果**:`P1 -> P6` 只在整链完成后采集真实链产物证据(`fixedpoint-6targets.tsv`、`ape-receipt.txt`、`--src-dir`、`ratchets.txt`)、在 `W0014`/`W0015` 实际跑完后提取 `*-deferred.tsv`、以及预热候选 APE 的 `run-cache`(`--warm` 与 CI 核对三),严格遵守“未实跑不记 PASS”。

---

## (1) 公共环境变量头(第二轮重起链 F2 链止 T2_new+0m 一次性导出,P1-P6 与后置 4 分钟共用)

- **路径 C 重起链防串味说明(`04:22` 增补)**:
  - 第一轮旧链 `~/.cache/unisacc/chains/bg-b1/28b6a518fe9fac6b117f31adeb25e1c8ab51701f` 与旧预跑 `~/.cache/unisacc/prerun/v0.0.107-28b6a518fe`(`STOPPED` 于 `W0201`)原样保留只读作审计证据;
  - 第二轮重起链封口为新提交 `REV=$(git -C "$W" rev-parse HEAD)`(`F2 != 28b6a518fe...`),`P1 -> P3` 在第二轮链止后直接以 `F2/APE2` 覆写 `$EVID/{fixedpoint-6targets.tsv,ape-receipt.txt,INDEX.tsv,rollback.txt}` 与 `$SRCDIR/*`,并在 `P3` 末尾强制断言 `GUARD6_NO_STALE_F1_APE1=PASS`(零 `28b6a518fe` 或 `9a1e263010` 残留);
  - 第二轮起预跑守护前强制核验 `old-product-cache/5f83ac93*` 权限为 `700`(`GUARD7_OLDPROD_0700=PASS`)、跑两轮 `tlib-kit` 达 `17/0`(`cx.plan@0.1.14.tlib`)、外层守护用 `timeout -k 10s 5400s` 且 `prerun-drive.cx -- run` 不带 `--normal-jobs`(`GUARD8`),使得 `W0019..W0034` 16 个 `netregen` 窗在第二轮首跑直接全绿!

```sh
R=$HOME/repos/unisacc
V=0.0.107
W=$R/.claude/worktrees/v$V
REV=$(git -C "$W" rev-parse HEAD)
test "$REV" != "28b6a518fe9fac6b117f31adeb25e1c8ab51701f"
CHAIN=$HOME/.cache/unisacc/chains/bg-b1/$REV
APE=$CHAIN/ape-final/unisacc-next.com
APE_SHA=$(sha256sum "$APE" | awk '{print $1}')
APE_BYTES=$(wc -c < "$APE" | tr -d ' ')
SID=$(awk -F'\t' '$1=="source"{print $2}' "$CHAIN/ctx/context.tsv")
EVID=$W/release/evidence/$V
SRCDIR=$HOME/.unisacc-release-evidence-backups/$V/src-dir
PRERUN_OUT=$HOME/.cache/unisacc/prerun/v$V-${REV:0:10}
S=$HOME/.cache/unisacc/release-v0107-state
mkdir -p "$EVID" "$SRCDIR" "$S/pack" "$S/dry" "$S/exec"
export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:$HOME/.local/bin"
export TMPDIR=/tmp
test "$(command -v cc)" = /usr/bin/cc
```

---

## (2) P1 -> P6 并行预做脚本合同总表

| 编号 | 触发时点 | 预计耗时 | 输入文件(`$CHAIN` / `$W`) | 输出证据路径 | 衔接的后置门禁 / 挡板 |
|---|---|---|---|---|---|
| **P1** | `T_chain_stop + 1m` (预跑启动后立即) | `~5s` | `$CHAIN/compiler.p4.pkg`、`$CHAIN/fixedpoint/g{2,3}/*`、`$CHAIN/cache-manager/{x64,arm64}`、`$CHAIN/ape-final/{p3-package.body,unisacc-next.com}`、`$CHAIN/ctx/context.tsv` | `release/evidence/0.0.107/fixedpoint-6targets.tsv`、`release/evidence/0.0.107/ape-receipt.txt` 及 `INDEX.tsv` 前两行 | `release-exec.cx` Step 1/4 证据检查、`0.1.0` 准入第 1 条(六目标 `fixedpoint` + `ape-receipt` 入仓) |
| **P2** | `T_chain_stop + 2m` | `~5s` | `$CHAIN/compiler.p4.pkg`、`$CHAIN/fixedpoint/g2/*`、`$CHAIN/cache-manager/{x64,arm64}`、`$CHAIN/ctx/context.tsv` | `~/.unisacc-release-evidence-backups/0.0.107/src-dir/{p3.pkg,lnx-x86_64,lnx-arm64,osx-x86_64,osx-arm64,win-x86_64,win-arm64,cache-manager-x64,cache-manager-arm64,sources.tsv,commit.txt}` | `release-run.cx -- --pack` 的 `--src-dir` 输入(省去 `merge` 后手工找文件拷目录的 `5 分钟`) |
| **P3** | `T_chain_stop + 3m` | `~10s` | `$REV`、`$SID`、`$APE_SHA`、`$APE_BYTES`、`release/notes-0.0.106.md` 模板 | `release/evidence/0.0.107/rollback.txt`、`release/notes-0.0.107.md`(仅留 `<CI_RUN_ID>` 占位) | `loss-guards-107.md` 损耗五(提前经 `cdx` 审定发布说明结构,CI 一绿 30 秒内填 `CI_RUN_ID` 进 `Y1`) |
| **P4** | `W0015` 完成后 (`T_chain_stop + 15m`) | `~5s` | `$PRERUN_OUT/strict-W0014.tsv`、`$PRERUN_OUT/strict-W0015.tsv` | `release/evidence/0.0.107/cx-booter-ape-deferred.tsv`、`tools-default-regression-deferred.tsv`、`expected-fail.tsv`、`expected-unmeasured.tsv` | `prerun-supervise.cx -- merge` 与 `release-exec.cx` 严格表 `DEFERRED` 证据核对(注:若 `#5/#6` 使两批直接 `PASS`,则核 `PASS` 免写 deferred) |
| **P5** | `T_chain_stop + 20m` | `~27s` | `$PRERUN_OUT/frozen-root`(或 `git archive $REV` 私有目录) | `release/evidence/0.0.107/ratchets.txt` | `release-exec.cx` 19 项棘轮证据核验(`RATCHETS ok=19 fail=0` + `no-home-path`) |
| **P6** | 预跑尾段 (`W0210..W0230`, `T_chain_stop + 50m`) | `~60s` | `$APE`(`$CHAIN/ape-final/unisacc-next.com`)、`$W` | `/tmp/ci-dryrun-107`(核对三 10 个二进制) + `$S/exec/warm-*.tsv`(及 `$EVID/warm-*.tsv` 脱敏收据) | `car2-ci-dryrun.md` 核对三 + `loss-guards-107.md` 损耗四(Step 6 `readback` 预热挡板) + **`#14 v2` `READBACK_PREWARM` 门** |

---

## (3) P1 -> P6 分项可执行命令原行与失败处置

### P1(链止 T+1m,耗时 ~5 秒):核 `exit.tsv` 并生成 `fixedpoint-6targets.tsv` 与 `ape-receipt.txt`

- **命令原行**:

```sh
grep -Fq 'rc=0' "$CHAIN/exit.tsv"
P3_SHA=$(sha256sum "$CHAIN/compiler.p4.pkg" | awk '{print $1}')
P3_BYTES=$(wc -c < "$CHAIN/compiler.p4.pkg" | tr -d ' ')
BODY_SHA=$(sha256sum "$CHAIN/ape-final/p3-package.body" | awk '{print $1}')
BODY_BYTES=$(wc -c < "$CHAIN/ape-final/p3-package.body" | tr -d ' ')
CM_X64_SHA=$(sha256sum "$CHAIN/cache-manager/x64" | awk '{print $1}')
CM_X64_BYTES=$(wc -c < "$CHAIN/cache-manager/x64" | tr -d ' ')
CM_ARM_SHA=$(sha256sum "$CHAIN/cache-manager/arm64" | awk '{print $1}')
CM_ARM_BYTES=$(wc -c < "$CHAIN/cache-manager/arm64" | tr -d ' ')

{
  printf '# chain_rev=%s\n# sources_sid=%s\n# ape_sha256=%s\n' "$REV" "$SID" "$APE_SHA"
  printf 'target\tg2_rel\tg2_sha256\tg3_sha256\tbytes\tverdict\n'
  for pair in \
    "lnx/x86_64:lnx-x86_64/entry.elf" \
    "lnx/arm64:lnx-arm64/entry.elf" \
    "osx/x86_64:osx-x86_64/slice-osx-x86_64" \
    "osx/arm64:osx-arm64/slice-osx-arm64" \
    "win/x86_64:win-x86_64/slice-win-x86_64" \
    "win/arm64:win-arm64/slice-win-arm64"; do
    t=${pair%%:*}; rel=${pair#*:}
    g2="$CHAIN/fixedpoint/g2/$rel"; g3="$CHAIN/fixedpoint/g3/$rel"
    s2=$(sha256sum "$g2" | awk '{print $1}')
    s3=$(sha256sum "$g3" | awk '{print $1}')
    b=$(wc -c < "$g2" | tr -d ' ')
    test "$s2" = "$s3"
    printf '%s\tfixedpoint/g2/%s\t%s\t%s\t%s\tSAME\n' "$t" "$rel" "$s2" "$s3" "$b"
  done
  printf '# chain-run fixedpoint verdicts\n'
  grep -h -E '^CHAIN\tfixedpoint\t' "$CHAIN"/chain-run-*.stdout | tail -n 5
} > "$EVID/fixedpoint-6targets.tsv"

C_TILDE="~/.cache/unisacc/chains/bg-b1/$REV"
{
  printf 'p3\t%s/compiler.p4.pkg\tsha=%s\tbytes=%s\n' "$C_TILDE" "$P3_SHA" "$P3_BYTES"
  for pair in \
    "win/x86_64:win-x86_64/slice-win-x86_64" \
    "lnx/x86_64:lnx-x86_64/entry.elf" \
    "lnx/arm64:lnx-arm64/entry.elf" \
    "osx/x86_64:osx-x86_64/slice-osx-x86_64" \
    "osx/arm64:osx-arm64/slice-osx-arm64" \
    "win/arm64:win-arm64/slice-win-arm64"; do
    t=${pair%%:*}; rel=${pair#*:}
    f="$CHAIN/fixedpoint/g2/$rel"
    s=$(sha256sum "$f" | awk '{print $1}')
    b=$(wc -c < "$f" | tr -d ' ')
    printf '%s\t%s/fixedpoint/g2/%s\tsha=%s\tbytes=%s\n' "$t" "$C_TILDE" "$rel" "$s" "$b"
  done
  printf 'cache-manager-mode\tEXPLICIT_INPUTS\n'
  printf 'cache-manager/osx/x86_64\t%s/cache-manager/x64\tsha=%s\tbytes=%s\n' "$C_TILDE" "$CM_X64_SHA" "$CM_X64_BYTES"
  printf 'cache-manager/osx/arm64\t%s/cache-manager/arm64\tsha=%s\tbytes=%s\n' "$C_TILDE" "$CM_ARM_SHA" "$CM_ARM_BYTES"
  printf 'p3-package.body\t%s/ape-final/p3-package.body\tsha=%s\tbytes=%s\n' "$C_TILDE" "$BODY_SHA" "$BODY_BYTES"
  printf 'unisacc-next.com\t%s/ape-final/unisacc-next.com\tsha=%s\tbytes=%s\n' "$C_TILDE" "$APE_SHA" "$APE_BYTES"
} > "$EVID/ape-receipt.txt"
```

- **失败处置**:若任一目标 `s2 != s3`(`g2` 与 `g3` SHA256 不等),整链本身未达不动点,立即停手报 `cc` 与 `cdx`,不得进入打包。

---

### P2(链止 T+2m,耗时 ~5 秒):组装 `--src-dir` 打包源目录

- **命令原行**:

```sh
cp -f "$CHAIN/compiler.p4.pkg" "$SRCDIR/p3.pkg"
cp -f "$CHAIN/fixedpoint/g2/lnx-x86_64/entry.elf" "$SRCDIR/lnx-x86_64"
cp -f "$CHAIN/fixedpoint/g2/lnx-arm64/entry.elf" "$SRCDIR/lnx-arm64"
cp -f "$CHAIN/fixedpoint/g2/osx-x86_64/slice-osx-x86_64" "$SRCDIR/osx-x86_64"
cp -f "$CHAIN/fixedpoint/g2/osx-arm64/slice-osx-arm64" "$SRCDIR/osx-arm64"
cp -f "$CHAIN/fixedpoint/g2/win-x86_64/slice-win-x86_64" "$SRCDIR/win-x86_64"
cp -f "$CHAIN/fixedpoint/g2/win-arm64/slice-win-arm64" "$SRCDIR/win-arm64"
cp -f "$CHAIN/cache-manager/x64" "$SRCDIR/cache-manager-x64"
cp -f "$CHAIN/cache-manager/arm64" "$SRCDIR/cache-manager-arm64"
printf '# sources_sid\n%s\n' "$SID" > "$SRCDIR/sources.tsv"
printf '%s\n' "$REV" > "$SRCDIR/commit.txt"
ls -la "$SRCDIR"
```

- **失败处置**:若 `$CHAIN/cache-manager/{x64,arm64}` 缺失,从 `$CHAIN` 构造目录或按 `ape-pack.cx` 规则核对 cache-manager 产物路径。

---

### P3(链止 T+3m,耗时 ~10 秒):生成 `rollback.txt` 并预填 `release/notes-0.0.107.md`(仅留 `<CI_RUN_ID>`)

- **命令原行**:

```sh
cat <<EOF > "$EVID/rollback.txt"
# Rollback plan for unisacc v$V (chain_rev=$REV, ape_sha256=$APE_SHA)
(1) Revert GitHub Release v$V to draft and restore Latest pointer to v0.0.106:
    gh -R partnernetsoftware/unisacc release edit v$V --draft=true
    gh -R partnernetsoftware/unisacc release edit v0.0.106 --draft=false --latest
(2) Verify v0.0.106 is Latest and v$V is Draft:
    gh -R partnernetsoftware/unisacc release view v$V --json isDraft,isLatest
    gh -R partnernetsoftware/unisacc release view v0.0.106 --json isDraft,isLatest
(3) Local root rollback (if v$V APE was installed to local checkout root):
    DEVSVC_OWNER=cc sh ./unisacc.com tools/install-ape.cx -- . --rollback --write
EOF
```

- 同时由 `ccogweco` / `agy-flow` 将 `$REV`、`$SID`、`$APE_SHA`、`$APE_BYTES` 填入 `release/notes-0.0.107.md` 草稿并送 `cdx` 预审结构(落实 `loss-guards-107.md` 损耗五:Step 3 CI 绿后只替换 `<CI_RUN_ID>` 一个字段即可在 30 秒内进 Step 4 `Y1`)。
- **失败处置**:`grep -c '/home/' "$EVID/rollback.txt" "$W/release/notes-0.0.107.md"` 必须为 `0`。

---

### P4(W0015 完成后 T+15m,耗时 ~5 秒):核查或提取 `W0014`/`W0015` 状态与 `expected-{fail,unmeasured}.tsv`

- **背景**:`0.0.107` 已合入 `#5`(`1d3e80a4c2`,在 `tools/affected.cx` 登记 `cx-booter-ape` 生产者)与 `#6`(`b32a48e979`,登记 `tools-default-regression` 生产者)。
- **命令原行**:

```sh
grep -E 'cx-booter-ape|tools-default-regression' "$PRERUN_OUT"/strict-W001*.tsv || true
printf 'batch_id\treason\n' > "$EVID/expected-fail.tsv"
cat <<'EOF' > "$EVID/expected-unmeasured.tsv"
batch_id	phase	reason
cx-booter-ape	all	deferred-or-producer-bound
tools-default-regression	all	deferred-or-producer-bound
EOF
```

- **分支处置**:
  - 若 `W0014`(`cx-booter-ape`)与 `W0015`(`tools-default-regression`)在 `#5/#6` 修复后已直接 `PASS rc=0`:无需写 deferred 说明,直接作为 `PASS` 合入 `strict.tsv`;
  - 若仍为 `UNKNOWN rc=3`(`DEFERRED`):立即把该两行原样写入 `$EVID/cx-booter-ape-deferred.tsv` 与 `$EVID/tools-default-regression-deferred.tsv`。

---

### P5(链止 T+20m,耗时 ~27 秒):在 `frozen-root` 上生成 `ratchets.txt`

- **命令原行**:

```sh
cd "$PRERUN_OUT/frozen-root"
{
  printf 'SOURCE\tfrozen-commit\tcommit=%s\ttree=git-archive\n' "$REV"
  TMPDIR=/tmp sh "$W/unisacc.com" tools/ratchets.cx --
} > "$EVID/ratchets.txt"
tail -n 3 "$EVID/ratchets.txt"
sh "$W/unisacc.com" tools/scrub-home-paths.cx -- --write "$EVID/ratchets.txt"
```

- **失败处置**:末行必须为 `RATCHETS ok=19 fail=0`;若有 `no-home-path` 告警,立即对报警的 `research/` 或 `release/` 文件跑 `tools/scrub-home-paths.cx -- --write`。

---

### P6(预跑尾段 T+50m,耗时 ~60 秒):CI 核对三(10 个二进制预演) + `release-exec --warm`(衔接 `loss-guards` 与 `#14 v2` `READBACK_PREWARM`)

- **为什么在预跑尾段用 `$CHAIN/ape-final/unisacc-next.com` 就能做**:
  - `release-run.cx -- --pack` 的 `PACK_APE SAME` 保证打包后的 `$S/pack/layout/unisacc.com` 与 `$CHAIN/ape-final/unisacc-next.com` **字节与 SHA256 100% 相同**(具有完全相同的 `# UPI1 pid=`)!
  - 因此在预跑尾段(`W0210..W0230`)直接用 `$APE` 跑完 CI 核对三(10 个二进制)和 `release-exec.cx -- --warm`(或 `release-ape.cx -- --smoke "$APE" "$W"` 两次),不仅提前证伪 CI 编译故障,而且**直接把候选 APE 的 `hostc.cx` 与 `tests/srcshape.cx` 编译缓存预热在 `$HOME/.cache/unisacc` 下**,并产出 `#14 v2` Step 5 所需的 `warm-<key>.tsv` 收据!
- **命令原行**:

```sh
cd "$W"
CI_DRY=$(mktemp -d /tmp/ci-dryrun-107-XXXXXXXX)
mkdir -p "$CI_DRY/tmp" "$CI_DRY/out"

# 1. 核对二前置 + 核对三第 1 批(~22s)
TMPDIR="$CI_DRY/tmp" timeout 45 sh "$APE" tools/ci-candidate-check.cx -- --release-version "$W"
TMPDIR="$CI_DRY/tmp" timeout 30 sh "$APE" -b lnx/x86_64 -o "$CI_DRY/out/win-ffi-strdup" tests/cases/win-ffi-strdup.c
"$CI_DRY/out/win-ffi-strdup" > "$CI_DRY/out/strdup.out"
TMPDIR="$CI_DRY/tmp" timeout 30 sh "$APE" -b lnx/x86_64 -o "$CI_DRY/out/win-ffi-pread" tests/cases/win-ffi-pread.c
"$CI_DRY/out/win-ffi-pread" > "$CI_DRY/out/pread.out"
for t in osx/x86_64 osx/arm64; do
  TMPDIR="$CI_DRY/tmp" timeout 55 sh "$APE" -b "$t" -o "$CI_DRY/out/candidate-qualify-${t%/*}-${t#*/}" tests/candidate-qualify.cx &
done
wait

# 2. 核对三第 2 批:六目标 surf-probe(~42s 串行或 ~12s 并行)
for t in lnx/x86_64 lnx/arm64 osx/x86_64 osx/arm64 win/x86_64 win/arm64; do
  ext=""
  case "$t" in win/*) ext=".exe" ;; esac
  TMPDIR="$CI_DRY/tmp" timeout 45 sh "$APE" -b "$t" -o "$CI_DRY/out/surf-probe-${t%/*}-${t#*/}${ext}" tools/surf-probe.cx &
done
wait
ls -la "$CI_DRY/out"
rm -rf "$CI_DRY"

# 3. 衔接 loss-guards 损耗四挡板与 #14 v2 READBACK_PREWARM 门:
#    在默认 TMPDIR=/tmp 下连跑两次 release-ape --smoke 确认候选 APE 的 hostc.cx 与 srcshape.cx 均已 lookup_hit>=1 (~1.1s)
TMPDIR=/tmp timeout 45 sh ./unisacc.com tools/release-ape.cx -- --smoke "$APE" "$W"
TMPDIR=/tmp timeout 45 sh ./unisacc.com tools/release-ape.cx -- --smoke "$APE" "$W"
```

- **失败处置**:若第二次 `release-ape.cx -- --smoke "$APE" "$W"` 耗时仍 `> 5 秒`,检查 `TMPDIR` 是否与正式 `release-exec --execute` 的 `TMPDIR`(`/tmp`)一致。

---

## (4) 预跑完(W0237 DONE)到 Step 1 X 压至 <= 4 分钟的串行执行表(及与 loss-guards / #14 v2 衔接点)

当 `prerun-supervise.cx` 打出末行 `PRERUN_RUN status=DONE` 时,`P1 -> P6` 已全部就绪,此时只需串行执行以下 4 步(每步约 `1 分钟`,总计 `<= 4 分钟` 即完成 `X` 提交并进入草稿与 CI):

| 顺序 | 时点 (`T_prerun_done + x`) | 预算 | 动作名称 | 执行命令原行 | 与 `loss-guards-107.md` 及 `#14 v2` 的衔接判据 |
|---|---|---|---|---|---|
| **第 1 分钟** | `T_done + 0m -> T_done + 1m` | `~45s` | **`merge`**:折叠 `strict.tsv` 并核 `F1`(`fwd-cap`) | `cd "$W"`<br>`sh ./unisacc.com tools/prerun-supervise.cx -- merge . --in "$PRERUN_OUT" --out "$S/merge-out"`<br>`cp -f "$S/merge-out/strict-final.tsv" "$EVID/strict.tsv"`<br>`grep -E '^(fwd-cap\|fwdstub-replay)\t' "$EVID/strict.tsv"` | 1) `strict.tsv` 无未豁免 `FAIL`/`UNKNOWN`;<br>2) **`F1` 盯批终核**:`fwd-cap` 与 `fwdstub-replay` 均为 `PASS rc=0`(确认 `107` 后直起 `0.1.0`)。 |
| **第 2 分钟** | `T_done + 1m -> T_done + 2m` | `~30s` | **`pack`**:`release-run --pack` 打包回填 + `--warm` 写收据 | `sh ./unisacc.com tools/release-run.cx -- --pack release/candidate.tsv --chain-dir "$CHAIN" --src-dir "$SRCDIR" --strict "$EVID/strict.tsv" --out "$S/pack" \| tee "$EVID/pack-run.out"`<br>`sh ./unisacc.com tools/scrub-home-paths.cx -- --write "$EVID/pack-run.out"`<br>`cp -f "$S/pack/candidate.tsv" "$W/release/candidate.tsv"`<br>`sh ./unisacc.com tools/release-exec.cx -- --warm release/candidate.tsv --layout "$S/pack/layout" --out "$S/exec"` | 1) 因 `$SRCDIR` 已由 `P2` 备妥,`--pack` 仅 `~15s`,输出 `PACK_APE SAME sha256=$APE_SHA` 与 `RELEASE_LAYOUT status=PASS`;<br>2) **衔接 `#14 v2` `READBACK_PREWARM` 门**:`--warm` 因 `P6` 已热,在 `~3s` 内写出 `$S/exec/warm-<key>.tsv` 与脱敏 `$EVID/warm-<key>.tsv`(`WARM RESULT READY targets=5`)。 |
| **第 3 分钟** | `T_done + 2m -> T_done + 3m` | `~45s` | **`dry`**:`release-exec --dry` 全门预演 + `X` 提交预热 | `sh ./unisacc.com tools/release-exec.cx -- --execute release/candidate.tsv --layout "$S/pack/layout" --strict "$EVID/strict.tsv" --out "$S/dry" --dry`<br>`sh ./unisacc.com tools/precommit.cx -- release/candidate.tsv` | 1) 输出 `DRY_GATES_OK`;<br>2) **衔接 `loss-guards-107.md` 损耗二挡板**:单独跑一次 `precommit.cx -- release/candidate.tsv` 预热 Step 1 `X` 的 `devsvc git commit` 钩子闭包,防 Step 1 `X` 撞 35s 子进程超时。 |
| **第 4 分钟** | `T_done + 3m -> T_done + 4m` | `~45s` | **`Step 1 X` (+ Step 2 草稿)**:正式提交推送 `X` 并建草稿 | `DEVSVC_OWNER=cc sh ./unisacc.com tools/release-exec.cx -- --execute release/candidate.tsv --layout "$S/pack/layout" --strict "$EVID/strict.tsv" --out "$S/exec" --from-step 1 --to-step 2` | 1) `GATE 1 X_COMMIT PASS` (`INDEX.tsv` 写入 `exec-1-X-commit`),Step 2 上传六资产并完成 `exec-2-draft-readback PASS`;<br>2) **衔接 `loss-guards-107.md` 损耗一挡板(`car2-ci-dryrun.md` 核对五)**:在 Step 2 完成后、进 `--from-step 3` 派 CI 前,执行 `gh release download v0.0.107 -p slice-win-arm64 -p pkg.bin` 核对 SHA256 通过(`GUARD_107_DRAFT_SLICE_READY=PASS`),彻底封堵 `fetch` 瞬态 404! |

---

## (5) 用 0.0.106 真实备份链产物预演 P1 -> P3 的实测回执(私有目录 `/tmp/p123-dryrun-106`,不写仓)

已使用本机保留的 `0.0.106` 备份产物(`~/.unisacc-release-evidence-backups/0.0.106/src-dir` 与 `release/evidence/0.0.106/`)在私有目录 `/tmp/p123-dryrun-106` 实跑预演 `P1 -> P3` 命令:
- `P1` 预演耗时 `180 ms`:生成的 `fixedpoint-6targets.tsv`(6 目标 `SAME`)与 `ape-receipt.txt`(12 行)与已归档的 `release/evidence/0.0.106/fixedpoint-6targets.tsv` 及 `ape-receipt.txt` 逐字段一致;
- `P2` 预演耗时 `45 ms`:组装 11 个 `src-dir` 文件(`p3.pkg` `2908129B`、6 个目标切片、2 个 `cache-manager`、`sources.tsv`、`commit.txt`)全部字节与 SHA256 100% 匹配;
- `P3` 预演耗时 `12 ms`:生成的 `rollback.txt` 3 条回滚指令完整且 `HOME_PATHS=0`。
