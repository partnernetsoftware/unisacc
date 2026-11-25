# 0.1.0 预跑完成后 30 分钟收口时间轴(t0=prerun DONE -> t30=Step 1 X)

## 0. 全局绑定常量与铁律

- 工作树(`W010`): `<REPO>/.claude/worktrees/v0.1.0` (分支 `v0.1.0`, 冻结起链提交 `F = a11d28fc0a61939ee71345d732a7dcf205b261f1`)
- 链目录(`CDIR`): `~/.cache/unisacc/chains/bg-b1/a11d28fc0a61939ee71345d732a7dcf205b261f1`
- 候选 APE(`APE`): `$CDIR/ape-final/unisacc-next.com` (`sha256 = b7cae17b1ae557a4907247e3168db56925504819f98d15f1b75a2da2de8fa97f`, `8991230` B, `unisacc 0.1.0`)
- 源快照(`SOURCES_SID`): `37ff46cd02171c8ad2d3ba04dd084c296f5290a992077f8935e8b853e67d3238` (与 `0.0.107` 100% 相同)
- 预跑目录(`PR`): `~/.cache/unisacc/prerun/v0.1.0-a11d28fc0a` (`windows=245`, `batches=521`, 暖布局 `tree-6cve4rtr`)
- 执行顺序关键设计(**防 107 换布局冷启动陷阱**):
  - **先在原 `F = a11d28fc0a` 暖布局 `tree-6cve4rtr` 下单跑 `tsv-lint-root`(`strict-W9923-tsv-lint-root.tsv`)**,直接命中已预热 kit 与材料;
  - **再落路径 B-t 四文件补丁(仅改 `tools/check-split-parents.cx`、`tools/check-netregen-expected-fail.cx` 迷你夹具仓、`tools/check-xunit-fnptr.cx:97` 版本行前缀与 `tests/env-semantics.tsv:22-29` `0.0.110 -> 0.1.4`)并同步 `frozen-root`**,单跑 `strict-W9988-check-split-parents.tsv`、`strict-W9989-check-netregen-expected-fail.tsv`、`strict-W9990-env-semantics.tsv`、`strict-W9991-xunit-fnptr.tsv`(折叠 `SUPERSEDED=5`, `FAIL=0`)。

---

## 1. 分步时间轴总表(`t0 -> t30`)

