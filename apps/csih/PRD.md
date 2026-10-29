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

`/exit` 与 `/quit` 退出。输入有字时 Ctrl-C 清空，不退出。输入已空时 Ctrl-C 清结果/错误行并提示按 Ctrl-D；这时 Ctrl-D 才退出。请求进行中且输入已空，Ctrl-C 仍只取消这一轮。工作目录持久化超出纯 `cd`、`~/.csih/commands/*.md` 当时都还没做（commands 已在第三十五批落地）。`/goal` 设定或显示当前目标，`/loop` 在有限次数内把该目标再提交。`/reload` 空闲时清掉从 `~/env.jsonl` 读入的密钥静态缓冲（页每帧已重读），请求进行中只回一行拒绝。三者都不发起模型回合，状态只进画面日志、不写 `tui.jsonl`。

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
- 头部一行：模型名 · cwd · 本轮轮次/预算 · key 来源（env/file），出错时一眼看出是 key、网络还是模型。**已落地**（轮次/预算未加）：`csih · agent · deepseek-chat · key=file|env|missing · ~/目录`，home 前缀画成 `~`，按列宽截断（第十四批把目录挪到最后：窄窗口下截掉的是最长的目录，而不是 key；tmux 宽 70 时原先截成 `key=fil`）。selftest：`title shows model, cwd and key source`；tmux 实窗已见。

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
- 候选清单的去向（2026-10-29 更新）：转录按 cwd 分开 → 第十六批；exec 三档权限 → 第十五批；P0 根治 → 第十二、十七批。**正文流式显示：REJECT**。模型每次回复都是一个完整的 JSON 动作对象，正文只是其中 `text` 字段的值；边收边画要么露出半截 JSON，要么得写一个流式 JSON 解析器，只为提前几秒看到文字。忙碌行已经实时显示已收字节，回复通常几秒内收完。以后若改为工具调用 API（正文与动作分开下发），再重新评估。
- **第十批已落地（2026-10-29）**：忙碌行。原来执行命令时一直显示「等模型」；现在收到 exec 行（命令执行前发出）就显示 `跑命令 <命令>`，下一次请求在途（`net_in_flight()`）时切回「等模型」。文案从 `请求中 等模型 3秒 7745词元 上传 32839字节 已收 574字节 hit 1152 miss 6561` 改为 `⏳ 等模型 3秒 · 7745词元 · ↑32.1KB 已收574B · 缓存命中15%`，保留「已收」二字（已有断言依赖它）。selftest：`busy line names the running command and reads sizes`；tmux 实测已见切换。原来 exec 同步执行时秒数不走、界面不刷新，第十一批已解决。
- **第四十二批已落地（2026-10-29）**：状态行上色：`✓` 绿、`◐` 黄、`✗` 和 `│ blocked:` 红、`项目>` 青。颜色只在写终端时加（`tui_put_line`），帧本身仍是纯文本，宽度计算、差量比较和所有读取帧的测试都不受影响；差量刷新改用 tui 自己的循环，`render.cx` 的 `frame_diff` 没动。遵守 `NO_COLOR`，也可以用 `CSIH_COLOR=0` 关闭。写屏缓冲按每行 32 字节预留，满屏彩色行也放得下。selftest：`status rows are coloured on output, frames stay plain`（设了 NO_COLOR 时打印 skip）；tmux `capture-pane -e` 实测 `◐` 行带 `ESC[33m`。
  - 观察（不是本批改动引起）：一道不用工具的算术题，这次评审判为「未验证」（理由是没有工具记录可核实）；之前同类问题判过「已验收（answer_only）」。评审对纯问答的判法不稳定，第四十三批处理。
