# research/ 归档 dry-run 记录(0.1.0 公开后;2026-11-25 ccogweco)

依据:research/agy-h/research-archive-plan.md(预案,含 306 文件分层与命令序);本页只记录盘点与 dry-run 结果,
**未执行任何迁移**。归档动作由 agy-h/agyneon 在 P0 窗口间隙执行,先 dry-run 报 cc。

## 0 只读实测(命令原行)

    git ls-files research/ | wc -l
    # 306(与预案基线一致)
    for d in research/2026-11-22 research/2026-11-23 research/2026-11-24 \
             research/agy-flow research/agy-h research/ccogweco research/omp-sonnet; do
      printf '%s %s\n' "$d" "$(git ls-files $d | wc -l)"
    done
    # 22 / 35 / 33 / 17 / 50 / 105 / 1
    git ls-files research/ | cut -d/ -f2 | sort | uniq -c | sort -rn
    # 另有 lean 10、arxiv-paper-a 7、figures 3、论文 3(unarxiv/ujs 等顶层)
    sh ./unisacc.com tools/check-archive-refs.cx
    # ARCHIVE_REFS rows=318 live_references=3 failed=0(PASS;3 条 live 引用均为
    # archive/reference-c 的契约,不是 research/)

## 1 分层与迁/不迁(按预案口径)

| 层 | 范围 | 追踪数 | 拟迁 | 说明 |
|---|---|---|---|---|
| 一 | research/2026-11-22/ | 22 | 22(分两步) | 301/302 旧页;含 r69/r70/r76/r78、u4-slices 等 |
| 一 | research/2026-11-23/ | 35 | 34 或 35 | v095..v0102、u7-exp、x16 设计等 |
| 一 | research/2026-11-24/ | 33 | 33(先改引) | G19..G29/G53 评审、R22..R40 裁定等 |
| 二 | research/agy-flow/ | 17 | 12 | 迁 car2 早期页;活页留 |
| 二 | research/agy-h/ | 50 | 38(不含外补丁/活页) | 106/107 值守与审计迁,0.1.0 活页留 |
| 二 | research/ccogweco/ | 105 | 约 90(分两步) | 11-24 与 11-25 08:00 前 106/107 页迁;0.1.0 页留 |
| 二 | research/omp-sonnet/ | 1 | 1(改引后迁) | handoff.md 被 prd_mempalace.md:280 引用 |
| 三 | 0.1.0 活页与早期论文区 | 约 69 | 0 | 明确不迁,留 research/ |

- 拟迁总量:约 230(分两步;第一步只做零引用或已改引的页)。
- 不迁(至少到 0.1.1 公开后):research/agy-h 与 agy-flow 的 v010-* 活页、research/ccogweco 的
  notes-0.1.0-*、v010-*、incident/risk/loss 等 0.1.0 起车与公开相关页;论文区(arxiv-paper-a、figures、
  unisacc-paper*、ujs-paper-outline)与 lean/。
- 被 prd_mempalace.md、plans/v0.0.109-110、build/version-seq.tsv、tests/merge-rules.tsv、tools/release-post.cx
  等点名且未改引的页,不迁(见 remove-list 页)。

## 2 引用者盘点(七处范围)

    for f in $(git ls-files research/ | sed 's|^|./|'); do
      n=$(git grep -nF -- "$f" -- prd_mempalace.md prd.md release/ plans/ build/ tests/ tools/ | wc -l)
      [ "$n" -gt 0 ] && printf '%s\t%s\n' "$n" "$f"
    done

- 预案第 3 节已列主要引用者:prd_mempalace.md(33 处)、prd.md:712、release/notes-0.0.106.md:231 与
  notes-0.0.107.md:24、build/owner-reqs.tsv(:61,:68,:70 等)、build/version-seq.tsv(:43,:52)、
  plans/(memory-safety、roadmap、v0.0.96/97/107/109/110 等)、tests/merge-rules.tsv(:2-:25)、
  tools/(release-post.cx:203、merge-version.cx:30、archive-move.cx:175)。
- 行级改写清单草案见 `archive-ref-fix-list.tsv`(先改引用再迁;未列全的行按上面命令现查)。

## 2b 第一批改引实测(2026-11-22..24 与 omp-sonnet)

- 第一批文件 90;其中有源引用者 32 个文件、67 行源引用(不含派生表 build/doc-links.tsv)。
- 分布:tests/merge-rules.tsv 23、prd_mempalace.md 21、build/owner-reqs.tsv 9、prd.md 2、plans/ 8(97/96/109/110/roadmap/memory-safety)、tools/ 3(release-post.cx:203、merge-version.cx:30、archive-move.cx:175)、build/version-seq.tsv 1。
- build/doc-links.tsv 另有 499 行匹配,属派生表,不手改;迁移后由 `tools/doc-links-gen.cx -- --write` 重算。
- 行级清单:`archive-ref-fix-list-first.tsv`(69 行:67 数据行 + 2 注释;列=research 文件/引用者:行号/新路径)。
- 预演命令(只读):`for f in $(git ls-files research/2026-11-22 research/2026-11-23 research/2026-11-24 research/omp-sonnet); do git grep -nF -- "$f" -- prd_mempalace.md prd.md release/ plans/ build/ tests/ tools/; done`

## 3 dry-run 结论与执行序

- `check-archive-refs` 当前 PASS(rows=318, live=3, failed=0);三条 live 引用属 archive/reference-c 契约,
  与 research/ 无关。
- 执行前必须:main 无活动冻结行;申请 main 写锁;先改 prd_mempalace 等引用(否则迁移后该检查会红);
  分批 `archive-move.cx` 干跑,再实迁;每批后跑 `check-archive-refs` 与 doc-links 重算。
- 回滚:按 `archive/ARCHIVE.tsv` 反向移动;引用改写单独一笔提交,可单 revert。

## 4 边界

- 本页只读盘点 + 命令原行;未迁任何文件、未改任何引用。
- 0.1.0 公开相关页不迁;归档预案执行由 agy-h/agyneon 做,先 dry-run 报 cc。
