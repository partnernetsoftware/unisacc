# K5-1i stamp-B 五键写键回执（cc，2026-10-11 ~08:50 SGT）

- 授权：机房主任 08:47 代裁（条件式；真审 PASS 见 research/c40-stamp-B-guard-k5-1i/guard-true-review-cdx-0844.md 末行“PASS 可进刷键裁”）。
- 基：tip c928581fd386b2da90cb05e6779c9b480c79cb50（= origin/main 推送前）。
- 提交：1e2f7b51b899453788573997f5c712f13b1c3889（普通快进推送 c928581f..1e2f7b51，未 force）。
- pathspec：仅 `tests/gatedeps.json`；diffstat 5 insertions / 5 deletions，unified diff 恰五行字符串替换。

## 五键核表
| 键 | 写前旧值 | 写后新值 |
|---|---|---|
| suites.exec-chain-1.guards["exec/c/chain.sh"] | ae17a951… | 5f5d19ba… |
| suites.exec-chain-2.guards["exec/c/chain.sh"] | ae17a951… | 5f5d19ba… |
| suites.exec-chain-3.guards["exec/c/chain.sh"] | ae17a951… | 5f5d19ba… |
| suites.exec-chain-4.guards["exec/c/chain.sh"] | ae17a951… | 5f5d19ba… |
| suites.exec-chain-5.guards["exec/c/chain.sh"] | ae17a951… | 5f5d19ba… |

- 写前：旧值恰 5 次，新值 0 次（Python 断言通过）。
- 写后：JSON 解析通过；五键均为 5f5d19bab8a0c9a531df4693777ebe3840237df60f80dbcd615f1b715685c0a0。
- 核对 `exec/c/chain.sh` sha256 = 5f5d19bab8a0c9a531df4693777ebe3840237df60f80dbcd615f1b715685c0a0（与写入值一致）。
- 未改：reviewed_trees（exec 仍 828a6f67…）、其它 guards、REFERENCE_KEYS、gatequeue、exec 源、version。

## 未做（按裁定）
- 未 bump、未开 Draft、未跑门、未改其它键、未 force。
- 未夹带 research/ 或 prd.md 入本次 commit（prd.md 仍有 cdx2 旁观的未提交改动，属另案）。

## 说明
- 本回执只证明五键写入与哈希核验；不构成产品覆盖或发布验收证据。
