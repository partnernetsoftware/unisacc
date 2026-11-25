# v0.1.3 起链与发布总计划 (只读实证与逐门推演)

- 编制窗口: `agy-flow` | 日期: `2026-11-26`
- 快照基点:
  - `origin/main`: `772b6e9eb` (已落批次容量 4096 `fd3994e9a` 与互斥安全子集 `772b6e9eb`)
  - `v0.1.3` 版本树: `43b11e5ce` (`C3-S1b-3`, 领先 `origin/main` 34 笔, 落后 `origin/main` 1 笔 `772b6e9eb`)
  - `tape/m0` 工作树: `e31159454` (已合并 `fd3994e9a`)
  - 宿主机可用空间: `53 GiB` (`80%` 已用, `>= 20G` 绿线但须防峰值逼近)
- 交付副本: `~/.local/share/unisacc/research/agy-flow/v013-release-plan.md` 与 `~/repos/unisacc/.claude/worktrees/v0.1.3/research/agy-flow/v013-release-plan.md` (全只读推演, 未起任何编译或重门)

---

## 1. 冻结前必须落地的清单 (按依赖顺序)

### 1.1 产品车 `v0.1.3` 必落项 (进入候选 `SOURCES_SID`)

| 顺序 | 项目 | 当前实测状态 | 冻结前必做动作 | 是否挡冻结 |
| --- | --- | --- | --- | --- |
| 1 | `C3 S1b-2` / `S1b-3` + `S2 v2` | 已提交 `v0.1.3` (`4bc212f70`, `ec78ceeb7`, `43b11e5ce`) | `cdx` 表夹具 v2 (`tools/check-proc-own-table.cx` 等) 已在 `v0.1.3` 工作树就位待提交; `S2` 零等待回归需确认 `ec78ceeb7` 已闭合 | 是 |
| 2 | `C3 S3` / `S5` 收口 | `v013-freeze-readiness.tsv` 标 `S3` 缺 25 个规格键与三类变异、`S5` 有 4 处绕过待退修 | 按主调度 03:30 裁剪线决定: 达标则合入, 逾期则收口/顺延 `v0.1.4` 并在发布说明如实披露 | 视范围裁定 |
| 3 | `A3-4 v4` (`nn` 外部优先快照与缓存防线) | 私有补丁就位 `~/.local/share/unisacc/research/agy-h/a34-v4.patch` (`sha256=5b406dadc2f6114eb1d3f86e058839fdf115890a902a3f044e8b6c4cf2b07bb9`, 15 文件含 `cx/driver/{nnext,runsplit}.cx`, `cx/sys/{fs,posix}.cx`, `src/{drive,drivestream,packmodel,runcache}.cx`) | 通过 `cdx` 独立复核后合入 `v0.1.3`, 同步 `tests/core-interface.tsv` 与 `tests/batch-product-inputs.tsv`; 核对 `a_files_cap` 余量 | 是 (若留车) |
| 4 | `tape/m0` 落 `main` / 并入 `v0.1.3` 范围裁定 | `tape/m0` HEAD `e31159454` (40 码 + M1 8 冻结槽 `mask=383`); `ompgpt` 正式速度窗已实测 16 对象 `94.0x`、64 对象 `114.3x`、102 对象未完成, **15 倍门未达** | 由 `1:cc` 明确裁定: (a) 仅以 M1 功能/工具/容器子集 (`tapepack`、`tapemode-run`、16 对象等价批) 随 `v0.1.3` 落库并在发布说明如实写明「15 倍速度门未达, 待 T1.6」; 或 (b) 不进 `v0.1.3` 候选, 顺延 `v0.1.4` | 须先裁定范围 |
| 5 | `v0.1.3` 合并最新 `origin/main` | 当前落后 `origin/main` 1 笔 (`772b6e9eb` 互斥安全子集) | 冻结前做最后一次 `merge origin/main`, 确保 `HEAD..origin/main == 0` | 是 |
| 6 | 全量派生物、接口表与防红停预检同步 (必须在 `C0b` 冻结行之前!) | 尚未在终态树执行 | 依次执行: `layer-api --bump cx.os,ext.sysx` (及受触包)、`iface-hash --write`、`tables-gen`、`st-gap`、`chain-phases`、`libc-decls`、`cx0-three-source`、`product-map`、`batch-packages-gen` (静默单跑)、`batch-domains`、`daily-compile-smoke --write`、`lane-inputs-gen --write`、核 `CGAC_GENERATORS` 与 `tests/core-interface.tsv`、`tests/batch-product-inputs.tsv` | 是 (阻断 `v0.1.2` 同款红停) |

