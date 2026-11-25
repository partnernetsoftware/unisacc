# 教训:置顶批不应占 W0000 冷窗位(草稿,待并入技能页;2026-11-25 ccogweco)

## 现象

0.0.107 第二轮预跑用 priority-file 把新增/改动批置顶;结果 `module-units-forward` 被排到 **W0000**,
承担首窗冷编译,两次 58 秒 rc=124,需要补跑。第一轮也有同类:`cxmod-tri-exports` 在 W0000,一次重试。
置顶的初衷是让风险批早暴露,不是让重材料批吃冷启动。

## 根因

- priority(tier 2)只调顺序,不区分冷/热;首窗仍是冷 kit,重材料批放这里必然撞 55 秒窗;
- 排序逻辑没有"W0000 固定轻载"的硬约束;priority 批可能因最短耗时选择被吸进首窗;
- 置顶批与"首窗冷态探针"的目标不同:前者要早发现红,后者要验证 kit/环境,混在一起两边都做不好。

## 固化

1. W0000 固定为一个**轻载、jobs=1**的批(优先选已知短批或专用 warmup 批),不吃 priority 顺序;
   priority 列表里即使包含它,也必须在计划生成后核 W0000 的实际批名。
2. 重材料批(module-units、tsv-lint、ratchets-tool 等)在 priority 段内也要排在 W0000 之后,通常从 W0001 起;
   大批若首窗 rc124,先等自动 attempt=2,不要手改计划。
3. 计划验收三行(新会话计划):

       head -n 25 <prerun>/windows.tsv
       rg -n -F -f <priority-file> <prerun>/windows.tsv      # 置顶批落位
       rg -n '^(# window-lanes=|W0000[[:space:]])' <prerun>/windows.tsv

   判据:W0000 为轻批且 jobs=1;21 个置顶 id 在前 32 窗内;W0000 不是任何重材料批。
4. 若计划把重批排进 W0000:先查 timing 输入与首窗选择,再起跑;不得用"跑起来再说"。

## 0.1.0 草案核对(cdx 的 seal-priority-0.1.0.md)

- 该页已写明:"W0000 保留一个轻批、jobs=1","显式优先表不置 nonpass,还须确认高风险批没有被最短耗时选择吸进
  W0000",并列了上面三条验收命令。**不会重蹈**,但有两个执行前提:
  (a) 优先表按批 ID 去重、不写 `ID<TAB>2` 的旧格式,而用 `prerun-inputs --priority-src` 的输入格式;
  (b) 计划生成后必须实际核 W0000 的批名,不能只看首页说明。
- 建议在 0.1.0 起预跑前把该页的三条命令原行贴进值守记录,作为 W0000 验收证据。

## 108/0.1.x 候选

- prerun-drive:新增"W0000 轻载锁":无论 priority 如何,首窗只接受轻批/专用 warmup 批;若计划把重批放入 W0000,
  直接 REFUSE 并点名。
- 或退一步:计划生成后由 `prerun-inputs` 打印 W0000 批名与估算时长,超阈值即拒。
- 归属:工具线(不触 SID);与 W0201"新增批实跑"、priority 排序同批落 0.1.x。

## 落点

- skills/unisacc-orch-supervise 的预跑节;roadmap-0.1.x.md 的预跑提速与 priority 节;
- 证据:本页、第二轮 supervise.log 的 W0000 两次 rc124 原行(agy-h 账本)。
