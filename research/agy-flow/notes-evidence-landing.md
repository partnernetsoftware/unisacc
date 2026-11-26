# tools/notes-evidence.cx 工具线落库准备单(v0.1.3 公开后落 main)

- 性质: 私有落库准备单(不提交进任何 git 工作树; 当前不碰 `main` 或版本树 git 索引)
- 准备日期: `2026-11-26`
- 验证基线: `origin/main` `70756430eb4789fa0b4be3cdf71fda95aeef0c7c` (带完整 git 索引私有归档仓)
- 历史命名核查: `git log --all --oneline -- tools/notes-evidence.cx tools/check-notes-evidence.cx` 输出为空(无历史占用)

---

## (1) 交付产物与摘要

所有产物均已同步落盘至 `~/.local/share/unisacc/research/agy-flow/` 与 `~/repos/unisacc/.claude/worktrees/v0.1.3/research/agy-flow/`, 全量通过 `scrub-home-paths.cx -- --check` (`SCRUB files=0 lines=0`):

| 文件 | 用途与边界 | `sha256` |
|---|---|---|
| `notes-evidence.patch` (**推荐落库入口**) | 仅含 2 个源码文件(`tools/notes-evidence.cx`、`tools/check-notes-evidence.cx`), 不含共享登记表尾部 hunk, 跨提交零冲突 | `e5e0406c2a7f011844bc0d4ae18a788a2eb02e92ba1e9c8ef67119a06eac76cc` |
| `notes-evidence-full.patch` (完整参考) | 含上述 2 源码 + `tests/manifests/notes-evidence.tsv` + `tests/batches.tsv` + `tests/coverage.tsv` + `tests/tools-compile-sources.tsv` + `build/links.tsv` 共 7 文件 | `5082b865163ff9c646f947f7faea2eecc7325b7b9343866e446da618afd2f8ac` |

---

## (2) 规范化要点(相对私有研究版 `research/agyneon/notes-evidence.cx` 的加固)

1. **仓内零符号链接直跑**: 落入 `tools/` 后直接 `sh ./unisacc.com tools/notes-evidence.cx -- ...` 运行, 彻底消除 `v0.1.2` 期间在 `/tmp` 手工建 `cxroot.tsv`/`cx`/`ext` 符号链接的脆弱做法; 自检 `tools/check-notes-evidence.cx` 从 `tools/devsvc-repo.tsv` 读产品路径, 零 `R1-product` 字面量、零符号链接、零写仓内路径(临时合成夹具写 `$TMPDIR/cne-<pid>` 并退出清理)。
2. **严格校验与具名拒绝(消除原版静默 `rc=0`)**:
   - 用法错(缺 `--evidence-dir`、重复开关、未知开关) -> `NOTES_EVIDENCE\tREFUSE\treason=usage` (`rc=2`);
   - `ape-receipt.txt` 缺失/不可读/非 `sha=<64hex>`/非正整数 `bytes=` -> `NOTES_EVIDENCE\tREFUSE\treason=ape-receipt-invalid` (`rc=1`);
   - `fixedpoint-6targets.tsv` 缺失/不足 6 目标/目标重复/`g2_sha256 != g3_sha256`/非 `SAME`/非 64 位小写 hex -> `NOTES_EVIDENCE\tREFUSE\treason=fixedpoint-invalid` (`rc=1`);
   - 显式 `--chain-dir` 或存在 `owner.tsv`/`exit.tsv` 时, 缺其一、非正整数纪元秒或 `T2 < T1` -> `NOTES_EVIDENCE\tREFUSE\treason=chain-times-invalid` (`rc=1`); 同时修正了 `sy_local_fmt` 返回值判断(`<= 0` 为错);
   - 新增可选 `--check-notes FILE`: 逐字节核查提取出的 `APE_SUMMARY` 原行与全部 6 条 fixedpoint 行均原样出现在 `FILE` 中, 任意 1 字节篡改即报 `NOTES_EVIDENCE\tREFUSE\treason=notes-mismatch\titem=...` (`rc=1`), 全中输出 `NOTES_CHECK\tOK\tmatched=7\tfile=...`。
