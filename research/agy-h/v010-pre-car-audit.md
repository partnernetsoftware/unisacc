# 0.1.0 起车前只读核查页 (基于 origin/main 521797c3a8)

核查窗口: `agy-h` | 基线: `HEAD = origin/main = 521797c3a8` (`2026-11-25 08:15 +0800`, 纯只读核查, 未运行重命令、未修改任何仓内源文件或清理工作树)

总结结论: 4 项通过 / 1 项缺口(待补) —— `build/freeze.tsv` 无活动冻结、`0.0.107` 已公开且为表内最后 `已公开` 行、`release/` 下 `0.0.107` 证据齐整、`107` 提交链已完整并入 `origin/main`; 但 `build/version-seq.tsv` 中 尚未登记 `0.1.0` 行 (`NO_0.1_ROW_FOUND`), 且 `.claude/worktrees/v0.0.107` 含已入 `main` 的 `INDEX.tsv` 尾行未提交脏改, 须由 `cc` 先补 `0.1.0` 槽位与清理旧工作树后再起 `0.1.0` 车。

---

## (1) `build/freeze.tsv` 无活动冻结行 -> PASS

- 核查命令: `cat build/freeze.tsv`
- 实测原行 (`2` 行均为 `#` 注释, 活动冻结数为 `0`):
```text
# candidate_sid	base_commit	owner	deadline_epoch_s	paths
# 0.0.107-FREEZE	1fa88fbf463f208ed223937f8c4bcb974ab33e88	cc	1795572000	src cx ext exec include seed kernel tests release (published 2026-11-25 Y2=6f55d3c1b583332322078c072b8efe3c6c273937)
```

---

## (2) `build/version-seq.tsv` `0.1.0` 行状态与槽位及 `0.0.107` 最后已公开行核验 -> MISSING (`0.1.0` 未立槽)

- 核查命令:
  - `grep -n '0\.1\.' build/version-seq.tsv || echo "NO_0.1_ROW_FOUND"`
  - `grep -n $'\t已公开\t' build/version-seq.tsv | tail -n 3`
  - `nl -ba build/version-seq.tsv | sed -n '41,45p'`
- 实测原行与结论:
  1. `0.1.0` 行检查: 输出 `NO_0.1_ROW_FOUND` (全表 `60` 行 `seq=1..64` 中当前无 `0.1.0` 槽位行; 为起 `0.1.0` 车唯一前置缺口, 须先立槽再起车)。
  2. `0.0.107` 状态与最后已公开行: PASS — 第 `43` 行(槽位 `47`) 标 `已公开`, 前后两条相邻为槽位 `45` (`0.0.106 已公开`, 第 `41` 行) 与槽位 `46` (`0.0.109 建议`, 第 `42` 行), 第 `44..60` 行均为 `建议`, `0.0.107` 确为当前表内物理及语义最后一条 `已公开` 行:
```text
41:45	0.0.106	集成车(...)	已公开	...
42:46	0.0.109	前端 B2(...)	建议	...
43:47	0.0.107	前端 B1(...)	已公开	...
```

---

## (3) `release/` 下 `0.0.107` 证据完整性 -> PASS

- 核查命令:
  - `cat release/candidate.tsv`
  - `ls -1 release/evidence/0.0.107/`
  - `cat release/evidence/0.0.107/INDEX.tsv`
- 实测原行摘要:
  1. `release/candidate.tsv` (`ci_run_id=45512387135`, 七哈希完整):
     `partnernetsoftware/unisacc	v0.0.107	63354b4a2cb707ecc923a89f0df6030149fa697f	0.0.107	candidate-native.yml	v0.0.107	45512387135	unisacc.com	b773c7e221ab059e935276f41c0ac4070737f2ae6c548cd56ea8b6b0acc53c63	pkg.bin	4d13b970d75732d61126e67f1f4574a70f48eb9675b95900453dfd370d3b4f1b	slice-win-arm64	ba133d3ce42d603438b12fdfd8f0b3544cc7199f3214c5c2d101ce202da114b5	unisacc.com.build.json	d66f4ac3811acecd532917162be0fe9f9c8b08f6ce1c84290166d955d92461fc	build.json	d66f4ac3811acecd532917162be0fe9f9c8b08f6ce1c84290166d955d92461fc	-	-	SHA256SUMS	643a77e285820d541d7aaa886ff657462067fce51b5281664116233dad193f33`
  2. `release/evidence/0.0.107/` 目录含 `29` 个完整资产与严格证据文件(`ape-receipt.txt`, `cache-manager.tsv`, `ci-45512387135.{json,log}`, `draft-readback.txt`, `evidence-rows.tsv`, `expected-unmeasured.tsv`, `fixedpoint-6targets.tsv`, `INDEX.tsv`, `list-cap-margin.md`, `pack-run.out`, `prerun-supervise.log`, `prerun-windows.tsv`, `public-readback.txt`, `ratchets.txt`, `rollback.txt`, `strict.tsv` 与 `12` 个补跑/分片单文件)。
  3. `release/evidence/0.0.107/INDEX.tsv` 七步 `DONE` 与 `post-publish` 封板末段原行:
