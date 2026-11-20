---
name: unisacc-orch-dispatch
description: 编排者向执行者窗口或子代理派工、并把交付落库到 main 或版本分支时调用;覆盖五段简报、隔离工作树与分支命名、devsvc 提交与推送组合、非快进处理、新工具登记、常见拦截与处置。
---

# 派工与落库规程

适用:编排者拆任务、写简报、收交付、提交推送。所有规则都附原因,原因不成立时再讨论例外。

## (1) 五段简报

每份简报固定五段,缺一段不派:

- 目标:一句话可验收的结果。原因:模糊目标会让执行者自行扩大范围。
- 写域:只许改的路径清单。原因:写域互不相交才能并行,越界改动会打断别的车道。
- 验收:命令加预期值(退出码、关键输出行)。原因:没有预期值的验收无法判定。
- 截止:单个子代理不超过 15 分钟,12 分钟催收尾。原因:长任务失控后上下文与证据都难回收。
- 禁区:不改写域外、不删断言、不缩覆盖、不放宽时限、不提交他人路径、不 stash/reset/clean/checkout。

## (2) 通用规则

- 中文、无 emoji、箭头写 ->,编号写 (1)(2)。
- 命令 55 秒内;长作业用 `sh ./unisacc.com tools/devsvc.cx -- x --bg` 加 `wait --timeout-ms`。原因:铁律 60 秒上限,留 5 秒给清理。
- 临时产物 `mktemp -d $CLAUDE_JOB_DIR/tmp/xxx.XXXXXX`,不写仓库 out/。
- 改仓内文件只用 devsvc `f write/edit` 或 Edit/Write,不用 sed/awk/python/perl。
- 不用 load1 做任何判定。原因:负载不是根因,rc 2 与 REFUSE 默认当确定性缺陷。
- 同一失败连续两次即停手回报证据,不换说法重试。

## (3) 隔离工作树与分支命名

- 分支只用三种名:`main`、`v0.0.NN`、`v0.0.NN-短名`。原因:发布与清理工具按这三种模式识别,其它名字会成为孤儿。
- 工作树只建在仓内 `.claude/worktrees/<分支名>`,根放 `unisacc.com` 与 `unisacc.com.build.json` 指向共享检出同名文件的符号链接(已忽略,不提交)。
- 守卫钩子 `tools/guard-hook.cx` 对 `git worktree add` 只放行纯数字的 `v0.0.NN` 目标;带短名的工作树需由编排者在共享会话里建好再交给执行者,执行者不得绕钩子。
- 多写者各用独立工作树;共享检出只提交自己的路径。

## (4) 提交与推送组合

```
sh ./unisacc.com tools/devsvc.cx -- git commit --bg --whole -m "中文说明" -- 路径1 路径2
sh ./unisacc.com tools/devsvc.cx -- wait --timeout-ms 52000   # 取 GIT_COMMIT 行
git push origin HEAD:main
```

- 逐文件或逐批 `-m`,说明只写做了什么,不加署名行。
- 非快进:先 `git pull --rebase`。冲突处置:
  - 派生表(build/*.tsv 中由 derive 生成者)取 origin/main 版,再 `sh ./unisacc.com tools/derive.cx -- --write`(或 `--only 名`)重算。原因:派生表手合必错,重算才有身份。
  - `tests/batches.tsv`、`tests/coverage.tsv`、`prd.md` 保留两边行。原因:这些是多写者键控表,丢行等于删覆盖。
- 门报 `MAIN_ADVANCED`:说明提交期间 main 前进,重新 rebase 后再提交,不强推。

## (5) 新工具与检查

- 新工具必带同名 `check-*.cx`,经 `tools/register.cx` 登记批次并进 tools-compile 清单。原因:无检查工具会被门以 new tool missing check 拒绝。
- register 冷跑 33 秒门不放宽;超了就拆检查或减夹具。
- 一次只登记一个新检查。原因:多个同时登记失败时无法定位是哪一个。

## (6) 常见拦截与处置

| 拦截 | 处置 |
|---|---|
| private index 残锁 | 先确认持锁 PID 真不在(pgrep),再按 devsvc 提示清自己的锁;不清他人的 |
| new tool missing check | 补同名 check 并 register |
| MAIN_ADVANCED | rebase 后重提 |
| 冻结门 L0-CORE-FREEZE | 需集成者身份;执行者不提交,交补丁给集成者 |
| doc-links 漂移 | `derive.cx -- --write --only doc-links` 重算并带进同一提交 |

## (7) 公开前冻结期

某版公开前不向 main 落非阻塞项,只推版本短名分支,公开后由编排者合入。原因:冻结合并时 main 的额外提交会改变候选身份,迫使整链重跑。
