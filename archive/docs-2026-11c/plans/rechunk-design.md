# 节点可拆维度与自动再分块 (0.0.50 G2 前置)

目标:把「相位贴 60 秒上限就手工拆」变成通用机制。每个构造图节点声明可拆维度,调度器按上一波实测墙钟自动再分块,可按检查点续跑。不改产品源。

## 1. 图列 (build/graph.tsv, 14 列)

新增第 13、14 列(位于 root_dependent 之后):

| 列 | 取值 | 含义 |
|---|---|---|
| split_dim | jobs / units / targets / stages / none | 超预算时调度器沿哪个维度切节点 |
| split_min | 整数 >= 1 | 最小粒度(一波至少处理多少项) |

当前 203 节点的分布(全部 inferred,按节点角色规则一次性迁移,没有一个是实测值):jobs 131(job:* 作业)、units 14(front0/1/2、units)、stages 15(e3、image、B、g2/g3 的 osx/win 八步相位)、targets 4(g2/g3 的 lnx 两片)、none 39。

graph-check 新增三条校验:split_dim 必须在枚举内;split_min 必须是不小于 1 的整数;budget_s 不小于 45 且 split_dim 为 none 的节点,note 必须含 `split:none <理由>`。预算小于 45 的 none 节点不要求理由。

## 2. 库 tools/cxlib/rechunk.cx

纯整数函数,不做 I/O、不读时钟;墙钟由调用方测量,检查点由调用方写。

- `rc_next(total, first, prev_count, prev_wall_ms, budget_s, min, max)` 返回下一波分块大小。
- `rc_first(first, count)` 返回续跑游标 first + count。
- `rc_tag(buf, cap, phase, first, count)` 生成 `jobs-PHASE-FIRST-COUNT`,与 candidate-chain 的 wave_phase 检查点标签一致。
- `rc_over` / `rc_stuck`:是否超预算 / 是否已在粒度下限仍超预算。

规则(预算只是输入,绝不放宽):

1. 首波(prev_count 不大于 0)取 max,夹到 min 与剩余量之间。
2. 上一波墙钟大于预算:分块减半。
3. 墙钟在预算的 75% 到 100%:按 60% 目标比例缩小,至少减 1。
4. 墙钟在预算的 40% 到 75%:不变(45% 到 75% 是目标带,40% 到 45% 为死区,避免来回抖动)。
5. 墙钟小于预算的 40%:按 60% 目标比例增长,至少加 1,不超过 max(max 可配)。
6. 结果总是夹到 [split_min, max] 与 total - first 之内。
7. 已在 split_min 仍超预算:rc_next 返回 min,rc_stuck 为真;调用方必须失败并报告,不得放宽预算。

## 3. 调度器如何按 split_dim 调用

调度器对每个节点读 split_dim 与 split_min,把节点展开成「项」序列:

- jobs:项 = 作业,总量 total = 该相位作业数(tool 5、gen/tbl/net 各 42)。
- units:项 = 前端单元,游标即单元序号。
- targets:项 = 目标切片。
- stages:项 = 相位步骤。
- none:不展开,单波执行;超预算只能报告,不能自动拆。

循环:

```
first = 0; count = 0; wall = 0
while first < total:
    count = rc_next(total, first, count, wall, budget_s, split_min, max)
    tag = rc_tag(phase, first, count)
    if resume and checkpoint 已含 tag: first += count; continue   # 续跑
    t0 = now; 运行 [first, first+count); wall = now - t0
    写检查点 DONE tag wall_ms=...
    if rc_stuck(count, wall, budget_s, split_min): 失败退出
    first = rc_first(first, count)
```

续跑兼容:检查点键沿用 candidate-chain 现有的 `jobs-PHASE-first-count`。续跑时已完成波的 (first,count) 来自检查点自身,不依赖重新计算,所以自适应规则以后调整不会使旧检查点失效。

## 4. candidate-chain 后续接入(由 omp-grk 做,本任务不改)

candidate-chain 的 wave_phase 目前用固定常量:默认 8、超 40 秒减半、低于 25 秒加 4、上限 16。接入方式:

1. 删除这些常量,改为 `#pragma unisa use cxlib.rechunk`,首波与每波后调用 rc_next。
2. 预算与 split_min 不硬写,从 graph.tsv 读(budget_s、split_min、split_dim),max 取 16 或节点配置。
3. 墙钟超预算且已在下限时调用 rc_stuck 并 die,而不是继续。
4. 检查点标签调用 rc_tag,格式不变。
5. 非 jobs 维度(units/targets/stages)的相位用同一循环,只是 total 与「运行一段」的动作不同。

## 5. 已知边界

- 全部 split 值为推断,未按节点实测校准;第一次真实运行后应据 observed_wall_s 修正 split_min。
- 39 个 none 节点中 budget_s 不小于 45 的都已附理由;若实测显示它们贴线,需要产品侧先给出可拆接缝(例如 e3 按全局索引分批,对应计划中的 Y 包)。
- rc_next 假设单项耗时大致线性;固定开销大的节点(单项耗时小而每波固定开销大)会比线性估计更快贴线,首波之后的缩减靠减半兜底。
