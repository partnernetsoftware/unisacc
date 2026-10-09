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

## 归因复核（cdx2 要求；只用三次已有 results/日志，未新跑）

| | 修前 | 修后 | 终版 |
|---|---|---|---|
| 完成/通过 | 14/13 | 58/56 | 62/60 |
| 结果作业秒合计 | 180.1 | 672.4 | 649.0 |
| jobs 墙钟 s（分母） | 230.8 | 273.9 | 266.5 |
| 已入账作业秒 / 作业段墙钟（只含 results，非并发利用率） | 0.78 | 2.45 | 2.44 |
| DEFER 尝试秒（只在日志/history，不进 results） | 96.5 | 74.3 | 0 |
| 全部尝试秒（DONE+DEFER，日志补账）/ 作业段墙钟 | 276.6 / 230.8 = 1.20 | 746.7 / 273.9 = 2.73 | 648.9 / 266.5 = 2.44 |
| 三次共同通过的 8 项耗时合计 s | 119.3 | 118.4 | 141.8 |
| DEFER 次数 | 6 | 4 | 0 |

结论更正：**单套件并不更快**（共同 8 项 119.3 / 118.4 / 141.8 s，终版偏高，三组均保留）；“14→58/62”不是四倍速度。可归因的部分：①暖身少占 1 窗（作业窗 5→6，jobs 墙钟 +15–19%）；②修前暖身连败使 warningdriver 三项在冷缓存下作为独占作业运行（Werror/Wall 各被 DEFER、Wextra 45.4 s 后 rc1），独占时其余三槽空闲；按 DONE+DEFER 日志补账，平均在途作业秒约 1.20 → 2.73/2.44（外杀未结束的作业不在其中，属下界；不是 CPU 利用率）。混杂因素：调度历史文件（TMPDIR 下 unisacc-gate-times-*）跨次累积，三次的入窗顺序与作业构成不同（同候选 ≠ 同套件集合），所以完成数之比不能当作整轮提速。整轮收益须以完整 queue 实测为准。

首次全量 queue 时同时记录调度 history 快照与暖/冷状态，再谈整轮收益。

## 首次完整全量 queue：full038c（2026-10-10；实际跑过）

候选 740007ef（source 689a91de，不重造）。**649/649 全部有结果，queue 真实出口 rc 1（有红）**；不是绿链，不作发布验收。运行分四段（同一 state）：04a5087b 起跑 → 宿主内存回收暂停 → 续跑至 389 停（rowcov 内存）→ 董秘裁迁 heavy 准入 81a4ee4a（contract digest 不变）续跑，复用 350、失效 39 → 窗 58 comboot 重装根 build.json 致 124 项失效 → queue.sh a7b059dc 修后续跑至出口。各段代码身份不同，读数按段归属；暂停与等裁时间不在下表。

| 段（stagesum，窗内墙钟） | 合计 s | 次数 | 每次约 s |
|---|---|---|---|
| window（queue.sh 每窗） | 8533.5 | 175 | 48.8 |
| release-checks | 44.1 | 176 | 0.25 |
| warmup | 43.5 | 1 | 43.5 |
| prologue（gatequeue 内） | 327.7 | 172 | 1.9 |
| jobs | 7637.6 | 172 | 44.4 |
| epilogue（含回收） | 298.1 | 172 | 1.7 |
| backup（窗外） | 46.3 | 175 | 0.26 |
| setup | 1.1 | 4 | 0.3 |

整轮窗内墙钟约 2 h 22 min（175 窗）；每窗前后置合计约 3.6 s（7.4%）。stagesum 状态 INCOMPLETE：被外杀/回收的窗没有分段 end，按 unknown 处理。

浪费（作业秒，非墙钟；按尝试类别，本轮运行树无 attempt 类别字段，按日志 DEFER/重试推定）：

| 类别 | 次数 | 作业秒 |
|---|---|---|
| DEFER（运行树 04a5087b 按入窗时记录的 tail 类别判定；是否可避免 unknown） | 175 | 4816.9 |
| solo 重试后最终超时（规则所需） | 63 | 2863.8 |
| 整窗超时（随后 solo） | 1 | 46.1 |
| 资源拒启（非提速） | 10 | 1.2 |
| 宿主 UNVERIFIED（非提速） | 7 | 1.0 |

出口表：538 PASS、57 待裁、27 BLOCKED（挂三个根：rowcov-parse2-build 25、com-luatests-build 1、rowcov-enc 系 1）、15 H1 宿主超时、6 资源 UNKNOWN、6 宿主 UNVERIFIED。下一刀按浪费排：①DEFER 4817 作业秒：入窗时的 attempt 类别判定（2df6655f）与 per-suite history 有效期（740b52a0）**已在运行树 04a5087b 生效**；未生效的只有类别持久化字段（7de2efa0，故终结果无 kind）与之后的实时内存准入（cce2de15/e5ddec42/2cedd407）、stage 对安装（a7b059dc/a3d3bb78）。这些 DEFER 是按记录类别判为 tail 的尝试，是否可避免需另核，不可全数记为可省；②rowcov/closure/stages/self 系 H1 超时；③comboot 安装幂等与回执来源前置核。

更正（2026-10-10，cdx2 复核）：上表 DEFER 原标“可避免候选”、BLOCKED 原写“两个根”、并称 2df6655f/740b52a0 未在运行树生效，三处均已改正；可避免性保留 unknown。
