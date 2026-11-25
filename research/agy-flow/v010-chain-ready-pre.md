# 0.1.0 起车前 chain-ready 九大类前置预检页

- 基线快照: `origin/main = 521797c3a882855dca29c44e6b6e8bd7cae61a5b` (经 `git archive` 解至私有临时目录 `/tmp/v010-chain-ready-pre`, 只读实测 `sh ./unisacc.com tools/chain-ready.cx -- --version 0.1.0`, 不触碰共享检出)。
- 对齐底稿: `cdx` 的 `0.1.0` 运行序 (`releaseseq` 取号落 `main` -> 建/切 `v0.1.0` -> `premerge-union --shard prep` -> 落 `0.1.0-FREEZE` 封口提交 `F` -> `chain-ready` 全绿 `READY=yes` -> 起整链)。

---

## (1) 九大类前置 (含全部 25 子项) 在 `521797c3a8` 上的实测状态与 `0.1.0` 取得方法

| 序号 / 前置项 | `521797c3a8` 实测状态与耗时 | `0.1.0` 起车预期状态 | 取得方法与精确命令 |
|---|---|---|---|
| `(1) 1-rev` | `PASS` (`121ms`): `no uncommitted or untracked paths under src cx ext exec/facts; branch main` | `PASS` | 保持 `src cx ext exec/facts` 零未提交/未跟踪文件; 若有 `DEFERRED` 门记录, 在最终树 `F` 上跑 `verify-deferred` 使 `verified_deferred_tree == HEAD^{tree}` |
| `(2) 2a-derive` | `PASS` (`45131ms`): `shards=6/6 drift=0 refused=0` | `PASS` | 取号改表后先跑 `sh ./unisacc.com tools/derive.cx -- --write`, 再核 `sh ./unisacc.com tools/chain-ready.cx -- --only 2a-derive` (避开与重编译并发导致 `product-map` 读集抖动) |
| `(2) 2b-ratchets` | `PASS` (`28629ms`): `RATCHETS ok=19 fail=0 elapsed_ms=28509 (roster=19)` | `PASS` | `sh ./unisacc.com tools/ratchets.cx --` (确认 `19/19 ok, fail=0`) |
| `(2) 15-gen-all` | `PASS` (`39184ms`): `GEN_ALL_SUMMARY total=44 pass=44 diff=0 error=0 uncovered=2 ms=34149` | `PASS` | `sh ./unisacc.com tools/gen-all-check.cx -- --root .` |
| `(2) 21-verseq-gen` | `PASS` (`82ms`): `VERSEQ_SUMMARY checks=719 fail=0` | `PASS` | `0.1.0` 入 `build/version-seq.tsv` 与 `plans/v0.1.0.md`、`prd_mempalace.md` 后跑 `sh ./unisacc.com tools/verseq-gen.cx -- --check` (严守 R1/R2/R3/R4/R5) |
| `(2) 22-premerge-union` | `FAIL` (`45131ms`): `rc=2 no PREMERGE_UNION_SUMMARY` (因 `refs/heads/v0.1.0` 尚未创建) | `PASS` | 建好 `v0.1.0` 分支且 `main` 与 `v0.1.0` 提交固定后, 先跑 `sh ./unisacc.com tools/premerge-union.cx -- --repo . --branch v0.1.0 --shard prep` 按 `(base_sha, branch_sha)` 预建联合树缓存, 再跑 `--only 22-premerge-union --version 0.1.0` |
| `(3) 3a-cxmod-tri` / `exports` | `PASS` (`674ms` / `155ms`, 预编 `16736ms`): `passed=25/21 failed=0 gaps=0` | `PASS` | `chain-ready.cx` 自动预编 `check-cxmod-tri` 与 `cx0-extract` 到私有临时目录后分两片执行 |
| `(3) 3b-cx0-extract` | `PASS` (`2647ms`): `CX0_EXTRACT EXACT bytes=118131 sha256=a971dd84...` | `PASS` | `sh ./unisacc.com tools/cx0-extract.cx -- --check` |
| `(4) 4-p1` | `PASS` (`38622ms`): `2750 files planned by cx0_plan, new-text rejections=0, child failures=0` | `PASS` | 正式起车必须跑不带 `--fast` 的完整 `chain-ready.cx` (`--fast` 会将 `4-p1` 置为 `SKIPPED` 并导致 `READY=no`) |
| `(5) 5-candidate-chain` | `FAIL` (`60128ms`): 全量冷并发下 `failed_shard=core-c(rc=142,timeout,pass=12,fail=0)` | `PASS` | 全量 `chain-ready` 前先独立预热候选链测试缓存, 或单跑 `--only 5-candidate-chain` (见第 2 节预备命令) |
| `(6) 6-affected` | `PASS` (`8113ms`): `0 affected batches for changed .cx files` | `PASS` | 若 `0.1.0` 仅改版本号与表而未改 `.cx`, 耗时 ~8s 直过; 若改 `.cx`, 先单跑 `sh ./unisacc.com tools/affected.cx -- --files ... --run` |
| `(7) 7-recipe` | `UNMEASURED` (`0ms`): 设计保留人工核对项 (不影响 `READY=yes`) | `UNMEASURED` | 起链器 `bg-candidate.cx` 自动落私有根检出、`commit-tree`、`UNISACC_IDENT_HELPER`、独立 `--out` 与 `5400s` 时限 |
| `(8) 8-inflight` | `PASS` (`40ms`): `no candidate-chain process` | `PASS` | 起车前确认 `pgrep -af candidate-chain` 零在途进程 |
| `(9) 9-ledger` | `UNMEASURED` (`0ms`): 设计保留人工账本项 (不影响 `READY=yes`) | `UNMEASURED` | 起链前在 `prd_mempalace.md` 记一行 (`REV`、真实 `PID/PGID`、截止时刻、输出目录) |
| `(10) 10-envhygiene` | `PASS` (`0ms`): `no inherited deadline and no forced cache variable` | `PASS` | 起车 shell 先执行 `unset UNISACC_RUN_CACHE UNISACC_CACHE_DIR UNISACC_BOUND_DEADLINE_MONO` 并清理非白名单 `UNISACC_*` 变量 |
| `(11) 11-version-slot` | `FAIL` (`7596ms`): `slot-version-differs-from-target 0.0.107 0.1.0` | `PASS` | `releaseseq` / 版本形态提交须先落 `main` (更新 `release/candidate.tsv` 目标行版本为 `0.1.0`, `workflow_ref=v0.1.0`), 跑 `sh ./unisacc.com tools/release-packcheck.cx -- slot . --version 0.1.0` 核验 |
| `(12) 12-root-files` / `12p` / `13` / `14` / `17` | `PASS` (`60ms` / `60ms` / `33ms` / `34ms` / `0ms`) | `PASS` | 确认 `unisacc.com` 与 `unisacc.com.build.json` 均为普通文件 (非符号链接), 且不存在 `unisacc.com.candidate.tsv` |
| `(16) 16-disk-free` | `PASS` (`2ms`): `avail_gb=73 (>=20G)` | `PASS` | 链目录所在磁盘可用空间 `>= 20G` (当前 `73G`) |
| `(18) 18-parse2-gen` | `PASS` (`64ms`): `parse2-inputs-unchanged` | `PASS` | `0.1.0` 不触 `exec/parse2` 与 `seed/gen.c`, 直过 |
| `(19) 19-chain-preflight` | `FAIL` (`54153ms`): `version-plan-missing version=0.1.0 path=./plans/v0.1.0.md` | `PASS` | 8 个 preflight flow 节点在 `521797c3a8` 已全过; 落 `plans/v0.1.0.md` (含合法正整数 `planned_units`、唯一 `a_class`、以及 `<= a_files_cap` 的手改 lane-A 文件上限) 后即转 `PASS` |
| `(20) 20-ident-helper` | `FAIL` (`0ms`): `UNISACC_IDENT_HELPER unset` | `PASS` | 从 `seed/ident.c` 预编常驻二进制到 `~/.cache/unisacc/ident-helper/identity-helper-real5` 并 `export UNISACC_IDENT_HELPER` (见第 2 节) |
| `(23) 23-freeze-armed` | `FAIL` (`2ms`): `FREEZE_NOT_ARMED` (`0.0.107-FREEZE` 已解冻) | `PASS` | 取号提交与所有 `build/tlib-packages.tsv`、`build/layer-api`、`build/iface-hash.tsv` 改动落稳后, 才落 `0.1.0-FREEZE` 封口提交 `F`, 并以 `DEVSVC_OWNER=cc` (或 `UNISACC_INTEGRATOR=cc`) 跑 `chain-ready.cx` |

