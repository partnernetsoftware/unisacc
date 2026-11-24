# 挂死 unisacc 测试进程终止留痕与根因分析(2026-11-24)

采样与执行时刻: 2026-11-24 18:22-18:24 +08:00。
处置范围:仅终止已确认挂死的 `check-l0-capture`(3 组共 10 个)与 `check-cx-sys-net`(1 组共 2 个)所属进程组,共 12 个进程;未动 `http.server`、`revise-oracle` 及任何目录或源码。

## 1. 终止前核验表(先写留痕再发信号)

核验条件: `etimes > 7200` 且 `%CPU < 1.0`,命令属 `check-l0-capture` 或 `check-cx-sys-net` 进程树。

| 组别 | PID | PPID(父进程) | PGID | 年龄(etimes) | %CPU | STAT | WCHAN | cwd | 完整命令行 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| L0-1 | 2098657 | 1 | 2098657 | 108395s (30.1h) | 0.0 | Ss | `do_wait` | `/tmp/mg96/out/slot-0 (deleted)` | `sh -c sh ./unisacc.com tools/check-l0-capture.cx` |
| L0-1 | 2098658 | 2098657 | 2098657 | 108395s (30.1h) | 0.0 | S | `do_poll.constprop.0` | `/tmp/mg96/out/slot-0 (deleted)` | `tools/check-l0-capture.cx` |
| L0-1 | 2099254 | 2098658 | 2099254 | 108390s (30.1h) | 0.0 | Z | `-` | `N/A (zombie)` | `[sh] <defunct>` |
| L0-2 | 3028004 | 1 | 3028004 | 101583s (28.2h) | 0.0 | Ss | `do_wait` | `/tmp/mg96/out/slot-1 (deleted)` | `sh -c sh ./unisacc.com tools/check-l0-capture.cx` |
| L0-2 | 3028005 | 3028004 | 3028004 | 101583s (28.2h) | 0.0 | S | `do_poll.constprop.0` | `/tmp/mg96/out/slot-1 (deleted)` | `tools/check-l0-capture.cx` |
| L0-2 | 3028808 | 3028005 | 3028808 | 101578s (28.2h) | 0.0 | Z | `-` | `N/A (zombie)` | `[sh] <defunct>` |
| L0-3 | 3254849 | 1 | 3254849 | 100619s (28.0h) | 0.0 | Ss | `do_wait` | `/tmp/mg96/out/slot-0 (deleted)` | `sh -c sh ./unisacc.com tools/check-l0-capture.cx` |
| L0-3 | 3254853 | 3254849 | 3254849 | 100619s (28.0h) | 0.0 | S | `do_poll.constprop.0` | `/tmp/mg96/out/slot-0 (deleted)` | `tools/check-l0-capture.cx` |
| L0-3 | 3255368 | 3254853 | 3255368 | 100615s (28.0h) | 0.0 | S | `do_wait` | `/tmp/mg96/out/slot-0 (deleted)` | `sh -c head -c 262144 /dev/zero >&2; printf 'out\n'` |
| L0-3 | 3255369 | 3255368 | 3255368 | 100615s (28.0h) | 0.0 | S | `pipe_write` | `/tmp/mg96/out/slot-0 (deleted)` | `head -c 262144 /dev/zero` |
| NET-1 | 2417764 | 1 | 2417762 | 14124s (3.9h) | 0.0 | SN | `do_poll.constprop.0` | `~/.claude/jobs/7f46e032/tmp/H6/w (deleted)` | `script -qec stty rows 37 cols 101 && sh ./unisacc.com -run tools/check-cx-sys-net.cx -- child tty /tmp/cx-sys-net-zhoiir/t.tty /dev/null` |
| NET-1 | 2417765 | 2417764 | 2417765 | 14124s (3.9h) | 0.0 | ZNs | `-` | `N/A (zombie)` | `[check-cx-sys-ne] <defunct>` |

## 2. 终止后核验表

操作: 按 PGID 执行 `kill -TERM -2098657 -3028004 -3254849 -3255368 -2417762`,等待 5 秒后执行 `ps -o pid,ppid,pgid,etimes,pcpu,stat,args -p 2098657,2098658,2099254,3028004,3028005,3028808,3254849,3254853,3255368,3255369,2417764,2417765`。全部 12 个进程均已退出,无需 `SIGKILL`。

| 组别 | PID | 目标 PGID | 发送信号 | 5 秒后状态 | 备注 |
| --- | --- | --- | --- | --- | --- |
| L0-1 | 2098657 | 2098657 | SIGTERM | 已退出(ps 无记录) | 会话首进程退出 |
| L0-1 | 2098658 | 2098657 | SIGTERM | 已退出(ps 无记录) | 随进程组终止 |
| L0-1 | 2099254 | 2099254 | 父进程终止后回收 | 已清除(ps 无记录) | 原 `<defunct>` 随父清理 |
| L0-2 | 3028004 | 3028004 | SIGTERM | 已退出(ps 无记录) | 会话首进程退出 |
| L0-2 | 3028005 | 3028004 | SIGTERM | 已退出(ps 无记录) | 随进程组终止 |
| L0-2 | 3028808 | 3028808 | 父进程终止后回收 | 已清除(ps 无记录) | 原 `<defunct>` 随父清理 |
| L0-3 | 3254849 | 3254849 | SIGTERM | 已退出(ps 无记录) | 会话首进程退出 |
| L0-3 | 3254853 | 3254849 | SIGTERM | 已退出(ps 无记录) | 随进程组终止 |
| L0-3 | 3255368 | 3255368 | SIGTERM | 已退出(ps 无记录) | 独立子进程组,一并按 PGID 终止 |
| L0-3 | 3255369 | 3255368 | SIGTERM | 已退出(ps 无记录) | 随子进程组终止 |
| NET-1 | 2417764 | 2417762 | SIGTERM | 已退出(ps 无记录) | `script` 退出 |
| NET-1 | 2417765 | 2417765 | 父进程终止后回收 | 已清除(ps 无记录) | 原 `<defunct>` 随父清理 |

