# K5-1d 回执（chain.sh pp → gen-delta；2026-10-11）

**状态**：产品 tip **1bd685b0** 已 merge 进 `origin/main`（merge commit 见 push 后 tip；勿 force；**不 bump**）。

## 授权
- 机房主任 ≈02:35 YES 窄实现（`ruling-k5-1d-narrow-impl-0235.md`）
- 02:43 解锁三方只读核；本轮代裁：受控绿 → 催 cdx/cdx2；核过则 FF/merge；回执可落 research；不 bump

## 差集（4693779d → 1bd685b0）
| 路径 | 范围 |
|---|---|
| `exec/c/chain.sh` | 无 flag `gen.py pp` → `exec/pp/gen-delta.sh`；私有 `SEED_GEN_DIR="$T/.seed-gen-cache"`；stderr/out → `$T/e2-gen.{err,out}` |
| `exec/pp/gen-delta.sh` | **仅头注释**卫生（consumers=prepare K5-1c + chain K5-1d；run.sh 仍 gen.py） |
| `tests/seedppchaincheck.sh` | 新受控夹具 |
| `tests/gate.sh` | job `seedgen-ppchain`；LIST=3 `native-posix\|host\|cc,python3\|cdx\|seed-gen-pp-chain` |

未改：run.sh / prepare.sh / exec/opt/* / check 验收 / version.h（仍 0.0.39）/ gatedeps.json。

## 受控读数
| 轮 | tree | rc | SAME/DIFF | 外层 s | e2 sha256 | 夹具 sha |
|---|---|---:|---|---:|---|---|
| run2（脏树 overlay） | 4693779d+overlay | 0 | 20/0 | 48.172 | ee8ac744… | 135d299c…（≠ tip；cdx 阻断） |
| tip 复跑（闭合身份） | **1bd685b0** | 0 | 20/0 | 50.784 | ee8ac744… | **28a0c4c2…=tip** |

NEG red/missing/badsg：helper 3/2/2，chain 1，lex=0，无 Python 回退。MUTANT 识破。SEED_GEN=0 同字节。

## 只读核
- **cdx**（`/tmp/cc40-prep/next/k5-1d-review-cdx-0243.md`）：生产范围+NEG 限定通过；同提交夹具身份曾未闭合（上表）。
- **grk tip 复跑**（`/tmp/cc40-prep/k5-1d/receipt-tip-rerun-1bd685b0.md`）：闭合夹具身份；20/0 ≤60s。
- **grk/cdx2 清单**（`/tmp/unisacc-cdx2/workflow-efficiency/k5-1d-watch-grk-0248.md`）：墙钟/漏证/越权哨通过；不称五片全绿/净提速/门绿。
- cdx2 具名 `k5-1d-watch-0243.md` 本窗未落；以 tip 复跑+上列清单进 merge。

## 不称
exec-chain 五片端到端、净提速、gate-infra 绿、冷编单段、RSS、中断/损缓存、bump、Draft。

## 仓外证据
- schema：`/tmp/cc40-prep/k5-1d/evidence-schema.md`
- 原受控：`/tmp/cc40-prep/k5-1d/evidence-controlled/` + `receipt-controlled.md`
- tip 复跑：`/tmp/cc40-prep/k5-1d/evidence-tip-1bd685b0/` + `receipt-tip-rerun-1bd685b0.md`

#### K5-1d 当前口径（机房主任 02:55 限定收口；只追加，上段为历史）
上段“tip 复跑 rc0 20/0、外层 50.784 s”是 grk 那次复跑的读数（回执未引 02:47 授权），按 02:55 裁定作**只读旁证、不并入**。限定收口以 cc 依 02:47 授权的干净复跑为准：锁 1bd685b0，夹具身份 28a0c4c2 跑前落盘并闭合，gate rc0、20 SAME / 0 DIFF、外层 49.583 s（两次同机并发重叠，墙钟都不代表单独耗时）。schema INCOMPLETE 顺延（共享检出四文件前后 sha 未采；残留 scratch 来源 UNKNOWN；参考子 rc 未存；证据复制 `|| true`）；夹具无 INCOMPLETE 行 ≠ schema 齐。不称 exec-chain 五片全绿、不称净提速。回执：/tmp/cc40-prep/k5-1d/receipt-rerun-0247.md（仓外）。

