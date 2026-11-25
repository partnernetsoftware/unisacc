# 2026-11-24 变异门超时夹具旧实现残留进程组清理(ccogweco)

依据 G38:tools/mutation-gate.cx 旧收尾只写 `kill -0 -- -PGID`/`kill -TERM -- -PGID`,而本机 /bin/sh 是 dash,不认 `-- -PGID` 写法(kill: Illegal number: -),信号从未发出,核实永远答"组已空",夹具 group 断言假绿;早前夹具运行因此留下多组 TERM 杀不掉的 `sleep 3600`。

## 先核对(按真实 PGID,每组两个 sleep 3600,etime 7 到 13 分钟)

    pgid=2073186  pids 2073188 2073189
    pgid=2092819  pids 2092821 2092823
    pgid=2100741  pids 2100743 2100744
    pgid=2111898  pids 2111903 2111904
    pgid=2121340  pids 2121342 2121343
    pgid=2125669  pids 2125671 2125672
    pgid=2104837  pid  2104837

## 清理

先按 PGID 逐组 `kill -KILL -PGID`,复核每组 members=0(全部 0)。只做 kill,未做别的清理动作。

## 修法归属

(1) 去 `--` 或走原生 kill;(3) 发信号前核 /proc/PID/stat 第 20 项 starttime;(6) 头注更正——已在 v0.0.125 工作树本地改并编译 rc0,未提交;(2) 会话/槽 cwd 回收、(4) 读到 EOF 防 SIGPIPE 并补时间上限、(5) 夹具 deadline 8s 与阈值、(7) 活组探测断言与删核实步变异,等冻结结束再做并重出补丁送 gatekeeper。
