# csih

产品名 csih：C 语言为基础的超级智能驾具（c-based super intelligence harness）。源码目录和入口文件仍叫 cdsh，tmux 窗口仍是 cdsh-tui。状态在 `~/.csih`；若只有旧的 `~/.csih`，第一次使用时把 `~/.csih` 指过去。

本地 agent。三件工具，经 unisacc.com 以 C 源文件直接 `-run`。配置写在源码里，不进 `moltbaby/bin/llm`。密钥不进源码。

## 为什么是这三件

2852 次真实 dsh 调用里：`bash` 2505（87.8%），其中 `cd` 2107（84.1%）；`edit` 149（5.2%）；`read` 128（4.5%）；`write` 很少。mind / 子代理 / 作业合计约 2%。所以对外只有：

| 名字 | 做什么 |
| --- | --- |
| `file` | 读、写、精确替换 |
| `exec` | shell，带一句 `why`。纯 `cd` 或 `cd <dir>` 记住工作目录；`cd foo && bar` 不记 |
| `mind` | 只动两页，都在 `$HOME/.csih/`：`思维树.md`（markdown-tree-dag）和 `记忆宫殿.md`（mermaid-flowchart-memory-palace）。不跟工作目录走 |

`answer` 结束这一轮，不是第四件工具。不要用 `file` 或 `exec` 去改那两页。

`exec` 的 JSON 要带 `why`，一句中文，说明这条命令做什么。界面默认只画这句，前缀是 `exec[展开]>`。点这一行（鼠标左键）变成 `exec[收缩]>`，并在下一行露出命令，命令仍只占一行。再点收回。工具输出的 `→` 行不收进这个开关。

mind 的 JSON：`{"act":"mind","op":"add|read","target":"tree|palace","text":"..."}`。`which` 仍接受。思维树页是 markdown-tree-dag（缩进树，`├──` / `└──` 为包含，`══>` 为跨枝依赖）。记忆宫殿页是 mermaid-flowchart-memory-palace（一段 mermaid flowchart，边表示树里放不好的关系）。`op=add` 只追加一行短注，仍以 `- ` 开头，不改写整页。目录不存在时建 `$HOME/.csih`（0750）。已有文件不按模板重写（用 `access`，不用短读判断缺失）。

## 插件

对外名字、帮助、kind 走 `plugin_t {name, help, kind}`。kind 与内部 `ACT_*` 对齐：exec=1，file 读/写/改=2/3/4（对外都叫 `file`），answer=5，mind=8。提示词由目录生成：`Catalog:` 加上各行帮助，再加系统句「只用目录里点名的工具」。

新能力是目录旁边的一个 `.c`，登记后按名字调用。`plugin_ping.c` 是例子：`unisacc.com plugin.c plugin_call.c plugin_ping.c call ping` → `ping:ok`。在 unisacc 0.0.28 F1 之前，constructor 不跑，登记必须是显式调用（`csih_plugin_boot`）。引号 `#include` 只搜该 `.c` 所在目录。文件列表漏了实现时，unisacc 的拒绝句是 “not covered: library signature resource or duplicate definition”，意思是缺 `.c`，不是重复定义。

## 怎么跑

不要 `gcc` / `cc` / `clang`。macOS 对 APE 直接 `execve` 得到 ENOEXEC，所以用 `/bin/sh` 当加载器。开发期入口是同目录的 `cdsh.sh`：`unisacc.com cdsh.c` 加上文件表。`cdsh.c` 含 `tui.c`，不再单独占一个翻译单元。`~/.csih/run.c` 读密钥后执行 `cdsh.sh agent`。

自迭代的验证环在 `docs/self-iter.md`：改源文件后由 harness 用同一条 `u_spawn` 跑受影响的 `selftest`，红则拒绝 `answer`；整份 `check.c` 绿了才重启 TUI。

自测（必须带上 `plugin.c`）：

```
unisacc.com agent.c agent_cli.c file.c edit.c shell.c json.c session.c net.c plugin.c selftest
unisacc.com plugin.c plugin_cli.c selftest
```

TUI 需要 tty：`unisacc.com ~/.csih/run.c`。它从 `~/env.jsonl` 的 `deepseek.api_key` 导出 `DEEPSEEK_API_KEY`，转录写到 `~/.csih/tui.jsonl`。DeepSeek 不接受裸的 `role=tool`，磁盘上的工具记录在送出前改写成用户消息，前缀 `[tool]`。`answer` 之后先再问一次继续或停止；下一轮会顶到 `MAX_ROUNDS` 时不再问。红的 slice 拒绝 `answer`。空的 `messages` 是 HTTP 400；缺密钥是 401。非 2xx 时原因里带响应体片段，并写入 `~/.csih/last-http.txt`。

解析不了的一步显示模型原文（压成一行，大约 200 字），不显示 “(unparsed)”。空内容显示 `(empty)`。

