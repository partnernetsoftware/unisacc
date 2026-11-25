# worktree-version-shape-v010.patch 独立复核: chain-ready 与消费者逐项对账

## (1) 复核输入身份与环境

- 基线提交: `c78092c086a2fe04762ab85ff4a8c0ecc738ab65` (`origin/main`, 含 `c67b497670` `tools.cxlib 0.4.13 releaseseq` 与 `c78092c086` `build/version-seq.tsv` 槽位 65 `0.1.0 进行中`)
- 被核补丁: `research/ompgpt/worktree-version-shape-v010.patch`
  - `sha256 = e680659e772b0e95a5be76d9872a61a23a7d7f3c30f6fc3f2bfa98d973f0112e`
  - 范围: 16 个文件 (8 个工具: `tools/chain-ready.cx`、`tools/premerge-union.cx`、`tools/verseq-gen.cx`、`tools/freeze-gate.cx`、`tools/day-metrics.cx`、`tools/dvgitx.cx`、`tools/guard-hook.cx`、`tools/stable-eval.cx`; 8 个对应测试 `tools/check-*.cx`)
- 私有预演环境: `/tmp/wt-shape-rehearsal` (从 `c78092c086` 经 `git archive` 解出后 `git init -q -b main`, 绑定私有裸远端 `/tmp/wt-shape-origin.git`, 打基线标签 `v0.0.107`, 依次应用 `worktree-version-shape-v010.patch` 与 `v010-slot-commit.patch`, 落 `0.1.0-FREEZE` 封口提交并推 `origin/main`; 全程未触碰共享检出)

---

## (2) cdx 消费者清单逐条对账表

