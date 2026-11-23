# 0.0.94 公开后第一批执行检查表(只读)

作者:csr(共同执行人) 日期:2026-11-23  
状态:私有草稿(main 静默期不落库);公开后落 `research/post-publish-checklist.md`  
依据:`research/2026-11-23/freeze-runbook-draft.md` 第四节;`~/.local/share/unisacc/research/` 各补丁目录;归档草稿验收结论。

硬前置:0.0.94 已公开发布(Y2 完成)、main 静默解除、集成者宣布可写 main。  
每项给出:命令、验收行、归属窗口、gatekeeper 是否必需。未实跑不写 PASS。

---

## (0) 解冻与写权限(S10 前后夹缝)

| 项 | 命令 | 验收行 | 归属 | gatekeeper |
|---|---|---|---|---|
| 解冻提交闸 | 删除或提交移除 `build/freeze.tsv`(勿碰历史 `release/freeze.tsv`) | `test ! -f build/freeze.tsv`;裸 `git commit` 不再被 freeze_gate 拒 | cc/集成者 | 否 |
| 释放 freeze 锁 | `sh ./unisacc.com tools/devsvc.cx -- lock release freeze --owner <owner>` | `lock list` 无 freeze 持有 | cc | 否 |

---

## (1) S10 `release-post`(先 dry)

编排入口(runbook 第四节 / PHASE S10):

```
sh ./unisacc.com tools/release-post.cx -- run --version 0.0.94 --dry
# 全绿后再:
sh ./unisacc.com tools/release-post.cx -- run --version 0.0.94 --execute
# Root 升级另加(强前置见下):
sh ./unisacc.com tools/release-post.cx -- run --version 0.0.94 --execute --root-upgrade
```

| 子步 | 命令要点 | 验收行 | 归属 | gk |
|---|---|---|---|---|
| 1 dry 全步 | 上栏 `--dry` | dry 报告各步 OK/可执行;无自动 push | cc | 否 |
| 2 INDEX 封尾 | 随下一提交带 `exec-7-Y2-done` | `release/evidence/0.0.94/INDEX.tsv` 含该行 | cc | 否 |
| 3 mark-version-seq | `release-chain mark-version-seq --version 0.0.94 --dry` 再 `--commit --push` | dry 与 `verseq-gen --check` 无误后再 commit | cc | 否 |
| 4 backup-evidence | `release-chain backup-evidence --version 0.0.94 ...` 再 `--verify` | `BACKUP_VERIFY PASS files=N` | cc | 否 |
| 5 metrics | `version-metrics-gen -- --write-history` 再 `--write`;`day-metrics -- --write` | 三步均写出;不可漏 history | cc | 否 |
| 6 Root 升级前置 | 合规 0.0.93 备份槽 + `install-ape -- . --rollback` | `INSTALL_APE ROLLBACK_DRYRUN <0.0.93-sha256>` | cc | 否 |
| 7 Root 升级 | `--root-upgrade` 或手册装已验 APE | 本机 Root 身份=0.0.94 APE | cc | 否 |
| 8 mempalace | 手工改 `prd_mempalace.md` | 已公开版本/F/APE/CI/Y1/Y2/Root 原位更新 | cc | 否 |

---

## (2) 版本重排补丁 01..04 + `verseq-gen --check`

私有目录:`~/.local/share/unisacc/research/rearrange-patch/`  
顺序:刷新后的 `01` -> `02` -> `03`(或旧 01..03 再加 `04`;与直刷 01..03 字节等价)。勿对共享 main 在静默期 apply。

```
P=~/.local/share/unisacc/research/rearrange-patch
# 干净树预演(推荐 git archive 私有树):
git apply --check $P/01-version-seq-and-plans.patch
git apply --check $P/02-prd-candidate-pool.patch
git apply --check $P/03-roadmap-rearrange-note.patch
# 若用旧 01..03 再: git apply --check $P/04-branch-sync-and-pidfd-pool.patch

git apply $P/01-version-seq-and-plans.patch
git apply $P/02-prd-candidate-pool.patch
git apply $P/03-roadmap-rearrange-note.patch
sh ./unisacc.com tools/verseq-gen.cx -- --check
```

验收行:
- `APPLY_0N=OK`(`git apply --check` rc=0)
- `VERSEQ_SUMMARY	checks=29	fail=0`(刷新后 01..03 口径;旧树曾为 checks=21)
- sha256 与 README 表一致(防拿错备份版)

归属:cc 主导 apply;csr 可对拍 verseq。gatekeeper:建议对 `verseq-gen` 与计划路径规则要(改 version-seq/plans)。

---

## (3) main 合入版本分支(冲突预演)

重排定号后再合。先读后合:

```
for b in v0.0.95 v0.0.96 v0.0.97 v0.0.98 v0.0.99 v0.0.100 v0.0.101; do
  echo "==== $b ===="
  git fetch origin $b 2>/dev/null
  git log --oneline main..$b | head -20
  git log --oneline $b..main | head -10
  # 冲突预演(只读):
  sh ./unisacc.com tools/merge-version.cx -- preview --branch $b
  # 或: git merge-tree $(git merge-base main $b) main $b | head
done
```

合入顺序建议(runbook 10 + 重排说明):
1. 确认 `build/version-seq.tsv` 槽主题与各分支一致(尤其原 0.0.96「执行核」vs 进程生命周期已拆到重排后号段)
2. `v0.0.95`(e2/pp,与 94/96 零文件交集可较早)
3. `v0.0.96` 线(进程生命周期;合前把最新 main 合进分支并重跑 `check-proc-lifecycle`)
4. `v0.0.97` / `v0.0.98` / `v0.0.99` / `v0.0.100` / `v0.0.101` 按依赖与车道空闲串行

验收行:
- `MERGE_VERSION verdict=READY conflicts_left=0`(或 preview 等价)
- `git log -1 --merges` 见双父合并;`ff=0`
- 各车道 check:如 `CHECK_PROC_LIFECYCLE`、前端相关批次(按分支主题)

归属:cc 集成;各版本窗口准备自己的分支 tip。gatekeeper:合入产品输入的分支要。

---

## (4) 归档搬迁

草稿:`~/.local/share/unisacc/research/archive-move-draft/`(仓内镜像 `.claude/archive-move-draft/`)  
公开后拷 `archive-move.cx`/`check-archive-move.cx` -> `tools/` 并登记批次。

```
# 落库后:
sh ./unisacc.com tools/check-archive-move.cx --
# 预期: CHECK_ARCHIVE_MOVE failed=0

sh ./unisacc.com tools/archive-move.cx -- --dry --manifest tools/archive-move-manifest.tsv
# 预期: ARCHIVE_MOVE DRY n=21

sh ./unisacc.com tools/archive-move.cx -- --apply --manifest tools/archive-move-manifest.tsv --date $(date -u +%F) --note post-0.0.94
sh ./unisacc.com tools/doc-links-gen.cx -- --write
sh ./unisacc.com tools/check-archive-refs.cx
# 手工: research/2026-11-22/README.md 改指归档位置或标已归档
```

搬前核对(已采纳):
- `owner-reqs` 的 flow-next 非 `research/` 路径
- A+B 无未完结 0.0.95 产品面

验收行:`ARCHIVE_MOVE DONE n=21`;`check-archive-refs` 绿;`doc-links` 无漂移。  
归属:csr 执行;cc 复核。gatekeeper:否(文档归档),但 `check-archive-refs` 必跑。

---

## (5) 未提交补丁队列(按依赖排序)

图例:归属 / gatekeeper(是/否) / 依赖。

### 5.1 已落库(公开后只核身份,不重 apply)

| 项 | 核命令/验收 | 归属 | gk |
|---|---|---|---|
| check-signal-targets 父批补断言 | `sh ./unisacc.com tools/check-signal-targets.cx --` -> failed=0;慢拆 `check-slow-split` 父=片并集 | ccogweco(已落 04bda40737 一带) | 否 |
| check-precommit-derive ln -sn | 批次/夹具绿;与 `check-ln-noderef` 一致 | ccogweco | 否 |
| freeze-day / guard-hook 等 F 提交 | 以 main 日志为准,不重打补丁 | 各作者 | 否 |

### 5.2 待落(建议序)

