# 裁定：0.0.40 冻前未完门闩具名处置（机房主任 1011）

- **时间**：2026-10-11 ~10:11 SGT
- **权限**：宿主/卫生/顺延类代裁；**不**代裁冻结批准、**不** bump、**不** Draft、**不**改验收/删测/归因。
- **目的**：把「授权冻」前置里仍须具名的未完切口一次列清，供政委一裁冻结。

## 具名处置（本冻）

| 切口 | 处置 | 归属 / 说明 |
|---|---|---|
| run/.cx 入口缺陷（0937 intake） | **顺延（不依赖本冻）** | 公开 v0.0.39 已用 `-o` 绿基线顶住；根因在 model driver 续源不认 `.cx`。优先热修进 **0.0.41 早段**（或 0.0.40.x 若政委另裁）。不挡 0.0.40 冻结/Draft。 |
| 分层并发（共享冲突实测） | **顺延（不依赖本冻）** | 未测共享冲突读数；机制在仓；不挡本冻。 |
| WF2 填尾（tail/DEFER 链） | **顺延**（既有） | 已记；非产品验收放行。 |
| 等裁空转裁剪真实 Stop-hook | **顺延**（既有） | 夹具已证；非真实首观察。 |
| 改错声明 sha 负例 | **顺延**（既有） | 不挡门闩。 |
| PGID/TERM/UA 本体 / 门预算限定 | **顺延**（首红限定保留） | 0942 首红命令层已闭合；本体/预算不升格本冻阻断。 |
| 首观察实跑（preflight→首 make） | **另授** | 冻结获批 + version bump 之后另开；本裁定不授实跑。 |

## 仍须政委/授权人一裁

1. **批准 0.0.40 冻结**（承认上表顺延项不挡本冻）。
2. 授权后由机房主任开窗：**单独** `src/version.h`→`0.0.40` → `freezecheck` → `refresh_gatedeps` → 三者一致重核 → 再议 Draft。

## 不授

- 不授 bump / Draft / freeze 实跑 / 首观察实跑。
- 不授把 run/.cx 产品修复塞进本冻闭包。
- 不改 prd / gatedeps / 验收措辞。

## 依据

- `research/c40-stamp-B-guard-k5-1i/c40-freeze-gate-checklist-0955.md` + `…-1000-addendum.md`
- `/tmp/unisacc-cdx2/observe-10m/ruling-run-mode-defect-intake-0937.md`
- `/tmp/unisacc-cdx2/observe-10m/cdx2-postclose-0942.md`
- tip 采样本轮：`fb2227cf`；version.h 仍 `0.0.39`；Latest v0.0.39；无 Draft v0.0.40。
