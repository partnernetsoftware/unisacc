# P1 seedparse2 内存准入实测 schema（机房主任 07:21；首跑前落盘，之后不改；不预填 PASS）
- 锁定：tip = origin/main 全长，私有 wt /tmp/cc40-prep/k5-1h/wt，status 0；1b81c2e4 之后在审核目录、seedmemory、seedparse2check、gatedeps 下的差集数。
- 键复算：tests.seedmemory.reference_key('parse2') 的原文输出（key.txt），必须等于 REFERENCE_KEYS['parse2']=3d19910f75b9932c，否则不跑。
- 环境：NETWORK=0；TMPDIR 设为新建的空私有目录（因此 unisacc-seedparse2 缓存是冷的）；其余相关变量照实记录。
- 作业（串行，首红停；每个都放进自己的进程组，timeout 58）：seedparse2-1 = `./tests/seedparse2check.sh x locations warnings errors`；seedparse2-2 = `./tests/seedparse2check.sh locations,warnings locations,errors warnings,errors locations,warnings,errors`。命令与 tests/gate.sh 第 569–570 行相同；脚本自己会先调用 seedmemory 做准入，再 --verify-binary。
- 采集：child rc、outer mono 起止、stdout/stderr 日志及其 sha、seedmemory 的拒收原因（如有）、same/differ 计数、缓存目录 $TMPDIR/unisacc-seedparse2 下的文件列表。
- 判读：rc 0 只说明在这个宿主、这个时刻，准入路径本身能走通。rc 2（UNKNOWN）或 rc 77（已知内存不足）照实记，不改键，不改期望。