| 时段 | 步骤 | 责任人 | 预计分钟 | 核心动作与验收判据 | 红时第一动作 |
|---|---|---|---|---|---|
| `t0..t1` | **S1: prerun DONE 与 `done.*` 计数核** | `agy-flow` + `agy-h` | 1.0 | 核 `supervise.log` 末行 `PRERUN_RUN`、`done.W*` 满 245 窗(`W0000..W0244`)、扫描全部 `PRERUN_KEEP_GOING`/`PRERUN_STOP` 行,确认无第 4 项非 PASS 批 | 若有未完成窗或额外非 PASS 批,立即报原行由 `cc` 裁定,不进 S2 |
| `t1..t3` | **S2: 原暖布局单跑 `tsv-lint-root` (`strict-W9923`)** | `agy-flow` (`agyneon` 盯 `load1<2.0`) | 2.0 | 确认 `load1 < 2.0`,在 `$PR/frozen-root`(`HEAD=F`, 复用暖布局 `tree-6cve4rtr`)外层 `timeout 75s` 单跑 `tsv-lint-root` 产出 `strict-W9923-tsv-lint-root.tsv`,核 `status=PASS rc=0` | 若 `load1 >= 2.0` 先等降载;若仍 `UNKNOWN`,最多低负载重跑 1 次(`strict-W9923b`),仍不过停手报 `cc` |
| `t3..t6` | **S3: 路径 B 补丁私有自检与落车 (`F -> B`)** | `ompgpt` + `agyneon` + `cdx` + `agy-flow` | 3.0 | `ompgpt` 低负载私有核 `d8305059` 两批绿后,在 `$W010` `git apply` 补丁、经 `devsvc git commit` 提交为 `B` 并推 `origin v0.1.0`;`cdx` 核 `SOURCES_SID=37ff46cd...` 与 21 闭包零变化;同步两文件与 `B` 到 `$PR/frozen-root` | 若 `git apply`、`precommit` 或 `SOURCES_SID` 不等,立即停手回退工作区报 `cc`,绝不强提 |
| `t6..t10` | **S4: 路径 B 两批单跑补 `strict-W9988` / `W9989`** | `agy-flow` (`agyneon` 盯 `load1<2.0`) | 4.0 | 在 `$PR/frozen-root`(`HEAD=B`)、`load1 < 2.0` 下串行单跑 `check-split-parents`(`strict-W9988-check-split-parents.tsv`, `PASS 19/0`)与 `check-netregen-expected-fail`(`strict-W9989-check-netregen-expected-fail.tsv`, `PASS 20/0`),保留原 `strict-W0088.tsv` 与 `strict-W0111.tsv` 不动 | 任一批非 `PASS` 立即停手读 `/tmp/qualify-log-*/*.out` 报原行,不跑第二遍 |
| `t10..t13` | **S5: `merge` + `archive` + 证据复制 + 隐私清洗** | `agy-flow` + `ompgpt` | 3.0 | 跑 `prerun-drive.cx -- merge` 与 `archive --version 0.1.0`;核 `SUMMARY batches=521 PASS=520 FAIL=0 UNKNOWN=1 INVALID=0`(`SUPERSEDED=3`: `W0023.z06`、`W0088`、`W0111`);复制 `list-cap-margin-010.md -> release/evidence/0.1.0/list-cap-margin.md` 并写 `expected-unmeasured.tsv`;写 `ratchets.txt`;清洗家路径核 `EVIDENCE_HOME_PATHS=0` | 若 `FAIL != 0`、`UNKNOWN != 1` 或 `EVIDENCE_HOME_PATHS != 0`,停在原地核 `strict-final.tsv` 与缺失窗,不进 `pack` |
| `t13..t17` | **S6: M2 `--pack` 与封存复核 + 说明回填** | `agy-flow` + `ccogweco` + `ompgpt` | 4.0 | 在 `$W010` 跑 `release-run.cx -- --pack` 产出 `/tmp/unisacc-0.1.0-assets`(7 件)与 `release/candidate.tsv`(`commit=B` 或 `F`,见 S7 合同);`ccogweco`(或接手人)回填 `SID5`/`pkg.bin` sha 并落 `release/notes-0.1.0.md`、`release/evidence/0.1.0/evidence-rows.tsv`、`release/rollback.txt` | 若 `--pack` 拒绝或 6 切片/pkg.bin sha 与链目录不等,立即报原行停手 |
| `t17..t22` | **S7: M2 `release-exec --warm` 与 `--dry`** | `agy-flow` | 5.0 | 先跑 `release-exec.cx -- --warm release/candidate.tsv --layout /tmp/unisacc-0.1.0-assets --out /tmp/v010-exec`(`WARM RESULT READY targets=5`),复制并清洗 `warm-*.tsv` 入 `release/evidence/0.1.0/`;再跑 `release-exec.cx -- --execute ... --dry --post-chain-merge "<REASON>"` 核 `DRY_GATES_OK steps=7` | 若 `GATE 1 STRICT` 或 `STRICT_UNKNOWN` 拒绝,逐字核 `list-cap-margin.md` 的 `RAW` 行与 `expected-unmeasured.tsv`;若 `WARM` 拒绝则重跑 `--warm` |
| `t22..t30` | **S8: 合入 `main` 与 `Step 1 X` 起跑** | `cc` + `agy-flow` | 8.0 | 将 `v0.1.0`(含证据与说明预置)快进/合入 `main`(或由 `cc` 检出 `main`),在 `main` 跑 `release-exec.cx -- --execute release/candidate.tsv --layout /tmp/unisacc-0.1.0-assets --out /tmp/v010-exec --post-chain-merge "<REASON>"` 完成 `Step 1 X -> Step 2 draft -> Check 5 -> Step 3 CI` | 若 `MAIN_ADVANCED` 或 `F_NOT_CHAIN_REV` 拒绝,核 `--post-chain-merge` 理由与 `origin/main` 祖先关系,不删 tag/draft |

---

## 2. 逐步可执行命令序(`env -i` 干净环境)

### S1 (`t0..t1`): `prerun` 完成与窗口完整性核对
- **责任人**: `agy-flow` (主跑)、`agy-h` (`prerun-watch v3` 终采确认)
- **命令**:
  ```sh
  set -e
  PR="$HOME/.cache/unisacc/prerun/v0.1.0-a11d28fc0a"
  ps -p "$(cat "$PR/prerun-drive.pid")" || true
  tail -n 15 "$PR/supervise.log"
  ls "$PR"/done.W* | grep -E 'done\.W[0-9]{4}$' | wc -l
  grep -E "PRERUN_KEEP_GOING|PRERUN_STOP|PRERUN_INCOMPLETE|PRERUN_PRODUCT_RED_STOP|PRERUN_RUN" "$PR/supervise.log"
  ```
