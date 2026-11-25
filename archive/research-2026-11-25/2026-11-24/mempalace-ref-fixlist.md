# prd_mempalace.md 引用改指向清单(2026-11-24)

检索命令: `grep -n '会话临时目录\|jobs/7f46e032\|待归档' prd_mempalace.md`, 共命中 `8` 行。本清单只读核对生成, 未修改 `prd_mempalace.md` 或任何已有文件。

## 1. 逐行改指向清单(共 8 行, 与 grep 命中一一对应)

| 行号 | 原文摘录(前后各约 40 字) | 应指向的已归档文件 / 缺口说明 |
| --- | --- | --- |
| 43 | `...已审未过期豁免红不应停机(S1,agy-flow)。证据在会话临时目录 R25 与 R27,待归档。- planned_units 只计进候选键的工作项且必须是数字...` | `research/2026-11-24/R25-plan.md`、`research/2026-11-24/R27-design.md` |
| 44 | `...0.1.0 公开前无槽位的新发现不登记,只记未验证项。证据 R30(会话临时目录,待归档)。- 已发布冻结链 0.0.105...网级证据(cdx 2026-11-24,research/cdx/r24-l2,待归档):把该边目标改为 f0.r 后...` | (a) `R30` -> `research/2026-11-24/R30-boundary.md`(同行 `fable R31` 可并指 `research/2026-11-24/R31-plan24h.md`); (b) `research/cdx/r24-l2` -> 无归档,需补拷(源在 `~/.local/share/unisacc/research/cdx/r24-l2/`, 含 `normal-entry.md` 等 11 个文件, 仍在盘上) |
| 91 | `...(4) devsvc 默认按 HEAD 构建,核心改动须加 --dev 自测工作区版本。入口:证据在会话临时目录,待归档(R5 路线、R4 结构评审)。回滚:删本条。` | 无归档,需补拷(缺源 `~/.claude/jobs/7f46e032/tmp/R4/structure.md`、`~/.claude/jobs/7f46e032/tmp/R5/devsvc-roadmap.md`, 会话临时目录中已不存在) |
| 99 | `...造工具护栏(试行到 2026-12-04):(1) 优先并入现有工具的子命令,不新开文件;(2) 一次性分析脚本仍用 .cx,留会话临时目录不入库;(3) 每天度量 tools...` | 规则条文(规定一次性分析脚本留会话临时目录, 非具体证据指向, 无需改指向) |
| 113 | `...不实跑 affected 选出的批次,改被多处 use 的模块可绿进 main。入口:证据在会话临时目录,待归档(E4 评审)。清零:提交门接 affected --run...` | 无归档,需补拷(缺源 `~/.claude/jobs/7f46e032/tmp/E4` review, 会话临时目录中已不存在) |
| 115 | `...run-batch 断言计数盲区(E9):零断言或计数不进结论的批次可记 PASS,候选清单在会话临时目录,待归档(E9 zero-candidates);不据此删批,逐批补断言或标 UNKNOWN。` | 无归档,需补拷(缺源 `~/.claude/jobs/7f46e032/tmp/E9` report/zero-candidates, 会话临时目录中已不存在) |
| 135 | `...崩溃另立 0.0.126 单元(定位崩溃点、统一宿主容量与表限值事实源、具名拒绝)。证据 research 待归档,会话临时目录 R24/R26。未入 state-unverified.tsv,待补行。` | `R26` -> `research/2026-11-24/R26-ruling.md`; `R24` -> 无归档,需补拷(源 `~/.claude/jobs/7f46e032/tmp/R24/design.md` 仍在盘上, 第一件因第 97 行编译器位置表名触发关键词过滤而跳过) |
| 149 | `...过期冻结行在新 pre-push 门(D7)落库前会被 devsvc 当解冻、被 pre-push 忽略,两端口径不一。依据:0.0.94-FREEZE 过期...入口:build/freeze.tsv;D7 报告在会话临时目录,待归档。` | 无归档,需补拷(缺源 `~/.claude/jobs/7f46e032/tmp/D7` report, 会话临时目录中已不存在; 后续修订版已归档为 `research/2026-11-24/D8-report.txt`) |

## 2. research/2026-11-24/ 下未被 prd_mempalace.md 引用的已归档文件

当前 `research/2026-11-24/` 共 24 个文件(含本清单前); 其中 5 份已被 `prd_mempalace.md` 提及(`R25-plan.md`、`R26-ruling.md`、`R27-design.md`、`R30-boundary.md`、`R31-plan24h.md`), 其余 `19` 个文件尚未在 `prd_mempalace.md` 中出现引用:

(1) 审查报告 `G19` 至 `G29`(11 个):
- `research/2026-11-24/G19-review.txt`
- `research/2026-11-24/G20-review.txt`
- `research/2026-11-24/G21-review.txt`
- `research/2026-11-24/G22-review.txt`
- `research/2026-11-24/G23-review.txt`
- `research/2026-11-24/G24-review.txt`
- `research/2026-11-24/G25-review.txt`
- `research/2026-11-24/G26-review.txt`
- `research/2026-11-24/G27-review.txt`
- `research/2026-11-24/G28-review.txt`
- `research/2026-11-24/G29-review.txt`

(2) 设计与修订报告(5 个):
- `research/2026-11-24/D8-report.txt` (对应第 149 行 D7 的后续修订版)
- `research/2026-11-24/R22-design.md`
- `research/2026-11-24/R23-design.md`
- `research/2026-11-24/R28-design.md`
- `research/2026-11-24/R29-design.md`

(3) 本日清理与进程留痕(3 个):
- `research/2026-11-24/tmp-inventory.md`
- `research/2026-11-24/hung-tests.md`
- `research/2026-11-24/tmp-cleanup-receipt.md`

## 3. 汇总

(1) `grep` 命中行数: `8` 行(第 43、44、91、99、113、115、135、149 行; 其中第 99 行为规则条文, 其余 7 行为具体证据指向; 第 44 行与第 135 行各自含 2 处证据引用)。
(2) 已有归档可立即改指向: `5` 份(`R25-plan.md`、`R26-ruling.md`、`R27-design.md`、`R30-boundary.md`、`R31-plan24h.md`)。
(3) 需补拷项合计 `7` 项(源仍在盘可补拷 `2` 项共 `12` 个文件, 源已丢失 `5` 项):
- 可立即补拷(源在盘): `~/.claude/jobs/7f46e032/tmp/R24/design.md` (1 个文件, 经确认第 97 行位置表名非凭据) 及 `~/.local/share/unisacc/research/cdx/r24-l2/` (11 个文件: `entry-probe.cx`、`l2-entry.log`、`l2-entry-repaired.cov`、`l2-entry-repaired.log`、`l2-struct.cov`、`l2-struct.sim.log`、`module-import.cx`、`module-tape-resources.tsv`、`normal-entry.md`、`struct-call.c`、`struct-return.c`)。
- 源已不存在(`~/.claude/jobs/7f46e032/tmp/` 下已无目录): `R4/structure.md`、`R5/devsvc-roadmap.md`、`E4 review`、`E9 report(zero-candidates)`、`D7 report` 共 5 项(建议另寻备份或按实况标明源已佚)。
