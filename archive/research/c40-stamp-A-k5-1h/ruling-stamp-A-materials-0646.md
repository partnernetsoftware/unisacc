# 代裁 ≈06:46 SGT — 审核戳 A 材料刀（机房主任）

## 事实
- tip **origin/main = a7069c89**（K5-1h 推送闭合；fc397468+a7069c89；两对 patch-id 守恒已核）。本地检出可滞后，**以 origin/main 为准**。
- Latest **v0.0.39**；无 Draft v0.0.40（旧 Draft v0.0.31 忽略）。
- parse2gen↔memkey 核验材料 **CLOSED**（ruling-parse2gen-memkey-audit-0637）；写键 fbb12970 **CLOSED**（勿重开）。
- 三窗自 ~06:41 Idle/Ask；cc 候选页 `/tmp/cc40-prep/next/next-cut-after-k5-1h.md` 建议优先 **候选 A**（cdx2 同此）。
- exec 差集（git name-status e418bdcf..a7069c89）：**A** `exec/parse2gen/gen-delta.sh`、**M** `exec/pipeline/prepare.sh`（成员数/戳以 inv.py 复算落盘为准）。

## 代裁（宿主/卫生；只出材料）
1. **开刀：审核戳 A 材料**（post-K5-1h）。基线固定 **e418bdcf**（写入 fbb12970 时的 exec；全长 `fbb129700659fe8f225eb53a64fb384898f118b405d0a98f16ef2221bdd56336`）。目标 tip **a7069c89**。
2. **产物**：只读材料一页（形同 0506/0537）：在 a7069c89 上用 inv.py 复算，**rc 写入文件**；列出成员差集；每文件 diff 摘要 + 执行路径（prepare 冷准备）；空白勾选清单。材料落 `/tmp/cc40-prep/stamp-A-0646/`（或 research 私有草稿目录）；**本刀可不 push**；若短 commit 入 `research/` 须另报，仍不刷 stampedeps。
3. **三步分授**：本刀只授 **材料**。**不授** 真审、不授刷戳、不授条件式单键自动跳真审（「条件式单键」可写进材料作提案，不能省真审）。
4. **本轮不授**：bump / Draft / 改哈希或 DIFF / 改 REFERENCE_KEYS / chain.sh 迁 helper / 重开 fbb12970 / 实现刀 / 扩测试期望。

## 下一刀（执行）
- **cc**：立刻做材料刀；完成后只写一页「材料 DONE + 待真审清单」→ 停等批。勿叠真审/刷戳/实现。
- **cdx**：只读核材料完整性（基线 e418、差集两项、inv rc 落盘、勾选空白、未越权刷戳）；漏证立刻 mux；不代跑不改仓。
- **cdx2**：只读盯墙钟/出口/漏证/越权；确认材料优先于真审/刷戳；提效 KPI（ruling≤2、Idle≤5m）本轮 Idle 已超，本 paste 计入恢复；不写仓不代跑。

## 度量
- K5-1h ≈**100%** CLOSED；memkey 核验 CLOSED
- 审核戳 A 材料 ≈**0%→开刀**；真审/刷戳 未授
- 在研 **0.0.40** 可度量收口约 **99%**（主路径收口；戳材料在途；下一产品刀待材料后批）
- Latest **v0.0.39**；无 Draft v0.0.40

## 追记 ≈06:47
- tip 又进 **98e0f4db**（#94 paper-a P-2 walker research，仅 research/*；exec/gatedeps 相对 a7069c89 **空差**）。材料刀目标仍可用 a7069c89 或当前 tip（exec 等价）；不改授权范围。
