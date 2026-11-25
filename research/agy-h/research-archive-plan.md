# research/ 归档方案预案 (`0.1.0` 公开后执行)

编写窗口: `agy-h` | 状态: 只读盘点与预案(未执行任何迁移) | 基线: `origin/main` 与 `research/` 现存 306 个入库追踪文件

---

## 1. 工具实测口径与适用边界

1. `tools/archive-candidates.cx`: 只读扫描分级(`sh ./unisacc.com tools/archive-candidates.cx -- --days N`)。本次实测(`2026-11-25 10:16`, `load1=2.77`) 触发 `archive-candidates: git grep rc=124` (`55s` 子进程限截断全仓单次 `git grep -o -F`)，因此本预案改用分目录 `git ls-files` + 定向 `git grep` 完成零误差只读盘点。
2. `tools/check-archive-refs.cx`: 今日实测 PASS (`ARCHIVE_REFS rows=318 live_references=3 failed=0`)。关键规则(`:31-45,127-175`):
   - 清单 `archive/ARCHIVE.tsv` 中 `D<TAB>path...` 目录级行只登记不重扫; 普通路径行若在 `archive/` 外被引用且前缀不是 `archive/<一级目录>/`, 即报 `FAIL referenced-by:<file>` (豁免集 `:152-159` 不包含 `prd_mempalace.md`、`release/`、`research/`、`plans/v0.0.10x`)。
   - 目标 `archive/research-2026-11-25/<窗口>/` 中 `research-2026-11-25` 为 `archive/` 下合法一级目录, 满足 `archived_at()` 规则。
3. `tools/archive-mv.cx` 与 `tools/archive-move.cx`:
   - `archive-mv.cx`: 单文件原子移动与回滚工具, 可用 `--dry-run` 逐文件打印全部 `REF` 行; 但其 `:123` 硬编码目标为 `archive/<原路径>`, 不直接映射到 `archive/research-2026-11-25/<窗口>/`。
   - `archive-move.cx` + `tools/archive-move-manifest.tsv`: 按清单 `F|D<TAB>src<TAB>dst` 批量执行 `git mv`、追加 `archive/ARCHIVE.tsv` 与 `archive/INDEX.md` 存根, 可直接指定目标 `archive/research-2026-11-25/<窗口>/`, 但不自动重写交叉引用, 需由迁移步先重写引用再调用, 最后重跑 `doc-links-gen --write` 与 `check-archive-refs`。

---

## 2. 按窗口与日期分层的迁移清单 (仓内 306 文件; 目标 `archive/research-2026-11-25/<窗口>/`)

说明: 仓内 `research/` 共追踪 306 个文件 (`git ls-files research/`)。另仓外 `~/.local/share/unisacc/research/` (`cdx/`, `ompgpt/`, `agyneon/`, `fable-v01x-flow/` 等) 未入 git 追踪, 不进本次 `git mv` 范围; `research/` 顶层既有论文与早期设计 (`prior-art.md`, `paper-<id>-intent.md`, `cx2-module-design.md`, `arxiv-paper-<id>`, `figures/`, `lean/` 等约 43 文件) 保持原位。

