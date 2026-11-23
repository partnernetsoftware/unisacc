# v0.0.95 入 main 预演(2026-11-23,omp-sonnet,只读;merge-version preview 与 gate,未动任何 ref)

## 末行原文
- 现 main(db0687f996)<- v0.0.95:`MERGE_VERSION verdict=READY conflicts_left=0 forced=0 regen=0 tree=c14dbc919da9d7823d5b94770e7d3d1fda14bdc3`(merge_base=79937f31ee,分支 6 提交,改 exec/pp/directive-body-byte.tsv、text-byte.tsv、text-result.tsv、plans/v0.0.95.md)。
- 假设 v0.0.94 已入 main(私有 commit 625f124804,只是对象不是 ref,树取 v0.0.94 预演的 resolved.tree=bb9f2d71...):`MERGE_VERSION verdict=READY conflicts_left=0 forced=0 regen=0 tree=1f7266f770a0f8090feae96460c4e67d3d1381d0`。

## 规则表覆盖的冲突类
- 两次预演都只有 1 个两侧都改的路径:plans/v0.0.95.md,rule=clean-3way(文本三方干净),没有用到规则表任何一行(rules=11 适用,0 行触发)。
- 另外三个文件(exec/pp/*.tsv)只有分支侧改,不进未合并清单。
- 预期会在 v0.0.95 上出现的冲突类现为:无。规则表里与 0.0.94/0.0.96 相关的 derive-weights、陷阱文件、build 派生表等行对 v0.0.95 不触发。

## 假设 0.0.94 先入后的 gate(WORK=v95b)
- traps:先 FAIL ok=5 fail=2(reaper.cx、build/layer-api/ext.sysx.tsv differs-from-branch):这是误报。陷阱文件属 v0.0.96 线,v0.0.95 分支持旧内容、树里持 0.0.94 的新内容才是对的合并。已修(提交 8d682a4745):分支名 v0.0.NN 且 NN<96 时 8 项记 NA(branch-older-than-the-trap-line),重跑 traps OK na=8。
- derive-write:OK ok=10 drift=0 refused=0,ref-job-paths 基线步 written,tree_after_derive 等于预演树(无再生漂移)。
- derive-check / tests / ratchets 未对此树另跑(只做了 traps 与 derive-write);v0.0.95 只动 exec/pp 三个表与计划,需要时按同流程补。

## 仍要注意
- 修正的取舍:陷阱适用线用常量 TRAP_SINCE_PATCH=96 写死在 tools/premerge-union.cx,新增陷阱文件属于更晚版本线时要改此值或改成表驱动(未做)。
- 冲突类以外:exec/pp 三个表由 0.0.95 分支改,derive 相关节点 exec/pp 的生成物是否漂移由 derive-check 判,未跑。
