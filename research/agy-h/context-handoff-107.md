# 0.0.107 起链前各窗口上下文余量体检与接力点建议

编写窗口: `agy-h` (严格只读 `tmux capture-pane` 与文件系统巡检, 未向任何窗口发送按键或改写任何代码)
采样时点: `2026-11-25 02:29 +0800`

---

## 1. 五窗口只读体检总表

| 窗口 (`tmux` 位置 / 运行器) | 当前所见上下文余量 / 状态栏原样 | 02:29 在途动作 | 到公开前 (`05:00..06:30`) 耗尽风险 | 建议接力时机 |
|---|---|---|---|---|
| `cdx` (`1:5`, `codex 0.172.0` / `GPT-6.2-Sol low fast`) | 明确显示 `Context 72% left · Context 28% used` (`475K window`) | 正跑全量 `chain-ready` 端到端演练 (`/tmp/cdx-car107-full.pOucnD`, 含 1 后台终端) | 低 (尚余 `72%` 约 `342K`; 仅若连续读大量长日志才逼近上限) | 起链后保持即可; 若 `left` 跌破 `30%` (如 CI 多轮分诊后) 再开新会话 |
| `ompgpt` (`1:17`, `omp` / `GPT-6.2 Sol`) | 状态条显示已用 `41%` (`500K` 窗口, `S46.61 (+6.07)`, 剩约 `59%`) | 正跑首窗/第二窗冷缓存候选对照 (`bg_232 exit 124 · 54.2s`); 刚落盘 `prerun-prewarm-plan.md` | 中高 (当前高密跑冷热对照与日志读回, 预计 `04:00..05:30` 可能触近 `75%-80%` 压缩区) | `107` S2 预热方案收尾、预跑稳定起跑后 (或进度条过 `70%`) 换新会话接续读回核验 |
| `agy-flow` (`1:18`, `agy` / `Gemini 4 Argon · high` + `Swarm neon`) | `agy` UI 不显示数字百分比; 底栏 Swarm `290 posts`, 收件箱另待 3 条未读通知 | 刚收 `cc` `P1..P6` 脚本合同任务 (`Loading...`), 另堆积 `门19` 等 3 条消息; 后续将全程担负起链/预跑/打包/`Step 1-7` | 极高 (当前多任务并队列叠加将承接全链与 `2-lane` 预跑长输出, 极易在预跑中段触发自动压缩丢失环境变量/句柄记忆) | 写完 `car2-parallel-prep.md` 与门 19 更新后、正式 `bg-candidate start` 前主动切净新会话再起链 |
| `agyneon` (`1:14`, `agy` / `Gemini 4 Neon · low`) | `agy` UI 不显示数字百分比; 底栏轻量模型值守待命 | 已完成 `f1-watch.md` 与 `pre-chain-gaps.md`, 正于 `48f9e054ad` 干净工作区只读待命 | 极低 (`107` 期间几乎不动车树、无高频读写) | 无需提前换会; 等 `release-post` 解冻转入 `0.1.x`/工具线追加集时再视余量决定 |
| `ccogweco` (`1:9`, `Claude Code` / `deepseek-v4.1-flash-eco`) | 状态栏明确显示 `ctx 64%` (`⎇ v0.0.107`) | 正写 `car2-final-patchlist.md` (核 `notes-107-patchlist.md` 对 `git log` `13` 份入车提交) | 高 (已达 `64%`, 当晚已高产多份长文, 预计 `03:00..04:00` 进入证据/发布说明核对时最早撞压缩区) | 交完 `car2-final-patchlist.md` 后立即运行 `/compact` 或开新会话接手公开核对与 `0.1.0` 文档 |

注:
- 状态栏截屏依据(均取只读 `tmux capture-pane`):
  - `1:5` `cdx`: `Context 72% left · Context 28% used · weekly 25% left · 0.172.0 · 475K window · Fast`
  - `1:17` `ompgpt`: `▶─────41%─────╎───┃───500K─`
  - `1:18` `agy-flow`: `● Swarm neon: ... · 290 posts ... auto · Gemini 4 Argon · high`
  - `1:14` `agyneon`: `auto · Orchestrator · Gemini 4 Neon · low`
  - `1:9` `ccogweco`: `deepseek/deepseek-v4.1-flash-eco | .../.claude/worktrees/v0.0.107 | ⎇ v0.0.107 | ctx 64%`
- 只读发现附记: `ccogweco` 当前 `cwd` 在 `.claude/worktrees/v0.0.107`, 写文档时虽靠绝对路径经 `devsvc` 写回主仓 `research/`, 开新接力会话时建议直接锚在主仓根以防误落车树分支。

---

## 2. 各窗口已落盘可续页与关键状态文件 (新会话接力清单)

