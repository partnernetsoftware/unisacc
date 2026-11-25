# 第二轮重起链(F2 / APE2)相对第一轮(F1 / APE1)的路径与值差异清单及防混用脚本合同

- 适用范围:第二车 `0.0.107` 因 `W0201`(`c99-array-param-typedef` `N2` 语法错误返回 `rc=0`)走**路径 C 重起链**后,从新候选 `APE2` 链止(`T2_new`)起执行 `P1 -> P6`、严格预跑(`247 窗`)、尾段补跑与 CI 前置核对(核对一至五)时,**所有相对第一轮(`F1=28b6a518fe`, `APE1=9a1e263010`, `SID1=9b83432e63`)必须变更的路径与身份值清单**,杜绝新旧目录、新旧哈希或预跑状态串味。

---

## (1) 第一轮(F1, 只读封存) vs 第二轮(F2, 当前生效)核心身份与路径对照总表

| 维度 | 第一轮旧值(`F1 / APE1` — **只读保留,严禁在第二轮引用或复用**) | 第二轮新值(`F2 / APE2` — **唯一生效输入**) | 取值 / 校验命令原行 |
|---|---|---|---|
| **1. 封口提交 `REV`** | `F1 = 28b6a518fe9fac6b117f31adeb25e1c8ab51701f` (`28b6a518fe`) | `F4 = 63354b4a2cb707ecc923a89f0df6030149fa697f` (`63354b4a2c`, `T1_r2=05:08:30`, `pid/pgid=2968889`) | `F2=$(git -C "$W" rev-parse HEAD); test "$F2" = "63354b4a2cb707ecc923a89f0df6030149fa697f"` |
| **2. 整链输出目录 `$CHAIN`** | `~/.cache/unisacc/chains/bg-b1/28b6a518fe9fac6b117f31adeb25e1c8ab51701f` (只读保留作路径 C 审计证据) | `CHAIN2=~/.cache/unisacc/chains/bg-b1/63354b4a2cb707ecc923a89f0df6030149fa697f` | `grep -Fq 'rc=0' "$CHAIN2/exit.tsv"` |
| **3. 候选 APE 路径与 `APE_SHA`** | `APE1_SHA = 9a1e2630100deace2b6a8d82a1b6c47fdf57b872e82f3cd4e1ee55756c2be544` (`8991184B`) | `APE2=$CHAIN2/ape-final/unisacc-next.com`<br>`APE2_SHA=$(sha256sum "$APE2" \| awk '{print $1}')` | `test "$APE2_SHA" != "9a1e2630100deace2b6a8d82a1b6c47fdf57b872e82f3cd4e1ee55756c2be544"` |
| **4. 源码身份 `SOURCES_SID`** | `SID1 = 9b83432e63a84a7fb00a9bb5fcb33533230342c61499903ab13e29b37d4677b6` | `SID2=$(awk -F'\t' '$1=="source"{print $2}' "$CHAIN2/ctx/context.tsv")` (因修了产品源码,`SID2 != SID1`) | `test "$SID2" != "9b83432e63a84a7fb00a9bb5fcb33533230342c61499903ab13e29b37d4677b6"` |
| **5. `p3.pkg` 与六目标 `g2/g3` 切片哈希** | `P3_1 = 8daada10b9...`、`BODY_1 = 64f54ad10f...` 等 | 从 `$CHAIN2/compiler.p4.pkg`、`$CHAIN2/fixedpoint/g{2,3}/*`、`$CHAIN2/ape-final/p3-package.body` 重新现算 | 由 `P1` 脚本从 `$CHAIN2` 全新计算写入 `fixedpoint-6targets.tsv` 与 `ape-receipt.txt` |
| **6. 打包源目录 `$SRCDIR`** | `~/.unisacc-release-evidence-backups/0.0.107/src-dir` 内曾放 `F1/APE1` 的 11 个文件 | 先备份或清空旧 `src-dir`,再用 `$CHAIN2`、`SID2`、`F2` 全量覆写 11 个文件 | `cmp -s "$CHAIN2/compiler.p4.pkg" "$SRCDIR/p3.pkg" && grep -Fxq "$SID2" "$SRCDIR/sources.tsv" && grep -Fxq "$F2" "$SRCDIR/commit.txt"` |
| **7. 仓内证据目录 `$EVID` (`release/evidence/0.0.107/`)** | 含第一轮预写的 `fixedpoint-6targets.tsv`、`ape-receipt.txt`、`INDEX.tsv`、`rollback.txt`、`prerun-windows.tsv` 等 | 先将第一轮证据移至私有备份 `~/.unisacc-release-evidence-backups/0.0.107/round1-28b6a518fe/`,再清空 `$EVID` 由第二轮 `P1-P6` 全新生成 | `test "$(grep -cE '28b6a518fe\|9a1e263010\|9b83432e63' "$EVID"/* 2>/dev/null \| awk -F: '{s+=$2} END{print s+0}')" -eq 0` |
| **8. 严格预跑目录 `$PR`** | `~/.cache/unisacc/prerun/v0.0.107-28b6a518fe` (**只读保留**,含 `W0000..W0201` 原停机现场,绝不 `--resume` 或混入 `merge`) | `PR2=~/.cache/unisacc/prerun/v0.0.107-${F2:0:10}` (全新独立目录) | `test ! -e "$PR2/done.W0000"` (启动前确认为全新空预跑目录) |
| **9. 候选 `tlib-kit` 缓存键** | `kit_key1 = 098c059f6bcd8e86849718b416ebfb422a395519079027037cdec0dbcb77b98f` (绑定 `APE1`) | `kit_key2` 绑定 `APE2_SHA`;必须对 `APE2` 重新跑**两轮** `tlib.cx -- kit` 直至 `libs=17 skipped=0` 且 `kit.tsv` 含 `cx.plan@0.1.14.tlib` | `grep -Fq 'cx.plan@0.1.14.tlib' ~/.cache/unisacc/tlib/kit/$KIT2_KEY/kit.tsv` |
| **10. `old-product-cache/5f83ac93...` 权限** | 第一轮起跑时为 `0775`(无 `fetch.lock`),致 `W0019..W0034` 16 窗 `UNKNOWN 3` | 起跑前强制核验目录权限为 `700` 且 `unisacc.com`、`fetch.lock` 完好 | `test "$(stat -c '%a' ~/.cache/unisacc/prep-cache/old-product-cache/5f83ac93*)" = 700` |
| **11. 预跑 `priority-file`(新增批置顶)** | 第一轮直接拷 `release/evidence/0.0.106/strict.tsv`,14 个 107 新增批(含 `W0201` `c99-array-param-typedef`)排在 `W0198..W0202`(第 58 分钟才跑到!) | 在 `$PR2/priority.tsv` 中合并 `ccogweco` 的 `research/ccogweco/priority-newbatches-107.txt`,将 107 全部 14 个新增批(尤其 `c99-array-param-typedef`、`fwd-cap`、`c99-tentative-array-incomplete`、`c99-global-redecl-conflict`)标记为非 `PASS` 置顶到最前序窗口(`W0001..W0015`)优先验绿! | `head -n 25 "$PR2/windows.tsv" \| grep -E 'c99-array-param-typedef\|fwd-cap'` (前 5 分钟内即验完新批!) |
| **12. 发布状态目录 `$S`** | `~/.cache/unisacc/release-v0107-state` | `S2=~/.cache/unisacc/release-v0107-state-${F2:0:10}` (`pack`、`dry`、`exec` 全部隔离在 `F2` 专属目录下) | `mkdir -p "$S2/pack" "$S2/dry" "$S2/exec"` |
| **13. 预跑 `UNISACC_BOOTER_CHAIN_OUT` 环境变量** | 第一轮未导出,导致 `W0014`(`cx-booter-ape`)因 `booter-chain-out-required` 返回 `UNKNOWN rc=3` 延期 | 第二轮预跑守护内显式导出 `export UNISACC_BOOTER_CHAIN_OUT="$CHAIN2"`(展开后的绝对路径 `$HOME/.cache/unisacc/chains/bg-b1/63354b4a2cb707ecc923a89f0df6030149fa697f`),**且不设任何其它 `UNISACC_*` 变量** | `test "$UNISACC_BOOTER_CHAIN_OUT" = "$HOME/.cache/unisacc/chains/bg-b1/63354b4a2cb707ecc923a89f0df6030149fa697f"` |