| 分层 | 仓内源目录/范围 | 追踪文件数 | 拟迁移数量 | 拟迁目标路径 | 说明 |
|---|---|---|---|---|---|
| 层一: `2026-11-22..24` 旧页 | `research/2026-11-22/` | 22 | 22 (分两步) | `archive/research-2026-11-25/2026-11-22/` | 含 `r69`/`r70`/`r76`/`r78`、`u4-slices/`、`zero-assert/`、`mempalace-rewrite/`、`uaf-census-retro/` 等; 多项被 `build/owner-reqs.tsv`/`prd_mempalace.md`/`plans/` 引用, 须先改引用 |
| 层一: `2026-11-22..24` 旧页 | `research/2026-11-23/` | 35 | 34 或 35 | `archive/research-2026-11-25/2026-11-23/` | 含 `v095`/`v096`/`v097`/`v0100`/`v0102`、`u7-exp/`、`own-flow-caps/`、`x16-block-extern-<kind>`、`s10-old-worktrees-cleanup.md` 等; `release-post.cx:203`、`merge-version.cx:30`、`tests/merge-rules.tsv` 有硬路径引用, 须先同步改引或暂留 |
| 层一: `2026-11-22..24` 旧页 | `research/2026-11-24/` | 33 | 33 (先改引) | `archive/research-2026-11-25/2026-11-24/` | 含 `G19-review.txt`..`G29-review.txt`、`G53-review.txt`、`D8-report.txt`、`R22`..`R40` 裁定页、`hung-tests.md`、`tmp-<kind>` 等; 其中 `R25/R26/R27/R30/R31/R37/R38/R39` 与 `D8` 被 `prd_mempalace.md` 与 `0.1.0` 活页引用 |
| 层二: `0.0.107` 及之前窗口页 | `research/agy-flow/` | 17 | 12 | `archive/research-2026-11-25/agy-flow/` | 迁 `car2-ci-dryrun.md`、`chain-to-x-ledger.md`、`car2-stage-c-run.md`、`handoff-for-fresh-session.md`、`w0014-rerun.md`、`car2-parallel-prep.md`、`car2-live-clock.md`、`round2-diffs.md`、`round2-outer-timeout.md`、`keepgoing-rerun-plan.md`、`round2-w0213-plans.md`、`release-107-done.md`; 留 5 份 `0.1.0` 活页 |
| 层二: `0.0.106`/`0.0.107` 窗口页 | `research/agy-h/` | 50 | 38 (不含外补丁/活页) | `archive/research-2026-11-25/agy-h/` | 迁 `0.0.106`/`0.0.107` 值守、审计与草稿页(如 `margin-audit.md/.tsv`、`margin-packages.md`、`car2-timeline-v2.md`、`prerun-rate.tsv`、`prerun-rate-107.tsv`、`prerun-rate-107-r2.tsv`、`chain-to-public.tsv`、`chain-to-public-r2.tsv`、`path-c-event-107.md`、`two-chains-107-facts.md`、`round2-sampling.md`、`stop-event-r2.md`、`prerun-r2-summary.md`、`publish-day-card-107.md`、`lessons-107-night.md`、`ci-watch-107.md`、`wp7-plan.md/patch`、`u06-load-plan.md` 等); 留 12 份 `0.1.0`/`0.1.x` 活页及 `prerun-watch.patch` |
| 层二: `0.0.106`/`0.0.107` 窗口页 | `research/ccogweco/` | 105 | 约 90 (分两步) | `archive/research-2026-11-25/ccogweco/` | 迁 `11-24` 与 `11-25 08:00` 前 `106`/`107` 会话与核对页(`notes-0.0.106-final.md`、`notes-0.0.107-<stage>.md`、`wp6-<part>`、`wp8-<part>`、`e01-<part>`、`e02-<part>`、`u08/u11/u30/u33`、`post-106-<part>`、`incident-playbook-107.md`、`loss-guards-107.md`、`gate5-margin.md` 等); 留 `0.1.0`/`0.1.x` 活页与未合补丁 |
| 层二: 已退场窗口 | `research/omp-sonnet/` | 1 | 1 (改引后迁) | `archive/research-2026-11-25/omp-sonnet/` | `handoff.md`, 被 `prd_mempalace.md:280` 引用, 改指归档路径后迁 |
| 层三: `0.1.0` 活页与早期论文区 | `research/agy-flow`、`agy-h`、`ccogweco` 活页 + 顶层论文区 | 约 69 | 0 (明确不迁) | 留原地 `research/` | 见第 4 节不迁清单 |

---

## 3. 仓内引用统计与需同步改写的引用行