### 1.2 工具线序列在冻结前是否必进 `main` 的裁定

| 工具线项 | 现状与 SHA-256 / 适用性 | 冻结前建议 | 理由 |
| --- | --- | --- | --- |
| 互斥安全子集 (`commit-gate-safe-subset.patch`) | **已落 `main` (`772b6e9eb`)** | 已完成 | `v0.1.3` 合入 `origin/main` 即获得 |
| 互斥完整版 v5 (`commit-gate-mutex.patch`) | `sha256=611b08ac...`, `gatekeeper` 复核中 | **不挡冻结** (属 `v0.1.4` 工具线) | 不触 `SID`, 冻结前若过审可顺带落 `main`, 不过审不拖延发车 |
| `0001-chain-ready-launch-env.patch` | `sha256=e8c210c120773dd4999eaeff8d9ef177c26c849a6357ae8eaea4373f9558cc40`, `MAIN_OK` / `V013_OK`, `cdx` 首轮退修中 | **有则更好, 不挡冻结** | 手工按规范 `env -i` + 显式 `UNISACC_IDENT_HELPER` + 分步预热 `derive --prepare-native` 与单跑 `batch-packages-gen` 100% 等效 |
| `0002-gate23-pik1.patch` | `sha256=a3cb9f7fad798fbb09c65261ad2c0bc8bd442db2fa1fa4ef3a5976fcd296c646`, `MAIN_OK` / `V013_OK` (需叠在 `0001` 上), `cdx` 首轮退修 `PIK1`/`SID` 同源 | **不挡冻结** | 即使门 23 改 `PIK1`, `release-exec gate_main` 仍需祖先关系或 `post-chain-merge`; 未落时沿用 `v0.1.2` 已验证的「冻结期人工冻 `main`」规则即可 |
| `0003-freeze-cut.patch` | `sha256=0e015c8412430c9a55179ddbf1d732e7ea7602408ac1a03904e3a08a461b44a2`, **`MAIN_FAIL` / `V013_FAIL`** (共享表冲突) + 代码漏改 `candidate.tsv` 列 2/6/7 + `cdx` 阻断 | **明确不进 `v0.1.3` 冻结前置** | 见第 2 节代码实证, 必须延后 `v0.1.4` 修复 |
| `prerun-rebase.patch` (`--rebase-rev`) | 实核 `~/.local/share/unisacc/research/agyneon/prerun-rebase.patch` 为 **0 字节空文件** (`sha256=e3b0c442...`), `tools/prerun-drive.cx` 中无此实现 | **当前不可用, 不挡冻结** | 详见第 5 节 |

---

## 2. 编号提交决策: 手工两笔隔离提交 (明确不用 `freeze-cut`)

### 2.1 代码实证: 为何 `v0.1.3` 不可用当前 `0003-freeze-cut.patch`

只读核查 `research/agy-h/toolline-series/0003-freeze-cut.patch:773-813` (`fc_update_candidate_version`) 证实三处硬阻塞:

1. **`release/candidate.tsv` 漏写 3 列 (核心缺陷)**: `fc_update_candidate_version` 仅查找并替换列 4 (`version`), **完全不修改列 2 (`tag`)、列 6 (`workflow_ref`) 与列 7 (`ci_run_id`)**。在当前 `v0.1.3` 树 (`tag=v0.1.2`, `version=0.1.2`, `workflow_ref=v0.1.2`, `ci_run_id=45638397154`) 上运行将残留旧版 `v0.1.2` 标签、旧 `workflow_ref` 与旧 `ci_run_id=45638397154`!
2. **补丁在现行树无法直接应用**: `git apply --check` 在 `origin/main (772b6e9eb)` 与 `v0.1.3 (43b11e5ce)` 上均因 `tests/batches.tsv:2042` 与 `tests/coverage.tsv:1659` 冲突报 FAIL。
3. **复核未放行**: `v013-freeze-readiness.tsv:12` 记录其首轮复核仍存在初始化与失败续作退修项。

### 2.2 `v0.1.3` 手工编号与冻结标准操作步骤 (`C0a` + `C0b` 两笔隔离提交)

在全部产品改动、合入 `origin/main` 及第 1.1 节全部派生物同步提交落地之后, 按两笔紧接提交执行:

1. **第一笔 `C0a` (编号与 `table-index` 同步提交, 0 个产品源文件)**:
   - `build/version-seq.tsv`: 核对第 67 槽 `0.1.2` 已为 `已公开`, 追加第 68 槽:
     `68<TAB>0.1.3<TAB><定稿主题><TAB>进行中<TAB>编排者 2026-11-26 立项 0.1.3;前置 0.1.2;计划 plans/v0.1.3.md`
   - `release/candidate.tsv`: 同步修改数据行全部 4 个版本槽位列:
     - 列 2 `tag`: `v0.1.2` -> `v0.1.3`
     - 列 4 `version`: `0.1.2` -> `0.1.3`
     - 列 6 `workflow_ref`: `v0.1.2` -> `v0.1.3`
     - 列 7 `ci_run_id`: `45638397154` -> `0` (清空旧 run 编号, 待 `X` 与 CI 后由 `release-exec` 填入真实 run)
   - `plans/v0.1.3.md`: 按 `verseq-gen` R1/R2/R5 与 `19-chain-preflight` 机械检查写入首行 `# 0.1.3 计划:<定稿主题>`、`planned_units=<N>`、`a_class=<唯一类标签>`、`a_files_cap=<覆盖实改 lane-A 文件数的上限, 如 24>`、`freeze_at=TBD` 与计划自指回滚行。
   - `build/table-index.tsv`: 运行 `sh ./unisacc.com tools/derive.cx -- --write --only table-index` 清零 `v0.1.2` 遗留的 `DIFF build/table-index.tsv line=17` 漂移并随 `C0a` 一并提交。
   - 提交前过门校验: `tools/verseq-gen.cx -- --check` (`fail=0`)、`tools/release-packcheck.cx -- slot . --version 0.1.3` (`VERSION_SLOT OK`)、`tools/derive.cx -- --check --only table-index` (`OK`)、`tools/no-home-path.cx -- --check` (`over=0`)。
2. **第二笔 `C0b` (冻结封口提交, 仅改 `build/freeze.tsv`, 作为起链提交 `F`)**:
   - 核对 `build/freeze.tsv` 中 `0.1.2-FREEZE` 已注释; 追加唯一活跃行:
     `0.1.3-FREEZE<TAB><C0a_40位SHA><TAB>cc<TAB><DEADLINE_EPOCH_S><TAB>src cx ext exec include seed kernel tests release`
   - 保证 `build/tlib-packages.tsv`、`build/layer-api/`、`build/iface-hash.tsv` 最后提交时间不晚于 `C0b` (满足门 23 `FREEZE_TLIB_AFTER_FREEZE` 防线)。

---

## 3. 只读逐门推演: `tools/chain-ready.cx` 全 23 门在当前 `v0.1.3` (`43b11e5ce`) 状态与预期红点

基于只读实查 `tools/chain-ready.cx`、`v0.1.3` 工作树现场与 `v013-numbering-dryrun.md`, 逐门列明当前预期结果与消红动作:

| 门号/名 | 当前 `v0.1.3` (`43b11e5ce`) 预期 | 精确依据 / 预期红点 | 冻结前消红动作 |
| --- | --- | --- | --- |
| `1-rev` | **FAIL (红)** | 工作树当前有 4 个已修改跟踪文件 (`prd.md`, `tests/{batches,coverage,tools-compile-sources}.tsv`) 与未跟踪 `tools/check-proc-own-table.cx` 等; 另需核 `.git/unisacc-deferred-gates.tsv` (292 B) 无本工作树未验 `DEFERRED` 行 | 提交 `cdx` 表夹具、移出或清理 `v0.1.3` 下私有草稿/未跟踪 `.cx`, 核 `git status --short src cx ext exec/facts` 为空 |
| `2a-derive` | **FAIL (红)** | (1) `build/table-index.tsv` 承接 `v0.1.2` 漂移 (`DIFF line=17`); (2) `C3` 与待合 `A3-4` 改动 `cx/sys/*`, `src/*` 后 `layer-api`/`iface-hash`/`product-map`/`tables-gen` 必漂; (3) `batch-packages-gen` 并发下必撞 `30s` 超时 (`rc=142`) | 先 `derive --write --only table-index` + 全量受触派生写入; 起链前先跑 `derive --check --prepare-native`, 静默单跑 `--only batch-packages-gen` (约 `21s`) |
| `2b-ratchets` | **高危待核** | `v0.1.3` 工作树存在未跟踪私有目录 `research/agy-flow/` 与 `research/cdx/` (虽已脱敏但未跟踪 `.cx` 如 `check-tape-tldce-speed.cx` 在私有目录可能被全树棘轮扫到; `dup-census`/`cxtrunc`/`no-home-path` 须在干净冻结头实跑) | 起链前核实未跟踪文件不触发 `cxraw`/`cxtrunc`/`no-home-path`/`reserved-names`, 跑满 `ratchets.cx` (`ok=19 fail=0`) |
| `3a-cxmod-tri` | **可能超时/待测** | 冷材料编译耗时长, `C3`/`A3-4` 触及 `src/` 与 `cx/` | 事前跑 `affected --materials-dump --ids cxmod-tri,cxmod-tri-exports --prepare-materials -j 1` 预热原生执行体 |
| `3b-cx0-extract` | **PASS (绿)** | `v0.1.3` 当前未改 `cx/cx0/` 与 `src/cxmod.c` | 冻结前复核 `CX0_EXTRACT EXACT` |
| `4-p1` | **待测 (不可 `--fast` 跳过)** | 新增/修改的 `.cx` (`check-proc-own*.cx`, `check-nn-external.cx`, `tapepack.cx` 等) 须零新增 `bad symbol name`/`does not export`/`undeclared function` | 正式起链前必跑不带 `--fast` 的完整 `chain-ready` |
| `5-candidate-chain` | **低核/高载易超时** | 干跑显示 2 核 + 高载下 `rc=124`; 须在无限核静默窗口实跑 | 起链前静默单跑两轮确认 `rc=0` 无 `FAIL` |
| `6-affected` | **重跑耗时高** | `C3`+`A3-4` 改 `cx/sys/proc.cx`, `src/drive.cx` 等核心文件, `--since` 命中大批量批次 | 预热 `affected.cx` 与重型材料 (`module-units` 等), 避免 60s 窗超时 |
| `7-recipe` | `UNMEASURED` (设计手工) | 手工核私有根、commit-tree、`UNISACC_IDENT_HELPER`、新 `--out`、5400s | 人工逐项勾选 |
| `8-inflight` | **当前 PASS (绿)** | 须在起链时刻无任何在途 `candidate-chain` 进程 | 起链前 `pgrep -af candidate-chain` 确认空 |
| `9-ledger` | `UNMEASURED` (设计手工) | 冻结起链前记一行工单/账本 (REV、PID、截止、目录) | 人工落行 |
| `10-envhygiene` | **易误红** | 宿主 shell 常带过期 `UNISACC_BOUND_DEADLINE_MONO` 或 `UNISACC_RUN_CACHE` | 严格用 `env -i HOME="$HOME" PATH="/usr/bin:/bin:$HOME/.local/bin" TMPDIR=/tmp DEVSVC_OWNER=cc UNISACC_IDENT_HELPER="$HOME/.cache/unisacc/prep-cache/identity-helper-b9ed85e8f3890fa8.bin"` |
| `11-version-slot` | **FAIL (红)** | 当前 `v0.1.3/release/candidate.tsv` 列 4 仍为 `0.1.2` (`slot-version-differs-from-target 0.1.2 0.1.3`) | 在 `C0a` 同步改列 2/4/6/7 为 `v0.1.3` / `0.1.3` / `v0.1.3` / `0` |
| `12-root-files` | **FAIL (红)** | 实测 `v0.1.3/unisacc.com` 为**符号链接** (`-> ~/repos/unisacc/unisacc.com`), 且 `v0.1.3/unisacc.com.build.json` **缺失** | 起链前 `cp --remove-destination -L ~/repos/unisacc/unisacc.com ~/repos/unisacc/unisacc.com.build.json .claude/worktrees/v0.1.3/` |
| `12p-preflight-entry` | **FAIL (红)** | 同上, `v0.1.3/unisacc.com.build.json` 缺失 (`kind=missing`) | 同门 12 拷贝常规文件副本即绿 |
| `13-snap-cache` | **PASS (绿)** | `C0b` 为新 commit SHA, `snap/` 下无旧快照; 但若升级 Root 须清旧快照 | 若复用过同一 rev 快照或升级 Root 后, 移走 `~/.cache/unisacc/chains/snap/` 旧条目 |
| `14-root-record` | **PASS 带 WARN** | 实测共享根 `unisacc.com.build.json` 仍为 `commit=414a024ea5` (`0.0.105`, `8,842,611 B`), 对应 `release/candidate.tsv` 无 `0.0.105` 行故发 `WARN root-record-stale-fields` | 不阻断起链 (`PASS`), 但建议如条件允许可核是否升级本机 Root 到 `v0.1.2` (`8,992,558 B`, 需注意升级后全工树同步常规副本与清 `snap/`) |
| `15-gen-all` | **高危 (易漏同步或超时)** | `v0.1.2` 曾因 `CGAC_GENERATORS` 未同步在 `W0081` 红停; 冷态/高载下易 `rc=124` | 确认 `total=N` 与 `tools/check-gen-all-check.cx` `CGAC_GENERATORS` 一致, 静默窗口实跑 `GEN_ALL_SUMMARY diff=0 error=0` |
| `16-disk-free` | **PASS (绿, 须监控)** | 实测 `/` 可用 `53G` (`>=20G`, 使用率 `80%`) | 整链+预跑峰值约吃 `15-25G`; 起链前核余量 `>= 40G` (清理已完成旧链临时目录, 绝不动在途会话根) |
| `17-root-public` | **PASS (绿)** | 共享根无 `unisacc.com.candidate.tsv` | 保持非 candidate 安装态 |
| `18-parse2-gen` | **PASS (绿)** | 本地已有 `refs/tags/v0.1.2` (`git tag -l`), 当前 `v0.1.3` 未改 `exec/parse2` 与 `seed/gen.c` | 若 `A3-4`/`tape` 均不碰 `exec/parse2` 与 `seed/gen.c`, 保持 `parse2-inputs-unchanged` |
| `19-chain-preflight` | **FAIL (红)** | (1) `plans/v0.1.3.md` 尚不存在 (`ENOENT`); (2) `C3` (`6` 文件已改) + `A3-4` (`8` 产品文件) 合计手改 lane-A 文件需 `<= a_files_cap`; (3) `chain-to-snap` 与 `sixtarget` 在高载下易超 55s | `C0a` 补齐 `plans/v0.1.3.md` 三机械字段 (`a_files_cap=24` 留足余量); 静默窗口分步核 `chain-preflight.cx --strict` |
| `20-ident-helper` | **FAIL (默认路径缺失)** | 实测 `~/.cache/unisacc/ident-helper/identity-helper-real5` **不存在**; 真实可用文件为 `~/.cache/unisacc/prep-cache/identity-helper-b9ed85e8f3890fa8.bin` (`48696 B`, 含 `.rcpt`) | `env -i` 显式传 `UNISACC_IDENT_HELPER="$HOME/.cache/unisacc/prep-cache/identity-helper-b9ed85e8f3890fa8.bin"` (或拷贝至默认路径前先用 `seed/ident.c` 复核) |
| `21-verseq-gen` | **FAIL (红)** | 当前 `build/version-seq.tsv` 最高槽为 `67 (0.1.2)`, 缺 `68 (0.1.3)` 与 `plans/v0.1.3.md` | `C0a` 补齐并跑 `tools/verseq-gen.cx -- --check` (`fail=0`) |
| `22-premerge-union` | **FAIL / 高危** | 当前 `v0.1.3` 落后 `origin/main` 1 笔 `772b6e9eb`; 且冷态两分片 (`derive` + `check-derive`) 易在并发下超时 | 先合 `origin/main` 清零落后, 再预热原生后分别跑 `--shard derive` 与 `--shard check-derive` |
| `23-freeze-armed` | **FAIL (红)** | (1) `build/freeze.tsv` 当前全为注释行, 无活跃 `0.1.3-FREEZE`; (2) `HEAD` 落后 `origin/main` 1 笔 `772b6e9eb` (`FREEZE_BEHIND_MAIN`); (3) 须用 `DEVSVC_OWNER=cc` | 合 `origin/main` -> 同步全派生物 -> `C0a` -> 最后独立提交 `C0b` (`0.1.3-FREEZE`, `owner=cc`, 留 12h 截止) |