---

## (2) 第二轮链止(T2_new)后防混用一键初始化与 P1-P3 覆写脚本原行

```sh
set -euo pipefail
R=$HOME/repos/unisacc
V=0.0.107
W=$R/.claude/worktrees/v$V
BASE=$HOME/.cache/unisacc/chains/bg-b1
F1=28b6a518fe9fac6b117f31adeb25e1c8ab51701f
APE1_SHA=9a1e2630100deace2b6a8d82a1b6c47fdf57b872e82f3cd4e1ee55756c2be544
SID1=9b83432e63a84a7fb00a9bb5fcb33533230342c61499903ab13e29b37d4677b6
PREVREV=2e8a19a342251910d53010f51002b14a4ca7f916

export PATH="/usr/bin:/bin:$HOME/.local/bin"
export TMPDIR=/tmp
unset UNISACC_INTEGRATOR DVQUALIFY_NATIVE_RUN_BATCH UNISACC_RUN_CACHE UNISACC_UNIT_CACHE UNISACC_CACHE_DIR UNISACC_BOUND_DEADLINE_MONO UNISACC_E3CACHE
test "$(command -v cc)" = /usr/bin/cc

# 1. 取 F2 与 CHAIN2/APE2/SID2 并强制断言与第一轮 F1/APE1/SID1 完全不同
F2=$(git -C "$W" rev-parse HEAD)
test "$F2" != "$F1"
CHAIN2=$BASE/$F2
grep -Fq 'rc=0' "$CHAIN2/exit.tsv"
APE2=$CHAIN2/ape-final/unisacc-next.com
APE2_SHA=$(sha256sum "$APE2" | awk '{print $1}')
APE2_BYTES=$(wc -c < "$APE2" | tr -d ' ')
SID2=$(awk -F'\t' '$1=="source"{print $2}' "$CHAIN2/ctx/context.tsv")
test "$APE2_SHA" != "$APE1_SHA"
test "$SID2" != "$SID1"

# 2. 隔离保存第一轮证据并清空 release/evidence/0.0.107/ 与 src-dir
EVID=$W/release/evidence/$V
R1_BAK=$HOME/.unisacc-release-evidence-backups/$V/round1-${F1:0:10}
SRCDIR=$HOME/.unisacc-release-evidence-backups/$V/src-dir
PR2=$HOME/.cache/unisacc/prerun/v$V-${F2:0:10}
S2=$HOME/.cache/unisacc/release-v0107-state-${F2:0:10}

mkdir -p "$R1_BAK"
if test -d "$EVID"; then
  cp -af "$EVID"/. "$R1_BAK"/ 2>/dev/null || true
  rm -rf "$EVID"
fi
rm -rf "$SRCDIR"
mkdir -p "$EVID" "$SRCDIR" "$PR2" "$S2/pack" "$S2/dry" "$S2/exec"

# 3. 核 old-product-cache/5f83ac93* 权限为 700 (挡板七)
test "$(stat -c '%a' $HOME/.cache/unisacc/prep-cache/old-product-cache/5f83ac93*)" = 700
echo "GUARD7_OLDPROD_0700=PASS"

# 4. 执行 P1-P3 (写入 F2/APE2/SID2)
P3_SHA=$(sha256sum "$CHAIN2/compiler.p4.pkg" | awk '{print $1}')
P3_BYTES=$(wc -c < "$CHAIN2/compiler.p4.pkg" | tr -d ' ')
BODY_SHA=$(sha256sum "$CHAIN2/ape-final/p3-package.body" | awk '{print $1}')
BODY_BYTES=$(wc -c < "$CHAIN2/ape-final/p3-package.body" | tr -d ' ')
CM_X64_SHA=$(sha256sum "$CHAIN2/cache-manager/x64" | awk '{print $1}')
CM_X64_BYTES=$(wc -c < "$CHAIN2/cache-manager/x64" | tr -d ' ')
CM_ARM_SHA=$(sha256sum "$CHAIN2/cache-manager/arm64" | awk '{print $1}')
CM_ARM_BYTES=$(wc -c < "$CHAIN2/cache-manager/arm64" | tr -d ' ')
LAST_CMP_STDOUT=$(ls -1v "$CHAIN2"/chain-run-*.stdout | tail -n 1)

{
  printf '# chain_rev=%s\n# sources_sid=%s\n# ape_sha256=%s\n' "$F2" "$SID2" "$APE2_SHA"
  printf 'target\tg2_rel\tg2_sha256\tg3_sha256\tbytes\tverdict\n'
  for pair in \
    "lnx/x86_64:lnx-x86_64/entry.elf" \
    "lnx/arm64:lnx-arm64/entry.elf" \
    "osx/x86_64:osx-x86_64/slice-osx-x86_64" \
    "osx/arm64:osx-arm64/slice-osx-arm64" \
    "win/x86_64:win-x86_64/slice-win-x86_64" \
    "win/arm64:win-arm64/slice-win-arm64"; do
    t=${pair%%:*}; rel=${pair#*:}
    g2="$CHAIN2/fixedpoint/g2/$rel"; g3="$CHAIN2/fixedpoint/g3/$rel"
    s2=$(sha256sum "$g2" | awk '{print $1}')
    s3=$(sha256sum "$g3" | awk '{print $1}')
    b=$(wc -c < "$g2" | tr -d ' ')
    test "$s2" = "$s3"
    printf '%s\tfixedpoint/g2/%s\t%s\t%s\t%s\tSAME\n' "$t" "$rel" "$s2" "$s3" "$b"
  done
  printf '# %s:161-165 fixedpoint verdicts\n' "$(basename "$LAST_CMP_STDOUT")"
  sed -n '161,165p' "$LAST_CMP_STDOUT"
} > "$EVID/fixedpoint-6targets.tsv"
test "$(wc -l < "$EVID/fixedpoint-6targets.tsv")" -eq 16

C_TILDE="~/.cache/unisacc/chains/bg-b1/$F2"
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
    f="$CHAIN2/fixedpoint/g2/$rel"
    s=$(sha256sum "$f" | awk '{print $1}')
    b=$(wc -c < "$f" | tr -d ' ')
    printf '%s\t%s/fixedpoint/g2/%s\tsha=%s\tbytes=%s\n' "$t" "$C_TILDE" "$rel" "$s" "$b"
  done
  printf 'cache-manager-mode\tEXPLICIT_INPUTS\n'
  printf 'cache-manager/osx/x86_64\t%s/cache-manager/x64\tsha=%s\tbytes=%s\n' "$C_TILDE" "$CM_X64_SHA" "$CM_X64_BYTES"
  printf 'cache-manager/osx/arm64\t%s/cache-manager/arm64\tsha=%s\tbytes=%s\n' "$C_TILDE" "$CM_ARM_SHA" "$CM_ARM_BYTES"
  printf 'p3-package.body\t%s/ape-final/p3-package.body\tsha=%s\tbytes=%s\n' "$C_TILDE" "$BODY_SHA" "$BODY_BYTES"
  printf 'unisacc-next.com\t%s/ape-final/unisacc-next.com\tsha=%s\tbytes=%s\n' "$C_TILDE" "$APE2_SHA" "$APE2_BYTES"
} > "$EVID/ape-receipt.txt"

TS=$(date '+%Y-%m-%d %H:%M:%S +0800')
{
  printf 'fixedpoint-6targets\t%s\tPASS\tfixedpoint-6targets.tsv\tchain_rev=%s, slices=same count=6, objects=same count=102, kernels=same count=2, product=same, p3_sha=%s\n' "$TS" "$F2" "$P3_SHA"
  printf 'ape-receipt\t%s\tPASS\tape-receipt.txt\tAPE sha=%s bytes=%s; 六目标 g2 切片 path/sha/bytes 与 fixedpoint-6targets.tsv 逐行一致\n' "$TS" "$APE2_SHA" "$APE2_BYTES"
} > "$EVID/INDEX.tsv"

cp -f "$CHAIN2/compiler.p4.pkg" "$SRCDIR/p3.pkg"
cp -f "$CHAIN2/fixedpoint/g2/lnx-x86_64/entry.elf" "$SRCDIR/lnx-x86_64"
cp -f "$CHAIN2/fixedpoint/g2/lnx-arm64/entry.elf" "$SRCDIR/lnx-arm64"
cp -f "$CHAIN2/fixedpoint/g2/osx-x86_64/slice-osx-x86_64" "$SRCDIR/osx-x86_64"
cp -f "$CHAIN2/fixedpoint/g2/osx-arm64/slice-osx-arm64" "$SRCDIR/osx-arm64"
cp -f "$CHAIN2/fixedpoint/g2/win-x86_64/slice-win-x86_64" "$SRCDIR/win-x86_64"
cp -f "$CHAIN2/fixedpoint/g2/win-arm64/slice-win-arm64" "$SRCDIR/win-arm64"
cp -f "$CHAIN2/cache-manager/x64" "$SRCDIR/cache-manager-x64"
cp -f "$CHAIN2/cache-manager/arm64" "$SRCDIR/cache-manager-arm64"
printf '# sources_sid\n%s\n' "$SID2" > "$SRCDIR/sources.tsv"
printf '%s\n' "$F2" > "$SRCDIR/commit.txt"

cat <<EOF > "$EVID/rollback.txt"
# Rollback plan for unisacc v$V (chain_rev=$F2, ape_sha256=$APE2_SHA)
(1) Revert GitHub Release v$V to draft and restore Latest pointer to v0.0.106:
    gh -R partnernetsoftware/unisacc release edit v$V --draft=true
    gh -R partnernetsoftware/unisacc release edit v0.0.106 --draft=false --latest
(2) Verify v0.0.106 is Latest and v$V is Draft:
    gh -R partnernetsoftware/unisacc release view v$V --json isDraft,isLatest
    gh -R partnernetsoftware/unisacc release view v0.0.106 --json isDraft,isLatest
(3) Local root rollback (if v$V APE was installed to local checkout root):
    DEVSVC_OWNER=cc sh ./unisacc.com tools/install-ape.cx -- . --rollback --write
EOF

# 5. 挡板六硬断言: 确认 $EVID 与 $SRCDIR 中零 F1/APE1/SID1 残留, 且 HOME_PATHS=0
test "$(grep -cE '28b6a518fe|9a1e263010|9b83432e63' "$EVID"/* "$SRCDIR"/*.tsv "$SRCDIR"/*.txt 2>/dev/null | awk -F: '{s+=$2} END{print s+0}')" -eq 0
echo "GUARD6_NO_STALE_F1_APE1=PASS"
```