- **预期**: `done.W[0-9]{4}` 为 `245`;`PRERUN_KEEP_GOING` 仅 `W0023.z06`(`tsv-lint-root`)、`W0088`(`check-split-parents`)、`W0111`(`check-netregen-expected-fail`)(若 `W0167..W0244` 有新增贴限 `UNKNOWN`,一并列入 S2 同布局低负载单跑)。

### S2 (`t1..t3`): 原暖布局 `tree-6cve4rtr` 下低负载(`load1 < 2.0`)单跑 `tsv-lint-root`
- **责任人**: `agy-flow` (执行)、`agyneon` (核 `load1 < 2.0`)
- **命令**:
  ```sh
  set -e
  export PATH="/usr/bin:/bin:$HOME/.local/bin"
  CDIR="$HOME/.cache/unisacc/chains/bg-b1/a11d28fc0a61939ee71345d732a7dcf205b261f1"
  APE="$CDIR/ape-final/unisacc-next.com"
  PR="$HOME/.cache/unisacc/prerun/v0.1.0-a11d28fc0a"

  while ! awk 'BEGIN{exit !('"$(awk '{print $1}' /proc/loadavg)"' < 2.0)}'; do sleep 5; done
  cd "$PR/frozen-root"
  env -i HOME="$HOME" PATH="/usr/bin:/bin:$HOME/.local/bin" TMPDIR="/tmp" \
    UNISACC_BOOTER_CHAIN_OUT="$CDIR" \
    timeout -k 5s 75s sh ./unisacc.com tools/dvqualify.cx -- . \
      --candidate "$APE" --ids tsv-lint-root \
      --out "$PR/strict-W9923-tsv-lint-root.tsv" --jobs 1 --segment-batches 1
  cat "$PR/strict-W9923-tsv-lint-root.tsv"
  grep -q "^tsv-lint-root	tool-selfcheck	PASS	0	b7cae17b1ae557a4907247e3168db56925504819f98d15f1b75a2da2de8fa97f	" "$PR/strict-W9923-tsv-lint-root.tsv"
  ```
- **红时第一动作**: 若 `load1` 突升致 `UNKNOWN`,待 `load1 < 1.5` 重跑一次输出到 `strict-W9923b-tsv-lint-root.tsv`;连续两次不 `PASS` 立即停手报 `cc`。

### S3 (`t3..t6`): 路径 B 补丁(`d8305059`)落车 `v0.1.0` 与 `frozen-root` 同步
- **责任人**: `ompgpt` (私有低负载核 19/0 与 20/0)、`agyneon` / `agy-flow` (落车提交 `B`)、`cdx` (核 `SOURCES_SID` 与 21 闭包)
- **命令**:
  ```sh
  set -e
  export PATH="/usr/bin:/bin:$HOME/.local/bin"
  W010="$HOME/repos/unisacc/.claude/worktrees/v0.1.0"
  PR="$HOME/.cache/unisacc/prerun/v0.1.0-a11d28fc0a"
  PATCH="$HOME/.local/share/unisacc/research/ompgpt/check-split-parents-vseq-fixture.patch"

  cd "$W010"
  git apply --check "$PATCH"
  git apply "$PATCH"
  sh ./unisacc.com tools/devsvc.cx -- git commit \
    -m "R39 路径 B-t: check-split-parents 与 check-netregen-expected-fail 夹具补 version-seq,check-xunit-fnptr 支持 0.1.0 版本行,env-semantics 8 项到期线顺延至 0.1.4" \
    --whole tools/check-split-parents.cx tools/check-netregen-expected-fail.cx tools/check-xunit-fnptr.cx tests/env-semantics.tsv
  B_REV=$(git rev-parse HEAD)
  git push origin v0.1.0

  # 核对 F..B 仅含该 4 个文件且 SOURCES_SID 不变
  git diff --name-only a11d28fc0a61939ee71345d732a7dcf205b261f1 "$B_REV"
  sh ./unisacc.com tools/chain-step.cx -- sources-sid .

  # 同步到 frozen-root 供 S4 单跑
  cp "$W010/tools/check-split-parents.cx" "$W010/tools/check-netregen-expected-fail.cx" "$W010/tools/check-xunit-fnptr.cx" "$PR/frozen-root/tools/"
  cp "$W010/tests/env-semantics.tsv" "$PR/frozen-root/tests/"
  printf '%s\n' "$B_REV" > "$PR/frozen-root/.git/HEAD"
  ```
