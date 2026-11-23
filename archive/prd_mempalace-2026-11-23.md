# prd_mempalace 地图段原文(2026-11-23 归档)

以下为 2026-11-23 从 prd_mempalace.md 地图段(原第 15-71 行)整段移出的流程图与节点原文,仅作历史。现行地图已改为指针(见 prd_mempalace.md 地图段)。

当前(2026-11-23 11:50):0.0.94 已公开(11:07,v0.0.94 Latest,APE e1408291,CI 45103237968,Y2 98f9f15532,本机 Root 已升 0.0.94);所有者裁定从 0.0.97 重新出发:0.0.95、0.0.96 两号跳过,下一发布 0.0.97 = main 上已有的 e2 重扫修复(原 0.0.95,ca2f342aab)加执行核每步成本(分支 v0.0.97,须先合入 main 再重跑 check-core-run-diff);进程生命周期(分支 v0.0.96)不进 0.0.97,做完按下一空号发;0.0.98 到 0.0.102 分支保留、暂停加活。发布方式改为'先证明再冻结':合入后后台出候选整链与预跑,冻结只登记已证明 APE(冻结行与 freeze_at 不进 APE,只有 release/candidate.tsv 版本串进,已核),冻结日只核对 SID 与 APE sha 并提升。入口:build/version-seq.tsv、research/2026-11-23/promote-not-rebuild.md、research/release-gotchas.md。

```mermaid
flowchart LR
  subgraph 版本流水线
    V66["0.0.66 已公开,备份 ~/.cache/unisacc/ape-0.0.66"]
    V67["0.0.67 已公开 Latest 已装(APE a99ea71d)<br/>release/、archive/plans/release-notes-0.0.67.md"]
    V68["0.0.68 分支 v0.0.68 开发中<br/>驱动与运行缓存二期、500 ms 门<br/>prd.md 版本表"]
    V69["0.0.69 prune 原样判定前置(性能 R1)"]
    V70["0.0.70 cxmod 内存安全乙期"]
    V71["0.0.71 正规构造499/499,APE ca84同字节复现<br/>/tmp/unisacc-session-v71-rc2"]
    V72["0.0.72 已公开 Latest(冻结 F 5cdbd305b1)<br/>archive/pre080/release/notes-0.0.72.md"]
    V73["0.0.73 已公开 Latest(冻结 F 3865c7c9e0)<br/>release/evidence/0.0.73/"]
    V74["0.0.74 已公开 Latest(冻结 F 0dae4d3024)<br/>release/evidence/0.0.74/"]
    V75["0.0.75 已公开 Latest(冻结 F fe6dfb32cf)<br/>release/evidence/0.0.75/"]
    V76["0.0.76 已公开 Latest(冻结 F debb8f43f9)<br/>release/evidence/0.0.76/"]
    V77["0.0.77 cxmod 内存安全乙期一段,分支 v0.0.77<br/>archive/plans/v0.0.77.md"]
    ECO["0.0.78 至 0.0.81 已公开(0.0.81 冻结 F 8b46bf21ee,对象产出二期,NOT_QUALIFIED)<br/>release/notes-0.0.81.md、release/evidence/0.0.81/"]
    PERF["0.0.82 前端诊断二期与 ref 判据替换、0.0.83 Q-P 首次尝试、0.0.84 执行器与模型包,0.0.100 起汇编级精修(入口准则在 0.0.99 封板前满足)<br/>archive/plans-pre094/roadmap-rev3.md、plans/roadmap-0.0.83-0.0.99.md、archive/plans-pre086/v0.0.82.md、archive/plans-pre086/v0.0.83.md、plans/nn-migration-rev3.md"]
    V66 --> V67 --> V68 --> V69 --> V70 --> V71 --> V72 --> V73 --> V74 --> V75 --> V76 --> V77 --> ECO --> PERF
  end
  subgraph 发布
    REL["发布 12 步与回滚<br/>release/rollback.txt、archive 起链配方节"]
    Q["裁决恒 NOT_QUALIFIED unknown=1<br/>s9 daily/release 未跑通<br/>release/decision.txt"]
  end
  subgraph 质量门
    GATE["棘轮 13、derive 11、chain-ready 9 项<br/>tools/ratchets.cx、tools/derive.cx、tools/chain-ready.cx"]
    TEST["测试入口 tests/run.cx<br/>tests/batches.tsv、tests/coverage.tsv"]
    UV["仍未验证 19 条<br/>build/state-unverified.tsv"]
  end
  subgraph 设计与规划
    PRD["结构与路线、候选池(加速 A 至 P)<br/>prd.md"]
    CX["cx 设计与库包审视<br/>prd_cx.md 0.2 节"]
    SPEC["cx 使用规范与库包一览<br/>prd_cx_spec.md 3.4a"]
    SPEED["速度只认四工具<br/>build/speed-baseline.tsv"]
  end
  subgraph 编排
    ORCH["主调度 cc;窗口 omp-go、omp-cdx 经 bin/mux envelope;子代理至多 2"]
    WT["版本工作目录 .claude/worktrees/v0.0.NN"]
  end
  V67 --> REL --> Q
  V68 --> GATE
  GATE --> V67
  TEST --> GATE
  UV --> Q
  PRD --> V68
  CX --> V70
  SPEC --> CX
  SPEED --> V68
  ORCH --> WT --> V68
  T71["P2指定175项实跑全过,全套仍NOT_QUALIFIED<br/>/tmp/unisacc-session-v71-rc2/p2-evidence.md"]
  S71["封存复核已在 v0.0.71 修复,待整轮复验<br/>tools/bg-candidate.cx"]
  V71 --> T71 --> Q
  V71 --> S71 --> Q
    ITER["迭代速度方法学:五条元原则与收口标准<br/>prd_paradigm.md 第 9 节、prd_iteration.md"]
    OREQ["所有者需求台账:状态、证据、告知记录,未告知断点只降不升<br/>build/owner-reqs.tsv、tools/check-owner-reqs.cx"]
```
