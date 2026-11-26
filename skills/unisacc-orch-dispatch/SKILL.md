---
name: unisacc-orch-dispatch
description: 编排者向执行者窗口或子代理派工、并把交付落库到 main 或版本分支时调用;覆盖五段简报、分支与工作目录规则、devsvc 提交与推送组合、非快进处理、新工具登记、常见拦截与处置。
---

# 派工与落库规程

适用:编排者拆任务、写简报、收交付、提交推送。所有规则都附原因,原因不成立时再讨论例外。

## (1) 五段简报

每份简报固定六段,缺一段不派(第六段"工作位置"是 2026-11-26 所有者令加的):

- 目标:一句话可验收的结果。原因:模糊目标会让执行者自行扩大范围。
- 写域:只许改的路径清单。原因:写域互不相交才能并行,越界改动会打断别的车道。
- 验收:命令加预期值(退出码、关键输出行)。原因:没有预期值的验收无法判定。
- 截止:单个子代理不超过 15 分钟,12 分钟催收尾。原因:长任务失控后上下文与证据都难回收。
- 禁区:不改写域外、不删断言、不缩覆盖、不放宽时限、不提交他人路径、不 stash/reset/clean/checkout。
- 工作位置:树路径(`.claude/worktrees/w-<窗口名>`)、基点提交号、交付等于"在该树里提交并交提交号"、报告放该树 `.work/`。原因:不写位置,执行者会各自把整仓解到 /tmp 或 ~/.local/share(2026-11-26 实测 /tmp 38GB、research 12GB,含 100 多份整仓副本)。

## (2) 通用规则

- 中文、无 emoji、箭头写 ->,编号写 (1)(2)。
- 命令 55 秒内;长作业用 `sh ./unisacc.com tools/devsvc.cx -- x --bg` 加 `wait --timeout-ms`。原因:铁律 60 秒上限,留 5 秒给清理。
- 临时产物:一次性小物写 `$CLAUDE_JOB_DIR/tmp`,不写仓库 out/;每条跑检查、冷测、register、commit-own 的命令前加 `export TMPDIR=<自己的 w- 树>/.work/tmp`,让工具会话根落进自己的树。严禁往 /tmp 与 ~/.local/share/unisacc 新建仓树副本、补丁、报告或工作目录(这两处不是工作目录;/tmp 只留 tmux 与 claude 运行文件)。
- 改仓内文件只用 devsvc `f write/edit` 或 Edit/Write,不用 sed/awk/python/perl。
- 不用 load1 做任何判定。原因:负载不是根因,rc 2 与 REFUSE 默认当确定性缺陷。
- 同一失败连续两次即停手回报证据,不换说法重试。

## (3) 分支与工作目录

- 分支只有两类:`main`(集成与发布)和每个开发中版本一条 `vX.Y.Z`;每个窗口另有一棵仓内常驻私有工作树 `.claude/worktrees/w-<窗口名>`(分离头或窗口自有提交,不是长期分支)。窗口一律在自己的 w- 树里工作、提交、交提交号;编排者在目标分支的工作树里 `git cherry-pick <提交号>` 落库(同一对象库,冲突由 git 直接报,取代"交补丁文件再 apply")。建树:`EnterWorktree` 工具建 `w-<窗口名>` 后 `ExitWorktree keep`,根放 `unisacc.com` 符号链接与 `.work/`(两者写进该树 `info/exclude`),守卫钩子放行形态见 tools/guard-hook.cx。原因:2026-11-26 复盘,窗口没有常驻私有树才各自 git archive 整仓到 /tmp,基点漂移只能靠落库时 apply --check 发现。
- 版本分支的工作目录是仓内 `.claude/worktrees/vX.Y.Z`,窗口的 w- 树与它共用对象库;根放 `unisacc.com` 与 `unisacc.com.build.json` 指向共享检出同名文件的符号链接(已忽略,不提交)。直接 `git worktree add` 被守卫钩子 `tools/guard-hook.cx` 拒绝,经 `sh ./unisacc.com tools/devsvc.cx -- x --par 1 -- git worktree add ...` 建。
- 多个窗口共用同一检出(main 共享检出或版本工作目录),各自只提交自己的路径(`devsvc git commit --whole 路径`,即 commit-own 的设计目的)。devsvc 与产线工具线(tools/ tests/ build/ plans/ skills/ 的 C 类改动)直接在 main 共享检出上提交。

## (4) 提交与推送组合

```
sh ./unisacc.com tools/devsvc.cx -- git commit --bg --whole -m "中文说明" 路径1 路径2
sh ./unisacc.com tools/devsvc.cx -- wait --timeout-ms 52000   # 取 GIT_COMMIT 行
git push origin HEAD:main
```

