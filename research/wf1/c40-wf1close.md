# 0.0.40 WF1 快测闭环（机房主任 21:57 代裁：通过（限定））

同身份：动态参考 PRECHECK → 受影响正式分片 → 实际结果。工作树 /tmp/cc40-prep/wf1wt detached @0a6f334e，前后 status 0 行。原始证据在仓外 `/tmp/cc40-prep/wf1close/`（receipt.md、precheck.log、formal-difftest_o-{1,4,2}.log）；PRECHECK 回执副本：`research/wf1/cov1-precheck-0a6f.json`（旧 `cov1-precheck.json` 是 19daaf1d 的历史，不抵本轮）。

## PRECHECK（2026-10-10 ~21:45 SGT）
`c99precheck.py tests/wf1/cov1.tsv`：GREEN，cold 11.68 s，warm 0.6 s，rc=0。
- 身份：source 0bd83174…，UA fd8f3877…，cc Debian 14.2.0-19，oracle cc 5c036d42…，gcc-14 后端 a23ecab8…，difftest_o.sh e5d8c6be…，build_ref.sh 061f6446…，c99precheck.py b6b5dc74…，group ac5a9da2…
- formal_plan：4 片，257 个成员，ordered_list_sha256 c889acd7…
- 映射：fb12-31 → difftest_o-1，fb12-29 → difftest_o-4，fb12-32 → difftest_o-2；每个都是 3/3 agree

## 正式分片
`gate.sh --suite difftest_o-K`，UA 用同一个文件，`env -u WF1_PRECHECK -u PROBES`。

| 分片 | 行首 rc | 子 rc | 探针 | agree | wrong/refuse/known/revived/named | gate 自报 |
|---|---|---|---|---|---|---|
| difftest_o-1 | 0 | 0 | 65 | 195 | 0/0/0/0/0 | 3s |
| difftest_o-4 | 0 | 0 | 64 | 192 | 0/0/0/0/0 | 3s |
| difftest_o-2 | 0 | 0 | 64 | 192 | 0/0/0/0/0 | 3s |

每片前后 UA 都是 fd8f3877。事后（21:50:15）复算 HEAD、净树和六个 SHA（difftest_o.sh、build_ref.sh、c99precheck.py、oracle cc、gcc-14 后端、UA），都与 PRECHECK 相同。

## 三方核
- cdx：限定通过
- cdx2：限定通过（`/tmp/unisacc-cdx2/workflow-efficiency/wf1-close-independent-2150.md`）
- grk：读数对上，com 义务不免

三方都独立复算了身份和映射。

## 边界（照列，不称已证）
- 外层墙钟 UNKNOWN：计时用的 bc 本机没装，wall 字段为空；没有重跑，没有推算，只引用 gate 自报的 3s。
- 逐探针结果靠守恒推出（agree = 探针数×3，其余为 0），正式日志里没有逐个点名。
- 原日志没有完整 argv；env 清除和事后采点只在回执里有记载。
- 只证动态参考路线：com-difftest_o 没跑，产品义务不免，按 21:57 顺延到产品/K5 切口。
- UA 有效性键含包装 cc 的 sha，不含 gcc-14 后端的 sha，所以后端变化不会让 UA 自动失效。
- 第 3 分片不受影响，没有跑。
- 不量化提效。