- **第四十九批已落地（2026-10-29）**：把第四十八批那类 bug 做成门禁。新增 `tools/litlen.cx`：扫描 `strncmp`、`memcmp`、`memcpy`、`write`、`tui_write_all` 调用里「字符串字面量 + 写死的长度」，按 C 转义（`\n \xHH`、八进制等）和 UTF-8 字节算出真实长度，和写死的数字比对；`memcpy` 多 1（连结尾 NUL 一起复制）也算对。夹具上能抓出原来的 11 对 9，不会误报正确的「命令退出 」（13 字节）和 `memcpy("...",4)`。全量扫描 csih 又发现一处：selftest 里 `strncmp(行, "csih · agent", 12)`，`·` 占 2 字节，实际 13 字节，断言比本意弱（只比到「csih · agen」），已改为 13。`check.sh` 新增第 5 项「literal lengths match their counts」，全部通过。局限：拆成多段的字面量，或长度是表达式时，不检查。
- **第四十八批已落地（2026-10-29）**：退出时留一行会话小结：`csih 本次会话：请求 2 次 · 输入 4037 词元（缓存命中 60%）· 输出 107 词元 · 文件改动 0 步`；有文件改动时提醒「退出后不能再 /undo」；没发过请求就不打印。实测时发现并修了一个原有 bug：退出时恢复终端写的是 `tui_write_all("\x1b[?25h\x1b[0m\n", 9)`，实际是 11 字节，只写出 `ESC[?25h ESC[0`；下一个输出的字符（以前是 shell 提示符的首字符，这次是小结的 `c`）把它补成了 `ESC[0c`，即「查询设备属性」，结果那个字符丢了，终端的 `^[[?1;2c` 回复还被回显到屏幕上。现在按 `sizeof` 写。顺带把 `term_raw_leave` 改为 `TCSAFLUSH`，丢掉退出时还没读的输入，不让迟到的终端回复被回显。selftest：`exit summary reports requests, tokens and file changes`；tmux 实测退出后小结完整、没有乱码。
- **第四十七批已落地（2026-10-29）**：`/export [路径]` 把当前对话导出为 Markdown（默认 `./csih-<秒>.md`）：用户的话放在 `## 你` 下，答复正文从 act=answer 的 JSON 里取出放在 `## csih` 下，工具输出放进代码块（每条最多 1500 字节，超出标「截断」）；评审 JSON、decision 等内部记录不导出。owned 会话导出自己的 journal。selftest：`/export writes user, answers and tool output as Markdown`（评审理由和原始 JSON 都不出现）；tmux 实测导出一轮 `echo 导出测试`，得到「你 / 工具 exec / csih」三段。
- **第四十六批已落地（2026-10-29）**：首屏引导。日志为空、输入为空且空闲时，提示行显示 `Enter 发送 · /help 看命令 · / 加 Tab 补全 · Alt-Enter 换行`；一有日志就恢复为简短的 `Enter 发送`，不长期占位。窄窗口下按列宽截断。selftest：`empty screen hints /help, Tab and Alt-Enter; then steps aside`；tmux 实测首屏可见。
- **第四十五批已落地（2026-10-29）**：成功行加上本轮耗时：`✓ 已验收 · 本轮结束 1 轮 1 步 · 10秒 · 理由`。耗时取自本轮计时，为 0（不知道）时不显示，所以原来的精确匹配测试只需在前面把计时归零。tmux 实测 `sleep 3; echo ok` 一轮显示 `10秒`（含两次模型请求和评审）。
- **第四十四批已落地（2026-10-29）**：Alt-Enter 在输入框里换行，Enter 才发送，输入行把换行画成 `⏎`。终端把 Alt-Enter 发成 `ESC CR`（或 `ESC LF`），原来的切分会拆成「单独的 Esc（清空输入）+ Enter（发送）」，等于清掉已打内容再发出去；现在 `term_read_keys` 把这两个字节识别为新的 `TERM_KEY_ALT_ENTER`。term selftest 新增：stdin 换成 pipe 后，`ab ESC CR c` 切成 a、b、Alt-Enter、c。tui selftest：`alt-enter adds a newline without sending`。tmux 实测：`M-Enter` 打出三行，发送后模型按多行内容答 7，`✓ 已验收`。
- **第四十三批已落地（2026-10-29）**：评审规则补一句：没有工具记录、答复是直接作答且不需要外部事实（算术、常识、措辞、基于用户所给信息的推理）时，以 answer_only、空 evidence 接受；只有声称了需要工具才能证实的事实（文件、命令、系统或外部世界）时才判 unverified。验收严格程度不变。agent selftest 通过。tmux 实测 4 题：「三加三」「翻译你好世界」「七乘八」都 `✓ 已验收`；对照题「不用工具说出当前目录有哪些文件」没有被误判通过（模型报告做不到，`✗ 失败`）。
- **第四十一批已落地（2026-10-29）**：用户自定义遮蔽清单。内置清单（env.jsonl、.netrc、.git-credentials、.ssh、.aws、.gnupg、.config/gh、.docker）没法穷举，现在可以在 `~/.csih/secrets` 里每行写一个路径（支持 `~/` 开头，`#` 开头为注释），沙箱启动时一并遮蔽：文件读出为空，目录变成空的只读目录。项目里的 `.env`、某个 token 文件都可以这样加。agent_cli 新增：清单里的绝对路径和 `~/` 路径在沙箱里都读出为空；测试会把用户原有的清单原样恢复（本机原本没有这个文件，测试后也不存在）。
- **第四十批已落地（2026-10-29）**：`/net on|off` 控制 work 档命令能否联网。work 档默认保留网络（`git fetch`、装依赖要用）；`/net off` 之后，work 档的沙箱也加 `-n`，只剩 loopback。read 档一直断网；any 档不进沙箱，不受这个开关影响；本机没有沙箱时，`/net` 会说明开关无效。harness 自己调用模型不在沙箱里，不受影响。标题栏显示 `work·断网`。agent_cli 新增：work 档默认有网络、`/net off` 后只有 lo。tmux 实测：`/net off` 后命令里的 `curl https://api.deepseek.com` 报 `Could not resolve host`（rc=6），本轮照常 ✓ 已验收。
- **第三十九批已落地（2026-10-29）**：`/compact` 压缩上下文。先像 `/new` 一样归档完整记录（可以 `/resume` 换回），新上下文里只写一条用户记录：最近 5 条用户请求（旧到新）加上一轮答复的**原文摘录**，并注明「不是模型总结」。不调用模型，不花词元，每个字都能和归档核对，也不会编造。再次 `/compact` 时不会把上一次的摘录嵌套进去。忙时或 owned 会话拒绝。selftest：`/compact keeps the last 5 requests and answer, quoted`（7 条请求只留后 5 条、答复保留、再压一次不嵌套）；tmux 实测：告诉模型代号「银杏」后 `/compact`，`/context` 显示只剩 1 条记录（283B），再问代号，模型答「银杏」。已知：摘录不做提炼，长对话里更早的细节只能 `/resume` 回去找。
- **第三十八批已落地（2026-10-29）**：斜杠命令 Tab 补全。输入以 `/` 开头、还没有空格时按 Tab：唯一匹配就补全并加一个空格；多个匹配就补到公共前缀，并在日志里列出候选；没有匹配时提示。候选包括内置命令和 `./.csih/commands`、`~/.csih/commands` 里的自定义命令。空输入时按 Tab 仍是展开或收起最近的工具行。selftest：`tab completes slash commands`（唯一匹配、多个匹配、公共前缀、没有匹配）；tmux 实测：`/co` 按 Tab 列出 `/cost /copy /context`，`/con` 按 Tab 补成 `/context`。
- **第三十七批已落地（2026-10-29）**：`/context` 显示当前对话记录有多少条、多大、在哪个文件，以及上下文规则：每轮只送最近 50 条（`MAX_CTX_RECS`），文件超过 256KB 时裁到最近 200 条（`AGENT_JOURNAL_MAX/KEEP`，通过 `agent_context_limits` 读出，不在 tui 里另写一份），再附上次请求上传的字节数和本会话的输入词元。owned 会话用它自己的 journal。selftest：`/context reports records, size and the trim rule`；tmux 实测显示「6 条记录 · 536B · ~/.cdsh/cwd/…/tui.jsonl」。
- **第三十六批已落地（2026-10-29）**：`/retry` 把上一条输入（最新的一条历史）原样再发一次，执行中时进入待发送队列；没有历史时提示「还没有发过输入」。`/retry` 本身不进历史，连续重发不会堆出重复条目；因为历史跨重启保留，重启后也能重发上次会话的最后一条。selftest：`/retry sends the last prompt again`；tmux 实测重发「二加二等于几？不要用工具」，模型答「四。」。
- **第三十五批已落地（2026-10-29）**：自定义斜杠命令。`/名字 参数` 不是内置命令时，先找 `./.csih/commands/名字.md`（项目级），再找 `~/.csih/commands/名字.md`（个人级）；文件内容里的 `$ARGS` 换成参数后当作提示词发出，执行中时进入待发送队列。命令名只能由字母、数字、`_`、`-` 组成（沿用斜杠命令的识别规则），不会有路径穿越。展开结果以 `/` 开头时，前面加一个空格，避免被当成命令再解析一次。空文件会报错；都找不到时报 unknown command，并提示自定义命令放在哪里。selftest：`custom /command expands $ARGS from ~/.csih/commands`；tmux 实测：项目级 `/lines four.txt` 展开为「用 sh 运行 wc -l four.txt，只回复行数」，模型答 4。
- **第三十四批已落地（2026-10-29）**：项目说明文件。启动目录里有 `CSIH.md` 就读它，没有就读 `AGENTS.md`，取前 8KB（按码位截断），追加在系统提示词末尾，并注明「是本项目的约定，不是授权；权限档、沙箱和危险命令门照常生效」。文件不变时系统提示词的字节也不变，不影响缓存命中。在 unisacc 仓库根目录启动时会自动带上根目录的 `AGENTS.md`。agent_cli selftest：没有文件时不追加、`AGENTS.md` 兜底、`CSIH.md` 优先、超长内容按码位截到 8KB。tmux 实测：`CSIH.md` 约定「答复末尾加【约定生效】」，模型答「一加一等于二。【约定生效】」。
- **第三十三批已落地（2026-10-29）**：`/copy` 把上一轮答复放进 tmux 粘贴缓冲区（`tmux set-buffer`），在任意窗口用 `prefix ]` 粘贴；不在 tmux 里或还没有答复时如实提示。顺手把 `last_answer` 从 1024 字节放大到 4096 字节：原来长答复只保留前 1024 字节。selftest：`/copy refuses when empty or outside tmux; quoting is exact`（含单双引号、`$`、反引号、反斜杠、中文的 shell 往返；为了不覆盖用户的缓冲区，不真去写 tmux）。tmux 实测：答复 `复制测试 it's "ok"` 原样进了缓冲区，测完已删掉这条。
- **第三十二批已落地（2026-10-29）**：长任务结束提醒。一轮耗时 10 秒以上时，结束后向终端发 BEL，tmux 会把该窗口标为有铃声，切到别的窗口工作时也能知道 agent 跑完了。只在 stdout 是终端时发；`CSIH_BELL=0` 关闭。耗时取自本轮的网络计时，最终答复本身就是一次请求，所以结束时基本等于整轮耗时。selftest：`bell only on a tty, not disabled, after 10s`；tmux 实测：后台窗口跑 `sleep 11`，结束后 `window_bell_flag=1`。
- **第三十一批已落地（2026-10-29）**：在日志区点击文件路径，直接用 `/view` 打开。按点击的列位置取出所在的词（分隔符是空格、引号、括号、逗号和任何非 ASCII 字节，所以 `│`、`→` 不会粘进路径；代价是含中文的路径认不出来），支持 `路径:行号`、`~/` 和相对路径（以工作目录为准）。只有确实存在的普通文件才会打开，否则照旧走工具行的展开/收起。行为变化：点击工具行标题里的文件名会打开该文件，点击其他位置才是展开/收起。忙时不触发。selftest：`clicking a file path opens it in /view at path:LINE`（含点击普通单词不打开）；tmux 实测：向窗口发送 SGR 鼠标序列点击工具行里的 `clickme.txt`，显示出它的 3 行内容。
- **第三十批已落地（2026-10-29）**：输入历史跨重启保留。每次提交的输入行追加写进 `~/.csih/history`（条目内的换行存为 `\x1e`），agent/run 启动时读回最近 16 条，上下键和 Ctrl-R 都能用；文件超过 1000 条时，加载那一刻重写为最近 16 条。只有真正的 agent/run 入口会设置这个文件路径，owned 会话和 selftest 都不读不写它。selftest：`history persists across restarts, newest 16, multi-line kept`。tmux 实测：发一条输入后关窗口重开，Ctrl-R 能找回。
- **第二十九批已落地（2026-10-29）**：Ctrl-R 搜索历史输入。用当前输入的文字作为关键词，找最近一条包含它的历史输入并填进输入框；再按 Ctrl-R 继续往更早找；找不到时提示「历史里没有匹配」或「没有更早的匹配」；按其他任何键就退出搜索。历史仍是内存里的 16 条，不包括斜杠命令。selftest：`ctrl-r searches history newest first`；tmux 实测：输入「不要用」后按 Ctrl-R，找回了「只回复 好，不要用任何工具」。
- **第二十八批已落地（2026-10-29）**：修 `/undo` 备份泄漏。关 tmux 窗口时进程收到 SIGHUP 直接结束，`atexit` 不会执行，备份留在 `~/.csih/undo/` 里（实测留下了一个 `364661-1.bak`）。现在每次做快照时，都会顺手清掉属于已退出进程的备份（`kill(pid,0)` 返回 ESRCH 才算已退出）。agent_cli 新增 `a dead process's leftover backup is swept`。同时补跑了第二十五到二十七批之后的完整门禁：check.sh rc=0，探针 11/11，shell/term/net selftest 都通过。
- **第二十七批已落地（2026-10-29）**：`/view 路径 [起始行]` 在日志区显示文件 20 行，带行号，不经过模型，也不耗词元。相对路径以工作目录为准，支持 `~/`；后面还有内容时提示下一页的命令（`/view 路径 21`）；打不开时报错误原因。点击工具输出里的路径来打开，需要先可靠地识别出路径，暂不做。selftest：`/view shows numbered lines and paging`；tmux 实测 `/view apps/csih/csih.sh 20`。
- **第二十六批已落地（2026-10-29）**：`/diff` 列出 agent 本次运行改过的文件，逐个用 `diff -u` 和改动前的备份比较，每个文件最多 12 行、总共最多 30 行。同一文件改过多次只列一次，用最早的备份比较，所以看到的是整体改动。新建的文件和 /dev/null 比较。数据来自 `/undo` 的快照，所以 exec 的改动不在其中。selftest：`/diff lists each changed file once with its diff`。tmux 实测：edit 改了 v.txt 一行、write 新建 w.txt，`/diff` 显示 `-第二行 +第二行已改` 和 `新建 … +新文件`。
- **第二十五批已落地（2026-10-29）**：`/undo` 撤销 agent 最近一次文件写入/编辑。每次 write/edit 前先把原内容复制到 `~/.csih/undo/<pid>-<n>.bak`（原文件不存在则只记一笔「新建」）；`/undo` 恢复最新一步：有旧内容就写回，是新建的就删掉。最多 20 步，只在本进程内有效，进程退出时清掉自己的备份（`atexit`）。不覆盖 exec 的副作用。agent_cli selftest：一次写入记一步、撤销覆盖写回原内容、撤销新建删除文件、没有可撤销时如实报告。tmux 实测：agent 把 `u.txt` 改写为「被改了」，`/undo` 后恢复为「原始内容」。
- **第二十四批已落地（2026-10-29）**：提示词里加一条完整的答复示例（带 `quote`，`start/end` 写 0,0）。此前模型很少用 quote，claims 第一次常写错（support 填 evidence ID、unknown 写成数组）。实测 write→read→sh 4 次：4/4 第一遍就 `✓ 已验收`、0 次重答，4/4 用了 quote（例如 `"quote":"wrote 7 bytes"`、`"quote":"7 xd.txt"`），由 harness 定位字节。
- **第二十三批已落地（2026-10-29）**：沙箱资源配额，并扩大机密遮蔽范围。每条命令默认 `ulimit -v` 4GB、`ulimit -t` 120 秒 CPU，可用 `CSIH_SANDBOX_MEM_MB` / `CSIH_SANDBOX_CPU_SEC` 调整。遮蔽范围加上 `~/.netrc`、`~/.git-credentials`（文件，用 /dev/null 盖住）和 `~/.aws`、`~/.gnupg`、`~/.config/gh`、`~/.docker`（目录，用空的只读 tmpfs 盖住）。unisacc 编译在配额下实测正常。agent_cli 新增 `sandbox sets default memory and CPU quotas`；check.sh rc=0、探针 11/11。仍然管不到：遮蔽清单之外的机密、work 档的网络。
- **第二十二批已落地（2026-10-29）**：沙箱补上第十九批写明的三个缺口。
  - 机密：沙箱里 `~/env.jsonl` 被 /dev/null 盖住，`~/.ssh` 被空的只读 tmpfs 盖住，模型跑的命令读不到 API key 和 ssh 私钥。harness 进程本身不在沙箱里，照常读 key、调用模型。
  - 进程：`-p --fork --mount-proc` 建私有 PID namespace，命令看不见、也杀不掉沙箱外的进程。
  - 网络：read 档加 `-n`，只剩 loopback；work 档保留网络（`git fetch`、装依赖要用）。
  - 仍然管不到：其他机密文件（只遮了这两处）、work 档的网络、CPU/内存配额。
  - agent_cli selftest 新增：work 档 `~/env.jsonl` 读出为空、沙箱有自己的 PID 空间、read 档 `/proc/net/dev` 只有 lo。tmux 实测：`wc -c ~/env.jsonl` 为 0，`ps -e | wc -l` 为 4，本轮 `✓ 已验收`。