### (1) `cdx` 接力入口
- 关键状态/门槛页 (均在 `~/.local/share/unisacc/research/cdx/`):
  - `car2-chain-ready-gates.md` (门 5 `--prepare` 双对象与门 22 正解)
  - `pre-chain-timings.md` (刚完成的全量 `chain-ready` 时限基线)
  - `car2-b-audit.md`、`car2-add-acceptance.md` (107 阶段 B 验收底账)
  - `car2-16-ci-run-attempt.md`、`ci-triage-106.md`、`car2-runbook.md` (CI 格分诊、G4 `--redispatch` 规则)
  - `car2-17-tlib-bump-gate.md` (顺延 108)
- 接力做法: 新会话只读以上 4 页即可直接承接预跑红停 `strict-Wxxxx.tsv` 定因与 CI 6 格异常分诊。

### (2) `ompgpt` 接力入口
- 关键状态/补丁页 (均在 `~/.local/share/unisacc/research/ompgpt/`):
  - `prerun-prewarm-plan.md` (最新 `S2` 预热方案, 已录入 `fwd-cap` 冷跑 `55.002s rc124` / 热 `40.716s`)
  - `readback-prewarm-gate-v2-on-0107.patch` 与 `readback-prewarm-gate-v2-on-0107-evidence.md` (已入 107 的 `#14 v2` 读回预热门)
  - `readback-smoke-timing.md`、`release-exec-prewarm-design.md` (预热与 `c47-1-readback` 耗时口径)
  - `ci-download-hardening.patch`、`check-tool-material-macro-v4.patch` (工具线顺延材料)
- 接力做法: 新会话加载 `prerun-prewarm-plan.md` 与 `release-exec-prewarm-design.md`, 即可在 `Step 5-6` 协助复核 `--warm` 与回读。

### (3) `agy-flow` 接力入口
- 已落盘流程页 (`research/agy-flow/`):
  - `car2-stage-c-run.md` (阶段 C 到全链/预跑命令、门 19 预备)
  - `car2-ci-dryrun.md` (`dry`/打包与 CI 核对三)
  - `car2-live-clock.md` (时钟重估与阶段时限)
  - `chain-to-x-ledger.md` (`P1..P6` 并行动作设计) 及正在输出的 `car2-parallel-prep.md`
- 活状态文件 (一切进程/身份真源在盘不在对话历史):
  - `build/freeze.tsv`、`.claude/worktrees/v0.0.107` (`48f9e054ad`)
  - 链目录 `$OUT/owner.tsv`、`$OUT/exit.tsv`、`$OUT/chain-run.tsv`、`$OUT/ctx/context.tsv`、`$OUT/ape-final/unisacc-next.com`
  - 预跑目录 `$PR/windows.tsv`、`$PR/supervise.log`、`$PR/run.done`、`$PR/prerun-rate.tsv`
  - 发布证据目录 `release/evidence/0.0.107/INDEX.tsv`、`release/candidate.tsv`
- 接力做法: 主动在起链前切新会话, 先读 `car2-stage-c-run.md`、`car2-parallel-prep.md` 与 [car2-chain-launch.md](car2-chain-launch.md), 并用 `owner.tsv`/`exit.tsv`/`supervise.log` 核真实在途进程。

### (4) `agyneon` 接力入口
- 已落盘与共享页:
  - `~/.local/share/unisacc/research/agyneon/notes-107-patchlist.md`、`pre-chain-gaps.md`、`f1-watch.md`、`car2-chain-standby.md`
  - 车树状态: `.claude/worktrees/v0.0.107` (`48f9e054ad`, `git status` 干净), 顺延总表 [car2-108-candidates.md](car2-108-candidates.md)
- 接力做法: 若需新会话接手解冻后工单, 读 `pre-chain-gaps.md`、`f1-watch.md` 与 `car2-108-candidates.md` 即明现状。

### (5) `ccogweco` 接力入口
- 已落盘总集 (`research/ccogweco/`):
  - 当日运行总册: `chain-day-roster.md`、`incident-playbook-107.md`、`pre-chain-crosscheck.md`、`cc-chain-day-script.md`
  - 裁定与证伪锚点: `decision-24h-fable.md`、`prerun-speedup-0.1.x.md`、`lessons-106-night.md`、`loss-guards-107.md`
  - 发布与证据占位: `evidence-107-template.md`、`notes-0.0.107-prefill.md`、`freeze-107-row.md`、`car2-final-patchlist.md` (刚完成中)
  - `0.1.0` 封板底账: `seal-parallel-prep.md`、`seal-clock-margin.md`、`seal-pre-run.md`、`notes-0.1.0-draft.md`
- 接力做法: `car2-final-patchlist.md` 落盘后马上 `/compact` 或开新会话 (主仓根启动), 读 `chain-day-roster.md`、`incident-playbook-107.md` 与 `evidence-107-template.md` 继续盯 107 公开归档。
