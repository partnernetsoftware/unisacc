# 0.1.0 说明:预跑证据节骨架(占位与取值;2026-11-25 ccogweco)

## 0 正文骨架(正式说明用)

### 严格预跑

本版候选在 <DATE> 起严格预跑,共 <WINDOWS> 窗、<BATCHES> 批,用时 <MIN> 分钟;命令口径:prerun-drive 单次 exec、
无 --normal-jobs、2 lane、W0000 轻窗口、优先清单 v010-priority-newbatches.txt。
证据:release/evidence/0.1.0/prerun-supervise.log、prerun-windows.tsv、strict 合并表。

首窗披露:首次 W0000 在冷态下重材料批(module-units-limits)两次 rc=124(58041 ms),按规则停 W0000 并重起;
重起后 W0000=module-units-forward,5297 ms 通过;重起损失约 8 分钟。冷首窗根因与分相引用 cdx 页
(research/cdx/v010-prerun-cold.md,待落)。该损失如实记,不并入任何 PASS 时长。

贴限四批:list-cap 等若为 UNKNOWN,按 window-timeout 登记 expected-unmeasured(引用
research/ompgpt/margin-disclosure-107.md 的同类口径);premerge-selfcheck-main/-rd、slow-split 按 PASS
但余量不足披露(main 50251/余 2749、rd 49898/余 3102、slow-split 51144/余 1856)。

### 与 0.0.107 对比

| 项 | 0.0.107(第二轮) | 0.1.0 |
|---|---|---|
| 窗数 / 批数 | 241 / 519 | <WINDOWS> / <BATCHES> |
| 预跑用时 | 80.9 分钟 | <MIN> |
| 首窗 | W0000 冷态两次 rc124 后 PASS | W0000 首次两次 rc124 后重起,PASS 5297 ms |
| strict | 518 PASS / 1 UNKNOWN | <PASS>/<UNKNOWN> |
| UNKNOWN 处置 | list-cap 设计性贴限 | <按实际> |

## 1 占位与取值命令(只读)

| 占位 | 取值命令 |
|---|---|
| <WINDOWS> / <BATCHES> | `grep -c '' <PDIR>/windows.tsv`(数据行)与 `grep -m1 'total=' <PDIR>/supervise.log` |
| <MIN> | `head -1 <PDIR>/supervise.log` 与 `grep -m1 'PRERUN_RUN status=DONE' <PDIR>/supervise.log` 的 epoch 差 |
| <PASS>/<UNKNOWN> | `grep '^SUMMARY' release/evidence/v0.1.0-strict-<DATE>.tsv` |
| 首窗两次 rc124 原行 | `grep -h 'W0000' <PDIR>/supervise.log | grep 'rc=124'` |
| 重起 W0000 行 | `grep -h 'PRERUN_WINDOW.*window=W0000' <PDIR>/supervise.log | tail -1`(期望 rc=0 wall_ms=5297) |
| 贴限四批 | `grep -h 'rc=124\|wall_ms=5[0-9][0-9][0-9][0-9]' <PDIR>/supervise.log` |
| expected-unmeasured 行 | `grep -P '^precommit-list-cap\t' release/evidence/0.1.0/expected-unmeasured.tsv` |

## 2 边界

- 首窗重起损失只作披露,不写成通过时间;四批贴限按 PASS 记但必须写余量。
- 未测项保持 UNMEASURED;不引用 107 的 strict 数字替代 0.1.0。
