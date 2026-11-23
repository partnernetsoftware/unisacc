# 0.0.95 冻结准备报告

日期: 2026-11-23
分支与基点实测:
- `v0.0.95` HEAD: `3cc877408e9eebed966aa693d33d64aa792ab49e`
- `main` HEAD (核查时): `de7b0c4c46ee0a0871b6a090f30b45f524800be7`
- `merge-base(main, v0.0.95)`: `79937f31ee97695e432a909b1f6b753b644cf789`
- `0.0.93` 冻结点 F2: `78346c972b23c7c6ae0990e3e4e12ae889428409`

## 一、v0.0.95 与 main 合并预演及与 v0.0.94 接力关系

### 1.1 改动面与交集
- `v0.0.95` 相对 `merge-base` (`79937f31ee`) 仅改 4 个文件:
  - 产品输入 (A 类 3/4): `exec/pp/directive-body-byte.tsv`, `exec/pp/text-byte.tsv`, `exec/pp/text-result.tsv` (`40df7913c5`)
  - 计划文档: `plans/v0.0.95.md`
- `v0.0.94` 与 `v0.0.95` 改动文件交集: **0 个文件** (完全无文件冲突)。
- `v0.0.95` 与 `main` 改动文件交集: **1 个文件** (`plans/v0.0.95.md`)。

### 1.2 私有副本合并预演原行 (`premerge-union --branch v0.0.95 --shard prep`)
```text
PREMERGE_FILE	plans/v0.0.95.md	OK	rule=branch-side
PREMERGE_UNION_TREE	base=de7b0c4c46	branch=3cc877408e	merge_base=79937f31ee	branch_only=3	both=1	conflict=0	ms=6968
PREMERGE_GEN	tables-gen	OK	2054	rc=0	TABLES_GEN manifests=722 stale=0 written=0 legacy=1218 hard=0
PREMERGE_GEN	signal-targets-gen	OK	30	rc=0	signal-targets-gen CHECK
PREMERGE_GEN	cx-flatten	OK	51	rc=0	SAME include/cx-flat.h 41 headers
PREMERGE_GEN	iface-hash	OK	173	rc=0	IFACE_HASH ok packages=17
PREMERGE_GEN	st-gap-gen	OK	31	rc=0	ST_GAP_GEN ok rows=7
PREMERGE_GEN	libc-decl-scan	OK	209	rc=0	OK build/libc-decls.tsv
PREMERGE_GEN	verseq-gen	DRIFT	61	rc=1	VERSEQ_SUMMARY checks=19 fail=1
PREMERGE_UNION_SUMMARY	verdict=FAIL	shard=prep	conflict=0	gen_fail=1	derive_drift=0	derive_refused=0	check_derive_fail=0	ms=9608
```
- 三方合并复核 (`git merge-tree 79937f31ee main v0.0.95`): `plans/v0.0.95.md` 为 `changed in both`, **0 个文本冲突标记 (`<<<<<<<`)**, `git merge` 可无冲突自动合并(`main` 改头部第 1-26 行, `v0.0.95` 改尾部追加第六/七节)。
- **`verseq-gen DRIFT fail=1` 根因**: `v0.0.95` 分叉后未合入 `main`, 其 `plans/v0.0.95.md` 头部仍保留旧版 `a_class=驱动与运行缓存` / `a_files_cap=12` (而 `main` 已于 2026-11-22 按 fable 裁定 A 更新为 `a_class=e2 预处理步数(模型与网)` / `a_files_cap=4`)。`premerge-union` 对双改文件默认 `rule=branch-side` 直接覆盖 `main` 头部, 触发 `verseq-gen` 与 `build/version-seq.tsv` 口径不一致。

### 1.3 与 v0.0.94 的接力合并顺序
1. 先完成 `v0.0.94` 合入 `main`、冻结、整链、公开与 `Root` 升级(含 `0.0.94` 公开后 `build/version-seq.tsv` 标已公开)。
2. **`v0.0.95` 冻结前置必做**: 在 `.claude/worktrees/v0.0.95` 中先 `git merge main`, 自动合入 `main` 上最新的 `plans/v0.0.95.md` 头部(`a_class=e2 预处理步数(模型与网)`, `a_files_cap=4`)、0.0.94 产品改动及最新工具/批次表; 同时将 `plans/v0.0.95.md` 的 `done_units` 从 `UNMEASURED` 更新为 `1`、并在冻结时填 `freeze_at`。
3. 合入 `main` 后在 `v0.0.95` 上重跑 `premerge-union` 与 `verseq-gen --check`, 确认 `conflict=0` 且 `verseq-gen fail=0`, 再进入 0.0.95 S0 合并入 `main`。

## 二、0.0.95 模板与参数化自检

- 为避免覆盖正在服务 `0.0.94` 冻结的 `release/rollback.txt` 与 `release/decision.txt`, `0.0.95` 预备模板落于以下路径(`0.0.94` 公开接力后再移入 `release/rollback.txt` / `release/decision.txt` 并把回退目标更新为 `0.0.94`):
  - `release/notes-0.0.95.md`
  - `release/rollback-0.0.95.txt`
  - `release/decision-0.0.95.txt`
- 参数化验收命令与实跑结果:
  - `sh ./unisacc.com tools/check-release-prep.cx -- --part notes --notes release/notes-0.0.95.md` -> `CHECK_RELEASE_PREP	pass=5	fail=0` (`rc=0`)
  - `sh ./unisacc.com tools/check-release-prep.cx -- --part rollback --ver 0.0.95 --prev 0.0.93 --rollback release/rollback-0.0.95.txt --decision release/decision-0.0.95.txt --notes release/notes-0.0.95.md` -> `CHECK_RELEASE_PREP	pass=23	fail=0` (`rc=0`)
  - 同步复核 `0.0.94` 默认模板未受影响: `--part notes` `pass=5 fail=0`, `--part rollback` `pass=23 fail=0`

