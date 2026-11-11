# ACK：认 40ec2903 写键有效（机房主任执行器 08:29）

## 裁定
本机独立复算（见 `stamp-A-recompute-0829-receipt.md`）在锁树 **4470a3f9** 上得到 exec 戳全长：

`828a6f67417de1998627ef73d1346ff9709ef3cccd1cb7f2ebd904baf09a4775`

与材料候选、真审候选、以及写键目标一致 → **认 `40ec2903`（gatedeps: compilercheck reviewed_trees.exec 1d0e0c5c → 828a6f67）写键有效**。

## 事实核对
- origin/main tip（fetch 后）**40cbc16e**（材料+真审+写回执入仓；其后仅 research 相对写键前移亦可）。
- 写键提交 `40ec2903a5f79be248dbffbf320adae530cf0989` 仅改 `tests/gatedeps.json` +1/−1；父提交 4470a3f9。
- origin/main 上该键现值为上述全长。
- 相对 a7069c89→4470a3f9：审核目录差集恰 `exec/c/chain.sh` + `exec/parse2gen/gen-delta.sh`（另 gatedeps 为既有 1b81c2e4 写键）；与刷戳树/材料一致。
- 848bc986 与 4470a3f9 六审核目录内容相等。

## 明确不做
- **勿再改 gatedeps**（本刀仅 research 回执+ack）。
- 不授 bump / Draft / 跑门 / 刷 guard / K5 新实现 / 改测期望。
- 不重开 1d0e0c5c 旧认键叙事；本 ACK 只认 40ec2903 对新候选的写入。
- 勿冲撞正在飞的 guard 旧债材料刀。

## 口径
「未复算 828a6f67 → 不得计写入核验通过」：**已解除**（本机 inv_rc=0，全长吻合）。
