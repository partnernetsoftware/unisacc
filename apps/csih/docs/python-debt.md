# csih 的 Python 依赖登记

2026-10-30 盘点。原有 34 个 `.py`，分三批归档到 `archive/reference-py/apps/csih/`，相对路径不变：第一批 28 个，第二批 5 个，第三批 1 个。**apps/csih 里已经没有 `.py`**，`.cx` / `.h` / `.sh` 里也没有 `python3` 调用。

判定"活调用方"的依据：有 `.cx` / `.h` 把它当路径字符串传给 exec，有 `.sh` 直接调用，或有仓外入口调用。注释里写"C port of X.py"的不算。

## 第三批归档（1 个，2026-10-30）

| 原文件 | 换成了什么 | 证据 |
| --- | --- | --- |
| `deepseek-proxy.py`（本机明文代理，转发到 api.deepseek.com） | `apps/csih/deepseek-proxy.cx`；仓外调用方 `moltbaby/bin/rawcsih` 改为首次把它编译到 `~/.cache/csih/deepseek-proxy`，`.cx` 或 `net.cx` 比缓存新时重编，然后拉起 | 探针 `probes/deepseek_proxy.cx` 12 条，合同 `deepseek-proxy`；另用 rawcsih 的启动段对真 DeepSeek 发了一次请求，返回 200 和一个 completion |

- 行为和 `.py` 一样：只转发 POST `/v1/chat/completions` 和 `/chat/completions`，其它路径 404，GET 501，没有 key 500，上游的状态和正文原样透传，传输失败 502。
- key 的来源和 csih 相同：先看 `DEEPSEEK_API_KEY`，没有再读 `~/env.jsonl`。由 net.cx 只在发往 api.deepseek.com 时加上。
- 移植中查出并修了 `net.cx` 的一个缺陷：明文 http 请求把头和正文拼进一个 8KB 的缓冲，所以正文超过约 8KB 的请求一律被拒（-2002）。
  - 影响：rawcsih 正是让 csih 走 `http://127.0.0.1` 这个明文端点，而 agent 的请求常有 30KB 左右。
  - 修法：现在头和正文分开发送，探针用 200KB 的正文验证过。

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
- **`.cx` 当时只移植了一部分，`.py` 没有调用方**：`message_send.py`、`reload_launcher.py`。两处都已在 2026-10-30 补完（见下面的 BLOCKED 一节）；归档的 `.py` 仍是对照用的参考实现。
- **没有 `.cx` 移植、也不在任何门禁里的探针**：probes 下的 `claim_grounding`、`completion_contract`、`evidence_reconciliation`、`managed_handoff`、`owned_session`、`reload_runtime`、`resume_pending`、`task_context`、`task_working_set`、`tool_result`、`rejected_context`、`message_inbox`、`reload_launcher_lock`、`slice_env`、`reload_launcher`，以及它们共用的 `_flat.py`。归档没有让实跑的覆盖变少，但这些合同目前没有 `.cx` 测试，属于**覆盖缺口，不算 PASS**。

## BLOCKED：要走产品慢车道（2026-10-30 在 0.0.43 已装产品上实测）

7 个接口各写一个最小 `.c` 用 `unisacc.com` 编译，全部报 `undefined function` 或 `unknown identifier`。它们在 `include/*.h` 这套随产品携带的运行库里都还没有：

| 接口 | 卡住的 csih 功能 |
| --- | --- |
| `tcgetpgrp`、`socketpair`、`killpg`、`setsid` | `reload_launcher.cx` 的运行循环（当时 `lr_run` 直接 fail；2026-10-30 已经 cx/sys 移植，见下） |
| `openat`、`linkat`、`lockf` | `message_send.cx` 的 publish（2026-10-30 已经 cx/sys 移植，见下） |
| `signal()`（没有 `rt_sigaction`） | csih 收不到外部信号；被 kill 时靠 `csih.sh` 恢复终端；探针桩没法忽略 SIGPIPE |

补这些要改 `include/unistd.h`、`include/signal.h` 等携带头（`signal` 还要写信号返回跳板），会牵动 `kernel/` 生成物和产品重建，属于慢车道，不在 csih 快车道里做。源码里"missing in unisacc 0.0.39"的说法，在 0.0.43 上仍然成立。

