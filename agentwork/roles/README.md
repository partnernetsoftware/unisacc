# agentwork/roles/ —— 角色提示词(稳定、可复用)

每个 `agentwork/roles/<role>.md` 同时是:① 人读的职责说明;② `omp-workflow` 工具的提示词模板(`workflow(role=<name>, context=..., decision=..., budget=...)` 直接加载)。
角色与窗口、模型无关:任何 agent 接到工作项,读对应角色文件即可开工。
统一骨架(每个角色文件都按这个写,段落名固定,便于工具解析):

```
# <role>
ROLE: 你是谁,唯一产出是什么
READ: 读什么(由工作项 context 给定;越界先在 needs 声明)
DECIDE: 要做的决策与判据
SPEED: 先最短可行方案+一条命令验证;失败再扩展;预算用尽必须出结论;不写推理草稿
DON'T: 不做任务外的审计/重构/顺手修复;不扩 domain;不改文件(除非 ROLE 明确授权)
OUTPUT: agentwork/WORKFLOW.md §6 的 JSON
```
角色:`locator`、`reviewer`、`implementer`、`arbiter`、`subagent-common`(所有子代理共用的工作规则:简报只写任务特有的五段,其余写在这里)、`orchestrator`(编排者:拆任务、写简报、派工、独立复验、集成;含本会话沉淀的经验与反模式)。改动角色文件=改变全体行为,走正常工作项与评审。