`json.c` 解析，并用 `json_escape` 写字符串体（不加外层引号）。非法 UTF-8 字节丢掉，转义或一个码位写不下就停，不留下半个 `\` 或半个码位。请求体仍由 `agent.c` 拼，转义走这一个函数。

## 界面（现状，不是下一刀）

整帧重画，改过的行末尾清到行尾（ESC `[K`）。帧变矮时清下方还没做。列宽夹到 200。空闲状态不因 tick 重画。按键队列每帧排空。请求进行中只读队列里还有空位的键，忽略回车，避免再套一层 agent。

日志视口按终端实际高度取值，最多 20 行，条数不够就补空行，多了只留最后 20 行。终端剩余高度不够 20 时按能放下的行数缩小。展开的 exec 命令行算进这 20 行。输入行下方先是 `Ctrl-C 清空 · Ctrl-D 退出 · Enter 发送`（请求中时换成秒、词元、上传字节、已收字节；这次响应还没有 usage 时不沿用上一轮的 hit/miss。输入已空再按 Ctrl-C 则改成 `要退出请按 Ctrl-D`），再一行 `错误>`。没有错误时是 `错误> -`。harness 看到的 `error:`、`not covered`、`not responding`、非 0 的 `exit=` 写在这一行，不插进画面中间。内核直接写到 tty 的同一句，下一拍整帧重画盖掉。系统区上沿默认是 `-<系统提示词>[展开]>` 再接 `-` 铺满剩余列，下面不画正文。点这一行变成 `-<系统提示词>[收缩]>`，展开 `agent_model_rules()` 折成的 10 行，滚轮只在这 10 行里上下滚。不再加 `系统>`。一次粘贴是一条消息：括号粘贴里的换行留在输入里，结束标记才发送；同一拍后面还有键的换行同样不发送。输入行把换行画成 `⏎`。画面上的 `/Users/<用户名>/` 和 `/home/<用户名>/` 画成 `~/`，用户名取自 `$HOME`。送给模型和工具的路径不改。送去模型的 system 更长：先是 `Catalog:` 和插件目录，再接这整段。tmux 窗口列表不在 system 里，附在当次 user 消息末尾。思维树和记忆宫殿的上沿是 `-<思维树>` 接 `-`，中间一列 `┬`，右侧 `-<记忆宫殿>` 再接 `-`。下面不再单独放标题。左右各 8 行，中间一列是 `│`。左半是 `~/.csih/思维树.md`，右半是 `~/.csih/记忆宫殿.md`，各取最后 8 行有内容的行。成对的 ` ``` ` 围栏和单独的 `flowchart` 方向行不画。文件本身不改。缺文件显示 `(无)`，空文件显示 `(空)`。行缓冲按每列 3 字节，填充按显示列，避免把 `─` 或汉字从中间切断、行尾留下旧字。下一帧重画时重读。不引入 curses。

`/exit` 与 `/quit` 退出。输入有字时 Ctrl-C 清空，不退出。输入已空时 Ctrl-C 只提示按 Ctrl-D；这时 Ctrl-D 才退出。请求进行中且输入已空，Ctrl-C 仍只取消这一轮。工作目录持久化超出纯 `cd`、`~/.csih/commands/*.md`，都还没做。`/goal` 设定或显示当前目标，`/loop` 在有限次数内把该目标再提交。两者都不发起模型回合。

## 远景

能稳定当作 harness 使用之后才做 `cdsh.com`。仓位迁移现在开始：正在用的源仍是 moltbaby 的 `skills/llm/dsh/cdsh`，目标是 unisacc 的 `apps/cdsh`。那边 selftest 绿、引用改完之前，不删这边。

发布两种形式：

| 形式 | 是什么 |
| --- | --- |
| `cdsh.sh` | 开发期。一条命令：`unisacc.com cdsh.c` 加上文件表。脚本只是把这条命令包短 |
| `cdsh.com` | 交付期。同一份源做成一个跨架构可执行文件。现在还没有这个文件 |

代码从 moltbaby 仓迁到 unisacc 仓新建的 `apps/cdsh`。这是真正的应用目录。`examples/apps/` 仍是演示，不放 cdsh。

迁完后修正 moltbaby 的引用，包括 `skills/`（含 `skills/unisacc/SKILL.md`）和 `bin/llm` 相关（`bin/llm` 里 cdsh 指向 `bin/rawcdsh`，`bin/rawcdsh` 的 `CDSH_DIR` 与源文件表）。仓内写死的 `/dsh/cdsh`（`suite.c`、`agent.c`）和仓外 `~/.csih/run.c` 一起改。配置仍写在源码里，不把密钥或配置放进 `bin/llm`。

### 异步 I/O（规划参考）

走极简。不引入 libuv，也不自造一套事件库来换掉现有的泵。libuv、Go context、Rust Future 只提供思路，不提供要落地的结构。不把 async/await 加进编译器。

已有的异步就够当底座：`net_async_begin` 发起，帧循环调用 `net_async_pump`，`net_async_end` 收尾。取消是 `net_stop`，截止是 `NET_TOTAL_SEC`。文件、exec、mind 保持同步。

以后若改网络，只在这条泵上做小步，例如少建几次 curl 句柄，或让正文更早出现在帧上。每一步都要能单独退回，相关 selftest 仍绿。做不到这一点就不做。

下面这些不属于 cdsh 的顺序：新的跨平台事件库、线程池、六目标探针、把模型调用换到另一套队列上。出处只供以后想到类似问题时翻一下。

- libuv 设计：https://docs.libuv.org/en/v1.x/design.html
- Windows IOCP：https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports
- Go context：https://go.dev/blog/context
- Rust Future：https://doc.rust-lang.org/std/future/trait.Future.html
- 不要堵住事件循环：https://nodejs.org/en/learn/asynchronous-work/dont-block-the-event-loop

## 探针

`probes/` 里是可运行的检查，不是第二份规格。平台差（HTTPS、多文件、POSIX）已在 unisacc 0.0.26 里；cdsh 不再自带一层按操作系统分叉的封装。constructor 不执行记在 unisacc `plans/v0.0.28.md` F1。
