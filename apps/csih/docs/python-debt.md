# csih 的 Python 依赖登记

2026-10-30 盘点。原有 34 个 `.py`：28 个归档到 `archive/reference-py/apps/csih/`（相对路径不变），6 个仍有活调用方，留在原处并登记如下。另有一处不落文件的 Python 依赖。

判定"活调用方"的依据：有 `.cx` / `.h` 把它当路径字符串传给 exec，有 `.sh` 直接调用，或有仓外入口调用。注释里写"C port of X.py"的不算。

## 仍在用的（6 个文件 + 1 处模块依赖）

| 文件 | 调用方 | 为什么还没去掉 | 去掉的条件 |
| --- | --- | --- | --- |
| `apps/csih/deepseek-proxy.py` | 仓外 `moltbaby/bin/rawcsih`（`python3 "$CSIH_DIR/deepseek-proxy.py"`，明文代理跳到 api.deepseek.com） | `net.cx` 已经能直连 https，这个跳板对 csih 本身已不需要；但 rawcsih 还在拉起它，而 rawcsih 不在本仓 | rawcsih 改成直接用 `https://api.deepseek.com` 端点之后归档 |
| `apps/csih/probes/agent-stub.py` | `probes/agent_roundtrip.cx`、`probes/tui_agent_e2e.cx`（`"probes/agent-stub.py"`） | 已有 `probes/agent-stub.cx`，但两个调用方还在 exec python3 | 调用方改成 `sh unisacc.com probes/agent-stub.cx -- PORT`，并用同一组请求做对照 |
| `apps/csih/probes/llm_stub.py` | `probes/llm_roundtrip.cx`（`"probes/llm_stub.py"`） | 已有 `probes/llm_stub.cx`，调用方没切 | 同上 |
| `apps/csih/probes/tui_e2e.py` | `probes/tui_e2e.cx` | pty 见证要 `openpty`，不知道 unisacc 运行库有没有这个接口 | 有 openpty 接口，或改成像 `probes/net_retry.cx` 那样用私有 tmux 驱动 |
| `apps/csih/probes/tui_agent_e2e.py` | `probes/tui_agent_e2e.cx` | 同上，另外还依赖 agent-stub.py | 同上 |
| `apps/csih/probes/term_size.py` | `check.cx`（`"probes/term_size.py"`） | 在 pty 里读窗口尺寸，原因同上 | 同上 |
| （无文件）`python3 -m http.server` | `probes/http_roundtrip.cx` | 拿它当被测 GET 的服务端 | 换成 `.cx` 桩（可以参照 `probes/retry_stub.cx`） |

## 已归档的（28 个）

全部在 `archive/reference-py/apps/csih/` 下，路径与原来一致。

- **已有 `.cx` 移植，且活调用方已切到 `.cx`**：`fixtures/context_index_fixture.py`。`check.sh` 第 4 步改用 `.cx` 后，生成的目录树与 `.py` 生成的逐文件相同，context-index golden 仍然一致。
- **已有 `.cx` 移植，`.py` 没有调用方**：`reload_candidate.py`，以及 probes 下的 `agent_loop_bound`、`agent_multi_action`、`context_preview`、`file_observation`、`net_timeout`、`reload_owner`、`reload_session`、`reload_state`。
- **`.cx` 只移植了一部分，`.py` 没有调用方**：`message_send.py` 和 `reload_launcher.py`。
  - `message_send.cx` 的 publish 因为缺 `openat/linkat/lockf` 还是 BLOCKED。
  - `reload_launcher.cx` 的运行循环因为缺 `tcgetpgrp/socketpair/killpg/setsid` 还是 BLOCKED。
  - 归档的 `.py` 是这两处唯一完整的参考实现。补齐时对照它们，不要把它们放回活路径。
- **没有 `.cx` 移植，也没有任何门禁在跑的探针**：probes 下的 `claim_grounding`、`completion_contract`、`evidence_reconciliation`、`managed_handoff`、`owned_session`、`reload_runtime`、`resume_pending`、`task_context`、`task_working_set`、`tool_result`、`rejected_context`、`message_inbox`、`reload_launcher_lock`、`slice_env`、`reload_launcher`（probes 下的），以及它们共用的 `_flat.py`。
  - 归档前这些就不在任何门禁里，所以归档没有让实跑的覆盖变少。
  - 但它们描述的合同目前没有 `.cx` 测试，属于**覆盖缺口，不算 PASS**。
  - 其中好几个测的是归档了的 `reload_launcher.py` / `reload_candidate.py` 本身。