按当前 `git grep` 实测(排除 `archive/` 与只读产物基线 `build/doc-links.tsv`、`build/no-home-path-baseline.tsv`、`tools/doc-links-baseline.tsv`):
- `build/doc-links.tsv` 中匹配 `research/(2026-11-22|2026-11-23|2026-11-24|agy-flow|agy-h|ccogweco|omp-sonnet)/` 的去重链接目标共 81 个 (`research/` 全库共 1023 行链接记录), 迁移完成后由 `tools/doc-links-gen.cx --write` 自动重算。
- 非派生源文件引用数与具体位置如下(引用计数不为 0 的文件迁移前必须先改写为 `archive/research-2026-11-25/<窗口>/<路径>`, 否则 `check-archive-refs.cx` 拒收):

| 引用者文件 | 引用次数/行号 | 被引 `research/` 路径 | 迁移前/后改写要求 |
|---|---|---|---|
| `prd_mempalace.md` | 33 处命中 (`:39,:43,:44,:46,:109,:138,:152,:210,:211,:220,:263-:286,:317-:320`) | `2026-11-22/`、`README.md`、`dropped.tsv`、`u4-slices/`、`r70-review`、`r76-<kind>`、`../../archive/research-2026-11-25/2026-11-22/zero-assert/phase2-design-v2.md`; `../../archive/research-2026-11-25/2026-11-23/pp-equiv-evidence.md`; `2026-11-24/{D8-report.txt,R25,R26,R27,R30,R31,R37,R38,R39}`; `agy-h/{car2-timeline-v2.md,margin-audit.tsv,margin-packages.md}`; `ccogweco/<files>` (15 个文件); `../../archive/research-2026-11-25/omp-sonnet/handoff.md` | 决定块与索引页被引路径改写为 `archive/research-2026-11-25/...`; 其中 `../ccogweco/roadmap-0.1.x.md`、`prerun-speedup-0.1.x.md`、`notes-0.1.0-draft.md`、`seal-0.1.0-evidence-map.md`、`file-macro-scan.md`、`cxm-maxu-256.patch` 属 `0.1.0`/`0.1.x` 在用件, 暂不迁 |
| `prd.md` | `:712` (2 处) | `../../archive/research-2026-11-25/2026-11-23/u7-experiment.md`、`../../archive/research-2026-11-25/2026-11-23/label-local-phase1.md` | U7/标号局部化仍在 `0.1.x` 规划内; 要么两页暂不迁, 要么把 `prd.md:712` 改为 `archive/research-2026-11-25/2026-11-23/...` 后迁 |
| `release/notes-0.0.106.md` 与 `release/notes-0.0.107.md` | `106:231`; `107:24` | `../ccogweco/unverified-classification.md` (`106`); `../ccogweco/incident-playbook-107.md` (`107`) | 重要边界: 已发布 `notes-0.0.106/107.md` 原则上不随归档回改历史发布说明时, 上述 2 份被已发布 notes 直接点名的文件建议暂留原地或征得公开后统一修订确认再迁 (`margin-disclosure-107.md` 与 `path-b-proof-107.md` 在仓外, 不受 `git grep` 校验影响) |
| `build/owner-reqs.tsv` | `:61,:68,:70` 等 (`2026-11-22/<files>`) | `../../archive/research-2026-11-25/2026-11-22/litlen/real-bugs-41.tsv`, `../../archive/research-2026-11-25/2026-11-22/r69-report/report.md`, `../../archive/research-2026-11-25/2026-11-22/r69-6-shadow/report.md`, `../../archive/research-2026-11-25/2026-11-22/zero-assert/phase2-design-v2.md`, `../../archive/research-2026-11-25/2026-11-22/r76-design/design.md`, `../../archive/research-2026-11-25/2026-11-22/r76-review2/review.md`, `r78-v090-named/`, `../../archive/research-2026-11-25/2026-11-22/uaf-census-retro/report.md` | 若迁 `2026-11-22/` 这些报告, 同步更新 `build/owner-reqs.tsv` 对应列为 `archive/research-2026-11-25/2026-11-22/...`, 或对 `2026-11-22/` 先迁零引用叶目录(`G19..G29`/`u7` 等) |
| `build/version-seq.tsv` | `:43,:52` | `2026-11-23/v099-b1-series/` (第 43 行注记)、`../../archive/research-2026-11-25/2026-11-23/x16-block-extern-design.md` (第 52 行 `0.0.110` 建议行) | 第 43 行可改 `archive/research-2026-11-25/...`; 第 52 行为未来 `0.0.110` (`0.1.x`) 设计依据, 建议 `x16-block-extern-design.md` 与 `x16-block-extern-cases/` 暂留不迁 |
| `plans/` | `plans/memory-safety.md`, `roadmap-0.0.83-0.0.99.md:341`, `v0.0.96.md:77`, `v0.0.97.md:14,16`, `v0.0.107.md:10`, `v0.0.109.md:11`, `v0.0.110.md:11` | `../../archive/research-2026-11-25/2026-11-22/uaf-census-retro/report.md`, `../../archive/research-2026-11-25/2026-11-23/version-rearrange-proposal.md`, `frontend-phase3-scope.md`, `v097-kernel-step-proto.md`, `v099-b1-series/`, `../../archive/research-2026-11-25/2026-11-23/v0100-b2-series/0001-b2-k2-caps-table-gen-probe.patch`, `x16-block-extern-design.md` | 已关闭版(`0.0.96/97/107`)计划行同步改指 `archive/...`; 未来 `0.1.x` 待办(`0.0.109/110`)引用的 `v0100-b2-series/` 与 `x16-block-extern-<files>` 建议留原地 |
| `tests/merge-rules.tsv` | `:2-:25` (23 处) | `../../archive/research-2026-11-25/2026-11-23/v096-merge-preview-2026-11-23.md` | 该文件被 23 条合并规则注释及 `tools/merge-version.cx:30` 硬引用: 迁前须先改写 `tests/merge-rules.tsv` 与 `tools/merge-version.cx:30`, 或将其列为不迁锚定文件 |
| `tools/` | `tools/release-post.cx:203`, `tools/merge-version.cx:30`, `tools/archive-move.cx:175` | `../../archive/research-2026-11-25/2026-11-23/s10-old-worktrees-cleanup.md`, `../../archive/research-2026-11-25/2026-11-23/v096-merge-preview-2026-11-23.md`, `../../archive/research-2026-11-25/2026-11-22/README.md` | 改这三行 `tools/` 源码会触及 `tools/` 提交与工具校验, 建议将上述 3 个被 `tools/` 硬引用的文件暂留, 或随工具线批次一起改引后迁 |
| 留存 `research/` 活页内部 | `clock-rule-draft.md:5,17`, `seal-0.1.0-checklist.md:3`, `u06-load-plan.md:5`, `car2-108-candidates.md` 等 | 引用 `../../archive/research-2026-11-25/2026-11-24/R27-design.md`, `R30-boundary.md`, `R31-plan24h.md`, `R37-seal.md`, `R38-ruling.md`, 以及部分 `ccogweco/` 页 | 若迁移目标页而活页保留原地, 必须同步把活页内引用改写成 `archive/research-2026-11-25/...` (`check-archive-refs.cx` 扫 `research/`) |

