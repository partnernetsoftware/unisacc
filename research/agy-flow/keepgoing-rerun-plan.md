# 第二车 0.0.107 第二轮预跑(F4=63354b4a2c)补跑集与合并计划

- 版本与状态:`v9 第二轮 B-t 裁定版 (2026-11-25 06:45 +0800, PGID 3163300 已于 06:43:35 自然退净; W0000 已在 strict-W0000.tsv 转 PASS 8520ms; 待两个 B-t 提交落车后 --resume 续跑 W0213..W0240, 再串行补跑 W9915/W9916/W9917/W9993)`
- 第二轮预跑目录:`PR2=~/.cache/unisacc/prerun/v0.0.107-63354b4a2c`
- 冻结源目录:`FROZEN_ROOT=$PR2/frozen-root`(`REV=63354b4a2cb707ecc923a89f0df6030149fa697f`)
- 第二轮候选 APE:`APE2=~/.cache/unisacc/chains/bg-b1/63354b4a2cb707ecc923a89f0df6030149fa697f/ape-final/unisacc-next.com`(`sha256=b773c7e221ab059e935276f41c0ac4070737f2ae6c548cd56ea8b6b0acc53c63`, `bytes=8991230`)
- **累计计数看板(随预跑推进实时更新)**:
  - 截至 `06:08 +0800`(已完成 `96/241` 窗,含 `W0023..W0090` 全部 68 个 `netregen` 重窗 100% `PASS rc=0`;当前在途 `first=W0095 windows=2`):
    - **`PRERUN_INCOMPLETE` 累计 = `2` 窗**(`W0000` `module-units-forward`、`W0015` `precommit-list-cap`)
    - **`PRERUN_KEEP_GOING` 累计 = `3` 窗**(`W0016` `ratchets-tool-full`、`W0017` `tsv-lint-root`、`W0093` `slow-split`)
    - **`PRERUN_PRODUCT_RED_STOP` 累计 = `0` 窗**
- **第一轮 17 个 `UNKNOWN 3` 窗口在第二轮已前置根除**:
  - `(1)` `cx-booter-ape`(第二轮位于 `W0030`):第二轮预跑守护已显式导出 `UNISACC_BOOTER_CHAIN_OUT=$CHAIN2`;
  - `(2)` `netregen-*`(第二轮位于 `W0035..W0102`):`~/.cache/unisacc/prep-cache/old-product-cache/5f83ac93...` 已在 `03:33` 修为 `0700` + `fetch.lock`(`GUARD7_OLDPROD_0700=PASS`)。
- **执行时点与并发铁律(`cc` 裁定)**:
  - **严禁与在途预跑并发执行**:所有补跑统一在第二轮 241 窗完成(`PRERUN_RUN status=DONE` 且 `prerun-drive.cx` 退出)后立即在整机空闲状态下**串行执行**;
  - **每批先独立预备材料再单批真跑**:同 `F4` (`$FROZEN_ROOT`)、同 `APE2`、同正式布局,每批先跑 `sh "$APE2" tools/affected.cx -- --materials-dump --ids "$b" --prepare-materials -j 1` 核收据/键并备妥材料,再以 `jobs=1 --segment-batches 1` 单开新 `--out`(`$PR2/strict-W9900-module-units-forward.tsv`、`$PR2/strict-W9915-precommit-list-cap.tsv` 等),逐批确认终态后调用 `prerun-drive.cx -- merge` 与 `-- archive` 汇账。

---

## (1) 第二轮预跑已登记补跑/待裁定清单(截至 05:51,INCOMPLETE=2, KEEP_GOING=2)

