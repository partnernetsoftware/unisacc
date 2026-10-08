# 动作结果证据设计（仅设计，未实现）

## 现状真实故障（读 agent.c 核实）
- agent_exec 返回 plugin_run 结果；agent_file 的 read 失败仍 return 1；write 只看参数是否非空，故"返回 1"不等于工具成功 rc。
- write 0 字节是合法空文件，不能当失败。
- agent_object_count（432）现在只在 agent_exec（1953）执行之后追加一句 "[only the first action ran; send one JSON object]"（1954），是事后注记，首条副作用已发生、第二条已漏。
- agent_may_stop_ans / at_judge_nudge 只看 answer 非空与 judge 计数，不读本轮失败证据。

## 结构化工具 status（从真实结果取，不从文案匹配）
新增 struct tool_status { int handled, success, gate_status; char op[8], key[64], code[24]; }，按工具类型填：
- exec：shell_result.rc；rc==0 才 success。handled=已跑。
- read（file_list/file_read 的 file_result）：read 成功与否由 file_result 自身判断，**不采信 return 1**，故 read 失败时 handled=0、success=0。
- write/edit（file_result/edit_result）：write 参数非空即 handled=1，但 success 需文件真的写出（0 字节空文件合法，记为 success 而非失败）。
- slice 门禁：gate_status 取 agent_slice/why 的真实 rc，rc!=0 即 gate_status=失败。
三态定义：handled=该动作被跑到（不含"输入含更多未跑动作"）；success=真实结果 rc 通过；gate_status=切片门禁 rc。

## 恢复关联键必须精确，不能用任意后续成功清 pending
- key = op + 目标/请求标识（如 read:<path>、write:<path>、exec:<命令首词或请求 id>）。
- pending 只由**同 key 的真实成功**解除；异 key 成功（如 pwd、同路径 read）**不能**解除一个 write 失败。
- 对任意 exec 的语义修复无法自动证明：保留 unknown，进入 judge，不作确定性保证。

## 送轮末判官
- ev_pending 记录未解除的 {refused|failed} 与其 key；ev_unknown 记录无法自动判定的。
- at_judge_nudge / agent_may_stop_ans：ev_pending 非空且无同 key 成功覆盖时禁止停止通过，只能作失败回合结束并保留 reason；ev_unknown 明确标注"不可自动证明"，交由模型判断，不伪称确定性。
- 不靠中文词匹配"完成"；不伪造任何工具结果。

## 多 JSON：执行前整体拒绝
agent_object_count 在**任何执行之前**判定：输入含 >1 个动作对象则整体拒绝、不跑首条，回 "send exactly one JSON object"，随后单对象重试即可恢复。避免首条副作用与漏第二条。不新增解析器，复用 agent_object_count。

## 非确定性边界
同 key 恢复中"这次成功是否真的修复了上次失败"的语义部分靠模型；工具只确定 handled/success/gate_status 与 key 关联，能减少"无证据通过"，不保证任意任务已被算法验证完成。

## 最小回归
1. 拒绝 -> pwd -> 假完成：先非法动作，再 pwd（异 key 成功），answer 称完成。期望 pending 仍在，判失败。
2. 空文件写入合法：write 0 字节记 success，不判失败。
3. 门禁失败未被 read 消除：slice 门禁 rc!=0 后同路径 read 成功，pending（key=slice:...）不解除；须同 key 门禁通过才清。
4. 多动作第二未执行：输入两个对象，整体拒绝不跑首条，单对象重试才可成功。