注: `../../archive/research-2026-11-25/2026-11-24/G19-review.txt`..`G29-review.txt`、`G53-review.txt` 等零仓外引用(经 `margin-audit.tsv` 引用的 `G24`/`G29` 将随 `margin-audit.tsv` 同迁或先改写) 可作为零冲突第一梯队直接归档。

---

## 4. 明确不迁清单 (公开后继续保留原位)

1. `0.1.0` 活页(预跑、折叠、发版、P0 交接期间直接引用):
   - `research/agy-h/`: `v010-pre-car-audit.md`, `v010-priority-newbatches.txt`, `v010-prerun-startcard.md`, `chain-to-public-010.tsv`, `prerun-rate-010.tsv`, `prerun-watch.cx`, `check-prerun-watch.cx`, `prerun-watch-v3.md`, `seal-0.1.0-checklist.md`, `roadmap-0.1.x-draft.md`, `car2-108-candidates.md`, `research-archive-plan.md`, `prerun-watch.patch`(待 `0.1.x` 合入)。
   - `research/agy-flow/`: `v010-plan-only.patch`, `v010-slot-commit.patch`, `worktree-shape-chain-check.md`, `v010-chain-ready-pre.md`, `lanes-ci-parallel-eval.md`。
   - `research/ccogweco/`: `notes-0.1.0-draft.md`, `notes-0.1.0-skeleton-v2.md`, `notes-0.1.0-final.md`, `v010-admission-forecast.md`, `v010-chain-fill-script.md`, `v010-final-docs-index.md`, `v010-notes-backfill.md`, `v010-prerun-evidence-section.md`, `v010-risk-stop-table.md`, `seal-parallel-prep.md`, `seal-0.1.0-evidence-map.md`, `prd-decision-block-fable-flow.md`, `chain-fill-dryrun.md`, `roadmap-0.1.x.md`, `prerun-speedup-0.1.x.md`, `file-macro-scan.md`, `cxm-maxu-256.patch`。
