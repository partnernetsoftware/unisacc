# reload-session 映射与实现边界

v2 尚未部署。checkpoint 与会话 codec 已实现；TUI export/resume 接线及受控换版尚未实现。下方原始定点读取记录保留其历史观察归因。

## 1. 字段映射：目标 = 来源 → 恢复到哪
- version：JSON 数字 2（不是字符串；见 §4 版本升级）→ 解码入口校验，非 2 拒。
- v2 严格要求全部 17 个顶层字段且每键恰好一次：version、session_id、handoff_id、candidate_hash、goal、loop_on、loop_left、cwd、role、peer、journal、input、pending_queue、history、history_pos、history_browsing、history_draft。缺失、重复或未知键均拒绝；旧缺 loop_on/loop_left 的未部署 v2 不再接受。
- v1 显式迁移仍是设计：未来入口须显式选择空历史及自动执行初值；现有 v2 codec 不提供该放宽入口。
- export 列全 input/pending_queue/history 四字段（含 history_browsing/pos/draft 派生），不漏。
- goal：tui.c 的 state.goal[TUI_INPUT_MAX] → 恢复写回 TUI struct goal。
- loop_on：state.loop_on → codec 的 int loop_on（只允许 0/1）→ 恢复持续执行开关；JSON 必须是 true/false，拒绝数字 0/1，不能从 goal 推断。
- loop_left：state.loop_left → codec 的 int loop_left → 恢复剩余自动重提交预算；JSON 数字须为 0..8 的整数。loop_on=true 且 loop_left=0 是合法状态，不重置预算。
- input：state.input/ninput（tui.c 122..123）→ 直接写回，作为未提交输入缓冲。
- pending_queue：state.pending[8]/npending（124..125）→ 直接写回，越界(>8)拒。
- history：state.history[16]/nhistory（126..127）→ 直接写回（v2 新增）。
- history_browsing：state.history_browsing（129）→ 写回。
- history_pos/cursor：state.history_pos（128）→ 写回，并约束 [0,nhistory]。
- history_draft：state.history_draft（130）→ 写回。
- session_id/handoff_id/candidate_hash：无会话字段 → 待新增（见 §3）。
- cwd/role/peer：来自环境/启动配置，非该 struct 字段 → 不序列化环境；只允许显式传三个值，缺则拒。

## 2. journal 实际路径与已提交偏移
- journal 对象：{path: 绝对路径, offset: 非负整数}（reload_state.c 135..146 校验）。
- session.c 实际 append：session_append(path,record) 用 fopen("a")/fwrite/fputc/fclose（session.c 78 起）。
- 已实现 journal_checkpoint：空闲、单写者前提下，实际日志 fsync 后 fstat/close 成功才返回 offset。fclose 成功或任意 stat 大小不能替代该同步边界；这尚不代表日志重放恢复已实现。

## 3. 身份真实保存位置：尚无 → 标待实现
- session/handoff/candidate 当前无持久字段。
- 待实现：会话记录中新增三元组写入点；解码时与 token 比对，不一致拒。不在本次设计内编造 getter。

## 4. v1 兼容与 v2 升级
- v1 schema 仅 11 键（reload_state.c 51..54）。
- history/browsing/pos/draft 与 loop_on/loop_left 属 v2 新增字段，v1 无。
- 策略：version 是 JSON 数字 2，不能是字符串 "2"。现有 v2 codec 严格拒绝 v1；显式 v1 迁移入口仍属待实现设计，须显式决定历史和 loop_on/loop_left 初值，不能冒充完整恢复。

## 5. 空闲快照 / 未激活恢复次序
- 次序：export → 恢复到未激活 state → 验收 → 之后受控换版。前两步不算 H1 完成。
- 采集只在空闲：busy 时不导出运行中 tool/HTTP；等空闲，暂停输入、写 journal，再取最终快照。
- 禁止序列化：环境、key、请求头。

## 6. 最小接入次序（仅设计）
1) export 全部 17 个字段，含三个身份、goal/loop_on/loop_left/cwd/role/peer、journal{path,offset}、input/pending_queue 及四个历史字段。
2) load 到未激活副本，跑校验，通过才替换。
3) 验收：diff 恢复后的 struct 字段。
4) 受控换版：v1→v2 显式升级路径。

（checkpoint 与 codec 的独立实跑证据见 prd.md；本节 TUI 接线与换版次序仍仅设计，未完成。）

## 源码实测（本步仅一次定点 read tui.c:120-180）
- tui.c:123 `input[TUI_INPUT_MAX]` / :124 `ninput`
- :125 `pending[8][..]` / :126 `npending`
- :127 `history[16][..]` / :128 `nhistory` / :129 `history_pos` / :130 `history_browsing` / :131 `history_draft`
- :175 `goal[TUI_INPUT_MAX]`（/goal 文本，未设为空）
- session.c session_append = fopen a / fwrite / fputc / fclose，无 fsync，无已提交偏移 getter。
