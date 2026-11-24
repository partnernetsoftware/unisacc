# 0.0.125 变异门超时杀进程组:补丁与证据(ccogweco)

补丁: research/ccogweco/mutation-gate-timeout.patch(2 文件: tools/mutation-gate.cx、tools/check-mutation-gate-timeout.cx)。基线 v0.0.125 车头 7341b0b5,整份 git apply --check 通过。未落分支,待 gatekeeper。

## 改法

- worker 用新的 mg_run_check 起检查:sy_spawn 子进程是其进程组组长,pid 先写到 OUT/cur-slot.pid,子进程结束后删;超时由 sy_kill_tree 杀整树并回 124。
- 内层检查不再各自 setsid,继承 worker 进程组;per-check 限时仍由 mg_run_check 的 deadline 与 sy_kill_tree 负责。
- run_all 收尾:对每个 slot 先检查组(cur-slot.pid)再查 worker 组,各自 TERM -> 等待(最长 5s)-> KILL -> 等待 -> kill -0 -PGID 核实;仍有成员则打印 MG_RESIDUE pgid=.. slot=.. what=check|worker 并 die2 拒判,不把活着的组当 killed 报出去。
- 只在收尾做这些 kill;没有别的清理动作。

## 夹具 tools/check-mutation-gate-timeout.cx

私有 git 仓:check 在原文件上 rc0,被变异后进入 TERM 免疫的永久挂起(hang.sh: trap '' TERM; exec sleep 3600),门 deadline 30s、1 个变异、par 1。计时前先跑一次门命令预热 -run 编译缓存(否则冷编译约 19s 会污染计时)。
断言:(1) 无 MG126MARK 残留;(2) 门总耗时 < deadline+10(40s);(3) 门真被执行(MG_RUN ms>=15000,杜绝空跑也绿);(4) worker 组 kill -0 已空;(5) 检查子进程组 kill -0 已空。

## 实测

绿(修复版):
    PASS mg-timeout.no-marker-process-left
    PASS mg-timeout.gate-finished-within-deadline-plus-10
    PASS mg-timeout.gate-really-ran
    MG_GATE_RUN_MS 30428
    PASS mg-timeout.worker-group-gone
    PASS mg-timeout.check-group-gone
    CHECK_MG_TIMEOUT failed=0
分相: prep 0.15s, 门自身 MG_RUN 30.4s(deadline 30), 总 30.4s(预热后)。

先红(把 983 行的 HEAD 版 tools/mutation-gate.cx 换入后实跑):
    FAIL mg-timeout.no-marker-process-left
    FAIL mg-timeout.gate-finished-within-deadline-plus-10
    PASS mg-timeout.gate-really-ran   (MG_GATE_RUN_MS 30389)
    CHECK_MG_TIMEOUT failed=2

变异(把 mg_stop_group 改成立刻 return 0,即不发 TERM/KILL、不核实):
    FAIL mg-timeout.no-marker-process-left
    FAIL mg-timeout.gate-finished-within-deadline-plus-10
    CHECK_MG_TIMEOUT failed=2

## 落分支前还差

- 新工具批次登记: tests/manifests/<新批>.tsv(entry tools/check-mutation-gate-timeout.cx)+ tables-gen --write 落 batches/coverage/tools-compile,本补丁未带这三张生成表。
- 夹具目前单次约 35s(30s deadline + 收尾),批次预算需 >=40s。
- 未跑:precommit-preview 对补丁路径(冻结期禁变异门,等解冻后在私有 archive 补)。