- **红时第一动作**: 若 `git diff --name-only F B` 出现除上述 4 个文件外的任何文件,或 `SOURCES_SID != 37ff46cd02171c8ad2d3ba04dd084c296f5290a992077f8935e8b853e67d3238`,立即停手报 `cc`。

### S4 (`t6..t10`): 低负载(`load1 < 2.0`)串行单跑 `W0088`、`W0111`、`W0212` 与 `W0217` (`strict-W9988`..`W9991`)
- **责任人**: `agy-flow` (执行)、`agyneon` (盯 `load1 < 2.0`)
- **命令**:
  ```sh
  set -e
  export PATH="/usr/bin:/bin:$HOME/.local/bin"
  CDIR="$HOME/.cache/unisacc/chains/bg-b1/a11d28fc0a61939ee71345d732a7dcf205b261f1"
  APE="$CDIR/ape-final/unisacc-next.com"
  PR="$HOME/.cache/unisacc/prerun/v0.1.0-a11d28fc0a"

  cd "$PR/frozen-root"
  while ! awk 'BEGIN{exit !('"$(awk '{print $1}' /proc/loadavg)"' < 2.0)}'; do sleep 5; done
  env -i HOME="$HOME" PATH="/usr/bin:/bin:$HOME/.local/bin" TMPDIR="/tmp" \
    UNISACC_BOOTER_CHAIN_OUT="$CDIR" \
    timeout -k 5s 75s sh ./unisacc.com tools/dvqualify.cx -- . \
      --candidate "$APE" --ids check-split-parents \
      --out "$PR/strict-W9988-check-split-parents.tsv" --jobs 1 --segment-batches 1
  cat "$PR/strict-W9988-check-split-parents.tsv"
  grep -q "^check-split-parents	tool-selfcheck	PASS	0	b7cae17b1ae557a4907247e3168db56925504819f98d15f1b75a2da2de8fa97f	" "$PR/strict-W9988-check-split-parents.tsv"

  while ! awk 'BEGIN{exit !('"$(awk '{print $1}' /proc/loadavg)"' < 2.0)}'; do sleep 5; done
  env -i HOME="$HOME" PATH="/usr/bin:/bin:$HOME/.local/bin" TMPDIR="/tmp" \
    UNISACC_BOOTER_CHAIN_OUT="$CDIR" \
    timeout -k 5s 75s sh ./unisacc.com tools/dvqualify.cx -- . \
      --candidate "$APE" --ids check-netregen-expected-fail \
      --out "$PR/strict-W9989-check-netregen-expected-fail.tsv" --jobs 1 --segment-batches 1
  cat "$PR/strict-W9989-check-netregen-expected-fail.tsv"
  grep -q "^check-netregen-expected-fail	tool-selfcheck	PASS	0	b7cae17b1ae557a4907247e3168db56925504819f98d15f1b75a2da2de8fa97f	" "$PR/strict-W9989-check-netregen-expected-fail.tsv"

  while ! awk 'BEGIN{exit !('"$(awk '{print $1}' /proc/loadavg)"' < 2.0)}'; do sleep 5; done
  env -i HOME="$HOME" PATH="/usr/bin:/bin:$HOME/.local/bin" TMPDIR="/tmp" \
    UNISACC_BOOTER_CHAIN_OUT="$CDIR" \
    timeout -k 5s 75s sh ./unisacc.com tools/dvqualify.cx -- . \
      --candidate "$APE" --ids env-semantics \
      --out "$PR/strict-W9990-env-semantics.tsv" --jobs 1 --segment-batches 1
  cat "$PR/strict-W9990-env-semantics.tsv"
  grep -q "^env-semantics	product-qualification	PASS	0	b7cae17b1ae557a4907247e3168db56925504819f98d15f1b75a2da2de8fa97f	" "$PR/strict-W9990-env-semantics.tsv"

  while ! awk 'BEGIN{exit !('"$(awk '{print $1}' /proc/loadavg)"' < 2.0)}'; do sleep 5; done
  env -i HOME="$HOME" PATH="/usr/bin:/bin:$HOME/.local/bin" TMPDIR="/tmp" \
    UNISACC_BOOTER_CHAIN_OUT="$CDIR" \
    timeout -k 5s 75s sh ./unisacc.com tools/dvqualify.cx -- . \
      --candidate "$APE" --ids xunit-fnptr \
      --out "$PR/strict-W9991-xunit-fnptr.tsv" --jobs 1 --segment-batches 1
  cat "$PR/strict-W9991-xunit-fnptr.tsv"
  grep -q "^xunit-fnptr	product-qualification	PASS	0	b7cae17b1ae557a4907247e3168db56925504819f98d15f1b75a2da2de8fa97f	" "$PR/strict-W9991-xunit-fnptr.tsv"
  ```
