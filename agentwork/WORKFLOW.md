# agentwork/WORKFLOW.md —— unisacc 的开发工作流与管道

本文件是稳定规范:谁都按它干活,换窗口、换模型、换人都不用重新交接。易变的内容各有归处:
规则 → `AGENTS.md`;产品与架构 → `prd.md`;**当前要做什么 → `agentwork/items.tsv`;做到哪了 → `agentwork/ledger.tsv`**;角色提示词 → `agentwork/roles/*.md`。
不再为"交接"单写带日期的 `plans/*.md`;`plans/` 只放调查、设计探针与一次性研究。

## 1 三条原则
1. **事实由命令产生,判断由模型产生。** 构建、检查、字节比对、棘轮是命令节点,模型不转述其结果;模型只做定位、评审、裁决,且必须引用命令输出。
2. **闸门是可执行命令。** 每个工作项开工前写好 gate;"完成"=gate 在干净副本上输出约定结果,而不是谁说完成。没有 gate 的工作项不入库。
3. **改动先在私有副本;只有一个集成者写 main。** 实现节点产出 patch,不直接写共享检出。

## 2 进入一个任务(任何 agent 的开工顺序)
1. 读 `AGENTS.md`(规则)→ 本文件 → 你的角色文件 `agentwork/roles/<role>.md` → `agentwork/items.tsv` 里你的那一行。
2. 只读该行 `context` 列出的文件;要读别的,先在回信 `needs` 里说明。
3. 在私有副本(`git archive HEAD` 到 `out/work/<id>/`)里做;跑 gate;把输出原样附在回信里。

## 3 工作项 `agentwork/items.tsv`
制表符分隔,一项一行:
`id | goal(一句) | role | domain(可写路径,分号分隔) | gate(验收命令) | context(必读路径,分号分隔) | budget(分钟) | deps | state`
- `domain` 外一律不改;两个工作项 domain 相交则后者排队。
- `state` ∈ `READY / DOING / VERIFYING / DONE / BLOCKED / REOPENED`;状态以 `agentwork/ledger.tsv` 最新行为准,items 里的 state 只是快照。

## 4 账本 `agentwork/ledger.tsv`(只追加)
`时间 | id | 事件 | commit | 证据(命令+输出一行或其哈希) | 角色/模型`
看板、todo、信封里的口头状态一概不作依据;"落后项"=roadmap 里已到期而账本无 DONE 的项。

## 5 节点类型与标准管道
| 类型 | 谁执行 | 产出 |
|---|---|---|
| CMD | pipeline runner(`tools/pipeline.cx`,cx 实现;每步沿用 55/60s bound) | 事实:rc、输出、哈希 |
| JUDGE | `workflow` 调用:`locator` / `reviewer` / `arbiter` | 固定 JSON 决策(见 §6) |
| IMPL | `workflow` 调用:`implementer` | patch(私有副本的 diff) |
| INTEGRATE | 单一集成者 | main 上的提交 + 账本行 |

```
INTAKE → LOCATE → [DECIDE] → IMPL → VERIFY → REVIEW → INTEGRATE → RATCHET → SLOW-DRYRUN
```
- **LOCATE**:两个不同模型并行给落点(`path:line`)与一条验证命令;一致直通,不一致交 arbiter。
- **VERIFY**:在**全新副本**上应用 patch,依次:编译受影响 `.cx` → gate → `tools/affected.cx --run` → 声明了的字节等价回归。失败不进 REVIEW;失败输出作为上下文退回 IMPL,最多 2 轮,再败 → BLOCKED 上报。
- **REVIEW**:输入=patch+VERIFY 输出;查越界、未验证声明、同类漏同步(grep 全部读写点)。只能给 PASS/FAIL+证据,无权批准。
- **INTEGRATE**:`commit-own` 只带 domain;提交说明写做了什么,不写"更新计划"。
- **PREFLIGHT(门前快检)**:提交前可用 `tools/integrate-patch.cx ROOT PATCH DEST --stop-after N`(N=1..5)只跑前N个gate阶段,输出 `INTEGRATE_PREFLIGHT status=PREFLIGHT_OK` 即止;不写 READY,不构成集成资格,正式集成仍须完整跑过全部阶段。
- **RATCHET**:提交后立即 `tools/ratchets.cx`;红则新提交 revert(不用 reset),账本 BLOCKED。
- **SLOW-DRYRUN(慢车道预演)**:构造输入(`tools/ exec/ cx/ src/ seed/`)有新提交 → 私有根预演慢车道;红则回到最近改该文件的工作项,置 REOPENED。