- `--whole` 后直接跟路径,不再写 `--`。原因:多写的 `--` 会被当成路径或截断后续参数。
- 单次路径上限 64;超过就分两笔,先落新工具与批次清单,再补文档或衍生件。原因:超出上限的调用会被拒或静默截断,后者最危险。
- 逐文件或逐批 `-m`,说明只写做了什么,不加署名行。
- 非快进:先 `git pull --rebase`。冲突处置:
  - 派生表(build/*.tsv 中由 derive 生成者)取 origin/main 版,再 `sh ./unisacc.com tools/derive.cx -- --write`(或 `--only 名`)重算。原因:派生表手合必错,重算才有身份。
  - `tests/batches.tsv`、`tests/coverage.tsv`、`prd.md` 保留两边行。原因:这些是多写者键控表,丢行等于删覆盖。
- 门报 `MAIN_ADVANCED`:说明提交期间 main 前进,重新 rebase 后再提交,不强推。

## (5) 新工具与检查

- 新工具必带同名 `check-*.cx`,经 `tools/register.cx` 登记批次并进 tools-compile 清单。原因:无检查工具会被门以 new tool missing check 拒绝。
- register 冷跑 33 秒门不放宽;超了就拆检查或减夹具。冷跑指未命中编译与材料缓存的运行;热态数字不算证据,高负载下门更紧。原因:冷跑覆盖编译、装载与夹具准备,热态只测缓存命中。
- 一次只登记一个新检查。原因:多个同时登记失败时无法定位是哪一个。

### (5a) 库存比对与索引状态

`register --tool` 的库存比对只认已进入 git 索引的文件。多个新工具要按下面的节奏登记:

1. 先把全部待登记工具与检查 `git reset -- <路径>` 移出索引;
2. 逐个执行 `register --tool`,一次只登记一个;
3. 每登记成功一个,立刻 `git add -N <该工具> <该检查>`,再登记下一个。

原因:未入索引的文件在库存比对中不可见,若先全部 `git add` 再登记,工具会被判为新批次重复或清单缺口;若全部不 add,后续登记又看不到前一个的状态。逐个 add 可把失败范围锁在单个工具。

### (5b) 登记后的派生与提交

```
sh ./unisacc.com tools/derive.cx -- --write
sh ./unisacc.com tools/devsvc.cx -- git commit --bg --whole <新工具.cx> <check-新工具.cx> --keyed tests/tools-compile-sources.tsv <完整路径字段> --registered <批次ID>
```

- `--keyed` 的 key 必须是目标文件里的完整字段(整条路径),不能写简称、文件名片段或通配。多个 `--keyed` 文件并列时,每个文件各给一个完整 key。
- 提交必须同时带新工具、检查、批次行与清单行;只带其中一部分会让下一次提交门在快照里缺文件。

### (5c) 路径上限

devsvc 与 commit-own 单次最多 64 条路径。超过时先提交工具、检查与批次登记,再单独提交文档、派生表与附属材料。原因:一次塞满会触发截断,而截断后的提交仍可能成功,留下"半提交"。

### (5d) 新 `.cx` 的提交门硬阻断

| 规则 | 要求 | 常见错法 |
|---|---|---|
| cxtrunc | 长度依赖输入的 `memcpy` 改 `mm_copy(dst, cap, src, n)`;`snprintf`/`strncpy` 用 `cx_fmt` 并检查返回值 | 直接 `memcpy(dst, src, n)`;忽略返回值 |
| R4-source-anchor | 断言行为与输出,不对 `.cx` 源码做 `strstr` 文本断言 | 读源码找字符串当验收 |
| R2-subprocess-timeout | 子进程一律带超时;不裸 `sy_run` | 无 deadline 的 run |
| ref-job-paths | tracked 文件不得出现 job scratch 路径字面量 | 在文档或脚本写死 `$CLAUDE_JOB_DIR` 下的具体路径 |
| no-home-path | tracked 文件不得出现绝对家路径;写 `~/`、`$HOME/` 或仓内相对路径 | 在检查或文档里写 `/home/<名>/...` |
| outdep | entry 源不得含 `out/` 字符串字面量;材料与临时目录走 `{out}` 参数 | 在检查里写死 `out/xxx` 作输入 |

原因:这六条是提交门直接拦截项,先写好比事后返工便宜。

### (5e) 共享登记表的并发

`tests/batches.tsv`、`tests/coverage.tsv`、`tests/tools-compile-sources.tsv` 多窗共写。他窗还有在途登记改动时不要抢写,等安静窗口再登记;登记前先 `git status` 与 `git diff -- <表>` 看是否有他人未提交行。

登记批次时 `run-batch` 读的是已跟踪树快照,基线与夹具类新文件必须与登记同一次提交入库,否则门在快照里看不到文件而红。

## (6) 常见拦截与处置

| 拦截 | 处置 |
|---|---|
| private index 残锁 | 先确认持锁 PID 真不在(pgrep),再按 devsvc 提示清自己的锁;不清他人的 |
| new tool missing check | 补同名 check 并 register |
| MAIN_ADVANCED | rebase 后重提 |
| 冻结门 L0-CORE-FREEZE | 需集成者身份;执行者不提交,交补丁给集成者 |
| doc-links 漂移 | `derive.cx -- --write --only doc-links` 重算并带进同一提交 |
| 提交门被他人未提交文件干扰 | 修提交门,或让该窗口先提交自己的路径;把门的拒绝原行报给编排者;不用私有分支或另建工作树绕开 |

## (7) 公开前冻结期

某版公开前不向 main 落非阻塞项,下一版的改动提交到下一版的版本分支 `vX.Y.Z`,公开后由编排者合入。原因:冻结合并时 main 的额外提交会改变候选身份,迫使整链重跑。