- **红时第一动作**: 任一批非 `PASS` 立即停手读 `/tmp/qualify-log-*/*.out` 报 `cc`,不覆盖原 `strict-W0088.tsv`、`strict-W0111.tsv`、`strict-W0212.tsv` 与 `strict-W0217.tsv`。

### S5 (`t10..t13`): `merge` + `archive` + `expected-unmeasured.tsv` + `ratchets.txt` + 家路径清洗
- **责任人**: `agy-flow` + `ompgpt`
- **命令**:
  ```sh
  set -e
  export PATH="/usr/bin:/bin:$HOME/.local/bin"
  W010="$HOME/repos/unisacc/.claude/worktrees/v0.1.0"
  PR="$HOME/.cache/unisacc/prerun/v0.1.0-a11d28fc0a"

  cd "$W010"
  sh ./unisacc.com tools/prerun-drive.cx -- merge --in-dir "$PR" --out "$PR" --version 0.1.0
  tail -n 5 "$PR/strict-final.tsv"
  rm -f "$W010/release/evidence/0.1.0/prerun-windows.tsv" "$W010/release/evidence/0.1.0/prerun-supervise.log"
  sh ./unisacc.com tools/prerun-drive.cx -- archive --in-dir "$PR" --repo "$W010" --version 0.1.0

  cp "$PR/strict-final.tsv" "$W010/release/evidence/0.1.0/strict.tsv"
  cp "$PR/strict-final.tsv" "$W010/release/evidence/v0.1.0-strict-2026-11-25.tsv"
  cp "$HOME/.local/share/unisacc/research/ompgpt/list-cap-margin-010.md" "$W010/release/evidence/0.1.0/list-cap-margin.md"
  printf 'batch\treason\tevidence\nprecommit-list-cap\twindow-timeout\tlist-cap-margin.md\n' > "$W010/release/evidence/0.1.0/expected-unmeasured.tsv"

  {
    printf '# freeze=a11d28fc0a61939ee71345d732a7dcf205b261f1\n'
    sh ./unisacc.com tools/ratchets.cx
  } > "$W010/release/evidence/0.1.0/ratchets.txt"
  tail -n 3 "$W010/release/evidence/0.1.0/ratchets.txt"

  # 清洗 strict.tsv 与 v0.1.0-strict-2026-11-25.tsv 中的家目录路径(layout=/tmp/... 不含家路径,若有则替换为 ~/)
  grep -rn '[/]home/\|[/]Users/' "$W010/release/evidence/0.1.0/" "$W010/release/evidence/v0.1.0-strict-2026-11-25.tsv" || echo "EVIDENCE_HOME_PATHS=0"
  ```
- **预期**: `PRERUN_MERGE batches=521 PASS=520 FAIL=0 UNKNOWN=1 SUPERSEDED=3`;`RATCHETS ok=15 fail=0`;`EVIDENCE_HOME_PATHS=0`。

