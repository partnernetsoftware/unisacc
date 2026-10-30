# csih 的 Python 依赖登记

2026-10-30 盘点。原有 34 个 `.py`，分两批归档到 `archive/reference-py/apps/csih/`，相对路径不变：第一批 28 个，第二批 5 个。现在只剩 1 个，而且它的调用方在仓外。apps/csih 的 `.cx` / `.h` / `.sh` 里已经没有 `python3` 调用。

判定"活调用方"的依据：有 `.cx` / `.h` 把它当路径字符串传给 exec，有 `.sh` 直接调用，或有仓外入口调用。注释里写"C port of X.py"的不算。

## 仍在用的（1 个）

| 文件 | 调用方 | 为什么还没去掉 | 去掉的条件 |
| --- | --- | --- | --- |
| `apps/csih/deepseek-proxy.py` | 仓外的 `moltbaby/bin/rawcsih`（`python3 "$CSIH_DIR/deepseek-proxy.py"`），当明文代理跳到 api.deepseek.com | `net.cx` 已经能直连 https，csih 本身用不着这个跳板了；但 rawcsih 还在拉起它，而 rawcsih 不在本仓 | rawcsih 改成直接用 `https://api.deepseek.com` 端点之后归档 |

## 第二批归档（5 个，2026-10-30）

| 原文件 | 换成了什么 | 证据 |
| --- | --- | --- |
| `probes/term_size.py`（pty 见证） | `check.cx` loop 阶段走私有 tmux 窗格（`probes/tmuxwit.cx`），分别开 100x30 和 5x2 | `term size reads the real tty`、`term size falls back…` 仍 ok |
| `probes/llm_stub.py` | `probes/llm_stub.cx` | `real LLM round trip` ok |
| `probes/agent-stub.py` | `probes/agent-stub.cx` | 6 组请求与 `.py` 对拍，线上字节逐个相同；`agent loop end-to-end` ok |
| `probes/tui_e2e.py` | `probes/tui_e2e.cx` + tmuxwit | 原 9 条断言全保留，另加 2 条 `Ctrl-C then Ctrl-D exits 0`，共 10 条 ok |
| `probes/tui_agent_e2e.py` | `probes/tui_agent_e2e.cx` + tmuxwit + agent-stub.cx | 原断言：屏幕上有 exec 输出、答复和输入文字，转录里有 exec、终结动作和答复，退出码 0。旧版过时：按 C 时代文件表冷编译、找 `you> ` 前缀、靠 Ctrl-C 退出，而且不在任何闸里。现在接进了 `check.cx probes-e2e` |

`http_roundtrip.cx` 原来用 `python3 -m http.server` 当服务端，现在换成探针自己 fork 的文件服务。

移植中发现并修了 3 处缺陷：
- 旧的 `.cx` 桩用 `write` 回包。端口探测连上就关，桩一写就收到 SIGPIPE 死掉；而 unisacc 的 `signal()` 不向内核注册，没法忽略 SIGPIPE。现在三个桩都改用 `send(MSG_NOSIGNAL)`。
- `agent-stub.cx` 的答复多转义了一层（`\\u5b8c`），已改成和 `json.dumps` 同字节。
- `http_roundtrip.cx` 的 `args[4]` 越界写了一格。

## 第一批归档（28 个）

- **已有 `.cx` 移植，活调用方已切到 `.cx`**：`fixtures/context_index_fixture.py`。`check.sh` 第 4 步改用 `.cx`，生成的目录树与原来逐文件相同，golden 仍然一致。
- **已有 `.cx` 移植，`.py` 没有调用方**：`reload_candidate.py`，以及 probes 下的 `agent_loop_bound`、`agent_multi_action`、`context_preview`、`file_observation`、`net_timeout`、`reload_owner`、`reload_session`、`reload_state`。
- **`.cx` 只移植了一部分，`.py` 没有调用方**：`message_send.py`、`reload_launcher.py`。见下面的 BLOCKED 一节。归档的 `.py` 是这两处唯一完整的参考实现。
- **没有 `.cx` 移植、也不在任何门禁里的探针**：probes 下的 `claim_grounding`、`completion_contract`、`evidence_reconciliation`、`managed_handoff`、`owned_session`、`reload_runtime`、`resume_pending`、`task_context`、`task_working_set`、`tool_result`、`rejected_context`、`message_inbox`、`reload_launcher_lock`、`slice_env`、`reload_launcher`，以及它们共用的 `_flat.py`。归档没有让实跑的覆盖变少，但这些合同目前没有 `.cx` 测试，属于**覆盖缺口，不算 PASS**。

## BLOCKED：要走产品慢车道（2026-10-30 在 0.0.43 已装产品上实测）

7 个接口各写一个最小 `.c` 用 `unisacc.com` 编译，全部报 `undefined function` 或 `unknown identifier`。它们在 `include/*.h` 这套随产品携带的运行库里都还没有：

| 接口 | 卡住的 csih 功能 |
| --- | --- |
| `tcgetpgrp`、`socketpair`、`killpg`、`setsid` | `reload_launcher.cx` 的运行循环（`lr_run` 直接 fail） |
| `openat`、`linkat`、`lockf` | `message_send.cx` 的 publish |
| `signal()`（没有 `rt_sigaction`） | csih 收不到外部信号；被 kill 时靠 `csih.sh` 恢复终端；探针桩没法忽略 SIGPIPE |

补这些要改 `include/unistd.h`、`include/signal.h` 等携带头（`signal` 还要写信号返回跳板），会牵动 `kernel/` 生成物和产品重建，属于慢车道，不在 csih 快车道里做。源码里"missing in unisacc 0.0.39"的说法，在 0.0.43 上仍然成立。