- **第二十一批已落地（2026-10-29）**：状态行的参考生成器 `tools/status.cx`，这样状态行不必非得靠别人写的工具。
  - 用法：`sh unisacc.com apps/csih/tools/status.cx -- [count DIR 完成后缀 待办后缀] [next 清单.md] [line 文本] [输出文件]`，每个选项写一行，默认输出 `./.csih-status`。
  - `count` 递归数文件系统，跳过点目录，输出 `进度 完成/总数 .cx（.py 还剩 N，扫描 DIR）`；`next` 取 Markdown 清单里第一个未勾选的 `- [ ]` 项；`line` 写固定一行（比如最近的拒绝句）。
  - 数字在运行时从磁盘算出，可以用 `find` 复核。目录读不了时退出码 1，用法错误时退出码 2。
  - 实测：夹具上计数与 `find` 一致（2 个 .cx、3 个 .py，`.git` 下的被跳过）；对 `apps/csih` 运行，csih 显示 `进度 108/142 .cx（.py 还剩 34…）`，`find` 复核为 108 个 .cx、34 个 .py。机房可以直接用它，或者换成自己的盘点工具。
- **第二十批已落地（2026-10-29）**：旧对话记录迁移。`/resume shared` 把按目录分开之前的共享文件 `~/.cdsh/tui.jsonl` 复制为当前目录最新的归档（原文件不动），之后 `/resume 1` 换回。顺手修了归档排序：同一秒内连续 `/new`、`/resume` 时，已有归档的秒数会被加一，可能比当前时间还新，导致刚归档的那份排不到第一、提示里的「归档 1」对不上；现在新归档的秒数取「当前时间」和「最新归档加一」中较大的那个。另外 `fopen("wx")` 在 unisacc 的 libc 上不可用，改为先 `access` 再 `"w"`。selftest 扩展了 `/resume lists and swaps archived transcripts`（导入、排序、原文件保留、来源缺失时报错）。tmux 实测：导入你的真实共享记录（77KB），`/resume` 第 1 项就是它。已知：列表里显示的「多久前」是复制的时间，不是原对话的时间。
- **第十九批已落地（2026-10-29）**：exec 文件系统沙箱。第十五批的权限档只是关键字和路径前缀门，现在 read/work 两档下 exec 跑在私有 mount namespace 里（`unshare -Urm`，不需要 root），由内核拒绝写入。
  - 做法：所有挂载点重挂为只读；work 档先 bind /tmp、再 bind cwd，然后把这两处重挂为可写（顺序不能反：cwd 在 /tmp 下时，后 bind 的 /tmp 会盖住它）。超出范围的写入报 `Read-only file system`。
  - read 档有沙箱时不再依赖只读命令白名单，任何命令都能跑，写入由内核拒绝；没有 `unshare` 时退回白名单。`CSIH_SANDBOX=0` 关闭沙箱。any 档不进沙箱。
  - 命令太长、包装不下时直接拒绝执行（`blocked: 命令太长，放不进沙箱`），不会悄悄裸跑。
  - 管不到的：网络、进程、读取机密（如 `~/env.jsonl`）。沙箱里用户映射为 root，`whoami` 显示 root，文件属主不变。
  - 代价：每条命令多约 50ms（5 次 `true`：0.244s 对 0.005s）。
  - agent_cli selftest：read 档 cwd 内写入被内核拒绝；work 档 cwd 可写；work 档家目录只读；超长命令被拒绝。没有沙箱的机器上打印 skip。tmux 实测：`touch ~/x` 报 Read-only，`touch inside.txt` 成功，家目录里确实没有生成文件。