2. `prd_mempalace.md` 与 `0.1.x` 长期决定/技术依据页:
   - 未把 `prd_mempalace.md` 引用路径改写为 `archive/research-2026-11-25/...` 之前, 严禁移动 `prd_mempalace.md:39-320` 点名的任何决定依据页(`R25`/`R26`/`R27`/`R30`/`R31`/`R37`/`R38`/`R39`、`D8-report.txt`、`pp-equiv-evidence.md`、`margin-audit.tsv`、`margin-packages.md`、`after-seal-roadmap.md`、`post-106-ledger.md` 等)。
   - 被 `plans/v0.0.109.md`、`plans/v0.0.110.md`、`build/version-seq.tsv:52`、`tools/release-post.cx:203`、`tools/merge-version.cx:30`、`tests/merge-rules.tsv`、`release/notes-0.0.106/107.md` 引用的页面, 除非同步改引(或先确认历史发布说明不可改原则), 否则留原地。

---

## 5. 迁移执行命令序与回滚办法 (`0.1.0` `release-post` 解冻后在 `main` 跑)

### 5.1 迁移命令序 (严禁在公开前执行)

1. 前置检查: 确认 `0.1.0` 已公开且 `build/freeze.tsv` 无活动冻结行; 通过 `tools/devsvc.cx` 申请 `main` 写锁, 在干净共享 `main` 上建迁移清单 `research/agy-h/archive-move-2026-11-25.tsv` (`F|D<TAB>src<TAB>archive/research-2026-11-25/<窗口>/...`)。
2. 逐文件只读预检引用(`archive-mv.cx --dry-run`): 对清单中的普通文件批运行干跑以核对所有字面引用(注意: 不执行无 `--dry-run` 的 `archive-mv.cx`, 因它固定迁到 `archive/<原路径>`):
```sh
set -e
while IFS="$(printf '\t')" read -r kind src dst; do
  [ -z "$kind" ] && continue
  [ "${kind#\#}" != "$kind" ] && continue
  if [ "$kind" = "F" ]; then
    sh ./unisacc.com tools/archive-mv.cx -- --dry-run "$src"
  fi
done < research/agy-h/archive-move-2026-11-25.tsv
```
3. 先改写仓内活引用: 对第 3 节列出的 `prd_mempalace.md`、`build/owner-reqs.tsv`、`plans/`、及留存 `research/` 活页, 使用 `tools/refactor.cx` 或 `edit` 工具将待迁文件旧路径精确改写为 `archive/research-2026-11-25/<窗口>/<路径>`; 对被保留且不改写引用者(如已公开 `release/notes-0.0.106/107.md` 所引页)从迁移清单剔除。
4. 批量移动并登记 `archive/ARCHIVE.tsv` 与 `archive/INDEX.md`:
```sh
set -e
sh ./unisacc.com tools/archive-move.cx -- --dry --manifest research/agy-h/archive-move-2026-11-25.tsv
sh ./unisacc.com tools/archive-move.cx -- --apply --manifest research/agy-h/archive-move-2026-11-25.tsv \
  --date 2026-11-25 --note "post-0.1.0 research cleanup (11-22..24 and 0.0.106/107 window pages)"
```
5. 补全文件级/校验与派生重算:
```sh
set -e
sh ./unisacc.com tools/doc-links-gen.cx -- --write
sh ./unisacc.com tools/check-archive-refs.cx
sh ./unisacc.com tools/derive.cx -- --check
sh ./unisacc.com tools/ratchets.cx
git status --short -- research/ archive/ prd_mempalace.md build/doc-links.tsv
```
6. 单一提交落 `main`: 待以上门全部 `rc=0` PASS 后, 用 `git commit -- <明确迁移与更新路径> && git push origin main` 一次收尾并释放写锁。