**2026-10-30 更新：前 7 个接口已经有了不改 `include/` 的路。** cx 底层新增了 `cx/sys/posix.cx`，直接用 `__syscall6` 加逐平台调用号，补齐了这批接口：`sy_tcgetpgrp`、`sy_socketpair`、`sy_killpg`、`sy_setsid`、`sy_openat`、`sy_linkat`、`sy_lockf`，另有 `sy_flock`、`sy_getpgid`、`sy_tcsetpgrp`。
- 检查在 `tools/check-cx-sys.cx`，posix 部分全部通过；只在 Linux x86_64 上实跑过，arm64 和 macOS 的调用号写了但没有实跑。
- **`message_send.cx` 的 publish 已移植**（2026-10-30）。
  - cx/sys/posix 另补了 `sy_mkdirat`、`sy_unlinkat`、`sy_dir_names`（按目录 fd 列名字），fs 补了 `sy_fstat`、`sy_read`、`sy_write_all`、`sy_fsync`、`sy_close`。
  - 和归档的 `message_send.py` 逐例对照：published、already-published、内容冲突、锁忙、未知条目、inbox 是符号链接、相对路径这几种情况，回执、rc、原因、目录树、权限、信封字节全部一致。
  - 唯一的差异是会话目录过不了 `trusted()` 时：.py 的原因是 "filesystem error errno=None"，.cx 给的是具体原因，回执和 rc 相同。
  - 探针 `probes/message_publish.cx` 20 条；合同 message-send-publish 由 BLOCKED 转为 IMPLEMENTED。
- **`reload_launcher.cx` 的运行循环已移植**（2026-10-30），对照归档的 `reload_launcher.py`：
  - 前台控制终端检查（`sy_tcgetpgrp(0)==getpgid(0)`）、基线 termios 保存并在每条退出路径上以 TCSAFLUSH 恢复、supervisor flock、`launcher-events.jsonl`（键排序、分隔符与 `json.dumps(sort_keys=True)` 同字节）、socketpair 控制通道、actor 留在启动器的前台进程组并把子端 fd 传过去、构建在独立会话里跑且超时 55s 用 killpg 杀组、FREEZE/RELEASE/COMMIT/ACTIVATE/RETIRE 换手、失败回滚 RESUME、清理失败报 `cleanup unconfirmed`/BLOCKED。
  - cx/sys 为此补了：proc 的 `sy_spawn_inherit`（同进程组、保留一个 fd、exec 失败回传 errno）、`sy_fork_session`（新会话子进程跑函数、stdout/stderr 分管道）、`sy_signal`；posix 的 `sy_tty_get`/`sy_tty_set`/`sy_tty_same`（内核 termios ioctl，不需要 `<termios.h>`）。`tools/check-cx-sys.cx` 87 条通过（Linux x86_64 实跑；arm64/macOS 调用号未实跑）。
  - 构建不再起 python3 子进程，而是在 fork 出的会话首进程里直接调 reload_candidate 的 `build()`，stdout 与 CLI 同一份 JSON。
  - 运行库的 `strerror` 表缺若干项（0.0.43 上 EAGAIN 是 "Unknown error"），启动器按符号自带了 Python 的文字（"Resource temporarily unavailable" 等）。
  - 探针 `probes/reload_launcher_run.cx`：私有 tmux 窗格里跑启动器和真实 csih actor，覆盖初始构建+ACTIVATE、REQUEST 触发并提交的换手（旧 actor 退役、草稿带过去）、第二个启动器被 supervisor 锁拒绝、无 tty 被拒且锁不残留、COMMIT 失败回滚（handoff-failed、candidate-exited、RESUMED，旧 actor 继续收键）、`/exit` 后 rc 0 且 `stty -g` 前后相同。合同 reload-launcher-run 由 BLOCKED 转为 IMPLEMENTED。
  - 已知限制：`reload_candidate.cx` 的 `build()` 用真编译器做不出 csih 候选——每条命令只有 14s（csih 冷编译约 50s，agent_cli 约 28s），而且命令行是 `-run ENTRY -o OUT`，当前 unisacc 把它当作"运行 ENTRY"而不是编译（实测输出 tui 的 usage）。探针因此给启动器一个回放编译器（复制已装的 csih 和探针编译的 agent_cli），冻结、两道闸、回执和校验仍是真跑。
    - **2026-10-30 已修三处**：
      - 命令改成 `compiler ROOT -o OUT`；
      - 构建单独给 55 秒时限，selftest 仍是 14 秒；
      - `apps/csih/sys` 这类模块软链不冻结也不跟随，以前会报 "source symlink"。
    - 真编译器跑通一次：build rc 0，98 秒；verify rc 0。回执在 `out/csih-candidate-check/receipt.txt`。
    - **仍然存在的问题**：一个候选要冷编两个门，合计超过启动器给整次构建的 55 秒。原因是编译缓存按路径算，冻结副本每次路径都不同。所以 `/reload-code` 现编候选在冷缓存下会超时，探针也就继续用回放编译器。要根治，得改成按内容命中缓存，或者放宽启动器的构建上限，这需要一个设计决定。
  - 依赖启动器运行循环的其余归档探针（reload-launcher、reload-launcher-lock、managed-handoff、reload-runtime、task-context、tool-result、rejected-context、message-inbox）不再是 BLOCKED，改记 GAP：还没有 `.cx` 移植，属覆盖缺口，不算 PASS。
- `signal()` 的内核投递不在这批里：它要 `rt_sigaction` 加信号返回跳板，仍然 BLOCKED。
