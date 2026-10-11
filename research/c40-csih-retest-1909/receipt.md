# csih retest under Latest v0.0.40 (m4pro)

墙钟: 2026-10-11 19:09–19:15 +0800

## 窗
- tmux `1:7` **cc-csih**（cwd `/Users/wjc/repos/moltbaby`；Claude Code）
- 产品主窗 `1:4` **cc-unisacc** 保持并行（K6）；本刀旁观/推动未抢其任务；曾误切后已 `select-window` 回 1:4

## 二进制（以新二进制为准；build.json 旧记录不采）
- path: `/Users/wjc/repos/unisacc/unisacc.com`
- `--version`: unisacc 0.0.40
- sha256: `50ab4c213feedb2f1a4f1f39c8bfca7d8d63c74a1d6b50915db8ceb233f5cf58`
- bytes: 2243662
- release target / seal: `1db30835`（rc/v0.0.40）；cand `50ab4c21`
- 0.0.39 bak: `…/scratchpad/unisacc.com.0.0.39.bak`（claude-501 unisacc session）
- 禁扩大发布/签名：本刀未触

## 推动
- 入口消息已由 envelope 送达 cc-csih（来自 cc-unisacc：0.0.40 就绪）；本刀确认其已开跑并旁观至首轮+ harness 修补重跑
- cc-csih 顺序：version → `csih.sh selftest` → `unisacc.com check.c suite.c probes/u_run.c` → 修 `check.c` loop-run `-include` → 重跑 → 继续挖 agent 行

## 绿/红
| 项 | 结果 | 对照 |
|---|---|---|
| `csih.sh selftest` | **绿** `selftest ok` | 0.0.39：run 模式拒 TUI 表 → rc=1 |
| check 套件首轮 | **红** 26 ok / 9 fail | 0.0.39 基线曾记 23 ok / 12 fail |
| check 套件（check.c +include 修补后） | **红** **28 ok / 7 fail** | loop-run + gate verdict 两红转绿 |

### 仍红（7）
- render selftest — `cols.cx:32:1: error: not covered: no main`
- tui selftest — rc=127
- shell selftest — budget timeout 标记
- agent selftest — rc=128
- TUI probes — ok=5 expected-red=1
- llm round trip failed
- agent roundtrip failed

## 产物
- `research/c40-csih-retest-1909/`（及 `/tmp/csih-retest-40-1909/` 镜像）
  - csih-sh-selftest.out
  - check-c-suite.out / check-c-suite-rerun.out / check-c-suite-after-include-fix.out
  - receipt.md
- 工作树：`apps/csih/check.c` 未提交修补（+include）；`csih.sh` 仍有既有未提交改动

## 卡点（续）
- `.cx` 单独 selftest 行缺 main / top-level（render；loop 已靠 -include 缓解）
- agent/tui/llm 链路仍红；cc-csih 仍在挖（绝对 -include 试 agent 行）
- 右侧仍挂 mycc/grok_config `[ask]` 面板（标题残 mycc）；未清、不挡套件
