# 独立复算回执：stamp-B chain.sh @ tip 546a196b（机房主任执行器 09:18）

## 锁 tree-ish
- **origin/main tip** `546a196b65d2c8300084120e723dd24937b86d92`（fetch 后；相对 0907 入口 de565baf 已前移 first-red + paper-a；chain.sh / 五键相对写键无再变）。
- 写键提交 `1e2f7b51b899453788573997f5c712f13b1c3889`（`chore(gatedeps): refresh exec-chain-1..5 chain.sh guards to 5f5d19ba`）；父 `c928581fd386b2da90cb05e6779c9b480c79cb50`。
- `exec/c/chain.sh` 自 `1e2f7b51`→tip **无差集**（blob 仍 `82956486c71734e400ed700d65aab8c89c225d5b`）。
- 私有 wt：`/tmp/cc40-prep/stamp-B-recompute-0918/wt` detached 到 tip；本刀仅 research。
- 说明：0907 同口径复算曾落 commit `cca9550c`，但未并入 main（被打断）；本 0918 在现行 tip 上重做独立复算并入仓。

## 算法
- 独立复算：`git show 546a196b65d2c8300084120e723dd24937b86d92:exec/c/chain.sh | sha256sum`（全长 64 hex）。
- 旁证：工作树 / 仓内 `sha256sum exec/c/chain.sh`；字节数 6670；与 tip blob `82956486c71734e400ed700d65aab8c89c225d5b` 一致。
- **不以**本机前置前缀一致代全长；本回执给出全长。

## 读数
| 项 | 值 |
|---|---|
| tip | `546a196b65d2c8300084120e723dd24937b86d92` |
| chain.sh blob | `82956486c71734e400ed700d65aab8c89c225d5b` |
| sha256 **全长** | **`5f5d19bab8a0c9a531df4693777ebe3840237df60f80dbcd615f1b715685c0a0`** |
| 目标约定值 | `5f5d19bab8a0c9a531df4693777ebe3840237df60f80dbcd615f1b715685c0a0` |
| 与目标 | **吻合** |
| gatedeps 五键 | `suites.exec-chain-{1..5}.guards["exec/c/chain.sh"]` **均** = 全长（target_count=5；旧值 ae17a951… count=0） |
| 写键 diff | 仅 `tests/gatedeps.json` ±5 行同值替换；无 reviewed_trees / 其它 guards 夹带 |

## 命令摘要
```
git fetch
TIP=$(git rev-parse origin/main)   # 546a196b65d2c8300084120e723dd24937b86d92
git show "$TIP:exec/c/chain.sh" | sha256sum
# → 5f5d19bab8a0c9a531df4693777ebe3840237df60f80dbcd615f1b715685c0a0
# Python：解析 tip gatedeps；五键均 == 全长；target_count=5 old_count=0
```

## 不宣称 / 未做
- **未改** `tests/gatedeps.json`（已是目标值，勿再写）。
- 未 bump；未 Draft；未跑门；未触 `tests/chainfirstred*`；未核 draft-front residual 为绿；未核 gate-infra 红计通过。
- 本复算是**本机独立核验**；此前「须独立复算全长才认写入核验」在本回执落盘后对 stamp-B 五键 **解除**。
- **未打断** first-red 残差刀（主仓工作树另案；本刀另 wt）。
