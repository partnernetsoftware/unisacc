# 第二车 0.0.107 W0014(cx-booter-ape)预跑尾段单批补跑与合并汇账命令草稿

- 状态:**仅草稿(当前不执行)**,待预跑进入尾段(或 `agy-h` 报完成窗数达 `~80%` / `~200 窗`)且主车道不争抢时执行。
- 根因与依据(`cdx` 定因 + `tools/affected.cx:52,1130-1140`):
  - `#5`(`1d3e80a4c2`)已在 `tools/affected.cx` 为 `cx-booter-ape` 接入 `booter-raw-elf` 材料生产者 `booter_material(Prep *P)`;
  - `tools/affected.cx:1131-1140` 要求从环境变量 `UNISACC_BOOTER_CHAIN_OUT` 读取已封口链目录的规范绝对路径(`sy_realpath`),并校验其下:
    - `$CHAIN/fixedpoint/g2/lnx-x86_64/entry.elf`
    - `$CHAIN/ape-final/ape-receipt.txt`
    - `$CHAIN/ape-final/unisacc-next.com`
    - `$CHAIN/seal.tsv`
  - 因 `prerun-drive.cx -- run` 启动时未注入 `UNISACC_BOOTER_CHAIN_OUT`,`W0014` 在预跑首轮因 `booter-chain-out-required` 记为 `UNKNOWN rc=3`(`no-BATCH_RUN rc=3`,见 `strict-W0014.tsv`)。
  - 注:同属 `#6`(`b32a48e979`)修复的 `W0015`(`tools-default-regression`)在本次 `0.0.107` 预跑中已直接 **`PASS rc=0`**(`29962 ms`,`RAW BATCH_RUN id=tools-default-regression status=PASS`),无需补跑或 deferred。

---

## (1) 前置只读核验(确认链目录四件材料与 seal.tsv 完好)

```sh
R=$HOME/repos/unisacc
V=0.0.107
W=$R/.claude/worktrees/v$V
REV=28b6a518fe9fac6b117f31adeb25e1c8ab51701f
CHAIN=$HOME/.cache/unisacc/chains/bg-b1/$REV
APE=$CHAIN/ape-final/unisacc-next.com
PR=$HOME/.cache/unisacc/prerun/v$V-${REV:0:10}
FROZEN_ROOT=$PR/frozen-root

test "$(readlink -f "$CHAIN")" = "$CHAIN"
test -s "$CHAIN/fixedpoint/g2/lnx-x86_64/entry.elf"
test -s "$CHAIN/ape-final/ape-receipt.txt"
test -s "$CHAIN/ape-final/unisacc-next.com"
test -s "$CHAIN/seal.tsv"
```

---

## (2) 预跑尾段单批补跑命令原行(同 frozen-root、同候选 APE、新输出路径)

在预跑进入尾段(`~80%` 完成窗后)或 `W0246` 跑完后、`prerun-drive.cx -- merge` 之前执行:

```sh
export PATH="/usr/bin:/bin:$HOME/.local/bin"
export TMPDIR=/tmp
unset UNISACC_INTEGRATOR DVQUALIFY_NATIVE_RUN_BATCH UNISACC_RUN_CACHE UNISACC_UNIT_CACHE UNISACC_CACHE_DIR UNISACC_BOUND_DEADLINE_MONO UNISACC_E3CACHE
export UNISACC_BOOTER_CHAIN_OUT="$CHAIN"
test "$(command -v cc)" = /usr/bin/cc

cd "$FROZEN_ROOT"

# 1. 先单跑 affected.cx --materials-dump --prepare-materials 核验 MATERIAL 与 PREPARED 三齐
sh "$APE" tools/affected.cx -- --materials-dump --ids cx-booter-ape --prepare-materials -j 1

# 2. 在同 frozen-root、同候选 APE 上以独立输出文件 strict-W9914-cx-booter-ape.tsv 跑 dvqualify 单批
W0014_RERUN_OUT="$PR/strict-W9914-cx-booter-ape.tsv"
rm -f "$W0014_RERUN_OUT" "$W0014_RERUN_OUT.stderr"
timeout -k 5s 55s sh ./unisacc.com tools/dvqualify.cx -- . \
  --candidate "$APE" \
  --ids cx-booter-ape \
  --out "$W0014_RERUN_OUT" \
  --jobs 1 \
  --segment-batches 1 \
  2> "$W0014_RERUN_OUT.stderr"

cat "$W0014_RERUN_OUT"
cat "$W0014_RERUN_OUT.stderr"
```

---

## (3) 必过验收判据(三项全满足才算补跑通过)

1. `affected.cx --materials-dump` 输出:
   - `MATERIAL\tcx-booter-ape\tbooter-raw-elf\t<实有绝对路径>`(第 4 列非 `-`,无 `REFUSED=`);
   - `PREPARED\tbooter-raw-elf\t...` 且 `PREBUILD\tmaterials=1\t...\tfailed=0\tdeferred=0`。
2. `$W0014_RERUN_OUT` 包含以下两条核心原行:
   - `cx-booter-ape\tproduct-qualification\tPASS\t0\t9a1e2630100deace2b6a8d82a1b6c47fdf57b872e82f3cd4e1ee55756c2be544\t...`
   - `RAW\tBATCH_RUN\tid=cx-booter-ape\tstatus=PASS\trc=0\ttransport=0\t...\tcandidate=9a1e2630100deace2b6a8d82a1b6c47fdf57b872e82f3cd4e1ee55756c2be544\t...\tscope=actual-one-batch-no-cached-result\t...`
3. 原始 `$PR/strict-W0014.tsv` 的 `UNKNOWN 3` 行**完整保留不删改**(供合并器按同候选后补 `PASS` 折叠覆盖并留存审计轨迹)。

---

## (4) 合并器汇账命令(保留原 W0014 UNKNOWN 行并合入最终 strict.tsv)

```sh
cd "$W"
cp -f "$W0014_RERUN_OUT" "$W/release/evidence/0.0.107/strict-W9914-cx-booter-ape.tsv"
sh ./unisacc.com tools/scrub-home-paths.cx -- --write "$W/release/evidence/0.0.107/strict-W9914-cx-booter-ape.tsv"

# 预跑全部窗口完成后统一执行 merge 与 archive:
sh ./unisacc.com tools/prerun-drive.cx -- merge --in-dir "$PR" --out "$PR" --version "$V"
sh ./unisacc.com tools/prerun-drive.cx -- archive --in-dir "$PR" --repo "$W" --version "$V"
grep -E '^cx-booter-ape\t' "$W/release/evidence/0.0.107/strict.tsv"
```
