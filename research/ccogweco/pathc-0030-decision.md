# 00:30 判定决策树(备用线;2026-11-25 00:1x ccogweco)

只写不执行。输入:第六笔(tests/env-semantics.tsv 追加四行)落车后,W0206 与三个消费者批
(env-semantics、envcompat、premerge-selfcheck)在同一候选 APE 5f83ac93… 与新车头 frozen-root 上的单批复跑结果。
冻结 03:00 不变;任何新红都不延期、不登记预期失败。

## 分支 A:三批 + W0206 全绿(并发假象)

- 触发判据:四条 BATCH_RUN 原行均 `status=PASS rc=0`;check-env-semantics 为 `assertions=133 failed=0`;
  check-envcompat 与 premerge-selfcheck failed=0;W0206 同绿。
- 立即动作:按 B-t 扩展完成合并(第六笔留在新车头)-> strict 表按 SUPERSEDED 折叠原 FAIL -> 发布准备
  -> 打包 -> 执行 -> 公开;披露按 env-semantics-acceptance.md 第 5 节。
- 时间账:发布准备+打包+执行约 40 分钟;从 00:30 起算到 02:40 公开,余 20 分钟;全绿分支不需要再补跑。
- 谁做:agy-flow(合并、补跑收尾、发布流程),cc 核披露与 INDEX。

## 分支 B:仍有红,且红指向六笔 tools/tests 修复

- 触发判据:红项能在六笔内定位(修复只碰 tools/ 或 tests/ 的检查源/夹具,不碰 src/cx/include/seed/kernel/exec),
  且红因可用"夹具/检查源"解释;不涉及产品闭包。
- 立即动作:
  1. 先判时间:现在到 00:37 前必须完成"补一笔 + 三批复跑";00:37 后不再开新补丁,直接转分支 C 或出窗。
  2. 补一笔只允许单文件或单批夹具(同 B-t 形态),提交前 cc 核;随后只复跑受影响批,不整预跑。
  3. 若 00:37 前补丁未绿:不再尝试第三种写法,写清红项原行后转 C。
- 时间账:补丁 5 分钟 + 复跑 5-8 分钟 = 10-13 分钟;00:37 截点后,发布仍按 40 分钟,02:40 公开可行,
  余量压到 15 分钟以内;超过 00:45 仍未绿即出窗(03:00 前无法保证发布准备)。
- 谁做:对应窗口出补丁,cdx 二审,agy-flow 复跑,cc 定是否启用;ccogweco 只更新本页时间账。

## 分支 C:红指向产品/SID/exec(或 00:37 前补不成)

- 触发判据:红项落在 src/、cx/、include/、seed/、kernel/、exec/,或修复必须动这些目录;或分支 B 在 00:37
  仍未绿;或穷举出现会改变结论的新读者。
- 立即动作:转路径 C(见 path-c-rechain-plan.md v2 表):新车头 = 879e446640 + exec/facts/env-semantics.tsv
  补登记(exec 在 SID,APE 与 SID 变);冻结行 base 改新车头,deadline 仍 03:00;不再动 tests 表那笔作为依赖。
  起链前检查 chain-ready 全量(DEVSVC_OWNER=cc 单传,25 节点,READY=yes)。
- 时间账:chain-ready 5 + 整链 28 + 预跑 55 + 发布 40 = 128 分钟。00:30 起 -> 02:38 公开,余 22 分钟;
  00:35 起 -> 02:43,余 17 分钟;**00:40 之后起 chain-ready 即出窗**(02:48 公开,余 12 分钟,不再接受)。
  旧 APE 5f83ac93 的预跑与补跑作废,只作定因证据。
- 谁做:agyneon 出车头与派生、agy-flow 起链与预跑、cc 决定启用与披露改口径。

## 兜底:04:00 前任何分支未完成

- 动作:保留全部原行(链目录、exit.tsv、strict 表、复跑证据),冻结行原地改期并留审计,owner 不变;
  由 cc 向所有者报"未按 03:00 完成"与失败节点,不写 PASS、不删断言、不改口径。
- 谁做:cc 报,ccogweco 只交证据路径清单。

## 判定顺序(一句话)

00:30 拿四条 BATCH_RUN 原行 -> 全绿走 A;有红先看目录:红在 tools/tests 且 00:37 前能绿走 B;否则走 C;
C 到 00:40 未起 chain-ready 即出窗,进兜底。