| 序号 | 窗口号 | 批次名 (`batch`) | 原始状态与退出码 | 补跑目标文件 | 具体根因与第一轮对照证据 |
|---|---|---|---|---|---|
| 1 | `W0000` | `module-units-forward` | `PRERUN_INCOMPLETE` (`raw=timed-window-missing-evidence rc=124`, `58044ms` / `58047ms`) | `$PR2/strict-W9900-module-units-forward.tsv` | 置顶至 `W0000`(`phase=warmup`,在 `W0001` `lane-key-probes` 之前),在全新 `APE2` 下首次冷构造 `module-units-tool` 材料(`~42.8s`)叠加外层冷开销微超 55s。第一轮位于 `W0202`(热态)一次 `PASS rc=0` (`42244ms`, `assertions=7`),且第二轮 `W0014`(`module-units`)已在 `attempt=2` 把 `module-units-tool` 备妥并以 `10372ms` `PASS`。 |
| 2 | `W0015` | `precommit-list-cap` | `PRERUN_INCOMPLETE` (`raw=timed-window-missing-evidence rc=124`, `58052ms` / `58048ms`) | `$PR2/strict-W9915-precommit-list-cap.tsv` | 与 `W0000` 同形:置顶批在 `first=W0014 windows=2` 双车道并发冷窗位下首次预备材料叠加双车道争用达 `58.0s`。第一轮位于 `W0207`(单车道热态)一次 `PASS rc=0`。预跑收尾后整机空闲先独立预备该批材料再 `jobs=1` 单批真跑。 |
| 3 | `W0016` | `ratchets-tool-full` | `PRERUN_KEEP_GOING` (`tool-selfcheck FAIL 1`, `12592ms`) | `$PR2/strict-W9916-ratchets-tool-full.tsv` | **订正记录**:`ratchets-tool-full` 是 `0.0.107` 追加集新增批(`0.0.106` 旧注册表只有 `ratchets-tool` 跑 `--rows-only`,无 `ratchets-tool-full`),本次运行暴露 `tools/check-ratchets.cx:550-565` 夹具合同欠账(19 门合同 `reserved-names` 在 `0.0.106` 已入库,但 `check-ratchets.cx` 的 `full_run` 段仍断言 18 门、`17 skipped`、`1/19` 必拒 `rc=2`)。**处置**:按路径 B 修 `tools/check-ratchets.cx` 补丁并在补丁落车后同候选 `APE2` 补跑至 `$PR2/strict-W9916-ratchets-tool-full.tsv` 转 `PASS` 为准,**不登记 `expected-fail.tsv`**。 |
| 4 | `W0017` | `tsv-lint-root` | `PRERUN_KEEP_GOING` (`tool-selfcheck UNKNOWN 0`, `55577ms`, `transport-failed-raw-neg1`) | `$PR2/strict-W9917-tsv-lint-root.tsv` | 在 `first=W0016 windows=2` 双车道并发下首次预备 `tsv-lint-tool` 材料叠加全仓 TSV 扫描贴 55s 限(`55577ms`)。预跑收尾后整机空闲先独立预备 `tsv-lint-root` 材料(`--materials-dump --ids tsv-lint-root --prepare-materials -j 1`)再以 `jobs=1` 单批真跑。 |
| 5 | `W0093` | `slow-split` | `PRERUN_KEEP_GOING` (`tool-selfcheck FAIL 0`, `55556ms`) | `$PR2/strict-W9993-slow-split.tsv` | 与第一轮 `W0088` 及 `0.0.106` 完全同形:在 `first=W0093 windows=2` 双车道并发下两次贴 55s 限(`55538ms` / `55556ms`)。预跑收尾后整机空闲时先备材料再以 `jobs=1` 单批独立复跑(最多试两次,仍红则按 `0.0.106` 先例保留原行)。 |

---

## (2) 第二轮预跑收尾后串行单批补跑命令模板(先独立预备材料,再单批真跑)

### 公共环境初始化(同 frozen-root、同候选 APE2、短 TMPDIR、clean PATH)

```sh
R=$HOME/repos/unisacc
V=0.0.107
W=$R/.claude/worktrees/v$V
REV=63354b4a2cb707ecc923a89f0df6030149fa697f
CHAIN2=$HOME/.cache/unisacc/chains/bg-b1/$REV
APE2=$CHAIN2/ape-final/unisacc-next.com
PR2=$HOME/.cache/unisacc/prerun/v$V-${REV:0:10}
FROZEN_ROOT=$PR2/frozen-root

export PATH="/usr/bin:/bin:$HOME/.local/bin"
export TMPDIR=/tmp
unset UNISACC_INTEGRATOR DVQUALIFY_NATIVE_RUN_BATCH UNISACC_RUN_CACHE UNISACC_UNIT_CACHE UNISACC_CACHE_DIR UNISACC_BOUND_DEADLINE_MONO UNISACC_E3CACHE
export UNISACC_BOOTER_CHAIN_OUT="$CHAIN2"
test "$(command -v cc)" = /usr/bin/cc
cd "$FROZEN_ROOT"
```

### 串行补跑步骤 1:`W0000`(`module-units-forward`,先独立预备材料 -> 再单批真跑)

```sh
# 1a. 独立预备 module-units-forward 所需材料(module-units-tool)并核收据/键
sh "$APE2" tools/affected.cx -- --materials-dump --ids module-units-forward --prepare-materials -j 1

# 1b. 新 --out 单批真跑(保留原 strict-W0000.tsv 与 supervise.log 两行 rc=124 不动)
OUT_W00="$PR2/strict-W9900-module-units-forward.tsv"
rm -f "$OUT_W00" "$OUT_W00.stderr"
timeout -k 5s 60s sh ./unisacc.com tools/dvqualify.cx -- . \
  --candidate "$APE2" \
  --ids module-units-forward \
  --out "$OUT_W00" \
  --jobs 1 \
  --segment-batches 1 \
  2> "$OUT_W00.stderr"

grep -E '^module-units-forward\t|^RAW\tBATCH_RUN\tid=module-units-forward\t|^SUMMARY\t' "$OUT_W00"
```

### 串行补跑步骤 2:`W0015`(`precommit-list-cap`,先独立预备材料 -> 再单批真跑)