## 三、version-seq 与 plans/v0.0.95.md 一致性核对

- `build/version-seq.tsv` 第 30 行:
  - 槽号/版本: `30 / 0.0.95`
  - 状态: `建议` (与冻结前口径一致, 公开后由 `release-chain mark-version-seq` 转已公开)
  - 主题: `e2 预处理步数(模型与网;必做:e2 重扫修复;可选:段拷贝须复测剩余可省 8 ms 以上才做;L5(a) 移出候选池)` -> 与 `40df7913c5` 实际落地及 `3cc877408e` 段拷贝关闭(<8ms)完全一致。
- `plans/v0.0.95.md` 核对:
  - `planned_units=1` 与实际(仅 1 项必做 `e2` 重扫修复 `40df7913c5`)**一致**;
  - `done_units=UNMEASURED` -> **待更新**: `40df7913c5` 已落库且 `3cc877408e` 已关闭段拷贝, 冻结前同步 `main` 时应改为 `done_units=1`;
  - `unplanned_units=0` **一致**;
  - `freeze_at=TBD` -> **符合冻结前状态**, S0 冻结时由集成者填写;
  - `a_class` / `a_files_cap`: `main` 为 `e2 预处理步数(模型与网)` / `4` (实际改 3 张表, `3/4` 合规); `v0.0.95` 分支自身尚未合 `main` 故仍为旧值 `驱动与运行缓存` / `12`, 合入 `main` 后即自动对齐。

## 四、0.0.95 预跑输入 (`prerun-inputs`, 只读)

### 4.1 直接对当前 `v0.0.95` HEAD (`3cc877408e`) 对比 `0.0.93` F2 (`78346c972b`)
```text
PRERUN_INPUTS_IDS	batches_implemented=1904	in_set=462	prev_ids=459	dropped=0	outside_set=1442	added=3	excluded=0
PRERUN_INPUTS_ADDED	tools-compile-182	est_ms=5000	measured=no	headers=product	tier=daily
PRERUN_INPUTS_ADDED	slice-audit	est_ms=5000	measured=no	headers=product	tier=commit
PRERUN_INPUTS_ADDED	verseq-gen	est_ms=7727	measured=yes	headers=product	tier=commit
PRERUN_INPUTS_ADDED_TOTAL	count=3	sum_ms=17727	est_wall_ms=738	(sum divided by lanes x jobs = 3 x 8)
```
- `v0.0.95` 自身未改 `tests/batches.tsv`, 上述 3 批来自其 `merge-base` (`79937f31ee`) 相对 `0.0.93` F2 的增量。

### 4.2 合并当前 `main` (`de7b0c4c46`) 后对比 `0.0.93` F2 (`78346c972b`)
```text
PRERUN_INPUTS_IDS	batches_implemented=1914	in_set=472	prev_ids=459	dropped=0	outside_set=1442	added=13	excluded=0
PRERUN_INPUTS_ADDED	tools-compile-182	est_ms=5000	measured=no	headers=product	tier=daily
PRERUN_INPUTS_ADDED	tools-compile-183	est_ms=5000	measured=no	headers=product	tier=daily
PRERUN_INPUTS_ADDED	tools-compile-184	est_ms=5000	measured=no	headers=product	tier=daily
PRERUN_INPUTS_ADDED	slice-audit	est_ms=5000	measured=no	headers=product	tier=commit
PRERUN_INPUTS_ADDED	verseq-gen	est_ms=7727	measured=yes	headers=product	tier=commit
PRERUN_INPUTS_ADDED	session-mirror	est_ms=217	measured=yes	headers=product	tier=commit
PRERUN_INPUTS_ADDED	premerge-union	est_ms=7616	measured=yes	headers=product	tier=commit
PRERUN_INPUTS_ADDED	team-pulse	est_ms=11073	measured=yes	headers=product	tier=commit
PRERUN_INPUTS_ADDED	tier-census	est_ms=1737	measured=yes	headers=product	tier=commit
PRERUN_INPUTS_ADDED	pp-equiv	est_ms=18473	measured=yes	headers=product	tier=commit
PRERUN_INPUTS_ADDED	unit-e2-compare	est_ms=18571	measured=yes	headers=product	tier=commit
PRERUN_INPUTS_ADDED	prerun-inputs	est_ms=24043	measured=yes	headers=product	tier=commit
PRERUN_INPUTS_ADDED	pair-o	est_ms=2626	measured=yes	headers=product	tier=commit
PRERUN_INPUTS_ADDED_TOTAL	count=13	sum_ms=112083	est_wall_ms=4670	(sum divided by lanes x jobs = 3 x 8)
```
- `0.0.94` 公开后 `main` 还会多出 `l0-subreaper`、`l0-posix2`; 届时 `0.0.95` 冻结用 `--prev 0.0.94 --prev-rev <0.0.94-F>` 将自动继承 `0.0.94` 全量批次集加 `0.0.94..0.0.95` 新增项。
- 工具观察备注: `prerun-inputs` 当前生成的 `PRERUN_CMD plan` 已用 `--normal-jobs 8 --window-lanes 3`, 但 `PRERUN_CMD run` 仍打印 `--max-jobs 4`, S3 实跑时应按手册定稿改为 `--max-jobs 8` (或请 `omp-sonnet` 同步 `tools/prerun-inputs.cx`)。