---

## (2) 五项关键预备动作与精确命令序列 (`cdx` 运行序对齐)

### (2.1) `releaseseq` / `0.1.0` 取号提交必须先落 `main`, 且早于冻结提交 `F`
- 原因:
  - `(a)` `11-version-slot` 要求 `release/candidate.tsv` 数据行的 `version` 字段为 `0.1.0`, 且整链构建出的候选 APE 自报版本由 `include/unisacc_version.h` (`0.1.0`) 决定。
  - `(b)` `19-chain-preflight` 要求 `plans/v0.1.0.md` 在位且头部带 `planned_units`、`a_class`、`a_files_cap`。
  - `(c)` `23-freeze-armed` (`tools/chain-ready.cx:74-79`) 要求 `build/tlib-packages.tsv`、`build/layer-api`、`build/iface-hash.tsv` 的最后修改提交 **早于或等于** 冻结提交 `F` (`FREEZE_TLIB_AFTER_FREEZE`), 且 `HEAD..origin/main` 计数为 `0`。
  - `(d)` `release-post` 的 `verify_version_seq_readback` (`tools/release-chain.cx:1025-1035`) 取 `build/version-seq.tsv` 中最后一个 `已公开` 行作为 `last_public`, 因此 `0.1.0` 在 `build/version-seq.tsv` 中的槽位行必须排在所有已公开行 (`0.0.107` 槽位 47) 之后。

