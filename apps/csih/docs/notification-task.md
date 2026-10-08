# notification-task.md（设计，未实现）——修订版

## 已核真实入口
- 收信入口在外仓：`skills/mux/mux.ts:5414` 包装标题+正文；发送用 `paste-buffer -p`（704/706）+ 单独 Enter（4904）。
- csih 侧真实路径：`tui.c:1055 TERM_KEY_PASTE_ON` 置 `st->pasting=1`；`tui.c:1058 TERM_KEY_PASTE_OFF` 置 0 后 `if (st->input[0]) tui_manual_submit(st)`——**粘贴字节直接追加 input，OFF 自动提交；已有草稿会被整段串入消息**（tui.c:1066 ENTER 分支在 pasting 时也只追加 `\n`）。
- `agent_cli.c` 的 `agent_watch_result/note`（81/82）是 watch 角色工具输出，**不是 TUI 收信入口**；原稿称其为收信提示属虚构。`pending_kind`、`tui_log_append` 均不存在，不引入。
- 故不采用进程级 `CSIH_MSG_KIND`：它无法给同一会话逐封分类。

## 显式分派约定（语义，非身份）
- 标志仅表种类，不做权限认证；不靠标题/来源自然语言猜。
- 可选把标题精确等于 `[notice]` 视为 notice 标志（外仓不改，标志仅语义）。

## 最小改动（改 C 时，本次不实现）
1. 新增独立暂存 `st->paste_buf/paste_len`（不动 `input`）。`PASTE_ON` 置 pasting 且记起点；粘贴期间字节进 `paste_buf`，原 `input` 逐字节保留。
2. `PASTE_OFF`：不再直接 `tui_manual_submit`。据**本次粘贴自己的**标志分派：
   - 无标志/普通用户粘贴 → 现行为兼容：把 `paste_buf` 按原样追加进 `input`，不自动提交（回到旧语义或保留自动提交二者择一，需父定；本条注明现行为）。
   - 显式 notice → 只写日志（新 helper `tui_note_line`），`input` 逐字节不变。
   - 显式 task → 用 `paste_buf` 独立文本排队，不拼草稿。
3. `PASTE_ON/OFF` 前缀精确匹配 `\x1b[200~`/`\x1b[201~`（tui.c:2743/2750 已是）；标志须在被包裹文本首行精确等于 `[notice]` 才生效。

## 边界
- 未知/无标志 → 按 task（旧信封兼容）。
- 容量：`paste_buf` 满或超长则整段拒绝并提示，不截断拼入。

## 三个验收
1. notice 互发：零模型请求。
2. 显式 task：执行一次。
3. 忙时有草稿时来 notice：草稿逐字节不变，notice 只进日志。

## 边界（外仓）
投递结构在 moltbaby，csih 只能改收信后分派；不改 envelope。未实现。