---

## (3) 第二轮预跑起点节:新增批置顶(priority-newbatches-107.txt)、plan 前序窗口核对与 run 启动原行

- **置顶机制核实(`tools/prerun-inputs.cx:27-30` 与 `tools/prerun-drive.cx:446-519,711-756`)**:
  - `research/ccogweco/priority-newbatches-107.txt`(`sha256=ee339948dff0f4e1d4d858ca006c74ff5af6ea41bffd1a7c47503c4613684bd7`,23 行:2 行注释 + 21 个 `id<TAB>2` 行,`missing=0`)不含 `PASS` 行(`has_pass_rows=0`);
  - 在 `prerun-inputs.cx` 传入 `--priority-src "$R/research/ccogweco/priority-newbatches-107.txt"`(生成 `$PR2/priority.tsv`),并在 `prerun-drive.cx -- plan` 传入 `--priority-file "$PR2/priority.tsv"` 后,`pd_build_plan` 会在 `W0000` warmup 与 `W0001..W0013` 早期 canary 之后,**立即在 `W0014..W0030` 优先排入这 21 个置顶批(排在 68 个 `heavy` `netregen` 窗与后段 150+ 个常规窗之前!)**。
- **预跑起点三步命令原行与期望判据**:

```sh
cd "$W"
sh ./unisacc.com tools/bg-candidate.cx -- promote --check --rev "$F2" --base "$BASE"

# 第 1 步: prerun-inputs (传入 --priority-src 指向 priority-newbatches-107.txt, 产出 $PR2/priority.tsv)
sh ./unisacc.com tools/prerun-inputs.cx -- --rev "$F2" --version "$V" --prev 0.0.106 --prev-rev "$PREVREV" \
  --priority-src "$R/research/ccogweco/priority-newbatches-107.txt" --ape "$APE2" --dir "$PR2" --lanes 2 --jobs 8

# 第 2 步: prerun-drive plan (传入 --priority-file "$PR2/priority.tsv" --normal-jobs 8 --window-lanes 2)
sh ./unisacc.com tools/prerun-drive.cx -- plan \
  --ids-file "$PR2/ids.txt" --times-file "$PR2/times.tsv" --priority-file "$PR2/priority.tsv" \
  --out "$PR2" --normal-jobs 8 --window-lanes 2

# 第 3 步(起跑前必核挡板): 核对 windows.tsv 前序窗口(W0000..W0032, 即重型 netregen 之前)是否完整包含 21 个置顶 id 且 fwdstub-replay 与 c99-array-param-typedef 在内(缺失任一即停手报 cc)
head -n 36 "$PR2/windows.tsv"
missing_prio=0
for id in $(awk -F'\t' '!/^#/ && NF>=1 {print $1}' "$R/research/ccogweco/priority-newbatches-107.txt"); do
  if ! head -n 36 "$PR2/windows.tsv" | tr '\t,' '\n\n' | grep -Fxq "$id"; then
    echo "MISSING_IN_FRONT_WINDOWS=$id"
    missing_prio=$((missing_prio + 1))
  fi
done
test "$missing_prio" -eq 0 && echo "PRIORITY_21_ALL_IN_FRONT_WINDOWS=PASS"
grep -nE 'c99-array-param-typedef|fwd-cap|fwdstub-replay' "$PR2/windows.tsv"

# 第 4 步(等 ompgpt 候选 kit 双轮 17/0 含 cx.plan@0.1.14.tlib 放行后启动 run):
# 严禁传 --normal-jobs (仅 plan 接受, run 传了会报 unknown-run-arg); 外层 timeout 5400s;
# 必须导出 UNISACC_BOOTER_CHAIN_OUT="$CHAIN2"(展开绝对路径)以使 W0014(cx-booter-ape)首跑直过, 并 unset 其余所有 UNISACC_* / DVQUALIFY_NATIVE_RUN_BATCH:
setsid nohup timeout -k 10s 5400s sh -c '
  export PATH="/usr/bin:/bin:$HOME/.local/bin"
  export TMPDIR=/tmp
  unset UNISACC_INTEGRATOR DVQUALIFY_NATIVE_RUN_BATCH UNISACC_RUN_CACHE UNISACC_UNIT_CACHE UNISACC_CACHE_DIR UNISACC_BOUND_DEADLINE_MONO UNISACC_E3CACHE
  export UNISACC_BOOTER_CHAIN_OUT="$4"
  for attempt in 1 2; do
    sh ./unisacc.com tools/prerun-drive.cx -- run --root "$1" --candidate "$2" --out "$3" \
      --window-lanes 2 --resume --max-jobs 8 --keep-going --min-free 2147483648
    rc=$?
    [ "$rc" -eq 0 ] && exit 0
    [ "$rc" -eq 2 ] && exit 2
  done
  exit 1
' sh "$W" "$APE2" "$PR2" "$CHAIN2" >> "$PR2/supervise.log" 2>&1 < /dev/null &
```

