# skills 目录

仓内可复用技能与代理定义。每个技能一个目录,内含 `SKILL.md`(frontmatter 的 name 与 description 写明何时调用)。

## 结构

| 路径 | 何时用 |
|---|---|
| `unisacc-cx/` | 写脚本、自动化、TSV 与文本处理时,用 .cx 代替 python/node/bun |
| `unisacc-orch-dispatch/` | 派工写简报、提交推送、处理非快进与常见拦截 |
| `unisacc-orch-review/` | 对抗复核交付、裁决能否落库 |
| `unisacc-orch-release-train/` | 推进发布列车:起链前置、整链、预跑、冻结、公开、解冻 |
| `unisacc-orch-supervise/` | 督导多窗口与后台作业、定期巡检 |
| `unisacc-pipeline-lessons/` | 事故转产线改进、评估复用提案、拆解版本墙钟 |
| `agents/unisacc-gatekeeper.md` | 代理定义:关键路径补丁的独立守门复核 |

## 安装

技能:在 `~/.claude/skills/` 下为所需技能建指向仓内目录的符号链接,例如

```
ln -s ~/repos/unisacc/skills/unisacc-orch-dispatch ~/.claude/skills/unisacc-orch-dispatch
```

代理定义:复制或链接到 `~/.claude/agents/`:

```
ln -s ~/repos/unisacc/skills/agents/unisacc-gatekeeper.md ~/.claude/agents/unisacc-gatekeeper.md
```

之后可用 `Agent(subagent_type=unisacc-gatekeeper)` 调用。

## 约定

正文不超过 120 行;规则附原因;命令用仓内真实工具名;不写个人信息与家目录绝对路径。
