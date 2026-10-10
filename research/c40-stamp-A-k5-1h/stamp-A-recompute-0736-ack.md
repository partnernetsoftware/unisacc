# ACK：认 1b81c2e4 写键有效（机房主任执行器 07:36）

## 裁定
本机独立复算（见 `stamp-A-recompute-0736-receipt.md`）在锁树 **a7069c89** 上得到 exec 戳全长：

`1d0e0c5cf47ba3befd18d391491cb24d343dfc242aad7993a454ee57557f1825`

与材料候选、真审候选、以及写键目标一致 → **认 `1b81c2e4`（gatedeps: compilercheck reviewed_trees.exec fbb12970 → 1d0e0c5c）写键有效**。

## 事实核对
- origin/main 上该键现值为上述全长（`git show origin/main:tests/gatedeps.json`）。
- 写键提交 `1b81c2e4fbe3a96ee9999867ab249380145de8ab` 仅改 `tests/gatedeps.json` +1/−1。
- 相对 a7069c89：六审核目录对 tip 差集空；gatedeps 差集即该单键（及后续仅 research 的 tip 前移）。

## 明确不做
- **勿再改 gatedeps**（本刀仅 research 回执+ack）。
- 不授 bump / Draft / 跑门 / K5-1i / 改测期望。
- 不重开 fbb12970 旧认键叙事；本 ACK 只认 1b81c2e4 对新候选的写入。

## 口径
「1d0e0c5c 本机未复算 → 不计独立核验通过」：**已解除**（本机 inv_rc=0，全长吻合）。