---

## 4. `agy-h` 工具线补丁 (`launch-env` / `gate23` / `freeze-cut`) 逐项核对

| 补丁 | 路径 (均为 `~/repos/unisacc/.claude/worktrees/v0.1.3/research/agy-h/toolline-series/`) | SHA-256 | 验证基点 | 对当前 `main (772b6e9eb)` / `v0.1.3 (43b11e5ce)` `apply --check` | 已知缺口与必须先落 `main` 判定 |
| --- | --- | --- | --- | --- | --- |
| `0001` | `0001-chain-ready-launch-env.patch` (`46115 B`) | `e8c210c120773dd4999eaeff8d9ef177c26c849a6357ae8eaea4373f9558cc40` | `a7ae7a494` | `MAIN_OK` / `V013_OK` | 首轮 `cdx` 复核退修 `launch-env` 合同; **必须先由 `1:cc` 落 `main` 并合入 `v0.1.3`** 才能在 `v0.1.3` 使用 `--prepare`/`--launch-env`, 但**不阻塞 `v0.1.3` 起链** (手工 `env -i` 等效) |
| `0002` | `0002-gate23-pik1.patch` (`20337 B`) | `a3cb9f7fad798fbb09c65261ad2c0bc8bd442db2fa1fa4ef3a5976fcd296c646` | 叠在 `0001` 上 | `MAIN_OK` / `V013_OK` (单测) | 必须严格串行在 `0001` 之后落 `main`; 首轮 `cdx` 退修 `PIK1`/`SID` 同源一致性; 需先落 `main` 并合入 `v0.1.3` 才生效, **不阻塞 `v0.1.3` 起链** |
| `0003` | `0003-freeze-cut.patch` (`51441 B`) | `0e015c8412430c9a55179ddbf1d732e7ea7602408ac1a03904e3a08a461b44a2` | 叠在 `0002` 上 | **`MAIN_FAIL` / `V013_FAIL`** (`tests/batches.tsv:2042`, `tests/coverage.tsv:1659` 冲突) | **未修 `candidate.tsv` 列 2/6/7 漏改 (`fc_update_candidate_version` 仅改列 4)** + 首轮 `cdx` 阻断初始化与失败续作 -> **`v0.1.3` 禁用, 改手工两笔提交** |