```text
exec-1-post-chain-merge	2026-11-25 07:38:35 +0800	WAIVED	2d387e524d9d2aec8d0a89f368dc864ca1fe241d	...
exec-1-X-commit	2026-11-25 07:39:14 +0800	-	8a0b2939730222c2c2581399bf962c59201e0be9	release/candidate.tsv
exec-2-draft-readback	2026-11-25 07:57:18 +0800	PASS	draft-readback.txt	gh release download, SHA256SUMS OK
exec-3-ci	2026-11-25 07:57:23 +0800	PASS	45512387135	six targets success, CANDIDATE_QUALIFY no fail/invalid
exec-5-publish	2026-11-25 07:57:29 +0800	-	v0.0.107	isDraft=false, Latest
exec-6-public-readback	2026-11-25 07:57:44 +0800	PASS	public-readback.txt	fresh download sha256 equals candidate, c47-1-readback rc 0
exec-7-Y2	2026-11-25 07:57:45 +0800	-	pending	evidence INDEX and decision recomputed equal; Y2 commit follows
exec-7-Y2-done	2026-11-25 07:57:53 +0800	-	6f55d3c1b583332322078c072b8efe3c6c273937	Y2 committed; this row rides with the next commit
post-publish	2026-11-25 08:00:10 +0800	PASS	build/version-seq.tsv	version=0.0.107 marked 已公开 (Y2=6f55d3c1b583332322078c072b8efe3c6c273937)
```

---

## (4) `0.0.107` 车 worktree 与分支状态 (仅只读核实并列清理命令, 未执行)

- 核查命令:
  - `git worktree list | grep 'v0\.0\.107'`
  - `git branch -a | grep -E '0\.0\.107|0\.1\.0|main'`
  - `git -C .claude/worktrees/v0.0.107 status --short`
  - `git -C .claude/worktrees/v0.0.107 diff`
- 实测原行:
  1. worktree 与分支:
```text
.claude/worktrees/v0.0.107     6f55d3c1b5 [heads/v0.0.107]
  main (checked out at repo root: 521797c3a8)
+ v0.0.107
  remotes/origin/main
  remotes/origin/v0.0.107
```
  2. 工作树未提交状态 (`INDEX.tsv` 在 `main` 提交 `521797c3a8` 已包含同一 `+exec-7-Y2-done` 行):
```text
 M release/evidence/0.0.107/INDEX.tsv
+exec-7-Y2-done	2026-11-25 07:57:53 +0800	-	6f55d3c1b583332322078c072b8efe3c6c273937	Y2 committed; this row rides with the next commit
```
- 待 `cc` 统一清理参考命令(仅列命令不执行; 因有上述已入 `main` 之未提交 `M`, 删除工作树需先复位该文件或带 `--force`, 分支 `v0.0.107` 依规则保留至 `0.1.0` 公开后再删):
```sh
git -C ~/repos/unisacc/.claude/worktrees/v0.0.107 checkout -- release/evidence/0.0.107/INDEX.tsv
git -C ~/repos/unisacc worktree remove .claude/worktrees/v0.0.107
```

---

## (5) `origin/main` 上自 `107 X` 起提交清单

- 核查命令: `git log --oneline --format='%h %ai %s' 8a0b293973^..origin/main`
- 实测原行 (`6` 笔提交, 已达当前 `origin/main = 521797c3a8`):
```text
521797c3a8 2026-11-25 08:14:10 +0800 v0.0.107 post-publish: mark version-seq public and seal INDEX
68b15c4028 2026-11-25 08:14:05 +0800 v0.0.107 post-publish: thaw freeze and register old-product asset
30dec679b2 2026-11-25 08:00:10 +0800 Merge branch 'origin/main'
6f55d3c1b5 2026-11-25 07:57:48 +0800 v0.0.107 Y2: public readback
72187d1e48 2026-11-25 07:56:01 +0800 v0.0.107 Y1
8a0b293973 2026-11-25 07:38:57 +0800 v0.0.107 X: candidate table
```