### 5.2 回滚办法 (`archive-mv` 逆操作)

1. 提交前回滚 (`archive-move --apply` 中途失败或 `check-archive-refs`/`ratchets` 失败):
   - 对清单内每个已移动条目按 `archive-mv.cx:213-215` 语义执行逆移动与表恢复:
```sh
set -e
while IFS="$(printf '\t')" read -r kind src dst; do
  [ -z "$kind" ] && continue
  [ "${kind#\#}" != "$kind" ] && continue
  if [ -e "$dst" ]; then
    mkdir -p "$(dirname "$src")"
    git mv "$dst" "$src"
  fi
done < research/agy-h/archive-move-2026-11-25.tsv
git checkout -- archive/ARCHIVE.tsv archive/INDEX.md build/doc-links.tsv prd_mempalace.md build/owner-reqs.tsv
```
   - 重跑 `sh ./unisacc.com tools/check-archive-refs.cx` 确认 `failed=0`。
2. 提交后撤回 (`archive` 提交已入 `main`):
   - 直接执行 `git revert --no-edit <ARCHIVE_COMMIT>` 还原全部文件位置、`ARCHIVE.tsv`、`INDEX.md`、`doc-links.tsv` 及被改写引用, 再跑 `doc-links-gen --write`、`check-archive-refs.cx` 与 `ratchets.cx` 验证通过后推送; 若仅需单文件退回, 对单文件按 `git mv archive/<dst> <src>` + 引用改回原路径 + 删 `archive/ARCHIVE.tsv` 对应行 + `doc-links-gen --write` + `check-archive-refs.cx` 收口。

---

## 6. 预计耗时、锁与时机要求

1. 时机硬禁区 (`0.1.0` 公开前不得执行): 归档会改动 `prd_mempalace.md`、`archive/ARCHIVE.tsv`、`build/doc-links.tsv`, 触发 `doc-links`/`derive`/`ratchets` 全表检查, 并推动 `origin/main` 前移(触犯 `prd_mempalace.md:39` 冻结禁推 `origin/main` 与干扰在跑预跑/尾段合入)。必须严格排在 `0.1.0 release-post` 完成解冻之后执行(可放在 `P0` 预备窗口末尾纯文档收尾段)。
2. 锁要求: 必须持 `tools/devsvc.cx` 写窗口锁。因改写涉及 `prd_mempalace.md`、`build/doc-links.tsv`、`archive/ARCHIVE.tsv`、`archive/INDEX.md` 等共享文件, 且需保持原子性重算 `doc-links-gen --write` 与验证 `check-archive-refs.cx`, 不得与其他改动文档或 `build/` 的窗口并发。
3. 预计耗时 (`UNMEASURED`, 未实际运行迁移):
   - 制作精确 `archive-move-2026-11-25.tsv` 与跑 `archive-mv --dry-run` 清单扫引用: 约 `3-5` 分钟;
   - 改写 `prd_mempalace.md` 与零星存活页引用 + `git grep` 复核清零: 约 `5-8` 分钟;
   - `archive-move --apply` + `doc-links-gen --write` + `check-archive-refs` + `derive --check` + `ratchets`: 约 `5-8` 分钟;
   - 全流程预计 `15-22` 分钟 (远低于 2 小时分治限; 若拆分, 可按"第 1 批零引用 `2026-11-24/G19..G53` 与 `0.0.106/107` 独立页 -> 第 2 批需改 `prd_mempalace` 引用页"两批串行落库, 每批 10 分钟内完成)。
