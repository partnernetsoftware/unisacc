# unisacc 0.1.0 零改动封存车逐步核对表

依据: `research/ccogweco/seal-0.1.0-evidence-map.md`、`research/ccogweco/notes-0.1.0-draft.md`、`archive/research-2026-11-25/2026-11-24/R37-seal.md`、`research/agy-h/clock-rule-draft.md`、`seed/ident.c:95-155`、`tools/bg-candidate.cx:677-758`、`tools/release-run.cx:432-630`、`tools/release-chain.cx:1-42,1540`、`tools/release-exec.cx:1420-1600`、`tools/release-post.cx:1-200`。
定位: `0.1.0` 为零代码生成变化的封版车(只收封版证明与版本号收口, 不进 `107` 追加集、不进新功能或材料补丁栈), 从 `0.0.107` 公开后推进至 `0.1.0` 公开。若准入第 1 条(连续 3 版六目标不动点)或第 3 条(零已知 P0)任一为假, 依据 `prd_mempalace.md:47` 与 `notes-0.1.0-draft.md:32` 当趟改称 `0.0.127`, 不称 `0.1.0`。

---

## 1. 核心硬断言 (0.1.0 与 0.0.107 零变化等价性)

封版车启动前、打包前与公开前均须通过以下三项相等性断言; 任一失败立即停止封版流程。

### (1) 零代码差异 `git diff --stat` 断言 (产品源码、表与打包适配器全零改动)

依据 `seed/ident.c:101, 110-115, 121-130, 152-155`, 进入产品闭包与 `SOURCES_SID` 的完整路径集合为 `src`、`cx`、`include`、`exec`、`kernel`、`seed`、`weights` 七目录加上 `iterate/kernel/typekw.tsv` 及 `tools/` 下 13 个构建支持文件:

```sh
git diff --stat <V107_FREEZE_COMMIT> <V010_FREEZE_COMMIT> -- \
  src cx include exec kernel seed weights iterate/kernel/typekw.tsv \
  tools/bound tools/bound.c tools/cxbin tools/safedel.cx tools/lib.sh \
  tools/build_ref.sh tools/export_ref.sh tools/refshim.h tools/reffoot.h \
  tools/sourceflat.cx tools/ape-pack.cx tools/release-ape.cx tools/cxlib/releaseversion.cx
```

辅核全树差异(确认只含版本槽、计划、能用清单冻结、`tools/old-product-assets.tsv`、封版证明/说明等非产品文件):

```sh
git diff --stat <V107_FREEZE_COMMIT> <V010_FREEZE_COMMIT>
```

- 期望原行: 第一条命令无任何输出 (`0 files changed`, rc=0); 第二条命令仅列版本元数据、证据与文档路径, 不含任何 `src/`、`cx/`、`include/`、`exec/`、`kernel/`、`seed/`、`weights/` 文件。
- 失败处置: 立即停链, 用 `git diff --name-only <V107_FREEZE_COMMIT> <V010_FREEZE_COMMIT> -- ...` 定位混入产品树的提交, 从 `v0.1.0` 分支剥离后再重验, 不得以跳过该检查继续封版。

### (2) 源快照 `SOURCES_SID` 相等断言

```sh
SNAP_107="$HOME/.cache/unisacc/chains/snap/<V107_FREEZE_COMMIT>"
SNAP_010="$HOME/.cache/unisacc/chains/snap/<V010_FREEZE_COMMIT>"
IH="${UNISACC_IDENT_HELPER:-$HOME/.cache/unisacc/ident-helper/identity-helper-real5}"
SID_107=$("$IH" "$SNAP_107")
SID_010=$("$IH" "$SNAP_010")
echo "SID_107=$SID_107"
echo "SID_010=$SID_010"
test -n "$SID_010" && test "$SID_107" = "$SID_010"
```

同步核对链产物记录中的 `source_sid` 与 `release-run --pack` 输出:

```sh
sh ./unisacc.com tools/bg-candidate.cx -- promote --check --rev <V010_FREEZE_COMMIT> | grep '^NEED	source_sid	'
```

- 期望原行:
  - `SID_107=<64HEX>` 且 `SID_010=<同一64HEX>` (`test` 返回 `rc=0`)
  - `NEED	source_sid	<同SID_107的64HEX>`
  - `release-run --pack` 输出 `SOURCES_SID	VERIFIED	<同SID_107的64HEX>` (`tools/release-run.cx:548`)
- 失败处置: 若 `SID_010 != SID_107`, 调 `"$IH" --list` (`seed/ident.c` `list_paths` 调试) 或比对两快照文件列表逐一排查越界改动(或检查是否错误启用了含 `.git/` 节点的旧版 `identity-helper`), 解决并重建快照后再重新起链。

