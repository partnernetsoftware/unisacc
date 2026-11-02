# 三方核对要点（cdx、cdx2、grk，0721 P1；cc 摘录，原文见各方短记）
- 键 A：tip f94f2e02 上 reference_key('parse2') = 3d19910f75b9932c，与 REFERENCE_KEYS 相同（MATCH），cdx 独立复算过。
- 两份日志都是 seedmemory UNKNOWN，拒收原因是缺带 flag 的 C 路线（locations、locations,warnings）的内存证据，不是键失配；没有编译，也没有生成缓存。准入实测绿**不成立**。
- 真实 child rc 记 UNKNOWN：源码里 UNKNOWN 对应 rc 2，但那是预期，不是实采；包装进程记下的 rc 0 / 12 ms 不采信。
- 两个作业是并发跑的，首红停没有生效；回执标题应读作“准入 UNKNOWN 拒收、首红停未生效”。
- flag 检查发生在键检查和冷暖缓存检查之前，所以本次准入并没有走到键或缓存那一步；补上 flag 证据后能否变绿，也推不出来。
- K5-1h run2 用同样写法、墙钟较长，这不证明那次 setsid 没有 fork。
- 驱动当时实际用的 argv/env 原件没有落盘；“无缓存、无残留进程”只是 cc 的陈述。
- 机房主任 07:27：P1 CLOSED，结论为 UNKNOWN 拒收；不追绿。
