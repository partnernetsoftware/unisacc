# Paste 模板：实现 + schema + NEG 同刀（默认；cdx 定稿 1413）

用法：主刀/机房主任开刀时**整段复制**，填 `⟦⟧`，由具名授权人批准执行包；后续默认一起安排实现、schema、NEG。模板本身不授改源或跑门权限。

---

```
【机房主任⟦HHMM⟧】同刀（实现+schema+NEG；并授）：
目标：⟦一句话 claim⟧
钉身份：源 tip/SHA ⟦⟧；输入 ⟦⟧；环境 ⟦⟧（允许不同则写明）

1) 实现：文件域 ⟦路径列表⟧；单步看门狗 ⟦N≤60⟧s、外层窗口 ⟦N≤60⟧s；累计墙钟/次数上限 ⟦⟧；首红停位置 ⟦步名⟧；续跑点 ⟦⟧。
2) schema（采证先于/同于首跑）：必填字段=
   claim_id | src_sha | input_id | env_id | t_start | t_end | rc_source |
   log_path+sha256 | artifact_path+sha256 | verdict(PASS|FAIL|SKIP|UNKNOWN) |
   UNKNOWN_reason(具名) | authorizer | run_id/job_id/attempt(无则N/A)。
   rc_source须指原记录位置；t_start/t_end注明时钟/单位，缺失记UNKNOWN，禁止事后倒填。
   账本路径：⟦observe 或 research 单页⟧（摘要只引用 ID，不重抄事实）。
3) NEG（同刀必带）：至少一负例 ⟦名⟧；预期失败判据 ⟦⟧；
   执行顺序：schema就位→具名实现→正例/NEG（各自限时）；不得首红后跳跑。
   NEG预期拒绝按原rc/诊断及无产物判据核；预期拒绝可记NEG检查PASS，
   意外接受/崩溃/超时/缺证必须停，不当预期拒绝；NEG检查PASS ≠ 产品验收PASS。
4) 独立复核：⟦窗名⟧ 只审新增证据↔主张；不代写键。
5) 硬禁：bump/Draft/seal/公开/放宽/⟦本刀特禁⟧。
6) 回执：路径 ⟦⟧；含 schema 行 + NEG 结果 + 禁项自检。
```

## 不做
- 不把「schema 另刀」「NEG 另裁」当默认。
- 不倒填证据；不把执行者口头 rc 升级为实测。
- 不因并授取消首红停或独立复核。

## 默认授权1413旁核边界
条件可执行，但须先核所有前提并记录消费次数；38111789528@82e1a9f0已由1354使用一次flaky探针，1413默认页不能重复授其二次rerun。诊断新run一次额度独立记账。固定产品源码SHA与诊断workflow/脚本版本分别具名，不假定已在飞旧SHA会自动获得新诊断补丁；无法通过现有触发绑定目标SHA则停在材料回裁。未明确触发方式不以模板自造改源push/扩大矩阵。