### (3) SID、未注版材料与两次正式 APE 重封复现

依据 `fable` 裁定(已执行为准): `0.1.0` 候选 `APE` 在打包阶段 (`tools/ape-pack.cx:294-296` `stamp_version` -> `rv_inject_slot`, 以及 `tools/version-inject.cx:1-5,25` 对 `slice-win-arm64`) 会向保留 32 字节版本槽注入 `0.1.0` (`cx/lib`/运行期槽位 `UNISACC_VERSION_SLOT_0000000000`), 因此 `0.1.0` 的 `APE sha256` 必不同于 `0.0.107` (`APE_010 != APE_107`)。`tools/release-exec.cx:667,676,827` 与 `.github/workflows/candidate-native.yml:44,76,245,723` 的 `APE_SHA` 钉都会直接拒绝版本或候选哈希不一致的复用; `0.1.0` 不复用 `0.0.107` 的整链产物、`strict` 原行或 `CI` 资格, 也不放宽任何时限, 必须以 `0.1.0` 候选独立全套实跑。

实际执行以 research/cdx/v010-reseal-plan.md 的两次重封为准。产品闭包、六个未注版切片、compiler.p4.pkg 与正式 pkg.bin 逐字节相等。使用同一套正式封包输入，按各自候选表注入 0.0.107 与 0.1.0，两次私有重封分别与各自正式链 APE 相等。32 字节是原始版本槽的宽度。压缩、PE 版本资源与 Mach-O 重签会传播注版差异，两版最终 APE 的 cmp -l 实测有 599194 个不同字节位置。

以下旧步骤仅供历史追溯。它们使用旧产物路径，且单切片注版与裸 APE 搜字串无法证明两次正式封包复现。封版准入执行上一段链接的重封命令。

```sh
CHAIN_107="$HOME/.cache/unisacc/chains/bg-b1/<V107_FREEZE_COMMIT>"
CHAIN_010="$HOME/.cache/unisacc/chains/bg-b1/<V010_FREEZE_COMMIT>"
# 1) 未注版核心包与六目标切片逐字节相等 (tools/release-run.cx:436,596)
for f in p3.pkg win-x86_64 lnx-x86_64 lnx-arm64 osx-x86_64 osx-arm64 win-arm64; do
  h107=$(sha256sum "$CHAIN_107/fixedpoint/g2/$f" | awk '{print $1}')
  h010=$(sha256sum "$CHAIN_010/fixedpoint/g2/$f" | awk '{print $1}')
  test -n "$h010" && test "$h107" = "$h010" || { echo "UNINJECTED_DIFF $f"; exit 1; }
done

# 2) 打包后 pkg.bin 与 107 相等, APE_010 必不同于 APE_107 且 --version 为 0.1.0
PKG_107=$(awk -F'\t' 'NR==2{print $11}' <V107_CANDIDATE_TSV>)
PKG_010=$(sha256sum -- <OUT>/layout/pkg.bin | awk '{print $1}')
test -n "$PKG_010" && test "$PKG_107" = "$PKG_010"
APE_107=$(awk -F'\t' 'NR==2{print $9}' <V107_CANDIDATE_TSV>)
APE_010=$(sha256sum -- <OUT>/layout/unisacc.com | awk '{print $1}')
echo "APE_107=$APE_107"
echo "APE_010=$APE_010"
test -n "$APE_010" && test "$APE_107" != "$APE_010"
sh -- <OUT>/layout/unisacc.com --version
```

旧的裸文件搜索与未封包切片槽位探针：

```sh
# 证伪步 1: 候选 APE 字节中不得残留 0.0.107 版本字面
cnt=$(grep -a -c '0\.0\.107' <OUT>/layout/unisacc.com || true)
echo "GREP_00107_COUNT=$cnt"
test "$cnt" -eq 0

# 证伪步 2: 同一未注版切片分别按 0.0.107 与 0.1.0 跑 tools/version-inject.cx 比 sha256
IN_SLICE="$CHAIN_010/fixedpoint/g2/win-arm64"
printf 'version\tcommit\n0.0.107\t<V107_FREEZE_COMMIT>\n' > /tmp/seal-v107.tsv
printf 'version\tcommit\n0.1.0\t<V010_FREEZE_COMMIT>\n' > /tmp/seal-v010.tsv
sh ./unisacc.com tools/version-inject.cx -- "$IN_SLICE" /tmp/seal-inj-107.bin /tmp/seal-v107.tsv
sh ./unisacc.com tools/version-inject.cx -- "$IN_SLICE" /tmp/seal-inj-010.bin /tmp/seal-v010.tsv
H_INJ_107=$(sha256sum /tmp/seal-inj-107.bin | awk '{print $1}')
H_INJ_010=$(sha256sum /tmp/seal-inj-010.bin | awk '{print $1}')
H_LAY_ARM=$(sha256sum -- <OUT>/layout/slice-win-arm64 | awk '{print $1}')
test -n "$H_INJ_010" && test "$H_INJ_107" != "$H_INJ_010"
test "$H_INJ_010" = "$H_LAY_ARM"
cmp -l /tmp/seal-inj-107.bin /tmp/seal-inj-010.bin | wc -l
```

