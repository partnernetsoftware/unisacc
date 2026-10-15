# c43 看门狗 402 秒根因（只读，2026-10-15）

**结论**：未能定位。已排除 suspend、已排除套件内 bound 失效的两条直接解释。402 秒这一行的原始时间线没有留下足够的采样，因此根因仍未证实。

## 1. 事实

- 记录（`/tmp/commitgate.9srhg0`，08:40）：`script-inventory rc=1 402s :: ORPHAN tests/stagecontractcheck.cx`；gate 墙钟 403s。
- 该行由 `tests/gate.sh` 的作业运行器产生：`out=$(python3 "$R/tests/bound.py" 60 env "$@" 2>&1)`，并在 `( … ) &` 子 shell 内计时（`t0` 在子 shell 开始后取）。作业运行器对所有套件路径相同，没有绕过分支（已读 `tests/gate.sh:120–160` 的分支）。
- 作业的命令是 `python3 ./tests/inventory.py --check`（`tests/gate.sh:606`）。`inventory.py` 不调用子进程，只读文件并做正则匹配。
- 它单独运行时：`python3 tests/bound.py 60 python3 tests/inventory.py --check` 墙钟 38 秒，rc=0，`orphan 0`（当时 S 文件已移出）。

## 2. 排除项

- **suspend**：`/sys/power/suspend_stats/success` 与 `fail` 均为 0；`/proc/uptime`（含 suspend）与 `time.monotonic()`（不含）差值为 -0.01 秒。本机自开机以来未休眠，墙钟与单调钟的差不能解释 402 秒。
- **bound 对睡眠子进程失效**：在 HEAD 干净导出中加入假作业 `sleep 70`（只改了导出的 gate.sh，未动主检出），`sh tests/gate.sh --suite fake-sleep70` 的输出为 `DONE rc=142 wall=60s`。作业级 60 秒上限对一个长寿命子进程有效。

## 3. 未能解释的一点

- 记录的行是 rc=1、402 秒、正常输出 `ORPHAN`。如果 bound 在 60 秒触发，应得到 rc=142 而不是正常结束。也就是说，这次作业进程是**自己**在 402 秒时正常退出的，而包装的 bound 没有截断它。
- 单独运行只需 38 秒。要使它慢到 402 秒，只能是运行时的并发负载或 I/O 阻塞（进程进入不可中断等待，`bound.py` 的 `process.wait` 或 `$()` 等待 EOF 才会拖长）。两者都没有证据：当时的负载未采样，且 `$()` 只有在某个持有 stdout 的后代仍存活时才会等待，而 inventory 没有子进程。

## 4. 仍需的证据（不改产品或门禁，只作采样）

1. 在作业运行器里为每个作业记录 `start`、`bound 返回`、`bound 进程退出码` 三个时间戳（需要改 `tests/gate.sh`，属于登记路径，待窗口）。
2. 下次出现长作业时，抓 `ps -eo pid,stat,etime,args` 快照与 `/proc/<pid>/wchan`，区分 D 态等待与 CPU 忙。
3. 记录当时的 `uptime` 负载（当前采样只有 1 分钟均值 0.99，五分钟均值 2.25）。

## 5. 本笔不做的事

- 不改 `tests/gate.sh`、`tests/bound.py`、登记文件（登记窗在 cdx W）。
- 不把 402 秒归为 bound 失效的已证实案例；它是未解释的异常，与 356、344 秒两样本并列为“同型未证实”。

## 6. 墙钟跳变实证（2026-10-15，30 分钟采样）

方法：仓外后台每秒记录 (time.time(), time.monotonic(), /proc/uptime)，共 1743 个样本，原始数据在 /home/box/c43/clock/samples.tsv，采样器 /home/box/c43/clock/watch.py。
结果：
- 单调钟与 uptime 全程一致（跨度 1799.7 s）；墙钟跨度 2859.3 s，比单调钟多走约 1060 s。
- 相邻样本中墙钟与单调钟差值超过 0.5 s 的区间共 3 处：
  - 第 89.7 s：墙钟 +863.8 s，单调钟 +22.6 s（采样器与 uptime 同步停顿约 22.6 s），墙钟比单调钟多走 841.2 s。
  - 第 422.4 s：墙钟 +110.5 s，单调钟 +17.4 s，多走 93.1 s。
  - 第 1458.3 s：墙钟 +144.3 s，单调钟 +19.2 s，多走 125.1 s。
- 每次跳变前后都有一次 17–23 s 的 VM 停顿（单调钟与 uptime 同步前进，说明是整机停顿而非时钟漂移），停顿结束后墙钟被一次性步进。
- 环境：KVM 虚拟机，clocksource kvm-clock，存在 /dev/ptp0；本机无 chronyc/timedatectl 输出，未能直接核对校时日志。

结论（证实）：本机墙钟会在 VM 停顿后被一次性步进数百秒。依赖墙钟的测量（gate.sh 作业运行器的 date +%s、gatesummary 的 time.time()、各处 time.time()）会把这类步进记成作业耗时。第 6 节之前的五个样本（356/344/402/889/447 s）若是用墙钟量得，需视为测量假象，待用单调钟重测后再下定论；“bound 没截住”的判断本身同样需要单调钟数据支持。
测量修正：tests/gate.sh 作业运行器与 tests/gatesummary.sh 的全部时长改用 CLOCK_MONOTONIC（提交 d6f86c05），墙钟仅作对照字段保留。
保留：0b7e7920 与 944b83f4 的防御性修法（stop() 与正常退出收尾的总时限）不因此撤回；它们在任一时钟下都限制清理阶段的无界等待。

## 7. 结案更正（2026-10-15）

356/344/402/889/447 五个超长样本的结论统一更正为：墙钟跳变（VM 停顿伴随 841/93/125 s 步进）造成的测量假象。bound 始终按单调钟正确工作，“bound 没截住”的判断有误（此前由 cc 提出、由 cc2 据墙钟数据转述，已在本节更正）。0b7e7920 与 944b83f4 的防御性修法保留。
