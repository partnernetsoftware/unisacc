# csih

产品名 csih：C 语言为基础的超级智能驾具（c-based super intelligence harness）。源码目录是 unisacc 的 `apps/csih`，入口是 `csih.sh` / `csih.c`，启动器是 moltbaby 的 `bin/rawcsih`。tmux 窗口是 csih-tui。状态在 `~/.csih`；若只有旧的 `~/.cdsh`，第一次使用时把 `~/.csih` 指过去。

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

日用（2026-10-29）：`sh apps/csih/csih.sh prepare` 一次（源 `*.cx *.h` 摘要变才重建 `out/csih-e2e-bin/csih`，未变秒返）；之后 `csih.sh`（默认 agent）或 `csih.sh run` 秒开，在调用者当前目录工作。产物缺或陈旧时具名报错，`CSIH_COLD=1` 才冷 `-run`。密钥：`DEEPSEEK_API_KEY` 优先，否则 `~/env.jsonl` deepseek 行的 `api_key`，再退 `key`；`csih.sh keyprobe` 只报长度与来源。模型默认 `deepseek-chat`，`CSIH_MODEL` 覆盖。下面是迁移前的历史说明，`.c` 路径已换成 `.cx`。

不要 `gcc` / `cc` / `clang`。macOS 对 APE 直接 `execve` 得到 ENOEXEC，所以用 `/bin/sh` 当加载器。开发期入口是同目录的 `csih.sh`：`unisacc.com csih.c` 加上文件表。`csih.c` 含 `tui.c`，不再单独占一个翻译单元。`~/.csih/run.c` 读密钥后执行 `csih.sh agent`。`bin/llm` 和 `bin/rawcsih` 只认 `CSIH_*`。

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