| # | 项 | 私有路径 | 命令/验收 | 归属 | gk | 依赖 |
|---|---|---|---|---|---|---|
| A | prerun-inputs 打印修正 | `.../cmd-parity-patches/01-prerun-inputs-cmd-parity.patch` | `git apply --check` 后 apply;`check`/`prerun-inputs` 相关夹具绿 | agy-flow/cc | 建议 | 无 |
| B | freeze-day 命令自检 | `.../cmd-parity-patches/02-freeze-day-cmd-selfcheck.patch`(+`04-freeze-day-only-selfcheck` 若仍有效) | `check-freeze-day` 绿;`--selfcheck` 过 | agyneon | 建议 | 可与 A 并行 |
| C | release-chain max-jobs 对齐 | `.../cmd-parity-patches/04-release-chain-maxjobs.patch`(`release-chain.cx:93`) | apply 后读码 `--max-jobs`/`--normal-jobs` 与 P0 一致;相关 check 绿 | agy-flow | 建议 | 最好在 A 后 |
| D | devsvc 冻结门作用域 r2 | `.../freeze-scope/freeze-scope-r2.patch` | `check-devsvc-freeze` 含 `FREEZE_SCOPE skip=worktree`;先红后绿日志在同目录 | omp-go/cc | 是 | 解冻后;动 devsvc |
| E | 守卫 v3 + prose 拆分 | `.../mech-guards/guard-v3.patch` + `prose*.patch`/`prose-v2`/`prose-apology`/`prose-ctx` | 按 README 序 apply;`guard-hook` 夹具/gatekeeper 条件通过 | omp-sonnet | 是 | 互不与 D 抢同一文件则并行 |
| F | cx0-extract 常量漂移 | `.../cx0-drift/01-constant-drift.patch` | `--check` 对 plan/fragment 漂移报 `CX0_EXTRACT DRIFT` 且 rc=2;对齐后 EXACT | ccogweco | 是 | 无强依赖 |
| G | stub TMPDIR 隔离 | `.../chain-speedup/stub-tmpdir-isolate.patch` | apply;`whole-front` stub 不共享污染 TMPDIR;相关探针绿 | agy-flow | 建议 | 与 H 同域,先 G 或同审 |
| H | front1..3 并行 | `.../chain-speedup/wf-front-parallel.patch`(+`chain-pack-early` 可选) | apply 后私有链或探针证明 front1..3 并行、merged 字节等价 | agy-flow | 是 | G 后或同批审 |
| I | split-pair 门 | `.../split-pair-gate/`(`check-split-pair.cx`+`01-precommit-wire.patch`) | `check-split-pair --selftest` 四夹具;`precommit` 命中拆分组时拒不等并集 | ccogweco | 是 | 解冻后;动 precommit |
| J | 水位哨(CXM headroom) | `.../headroom/check-cxm-syms-headroom.cx` | 入库为 `tools/check-cxm-syms-headroom.cx`(或 `check-cap-watermarks`);`HEADROOM_*` 行;超阈 REFUSE;登记批次 | omp-go | 建议 | 可独立;与 0.0.98 CXM_MAXU 叙事对齐 |
| K | evidence-identity / release-flow 缓存 | `.../evidence-identity/`、`.../release-flow/`(目录若仍空则向作者索路径) | 作者补齐补丁后再 apply;验收行以作者 README 为准 | 待定(cc 派) | 待定 | 标 UNMEASURED 直至路径齐 |
| L | consumer-kill / 信号清理类 | `.../consumer-kill-fixes/*.patch` | 按 README 逐个;`affected`/`bg-candidate`/`prerun-drive` 无裸 kill;夹具绿 | 多窗口 | 建议 | 互审文件冲突 |
| M | gen-all hang 夹具 | `.../s3-hang-fix/check-gen-all-hang.patch` | apply;`check-gen-all-check` hang 门绿 | agyneon | 建议 | 无 |
| N | prerun prepare 相位 | `.../prerun-prepare/01..03-*.patch` | 按 design.md 序;prepare 暖/冷夹具绿 | 待定 | 建议 | 可与链加速分开 |

依赖简图:

```
解冻 -> A/B/C (cmd 口径) 
     -> D (devsvc scope) || E (guard/prose) || F (cx0) || I (split-pair)
解冻 -> G -> H (front 并行)
解冻 -> J (水位哨,可后)
K 路径齐后再排
```

---

## (6) 旧工作树清理

备份已在:`~/.local/share/unisacc/archive-worktrees/2026-11-23/`  
(`v0.0.88-dirty.tar.gz`、`v0.0.89-old-*.tar.gz`/`*.bundle`、`v0.0.93-dirty.tar.gz` 及 manifest)

```
# 盘点
git worktree list
ls -la .claude/worktrees/

# 0.0.94 工作树(S10):分支保留到 0.0.95 公开
git worktree remove .claude/worktrees/v0.0.94   # 仅在无未提交必保改动时

# 旧树:先核备份完整,再问所有者(runbook: v0.0.88 / v0.0.89-old 有脏改动)
test -f ~/.local/share/unisacc/archive-worktrees/2026-11-23/v0.0.88.manifest.txt
test -f ~/.local/share/unisacc/archive-worktrees/2026-11-23/v0.0.89-old.manifest.txt
test -f ~/.local/share/unisacc/archive-worktrees/2026-11-23/v0.0.93.manifest.txt
# 所有者确认后:
git worktree remove --force .claude/worktrees/v0.0.88      # 示例名以 list 为准
git worktree remove --force .claude/worktrees/v0.0.89-old
git worktree remove --force .claude/worktrees/v0.0.93
```

验收行:`git worktree list` 不再列上述路径;备份 tar/bundle 可 `tar -tzf`/`git bundle verify`。  
归属:cc 发起;脏树问原所有者。gatekeeper:否。

---

## 建议总序(一天内)

1. (1) S10 dry -> execute(含 Root 前置)  
2. (0) 解冻  
3. (2) 重排 01..03 + verseq  
4. (3) 预演并合版本分支  
5. (5) 补丁队列 A..J(按依赖并行)  
6. (4) 归档搬迁  
7. (6) 旧工作树清理  
8. (5)K/L/M/N 收尾  

证伪:任一步验收行未出现即停,不宣称该步完成。