- **第十八批已落地（2026-10-29）**：项目状态行。回应机房的提议（希望一眼看到进度、余刀/下一刀、拒绝句）。
  - 评估：进度和下一刀**不做成 csih 内置**（REJECT）。csih 是通用 harness，不掌握 unisacc 的盘点分母；自己去数等于第二套口径，跟盘点对不上时就是「看起来可核验」的假进度。拒绝句和失败原因 ACCEPT，已有 `✗/◐` 结果行、`blocked:` 原因和错误行。
  - 做法：csih 只负责显示。读 `$CSIH_STATUS_FILE`（默认是当前目录的 `.csih-status`）的前 3 行非空内容，显示在标题下方，形如 `项目> …`；首行附带文件多久前写的，超过 `$CSIH_STATUS_STALE_SEC`（默认 600 秒）标「陈旧」。每帧重读，文件改了画面跟着变；文件不存在就不占行。日志区让出相应行数。
  - 内容由项目自己的工具写，例如机房的 `.cx` 输出 `进度 37/52 .cx（来源 inventory）`、`下一刀 grk: …`、`拒绝 not covered: …`。数字可不可核验，取决于写它的工具。
  - selftest：`project status file shows up to 3 rows with age`。tmux 实测：三行都显示，改写文件后 1.5 秒内画面更新。
