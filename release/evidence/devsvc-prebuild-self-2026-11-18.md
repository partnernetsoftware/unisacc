# devsvc 固定版本构造复验

Linux 本机，同一私有输入提交 `712aada3c45c817b18862cb19f438568ba10f9d5`，三个客户端执行 `config`。旧启动器取自 `febe070d95`。两轮分别使用空的 DV_PIN_DIR，轮内共享目录。用单调时钟从启动首个进程计到收齐最后一个退出状态。表内是单轮墙钟。

| 场景 | 调用方收据原文 | 实际启动固定版本编译器次数 |
|---|---|---:|
| 改前，三路冷调用 | `MEASURE mode=three callers=3 failed=0 wall_ms=29521` | 3 |
| 改后，三路冷调用 | `MEASURE mode=three callers=3 failed=0 wall_ms=27881` | 1 |
| 改后，同 HEAD 再调用 | `MEASURE mode=one callers=1 failed=0 wall_ms=21` | 0 |
| 预热构造者被杀 | `MEASURE mode=crash callers=1 failed=0 wall_ms=30049` | 2 |
| 预热构造者暂停且持续持锁 | `MEASURE mode=stuck callers=1 failed=0 wall_ms=58318` | 2 |

计数用 `strace -f --seccomp-bpf -e trace=execve -s 256` 记录调用方及后代，取实际启动 `sh PRODUCT /tmp/dv-pin-PID/tools/devsvc-core.cx -o .../devsvc.tmpPID` 的次数。冷、热两轮只执行正常调用。两项故障注入在真实 prebuild-self 持有写锁且 build.log 出现后暂停构造者，再启动等待者。崩溃项在 250 毫秒后杀构造者，超时项让它保持暂停直至等待者退出。两项均把等待者 stdout 与同目录后续正常 config 逐字节 cmp，均 rc=0。

服务快进合入另一个私有 HEAD 后，后台实得以下材料回执。

```text
PREPARED	key=1bf3a2e7edd2cfd9a644	commit=5b54c6f2e181f1b139bb3db9b401374b89eef997
```