- 期望原行:
  - 未注版 `p3.pkg` 与六切片 `h107 == h010`, 且 `<OUT>/layout/pkg.bin` `PKG_010 == PKG_107` (`rc=0`)
  - `<OUT>/layout/unisacc.com` 报 `unisacc 0.1.0`, 且 `APE_010 != APE_107`
  - 证伪步 1 输出 `GREP_00107_COUNT=0` (`test rc=0`)
  - 旧探针的槽位范围只描述直接注版的原始切片。最终 APE 的验收采用重封复现。
- 失败处置: 若未注版 `p3.pkg` 或六切片任何一项 `h107 != h010`, 说明自举构造链存在非确定性(环境变量泄漏、`cache-manager` 源不一致或 `UNISACC_RUN_CACHE` 污染), 立即中止封版车并用 `tools/fixedpoint-evidence.cx` 定位分叉点; 若 `GREP_00107_COUNT != 0` 或 `H_INJ_010 != H_LAY_ARM`, 说明打包版本槽未按当次表注入, 立即停发布并检查 `release/candidate.tsv` 与 `tools/ape-pack.cx`。

---

## 2. 时钟条件 6 的精确计时点锚定

依据 `research/agy-h/clock-rule-draft.md:39-62`、`notes-0.1.0-draft.md:28, 58-69` 与 `R37-seal.md:69-74`, 对封版车同时记录以下 4 个不可篡改时间戳(`T1` 为共同起点):

| 计时点 | 名称 | 来源文件与取值行 | 用途与阈值 |
|---|---|---|---|
| `T1` | 起链时刻 (`chain_start`) | `~/.cache/unisacc/chains/bg-b1/<V010_REV>/owner.tsv` 首行时间字段 | 两种口径共同起点 (`first_chain_start` 辅记重起次数 `<= 1`) |
| `T2` | 链止时刻 (`chain_stop`) | `bg-candidate` `chain-run` 末行 / `exit` 时间字段 | 链耗时 `(T2 - T1)/60` (目标 ~`28m`) |
| `T3_X` | 提交 `X` 时刻 (`exec-1-X-commit`) | `release/evidence/0.1.0/INDEX.tsv` 行 `exec-1-X-commit` 列 2 | 子段 `stop -> X` `(T3_X - T2)/60` (`<= 30m` 验收 / `<= 45m` 硬线) |
| `T3_R6` | `READY_TO_EXECUTE` 时刻 | `release-chain run` 末行 `CHAIN	RESULT	READY_TO_EXECUTE	version=0.1.0` 输出时刻 | `R6` 重述口径(封版车公开前可判)终点: `(T3_R6 - T1)/60 <= 120.0` |
| `T4` | 公开时刻 (`exec-5-publish`) | `release/evidence/0.1.0/INDEX.tsv` 行 `exec-5-publish` 列 2 | 条 6 原文口径终点: `(T4 - T1)/60 <= 120.0` |

核算命令原行 (T3_R6 / T4 产生后立即执行):

```sh
# 原文口径: chain_start (T1) -> exec-5-publish (T4)
date -d "<T1_STR>" +%s; date -d "<T4_STR>" +%s
# 计算 (T4_EPOCH - T1_EPOCH) / 60; 判据: <= 120.0 记 PASS, 否则 OVER_120M
```

说明: 按 `notes-0.1.0-draft.md:68-69`, 原文条款要求连续三版 (`0.0.106`、`0.0.107`、`0.1.0`) 均 `<= 120` 分钟。若 `0.0.106` 已超 `120` 分钟, 即使 `0.1.0` 本身达标, 三版连续样本数仍不足 (`<3`), 须在 `notes-0.1.0.md` 按实际数字如实记条 6 未满足并引用 R6/豁免记录, 不得剔除预跑耗时。

---