- **第十七批已落地（2026-10-29）**：P0 根治（引用原文，由 harness 定位）。
  - observation_refs 增加可选第六键 `quote`（非空，≤200 字节）。正文里能找到它时，用它的字节位置作为引用范围（`at_ref_resolve`），不管模型数的 start/end；找不到再走 `at_ref_span` 收拢。评审拿到的仍是真实原文。
  - 去重把 quote 也算进去（两条 0,0 但 quote 不同的引用是合法的）。顺手修了原有的 `strcmp(NULL,NULL)`：两条引用的 json_pointer 都为 null 时会走到这里（`agent_streq`）。
  - claims schema 不通过时，原来只回一句笼统的「response rejected…」，模型不知道错在哪，连发三次一样的错，最后失败。现在把规则写进拒绝信息（support/counter 是 observation_refs 的整数下标而不是 evidence ID，unknown 是字符串，等等），界面显示「↻ 答复的 claims 格式不对，已把规则告诉模型重发」。提示词里也写明了这两点。
  - agent_cli selftest：`observation_refs accept optional quote; reject empty/unknown keys and duplicates`（8 种形状）。tmux 实测 write→read→sh 共 6 次：改反馈后 3/3 通过，每次重发一回；改提示词后 3/3 通过，其中 2 次第一遍就过。模型目前还没主动用 quote。