### (2.2) `20-ident-helper` 常驻重建与环境变量导出
```bash
mkdir -p "$HOME/.cache/unisacc/ident-helper"
sh ./unisacc.com seed/ident.c -o "$HOME/.cache/unisacc/ident-helper/identity-helper-real5"
chmod 0755 "$HOME/.cache/unisacc/ident-helper/identity-helper-real5"
export UNISACC_IDENT_HELPER="$HOME/.cache/unisacc/ident-helper/identity-helper-real5"
sh ./unisacc.com tools/chain-ready.cx -- --only 20-ident-helper --version 0.1.0
```

### (2.3) `5-candidate-chain` 预备命令与 `env -i` 干净环境协议 (消除 `UNISACC_PREP_CACHE` 等未登记环境变量污染)
- 实测警示 (`0.1.0` 预检踩坑): `chain-ready.cx:2168` (`10-envhygiene`) 允许 `UNISACC_PREP_CACHE`, 但 `19-chain-preflight` 的 `chain-to-snap` 节点与 `5-candidate-chain` 的 `core-c` 分片内部调用 `tools/candidate-chain.cx`, 后者的 `allowed_chain_env` 不含 `UNISACC_PREP_CACHE`, 若环境中残留 `UNISACC_PREP_CACHE` 会直接触发 `candidate-chain: REFUSE chain-env UNISACC_PREP_CACHE 未登记 (rc=2)`!
- 强制规避: 跑 `check-candidate-chain.cx --prepare` 与 `chain-ready.cx` 前一律清理 `/tmp` 残留并用 `env -i` 构造只含白名单变量的干净环境:
```bash
rm -rf /tmp/cc-prep-010 /tmp/unisacc-session-* /tmp/chain-ready-*
env -i HOME="$HOME" USER="$USER" LOGNAME="$USER" \
  PATH="/usr/bin:/bin:$HOME/.local/bin" \
  UNISACC_IDENT_HELPER="$HOME/.cache/unisacc/ident-helper/identity-helper-real5" \
  sh ./unisacc.com tools/check-candidate-chain.cx -- --prepare /tmp/cc-prep-010 tool
env -i HOME="$HOME" USER="$USER" LOGNAME="$USER" \
  PATH="/usr/bin:/bin:$HOME/.local/bin" \
  UNISACC_IDENT_HELPER="$HOME/.cache/unisacc/ident-helper/identity-helper-real5" \
  sh ./unisacc.com tools/check-candidate-chain.cx -- --prepare /tmp/cc-prep-010 generator
```

### (2.4) `22-premerge-union` 按 `(base_sha, branch_sha)` 预热联合树缓存
- `tools/premerge-union.cx` 的联合树缓存键绑定 `(main_HEAD_sha, v0.1.0_HEAD_sha)`。每当 `main` 或 `v0.1.0` 前进一步, 旧缓存键即失效, 必须在封口提交 `F` 上先跑 `--shard prep`:
```bash
sh ./unisacc.com tools/premerge-union.cx -- --repo . --branch v0.1.0 --shard prep
sh ./unisacc.com tools/chain-ready.cx -- --only 22-premerge-union --version 0.1.0
```

