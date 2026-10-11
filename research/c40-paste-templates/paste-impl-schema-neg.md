# Paste 模板：实现 + schema + NEG 同刀（默认；cdx 定稿 1413）

用法：主刀/机房主任开刀时**整段复制**，填 `⟦⟧`；禁止拆成「先实现、另刀 schema、再刀 NEG」。

---

```
【机房主任⟦HHMM⟧】同刀（实现+schema+NEG；并授）：
目标：⟦一句话 claim⟧
钉身份：源 tip/SHA ⟦⟧；输入 ⟦⟧；环境 ⟦⟧（允许不同则写明）

1) 实现：文件域 ⟦路径列表⟧；预算墙钟 ⟦N⟧min；首红停位置 ⟦步名⟧；续跑点 ⟦⟧。
2) schema（采证先于/同于首跑）：必填字段=
   claim_id | src_sha | input_id | env_id | t_start | t_end | rc_source |
   log_path+sha256 | artifact_path+sha256 | verdict(PASS|FAIL|SKIP|UNKNOWN) |
   UNKNOWN_reason(具名) | authorizer。
   账本路径：⟦observe 或 research 单页⟧（摘要只引用 ID，不重抄事实）。
3) NEG（同刀必带）：至少一负例 ⟦名⟧；预期失败判据 ⟦⟧；
   明确：NEG 红 ≠ 产品 PASS；不以 NEG 当验收绿。
4) 独立复核：⟦窗名⟧ 只审新增证据↔主张；不代写键。
5) 硬禁：bump/Draft/seal/公开/放宽/⟦本刀特禁⟧。
6) 回执：路径 ⟦⟧；含 schema 行 + NEG 结果 + 禁项自检。
```

## 不做
- 不把「schema 另刀」「NEG 另裁」当默认。
- 不倒填证据；不把执行者口头 rc 升级为实测。
- 不因并授取消首红停或独立复核。
