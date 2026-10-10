# 代裁 ≈01:52 SGT — gate-infra 第 1 步只读定位（机房主任）

## 事实
- tip 已 FF：**6372e375 → a8a114a0**（#79 Paper A research + **c181aa19** K5-1c limited close 回执追加；非新产品差集）。
- K5-1c 产品锁仍 **96272cd2**；Latest **v0.0.39**；无 Draft v0.0.40。
- 上轮软卡（回执追加 + 下一刀草案）已落：`next-cut-gate-infra-draft.md`；cdx 限定通过；cdx2/grk 确认 stamp=清单 digest 映射非 git tree OID。
- 三方 Idle，等授权第 1 步。

## 代裁（宿主/卫生；只读定位）
1. **授权第 1 步只读定位**（按草案修订口径，不跑门、不改仓、不刷戳）：
   - 在 **b8ea2faa、ab4f75b8、当前 tip (a8a114a0)** 三处，各按**该提交内** gatequeue 算法重算 reviewed_trees/exec 清单 stamp，并列成员差集（新增/删除/变更路径）。
   - 先核旧键在 b8ea2faa 是否与重算一致 → 读资格失效 fallback 代码 → 核命令与 required_settings → 对 7 个 exec-driver-* 套件逐个核对失效原因。
   - 回执分两栏：**假设** vs **证明**；不得把未核因果升级为「很可能」。
2. **明确不授**：第 2 步两条私有复现（b8ea2faa/ab4f75b8 各一次）；任何处置 (a)(b)(c)；刷戳；改验收/删测/归因；bump / Draft；共享写者跑门。
3. **产出**：仓外回执 `/tmp/cc40-prep/next/gate-infra-step1-readonly-receipt.md`（或同等路径）；可选一行 research 进展；停等批。

## 分工
- **cc**：执行第 1 步只读定位 → 写回执 → 停。勿叠实现/复现。
- **cdx**：只读盯回执与差集是否按各提交算法；不代跑。
- **cdx2**：只读核漏证（缺成员差集？谓词未列？）+ 提效（本刀 Idle 端点）+ 第 2 步是否值得另授的建议。