日志视口按终端实际高度取值，最多 20 行，条数不够就补空行，多了只留最后 20 行。终端剩余高度不够 20 时按能放下的行数缩小。展开的 exec 命令行算进这 20 行。输入行下方先是 `Ctrl-C 清空 · Ctrl-D 退出 · Enter 发送`（请求中时换成秒、词元、上传字节、已收字节；这次响应还没有 usage 时不沿用上一轮的 hit/miss。输入已空再按 Ctrl-C 则清掉结果/错误行并改成 `要退出请按 Ctrl-D`），再一行 `错误>`。没有错误时是 `错误> -`。harness 看到的 `error:`、`not covered`、`not responding`、非 0 的 `exit=` 写在这一行，不插进画面中间。内核直接写到 tty 的同一句，下一拍整帧重画盖掉。系统区上沿默认是 `-<系统提示词>[展开]>` 再接 `-` 铺满剩余列，下面不画正文。点这一行变成 `-<系统提示词>[收缩]>`，展开 `agent_model_rules()` 折成的 10 行，滚轮只在这 10 行里上下滚。不再加 `系统>`。一次粘贴是一条消息：括号粘贴里的换行留在输入里，结束标记才发送；同一拍后面还有键的换行同样不发送。输入行把换行画成 `⏎`。画面上的宿主 home 前缀画成 `~/`，用户名取自 `$HOME`。送给模型和工具的路径不改。送去模型的 system 更长：先是 `Catalog:` 和插件目录，再接这整段。tmux 窗口列表不在 system 里，附在当次 user 消息末尾。思维树和记忆宫殿的上沿是 `-<思维树>` 接 `-`，中间一列 `┬`，右侧 `-<记忆宫殿>` 再接 `-`。下面不再单独放标题。左右各 8 行，中间一列是 `│`。左半是 `~/.csih/思维树.md`，右半是 `~/.csih/记忆宫殿.md`，各取最后 8 行有内容的行。成对的 ` ``` ` 围栏和单独的 `flowchart` 方向行不画。文件本身不改。缺文件显示 `(无)`，空文件显示 `(空)`。行缓冲按每列 3 字节，填充按显示列，避免把 `─` 或汉字从中间切断、行尾留下旧字。下一帧重画时重读。不引入 curses。

`/exit` 与 `/quit` 退出。输入有字时 Ctrl-C 清空，不退出。输入已空时 Ctrl-C 清结果/错误行并提示按 Ctrl-D；这时 Ctrl-D 才退出。请求进行中且输入已空，Ctrl-C 仍只取消这一轮。工作目录持久化超出纯 `cd`、`~/.csih/commands/*.md`，都还没做。`/goal` 设定或显示当前目标，`/loop` 在有限次数内把该目标再提交。`/reload` 空闲时清掉从 `~/env.jsonl` 读入的密钥静态缓冲（页每帧已重读），请求进行中只回一行拒绝。三者都不发起模型回合，状态只进画面日志、不写 `tui.jsonl`。

## 远景

能稳定当作 harness 使用之后才做 `csih.com`。正在用的源是 unisacc 的 `apps/csih`。moltbaby 的 `skills/llm/dsh/csih` 已归档到 `archived/skills/llm/dsh/csih`。

发布两种形式：

| 形式 | 是什么 |
| --- | --- |
| `csih.sh` | 开发期。一条命令：`unisacc.com csih.c` 加上文件表。脚本只是把这条命令包短 |
| `csih.com` | 交付期。同一份源做成一个跨架构可执行文件。现在还没有这个文件 |

代码从 moltbaby 仓迁到 unisacc 仓的 `apps/csih`。这是真正的应用目录。`examples/apps/` 仍是演示，不放 csih。

moltbaby 里的命令已经是 `bin/rawcsih` 和 `csih.sh`。`bin/llm` 的工具名是 `csih`。仓外 `~/.cdsh/run.c`（`~/.csih` 指到同一文件）执行的是这份 `csih.sh`。配置仍写在源码里，不把密钥或配置放进 `bin/llm`。

### 异步 I/O（规划参考）

走极简。不引入 libuv，也不自造一套事件库来换掉现有的泵。libuv、Go context、Rust Future 只提供思路，不提供要落地的结构。不把 async/await 加进编译器。

已有的异步就够当底座：`net_async_begin` 发起，帧循环调用 `net_async_pump`，`net_async_end` 收尾。取消是 `net_stop`，截止是 `NET_TOTAL_SEC`。文件、exec、mind 保持同步。

以后若改网络，只在这条泵上做小步，例如少建几次 curl 句柄，或让正文更早出现在帧上。每一步都要能单独退回，相关 selftest 仍绿。做不到这一点就不做。

下面这些不属于 csih 的顺序：新的跨平台事件库、线程池、六目标探针、把模型调用换到另一套队列上。出处只供以后想到类似问题时翻一下。

- libuv 设计：https://docs.libuv.org/en/v1.x/design.html
- Windows IOCP：https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports
- Go context：https://go.dev/blog/context
- Rust Future：https://doc.rust-lang.org/std/future/trait.Future.html
- 不要堵住事件循环：https://nodejs.org/en/learn/asynchronous-work/dont-block-the-event-loop

## 工具链约束（硬性，2026-10-10）

- 唯一允许的第三方运行时是 unisacc（`unisacc.com`）。禁止新增或保留 Python、Node、Bash 等脚本语言实现的产品代码、探针、门禁、夹具生成器。
- 源码只用 `.c` / `.cx` / `.h`，其中 `.cx` 是模块化 provider。禁止 `.py`、`.sh` 进入仓库的构建、测试或运行路径。
- 构建与运行统一经 unisacc：`-o` 构建后运行，不依赖 run 模式丢弃 `.cx` 与全局 `-include` 的行为。
- 宿主 `cc` 不作为门禁依赖；若 unisacc 缺能力，向 unisacc 维护者（cc-unisacc）反馈，等修复，不以外部工具绕过。
- 现有 `.sh` 与 `.py` 逐步淘汰：每个文件须有对应 `.c` 替代后删除。

## 探针

`probes/` 里是可运行的检查，不是第二份规格。平台差（HTTPS、多文件、POSIX）已在 unisacc 0.0.26 里；csih 不再自带一层按操作系统分叉的封装。constructor 不执行记在 unisacc `archive/plans/v0.0.28.md` F1。

## Harness 命令 P0（2026-10-10）

agent 模式新增 `/help`、`/status`、`/clear`，均只在本地处理、不请求模型、不写 journal、不进入发送历史。`/clear` 忙时立即拒绝；闲时清日志、错误与输入/历史浏览状态，保留 pending 队列、goal、loop 与预算、mind、模型上下文及磁盘状态。反馈显示保留的待发送条数。
命令仅识别单行 `/[A-Za-z][A-Za-z0-9_-]*` 后接空格或行尾；路径、多行文本保持普通输入。未知命令本地报错；`//foo` 发送字面 `/foo`。help 如实描述 `/goal` 与 `/loop` 现有行为，不承诺尚未实现的参数语义。

## UI/UX 与功能规划（2026-10-29）

按「每天打开就能用」排序。每条做完都要 tmux 实窗过一遍，并有 selftest 断言，不靠肉眼。

### P0 完成判定别再误伤（实测最痛）

- 现状：工具链全部成功，模型答 DONE，仍报 `✗ agent failed`。原因有二：一是模型漏了 `claims`，二是 `observation_refs` 要模型自己数 UTF-8 字节偏移，LLM 数不准。实测根因是模型把 ref 的 `id` 写成自编的 `o1/o2`。已修：提示词写明 id 必须是 evidence ID；缺 claims 或 id 未知时有界重答（共用 `parse_fail`，最多 2 次），并列出当前可用 ID；字节偏移由 harness 收拢到正文内的码位边界（`at_ref_span`）。修后 tmux 实测 write→read→sh 一次「语义接受」。下面是根治方向。
- 方向：把 grounding 交给 harness。模型只给 `evidence` ID 加一段原文引用（`quote`），由 harness 在该工具记录正文里定位出字节范围；定位不到才拒绝。验收强度不变，模型负担大减。
- 文案：「未验证」不等于「失败」。结果行分三种：`✓ 已验收`、`◐ 做完未验证（原因）`、`✗ 失败（原因）`，不再一律用 `agent failed`。**已落地（2026-10-29）**：`tui_result_row`，acceptance=unverified、outcome=partial/unverified 或 reason 以 `unfinished:` 开头记 `◐`，并进错误行；其余记 `✗ 失败`。selftest：`unverified row uses ◐`、`failed row uses ✗ 失败`。

### P1 交互手感

- 流式正文：answer 的 text 边收边画，不要等整包回来；忙时状态行的秒、词元已有，再补一个「当前在做什么」（等模型 / 跑工具 名字）。
- 工具行：默认一行摘要 + 退出码 / 字节数；失败自动展开最后几行输出。`[展开]` 支持键盘（Tab 选中，Enter 切换），不只靠鼠标。**第二批已落地（2026-10-29）**：修了原有 bug——agent 在工具行后先发 `  evidence ID`，它自成一行，后面的 `│` 正文挂不上，导致正文从不折叠、`[展开]` 形同虚设；现在 evidence 归入该行正文。收起时一行 `why · 命令 · 命令退出 N`（取正文首条状态）；`操作失败`/非 0 `命令退出` 自动展开；展开时正文逐行各带 `  │ `，最多 8 行，不再用 `⏎` 连成一行。selftest：`failed tool row opens by itself`、`closed tool row is one line with status`、`open tool body is one line per row`。键盘切换未做。
- 输入：多行编辑（Alt-Enter 换行）、Ctrl-A/E/W/U、历史按前缀搜索（Ctrl-R）。**已落地**：Ctrl-U 清整行、Ctrl-W 删末尾空白加一个词（输入无光标，Ctrl-A/E 等光标模型落地后再做）。selftest：`ctrl-u clears the line`、`ctrl-w deletes one word`。
- 中断：Esc 等同忙时 Ctrl-C（取消本轮），并在日志写一行「已取消，工具已执行的部分保留」。**第二批已落地**：新增 `TERM_KEY_ESC`。忙时 Esc 取消本轮并提示「已执行的工具保留」；闲时清输入。修了 `term_read_keys` 原有的问题：缓冲末尾单独的 ESC 一直留在 pending 里等下一个键，导致 Esc 没有反应；现在 25ms 内没有后续字节就当作 Esc 键（`ESC [` 序列照旧拼接）。term 测试中「裸 ESC」一条改为期望 `TERM_KEY_ESC`，仍要求不能吞键。selftest：`esc cancels a busy turn`、`esc clears idle input`。
- 头部一行：模型名 · cwd · 本轮轮次/预算 · key 来源（env/file），出错时一眼看出是 key、网络还是模型。**已落地**（轮次/预算未加）：`csih · agent · deepseek-chat · ~/目录 · key=file|env|missing`，home 前缀画成 `~`，按列宽截断。selftest：`title shows model, cwd and key source`；tmux 实窗已见。

### P2 功能

- 会话：`/new` 开新上下文、`/resume` 列最近转录接着做；转录按 cwd 分目录，不再全写一个 `~/.cdsh/tui.jsonl`。
- 权限：`exec` 分「只读 / 写工作区 / 任意」三档，危险命令（`rm -rf`、`git push`、`reset`）先确认一键 y/n。
- `/model`、`/cwd` 本地命令；`/cost` 显示本会话 token 与命中率。**第二批已落地**：`/model` 显示当前模型，`/model NAME` 设本会话模型（单词、<96 字节，忙时拒绝），请求与标题都随之变化；`/cwd` 显示工作目录；两者都列入 `/help`，都不发起模型回合。selftest：`/model sets the session model`、`/cwd shows the working directory`、`help lists /model and /cwd`。`/cost` 未做。
- **第三批已落地（2026-10-29）**：
  - 「无法解析」改为 `↻ 模型回复不是单个动作（一次多步或夹带文字），已让它重发`（重发本来就由 `parse_fail` 有界驱动）。
  - `/cost`：本会话请求次数、输入词元、缓存命中及其比例、输出词元。每个响应只计一次（`net_req_counted`）。同时修了 usage 解析的原有 bug：DeepSeek 的 usage 里嵌着 `prompt_tokens_details{...}`，用首个 `}` 当结尾会漏读 hit/miss，现在按括号深度匹配。实测：`请求 6 次 · 输入 47754（缓存命中 6016，12%）· 输出 1180 词元`。
  - 危险命令门：`rm -rf`、`rm -r `、`git push`、`git reset --hard`、`git clean`、`git checkout -- `、`mkfs`、`dd if=`、`shutdown`、`reboot` 等先拦截，告诉模型在 answer 里说明后等用户 `/allow`；一次 `/allow` 只放行一条。tmux 实测：拦截 → 模型说明风险 → `/allow` → 删除成功并「语义接受」。这是关键字门，不是沙箱。
  - 空输入时按 Tab 展开或收起最近的工具行；`/help` 新增一行按键说明。
  - selftest：`usage with nested details sums per response`、`/cost shows session usage`、`dangerous exec is blocked until /allow`、`/allow lets exactly one through`、`tab toggles the newest tool row`；agent_cli selftest 全部通过。
- **第四批已落地（2026-10-29）**：输入框光标。`back` 记光标离行尾的字节数（用时夹紧，旧代码直接改写 input 也不会越界）；左右键按码位移动，中文算一步；Ctrl-A/Ctrl-E 到行首/行尾；字符插入、退格、Ctrl-W 都以光标为准；光标不在行尾时画 `▏`，不往帧里写终端转义。提交和浏览历史时光标回到行尾。selftest：`cursor moves and edits mid-line`；tmux 实测：`运行 echo ▏世界` → 插入 → `用 sh ▏运行 echo 你好世界`。
- **第五批已落地（2026-10-29）**：`/new` 把当前转录改名为 `PATH.<秒>.jsonl` 归档（不删除），下一轮从空历史开始；转录不存在时报「已是空上下文」；忙时或 owned 会话拒绝。selftest：`/new archives the transcript and starts fresh`。tmux 实测：先告诉模型暗号，`/new` 后再问，模型答「不知道」。
- **第六批已落地（2026-10-29）**：
  - `/resume` 列出最近 9 个归档（多久以前 + 第一句用户话），`/resume N` 先把当前转录归档，再换回第 N 个。归档名同一秒内冲突时秒数加一，绝不覆盖已有归档（`/new` 和 `/resume` 连续操作时会撞名）。首句只读文件开头 64 行（`file_read` 会拒绝大文件），按码位截断。selftest：`/resume lists and swaps archived transcripts`。tmux 实测：换回旧转录后，模型答出了 `/new` 之前给的暗号。
  - 修 mind add 写成 `- - 文本` 的问题：模型自带的 `- ` 前缀先去掉。tmux 实测写入 `- 测试条目XYZ`（测试条目已从 `~/.csih/记忆宫殿.md` 删除）。
  - `/help` 原来一行超过 240 字节的日志上限会被截断，拆成两行。
  - 注意：`/new` 只清转录，不清 mind 两页；模型若把内容存进宫殿，新上下文仍读得到。
- **第七批已落地（2026-10-29）**：输入超出一行宽度时，显示窗口跟着光标滑动，左边被挤出的部分画成 `…`。原来超宽时尾部被直接截掉，正在打的字反而看不见；而且折叠缓冲只有 600 字节，长输入在这一步就丢了尾部，现在放大到能装下整个输入。selftest：`long input scrolls to keep the cursor visible`；tmux 实测中文长句：行尾可见，Ctrl-A 后回到行首。
- **第八批已落地（2026-10-29）**：成功行从 `↻ 本轮结束 rounds=1 actions=3 · 已答 · semantic acceptance (work): …` 改为 `✓ 已验收 · 本轮结束 1 轮 3 步 · 理由`（未经评审接受的写 `✓ 已答`，没有答复的写 `✓ 已结束（未作答）`），与 `◐`/`✗` 一起凑齐三态。保留「本轮结束」四字，因为 `probes/reload_runtime.py` 靠它判断一轮结束。selftest：`accepted row reads ✓ 已验收 and keeps 本轮结束`；tmux 实测：`✓ 已验收 · 本轮结束 1 轮 1 步 · exec ran date +%Y, exit 0, …`。
- 下一批候选：正文流式显示；转录按 cwd 分开；exec 三档权限；P0 根治（引用原文，由 harness 定位）。
- **第十批已落地（2026-10-29）**：忙碌行。原来执行命令时一直显示「等模型」；现在收到 exec 行（命令执行前发出）就显示 `跑命令 <命令>`，下一次请求在途（`net_in_flight()`）时切回「等模型」。文案从 `请求中 等模型 3秒 7745词元 上传 32839字节 已收 574字节 hit 1152 miss 6561` 改为 `⏳ 等模型 3秒 · 7745词元 · ↑32.1KB 已收574B · 缓存命中15%`，保留「已收」二字（已有断言依赖它）。selftest：`busy line names the running command and reads sizes`；tmux 实测已见切换。原来 exec 同步执行时秒数不走、界面不刷新，第十一批已解决。
- **第十三批已落地（2026-10-29）**：去噪。未通过时答复不再显示两遍（解析到答复时已显示为 `完成声明:/失败声明:` 行）。界面事件里的英文改成中文：`↻ round-end decision` → `↻ 交给评审核对`，`→ continue` → `→ 评审：继续`，回复次数用完、自测红、动作次数到上限三种停止原因，以及 `(no output)` → `(无输出)`，还有我前几批加的两条重答提示。`peer mail not delivered` 有探针断言，保留原文。
- **第十二批已落地（2026-10-29）**：P0 的最小根治。提示词和重答提示都告诉模型：数不准偏移时写 `start=0,end=0`，表示整段记录正文，由 harness 取原文（`at_ref_span` 本来就把空区间扩成整段）。评审看到的仍是真实原文，验收规则不变。回归：write→read→sh 连跑两次，都是一次就 `✓ 已验收`，没有触发重答。`check.sh` 全绿（rc=0）。
- **第十一批已落地（2026-10-29）**：长命令可见、可取消。
  - exec 仍是同步的，但 `shell_run_in` 的读循环改为每 200ms 醒一次，调用 `shell_set_tick` 注册的回调；总截止时间不变。
  - tui 注册 `tui_during_shell`：复用原来写好却从未接线的 `tui_during_net`（`net_tick_st` 从未赋值），读键并重画。命令执行期间秒数照走，按 Esc 或 Ctrl-C 返回 1，命令被杀掉。
  - 子进程 `setpgid(0,0)`，超时和取消都用 `kill(-pid)` 杀整个进程组，`sh -c` 后台起的子进程不会残留。
  - 工具结果写 `exit=cancelled (user pressed Esc; command killed)`，状态行写「命令已取消（Esc）」，与超时区分开。
  - shell selftest 新增：tick 要求停止 → 取消，且发生在第 3 次 tick 而不是截止时；后续命令没有执行；后台子进程随进程组退出（`pgrep` 计数为 0）。tmux 实测 `sleep 20`：秒数从 1 走到 5，Esc 后 2 秒内显示「已取消」，没有残留进程。
- **第九批已落地（2026-10-29）**：评审提示词末尾加一句，要求 reason 用简体中文简述，验收规则本身不变。agent selftest 全部通过。tmux 实测：`✓ 已验收 · 本轮结束 1 轮 1 步 · 执行记录显示 uname -s 输出 Linux、退出码 0，工作目录一致，声称与实际记录相符。`
- 自迭代：改 `apps/csih` 源后 `/reload` 自动跑 `csih.sh prepare`，绿了再换进程（复用现有 reload 链）。

### P3 以后

- `csih.com` 交付形式、跨平台；子代理；插件目录热加载。没有 P0–P2 的稳定前，不开工。
