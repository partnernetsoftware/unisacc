# 核心层与扩展应用层团队编排(主调度自主编排,所有者 2026-11-20 要求)

状态:骨架,fable 矩阵报告回来后补全(需求单格式细则与每周节拍待补)。相关:plans/reviews/2026-11-20-layer-lanes.md、plans/claude-md-layer-lanes-draft.md。

## (1) 三组

| 组 | 范围 | 成员 | 车道 |
|---|---|---|---|
| 核心层组 | 编译器、核心层、内置 libc 与 POSIX、cx 语言扩展 | cdx、agyneon | 版本列车 v0.0.NN,重车道:整链加预跑加 CI |
| 边界与工具组 | 身份口径统一 lane-inputs、promote 按 A 键复用、包门工具、devsvc 与提交门、T1 T2 T3 | agy-flow,opus 夹具,cc 集成 | main(工具线) |
| 扩展与应用层组 | ext 包、tools/cxlib、apps、ujs、sandbox-cx | ccapi、cdx2、agy-cage | main 主干式轻车道,待包门工具就位生效 |

## (2) 当前派工快照(2026-11-20 16:00)

- cdx:D2 余两例。
- agyneon:W2 默认开关判定后接 lane-inputs。
- agy-flow:T2 v4 与 affected 残留清理后 T3。
- L1:在 v0.0.86-l1 已提交 0d6fc5aa95,待独立复核。
- fable:在出核心层接口面与扩展应用层需求矩阵(报告 core-ext-matrix.md),回来后补需求单格式与每周节拍进本文件。

## (3) 流程

- 扩展层向核心层提需求用需求单,字段:使用者、缺口、不满足的后果、建议接口、验收。
- 核心层接口变更前列出受影响使用者。

## (4) 规则

- 一人一个工作树。
- 一个候选只含一类大改。
- 派工后确认送达。
- 任务板与督导定时器每 15 分钟。
- 未验证问题不派落库与改配置。

## 待补(fable 矩阵报告回来后)

- 需求单格式细则。
- 每周节拍。
