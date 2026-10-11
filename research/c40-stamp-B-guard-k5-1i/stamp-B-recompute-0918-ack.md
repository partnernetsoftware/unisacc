# ACK：认 1e2f7b51 写键有效（机房主任执行器 09:18）

## 裁定
本机独立复算（见 `stamp-B-recompute-0918-receipt.md`）在 tip **546a196b** 上得到 `exec/c/chain.sh` sha256 全长：

`5f5d19bab8a0c9a531df4693777ebe3840237df60f80dbcd615f1b715685c0a0`

与材料候选、真审候选、以及写键目标一致；gatedeps 五键均已是该全长 → **认 `1e2f7b51`（gatedeps: exec-chain-1..5 guards["exec/c/chain.sh"] ae17a951→5f5d19ba）写键有效**。

## 事实核对
- origin/main tip（fetch 后）**546a196b65d2c8300084120e723dd24937b86d92**（相对写键仅 research / first-red / paper-a 前移；chain.sh 与 gatedeps 五键相对写键无再变）。
- 写键提交 `1e2f7b51b899453788573997f5c712f13b1c3889` 仅改 `tests/gatedeps.json` +5/−5；父提交 c928581fd386b2da90cb05e6779c9b480c79cb50。
- origin/main 上五键现值为上述全长（恰 5 次；旧值 0）。
- `reviewed_trees.exec` 仍 828a6f67…（stamp-A；本刀不碰）。
- 0907 同口径 ACK 曾本地 commit `cca9550c` 但未入 main；本 ACK 在现行 tip 重确认并入仓。

## 明确不做
- **勿再改 gatedeps**（本刀仅 research 回执+ack）。
- 不授 bump / Draft / 跑门 / 再刷 guard / 改验收。
- 勿把未核 draft-front residual、未核 gate-infra 红计通过；本 ACK 只认 stamp-B 五键写入核验。
- **勿打断**正在飞的 first-red 残差刀；本刀另 wt 只 research。

## 口径
「须独立复算全长才认写入核验」：**已解除**（本机全长吻合；五键齐）。