## 3. 从 0.0.107 公开后到 0.1.0 公开的逐步清单 (Step 0 - Step 9)

预计总墙钟 (无失败单趟基准): 约 `94-102` 分钟 (公开后收口另计; 依 `fable` 裁定不复用 `0.0.107` 整链、`strict` 或 `CI` 资格, 亦不放宽任何时限)。合规安全垫来自五项无需裁定手段——前置并行、构造缓存、取证模板、公开前 `--warm` 与读回精确 argv 预热、`release-post` 预演——总计可提供约 `12-20` 分钟余量, 上沿情形靠预跑落至低端稳定达标。单条本地子命令均守 `<= 60` 秒界(后台长跑均以独立作业运行、有界观测)。

### Step 0: 107 公开收口、旧基线登记与准入材料冻结 (链外准备, 约 10-15 分钟)

- 前置:
  - `0.0.107` 已完成 `exec-5-publish`、`exec-6-public-readback` 与 `exec-7-Y2-done` 推送 `origin/main`
  - `0.0.107` 已有 `release/evidence/0.0.107/{fixedpoint-6targets.tsv,ape-receipt.txt}` 且 `strict.tsv` 实跑 `FAIL=0`
- 命令原行:

```sh
sh ./unisacc.com tools/release-post.cx -- run --version 0.0.107 --execute
# 核 build/version-seq.tsv 与 release/evidence/0.0.107/INDEX.tsv 已封口
grep -E '^(106|107|0\.1\.0)' build/version-seq.tsv || grep '0\.0\.107\|0\.1\.0' build/version-seq.tsv
```

登记 `0.0.107` 到 `tools/old-product-assets.tsv` (供 `0.1.0` `netregen` 12 批比对 `0.0.107`, 使字面 `FAIL=0`), 冻结准入 7 能用清单 30 项与准入 3 `P0` 账本 (`F1` 与 `chain-step` 在 `107` 实跑关行), 建立 `v0.1.0` 分支与工作树并同步 `origin/main`:

```sh
git worktree add .claude/worktrees/v0.1.0 -b v0.1.0 origin/main
cp unisacc.com unisacc.com.build.json .claude/worktrees/v0.1.0/
sh ./unisacc.com tools/tables-gen.cx -- --check
sh ./unisacc.com tools/derive.cx -- --check
```

- 期望原行:
  - `RELEASE_POST	status=DONE`
  - `0.0.107` 行在 `build/version-seq.tsv` 处于 `已公开`
  - `tools/old-product-assets.tsv` 末尾含 `0.0.107` 公开资产行 (`size` 与 `sha256` 同 `0.0.107` `candidate.tsv`)
  - `TABLES_GEN` `stale=0 hard=0` 且 `DERIVE_SUMMARY` `drift=0`
- 失败处置:
  - 若 `release-post` 某步 `BLOCKED:<step>`, 按 `RELEASE_POST_RETRY` 原命令补跑该步;
  - 若 `0.0.107` 缺 `fixedpoint-6targets.tsv` 或 `P0` 未清零 (`P0-126`/`P0-F1`/`P0-chain-step-reaped-kill`), 停止开 `0.1.0` 封版车, 按 `R37-seal.md:32, 53` 改开普通修复版 `0.0.127`;
  - 若 `old-product-assets.tsv` 漏登 `0.0.107`, `0.1.0` 的 `netregen` 12 批会继续红, 必须补登后再进 Step 1。
- 预计分钟: `10-15` 分钟 (此步在 `T1` 起链前完成, 不计入条 6 `chain_start` 墙钟)。

### Step 1: 零代码差异校验、SID 校验与封版起链前置 (`chain-ready`, 约 3-5 分钟)

- 前置:
  - Step 0 完成, `.claude/worktrees/v0.1.0` 内已合入 `0.1.0` 版本槽 (`release/candidate.tsv`、`plans/v0.1.0.md`、`build/freeze.tsv`), 已复制 `unisacc.com` 真实副本 (非软链)
- 命令原行:

```sh
cd .claude/worktrees/v0.1.0
# (1) 零代码差异断言 (第 1 节 (1) 全路径命令)
git diff --stat <V107_FREEZE_COMMIT> HEAD -- \
  src cx include exec kernel seed weights iterate/kernel/typekw.tsv \
  tools/bound tools/bound.c tools/cxbin tools/safedel.cx tools/lib.sh \
  tools/build_ref.sh tools/export_ref.sh tools/refshim.h tools/reffoot.h \
  tools/sourceflat.cx tools/ape-pack.cx tools/release-ape.cx tools/cxlib/releaseversion.cx
# (2) 派生/棘轮/起链前置 (20/20)
DEVSVC_OWNER=cc sh ./unisacc.com tools/chain-ready.cx -- --version 0.1.0
```

