# Orca 安装与 tmux `cc` 对接方案（先方案，未安装）

日期：2026-11-02
作者：omp-go2
状态：只读调研；未下载、未安装、未改 tmux/omp/procguard。等 cc/主人审后执行。

## 0. 结论

- 选 **Orca ADE 桌面端 Linux AppImage v1.4.246**（stablyai/orca，MIT）：用户目录安装，不需要 root；官方 release 提供 sha512，但**没有独立签名**，只能按 GitHub release + `latest-linux.yml` 的 sha512 核对完整性。
- iOS 与桌面配对：桌面 account/status 菜单出一次性配对码，手机 Orca → Pair 输入。**优先 LAN/Tailscale 直连**；Relay 需要 Orca 账号登录，且终端 scrollback/输入可能经 Orca 中继，隐私路径未在文档中写清（标未核实），建议先不用 Relay。
- Orca **没有官方 tmux 集成**。不要用 `tmux attach` 直接塞进 Orca（第二客户端可能改 tmux 窗口大小、干扰 cc）。建议用**自有 bridge 脚本**：Orca 终端跑一个 line-oriented bridge，`orca terminal send`/手机输入 → `tmux send-keys -l -t 1:0`；`tmux capture-pane -p` → bridge stdout → Orca 终端/手机。只读/发键，不改 tmux 配置。
- 最小验证路径：bridge 终端出现在 `orca terminal list`；`orca terminal read --screen` 能看到 cc 面板；`orca terminal send --text "..." --enter` 能在 cc 面板看到。手机是否能直接列这个普通终端 tab **未核实**；若不能，先用桌面 CLI，或退到 `tmux attach` 方案（有 resize 风险）。
- 不需要 sudo。只有 AppImage 因缺 libfuse2 不能直接运行时，才生成 `out/orca-install/install-libfuse2.sh`（幂等、dry-run、可回滚），由主人 sudo 执行；优先 `--appimage-extract-and-run` 完全避开 sudo。

## 0.1 四关核实（2026-11-02，下载前）

| 关 | 核实结果 | 结论 |
|---|---|---|
| 1 无头 | `DISPLAY`/`WAYLAND_DISPLAY` 为空，`XDG_SESSION_TYPE=tty`；`Xvfb`/`xvfb-run` 已装（`/usr/bin/Xvfb`、`/usr/bin/xvfb-run`），存在 X98/X99/X100 socket 但无活动图形会话。Orca 官方 `orca serve` 是“不开桌面窗口”的 headless runtime，`--mobile-pairing` 可直接给 iOS 配对码（`docs/remote-servers`）。 | **走 `orca serve` headless**，不启动 GUI；若 Electron runtime 仍要求显示，用 `xvfb-run -a` 作后备（可行，但非首选）。 |
| 2 网络 | `tailscale` 已装并在线：本机 `100.112.50.22`；LAN `eth0=10.49.27.71/24`，另有 `eth1=38.54.4.59/25`。手机是否已入同一 tailnet/局域网需配对时实测；不在本机可验证范围。 | **可走 Tailscale/LAN 直连**；**不启用 Relay、不登录 Orca 账号**。若配对时手机不可达，停下报 cc。 |
| 3 cc 写入面 | bridge 输入最终会进 `cc` 的 Claude Code；配对码等同写权限。 | 见 §4.2/§5：目标固定 `1:cc`；`[orca] ` 前缀；单行；过滤控制字符；拒绝多行/突发；配对码当密码、只 LAN/Tailscale、可撤销。 |
| 4 下载固定 | AppImage v1.4.246；sha512 来自 `latest-linux.yml`；无独立签名。 | sha512+size 核对通过才用；`--appimage-extract-and-run`；不装 libfuse2；`ORCA_TELEMETRY_DISABLED=1`；固定 1.4.246。 |

## 1. 版本与下载核对

| 项 | 值 |
|---|---|
| 版本 | `v1.4.246`（2026-11-02T03:19:48Z） |
| Release | https://github.com/stablyai/orca/releases/tag/v1.4.246 |
| Linux x86_64 AppImage | `orca-linux.AppImage`，251,057,732 B |
| 下载 URL | https://github.com/stablyai/orca/releases/download/v1.4.246/orca-linux.AppImage |
| sha512（base64，来自 `latest-linux.yml`） | `qM7YVeqPPgr038b1H1O0/H5a/QEJlk4Q54nxvmUCNq2riIRqrs46kJ1KY1Ujgf7SgP/Kd/5hDtj2yKN4N7NiNQ==` |
| 校验文件 | https://github.com/stablyai/orca/releases/download/v1.4.246/latest-linux.yml |
| 签名 | **无独立 .sig/.asc**；只有 sha512。完整性可核，发布者真实性依赖 GitHub release/TLS。 |

