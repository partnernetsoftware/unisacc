# 0.0.106 发布后账本草稿(只写不跑;2026-11-25 ccogweco)

格式仿记忆宫殿决定块:决定、依据、证据入口、回滚。时间均为 +0800。

## 决定 1:0.0.106 公开(含一次回滚后重新公开)

- 决定:0.0.106 于 2026-11-25 **01:42:35** 重新公开;首次公开 01:36:31 因发布后读回 smoke 冷编超 45 秒,
  由流程自动回滚到 Draft,6 分钟后重公开。两次都对外发生,记录保留,不删首次记录。
- 依据:读回失败是缓存冷态与超时窗口,不是产品字节差异;同候选 APE 5f83ac93… 与 SOURCES_SID c6eeca9f… 未变。
- 证据入口:release/evidence/0.0.106/INDEX.tsv 的 publish/readback 行;release/evidence/0.0.106/ci-45446325698.log;
  Y1 32f26cd024(2026-11-25 01:34:16,v0.0.106 Y1)、Y2 9d08f985fa(01:42:54,v0.0.106 Y2: public readback)。
- 回滚:需要撤回时按 release/rollback.txt 把 v0.0.106 转回 Draft 并读回;Y2/Y1 前向 revert、保留 tag 与 release。

## 决定 2:CI 两次 run 的记录口径

- 决定:CI 首个 run 45442318936 记失败(原因:windows-11-arm 格 fetch 任务对新建草稿的 gh release download
  撞 GitHub 最终一致性瞬态 404,被 `|| echo warning` 软失败吞掉,slice 资产缺载荷),同候选显式重派一次,
  run 45446325698 全绿 **42/0**(PASS=42 fail=0);两次 run 都记录,不用后一次覆盖前一次。
- 依据:同候选身份(冻结提交与 APE sha 不变),失败格属外部服务瞬态,非产品回归。
- 证据入口:release/evidence/0.0.106/ci-45442318936.log/json 与 ci-45446325698.log/json;
  notes-0.0.106 已知缺口 (192) 同步记录。
- 回滚:若重派 run 也被判无效,按 CI 证据不成立处理,重派第三次并保留全部原行。

## 决定 3:读回事故的根因与处置

- 决定:读回 smoke 的 45 秒超时是冷缓存下的工具冷编(不是产品缺陷)。处置为"读回前精确 argv 预热":
  发布前用与读回完全相同的 argv 先跑一次 warm,使第二次命中;实测预热后 1452 ms。首次公开按失败处置、
  自动回滚 Draft,不改时限、不跳过读回。
- 依据:冷编耗时在 45 秒窗口内不可控;预热是同 argv 重复执行,不改变被验证的字节与断言。
- 证据入口:release/evidence/0.0.106/ 的 warm-<key>.tsv 与 INDEX 的 warm/readback 行;
  release/evidence/0.0.106/ci-45446325698.log。
- 回滚:删预热步骤即回到冷编路径(保留 45 秒时限);不得放宽读回时限。

## 决定 4:107 追加集新增三条(本车暴露的流程缺口)

- 决定:107 追加集在原十二份之外再增三条:(a) 读回前自动预热(精确 argv,失败即具名);(b) warm_note
  收据的位置与 scrub(收据写 release/evidence/<V>/warm-<key>.tsv,家路径 scrub 后再入证据);
  (c) rollback 写 INDEX 行(回滚动作本身留 INDEX `rollback` 行,不只 stdout)。
- 依据:本车读回回滚暴露了"预热、收据、回滚留痕"三处无自动记录;三件均 tools-only,不触 SID。
- 证据入口:release/evidence/0.0.106/INDEX.tsv 的 warm/readback/rollback 行;notes-0.0.106 发布后账本。
- 回滚:三条任一被证伪就从 107 追加集撤回并在 107 说明中披露;不改已公开的 106 证据。

## 决定 5:准入条件 4/5/6 的诚实状态(截至 106 公开)

- 条件 4(state-unverified 声称类 ≤3):**未满足**。声称类基数 10,按现有证据预期 3 到 4;E02 保留,
  U05 与 U33 保留。证据:build/state-unverified.tsv、unverified-classification.md、seal-0.1.0-evidence-map.md 条 4 三值。
- 条件 5(前端三期):**未满足,按 R37 豁免**。B1 部分入 107,B2(0.0.109)未开工,封版说明须明写未满足。
  证据:R37 第 5 条;plans/v0.0.109.md。
- 条件 6(整链起到公开连续 3 版 ≤120 分钟):**原文不满足**。0.0.106 链起 21:31:05,公开 01:42:35,
  实测 **251.5 分钟**(链止 21:58:56 到公开 223.7 分钟);历史 98/100/101/105 只有 98 为 96.5 分钟达标。
  按 R6 重述(第三样本取封版车到 READY_TO_EXECUTE)仍需 107 与 0.1.0 样本,当前不足三样本,如实记未满足。
  证据:research/agy-h/chain-to-public.tsv(0.0.106 行待填公开列)、release/evidence/0.0.106/INDEX.tsv。

## 决定 6:本车账本写入位置与后续

- 决定:本节随 0.0.106 公开落记忆宫殿决定块(由 cc 落,ccogweco 只交草稿);107 与 0.1.0 的同类事实
  各自新起一节,不追改本节;条件 6 的 107/0.1.0 分钟数在各自公开后补。
- 依据:记忆宫殿只记决定与证据入口,过程细节留在 research 页。
- 证据入口:本页、research/ccogweco/notes-0.0.106-final.md(v4 留存)、release/evidence/0.0.106/。
- 回滚:删本节并把条件 4/5/6 状态回退为 106 公开前记法;已公开证据不动。