- 期望原行:
  - `git diff --stat` 输出为空 (0 行, `rc=0`)
  - `chain-ready.cx` 末行输出 `READY=yes` (20/20 项全绿)
- 失败处置:
  - 若 `git diff --stat` 非空: 撤除越界进产品目录的改动, 保证 `0.1.0` 为纯零改动封版槽;
  - 若 `chain-ready.cx` 返回 `READY=no`: 逐条排查红项(重点核 12-14 真实副本、`build/freeze.tsv` 截止、`origin/main` 落后数为 0), 修复并重新提交版本分支后再复检。
- 预计分钟: `3-5` 分钟。

### Step 2: 冻结提交与启动封版候选整链 (`[计时点 T1: chain_start]`, 约 28-32 分钟)

- 前置:
  - Step 1 返回 `READY=yes` 且 zero product diff 为真;
  - `main` 进入冻结窗口 (`prd_mempalace.md:39`: 至 `0.1.0` `release-post` 完成前不得向 `origin/main` 推送);
  - 环境中未设置 `UNISACC_RUN_CACHE` 与 `UNISACC_CACHE_DIR`, 仅设置 `UNISACC_IDENT_HELPER` (`prd_mempalace.md:65`)。
- 命令原行:

```sh
unset UNISACC_RUN_CACHE UNISACC_CACHE_DIR
export UNISACC_IDENT_HELPER="${UNISACC_IDENT_HELPER:-$HOME/.cache/unisacc/ident-helper/identity-helper-real5}"
V010_REV=$(git rev-parse HEAD)
# 在 v0.1.0 工作树起链(后台有界作业), 并记录 T1:
head -n 3 "$HOME/.cache/unisacc/chains/bg-b1/$V010_REV/owner.tsv"
```

同步执行 `SID_107 == SID_010` 校验 (第 1 节 (2) 命令原行)。

- 期望原行:
  - `owner.tsv` 首行时间戳记录 `[T1]` (`chain_start`), `mode=full`, `candidate=source`
  - `SID_010 == SID_107` (`test rc=0`)
  - 链作业结束输出 `DONE <N>/<N> rc=0` (`[T2]` `chain_stop`), 其中 `compare` 相位打印 `slices=same count=6`、`objects=same count=102`、`kernels=same count=2`、`product=same`
- 失败处置:
  - 若 `SID_010 != SID_107`: 立即中止链作业, 比对快照清单定位哪一文件变动;
  - 若链本体单步超时/失败: 读失败子作业日志定位具体步与源码行后再决策重跑; 按 `R30` 每版正式起链失败不超过 `1` 次, 辅记 `first_chain_start`; 若连续两次同一失败则按铁律停手定因, 超 `T+24` 或确认不满足不可豁免条则退为 `0.0.127`。
- 预计分钟: `28-32` 分钟 (计入条 6 墙钟 `T1 -> T2`)。

### Step 3: 候选提升复核、第 (3) 条等价断言与两步证伪、六目标不动点取证 (约 2-3 分钟)

- 前置:
  - Step 2 整链完成 (`rc=0`, `[T2]` 已记录); `chains/bg-b1/<V010_REV>/seal.tsv` 与 `ape-final/ape-receipt.txt` 已落盘。
- 命令原行:

```sh
sh ./unisacc.com tools/bg-candidate.cx -- promote --check --rev "$V010_REV"
# 校验 SID_010==SID_107 与第 1 节 (3) 的未注版切片/pkg.bin相等、APE_010!=APE_107 及两步证伪
# 提取并校验 0.1.0 六目标不动点两件入仓证据:
sh ./unisacc.com tools/fixedpoint-evidence.cx -- \
  "$HOME/.cache/unisacc/chains/bg-b1/$V010_REV" 0.1.0 .
sh ./unisacc.com tools/check-fixedpoint-evidence.cx -- \
  release/evidence/0.1.0/fixedpoint-6targets.tsv release/evidence/0.1.0/ape-receipt.txt
```

- 期望原行:
  - `PROMOTE_CHECK	OK	rev=<V010_REV>	phases=<N>/<N>	inputs=<M>	not=PUBLISHED	not=PASS`
  - `NEED	source_sid	<同V107的64HEX>`
  - `NEED	ape	...	sha=<V010_APE_SHA>` (打包并注版后 `APE_010 != APE_107`, 未注版 `fixedpoint/g2` 六切片与 `p3.pkg`/`pkg.bin` 与 `0.0.107` 相等, 且第 1 节 (3) 两步证伪通过)
  - `fixedpoint-6targets.tsv` 含六目标 (`lnx/x86_64`, `lnx/arm64`, `osx/x86_64`, `osx/arm64`, `win/x86_64`, `win/arm64`) 全部 `SAME`; `check-fixedpoint-evidence.cx` 退出 `rc=0`
