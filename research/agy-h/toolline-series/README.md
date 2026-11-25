# agy-h 工具线补丁序列 (L2 + L1-S5 落地说明)

- 维护窗口: `agy-h` | 日期: `2026-11-25`
- 验证基线:
  - 全套运行验证基线: `origin/main` (`cf25254fabab1d2a82c8604972083c76f7e7ea0a`) 的私有 `git archive` 树
  - 公开收尾后 `apply` 复核基线: 最新 `origin/main` (`a7ae7a494a84263d65742c3ef920c6d1e630fbc0`, 含 `v0.1.2` `release-post`) 私有 `git archive` 树，顺序 `git apply --check` 与 `git apply` 均零冲突通过 (`APPLY_ON_A7AE7A494=OK`)
- 隔离声明: 所有叠加、编译与测试均在 `/tmp/agy_h_work/` 私有 `git archive` 树完成, 未修改共享 `main` 与 `.claude/worktrees/v0.1.2`, 未提交/推送。

---

## (1) 串行补丁序列与叠加顺序 (`tools/chain-ready.cx` 串行点)

因为 `chain-ready-launch-env` 与 `gate23-pik1` 同改 `tools/chain-ready.cx`, 必须严格按以下三步串行合入:

| 顺序 | 补丁文件 | 涉及文件 | 串行原因 / 说明 |
| --- | --- | --- | --- |
| `1` | [0001-chain-ready-launch-env.patch](file:///home/devali/repos/unisacc/research/agy-h/toolline-series/0001-chain-ready-launch-env.patch) | `tools/chain-ready.cx`, `tools/check-chain-ready.cx` | 先落 `identity-helper` 回执自检、`--prepare`、`--launch-env`、拆分 `2a-derive7` (`table-index`) 与 `batch-packages-gen` 独占互斥 |
| `2` | [0002-gate23-pik1.patch](file:///home/devali/repos/unisacc/research/agy-h/toolline-series/0002-gate23-pik1.patch) | `tools/chain-ready.cx`, `tools/check-chain-ready-freeze.cx` | 基于 `0001` 行号定位对齐后, 将门 `23-freeze-armed` 落后 `origin/main` 判据改为 `PIK1` (`cr_is_pik1_path` + `git diff HEAD...refs/remotes/origin/main`) |
| `3` | [0003-freeze-cut.patch](file:///home/devali/repos/unisacc/research/agy-h/toolline-series/0003-freeze-cut.patch) | `tools/freeze-cut.cx`, `tools/check-freeze-cut.cx`, `tests/tools-compile-sources.tsv`, `tests/batches.tsv`, `tests/coverage.tsv` | 新增 `freeze-cut` 单命令事务化编号 (`C0a`) + 冻结 (`C0b`) + `--check` 与夹具及批次登记 (落地共享表 `batches.tsv` / `coverage.tsv` 时使用 `tools/commit-own.cx --keyed`) |

---

## (2) 每步验收命令与实跑原行输出

以下全部命令均在 `nice -n 19 taskset -c 12-13` 与干净环境 (`-u UNISACC_BOUND_DEADLINE_MONO -u UNISACC_RUN_CACHE -u UNISACC_CACHE_DIR -u UNISACC_FREEZE_NO_FETCH`) 下实跑:

### Step 1 (`0001-chain-ready-launch-env.patch` 之后)

验收命令:
```bash
git apply research/agy-h/toolline-series/0001-chain-ready-launch-env.patch
nice -n 19 taskset -c 12-13 env -u UNISACC_BOUND_DEADLINE_MONO -u UNISACC_RUN_CACHE -u UNISACC_CACHE_DIR -u UNISACC_FREEZE_NO_FETCH sh ./unisacc.com tools/check-chain-ready.cx
nice -n 19 taskset -c 12-13 env -u UNISACC_BOUND_DEADLINE_MONO -u UNISACC_RUN_CACHE -u UNISACC_CACHE_DIR -u UNISACC_FREEZE_NO_FETCH sh ./unisacc.com tools/check-chain-ready-freeze.cx -- --no-mutants
```

实跑原行 (`task-707`):
```text
PASS	prepare-builds-helper-prewarms-and-refreshes-table-index
PASS	launch-env-clean-prints-canonical-env-i
PASS	derive-2a-after-prepare-isolated-pass
PASS	derive-2a-detects-table-index-drift-independently
PASS	ident-helper-strict-verified-pass
PASS	ident-helper-mut-strict-unverified-refused
PASS	ident-helper-mut-tamper-1byte-refused
PASS	ident-helper-mut-seed-changed-refused
CHAIN_READY_CHECK	passed=112	failed=0
---FREEZE_AFTER_0001---
PASS	not-armed-no-row-FAIL
...
PASS	armed-marker-with-owner-PASS
CHAIN_READY_FREEZE	passed=16	failed=0
```

### Step 2 (`0001 -> 0002-gate23-pik1.patch` 之后)

验收命令:
```bash
git apply research/agy-h/toolline-series/0002-gate23-pik1.patch
nice -n 19 taskset -c 12-13 env -u UNISACC_BOUND_DEADLINE_MONO -u UNISACC_RUN_CACHE -u UNISACC_CACHE_DIR -u UNISACC_FREEZE_NO_FETCH sh ./unisacc.com tools/check-chain-ready.cx
nice -n 19 taskset -c 12-13 env -u UNISACC_BOUND_DEADLINE_MONO -u UNISACC_RUN_CACHE -u UNISACC_CACHE_DIR -u UNISACC_FREEZE_NO_FETCH sh ./unisacc.com tools/check-chain-ready-freeze.cx -- --no-mutants
# 全量 11 项变异可在 2 核下用 --mutants-only --mutant-range 0 3 / 3 2 / 5 2 / 7 2 / 9 2 分片跑满, 或多核直接不带参数运行
```

实跑原行 (`task-714`):
```text
CHAIN_READY_CHECK	passed=112	failed=0
---FREEZE_AFTER_0002---
PASS	behind-origin-main-FAIL
PASS	behind-origin-main-toolchain-pik1-FAIL
PASS	behind-origin-main-seed-whitelist-FAIL
PASS	behind-origin-main-research-pass-PASS
PASS	behind-origin-main-plans-tools-pass-PASS
PASS	behind-origin-main-empty-pass-PASS
PASS	behind-origin-main-seed-readme-pass-PASS
PASS	behind-origin-main-exec-exclude-pass-PASS
PASS	no-origin-main-FAIL
PASS	origin-advanced-fetch-behind-FAIL
PASS	origin-advanced-research-fetch-pass-PASS
...
PASS	armed-marker-with-owner-PASS
CHAIN_READY_FREEZE	passed=24	failed=0
```

### Step 3 (`0001 -> 0002 -> 0003-freeze-cut.patch` 全部叠加终态)

验收命令:
```bash
git apply research/agy-h/toolline-series/0003-freeze-cut.patch
nice -n 19 taskset -c 12-13 env -u UNISACC_BOUND_DEADLINE_MONO -u UNISACC_RUN_CACHE -u UNISACC_CACHE_DIR -u UNISACC_FREEZE_NO_FETCH sh ./unisacc.com tools/check-freeze-cut.cx
nice -n 19 taskset -c 12-13 env -u UNISACC_BOUND_DEADLINE_MONO -u UNISACC_RUN_CACHE -u UNISACC_CACHE_DIR -u UNISACC_FREEZE_NO_FETCH sh ./unisacc.com tools/check-chain-ready.cx
nice -n 19 taskset -c 12-13 env -u UNISACC_BOUND_DEADLINE_MONO -u UNISACC_RUN_CACHE -u UNISACC_CACHE_DIR -u UNISACC_FREEZE_NO_FETCH sh ./unisacc.com tools/check-chain-ready-freeze.cx -- --no-mutants
nice -n 19 taskset -c 12-13 env -u UNISACC_BOUND_DEADLINE_MONO -u UNISACC_RUN_CACHE -u UNISACC_CACHE_DIR -u UNISACC_FREEZE_NO_FETCH sh ./unisacc.com tools/check-chain-ready-freeze.cx -- --mutants-only --mutant-range 5 2
nice -n 19 taskset -c 12-13 env -u UNISACC_BOUND_DEADLINE_MONO -u UNISACC_RUN_CACHE -u UNISACC_CACHE_DIR -u UNISACC_FREEZE_NO_FETCH sh ./unisacc.com tools/check-chain-ready-freeze.cx -- --mutants-only --mutant-range 7 2
nice -n 19 taskset -c 12-13 sh ./unisacc.com tools/tsv-lint.cx -- --check
```

实跑原行 (`check-freeze-cut`, `task-724`, `task-730`, `task-735`):
```text
PASS	mut-bad-version-refused
PASS	mut-polluted-env-refused
PASS	mut-owner-unset-refused
PASS	mut-root-symlink-refused
PASS	mut-prev-not-public-refused
PASS	mut-prev-freeze-active-refused
PASS	mut-dirty-product-refused
PASS	mut-check-before-cut-fails
PASS	cut-single-command-pass
PASS	check-after-cut-pass
PASS	cut-git-structure-exact
PASS	mut-check-freeze-mixed-refused
PASS	mut-check-tlib-after-freeze-refused
PASS	mut-check-plan-r1-refused
CHECK_FREEZE_CUT	passed=14	failed=0
---TSV_LINT---
tsv-lint: ok: unresolved=10764 baseline=10764 ceiling=13260 errors=0
---FINAL_CHAIN_READY---
CHAIN_READY_CHECK	passed=112	failed=0
CHAIN_READY_FREEZE	passed=24	failed=0
PASS	mutant-drop-behind-turns-behind-origin-main
PASS	mutant-drop-pik1-classify-turns-behind-origin-main
CHAIN_READY_FREEZE	passed=2	failed=0
PASS	mutant-always-pik1-classify-turns-behind-origin-main-research-pass
PASS	mutant-drop-fetch-turns-origin-advanced-fetch-behind
CHAIN_READY_FREEZE	passed=2	failed=0
```

---

## (3) 新增工具 `register` / `check` 登记与 `SOURCES_SID` 核验

- **是否触 `SID`: 否 (`SID_UNCHANGED=1`)**
  - 本系列全部改动只涉及以下 8 个非产品路径:
    `tests/batches.tsv`, `tests/coverage.tsv`, `tests/tools-compile-sources.tsv`, `tools/chain-ready.cx`, `tools/check-chain-ready-freeze.cx`, `tools/check-chain-ready.cx`, `tools/check-freeze-cut.cx`, `tools/freeze-cut.cx`
  - 用基线 `seed/ident.c` 实测叠加前后源码身份严格一致 (`task-725`):
    ```text
    SID_BEFORE=88e190bdb8bc45df16e06ff9b22d959df08adf6105fb23598edc3c25b50bc8b6
    SID_AFTER=88e190bdb8bc45df16e06ff9b22d959df08adf6105fb23598edc3c25b50bc8b6
    SID_UNCHANGED=1
    ```
- **新增工具与检查登记** (`0003-freeze-cut.patch` 已附带):
  1. `tests/tools-compile-sources.tsv`: 加入 `tools/freeze-cut.cx` (`regular`) 与 `tools/check-freeze-cut.cx` (`regular`)。
  2. `tests/batches.tsv`: 登记批 `freeze-cut` (`group=interfaces`, `source=tools/check-freeze-cut.cx`, `expect=rc0:CHECK_FREEZE_CUT`, `tier=commit`)。
  3. `tests/coverage.tsv`: 登记覆盖项 `interfaces.freeze-cut`。
  4. 合入 `main` 时, 若同时有其它泳道在 `tests/batches.tsv` / `tests/coverage.tsv` 追加新行, 请用 `tools/commit-own.cx --keyed` 隔离各自键值行 (或重跑 `tools/register.cx --tool freeze-cut --source tools/check-freeze-cut.cx --expect rc0:CHECK_FREEZE_CUT --tier commit`)。
  5. `tools/chain-ready.cx`、`tools/check-chain-ready.cx`、`tools/check-chain-ready-freeze.cx` 沿用已有登记批 `chain-ready` 与 `chain-ready-freeze`, 无需新建批次。

---

## (4) `tools/prerun-drive.cx` 串行点单独落地顺序 (`L1` 泳道)

`agyneon` 的 `--rebase-rev` 与 `agy-h` 的短批出 `long` 窗装箱均改 `tools/prerun-drive.cx`, 与后续两件共同构成独立于上述 `L2` 系列的串行合入链:

| 顺序 | 泳道号 | 任务 / 补丁 | 写域交集 | 落地前提与过门命令 |
| --- | --- | --- | --- | --- |
| `1` | `S1` | `agyneon`: `--rebase-rev NEWREV` | `tools/prerun-drive.cx`, `tools/check-prerun-rebase.cx`, `tests/{batches,coverage}.tsv` (`--keyed`) | 先落 `main`, 确立增量重跑入口、依赖闭包判定与 `10%` 抽样兜底; 过门: `check-prerun-rebase.cx` 与 `check-prerun-drive.cx` 全绿 |
| `2` | `S5` | `agy-h`: 短批出 `long` 窗装箱 ([short-batch-packing.patch](file:///home/devali/repos/unisacc/research/agy-h/short-batch-packing.patch)) | `tools/prerun-drive.cx` (`plan` 排窗与 `pd_is_exclusive_heavy_id` 7 域白名单), `tools/check-prerun-drive.cx` (第 18 项 `short-batch-out-of-long`) | 严格排在 `S1` 合入 `main` 之后基于新 `main` rebase (预计 `0.5 - 1.0h`), 验证集守恒与独占批不并窗; 过门: `check-prerun-drive.cx` (`ok=95 failed=0`) + `check-prerun-rebase.cx` 全绿 |
| `3` | `S6` | `agy-flow`: `lane-isolation` 127 行 (`--cpuset`) | `tools/prerun-drive.cx`, `tools/check-prerun-drive.cx` | 紧接 `S5` rebase 后合入 `tools/prerun-drive.cx` |
| `4` | `S7` | `ompgpt`: `dvqualify --out` 唯一性护栏 + `prerun-drive` retry 不 `unlink` `W99xx` | `tools/dvqualify.cx`, `tools/check-dvqualify.cx`, `tools/prerun-drive.cx`, `tools/check-prerun-drive.cx` | 紧接 `S6` 合入 `tools/prerun-drive.cx` (`dvqualify.cx` 侧可与 `S1/S5` 并行先入) |

说明: 本 `L1` (`tools/prerun-drive.cx`) 序列与上方 `L2` (`0001 -> 0002 -> 0003`) 写域完全不相交 (仅 `0003` 与 `S1` 在 `tests/{batches,coverage}.tsv` 共享尾部登记行, 用 `tools/commit-own.cx --keyed` 错峰即可), `L1` 与 `L2` 两条串行链可相互并行落 `main`。