下载流程（审后执行）：
1. 新建 `out/orca-dl/1.4.246/`（空目录）。
2. 下载 `orca-linux.AppImage` 与 `latest-linux.yml` 到该目录。
3. 用 `sha512sum` 出 hex，与 yml 的 base64 解码后逐字节比较；同时核对 size=251057732。
4. 不通过就删除重下；不 `curl | sh`。

## 2. Linux 安装（用户目录，无 root）

推荐路径：

```
~/.local/opt/orca/1.4.246/orca-linux.AppImage
~/.local/bin/orca-appimage -> ~/.local/opt/orca/1.4.246/orca-linux.AppImage
```

步骤（审后）：
1. `chmod +x` AppImage；先 `--appimage-version`/`--appimage-help` 确认可执行。
2. 若 FUSE 可用：直接运行；若报 `libfuse.so.2` 缺失：优先 `--appimage-extract-and-run`（不装系统包），或 `--appimage-extract` 到 `~/.local/opt/orca/1.4.246/squashfs-root` 后跑 `AppRun`。
3. 首次启动按需 `--no-sandbox` **仅用于排障**；默认不加。AppImage/Electron 在无用户命名空间时可能要求它，需记录实际错误再决定。
4. CLI：在 Orca Settings 里注册 `orca`（文档里 “General → Orca CLI” / “Experimental → CLI” 两处措辞；以实际 UI 为准）。验证 `command -v orca`、`orca status --json`。若 UI 不注册，解包 AppImage 找 `orca` 二进制并软链到 `~/.local/bin/orca`。
5. 固定版本；未见官方“关闭自动更新”文档，若默认自动更新，记录并在验证期避免升级（未核实）。

**sudo 脚本**：只有 libfuse2 缺失且主人不愿用 extract 时才需要。路径 `out/orca-install/install-libfuse2.sh`，`--dry-run` 打印 `apt-get install -y libfuse2`；幂等（`dpkg -s libfuse2` 已装则退出）；回滚不自动卸载（可能影响其他 AppImage），只记录。

## 3. iOS 配对

文档：https://www.onorca.dev/docs/mobile

两种路径：

| 路径 | 做法 | 优点 | 风险 |
|---|---|---|---|
| **LAN/Tailscale 直连（推荐）** | 桌面 Orca account/status → pairing flow → 一次性 code；手机 Orca → Pair；展开 local network address，选本机 LAN/Tailscale 地址 | 不经过 Orca 云；终端内容只在本机/局域网 | 手机与桌面需同一网络；桌面关闭则断 |
| Relay | 手机 Pair 时选 Orca Relay；需登录 Orca 账号 | 跨网方便 | 终端 scrollback/输入可能经 Orca 中继；加密/保留策略未核实；需账号 |

- 账号登录/授权由主人本人完成；我不代填。
- 配对成功后手机可看 worktree/agent、最近终端 scrollback、发短回复/ Live 输入；桌面仍是 source of truth。
- 关闭桌面 App 会断 direct/LAN 会话；Relay 跟随 Relay 连接。
- 如果手机必须走 Relay，先按“终端内容可能离开本机”处理，不在 cc 里处理敏感内容，直到确认中继加密/保留策略。

## 4. 与 tmux `cc` 的对接

### 4.1 官方能力边界

- Orca 终端是 xterm.js；可以开 shell、跑 `tmux attach`，但文档没有“接入已存在 tmux 窗口”的原生功能。
- CLI 有 `orca terminal list/read/send/create/split/wait`，但只针对 Orca 自己创建的终端（handle）。
- 因此最小可行路径是：**Orca 终端里跑一个 bridge 脚本**，把 Orca 的 stdin 转成 `tmux send-keys`，把 `tmux capture-pane` 打到 stdout。

### 4.2 推荐：line-oriented bridge（不改 tmux 配置）

目录：`out/orca-bridge/`（不碰 unisacc 主工作树）
- `repo/`：一个只含一个 commit 的 dummy git repo，供 Orca 添加项目/建 worktree；`tmux attach`/bridge 的 cwd 不重要，避免动 unisacc 的 git worktree。
- `tmux-bridge.sh`：

```sh
#!/bin/sh
# Orca terminal <-> tmux pane bridge. Target default 1:0 (cc).
set -eu
target=${1:-1:0}
printf 'BRIDGE ready target=%s\n' "$target"
while IFS= read -r line; do
  case "$line" in
    '!snap') ;;
    '') tmux send-keys -t "$target" Enter ;;
    *) tmux send-keys -l -t "$target" -- "$line"; tmux send-keys -t "$target" Enter ;;
  esac
  printf '\n--- pane %s ---\n' "$target"
  tmux capture-pane -p -t "$target" -S -40
done
```

- `!snap` 只抓屏不发；空行只发 Enter；其他行按字面发到 cc。
- 不 `tmux attach`，因此不会新增 tmux client、不会改窗口大小、不改 tmux 配置。
- 只用 `tmux send-keys` / `capture-pane`，都是现有 tmux 命令；cc 窗口仍由原 client 控制。