- **第十六批已落地（2026-10-29）**：对话记录按工作目录分开存，默认路径为 `~/.cdsh/cwd/<cwd，/ 编码为 %，% 编码为 %%>/tui.jsonl`，两个项目不再共用上下文。`/new` 和 `/resume` 的归档就放在各自目录旁边，所以也自动按目录分开。`CSIH_TRANSCRIPT` 照旧优先；`CSIH_TRANSCRIPT_SHARED=1` 或目录名编码后超过 200 字节时，退回旧的共享文件 `~/.cdsh/tui.jsonl`。旧文件和它的归档原样保留，不迁移；要看旧对话请带 `CSIH_TRANSCRIPT_SHARED=1` 启动后再 `/resume`。agent_cli 中「默认是 home 下的 journal」这条断言随合同改为「默认是本目录的 journal」，并按同样规则算出期望路径。tmux 实测：在 A 目录告诉模型代号，B 目录答「不知道」，回到 A 目录答出代号。已知：路径用的是进程 cwd，设了 `CSIH_CWD` 时两者可能不一致。
- **第十五批已落地（2026-10-29）**：exec 三档权限，用 `/perm read|work|any` 切换，标题行显示当前档。
  - `read`：只读。写文件、编辑都拦；exec 只放行只读命令：首词在白名单（ls/cat/head/tail/wc/grep/rg/find/pwd/echo/stat/file/du/df/tree/which/date/uname/diff/sort/uniq/cut，以及 git status/log/diff/show/blame），管道每一段都要在白名单里，出现 `> ; & \` $(` 或 `find -delete/-exec/-ok/-fprint` 一律不算只读。故意不放 `env`（会打印 API key）、`awk`（`system()`）和 `sed`（`w`/`e`）。
  - `work`（默认）：写文件、编辑只能落在工作目录内（相对路径不能含 `..`、不能以 `~` 开头；绝对路径必须在 cwd 下，`/wx` 不算 `/w` 之内）；危险命令照旧要 `/allow`。注意这是行为变化：之前写工作目录外的文件不拦。
  - `any`：不拦，危险命令也不拦。
  - 被拦时工具结果写明原因和该用哪个 `/perm`。模型报告失败（outcome=failed）的结果行改为 `✗ 失败: 模型报告未能完成`，原来被误归为 `◐`。
  - 这是关键字和路径前缀门，不是沙箱：exec 在 work 档仍可通过 shell 写到任何地方。
  - selftest：`/perm read|work|any gates writes and commands`（16 个分支）、`model-reported failure is ✗ with a Chinese reason`。tmux 实测：read 档拦写入，work 档拦 `/tmp` 写入，文件都没有写出。
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