注: `research/agy-h/` 根目录下的旧散件 `gate23-pik1.patch` (`19946 B`) 与 `freeze-cut.patch` (`50838 B`) 已被 `toolline-series/0002`、`0003` 取代, 不再使用。

---

## 5. 整链与预跑并发策略及 `--rebase-rev` 可用性核实

1. **`--rebase-rev` 现状 (明确不可用)**:
   - 实查 `tools/prerun-drive.cx` (`main` 与 `v0.1.3`) 无任何 `rebase-rev` / `rebase_rev` 符号。
   - 实查 `~/.local/share/unisacc/research/agyneon/prerun-rebase.patch` 为 **0 字节空文件** (`sha256=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`), `git apply --check` 报 `unrecognized input`。
   - **操作后果**: `v0.1.3` 预跑一旦因测试表缺失 (`daily-compile-smoke`, `lane-inputs`, `CGAC_GENERATORS`) 产生新 `rev` (`C10` 式修补), **不能跨 `rev` 自动复用 219 窗**, 只能走 `v0.1.2` 先例: (a) 严控起链前把所有表先同步绿, 追求 0 红停; (b) 若仅单批纯表补丁 (`SOURCES_SID` 不变), 用同 `rev` 原生续跑 + 精准单批补跑窗 (`W99xx`) 与 `--post-chain-merge` 理由归档。
2. **整链与预跑并发规则 (严防高载超时)**:
   - **整链 (`bg-candidate`) 与预跑 (`prerun-drive`) 严禁并发**: 整链期间暂停一切变异门、预检整表、`--all`、宽并发 `affected` 及其他窗口重编译 (`prd_mempalace.md:43`)。
   - **整链阶段** (~28-35 min): 独享机器, `env -i` 干净环境启动 `bg-candidate`。
   - **预跑阶段** (~55-75 min):
     - 先单独执行 `prerun-drive prepare` + 预热 `W0000 (bpidecl)` (注意 `v0.1.2` 中 `W0000` 首轮冷编 `58046ms rc=124`, 第二轮 `55871ms rc=0`, 务必在低载下先过 `W0000` 与 `opt-frame4-material`);
     - 主体窗口维持显式固定并发 `jobs=8`、`window-lanes=2` (`v0.1.2` 实测 `budget_ms=22000` 稳定过 247 窗), **禁止**同时在其它窗口跑重型编译;
     - 若发生单批 `UNKNOWN` (如 `tsv-lint-root` 撞 55s), **绝不对已有 `strict-W99*.tsv` 执行 `rm -f`** (吸取 `v0.1.2 incident-W9942` 教训!), 始终新开递增窗号 `W99xx` 并在同布局预热后单独补跑。

---

## 6. 预计墙钟拆解: `v0.1.2` (9h) vs `v0.1.3` 预估 vs `fable` (3-4h) 差距账