创建：
```sh
orca terminal create --worktree path:<bridge-worktree> \
  --title "cc-bridge" \
  --command "sh /home/devali/repos/unisacc/out/orca-bridge/tmux-bridge.sh 1:0" --json
```
然后：
```sh
orca terminal list --json          # 取 handle
orca terminal read --terminal <handle> --screen --json
orca terminal send --terminal <handle> --text "orca bridge ping" --enter --json
```
`read --screen` 应看到 cc 面板；`send` 后 bridge 把字面文本发给 cc 并按 Enter，再打印新的 pane 快照。

### 4.3 备选：`tmux attach`（不推荐）

```
orca terminal create --command "tmux attach -t 1:0"
```
- 优点：Orca 直接显示 cc 的完整 TUI；如果 Orca 能通过 OSC 识别 Claude Code，手机可能把它当 agent 会话。
- 风险：tmux 会把窗口调整到最小 client 的尺寸，可能影响 cc 当前窗口；关闭 Orca 终端也会 detach，需确认不会 kill 原 session。只作为 bridge 在手机上不可见时的备选。

### 4.4 手机端

- 手机 Orca 会列 worktree/agent；**普通 bridge 终端是否出现在手机 session tab 未核实**。
- 若出现：打开 raw terminal，直接输入；bridge 按行转发。
- 若不出现：先用桌面 Orca/`orca` CLI；或退到 4.3 的 attach 方案（让 Orca 看到 Claude Code 的 OSC 状态）。这一点在安装后第一轮验证里确认，不先假设。

## 5. 隐私与网络

| 项 | 事实/判断 | 处置 |
|---|---|---|
| 遥测 | 匿名 ID，不传文件内容/prompt/终端输出/路径/分支；PostHog Cloud US；`DO_NOT_TRACK=1` 或 `ORCA_TELEMETRY_DISABLED=1` 关闭 | 安装后设置 `ORCA_TELEMETRY_DISABLED=1`（或 Settings→Privacy 关） |
| 手机 Relay | 手机能看“最近终端 scrollback”并发输入；Relay 是否端到端加密、是否存储终端内容，官方文档未写清（未核实） | 优先 LAN/Tailscale 直连；若用 Relay，按“终端内容可能离开本机”处理 |
| 账号 | Relay 需要 Orca 账号；LAN 配对是否必须同账号文档有冲突措辞（未核实） | 主人本人登录；我不代填 |
| 插件/Artifacts | 插件可执行代码；Artifacts 公开分享是 opt-in | 不装插件、不分享 artifact |
| 会话搜索 | 本地 transcript 索引；跨机搜索需授权 | 不使用跨机搜索 |
| 终端内容 | 手机/Relay 会传 scrollback；cc 里可能有工作内容 | 先只发测试消息；敏感内容等隐私路径确认后再用 |

## 6. 与现有体系不冲突

- 不碰 omp-* 窗口、不动 procguard、不改 tmux 配置。
- bridge 只对 `1:0` 做 `send-keys`/`capture-pane`；不 `kill-server`、不 `attach`、不改 option。
- 不用 sudo 安装；AppImage/桥目录都在用户目录或 `out/`。
- 需要清理时用精确 handle/PID：`orca terminal close --terminal <handle>`；不 `pkill -f orca`。
- 专用 dummy repo，避免在 unisacc 主 checkout 上新增 Orca worktree。

## 7. 审后执行顺序（最小验证）

1. 下载 + sha512 核对（§1）。
2. AppImage 用户目录安装；`--appimage-extract-and-run` 优先；`orca status --json`。
3. 建 `out/orca-bridge/repo` + `tmux-bridge.sh`；Orca 添加 repo、建 worktree、创建 `cc-bridge` 终端。
4. `orca terminal list/read/send` 验证 bridge 能看到/发到 `1:0`。
5. 先用 throwaway tmux session（如 `tmux new -d -s orca-test 'cat'`）验证 bridge 路径，再切 `1:0` 发一条明确测试消息。
6. 手机配对：优先 LAN/Tailscale；看是否列出 `cc-bridge`；能则发一条测试，不能则记录并选 attach 备选。
7. 关闭/回滚：关 bridge 终端；删 AppImage/桥目录/Orca 数据（可选）；tmux 原 session 不动。

## 8. 未核实

1. 手机是否列出普通（非 agent）Orca 终端 tab。
2. Relay 是否端到端加密、是否持久化终端内容。
3. AppImage 在本机是否需要 `--no-sandbox`/libfuse2；先按 extract-and-run 规避。
4. Orca CLI 注册在 Settings 的哪个页面（General vs Experimental）。
5. 自动更新是否可关；验证期固定 1.4.246。
6. `orca terminal send` 对运行 bridge 的终端是否严格按行（应如此，但未实跑）。

> 主人决定不采用，原因=复杂度，2026-11-02。未下载、未安装；`out/orca-dl/` 与 `out/orca-bridge/` 未创建。`plans/orca-reference-points-2026-11-02.md` 保留。
