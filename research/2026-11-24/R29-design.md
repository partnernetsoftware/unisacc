来源:会话临时目录 /home/devali/.claude/jobs/7f46e032/tmp/R29/design.md 拷贝于 2026-11-24
# R29 设计裁定:共享检出的脏路径、暂存区与代推

日期 2026-11-24,只读取证,未改仓内文件。标"推断"的未经实跑,标"未核"的未读到源码。

## 0 最重要的判断

今天四起事故里三起的根不是"缺归属登记",而是共享暂存区(.git/index)是一份无主、无时限、无人负责清空的共享可变状态。commit-own 早已用私有索引造提交(tools/commit-own.cx:15、599、729),共享索引在提交数据路径上已经多余,却仍留着三个写入口和一个读入口。裁定:把"共享索引恒等于 HEAD"立为不变式并机器守住,先于租约登记。租约登记做被动版,不做"改前必须声明"。

## 1 取证(真实数据)

(1) 当前共享检出:git status 31 条,其中暂存 25 条;25 条里 7 条为 MM(暂存内容与工作区再次分叉:prd_mempalace.md、team-scoreboard.md、batches.tsv、coverage.tsv、tools-compile-sources.tsv、check-precommit-flow.cx 等)。暂存项工作区时间从 12:29 到 17:28,跨五小时,即多个窗口多轮的残留叠在一份索引里。
(2) 共享索引的写入口:devsvc git add(devsvc-core.cx:3678,注释自述"清残留暂存用")、commit-own 提交后对已提交路径补 cacheinfo(commit-own.cx:22)、合并流程。读入口:commit-own --staged(commit-own.cx:1483、1641,"索引里的 blob 就是提交内容,不看工作区")与 --merge-continue。
(3) 旧基底快照回退的机理(推断,与现象吻合):某窗口早前 git add 了 precommit.cx 与 coverage.tsv;其后他人经 commit-own 落库同文件新版,commit-own 只给"刚提交路径"补 cacheinfo,但这两个文件若是经 keyed 或另一窗口提交,暂存 blob 停在旧基底;之后任何 --staged 或 WHOLE_AMBIGUOUS 提示下改用 --staged 的提交,都会把旧 blob 当作意图提交,等于回退。commit-own.cx:1616 的拒绝文案本身就在引导人用 --staged。
(4) devsvc git status(devsvc-core.cx:2602)已有 orphan 判定,但依据只有写窗口锁;自动锁 TTL 120 秒(2552 行),所以脏路径几乎全部判 orphan,信号无效(推断,未实跑)。
(5) 推送:push-replay.cx:761-770 在 owns() 失败时只打一个 commit= 加一句提示;devsvc git push 已支持 --range(devsvc-core.cx:3941),能力在,缺的是"拒绝时把可代推集与现成命令打出来"。当前 origin/main..main 为 2 个提交。
(6) 钩子:.git/hooks 下有 pre-commit、commit-msg、pre-push;仓内 tools/hooks/ 只有 pre-push,pre-commit 薄入口不在版本控制里(一行 exec precommit.cx)。

## 2 结构性原因

(1) 一份工作树六个写者是所有者定的规则(工具在 main 上干),不翻。可翻的是:索引不必共享。工作树冲突有文件粒度与行键粒度(--keyed)两级隔离,索引没有任何隔离,却承载"提交意图"这种最私有的状态。
(2) 归属信息只在提交那一刻由人手工给出(--whole 路径、--keyed 键),之前没有任何记录,所以"这条脏路径是谁的"只能靠问。
(3) 提交与推送解耦:提交落本地 main 后不推,后来者的推送范围里必然含他人提交,unowned-pending 是这个解耦的必然产物,不是偶发。
(4) 提交门预览读共享工作区而不是"HEAD 加本人路径"的私有树,他人未提交的表行就能把自己的预览染红(推断:未核今天所用的预览命令;commit-own --dry-run 是在私有根上跑门的,见 commit-own.cx:32-38)。

## 3 裁定

### (2) 暂存区:立"共享索引恒等于 HEAD"不变式(先做,最小一刀)