| 消费者文件与 cdx 清单行号 | 原 `v0.0.NN` 硬编码行为 | 补丁修改内容 (基于 `cxlib.releaseseq` `rs_version`) | 对账结论 |
|---|---|---|---|
| `tools/chain-ready.cx:409` (`j_branch`) | `tx_prefix(o, l, "v0.0.", 5)` + 仅末尾纯数字放行 | 新增 `branch_version(o, l, ver)` (`o[0] == 'v' && rs_version(ver)`), `1-rev` 放行 `main` 或任意合法 `vX.Y.Z` 分支 | 已改且实测通过 (`branch v0.1.0`) |
| `tools/chain-ready.cx:1005` (`seq_next_version`) | `tx_prefix(r.out, (long)r.outn, "v0.0.", 5)` 识别版本分支 | 改为 `branch_version(r.out, (long)r.outn, branch_ver)`: 在 `v0.1.0` 工作树上不误取 `main` 的下一槽位, 交由 `release-packcheck slot` 从当前分支名取 `0.1.0` | 已改且实测通过 (`source=branch`) |
| `tools/chain-ready.cx:1364` (`check_version_scope`) | `tx_prefix(r.out, l, "v0.0.", 5)` 提取 `ver` 查 `plans/v<ver>.md` | 改为 `!branch_version(r.out, (long)r.outn, ver)`: 在 `v0.1.0` 分支不传 `--version` 时自动提取 `0.1.0` 并校验 `plans/v0.1.0.md` | 已改且实测通过 (`scope version=0.1.0`) |
| `tools/premerge-union.cx:111/123` (`detect_active_branch`) | `starts_with(v, "0.0.")` 从 `build/version-seq.tsv` 找唯一未公开分支 | 改为 `rs_version(ver)` 并拼 `v%s`, 在 `main` 上不传 `--branch` 时自动从 `build/version-seq.tsv` 槽位 65 选出 `v0.1.0` | 已改且实测通过 (自动选 `v0.1.0`) |
| `tools/premerge-union.cx:265` (`merge_one_both` 分支优先判据) | `if (starts_with(rel, "plans/v0.0.") \|\| streq(rel, "release/candidate.tsv"))` | 新增 `version_plan(rel)` (支持 `plans/vX.Y.Z.md` 与 `plans/vX.Y.Z-<suffix>.md`, 经 `rs_version` 校验), `int is_plan = version_plan(rel)` | 已改且实测通过 (`plans/v0.1.0.md` 走分支侧) |
| `tools/premerge-union.cx:280` (`merge_one_both` `rule_out` 命名) | `starts_with(rel, "plans/v0.0.") ? "branch-side" : "branch-side-version-slot"` | 改为 `is_plan ? "branch-side" : "branch-side-version-slot"` | 已改且实测通过 (`rule=branch-side`) |
| `tools/verseq-gen.cx:285` (`scan_worktrees` R4 及 R1/R2/R3/R5/`--reorder`) | R4 仅扫 `v0.0.NN` 工作树目录, 表解析用 `minor_after("0.0.")` | 引入 48 位三段版本键 `version_token`/`version_text`/`version_fmt`, R4 改为 `d.name[0] == 'v' && rs_version(nm + 1)` 及 `version_token(l, t2, t1 + 1, &key) == t2`, 同步升级 R1/R2/R3/R5 与 `--reorder` | 已改且实测通过 (`.claude/worktrees/v0.1.0` 纳入 R4 扫描, checks `87 -> 125`, `fail=0`) |
| `tools/freeze-gate.cx:208` (默认基线 tag 查找) | `git tag --list "v0.0.[0-9]*" --sort=-v:refname` | 改为 `git tag --list "v[0-9]*" --sort=-v:refname` 并以 `rs_version(tagbuf + 1)` 过滤首个合法发布 tag | 已改且实测通过 |
| `tools/day-metrics.cx:284` (`is_allowed_branch`) | `!strncmp(b, "v0.0.", 5)` + 纯数字判定合规分支 | 改为 `return b[0] == 'v' && rs_version(b + 1);` (`v0.1.0`/`v0.1.4` 计为合规分支, 不误报 stray branch) | 已改且实测通过 |
| `tools/dvgitx.cx:178` (`wt_form`) | `strncmp(q, "v0.0.", 5)` 拦截非 `v0.0.NN` 工作树 | 改为 `if (br[0] != 'v' \|\| !rs_version(br + 1)) return 0;` | 已改且实测通过 (`dvgitx worktree add .claude/worktrees/v0.1.0 v0.1.0` `rc=0`) |
| `tools/guard-hook.cx:172/201` (`ver_branch_ok` / `worktree_leaf`) | 只放行 `v0.0.NN` 分支与 `.claude/worktrees/v0.0.NN` | 改为 `rs_version` 校验, 放行 `vX.Y.Z` 并拒绝 `v0.0`、`v01.0.0`、`v0.1.0.1` 及工作树与分支名不一致 | 已改且测试夹具通过 |
| `tools/stable-eval.cx:344` (`public_day`) | `tx_prefix(version.p, version.n, "0.0.", 4)` | 改为 `rs_version(ver)` 读取 `release/evidence/0.1.0/public-readback.txt` | 已改且测试夹具通过 |

---

## (3) 私有仓库模拟 `.claude/worktrees/v0.1.0` 与分支 `v0.1.0` 实跑原行

### (3.1) `dvgitx worktree add` 建 `.claude/worktrees/v0.1.0` 与分支 `v0.1.0`
```text
WORKTREE	add	rc=0	path=/tmp/wt-shape-rehearsal/.claude/worktrees/v0.1.0	branch=v0.1.0	branch_new=1	link=unisacc.com	ignored=yes
```

### (3.2) 在 `.claude/worktrees/v0.1.0` 上运行 `chain-ready.cx` (不传 `--version`, 验证分支 `v0.1.0` 自动识别)
```text
1-rev	PASS	no uncommitted or untracked paths under src cx ext exec/facts; branch v0.1.0	83
READY	yes	failed=none	57
11-version-slot	PASS	VERSION_SLOT OK version=0.1.0 target=0.1.0 source=branch expect_self_report=unisacc 0.1.0	147
READY	yes	failed=none	152
21-verseq-gen	PASS	VERSEQ_SUMMARY checks=125 fail=0	109
READY	yes	failed=none	114
23-freeze-armed	PASS	armed: freeze.tsv sid=0.1.0-FREEZE owner=cc deadline=1795590000; HEAD behind origin/main=0; numbering files unchanged after the freeze	499
READY	yes	failed=none	506
19-chain-preflight	PASS	CHAIN_PREFLIGHT rev=HEAD verdict=PASS failed=- nodes=9 ms=30782 mode=strict; scope version=0.1.0 planned_units=1 a_class=零改动封存 a_files=0/1	31055
READY	yes	failed=none	31060
```