## 6 模型节点的输出契约:规范化推理(`omp-workflow` 机器校验;不符退回重答一次,再不符记 INVALID)
推理允许且鼓励,但只能写成**带证据的推理链**,不能写成叙述。
```json
{"decision":"一句结论(≤240字)",
 "steps":[{"claim":"一句断言(≤140字)","ref":"path:line | cmd","quote":"逐字摘自该处原文(≥4字)"}],
 "rejected":[{"option":"","reason":""}],"risk":"一句","confidence":0.0,
 "verified":["实际运行/读到并有输出的"],"inferred":["未验证的推断"],
 "next":["可执行命令"],"needs":"需要调用者决定的事,无则空"}
```
机器校验项(失败即退回并说明哪一项):
1. `steps` 1..8 步,前提→推论顺序,最后一步直接支撑 decision;
2. 每步 `ref`=`path:line`:文件须存在、行号不越界、`quote` 须逐字出现在该行±3 行内(编造引用必被抓);`ref="cmd"` 表示命令输出,计入 `refs_unverified`;
3. 至少 1 步有可校验的 `path:line`(arbiter 除外);reviewer 的 decision 以 PASS/FAIL 开头;locator 的 next 含验证命令;
4. 废话词表(开场白、客套、“让我/首先/综上”等)出现即拒;总长 ≤1200 字(×3 字节);
5. `verified` 与 `inferred` 分开,没运行过的只能放 `inferred`。
输出自带 `checks:{refs_verified,refs_unverified}`,调用者据此判断可信度;记录落 `out/workflow/*.json` 与 `metrics.tsv`。

## 7 提交与集成规矩
- 提交请求必须含:改了哪些路径、**执行了什么、输出什么**。只有 `git apply --check`/只编译,不算验证。
- 共享表(`tests/batches.tsv`、`tests/coverage.tsv`、`tests/toolsloc.tsv`)用 `tools/commit-own.cx --keyed`,只带自己的行。
- 纯文档不单独提交,随下一个实质提交带上;文档提交后不重跑棘轮。
- `release-build` 取源码用 `git archive HEAD`:没提交的改动不会进构造结果。
- 构造链每步有硬 bound;构造运行期间不并行跑批次(并行负载会造成假超时)。

## 8 并发与冲突
读并行、写按 domain 串行;冲突域只有三类:工作树、候选/材料身份、harness。runner 持 `agentwork/locks.tsv`(domain→占用项)。

**按任务图并发推进(`agentwork/dag.tsv`,工具 `tools/dag.cx`)**:集成者每轮先跑 `sh ./unisacc.com tools/dag.cx -- plan --max N`,把输出里每个 `PLAN START` 节点**同时**派发出去(各带该行的 write_domain 与 acceptance),不逐个串行等;派发即 `dag.cx -- mark ID RUNNING`,节点的 acceptance 实跑通过并落提交后 `mark ID DONE SHA`,然后立刻再跑一轮 plan。`HOLD domain/shared` 表示与在跑节点写域相交,等对方 DONE;`WAIT` 表示依赖未完。共享登记表(batches/coverage/tools-compile-sources)在 S7 DONE 之前算同一个冲突域。改图后跑 `dag.cx -- check`(无环、依赖存在、DONE 的提交在 git 中)。

## 9 度量与淘汰
每次 JUDGE/IMPL 记录 token、时间、是否 INVALID、`verified` 条数;按"角色×模型"统计产出(DONE 数/消耗),低产组合降级或淘汰。

## 10 编排者的职责
写工作项与 gate、回答 `needs`、裁定 BLOCKED、维护 roadmap。不逐窗口转述、不手工转抄状态。人工介入点:gate 设计、BLOCKED 裁决、发布第 7 步(签名/Draft/Latest/公开)、花钱、对外发送。

## 11 归档准则(减少干扰信息)
- **只归档不删除**:`git mv` 到 `archive/<原目录名>/<日期或版本>/`,保留历史;不改写内容。
- 可归档判据(须同时给命令证据):① 无入站引用——除 `archive/` 外,被 `tools/ tests/ src/ cx/ prd.md AGENTS.md agentwork/WORKFLOW.md agentwork/ agentwork/roles/` 引用次数为 0,或引用已同步改到新路径;② 内容已被取代(有新文档/新工具覆盖)或对应工作项已 DONE/作废;③ 不是生成物、不是测试输入、不是 `tests/batches.tsv`/`coverage.tsv` 登记的 fixture。
- 禁区:`apps/csih/`、`ujs/`、`kernel/`(生成物)、`include/`、`seed/`、`weights/`、`exec/`、`tests/`、`tools/`(这些只能经各自工作项处理)。
- 归档后:入口文档(`prd.md` 记忆宫殿、`plans/README`)里的链接同步;`tools/ratchets.cx` 9/9;`tests/run.cx -- --smoke` 不退化。
- `plans/` 只留当前仍在驱动工作的调查;其余按日期归档。`out/`、`tmp/` 不入库,不属此处。