立场:采纳"共享检出禁止暂存",但实现不是靠预提交钩子识别"非 devsvc 的暂存"。钩子在 git add 时不触发,到 git commit 才触发,拦不住污染的产生,只能拦住污染被提交。做法分四件:
(a) commit-own --staged 在非合并态(无 MERGE_HEAD)一律拒绝,文案指向 --whole 或 --keyed;1616 行 WHOLE_AMBIGUOUS 的提示同步改掉。索引恒等于 HEAD 后 WHOLE_AMBIGUOUS 本身不再出现。
(b) devsvc git add 在共享检出主工作树、非合并态拒绝;新文件不需要预先 add,--whole 对未跟踪文件直接入私有索引(未核:--whole 对未跟踪路径是否已支持,--new-tool 存在说明有此路径)。
(c) 落 R13x 的 devsvc git unstage(D1 报告已核接口无冲突),并让 commit-own 收尾补 cacheinfo 保持不变。
(d) 守门两层:pre-commit 钩子首行判 DEVSVC_COMMIT 令牌环境变量,缺则拒(挡裸 git commit);devsvc git status 与 chain-ready 加一行 INDEX staged=N,N 非零即红,文案给 unstage 命令。

绕过面:--no-verify、git commit-tree 加 update-ref、直接 git add 后不提交。前两者由推送侧兜底:push-replay 的归属判定依赖提交回执,裸提交没有回执,推不出去。第三者不再有害,因为没有任何入口读共享索引,且 status 立刻报红。
代价:合并流程要保留例外(MERGE_HEAD 存在且持 git-land 锁);个别人习惯的"先 add 一半再改"失效,7 条 MM 路径就是这种用法,迁移时其暂存 blob 要先备份。
回退:迁移前 git write-tree 把当前索引固化成树对象并记 SHA(只写对象库,不动索引与工作区),撤销时可按树恢复任一路径的暂存 blob;代码回退为还原 (a)(b) 两处拒绝。
可证伪:落地后一周,若仍出现"提交内容不等于提交时工作区或键选行"的事故,本裁定错;或合并流程因例外条款卡住两次以上,需重设计例外。
收益估计(推断,对折):今天四起里暂存回退一起直接消除,WHOLE_AMBIGUOUS 类手工判断消除;每日省集成者约 20 到 40 分钟排查。

### (1) 归属登记:做被动登记,不做改前强制租约

立场:否决"改文件前必须经 devsvc 声明"作为强制门。理由:窗口用 Edit/Write 和 .cx 工具改文件,声明是额外一步,漏声明只会制造假孤儿,和现在锁判定全 orphan 是同一种噪声。改为三个来源自动得到归属,按优先级:
(a) 触碰账本:宿主钩子(guard-hook.cx 已是 PreToolUse 入口,未核其是否已挂 Edit/Write)在 Edit/Write 时向 devsvc 记一行 owner、path、时刻;devsvc f write/edit 同记。零声明成本。
(b) 显式声明:devsvc claim PATH... --ttl 秒(默认 4 小时,上限 24 小时,过期自退),给经 Bash 跑 .cx 工具批量改写的场景用。
(c) 写窗口锁:沿用,只作强信号。
devsvc git status 每条脏路径输出 owner、来源、最后触碰距今、文件 mtime 距今、类别。类别三种:own(单一活跃属主)、shared-table(batches.tsv、coverage.tsv、prd_mempalace.md 等多人表,按行键归属,不判孤儿)、orphan(无触碰记录或属主会话已死且租约过期,且 mtime 超过 2 小时)。orphan 数进主调度定时器摘要,不自动处置:自动还原他人路径违反铁律。
代价:账本是 devsvc 新状态,要守"常驻有截止、写共享表须持锁";钩子多一次本地请求(热启动约 7 毫秒量级,推断)。
风险:经 Bash 的改动没有触碰记录,首周孤儿误报率会高。可证伪:首周 orphan 误报率(被判 orphan 但 24 小时内被原窗口提交)超过三成,则被动来源不够,需要把 claim 接进 refactor.cx 与 edit-span.cx。
这件排在暂存不变式之后:它只让事故更早被看见,不消除事故。

### (3) 推送:拒绝时给出可代推集,回执齐全则自动带,只对无回执提交要集成者确认

立场:比"集成者一键确认"再进一步。经 commit-own 落库的提交都有回执,过的是同一道门,后来者按首父序带着推不构成越权;需要人确认的只有无回执提交(裸提交、旧版合并)。改法:
(a) push-replay 在 unowned-pending 时改为输出整张表:按 git rev-list --first-parent --reverse REMOTE..HEAD 逐行 CARRY 序号、完整 SHA、回执属主或 NO_RECEIPT、主题;末行给 CARRY_SET n=、tip=、digest=(有序 SHA 列表的摘要)和一条可直接执行的命令。
(b) devsvc git push --carry DIGEST:只许首父前缀整段(不许挑拣),执行时重算摘要,不等即拒(防列出到确认之间有人又提交);全部有回执时任何窗口可用,含 NO_RECEIPT 时要求 UNISACC_INTEGRATOR。冻结门照旧先判。
(c) 下一步(延后项):devsvc git commit 成功后默认随即推送(冻结或网络失败时留本地并记事件),让待推集常态下只含自己的提交,从源头消掉这一类。
风险:自动带推把他人"想先攒着不推"的提交推出去。依据现行规则"检查一过立即提交推送",不存在合法的攒着不推,故接受。可证伪:若一周内出现"被带推的提交本应撤回"的情况一次,退回到全部需集成者确认。回退:去掉 --carry 入口,拒绝文案退回单行。