- 失败处置:
  - 若 `promote --check` 报 `receipt-tampered`/`input-mismatch`/`no-source-sid`: 禁止手动修改 `seal.tsv`, 须排查缓存目录损坏并重跑对应收口相位或重起链;
  - 若不动点六切片任何一对哈希不一致 (`!=`): 准入第 1 条判定为假(不可豁免!), 立即中止 `0.1.0` 封版流程, 当趟降格改走 `0.0.127`。
- 预计分钟: `2-3` 分钟。

### Step 4: 同候选严格预跑与准入第 2 / 4 / 7 条机械核算 (约 30-50 分钟, 视 WP 拆片合入情况)

- 前置:
  - Step 3 `PROMOTE_CHECK OK`, 候选 APE 已确定(`APE_010`); 能用清单 30 项已在 Step 0 冻结。
- 命令原行:

```sh
# 依 fable 裁定不得复用 0.0.107 strict 原行(release-exec.cx:667,676,827), 必须用 0.1.0 候选 APE_010 完整实跑生成 strict.tsv:
# 结束后检查 SUMMARY、UNKNOWN 及能用清单 30 项命中数:
grep -E '^SUMMARY|^UNKNOWN|cx-booter-ape|tools-default-regression|netregen-' release/evidence/v0.1.0-strict-<DATE>.tsv | head -n 40
```

- 期望原行:
  - `SUMMARY` 行含 `ape_sha256=<APE_010>`, 且 `FAIL=0`, `INVALID=0` (`netregen` 12 批因已在 Step 0 登记 `0.0.107` 期望全转 `PASS`)
  - `UNKNOWN` 为 `0` (若 `3a`/`3b` 均已落 `107`), 或按 `R2` 不超 `2` (`cx-booter-ape`、`tools-default-regression`) 且全部登记于 `expected-unmeasured.tsv`
  - 准入 7 能用清单机械映射严格命中 `>= 25 / 30` (由固定脚本/批核输出具体分数)
- 失败处置:
  - 若 `FAIL > 0`: 按条 2 口径 (`R37-seal.md:45`), 封版车不接受 `FAIL > 0`; 如为 `netregen` 12 批红则核查 `old-product-assets.tsv` 基线选取(读日志定因); 如为不稳批(例如 `slow-split`)则单批复跑并按原始表规则记 `SUPERSEDED` 与定因, 或修复工具侧缺陷后只补跑受影响合集;
  - 若 `state-unverified` 声称类实落 `> 3` (如 `U06` 未能定因关闭而落 `4` 行): 按 `R38` 裁定不搬账、不改 `kind`, 在 `notes-0.1.0.md` 如实记条 4 未满足并保留豁免说明。
- 预计分钟: `30-50` 分钟 (依赖前置拆片优化到位情况; 此为条 6 `stop -> X <= 45m` 关键瓶颈段)。

### Step 5: 打包 (`--pack`)、预检草案 (`--prepare`) 与 `release-chain` 执行准备 (`[计时点 T3_X / T3_R6]`, 约 5-8 分钟)

- 前置:
  - Step 3 与 Step 4 全部完成 (`PROMOTE_CHECK OK`, `strict.tsv` `SUMMARY` 已固化)。
- 命令原行:

```sh
sh ./unisacc.com tools/release-run.cx -- --pack . release/candidate.tsv \
  "<SRC_DIR>" "<OUT>" --chain-out "$HOME/.cache/unisacc/chains/bg-b1/$V010_REV" --commit "$V010_REV"

sh ./unisacc.com tools/release-run.cx -- --prepare 0.1.0 --layout "<OUT>/layout" \
  --strict "release/evidence/v0.1.0-strict-<DATE>.tsv" --commit "$V010_REV" --prev 0.0.107

sh ./unisacc.com tools/release-chain.cx -- run --inputs "<INPUTS_FILE>" \
  --state "<STATE_DIR>" --layout "<OUT>/layout"
```