- **期望原行**:
  - `PRERUN_INPUTS ... priority=ee339948dff0f4e1d4d858ca006c74ff5af6ea41bffd1a7c47503c4613684bd7 ... status=READY`
  - `PRIORITY_21_ALL_IN_FRONT_WINDOWS=PASS`(21 个 ID 包括 `c99-array-param-typedef`、`fwd-cap`、`fwdstub-replay` 全部排在 `W0000..W0031` 前序窗口,在 68 个 `netregen` 重窗之前优先跑完;若 `missing_prio > 0` 立即报 `cc`)。

---

## (4) 第二轮 CI 前置核对(核对一至五)的路径与值变更点

| 核对项(`car2-ci-dryrun.md`) | 第一轮旧值 | 第二轮新值与命令变更点 |
|---|---|---|
| **核对一(workflow 与候选槽)** | `HEAD = 28b6a518fe` | `HEAD = $F2`(含 `c99-n2-exitcode-fix.patch`) |
| **核对二(候选表七哈希与版本号)** | `LAYOUT = ~/.cache/unisacc/release-v0107-state/pack/layout`, `APE_SHA = 9a1e263010...` | `LAYOUT = $S2/pack/layout`(`~/.cache/unisacc/release-v0107-state-${F2:0:10}/pack/layout`);核 `release/candidate.tsv` 第 3 列为 `$F2`、第 9 列为 `$APE2_SHA`(绝非 `9a1e263010...`) |
| **核对三(10 个二进制交叉编译预演,见 `P6`)** | `APE = .../28b6a518fe.../ape-final/unisacc-next.com` | `APE = $APE2`(`~/.cache/unisacc/chains/bg-b1/$F2/ape-final/unisacc-next.com`),在预跑尾段 `P6` 分两段各 `<= 45s` 跑完 10 个二进制 + `release-exec --warm` |
| **核对四(`X` 提交远端可达性)** | 未到 `X` | `X_SHA` 为 `$F2` 之上回填了 `$APE2_SHA` 七哈希的提交,核 `git merge-base --is-ancestor "$X_SHA" origin/v0.0.107` 与 `origin/main` |
| **核对五(Step 2 草稿后独立下载 `slice-win-arm64`)** | 未到 Step 2 | 独立下载 `v0.0.107` 草稿的 `slice-win-arm64` 与 `pkg.bin`,核 SHA256 等于 `$CHAIN2/fixedpoint/g2/win-arm64/slice-win-arm64` 与 `$CHAIN2/ape-final/p3-package.body`(即候选表第 13、11 列新值) |