3. **棘轮与 lint 合规**: 所有静态函数带唯一 `ne_` / `cne_` 前缀(`dup-census` 零新增), 全量使用 `cx_fmt` 并查返回值(`cxtrunc` 零新增), 顶行注释满足 `R4-source-anchor`, 全程 `CX_LINT findings=0`。

---

## (3) 私有带索引仓实跑原行汇总

### 3.1 仓内无符号链接直跑(`release/evidence/0.1.2` + `release/notes-0.1.2.md`)

```text
$ CC=/usr/bin/cc PATH="/usr/bin:/bin:$HOME/.local/bin" sh ./unisacc.com tools/notes-evidence.cx -- \
    --evidence-dir release/evidence/0.1.2 --check-notes release/notes-0.1.2.md
APE_SUMMARY	sha=e31c413a1287a935c62df9e5c9820f91d7d760ea7e3c2c1480f99f8fc6a7c269	bytes=8992558
  - `lnx/x86_64` (`fixedpoint/g2/lnx-x86_64/entry.elf`): `4542533b550c284189f91de4000f434be74baec4790128aee218c7a9b85402ed` (`1954382` bytes, `SAME`)
  - `lnx/arm64` (`fixedpoint/g2/lnx-arm64/entry.elf`): `f7ab55f6b8cb1466c517e0f3b25c819630317d572202e5fab224593721df055c` (`2105878` bytes, `SAME`)
  - `osx/x86_64` (`fixedpoint/g2/osx-x86_64/slice-osx-x86_64`): `cc9a98e17f2c15f582daebcaa1bd88b3d7099c83ad50aac4291ea10678f81114` (`3203554` bytes, `SAME`)
  - `osx/arm64` (`fixedpoint/g2/osx-arm64/slice-osx-arm64`): `b10fc08cc1436fd57653f245e99547eb87453bff46093b8a1fbaf43d4e0cf3a5` (`4326370` bytes, `SAME`)
  - `win/x86_64` (`fixedpoint/g2/win-x86_64/slice-win-x86_64`): `ff49983e97b4f3f8a58a5235f717704a2792d0ce4310f530395a541e37dcbc46` (`3514368` bytes, `SAME`)
  - `win/arm64` (`fixedpoint/g2/win-arm64/slice-win-arm64`): `7f29174a5232c63a0906496d03bc844b4b2e41bc3944b77ad06bbb6f96f72ee1` (`4708352` bytes, `SAME`)
NOTES_CHECK	OK	matched=7	file=release/notes-0.1.2.md
```

### 3.2 自检正负例(10/10 全绿)与两项单点变异必红原行

```text
PASS	notes-evidence.real-011-extract
PASS	notes-evidence.real-012-extract-and-check-notes
PASS	notes-evidence.synth-full-with-chain-times
PASS	notes-evidence.neg-missing-evidence-dir-rc2
PASS	notes-evidence.neg-unknown-flag-rc2
PASS	notes-evidence.neg-missing-ape-receipt-rc1
PASS	notes-evidence.neg-corrupt-ape-sha-rc1
PASS	notes-evidence.neg-fixedpoint-diff-verdict-rc1
PASS	notes-evidence.neg-chain-times-backwards-rc1
PASS	notes-evidence.neg-notes-tampered-1byte-rc1
CHECK_NOTES_EVIDENCE	passed=10	failed=0
```

变异负控(单点必红):
- `MUT1` (跳过 `ne_verify_notes` 中的 `APE_SUMMARY` 比对): 仅红 `FAIL notes-evidence.neg-notes-tampered-1byte-rc1` -> `CHECK_NOTES_EVIDENCE passed=9 failed=1`, `MUT1_RC=1`。
- `MUT2` (跳过 `ne_extract_times` 中的 `t2 < t1` 检查): 仅红 `FAIL notes-evidence.neg-chain-times-backwards-rc1` -> `CHECK_NOTES_EVIDENCE passed=9 failed=1`, `MUT2_RC=1`。

### 3.3 `precommit` 同款全套门实跑原行(带 git 索引私有归档仓)

