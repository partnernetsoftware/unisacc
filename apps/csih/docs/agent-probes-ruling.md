# 待裁定：两个 agent 旧探针的期望要不要按现行为改写

2026-10-30 整理，供主人裁定。这两个探针在 `tests/contracts.tsv` 里记为 BLOCKED，探针本身没有改动。

- `probes/agent_loop_bound.cx`：31 例里 12 例通过，19 例失败。
- `probes/agent_multi_action.cx`：9 例里 5 例通过，4 例失败。

失败原因只有一个，没有第二个。现行协议下，一个答复如果不带证据（没有 `evidence`，用过工具之后还缺 `claims`/`observation_refs`），会被记为"◐ 未验证"，这一轮就此结束，不再请评审，也就是少发一个请求。这两个探针写于 10 月 10 日，它们期望的是"答复之后总有一轮评审"。

## 逐例对照：agent_loop_bound

"请求数,退出码"写作 `(n,rc)`。

| 用例 | 期望 | 实际 | 差在哪 |
| --- | --- | --- | --- |
| normal | (3,0) | (2,1) | 裸答复 → 未验证，没有评审轮 |
| last_round_answer | (9,0) | (8,1) | 同上，发生在最后一轮 |
| outcome_last_continue | (9,1) | (8,1) | 少了最后那一轮评审 |
| prior_answer_budget | (9,1) | (1,1) | 第一个答复就未验证，预算没有用到；期望的提示 "MAX_ROUNDS after continue" 没出现 |
| stop_no_answer | (3,1) | (1,1) | 没答复就 `go:stop`，现在报 "stop without independent acceptance"，期望是 "go=stop without answer" |
| direct_stop_no_answer | (3,1) | (3,1) | 请求数和退出码都对，只差上面那句提示 |
| invalid_judge | (4,1) | (1,1) | 答复未验证，根本走不到评审，所以测不到"评审回了非法内容" |
| invalid_judge_recovery | (3,0) | (1,1) | 同上 |
| watch_failed、watch_partial、watch_stop | (1,1) | (1,1) | 请求数和退出码都对，差的是 watch 模式的提示文字 |
| watch_answer | (2,1) | (1,1) | 少了评审轮；还期望出现 "peer mail not delivered" |
| watch_no_tools | (2,0) | (1,1) | 少了评审轮 |
| watch_false_comment、watch_echo_receipt、watch_false_true、watch_wrong_peer、watch_exit1、watch_timeout | (3,1) | (2,1) | 少了评审轮；还期望出现 "peer mail not delivered" |

"peer mail not delivered" 这句话在 `agent.cx` 里还在（2938 行），只是现在走不到：在它之前，答复就已经因为未验证结束了这一轮。

## 逐例对照：agent_multi_action

| 用例 | 期望 | 实际 |
| --- | --- | --- |
| double-write-recovery | (5,0) | (4,1) |
| double-exec-rejected | (3,0) | (2,1) |
| single-fenced-braces | (3,0) | (2,1) |
| no-tools-braces | (2,0) | (1,1) |

这 4 例的情况一样：桩最后回的是裸答复，被记为未验证，少一轮，退出码为 1。

## 两种裁定

- **A. 现行协议是有意的。** 我建议选这个。PRD 的"结果三态 ✓/◐/✗"、第八批和第四十三批都把"未验证"写成正式状态，评审只处理带证据的答复。
  - 用例和输入一个都不删。
  - 每个用例按它原本想测的东西重写：
    - 测"评审轮"本身的用例（invalid_judge、watch_* 等），把桩的答复改成带证据的形状，像 resume_pending、file_observation 那样。这样评审轮照常出现，原来的请求数和"peer mail not delivered"等断言照常成立。
    - 测"裸答复"的用例，把期望改为现行为：未验证，少一轮，rc=1，提示是 "answer outcome=unverified"。
  - 这是改写测试数据和期望，产品不动。
- **B. 旧行为才是对的。** 那失败的就是产品：要恢复"未验证答复之后仍然请评审"，改 `agent.cx`（产品输入，要重建产品），这两个探针原样当回归。

在裁定之前，这两条保持 BLOCKED，探针不动。

## 进展（2026-10-30）

- 主人的长期授权是"技术细节自己裁定"。据此选 A，先做 agent_multi_action。
- 这个探针原来还有一处缺陷：每个用例都用 `-run agent_cli.cx` 现编，冷编译约 28 秒，第一个用例就超时，所以上面那 4 例其实从来没跑到。现在改为编一次 `agent_cli.bin`；副本里缺的 sys/mem/text 用 `csih_copy_l0` 补上。
- 4 例都以裸答复收尾，期望改为现行为（请求数减 1、rc 1、原因 `answer outcome=unverified`）。no-tools-braces 测的正是"带花括号的纯文本仍算答复"，桩保持纯文本。
- 多动作拒绝与恢复、写出的文件、错误 trace、纯文本 trace 这些断言一条没动。
- 结果 9/9 PASS。`tests/contracts.tsv` 里的 BLOCKED 不在本车道的可写范围，等有权限的人换标。
- agent_loop_bound 还没做：其中 invalid_judge、watch_* 等用例测的是评审轮本身，需要能回填 action ID 的动态桩（参照 completion_contract 的 extract_ids）。