- 期望原行:
  - `release-run --pack`: 输出 `SOURCES_SID	VERIFIED	<SID_010>`, `BUILD_JSON_COMMIT	<V010_REV>`, `RESULT	PACK_OK`
  - `release-run --prepare`: 输出 `PREP	SUMMARY	version=0.1.0	prev=0.0.107	commit=<V010_REV>	ape=<APE_010>	...	new_fail=0` (`rc=0`, 如存在已登记 `UNKNOWN` 则 `rc=1` 并确认在预期列表中)
  - `release-chain run`: 通过步骤 `1 prep -> 2 stage-a -> 3 stage-commit (产生 exec-1-X-commit, 记录 T3_X) -> 4 stage-dry`, 末行打印 `CHAIN	RESULT	READY_TO_EXECUTE	version=0.1.0` (记录 `[T3_R6]`), 同时校验 `(T3_X - T2)/60 <= 45.0` 且 `(T3_R6 - T1)/60 <= 120.0` (`R6` 重述口径)
- 失败处置:
  - 若 `--pack` 报 `REFUSE sources-sid-mismatch`: 检查 `--commit` 是否等于当前分支冻结提交与快照是否对应;
  - 若 `release-chain run` 在某步 `CHAIN FAIL <N> <NAME>`: 读日志修正后用 `--from <N>` 恢复; 若 `stage-commit` 报 `X_PUSHED` 失败则确认 `X` 已提交并推送至 `origin/main` (见 `tools/release-chain.cx:22-23`)。
- 预计分钟: `5-8` 分钟。

### Step 6: 创建草稿、草稿读回与原生六格 CI (`exec-2`, `exec-3`, 约 8-12 分钟, 绝不复用 107 CI)

- 前置:
  - Step 5 达到 `CHAIN	RESULT	READY_TO_EXECUTE	version=0.1.0`; `X` 提交已在 `origin/main`; `.github/workflows/candidate-native.yml:44,76,245,723` 的 `APE_SHA` 钉绑定当次 `APE_010`。
- 命令原行:

```sh
sh ./unisacc.com tools/release-exec.cx -- --execute release/candidate.tsv \
  --layout "<OUT>/layout" --strict "release/evidence/v0.1.0-strict-<DATE>.tsv" \
  --from-step 2 --to-step 3
```

(或整段执行至公开前一步, 由 `devsvc x --bg` 托管。)

- 期望原行:
  - `exec-2-draft-readback`: `PASS	draft-readback.txt	gh release download, SHA256SUMS OK`
  - `exec-3-ci`: 六目标 (`lnx/x86_64`, `lnx/arm64`, `osx/x86_64`, `osx/arm64`, `win/x86_64`, `win/arm64`) 全绿, `CANDIDATE_QUALIFY	verdict=QUALIFIED	pass=42	fail=0	not_run=0	invalid=0`
- 失败处置:
  - 若 CI 遇瞬态 404 或单格超时 (`prd_mempalace.md:146`): 先读发码源码行与日志, 若属已知瞬态只允许 `gh run rerun --failed` 一次并按 `release-exec --from-step 3 --redispatch` 重派, 并在 `notes-0.1.0.md` 披露; 第二次失败立即当确定性缺陷停手, 不强发 `0.1.0`。
- 预计分钟: `8-12` 分钟。

### Step 7: 发布说明回填与 `Y1` 决策提交 (`exec-4`, 约 2-3 分钟)

- 前置:
  - `exec-3-ci` `QUALIFIED`, `ci-<ID>.json/.log` 已落仓; `notes-0.1.0.md` 已按 `notes-0.1.0-draft.md` 填完七条准入真实值(`P0`、声称类计数、能用清单得分、不动点三版、`B2` 顺延豁免、状态行固定句 `同候选六格原生 CI 与全套 s9 是两件事`)。
- 命令原行:

```sh
sh ./unisacc.com tools/notes-fill.cx -- --notes release/notes-0.1.0.md \
  --candidate release/candidate.tsv --decision release/decision.txt \
  --evidence release/evidence.tsv --out "<OUT>/notes-0.1.0.filled.md"
# 审阅无待填且符合无绝对家目录路径/零星号后写入 release/notes-0.1.0.md, 执行 Y1:
sh ./unisacc.com tools/release-exec.cx -- --execute release/candidate.tsv \
  --layout "<OUT>/layout" --strict "release/evidence/v0.1.0-strict-<DATE>.tsv" \
  --from-step 4 --to-step 4
```

- 期望原行:
  - `NOTES_FILL filled=<K> unfilled=0`
  - `release-decide.cx` 产出当次 `0.1.0` `release/decision.txt` (`check_decision_current` 通过)
  - `GATE	4	Y1_PUSHED	PASS	<Y1_COMMIT_40HEX>`
  - `release/evidence/0.1.0/INDEX.tsv` 含 `exec-4-Y1` 与 `exec-4-Y1-frozen`