### 提交门预览被他人未提交行染红

裁定:开发提交门预览统一走 commit-own --dry-run(私有根为 HEAD 加本人路径与键选行),禁止对共享工作区直接跑门作预览。此项为推断,未核今天报红的具体命令;最便宜的验证是在他人有未提交 precommit-gates.tsv 行时对同一补丁各跑一次,对比结论。

## 4 切片

S0 今天,集成者,约 20 分钟(前置:R13x unstage 的 D5/D3 落库)
- 动作:git write-tree 记下当前索引树 SHA 写进记忆宫殿;对 25 条暂存路径逐属主确认后 devsvc git unstage;7 条 MM 路径先让属主决定以工作区为准。
- 验收:git diff --cached --name-only 输出 0 行;unstage 前后对 31 条路径的工作区内容摘要逐条相等;原 A 状态的 7 个新文件变为未跟踪且文件仍在。

S1 最小一刀,流程窗口,半天
- 改:tools/commit-own.cx(--staged 非合并态拒绝、1616 行文案)、tools/devsvc-core.cx(git_add 主工作树非合并态拒绝、git_status 加 INDEX 行)、tools/hooks/pre-commit(新增入库的薄入口,判令牌)、tools/check-commit-own.cx、devsvc-core 的 check、tools/chain-ready.cx 一条前置。
- 验收:check-commit-own 新增三例先红后绿:暂存旧 blob 加 --whole 提交,提交内容等于工作区;--staged 非合并态 rc 2 且未提交;合并态 --merge-continue 仍过。裸 git commit 在夹具仓被钩子拒。devsvc git status 在有暂存时末行 INDEX staged 大于 0 且 rc 非零。按宫殿第 60 行规则,须在共享检出同款条件下真实提交一次成功,并交 gatekeeper 复核。

S2 推送可代推集,流程窗口,半天
- 改:tools/push-replay.cx(CARRY 表与摘要)、tools/devsvc-core.cx(push --carry)、tools/check-push-replay.cx。
- 验收:夹具里造两条他人有回执提交加一条本人提交,拒绝输出 3 行 CARRY 与摘要;--carry 摘要正确则推成功;列出后再加一条提交则摘要不符被拒;含 NO_RECEIPT 且非集成者被拒。

S3 被动归属与孤儿,devsvc 维护窗口,一天
- 改:tools/devsvc-core.cx(touch、claim、status 输出)、tools/guard-hook.cx(Edit/Write 记触碰)、配置表列 shared-table 路径、对应 check。
- 验收:Edit 一个文件后 status 该行 owner 为本会话、来源 touch;claim 过期后自退并有事件;属主进程已死且 mtime 超 2 小时的夹具路径判 orphan;batches.tsv 判 shared-table 不判 orphan。

砍掉:改前强制租约门;孤儿自动还原或自动提交;按路径给多人表登记属主。
延后:提交即推送(S2 稳定一周后);每窗口独立工作树做工具开发(与所有者现行规则相悖,仅当 S1 到 S3 落地后一周内工作区级冲突仍每天出现才重提)。

## 5 我最可能错在哪里

(1) 旧基底回退的机理是推断。若实际是有人裸 git commit 整批暂存而非 --staged,S1 的钩子令牌仍挡得住,但 (a) 的优先级应让位给 (d)。最便宜的证伪:查今天事故提交的回执与 push-audit、reflog,看它是否经 commit-own。
(2) --staged 可能有我没看到的正当消费者(发布路径、register)。证伪:对 tools/ 全量查 --staged 调用点,十分钟。若有,改为"--staged 须同时给出预期 blob 摘要"。
(3) 自动带推的前提是"回执等于过门"。若回执可在未过门时产生(未核 owns() 的回执来源),(b) 要收紧为全部需集成者。证伪:读 push-replay.cx 的 owns() 与回执写入处。
(4) 被动触碰账本对经 Bash 的 .cx 批改盲区可能大到让 orphan 不可用。证伪:S3 上线前先只读统计一天,用现有事件流估算有多少脏路径拿不到触碰来源。
