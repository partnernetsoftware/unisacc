# 0.0.38 流水线首份分段墙钟基线（2026-10-10，云机 Linux x86_64 8 核；实际跑过）

口径：0.0.37 候选 740007ef（不重造），新私有 queue state，`QUEUE_WINDOWS=7`，`release/tools/stagelog.py` 采点、`stagesum.py` 汇总（39 条命令全配对 COMPLETE）。这是受控 7 窗的分段读数，**不是完整绿链**：646 项只完成 14 项（1 红），不作分钟级收益结论。

| 段 | 合计 s | 次数 | 每次约 s | 说明 |
|---|---|---|---|---|
| window（queue.sh 每窗总量） | 344.2 | 7 | 49.2 | 含下列各段 |
| release-checks | 1.0 | 7 | 0.14 | release.sh 自检到暖身前 |
| warmup | 90.4 | 2 | 45.2 | 新工作树暖身 3 两次失败 → 具名 COLD（新语义生效：不再冒充缓存已建） |
| prologue（gatequeue 内） | 10.3 | 5 | 2.1 | 指纹优化 6adefadd 后；0.0.37 实测约 3.2 |
| jobs | 230.8 | 5 | 46.2 | 作业执行 |
| epilogue（gatequeue 内） | 9.5 | 5 | 1.9 | provenance + 复指纹 |
| backup（窗外） | 0.4 | 7 | 0.06 | 状态备份 |
| 未直采（term.sh/进程启动等） | — | 7 | — | 未单独采点，记 unknown；不按残差归因 |

要点：每窗前后置合计约 4 s（优化前约 6.4 s）；首两窗被暖身占满且暖身在新工作树失败——这是下一刀（暖身 3 冷跑超 50 s 限时的根因，按 H1 处理，不放宽限时）。

## H2 宿主化真实复验（cdx，56 套件/57 次）

43 PASS（40 片参考 csmithdiff + native chain/stages/resources）、6 UNVERIFIED（rc 77：lib-stack-arm/x86/x86-hostabi、exec-bootstrap-osxarm/osxx86、lib-windows-gp）、5 FAILED（真实宿主错误，未被 77 吞：lib-windows-imports misleading-indentation 作 Werror、lib-lifecycle mincore 隐式声明、apps-real errno 未声明、proc-enum __lseek 隐式声明、lib-callable-catalog LeakSanitizer）、2 INTERRUPTED（exec-driver-language-2、exec-memory-cc-1 外层 55 s 杀窗，无 suite rc）。另保留一次 WRAPPER_FAILED：exec-native-resources 运行中 cc 改 tests/gate.sh 致 gate 读文件截断（流程违规，已认错），授权新 attempt rc0。hostcheck 对 56 选中项 missing/extra 0：47 READY / 5 UNVERIFIED / 4 UNKNOWN；READY ≠ PASS。待补：lib-windows-gp 的 cross-COFF 子义务 READY 与 native MS-ABI 子义务 77 需分别声明。

## 修后同口径复测（2026-10-10，同候选、新私有 state、7 窗；实际跑过）

| 受控 7 窗 | 修前（首份基线） | 暖身只建模型缓存后 | 加 setup/wait 采点后 |
|---|---|---|---|
| 完成套件 | 14/646 | 58/646 | 62/646 |
| 暖身 | 2 窗 90.4 s，两次失败 → COLD | 1 窗 44.6 s，建成 | 1 窗 49.0 s，建成 |
| 跑作业的窗 | 5 | 6 | 6 |
| jobs 合计 s | 230.8 | 273.9 | 266.5 |
| prologue / epilogue 每窗 s | 2.1 / 1.9 | 2.0 / 1.8 | 2.7 / 1.9 |
| setup / 互斥等待 s | 未采 | 未采 | 0.5 / 0.65 |

结论只限这 7 窗：同样墙钟内完成量 14 → 58–62（约 4 倍），来源是暖身不再在新工作树连败两窗、并且不再冒充缓存已建。全量 queue 与完整绿链未跑，不推分钟级整轮收益。term.sh 启动仍未直采，记 unknown。