### (2.5) `23-freeze-armed` 封口提交 `F` (最后一笔提交, `F = chain start commit`)
- 在 `0.1.0` 取号与代码/表全部落定并推送 `origin/main` 与 `origin/v0.1.0` 后, 才写入 `build/freeze.tsv` 唯一活动行 (`0.1.0-FREEZE<TAB><pre_freeze_sha><TAB>cc<TAB><deadline><TAB>src cx ext exec include seed kernel tests release`), 提交为 `F` 并同步 `origin/main` 与 `origin/v0.1.0`:
```bash
env -i HOME="$HOME" USER="$USER" LOGNAME="$USER" \
  PATH="/usr/bin:/bin:$HOME/.local/bin" \
  DEVSVC_OWNER=cc \
  UNISACC_IDENT_HELPER="$HOME/.cache/unisacc/ident-helper/identity-helper-real5" \
  sh ./unisacc.com tools/chain-ready.cx -- --version 0.1.0
```

---

## (3) `0.0.107` 夜踩过的 4 个坑与 `0.1.0` 强制规避表

| 编号 | `0.0.107` 夜实踩故障 | 触发机理与源码位置 | `0.1.0` 强制规避办法 |
|---|---|---|---|
| `(1)` | `UNISACC_IDENT_HELPER` 路径丢失 / 未传入 `release-run --pack` | `chain-ready.cx:2197` 与 `release-run.cx --pack` 均要求 `UNISACC_IDENT_HELPER` 指向可执行普通文件; 若指向 `/tmp` 临时路径或起子 shell 漏传环境变量, `20-ident-helper` 或 `M2 --pack` 直接 `REFUSE` | 统一固定编译到 `$HOME/.cache/unisacc/ident-helper/identity-helper-real5` (链产出后也可复用 `$CHAIN/ctx/identity-helper`), 所有 `chain-ready`、`bg-candidate`、`release-run --pack` 命令显式带 `export UNISACC_IDENT_HELPER="$HOME/.cache/unisacc/ident-helper/identity-helper-real5"` |
| `(2)` | 冻结后补提交 (`F1 -> F2 -> F3 -> F4`) 触发 `FREEZE_TLIB_AFTER_FREEZE` 与冻结行反复重写 | `23-freeze-armed` 要求 `F` 本身即起链提交, 且 `tlib-packages.tsv`、`layer-api`、`iface-hash.tsv` 不得晚于 `F`; 同时 `precommit.cx:664-669` 禁止同一提交同时改 `build/freeze.tsv` 与受冻路径 | 严格执行 "先落完 `releaseseq` / `plans/v0.1.0.md` / `derive --write` / `verseq-gen --check`, 最后单笔只写 `build/freeze.tsv` 作为封口提交 `F`"; 解冻时同样先单独提交 `build/freeze.tsv` 再跑 `release-post` |
| `(3)` | `21-verseq-gen` R3 (`palace-pair`) 与 `release-post` `last_public` 行序拒绝 (`F3 -> F4` 及 `post-publish` Step 2) | `(a)` `tools/verseq-gen.cx` R3 要求 `prd_mempalace.md` 中每个 `plans/v0.0.MM.md` 前面最近的独立版本号必须等于 `MM` (`0.0.107` 夜因同一行先写 `0.0.100/0.0.106` 后跟 `plans/v0.0.107.md` 触发 R3 FAIL, 迫使重封 `F4`); `(b)` `release-chain.cx:1025-1035` 扫描 `build/version-seq.tsv` 最后一个 `已公开` 行作为 `last_public` | `(a)` 改 `prd_mempalace.md` 或 `plans/v0.1.0.md` 后立即跑 `sh ./unisacc.com tools/verseq-gen.cx -- --check` 确认 `VERSEQ_SUMMARY fail=0`; `(b)` `0.1.0` 在 `build/version-seq.tsv` 的行位必须置于所有 `已公开` 行之后 |
| `(4)` | `2a-derive` 中 `product-map` 冷态超时或 `readset-changed-before-output` (`rc=2`) 及 `5-candidate-chain` `core-c` 分片冷编译超时 (`rc=142`) | 全量 `chain-ready.cx` 同时拉起 `derive` 6 分片、`ratchets`、`gen-all`、`check-candidate-chain` 5 分片; 冷 `e3cache` 下并发写缓存会令 `product-map` 报 `REFUSE readset-changed-before-output`, 并令 `core-c` 超过 60 秒子进程限 | 在跑全量 `chain-ready.cx` 前, 先串行跑一次 `sh ./unisacc.com tools/derive.cx -- --check --only product-map` 与 `check-candidate-chain.cx -- --prepare` 预热缓存, 随后再跑全量 `chain-ready.cx -- --version 0.1.0` |
