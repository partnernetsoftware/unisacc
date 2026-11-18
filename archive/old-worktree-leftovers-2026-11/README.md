# 旧版本工作树未提交内容存档(2026-11-18)

来源:.claude/worktrees/v0.0.67 至 v0.0.70 的未提交改动。每版一个 vX.patch(git diff 原文)与 vX.status.txt(git status --short 原文与 HEAD)。四个分支相对 main 均无未合入提交。

- v0.0.68:只有 build/links.tsv、table-index.tsv、tool-index.tsv 派生表重算物 -> 已移除工作树,分支保留。
- v0.0.70:派生表加 prd_productmap.md 生成快照哈希 -> 已移除工作树,分支保留。
- v0.0.67:含 prd.md 能力速查一行(runneridentity,工具已在 main 但这一行未在 main)-> 工作树留原位,待并入。
- v0.0.69:含未入 main 的 prune-skip-gen 工具、事实表、清单与批次行 -> 真实未合入工作,工作树留原位。

回滚:移除的工作树用 git worktree add .claude/worktrees/v0.0.68 v0.0.68 重建,再 git apply 对应补丁。