- 失败处置:
  - 若 `notes-fill` 输出 `UNFILLED`: 逐行补齐对应上游字段再重试;
  - 若 `DECISION` 门拒绝 `decision.txt does not reference candidate version`: 重新用 `release-decide.cx` 从当次 `evidence.tsv` 现算并检查 `0.1.0` 版本号写入。
- 预计分钟: `2-3` 分钟。

### Step 8: 正式公开 (`exec-5-publish`) 与时钟条件 6 终算 (`[计时点 T4]`, 约 1-2 分钟)

- 前置:
  - Step 7 `Y1_PUSHED PASS`; `release/evidence.tsv` 与 `release/decision.txt` 未再修改 (`gate_y1_frozen`)。
- 命令原行:

```sh
sh ./unisacc.com tools/release-exec.cx -- --execute release/candidate.tsv \
  --layout "<OUT>/layout" --strict "release/evidence/v0.1.0-strict-<DATE>.tsv" \
  --from-step 5 --to-step 5
# 立即核对 exec-5-publish 时间戳 (T4) 与 T1 差值:
grep '^exec-5-publish' release/evidence/0.1.0/INDEX.tsv
```

- 期望原行:
  - `INDEX.tsv` 新增 `exec-5-publish	<YYYY-MM-DD HH:MM:SS +0800>	-	v0.1.0	isDraft=false, Latest` (`[T4]`)
  - 时钟校验: `(T4 - T1)/60 <= 120.0` (及 `R6` 口径 `(T3_R6 - T1)/60 <= 120.0`), 同时核对 `0.0.106`、`0.0.107`、`0.1.0` 三版序列计数是否达到 `3/3`
- 失败处置:
  - 若 `step_publish` 报错 `PUBLIC_SHA release still draft`: 按 `rollback` (`release-exec.cx:292-300`) 处理或检查 GitHub Release 状态后再决定;
  - 若已公开且 `(T4 - T1)/60 > 120.0` 或前三版中已有超 `120` 分钟样本: 条 6 如实记为未满足/样本不足(不得修改历史时间或剔除预跑), 在后续勘误/公开后指标中留档。
- 预计分钟: `1-2` 分钟。

### Step 9: 公开读回、`Y2` 密封、`release-post` 与公开后即时核验 (`exec-6`, `exec-7`, `S10`, 约 3-5 分钟)

- 前置:
  - Step 8 `exec-5-publish` 已完成。
- 命令原行:

```sh
sh ./unisacc.com tools/release-exec.cx -- --execute release/candidate.tsv \
  --layout "<OUT>/layout" --strict "release/evidence/v0.1.0-strict-<DATE>.tsv" \
  --from-step 6 --to-step 7

sh ./unisacc.com tools/release-post.cx -- run --version 0.1.0 --execute

# 公开后即时只读复核 (<= 60s):
gh release view v0.1.0 -R partnernetsoftware/unisacc \
  --json tagName,isDraft,isPrerelease,isLatest,targetCommitish
gh release view v0.0.107 -R partnernetsoftware/unisacc \
  --json tagName,isDraft,isLatest
```

- 期望原行:
  - `public-readback.txt` 末段含 `C47_1_READBACK PASS expected_draft=false tag=v0.1.0 version=0.1.0 commit=<V010_REV>` 与 `READBACK_SHA unisacc.com <APE_010>`
  - `INDEX.tsv` 陆续写入 `exec-6-public-readback`、`exec-7-Y2`、`exec-7-Y2-done`、`post-publish`
  - `RELEASE_POST	status=DONE` (`build/version-seq.tsv` 中 `0.1.0` 标 `已公开`)
  - `gh release view v0.1.0` 返回 `"isDraft":false,"isLatest":true,"targetCommitish":"<V010_REV>"`; `v0.0.107` 自动降为 `"isLatest":false`
- 失败处置:
  - 若 `step_readback` 超时 `rc=124`: 工具内部会自动按 `--resume` 复跑 1 次 (`release-exec.cx:1504`); 若仍非零则触发 `rollback` (`v0.1.0` 转回 draft, `v0.0.107` 重新 `--latest`), 前向 `git revert` 并保留证据;
  - 若 `release-post` `BLOCKED`: 按输出的 `RELEASE_POST_RETRY ... --step <N>` 单步恢复, 在 `Y2` 与 `post-publish` 未落库推送前不解除 `main` 冻结口。
- 预计分钟: `3-5` 分钟 (不计入 `T1 -> T4`, 计入收口解除冻结)。