## 3. 根因分析(只读查证)

### (1) `check-l0-capture` 三组(L0-1 / L0-2 / L0-3,共 10 个进程)

直接观测事实:
- 历史源码在 `v0.0.96` (`git show 63a4d1fe2a:tools/check-l0-capture.cx`、`63a4d1fe2a:cx/sys/proc.cx`)。三组进程的 `cwd` 均为 `/tmp/mg96/out/slot-0 (deleted)` 或 `slot-1 (deleted)`,说明均在 `v0.0.96` 变异门 `mg96` 运行期产生。
- 源码 `tools/check-l0-capture.cx` 的 C1(`head -c 262144 /dev/zero >&2; printf 'out\n'`, `deadline_ms=0`)与 C6-C8、C11 故意不设内部截止(`syp_capture_os` 仅当 `o->deadline_ms > 0` 才设超时退出,见 `cx/sys/proc.cx:777,797`),文件头自证注释(第 16-18 行)已写明:一旦 `cx/sys/proc.cx` 里的双流 `poll`/stderr 读取被变异破坏,C1/C6 就会整批卡死并依赖外层 20 秒超时兜底。
- L0-3 组直接可见死锁现场:`3255369`(`head -c 262144 /dev/zero`)因 256 KiB 超出 64 KiB 管道容量而阻塞在 `pipe_write`(`pipe:[3525266208]`),父进程 `3254853` 持读写双管阻塞在 `do_poll.constprop.0`,未消费 stderr 管道数据;L0-1 与 L0-2 组的直接子进程 `[sh] <defunct>` 已退出,但 `check-l0-capture.cx` 仍持 `fd 5->pipe:[...]` 阻塞在 `do_poll.constprop.0`,说明其内层 `syp_capture_os` 未完成 EOF/挂断退出。
- 进程关系上,三个外层包装 `sh -c sh ./unisacc.com tools/check-l0-capture.cx` 均呈 `PPID=1`、`SID=PGID`,且 L0-3 的子进程 `3255368` 又被 `syp_capture_os`(`s.pgroup == -1`)建成了独立 `PGID=3255368`。

根因判断:
- 触发卡死的原因(高度确信,L0-3 为直接实证,L0-1/L0-2 为同源推断): `mg96` 对 `cx/sys/proc.cx` 的变异破坏了 `sy_proc_capture` 的管道排空或 `poll` 退出逻辑(如不读 stderr 导致 256 KiB 管道写满死锁,或破坏了写端关闭后的退出条件),在 `deadline_ms=0` 用例中进入永久 `poll` 等待。
- 遗留成 28-30 小时孤儿进程的原因(推断): `mg96` 启动检查时使用了 `setsid`,使每个 `check-l0-capture` 处于独立会话 `SID=PGID`,内层 `syp_capture_os` 又为子进程新建了 `PGID`;当 `mg96` 超时终止或清理 `/tmp/mg96` 时,只终止了外层自身或未按整棵会话树/后代 PGID 递归清理,导致已卡死的 `setsid` 子树过继给 `PID 1` 长期滞留。

### (2) `check-cx-sys-net` 一组(NET-1,共 2 个进程)

直接观测事实:
- 源码 `tools/check-cx-sys-net.cx:201-206` 通过 `timeout 50 sh -c "script -qec 'stty rows 37 cols 101 && sh ./unisacc.com -run tools/check-cx-sys-net.cx -- child tty ...' /dev/null >/dev/null 2>&1"` 启动 PTY 子用例。
- `2417764` (`script`)的 `cwd` 为 `~/.claude/jobs/7f46e032/tmp/H6/w (deleted)`, `PPID=1`, `PGID=2417762`, `SID=2413667`;其子进程 `2417765` (`[check-cx-sys-ne] <defunct>`)已退出成为僵尸(状态 `ZNs`, 自建会话 `SID=2417765`),且 `/tmp/cx-sys-net-zhoiir` 已被删除。
- `/proc/2417764/fd` 显示 `script` 同时持有主端 `fd 3->/dev/ptmx`、从端 `fd 4->/dev/pts/44` 与 `fd 5->anon_inode:[signalfd]`; `/proc/2417764/status` 显示 `SigBlk=fffffffe7ffbfeff`、`SigPnd=0`, `WCHAN=do_poll.constprop.0`。

根因判断:
- 子用例 `2417765` 早已结束并释放(形成僵尸且临时目录已删),不存在业务死锁;真正卡住的是外层 `util-linux script(1)`(`2417764`)。
- `script` 自身持有 PTY 从端 `fd 4`,因此子进程退出不会触发 `/dev/ptmx` 的 `POLLHUP`,只能依靠 `signalfd` 收到 `SIGCHLD` 后退出 `poll` 循环并 `wait` 僵尸子进程。`2417764` 的 `PPID=1` 且 `PGID=2417762` 仅剩它一个存活进程,推断为 `H6` 会话超时/清理时向外层 `timeout 50`(`PID 2417762`)发了强杀或只终止了首进程而未杀尽 `PGID=2417762`;至于 `script` 在子进程已成僵尸且 `SigPnd=0` 的情况下仍停在 `do_poll`,推断为其此前消费 `SIGCHLD`/处理 PTY 缓冲状态时未正确跳出主 `poll` 循环(或等待已关闭终端的残留分支)。