```text
CX_LINT	files=2	findings=0	elapsed_ms=9
BATCH_RUN	id=notes-evidence	status=PASS	rc=0	transport=0	marker=CHECK_NOTES_EVIDENCE	candidate=934c7c0bc09a9a4d2e107b8d6bdc855553cf05f64dbb7d6c1e5fefdb42117dd1	headers=product	scope=actual-one-batch-no-cached-result	assertions=10
REGISTER_LAYOUT_SELFTEST	id=notes-evidence	status=PASS	ms=10647
REGISTER	id=notes-evidence	status=REGISTERED
REGISTER	id=notes-evidence	manifest=WRITTEN
REGISTER_SYNC	mode=write	shards=196	shard_rows_added=0	shard_rows_removed=0
REGISTER_SYNC	status=PUBLISHED	changed_tables=1	changed_manifests=0
DERIVE_SUMMARY	ok=2	drift=0	refused=0	skipped=14	elapsed_ms=13882
REG_MANIFEST	manifests=818	bad=0	hard=0
AFFECTED_REGISTRY_CHECK	batches=2028
AFFECTED_RUN	pass=1	fail=0	deferred=0	not_applicable=0	elapsed_ms=36575	kit_ms=16459	prep_ms=0	run_ms=20116	kit_use=rebuilt	compile_use=cold
RATCHETS	ok=19	fail=0	elapsed_ms=24928
SCRUB	files=0	lines=0
```

注: `REGISTER_LAYOUT_SELFTEST` 冷跑新私有 Git 布局仅 `10647 ms` (远低于 `register.cx` 的 `33000 ms` 门槛); `dup-census` 保持 `verbatim_groups=304` 零增长; `cxtrunc` `0 above baseline, 3 below`。

---

## (4) `v0.1.3` 公开解冻后落 `main` 的可重放命令序列

在 `v0.1.3` 公开解冻、共享 `main` 恢复写窗口后, 于 `~/repos/unisacc` 执行以下序列即可一步无冲突落库并推送:

```bash
cd ~/repos/unisacc
git pull --rebase origin main

# 1. 应用 2 文件纯源码补丁(不含共享登记表 hunk, 零冲突)
git apply --check ~/.local/share/unisacc/research/agy-flow/notes-evidence.patch
git apply ~/.local/share/unisacc/research/agy-flow/notes-evidence.patch

# 2. 正规登记批次、同步工具编译源表与派生链接表
CC=/usr/bin/cc PATH="/usr/bin:/bin:$HOME/.local/bin" sh ./unisacc.com tools/register.cx -- \
  notes-evidence tools/check-notes-evidence.cx CHECK_NOTES_EVIDENCE \
  "从发布证据目录提取 APE_SUMMARY、6 目标 fixedpoint 与整链耗时，支持 --check-notes 逐字对账并具名拒绝缺失或篡改" \
  "tools/notes-evidence.cx tools/check-notes-evidence.cx" interfaces product

CC=/usr/bin/cc PATH="/usr/bin:/bin:$HOME/.local/bin" sh ./unisacc.com tools/register.cx -- --tool --sync
CC=/usr/bin/cc PATH="/usr/bin:/bin:$HOME/.local/bin" sh ./unisacc.com tools/derive.cx -- --write --only links,table-index

# 3. 核对 lint、登记与全量棘轮
CC=/usr/bin/cc PATH="/usr/bin:/bin:$HOME/.local/bin" sh ./unisacc.com tools/cx-lint.cx -- tools/notes-evidence.cx tools/check-notes-evidence.cx
CC=/usr/bin/cc PATH="/usr/bin:/bin:$HOME/.local/bin" sh ./unisacc.com tools/reg-manifest.cx -- --check
CC=/usr/bin/cc PATH="/usr/bin:/bin:$HOME/.local/bin" sh ./unisacc.com tools/ratchets.cx --

# 4. 经 devsvc git commit 一笔提交并推送(单个 --whole 后接全部自有路径)
CC=/usr/bin/cc PATH="/usr/bin:/bin:$HOME/.local/bin" sh ./unisacc.com tools/devsvc.cx -- \
  git commit -m "tools: land notes-evidence extractor, self-check, and batch registration" \
  --registered --push \
  --whole tools/notes-evidence.cx tools/check-notes-evidence.cx \
          tests/manifests/notes-evidence.tsv tests/batches.tsv tests/coverage.tsv \
          tests/tools-compile-sources.tsv build/links.tsv
```
