# unisacc 0.0.88 发布说明

整体裁决 NOT_QUALIFIED

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交 | 3038dc3e73afd983a41f94dbd11997e6c01f9888 |
| main 冻结合并提交 | e7dabf518d5a073ecf6f6e7fd9d1cccfd3fd5f28 |
| 发布 APE SHA256(unisacc.com) | 0e028e7de8dc1c4a5e6a5b3120e137daf8b2d3a8b3068d5af7f971a3c3a46854 |
| 源快照 SID | e07fa5731199613a936e15c663368d0c4696491de092e0ced69e086f105f32f2 |
| pkg.bin | 87b93943f2e43f78d06bb5f657120d23a2b3096fda9114c846282e9962b6c019 |
| slice-win-arm64 | a7cbaef021601bf591eb901b2455e1026332b73f2e928df652bb9f5e18ba92fc |
| SHA256SUMS | d0aec316c2194f84cffaad987888a5d71f60ac1e2bd08ac4f938d5a7f2722a12 |

## 严格证据

442 批:PASS 440、FAIL 0、UNKNOWN 2、INVALID 0

严格证据由预跑 prerun088-c2(157 窗,source=89debcb544 唯一一致)合并为 strict-final.tsv,候选同本版 APE(0e028e7d):全集 442 批中 440 批 PASS、0 批 FAIL、2 批 UNKNOWN(已知 no-producer,台账 R56)。同候选六格原生 CI(ci-early run_id=44893924553)6/6 全 PASS。首候选 00cbb826 因 cx/runtime/runexec.cx 手写 56 字节 ExStat 栈溢出作废,本候选 89debcb544 已改用 sys.fs sy_stat_t 并新增全仓一致性检查 sy-stat-struct(PASS)。各批按末行折叠计数。

## 产品改动

- cx-L0 核心库面小件包(Y1-Y8):调用号入事实表 exec/facts/sys-syscalls.tsv 经 target-gen 生成 cx/sys/sysnums.cx 并清零 cx/sys/posix.cx 四处字面号;新增 sy_gid/sy_egid、sy_getrlimit/sy_setrlimit、sy_wait_any/sy_wait_rusage、sy_stat_t 纳秒与设备字段(88 字节)、sy_syscall6(仅支持 nr 加 5 参数,第六参不支持)、sy_random 熵源与 POSIX 有界子进程独立进程组超时按组 SIGKILL。三目标现状:lnx(x86_64/arm64 实测);osx(sy_syscall6/sy_random impl,原生格 CI 覆盖);win(sy_syscall6/sy_random refuse -ENOSYS)。
- cx 层次整理与 cxroot 整名解析:cx/sys 迁入 fd/guard/poll/tty/net 实体并留 ext/sysx 回指、退役 ext.memx(arena 迁入 cx/mem)、ext/sysx/process.cx 去 dock、新增 ext-dock 门禁与 core-libc-map 映射表;cxroot.tsv 支持 R 行 PREFIX 整名解析到 DIR.cx(b5012ba114)。
- 正确性根修与测试侧同步:cx/runtime/runexec.cx 改用 sys.fs sy_stat_t 消除栈溢出并新增一致性检查 tools/check-sy-stat-struct.cx;同步 tools/check-lane-inputs-gen.cx 探针期望(ext/sysx/linuxns.cx 移出 B 类不误触,新增 cx/sys/posix.cx A 类探针)。

## 已知缺口

- cx-booter-ape 与 tools-default-regression 未覆盖(UNKNOWN,no-producer:booter-raw-elf、old-checker 材料没有生产者,台账 R56),不记 PASS。
- main 冻结合并提交 e7dabf518d 的源 SID(3d434244...)与本候选 rev 89debcb544 的源 SID(e07fa573...)差 2 个文件:exec/facts/north-star-facts.tsv(main 指标事实表)与 ext/textx/json.cx(经遗留软链 cx/text/json.cx 进入 v2 SID,属 0.0.91 身份与构建边界类消除的别名问题);本发布按同源原则绑定 89debcb544,不重建整链。
- sy_syscall6 仅支持调用号加 5 个参数,第六参不支持。
- 本机严格预跑只在 lnx/x86_64;其余五目标只由 CI 冒烟子集覆盖。全套 s9 未跑,第 4 步(全量覆盖)为 UNKNOWN。