### 6.1 分相位时间对照表

| 阶段 | `v0.1.2` 实测 (总 ~9.0h) | `v0.1.3` 零红停最优路径 | `v0.1.3` 保守现实路径 (含 1 次小修) | `fable` 3-4h 目标基准 | 主要时间差与压缩杠杆 |
| --- | --- | --- | --- | --- | --- |
| 0. 冻结前清表、派生、`C0a`/`C0b` + `chain-ready` 全门 | ~90 min (含反复补派生与冷超时) | 25 - 35 min | 45 - 60 min | 20 - 30 min | 事前预热 `derive --prepare-native`、单跑 `batch-packages-gen`、一次落齐 `lane-inputs`/`daily-compile-smoke`/`CGAC_GENERATORS`/`table-index` |
| 1. 整链 (`bg-candidate`, 六目标不动点 + APE) | 27.1 min x 2 趟 (`C9` 拆 `C3` 重链) = ~65 min | 28 - 35 min (1 趟) | 30 - 40 min (1 趟, 严禁链后拆大件!) | 27 - 35 min | **最硬杠杆: 杜绝重链!** 冻结前在 `v0.1.3` 必实跑 `processcore-compile`、`devsvc-git-c-operations` 热态、`check-nn-external` |
| 2. 严格预跑 (`prerun-drive`, 527+ 批) | ~260 min (4 次红停: `W0001`/`W0081`/`W0118`/`W0219` + 1 次 `W0016` + 补跑与人工排错) | 55 - 75 min | 100 - 140 min | 45 - 60 min | 无 `short-batch-packing` (`long` 138 窗未压至 78 窗, 差 ~25-35 min); 无 `--rebase-rev` (若遇 1 次纯表红停多耗 ~40-50 min) |
| 3. `release-run --pack` + `release-exec --warm` + 六目标 CI (`candidate-native.yml`) | ~55 min | 30 - 45 min | 40 - 55 min | 30 - 40 min | CI 六格本身 ~18-25 min 属外部固定墙钟; `pack`+`warm` 约 10-15 min |
| 4. `release-exec` 7 步 (`X`/草稿/Y1/公开/读回/Y2) + `release-post` + 解冻/旧资产登记 | ~70 min (含 `post-chain-merge` 触发 `no-home-path` 返工与 `W9942` 事故对账) | 20 - 30 min | 30 - 45 min | 20 - 25 min | 严守冻结期 `main` 不动, 消除 `post-chain-merge` 与路径脱敏返工; 显式核 `build/freeze.tsv` 注释与 `tools/old-product-assets.tsv` |
| **合计 (冻结起链 -> 公开收尾)** | **~9.0 小时** | **2.6 - 3.7 小时** | **4.1 - 5.7 小时** | **3.0 - 4.0 小时** | **零红停即可达标 3-4h; 每多 1 次预跑红停 +0.8~1.5h, 每多 1 次重链 +2.5h** |

### 6.2 距 `fable`「一版 3-4 小时」的核心结构性差距

1. **红停归零是 9h -> 3.5h 的决定性差距 (占 ~5h)**: `v0.1.2` 真正干活的机器墙钟不到 2.5 小时, 其余 ~6.5 小时全部消耗在 4 次预跑红停 (`lane-inputs`、`CGAC_GENERATORS`、`C3` 回归、`daily-compile-smoke`) 与 1 次整链重跑。**在 `v0.1.3` 冻结前把这 4 项变成硬前置命令即可立省 4-5 小时。**
2. **工具线三大提速件尚未入 `main` (合占 ~1.0-1.5h 弹性冗余)**:
   - `--rebase-rev` 补丁为空文件 (0 B) -> 一旦出现单表修补无法保 219 窗;
   - `short-batch-packing.patch` 尚未合入 -> `long` 窗仍为 138 个而非 78 个 (多占 ~25-35 min);
   - `0001-launch-env` + `0002-gate23-pik1` 尚未合入 -> 冻结期仍须人工禁推 `main`, 且起链前须手工冷预热 `derive`。
3. **当前 02:45 尚未冻结**: 在到达冻结切口前的开发收尾时间 (`C3 S3`/`A3-4 v4`/`tape` 范围裁定, 约 2-4h) 属于研发期而非整链发布期, 须与「起链 -> 公开」3-4h 发布时钟分开计量。

---

## 7. 风险排序 (Top 风险、触发条件与前置消解动作)