```sh
# 2a. 独立预备 precommit-list-cap 所需材料并核收据/键
sh "$APE2" tools/affected.cx -- --materials-dump --ids precommit-list-cap --prepare-materials -j 1

# 2b. 新 --out 单批真跑(保留原 strict-W0015.tsv 与 supervise.log 两行 rc=124 不动)
OUT_W15="$PR2/strict-W9915-precommit-list-cap.tsv"
rm -f "$OUT_W15" "$OUT_W15.stderr"
timeout -k 5s 60s sh ./unisacc.com tools/dvqualify.cx -- . \
  --candidate "$APE2" \
  --ids precommit-list-cap \
  --out "$OUT_W15" \
  --jobs 1 \
  --segment-batches 1 \
  2> "$OUT_W15.stderr"

grep -E '^precommit-list-cap\t|^RAW\tBATCH_RUN\tid=precommit-list-cap\t|^SUMMARY\t' "$OUT_W15"
```

### 串行补跑步骤 3:`W0016`(`ratchets-tool-full`,待路径 B `tools/check-ratchets.cx` 补丁落车并同步后 -> 先独立预备材料 -> 再单批真跑转 PASS)

```sh
# 3a. 确认 tools/check-ratchets.cx 路径 B 补丁已落入车树 $W(并按 cc 指示同步至补跑根),独立预备 ratchets-tool 材料
sh "$APE2" tools/affected.cx -- --materials-dump --ids ratchets-tool-full --prepare-materials -j 1

# 3b. 新 --out 单批真跑(保留原 strict-W0016.tsv 原红行不动,转 PASS 后不登记 expected-fail.tsv)
OUT_W16="$PR2/strict-W9916-ratchets-tool-full.tsv"
rm -f "$OUT_W16" "$OUT_W16.stderr"
timeout -k 5s 60s sh ./unisacc.com tools/dvqualify.cx -- . \
  --candidate "$APE2" \
  --ids ratchets-tool-full \
  --out "$OUT_W16" \
  --jobs 1 \
  --segment-batches 1 \
  2> "$OUT_W16.stderr"

grep -E '^ratchets-tool-full\t|^RAW\tBATCH_RUN\tid=ratchets-tool-full\t|^SUMMARY\t' "$OUT_W16"
```

### 串行补跑步骤 4:`W0017`(`tsv-lint-root`,先独立预备材料 -> 再单批真跑)

```sh
# 4a. 独立预备 tsv-lint-root 所需材料(tsv-lint-tool)并核收据/键
sh "$APE2" tools/affected.cx -- --materials-dump --ids tsv-lint-root --prepare-materials -j 1

# 4b. 新 --out 单批真跑(保留原 strict-W0017.tsv 不动)
OUT_W17="$PR2/strict-W9917-tsv-lint-root.tsv"
rm -f "$OUT_W17" "$OUT_W17.stderr"
timeout -k 5s 60s sh ./unisacc.com tools/dvqualify.cx -- . \
  --candidate "$APE2" \
  --ids tsv-lint-root \
  --out "$OUT_W17" \
  --jobs 1 \
  --segment-batches 1 \
  2> "$OUT_W17.stderr"

grep -E '^tsv-lint-root\t|^RAW\tBATCH_RUN\tid=tsv-lint-root\t|^SUMMARY\t' "$OUT_W17"
```

### 串行补跑步骤 5:`W0093`(`slow-split`,整机空闲先备材料 -> 再 `jobs=1` 单批真跑,最多试两次)

```sh
sh "$APE2" tools/affected.cx -- --materials-dump --ids slow-split --prepare-materials -j 1
OUT_W93="$PR2/strict-W9993-slow-split.tsv"
rm -f "$OUT_W93" "$OUT_W93.stderr"
rc=0
timeout -k 5s 60s sh ./unisacc.com tools/dvqualify.cx -- . \
  --candidate "$APE2" \
  --ids slow-split \
  --out "$OUT_W93" \
  --jobs 1 \
  --segment-batches 1 \
  2> "$OUT_W93.stderr" || rc=$?
grep -E '^slow-split\t|^RAW\tBATCH_RUN\tid=slow-split\t|^SUMMARY\t' "$OUT_W93" || true
```

---

## (3) 第二轮合并与归档命令原行(241 窗预跑 + 全部串行补跑确认 PASS 后执行)

```sh
cd "$W"
for f in "$PR2"/strict-W99*.tsv; do
  [ -f "$f" ] || continue
  base=$(basename "$f")
  cp -f "$f" "$W/release/evidence/0.0.107/$base"
  sh ./unisacc.com tools/scrub-home-paths.cx -- --write "$W/release/evidence/0.0.107/$base"
done

rm -f "$W/release/evidence/0.0.107/prerun-windows.tsv" "$W/release/evidence/0.0.107/prerun-supervise.log"
sh ./unisacc.com tools/prerun-drive.cx -- merge --in-dir "$PR2" --out "$PR2" --version "$V"
sh ./unisacc.com tools/prerun-drive.cx -- archive --in-dir "$PR2" --repo "$W" --version "$V"
cp -f "$PR2/strict-final.tsv" "$W/release/evidence/0.0.107/strict.tsv"
sh ./unisacc.com tools/scrub-home-paths.cx -- --write \
  "$W/release/evidence/0.0.107/strict.tsv" \
  "$W/release/evidence/0.0.107/prerun-supervise.log" \
  "$W/release/evidence/0.0.107/prerun-windows.tsv"

tail -n 5 "$W/release/evidence/0.0.107/strict.tsv"
```