### (3.3) 在 `main` 上运行 `21-verseq-gen` 与 `premerge-union` (含双侧同时修改 `plans/v0.1.0.md` 的 `rule=branch-side` 实测)
```text
21-verseq-gen	PASS	VERSEQ_SUMMARY checks=125 fail=0	57
READY	yes	failed=none	65
PREMERGE_FILE	plans/v0.1.0.md	OK	rule=branch-side
PREMERGE_UNION_TREE	base=2d25ca0571	branch=523c862c22	merge_base=3eca4049b4	branch_only=0	both=1	conflict=0	ms=7325
PREMERGE_GEN	tables-gen	OK	2240	rc=0	TABLES_GEN manifests=800 stale=0 written=0 legacy=1237 hard=0
PREMERGE_GEN	signal-targets-gen	OK	31	rc=0	signal-targets-gen CHECK
PREMERGE_GEN	cx-flatten	OK	41	rc=0	SAME include/cx-flat.h 41 headers
PREMERGE_GEN	iface-hash	OK	181	rc=0	IFACE_HASH ok packages=17
PREMERGE_GEN	st-gap-gen	OK	31	rc=0	ST_GAP_GEN ok rows=7
PREMERGE_GEN	libc-decl-scan	OK	192	rc=0	OK build/libc-decls.tsv
PREMERGE_GEN	verseq-gen	OK	82	rc=0	VERSEQ_SUMMARY checks=90 fail=0
PREMERGE_UNION_SUMMARY	verdict=PASS	shard=prep	conflict=0	gen_fail=0	derive_drift=0	derive_refused=0	check_derive_fail=0	ms=10797
```

---

## (4) 复核发现的一个必补派生表同步项 (`build/table-index.tsv`)

- 现象: 在 `22-premerge-union` 的 `shard=derive-a` 实测中发现:
  `DERIVE table-index DRIFT 276 rc=1 DIFF build/table-index.tsv line=17`
- 原因: `main` 上的提交 `c78092c086` (`build: add 0.1.0 seal slot row 65 in build/version-seq.tsv`) 为 `build/version-seq.tsv` 新增了第 65 号槽位行 (行数 `60 -> 61`), 但未同步刷新 `build/table-index.tsv` 第 17 行的 `build/version-seq.tsv` 行数记录。
- 处置建议: 在 `0.1.0` 车前取号/形态提交落 `main` 时, 一并执行 `sh ./unisacc.com tools/derive.cx -- --write --only table-index` 将 `build/table-index.tsv` 纳入提交, 使 `2a-derive` 与 `22-premerge-union` `shard=derive-a` 全绿。

## (5) 复核结论

- `research/ompgpt/worktree-version-shape-v010.patch` (`sha256 = e680659e...`) 对 `cdx` 清单所列全部消费者位置 (`chain-ready:409/1005/1364`、`premerge-union:111/123/265/280`、`verseq-gen:285 R4` 及 R1-R5/`--reorder`、`freeze-gate:208`、`day-metrics:284`) 以及 `dvgitx`、`guard-hook`、`stable-eval` 均已完整改写为 `cxlib.releaseseq` (`rs_version`), 并在 `.claude/worktrees/v0.1.0` (分支 `v0.1.0`) 私有仓库实测中全部一次通过。**建议放行合入 `main`** (合入时顺带 `derive.cx -- --write --only table-index` 消除 `c78092c086` 留下的 `build/table-index.tsv` 1 行漂移)。