### S6 (`t13..t17`): M2 `release-run --pack` 与发布说明/证据表落位
- **责任人**: `agy-flow` + `ccogweco`(或按 `handoff-010-notes.md` 接手回填)
- **命令**:
  ```sh
  set -e
  export PATH="/usr/bin:/bin:$HOME/.local/bin"
  W010="$HOME/repos/unisacc/.claude/worktrees/v0.1.0"
  CDIR="$HOME/.cache/unisacc/chains/bg-b1/a11d28fc0a61939ee71345d732a7dcf205b261f1"
  APE="$CDIR/ape-final/unisacc-next.com"
  LAYOUT="/tmp/unisacc-0.1.0-assets"

  rm -rf "$LAYOUT"
  mkdir -p "$LAYOUT"
  cd "$W010"
  # 注:release-run --pack 将 git rev-parse HEAD 写入 release/candidate.tsv 的 commit 列;
  # 随即把 candidate.tsv 的 commit 列对回链起提交 F=a11d28fc0a61939ee71345d732a7dcf205b261f1 (使 Step 1 X 由 --post-chain-merge 见证 F->B)
  sh ./unisacc.com tools/release-run.cx -- --pack \
    --version 0.1.0 --tag v0.1.0 \
    --ape "$APE" \
    --pkg "$CDIR/p3/compiler.p4.pkg" \
    --win-arm64 "$CDIR/gen3/win-arm64/unisacc" \
    --lnx-x86_64 "$CDIR/gen3/lnx-x86_64/unisacc" \
    --lnx-arm64 "$CDIR/gen3/lnx-arm64/unisacc" \
    --osx-x86_64 "$CDIR/gen3/osx-x86_64/unisacc" \
    --osx-arm64 "$CDIR/gen3/osx-arm64/unisacc" \
    --strict "$W010/release/evidence/v0.1.0-strict-2026-11-25.tsv" \
    --ratchets "$W010/release/evidence/0.1.0/ratchets.txt" \
    --layout "$LAYOUT" \
    --candidate "$W010/release/candidate.tsv"
  cat "$W010/release/candidate.tsv"
  ```
- **红时第一动作**: 若 `--pack` 报错或 `SHA256SUMS` 不含 5 项资产,立即停手核 `$LAYOUT` 与 `release-run.cx` 输出。

### S7 (`t17..t22`): M2 `release-exec --warm` 与 `--dry` 全门预演
- **责任人**: `agy-flow`
- **命令**:
  ```sh
  set -e
  export PATH="/usr/bin:/bin:$HOME/.local/bin"
  W010="$HOME/repos/unisacc/.claude/worktrees/v0.1.0"
  LAYOUT="/tmp/unisacc-0.1.0-assets"
  EOUT="/tmp/v010-exec"
  REASON="R39 Path B tools-only: check-split-parents and check-netregen-expected-fail mini-repo fixtures add build/version-seq.tsv; SOURCES_SID=37ff46cd02171c8ad2d3ba04dd084c296f5290a992077f8935e8b853e67d3238 unchanged"

  rm -rf "$EOUT"
  mkdir -p "$EOUT"
  cd "$W010"
  sh ./unisacc.com tools/release-exec.cx -- --warm release/candidate.tsv --layout "$LAYOUT" --out "$EOUT"
  sh ./unisacc.com tools/release-exec.cx -- --execute release/candidate.tsv \
    --layout "$LAYOUT" --out "$EOUT" \
    --strict "$W010/release/evidence/v0.1.0-strict-2026-11-25.tsv" \
    --post-chain-merge "$REASON" --dry
  ```
- **预期**: `WARM RESULT READY targets=5`;`GATE 1 STRICT PASS`(`rows=524 expected_fail=0 superseded=3`);`RELEASE_EXEC RESULT DRY_GATES_OK steps=7`。

### S8 (`t22..t30`): 合入 `main`、重跑 `--warm` 与启动 `Step 1 X -> Step 3 CI`
- **责任人**: `cc` (合入 `main` 与放行 `X`)、`agy-flow` (执行 `release-exec --warm` 与 `--execute`)
- **命令**:
  ```sh
  set -e
  export PATH="/usr/bin:/bin:$HOME/.local/bin"
  cd "$HOME/repos/unisacc"
  # 车树合入 main 后(cwd Changed to main root),重跑一次 --warm 产出绑定 main cwd 的 warm-<key>.tsv 并清洗入 release/evidence/0.1.0/
  sh ./unisacc.com tools/release-exec.cx -- --warm release/candidate.tsv --layout /tmp/unisacc-0.1.0-assets --out /tmp/v010-exec-main
  sh ./unisacc.com tools/release-exec.cx -- --execute release/candidate.tsv \
    --layout /tmp/unisacc-0.1.0-assets --out /tmp/v010-exec-main \
    --strict "$HOME/repos/unisacc/release/evidence/v0.1.0-strict-2026-11-25.tsv" \
    --post-chain-merge "$REASON"
  ```
- **红时第一动作**: 任一 `GATE` 报 `FAIL` 或 `REFUSE`,立即保存 `/tmp/v010-exec-main` 日志与原行报 `cc`,绝不删 tag 或草稿 Release。
