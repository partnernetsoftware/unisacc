# 0.1.0 说明链后回填清单(5 分钟内一次填完;2026-11-25 ccogweco)

对象:research/ccogweco/notes-0.1.0-final.md。所有命令只读;回填后由 agy-flow 同步到车树与草稿正文。
原则:每个占位只从其指定来源取,不从推断填;取不到就保持占位并报 cc。

## 1 占位与取值命令

| 占位/句子 | 来源文件 | 取值命令(只读) |
|---|---|---|
| <CI_RUN_ID> | CI run 列表与 INDEX | `grep 'exec-3-ci' release/evidence/0.1.0/INDEX.tsv`;`gh run list -R partnernetsoftware/unisacc --json databaseId,headSha -L 5` 按 headSha 过滤 |
| <APE5_SHA256> | pack layout | `sha256sum <pack>/layout/unisacc.com` |
| <SID5_64HEX> | pack 输出 | `grep -m1 'RELEASE_RUN	SOURCES_SID	VERIFIED' <pack>/pack-run.out`(取第 3 列) |
| 公开时刻 | INDEX | `grep 'exec-5-publish' release/evidence/0.1.0/INDEX.tsv`(第 2 列) |
| X 提交 | INDEX | `grep 'exec-1-X-commit' release/evidence/0.1.0/INDEX.tsv`(第 4 列) |
| Y1 | INDEX | `grep 'exec-4-Y1' release/evidence/0.1.0/INDEX.tsv` |
| Y2 | INDEX | `grep 'exec-7-Y2' release/evidence/0.1.0/INDEX.tsv` |
| 预跑窗数与耗时 | 预跑 supervise.log | `grep -m1 'PRERUN_RUN status=DONE' <prerun>/supervise.log`;窗数取 windows.tsv 数据行数 |
| strict 计数 | 合并 strict | `grep -E '^SUMMARY' release/evidence/v0.1.0-strict-<DATE>.tsv`;行数取 `rows=` 或 `wc -l` |
| list-cap 登记与披露 | expected-unmeasured 与披露页 | `grep -P '^precommit-list-cap\t' release/evidence/0.1.0/expected-unmeasured.tsv`;披露引 research/ompgpt/margin-disclosure-107.md |
| 三条零改动断言原行 | 链/pack 回执 | git diff 空、SID 两快照相等、fixedpoint/g2 与 pkg.bin 的 sha 对照(命令见 notes-0.1.0-final 第 2 节) |

## 2 回填顺序(建议,约 5 分钟)

1. strict 与 INDEX 就位后先取 CI run、X、公开时刻、Y1/Y2(四条 grep);
2. 取 APE5/SID5(pack 已存在);
3. 取预跑窗数与耗时;
4. 取 strict SUMMARY 与 list-cap 行;
5. 三条零改动断言的原行粘贴到正文对应句;
6. 全文 grep 占位符 `<` 残留,应为 0;grep 家目录路径应为 0。

## 3 起车期间新发现缺陷的记载

- notes-0.1.0-final.md 已补"起车中发现并处置的缺陷"一节,按 cc 周知列名:
  batch-domains 漏检、worktree 形态、version-seq 读回按行序;具体机制与处置原行以当轮窗口页为准,
  本节不写未核实细节。
- 若三项中任一影响候选字节或身份,必须在说明与证据 INDEX 同时披露;不影响则只作流程记录。

## 4 边界

- 不预填任何值;不在链前把预计写成实测;占位残留即不得过 Y1。
