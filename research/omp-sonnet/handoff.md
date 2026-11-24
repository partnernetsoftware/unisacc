# omp-sonnet 交接(2026-11-24)

## 1. v0.0.125 分支(.claude/worktrees/v0.0.125)止损合并 main
- 状态:合并 origin/main 进行中(MERGE_HEAD 在,未提交)。冲突只有两个生成表 build/iface-hash.tsv、build/tlib-packages.tsv:取 main 版(git checkout --theirs)后用 `sh ./unisacc.com tools/derive.cx -- --write --full`(约 115 秒)重算,再 `git add -A build`。
- 第一次尝试用的是落后 5 个提交的 origin/main,precommit 门 toolsreg 红(tools-compile REFUSE batch contract or duplicate:tests/batches.tsv 里 tools-compile-188 出现两次,分支自带一行与 main 已有一行重复;同时 tests/tools-compile-sources.tsv 缺 main 新登记的 pretimes/retcheck 等四个工具)。已 `git merge --abort` 并对 213 提交落后的最新 origin/main 重合并;重算后须再查 `grep -o '^tools-compile-[0-9]*' tests/batches.tsv | sort | uniq -d` 为空,有重复删分支自带那一行。
- 提交:load<8 时 `git commit --no-edit`(门约 6 分钟,慢门是 ratchets/flow);提交后 git show --stat 核对、推送。
- 提交后 main 仍会前进,再并一次;0.0.125 不入 0.0.106 车(cc 令)。号为临时号,冻结后才取。
- 另:precommit 报 TLIB_BUMP_UNRESERVED(cx.compile 0.2.24、tools.cxlib 0.4.10)仅告警,不阻塞。
- 0.0.125 分支已提交部分:3d28fa9825 check-releaseprepare sv<0 夹具;e2stream/dvreact 消费者 sv<0 只报 cleanup unconfirmed 不发信号。未核:check-reaper 与 check-proc-lifecycle 实跑原行(验收 3)尚未交给 cc。

## 2. 0.0.125 必做项:变异门超时杀进程组(已写入 plans/v0.0.125.md 第 13 行,该文件改动未提交,随合并提交带上或单独提交)
夹具草稿:
- 在 tools/ 下建 check-mutation-gate-timeout.cx:造一个变异体(其检查命令 `sh -c 'sleep 3600 & sleep 3600'` 永不终止),门 deadline 设 3 秒,跑 tools/mutation-gate.cx 的 worker 路径(g_deadline,mutation-gate.cx:922 起 `setsid timeout -k 5`)。
- 断言:(1) 门结束后以 pgrep -g/-s 查该 setsid 会话无后代;(2) 门记该变异 rc>=124 为 Q_UNKNOWN(mutation-gate.cx:656),不算 killed;(3) 门总耗时小于 deadline+10 秒。
- 出事源头是 cdx 的 pinscan 覆盖门(check-pinscan.cx 变异进程 7400 秒不退),其源码我未找到;须定位后同样加夹具。mutation-gate.cx 自身:每项检查走 ckk_sh_t 55 秒并在 setsid 内,收尾按 `kill -TERM -- -pid`,只发 TERM 不核实退出,须改为 TERM 后等待、仍在则 KILL、再核实 /proc 无同组成员(用 ext/sysx/reaper.cx 的 rp_kill_tree_pid)。
- 归属:0.0.125(进程生命周期),下一任接手。

## 3. R28 闭包收窄(设计 /home/devali/.claude/jobs/7f46e032/tmp/R28/design.md)
S0 已得证据(只读,零仓内改动):
- E2:dump.txt 对 dumpc.txt 的每批差集:默认档独有 C 行 720380,涉及 221 个工具;频次最高 source-package-pack(560 批)、kernel-link(559)、asm-material(559)、asm-source(558)。这些工具只被 chain-step、kernel-build、kernelinputs 等点名。
- 抽 5 个批次(run-reserve、random-expr-preservation 等)与差集前 5 个批次(a12-cycle、affected-drain、affected-gitnode、affected-domain-*):其检查源码都不点名这四个工具。结论倾向"差集主要是传递膨胀,非真执行",但样本小,S2 实测 spawn 集才是判据。
- 机制:tools/affected.cx:365 lit_unlimited 全局,387 行 V_LITUSE/V_ARGUSE 无限传递,--tier commit 置 0(1927 行)。
- 闭包摘要 .git/ 节点已修(3c04bf835b,ddf86b5398 记忆宫殿);四个 main 闭包摘要一次性变更已记。
- 三个闭包(release-run、precommit、chain-ready)同为 728 节点,来源 literal-use 689。
下一步:S1 加 --lit-depth full|2 与 --closure-sha-v2(默认不变,验收见设计第 6 节);S2 薄壳记录 spawn 集;S4 翻转只在无在途链且刚公开解冻后落,0.0.106 整链期间不落。临时分析源 tools/zz-e2.cx 已删。

## 4. 未提交路径与归属
- research/omp-sonnet/handoff.md(本文件):提交于下一个我的提交。
- plans/v0.0.125.md(v0.0.125 分支?不,main 检出里):见第 2 节,必做项行,未提交,归 0.0.125。
- v0.0.125 工作树:合并中,见第 1 节。
- ~/.claude 下的作业目录是临时物,不入库。
