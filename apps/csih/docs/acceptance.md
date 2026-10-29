# csih 产品验收

本文把 csih 的每项能力对应到能复跑的证据：自动测试名，或者 tmux 实窗场景。改动后按「复跑」一节重跑；没跑过的不算通过。

## 复跑

```sh
cd apps/csih
sh csih.sh prepare                                   # 源摘要变了才重建 out/csih-e2e-bin/csih
sh csih.sh selftest                                  # tui，末行 selftest ok
for m in agent_cli shell_cli term_cli net_cli; do sh ../../unisacc.com $m.cx -- selftest; done
CSIH_BIN=../../out/csih-e2e-bin/csih sh ./check.sh   # 末行 check ok
CSIH_BIN=../../out/csih-e2e-bin/csih UNISACC=../../unisacc.com \
  sh ../../unisacc.com check.cx -- probes            # 11 ok, 0 fail
```

## 最近一次结果（2026-10-29，HEAD 830fb4a6d8，本机 Linux；第二十二批沙箱加固后重跑）

| 门禁 | 结果 |
| --- | --- |
| prepare | up to date |
| tui selftest | 54 项 ok，selftest ok |
| agent_cli / shell_cli / term_cli / net_cli selftest | 全部通过 |
| check.sh | rc=0，check ok |
| check.cx probes | rc=0，11 ok，0 fail（含真实 HTTP 与真实 LLM 往返） |

只有本机证据：没有跑 macOS/Windows，也没有 CI。

## 能力 → 证据

| 能力 | 自动测试 | tmux 实窗 |
| --- | --- | --- |
| 秒开：prepare 按源摘要重建，产物陈旧时具名报错 | 验收命令中的 `csih.sh prepare` | `csih.sh` 0.1 秒内进入界面 |
| 密钥：`api_key` 或 `key`，keyprobe 不泄露 | keyprobe 假 HOME 探针 | 默认 agent 真实调用 DeepSeek |
| 结果三态 ✓/◐/✗，中文理由 | `unverified row uses ◐`、`failed row uses ✗ 失败`、`model-reported failure is ✗…`、`accepted row reads ✓ 已验收…` | write→read→sh 一次 ✓ 已验收 |
| 标题：模型 · key · 权限档 · 目录 | `title shows model, cwd and key source` | 宽 60 时目录被截、key 保留 |
| 忙碌行：跑命令/等模型、秒数、KB、缓存命中 | `busy line names the running command and reads sizes` | `sleep 20` 期间秒数从 1 走到 5 |
| 长命令可取消，整个进程组被杀 | shell：tick 停止、后台子进程随组退出 | Esc 后 2 秒内显示已取消，无残留进程 |
| 工具行：一行摘要、失败自动展开、逐行正文、Tab 切换 | `failed tool row opens by itself`、`closed tool row…`、`open tool body…`、`tab toggles…` | `ls /nonexistent` 自动展开 |
| 输入：光标、Ctrl-A/E/U/W、长行跟随光标 | `cursor moves and edits mid-line`、`long input scrolls…`、`ctrl-u…`、`ctrl-w…` | 中文长句插入、行首行尾 |
| Esc / Ctrl-C | `esc cancels a busy turn`、`esc clears idle input` | 草稿被清空 |
| /model /cwd /cost /help | `/model sets…`、`/cwd shows…`、`/cost shows…`、`usage with nested details…`、`help lists…` | `/cost` 显示真实命中率 |
| 会话：按目录存放、/new、/resume、/resume shared | `/new archives…`、`/resume lists and swaps…`；agent_cli 中按目录的路径 | 两个目录上下文互不相通；导入 77KB 旧记录 |
| 权限：/perm read/work/any | `/perm read|work|any gates writes and commands` | read 档拦写入，work 档拦 /tmp 写入 |
| 沙箱：read/work 档 exec 跑在只读 mount namespace | agent_cli：read 档 cwd 写入被内核拒绝；work 档 cwd 可写、家目录只读；超长命令被拒绝 | `touch ~/x` 报 Read-only |
| 沙箱：机密遮蔽、私有 PID、read 档断网 | agent_cli：`~/env.jsonl` 读出为空、PID 空间独立、read 档只有 lo | `wc -c ~/env.jsonl` 为 0，`ps -e` 只有 4 个进程 |
| 危险命令门 + /allow 一次 | `dangerous exec is blocked until /allow`、`/allow lets exactly one through` | 拦截 → 说明 → /allow → 执行 |
| 证据引用：quote 定位、0,0 整段、格式错误给出规则 | agent_cli：`observation_refs accept optional quote…` | write→read→sh 6/6 通过 |
| 项目状态行 + 生成器 | `project status file shows up to 3 rows with age` | `tools/status.cx` 显示 108/142，`find` 复核一致 |

## 已知边界（不算缺陷，但要知道）

- 沙箱管文件系统写入、常见机密（env.jsonl、.netrc、.git-credentials、.ssh、.aws、.gnupg、.config/gh、.docker）、进程可见性、每条命令 4GB 内存和 120 秒 CPU 配额，read 档还断网；不管清单之外的机密和 work 档网络。没有 `unshare` 的机器上退回关键字门。
- 危险命令门是关键字匹配。
- exec 同步执行，每 200ms 刷新一次界面；单条命令最长 60 秒（`CSIH_EXEC_TIMEOUT_SEC`）。
- 回答正文不流式显示（REJECT，理由见 PRD）。
- 第二十四批加了答复示例后，实测 4/4 用了 quote，且第一遍就通过；不用时 harness 收拢偏移或取整段正文。
