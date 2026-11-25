# 0.1.x 路线图立项页(107 之后的全部顺延项;2026-11-25 ccogweco)

前提(fable 裁定,cc 转述):107 之后**直接 0.1.0 封版**;0.0.108、0.0.109、0.0.125 与 126 的余项顺延到 0.1.x。
本轮不给 107 加任何件。分类沿用"一版一类大改";工具线与测试材料不占版本车,单独成批落 main。
依据:research/agy-h/car2-108-candidates.md(顺延十项表)、本窗口 prerun-speedup-0.1.x.md、gate5-margin.md、
plans/v0.0.109.md、plans/v0.0.125.md、plans/v0.0.126.md;fable 裁定页与"四车 9.7 小时账"原页尚未在仓内,
本页引用其结论为裁定依据,入口待补。

## 1 为何 109 与 125 降级到 0.1.x

- 0.0.109(原 0.0.102 重排)范围:前端 B2 硬上限事实表化——新建 `exec/facts/k2-caps.tsv`(17 个上限)、
  独立生成器 `tools/k2-caps-gen.cx` 与 `tools/cap-probe.cx` 读表驱动,前置 107 的 B1(plans/v0.0.109.md:1-20)。
- 0.0.125 范围:进程生命周期——reaper 与 proc-lifecycle 检查、/proc 不可证时只报 cleanup unconfirmed 不发信号;
  必做项含变异门超时杀整个进程组并核实退出(plans/v0.0.125.md:1-20);U33 随此车。
- 降级依据:fable 裁定"107 后直接 0.1.0",加上四车 9.7 小时账不足以再塞两个产品车;两者都触 SID,
  不能在封版车上搭车。证据入口:plans/v0.0.109.md、plans/v0.0.125.md、prd_mempalace 的 0.1.0 边界条;
  裁定原页待补。

## 2 批次划分(产品车)

| 批次 | 类别(一版一类) | 内容 | 依赖 | 触 SID |
|---|---|---|---|---|
| 0.1.1 | 诊断与拒绝文案 | 0.0.108 原范围:c15 未定义函数前移、c16 no file 候选路径、c17 does not export 带 use 行、-o 误覆盖源文件警告、diag-contract 基线消减;103 新判据若夹具就绪随车 | 无(纯前端与诊断表);需 derive | 是 |
| 0.1.2 | 前端上限事实表 | 0.0.109 B2:k2-caps.tsv 17 上限、k2-caps-gen 生成常量、cap-probe 读表;17 上限全绿负控 | 107 的 B1 已落;需要 0.1.1 之外的独立窗口 | 是 |
| 0.1.3 | 进程生命周期 | 0.0.125:reaper/proc-lifecycle、cleanup unconfirmed、变异门组杀与核实;U33 收口 | 0.0.107 追加集的 chain-step 修复已作同类前置 | 是 |
| 0.1.4 | 转发桩余项 | 0.0.126 余项:宿主侧容量事实源统一后的产品级验证、链接层上限与 CHECK_MAXU/UNIT_BOUNDARY 全链 | 107 的 F1 已落;需新候选 | 是 |

顺序:0.1.1 -> 0.1.2 -> 0.1.3 -> 0.1.4;两车之间允许插工具线批,但不混车。

## 3 工具线与测试批(不占版本车,落 main 或 0.1.x)

| 批 | 内容 | 依赖 | 触 SID | 建议时点 |
|---|---|---|---|---|
| T1 | #8 v4 材料宏检查(check-tool-material-macro-v4.patch,3394b077,余量 36.8 秒) | 先消 affected.cx:1203 与 devsvc-core.cx:538 的字面,否则收益减半 | 否 | 0.1.0 后第一波 |
| T2 | #13 r39e(runcache-units 的 sh 启动差异夹具)+ 宿主遮蔽三批(chain-precheck、source-bom、realprog 支持显式 CC) | 需补三批补丁与负控;PATH 遮蔽扫描清单已有 | 否 | T1 同波 |
| T3 | #15 release body 写入与回读校验(release-body-from-notes,本地 28.627 秒) | 与 #14/#16 同写域 tools/release-exec.cx,按序合并 | 否 | 0.1.0 后 |
| T4 | 第 16 份 CI run_attempt 固定与按 attempt 封存(ci-RUN-attempt-N.json/.log) | 先实测 gh 2.45.0 的 jobs/报告继承结构 | 否 | T3 之后 |
| T5 | 第 17 份 tlib bump 内容漂移硬阻断(tlib-frozen 升为拒绝) | 107 的 #1 list-cap 已入;须先清 build/tlib-packages.tsv 旧漂移 | 否 | T4 之后 |
| T6 | prerun-watch v2 与批次登记(tools/prerun-watch.cx + check) | 解冻后合入并 register | 否 | 随时 |
| T7 | 预跑提速 S1-S4 与结构点(红停不等人、冷预热、selector 测量、重窗先行、组屏障/共享判据/前置串行) | 见 prerun-speedup-0.1.x.md;S1/S2 先做 | 否 | 0.1.x 期间分步 |
| T8 | 门 5 稳定办法入手册(两次 --prepare 预热)与 rollback 写 INDEX(随 #14 v2) | 已有页与补丁 | 否 | 随时 |
| T9 | read 前预热门后续(pkg-gap #98 跨路径不命中、异头 table-index 重生) | 优先级低于 T4/T5 | 否 | 0.1.x 后段 |
| T10 | prerun-inputs 的 PRERUN_CMD 带 --normal-jobs 8,而 prerun-drive.cx:3067 不接受该参数 | 修参数或打印,属工具一致性 | 否 | 0.1.x 工具线(随 T6) |
| T11 | 预跑 priority 与 W0000 轻载锁(置顶批不得占首窗冷窗位;计划生成后核 W0000 批名与 21 个置顶 id 落位) | 第二轮 module-units-forward 占 W0000 两次 rc124;cdx 的 seal-priority-0.1.0.md 已有验收命令 | 否 | 0.1.x 工具线(随 T7) |
| T12 | prerun-watch v3 消除 --resume 期间读数陈旧(兼容读取 prerun-daemon.out 或约定 --resume 实时追加 supervise.log) | 0.0.107 resume-r1 实测 prerun-watch:410-413 只读 supervise.log 滞后于 prerun-daemon.out;详见 research/agy-h/prerun-watch-v3-note.md(不进 107,值守期间用 done.WNNNN + prerun-daemon.out) | 否 | 0.1.x 工具线(随 T6) |

## 4 依赖与风险

- 所有 T 项不触 SOURCES_SID,但多数改 `tools/release-exec.cx`、`tools/precommit.cx`、`tools/affected.cx` 等门工具;
  同写域必须串行,每件带 check 与负控,按"一车一类"不塞进产品车。
- 0.1.2 依赖 B1 与 0.1.1 的派生表稳定;0.1.3 与 0.1.4 各需新候选整链。
- #17 与 #16 都动发布门,先 T4 再 T5;否则两次改同一段 fail-closed 逻辑。
- 预跑提速 S1/S2 属值守与材料,不改调度;S4 与结构(a) 改调度,需单独回归与负控。

## 5 未定与待补

- fable 裁定原页与四车 9.7 小时账原文不在仓内,本页引用结论;落库时补证据入口。
- T1 的 36.8 秒余量与 3394b077 补丁身份取自 agy-h 顺延表;落地前按实际文件重算 sha。
- T3/T4/T5 的分钟数为各页实测或估算,未在本页重测。
