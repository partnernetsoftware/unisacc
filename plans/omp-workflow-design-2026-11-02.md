# omp-workflow 工具设计(2026-11-02)

目标:把"派活→评估→决策"做成一个有结构的调用,废话由格式杜绝,不靠劝告。核心是评估与决策,不是叙述。

## 形态
moltbaby 扩展(官方 omp 包不改),`pi.registerTool` 注册工具 `workflow`。底层起子进程 `omp --mode rpc --no-session --model M --thinking L --max-time T --tools ...`,rpc 模式使调用者可随时补充/调整(steer/follow-up)而不重启。

## 参数
| 参数 | 含义 |
|---|---|
| provider / model / modelArgs | 选模型;modelArgs 透传(thinking、tools、max-time、上下文预算) |
| role | 预置角色模板名(见下),或内联 |
| context | 必读清单:文件路径+行段、命令及其期望输出;**只能读这些**,超出须在决策里声明 |
| decision | 要做的决策:问题一句话 + 候选项(可空,空则由它提出≤3 个)+ 判据 |
| budget | 最大工具调用数、最大 token、最大时间;超限强制出结论 |
| output | 固定 schema(见下),不接受自由文体 |
| steer(调用中) | `workflow.steer(id, text)` 追加约束/纠偏;`workflow.abort(id)` |

## 输出契约(工具侧强制,不符则退回重答一次,再不符则标 INVALID)
```json
{"decision":"选了什么(一句)","why":["证据1(path:line 或命令输出)","证据2"],
 "rejected":[{"option":"..","reason":".."}],"risk":"最大风险(一句)",
 "confidence":0.0,"verified":["已实际运行/读到的"],"inferred":["未验证的推断"],
 "next":["下一步动作(可执行命令)"],"needs":"需要调用者决定的事(无则空)"}
```
- `verified` 与 `inferred` 分开:没运行过的只能进 inferred。
- 总长度上限(默认 1200 字符);没有"过程叙述"字段。

## 角色模板(prompt 的固定骨架)
```
ROLE: <角色>(例:审查官/定位官/实现官/裁决官);你的唯一产出是上面的 JSON。
READ: <context 清单>。读完立即决策;不扩大范围,需要扩大先在 needs 声明。
DECIDE: <问题>。判据:<判据>。候选:<候选或"自提≤3个">。
SPEED: 先给最短可行方案并用一条命令验证,失败再扩展;不写推理草稿;预算用尽必须出结论。
DON'T: 不做任务外的审计/重构/顺手修复;不改文件除非 ROLE 明确授权实现。
```
预置角色:`locator`(定位缺陷落点,只读)、`reviewer`(评审补丁,必须执行验证)、`implementer`(限定路径+验收命令)、`arbiter`(在 N 个方案中裁决)。

## 调度(替代现在的手工信封)
- 调用者(编排者)只提交:角色 + context + decision;工具负责起进程、限预算、校验输出、落盘 `out/workflow/<id>.json`。
- 并行:多个 workflow 同时发,按 id 收;冲突域沿用仓规(读并行、写串行,按路径所有权)。
- 度量:每次记录 token/时间/是否 INVALID/verified 数,周期性淘汰产出低的 角色×模型 组合。

## 第一步(最小可用,能在 1 天内验证)
1. 扩展骨架 + `locator`/`reviewer` 两个角色 + 输出校验器(JSON schema)。
2. 回放验证:拿今天已发生的 3 个决策(rb_record 分角色、F 补丁回退、footer 归属)作为任务,看 workflow 是否在预算内给出与最终裁定一致或更优的 decision。
3. 通过后接入编排:信封改为 workflow 调用,窗口里的常驻 agent 退役为"执行者"。