| 排名 | 风险项 | 严重度 | 触发条件 | 前置消解动作 (起链前必做) |
| --- | --- | --- | --- | --- |
| **R1** | **冻结前未清表/未验 `C3+A3-4` 导致预跑红停甚至重链** (`v0.1.2` 9h 头号元凶, 且无 `--rebase-rev` 兜底) | 极高 | `lane-inputs.tsv` / `daily-compile-smoke.tsv` / `CGAC_GENERATORS` / `core-interface.tsv` 任一漏同步, 或 `C3` 重演 `processcore-compile` / `devsvc-git-c-operations` 退化 | 冻结前在 `v0.1.3` 必实跑: `lane-inputs-gen --check`、`daily-compile-smoke --check`、`gen-all-check --list` 核 `CGAC_GENERATORS`、`check-processcore-compile`、`check-devsvc-git --cases c2` 热态 (`<25s`)、`check-nn-external` |
| **R2** | **`tape/m0` 范围未锁定导致发布口径与门冲突** | 高 | `tape` 15 倍速度门已实测未过 (`94x`/`114x`/102 未完成), 若未先裁定范围就盲目并入, 会在 `v0.1.3` 主题、计划 `planned_units`/`a_files_cap` 与发布说明上返工 | 冻结前由 `1:cc` 书面锁定二选一: (a) 仅入 M1 功能/工具子集并在 `plans/v0.1.3.md` 与 notes 如实写 15 倍门 `UNMET (T1.6)`; 或 (b) 整体顺延 `v0.1.4` |
| **R3** | **误用 `0003-freeze-cut.patch` 导致 `candidate.tsv` 列 2/6/7 残留 `v0.1.2` 与 `ci_run_id=45638397154`** | 高 | 试图把当前 `0003` 硬打入 `main` 并用其执行编号 | 明确禁用 `0003`, 严格按第 2.2 节手工 `C0a` + `C0b` 两笔隔离提交, 核对 `candidate.tsv` 第 2/4/6/7 列全部切齐 `v0.1.3`/`0.1.3`/`v0.1.3`/`0` |
| **R4** | **工作树根文件为软链 + 默认 `UNISACC_IDENT_HELPER` 缺失 + `table-index` 漂移导致 `chain-ready` 连续返工** | 中高 | 直接在当前 `v0.1.3` (`unisacc.com` 为软链、缺 `build.json`、缺默认 helper、`table-index line=17` 漂移) 起链 | 按第 3 节清单一次做全: `cp --remove-destination -L` 根两件、显式设 `prep-cache/identity-helper-b9ed85e8f3890fa8.bin`、`derive --write --only table-index` |
| **R5** | **冻结到公开期间他窗口推 `origin/main` 触发 `MAIN_ADVANCED` / `FREEZE_BEHIND_MAIN`** | 中高 | `0002-gate23-pik1` 与 `release-exec` 分支直发尚未落地, 任何推 `origin/main` 都会迫使发布流走 `post-chain-merge` 甚至引入 `no-home-path` 违例 | `C0b` 落 `build/freeze.tsv` 后同步发全员冻结通知: 除发布写手外禁止推 `origin/main` 直至 `release-post` 完成; 若被迫 `post-chain-merge`, 必先跑 `no-home-path --check` 与 `SID` 等价复核 |
| **R6** | **`/` 磁盘余量 (`53G`, 80% 已用) 在整链+预跑峰值被打穿 `20G` 警戒线或 `8G` 阻断线** | 中 | 整链快照 + 6 目标 g1/g2 + 预跑工作树与产物合计新增 `15-25G`, 若叠加其他窗口残留临时目录易压近 `20G` | 起链前核 `df -h /` 保持 `>= 40G` 可用; 清理已完成旧版本的 `/tmp` 与过期缓存, 严禁删在途链会话根 |
| **R7** | **`W99xx` 补跑证据误删或 `release-post` 漏解冻/漏登记旧资产** | 中 | `v0.1.2` 实发 `incident-W9942` (`rm -f` 误删原件) 与 `release-post` 不自动解冻 `freeze.tsv` / 登记 `old-product-assets.tsv` | 补跑永远递增新窗号 `W99xx`、严禁 `rm -f` 任何 `strict-W*.tsv`; `Y2` 后逐条核 `release-post`、`build/freeze.tsv` 注释 `0.1.3-FREEZE` 与 `tools/old-product-assets.tsv` 写入 `0.1.3` 资产摘要 |
