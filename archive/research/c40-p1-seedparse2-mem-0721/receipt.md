# P1 seedparse2 内存准入实测回执（cc；机房主任 07:21；schema sha 0cf17ee5…）——**首红停，且驱动有错，需如实看**
## 锁定与键
- tip 为 f94f2e02e982d6ab10f20610f1b8a1ff286f4ae6，私有 wt 的 status 为 0。1b81c2e4 之后，审核目录、seedmemory、seedparse2check、gatedeps 下都没有文件改动。
- key.txt 原文：reference_key('parse2') 为 3d19910f75b9932c，等于 REFERENCE_KEYS，**MATCH**。
## 运行结果（以日志原文为准）
- out-1.log（seedparse2-1）：`UNKNOWN: required=memory observed=unknown missing=memory-evidence reason=unmeasured C flags: locations`
- out-2.log（seedparse2-2）：`UNKNOWN: required=memory observed=unknown missing=memory-evidence reason=unmeasured C flags: locations,warnings`
- 两次都**被 seedmemory 准入拒收**，结果是 UNKNOWN。按 seedmemory 文档，UNKNOWN 对应 rc 2。拒收原因是**没有测过带 flag 的 C 路线（locations / locations,warnings）的内存证据**，不是 “generator inputs differ”，也就是说**键 A 本身没有失配**。seed/gen.c 没有编译，Python 参考没有生成，私有 TMPDIR 下也没有生成 unisacc-seedparse2 缓存目录。
- **这就是首红**：准入路径没能走通，所以“准入实测绿”**不成立**。
## 驱动错误（自报）
- 我用 `setsid sh -c '…; exec …'` 让每个作业单独占一个进程组。但这次外层 shell 本身就是进程组组长，setsid 因此 fork 到后台并**立即返回**。所以 times.txt 里两行“child_rc=0、wall 12 ms”记的是 setsid 包装进程的 rc，**不是**夹具的 rc。两个作业实际上是**并发**在后台跑的，不是串行，也没有触发“首红停”。
- 夹具真实的 child rc 和墙钟**没有采到，记 UNKNOWN**。日志 mtime 是 07:23:38.894 和 .910，几乎同时写入。07:24 查进程，已经没有残留。
- K5-1h 的 run2 用的是同一种写法，但那次墙钟读数是正常的（20 s 量级），说明当时 setsid 没有 fork；这一差别的成因我没有查清。
- 我没有为补齐 rc 而重跑。
## 判读
- 键 A 复算 MATCH 成立。准入路径**在这个宿主上不通过**：带 flag 的 C 路线缺内存证据，所以返回 UNKNOWN。这与 K5-1h 无关，因为拒收理由不是键失配。要变绿，需要补测带 flag 那几条路线的内存证据，这超出本刀范围，需另授。
- 没有改 REFERENCE_KEYS、gatedeps 或测试期望；没有跑门。

## 更正与限定（cdx、cdx2；只追加）
- **标题更正**：标题写的“首红停”与正文矛盾。实际上首红停**没有触发**：两个作业是并发跑的，第二个作业并没有因为第一个红了而不启动。标题应读作“准入 UNKNOWN 拒收；驱动出错，首红停未生效”。
- 真实的 child rc 记 UNKNOWN。源码里 UNKNOWN 对应 rc 2，但那只是预期，**不能**当作实际采到的 rc 2。包装进程记下的 rc 0 和 12 ms 不能采信。
- flag 拒收发生在 reference_key 检查和冷暖缓存检查**之前**（seedmemory.py 第 127 行附近）。所以本次**不能**说准入已经走过键检查或缓存检查；“键 MATCH”只来自 key.txt 里那次单独复算。也**不能**推出补齐 flag 证据后一定能变绿，其他冷路线是否同样缺证据还没有排除。
- run2 用同样写法、耗时较长，这**不能**证明那次 setsid 没有 fork。
- 运行时的参数和环境：pre-identity 记的是外层 shell 的状态（NETWORK、TMPDIR 都是 unset）。作业实际运行时，环境由 `env NETWORK=0 TMPDIR=<私有目录>` 在命令行里注入；完整的驱动 argv 和 env 原件**没有单独落盘**，只能以本 shell 历史里的命令为准，不按 schema 代替实采。
- 关于“没有缓存目录、没有残留进程”：这是我自己检查后的陈述（私有 tmp 下的 ls、07:24 的 ps），单独列出，不算独立证据。
