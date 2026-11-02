# 驱动卫生：setsid 与受控跑驱动清单（cc；机房主任 07:27）
## 教训（0721 P1）
- 0721 那次的驱动用 `setsid sh -c '…; exec …'` 给每个作业单独开一个进程组。但调用它的 shell 本身就是进程组组长，在这种情况下，util-linux 的 setsid 会 **fork 到后台并立即返回**，返回值是 0。结果 times.txt 记下的 rc 0、12 ms 都是包装进程的，两个作业实际上是**并发**跑的，首红停没有生效，夹具真实的 rc 和墙钟都丢了（记 UNKNOWN）。
- 再往前，K5-1h run2 用的是同样的写法，墙钟看起来正常（20 s 量级）。但这**不能**证明那次 setsid 没有 fork（cdx 已指出）。
## 规则（下次受控跑的驱动清单）
1. **平台**：本机是 /usr/bin/setsid，util-linux 2.41.5，Linux。下面的写法只对这个平台成立，不能直接推到其他宿主上。
2. **一律用 `setsid -w`（--wait）**：它会等子进程结束，并返回子进程的退出码。注意它只保证“等待”，**不**证明整棵进程树已被杀掉、进程组已清空。
3. **rc 要在命令后立刻取**，不经过管道。墙钟用 monotonic 记起止。墙钟短得不合理（比如编译加生成的作业只用了几十毫秒）时，先查原因，再写报告。
4. **串行执行，首红即停**：每个作业开始前先往 starts.txt 写一行；遇到红就 break。证据要能直接看出第二个作业**没有启动**：starts.txt 里没有它那一行，也没有它的 out 日志。
5. **进程组**：组号用 `ps -o pgid= -p $$` 实读，不要直接拿 `$$` 当组号。残留检查时 ps 的 rc 要另行采集，不能让它被 `| wc` 吞掉。“残留为 0”只能说明作业返回那一刻的状态；要证明 kill 分支真的有效，需要一个真正触发 TERM 的负例。
6. **负例要成套**：至少包括“作业以已知的非 0 rc 退出时，驱动记下的就是这个 rc”，以及“首红之后第二个作业零启动”。
## 0727 可选短核（只证驱动；预期仍是 UNKNOWN）
- 证据目录 /tmp/cc40-prep/seedparse2-mem-0727-driver/（driver.sh、pre-identity.txt、starts.txt、times.txt、out-seedparse2-1.log）。
- 运行时 wt 的 HEAD 是 f94f2e02，它和 29d99deb 之间只差 research。外层 shell 的 pid 和 pgid 相同（3776510），也就是说这次的调用者正是进程组组长，与 0721 的条件一致。键 A 复算结果为 MATCH。
- seedparse2-1：**child_rc=2**（这次是真实采到的子进程 rc），墙钟 79 ms，pid 和 pgid 都是 3776540（setsid 新开了一个组），作业返回时组内残留为 0。日志内容是 `UNKNOWN: … unmeasured C flags: locations`，与 0721 相同。
- 随后驱动记下 FIRST_RED_STOP；starts.txt 里只有 seedparse2-1 一行，也没有 out-seedparse2-2.log，**第二个作业没有启动**。
- 限定：这只是一次观察，**不能**泛化成“首红零启动”已在所有情形下得到证明。驱动里残留计数那一步仍然没有单独采 ps 的 rc（违反了上面第 5 条，下次要改）。TERM 分支没有触发过。
