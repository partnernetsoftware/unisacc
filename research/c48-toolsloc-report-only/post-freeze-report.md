# 最终受影响批次验证

结论：toolsLOC-only 24 批和 precommit-freeze 共 25 个正式批次均实际退出 0；另正式 chain-precheck 完整 74/0。全部逐句柄 poll 终态，不复用旧 PASS。只改 `tools/check-chain-precheck.cx`，未修改控制器、precommit、harness、注册表或基线。

检查源码修正：physical 路径复制改 cx_fmt 并检查拒截断；native mutation 的变量长度复制改 mem.bytes/mm_copy，dcap 为实际 malloc(n) 容量，复制前拒绝不能表示为 long 的长度；全部 74 断言保留。默认-run 正式执行已实际编译本检查。

冻结检查源码：23774B，SHA256 `03b850b65fc9c1cfdd186421346a15a4c070af66a75e95af007ad6d94aa2ae6d`。

执行使用父冻结 harness `out/c48-x5-formal/run-batch`，各批独占 TMPDIR，外 bound55；25 批按 8+8+8+1 分组，chain-precheck 独立执行。每个正式日志均绑定产品 `f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82`。原始 stdout/stderr 留在本目录 `<batch>.log/.stderr`。stderr 的 no child-subreaper 提示保留。

真实进程状态见 `actual-session-status.tsv`（26 项含 chain）；`integrated-status.tsv` 由本目录 evidence.cx 逐项校验退出 0、唯一 BATCH_RUN、PASS/rc0/transport0 和精确候选 SHA，校验器真实 exit0。chain-precheck session80563；precommit-freeze session37722；两者都实际 poll exit0。

静态门：cxapis session27641 exit0（全仓0项超基线）；cxtrunc session83505 exit1，本次 ownpath 无红，保留其他4路径超基线：research/c48-macos-distribution/evidence/cache-inventory.cx 1、cache-migration.cx 2、cache-post-verify.cx 2，tools/check-cache-retain.cx 11。不宣称全仓静态门绿，未扩额。

`input-identity.tsv` 与 `final-identity.tsv` 实际 cmp0。身份采样覆盖检查/控制器/precommit及其配套检查、报告工具、harness、两注册表与产品；第一次采样在前两组完成后，不能当作所有执行前的自动全依赖封印。各执行输入按父声明冻结，代理未在运行中修改。

未重跑已完成的 runner-faults 对照、M8 无输入变化批次或全量测试；未提交。无在途句柄。
