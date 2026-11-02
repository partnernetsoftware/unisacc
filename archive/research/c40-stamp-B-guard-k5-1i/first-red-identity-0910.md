# 首红残差身份回执（cc，0910 / 0931 处置；**事后采样**）

**本页所有哈希均为事后采样**：在 tip a4d85631 之后、本 commit 之前由 cc 读取，不是跑 r3 或写材料时的瞬时身份。它记录的是“现在磁盘与仓库上的内容”，用于核对文档残差是否闭合，不作为跑前身份证明。

## 仓库身份

- tip（HEAD，写本页前）：`a4d85631c47f881c76ed4ded0dbf9f0d18180c85`。
- `git status -sb`（写本页前）：`## main...origin/main [behind 2]`；工作树仅有 `first-red-zero-start-materials-0857.md` 已修改，以及两份未跟踪 review。
- origin/main 当时为 `546a196b`（paper-a #103，behind 2）。本回执不包含、也不拉取 paper-a 提交。
- `exec/c/chain.sh` blob：`82956486c71734e400ed700d65aab8c89c225d5b`（工作文件 sha256 `5f5d19ba…`，与 gatedeps 五键新值一致，未改）。

## 文件 sha256（事后采样）

| 文件 | sha256 |
|---|---|
| `tests/chainfirstredcheck.sh`（与 HEAD 字节相同） | `864613567dacaa841620511e67b9c343d86f99cb14a243a7fc694ebc7b54a205` |
| `research/c40-stamp-B-guard-k5-1i/first-red-zero-start-materials-0857.md`（工作文件，含 0931 前的 §6 修正） | `b6e796e5d86cd18b94c554b7de240a77bc2f1d8f1599f5e029b341ed56b01663` |
| `first-red-fixture-review-cdx-0902.md`（未跟踪） | `ef162acdce4ae3a17ce9f6717317c599948b3e09fe798b1df5b1d63a4a50fa59` |
| `first-red-zero-start-review-cdx-0857.md`（未跟踪） | `93193f33ad79824e02d79d892f46801823a4b9c999693537d1abae9e9819b9fd` |
| `first-red-receipt-cc-0902.md` | `9f309525419f33b5ec3719b84b0258928772bd6e249c9a2c333eab5de22e57e4` |

## 原件与副本

- 原件（仓外）：`/tmp/cc40-prep/first-red/r3/`；其 `SHA256SUMS` 总哈希 `71b3f97db6f1a12c7ca718dcb389f6dd15858218e2179ab25aa620cec1d56cbf`。
- 仓内副本：`research/c40-stamp-B-guard-k5-1i/first-red-run-0902/`；其 `SHA256SUMS` 总哈希同为 `71b3f97db6f1a12c7ca718dcb389f6dd15858218e2179ab25aa620cec1d56cbf`，两边 `sha256sum -c` 均通过。

## 与 0902 回执的关系

- 夹具与 r3 实跑结果见 `first-red-receipt-cc-0902.md`；本页不重复、不重跑。
- 材料 §6 已对齐：`chain: E3 gen failed` 在 stdout；六类计数（tbl / net / check-net / probe-run / ua-ref / gen）与所有事件总数在 r3 中全为 0。

## 限定（不因本页扩大）

- 仅为命令层零启动的限定 PASS；未证明 PGID/SID 清空、TERM、UA 本体行为、exec-chain 门预算、门绿或净提速。
- 未 bump、未改 version、未开 Draft、未 freeze、未改验收、未删测、未改 gatedeps/prd/chain.sh、未重跑整门。
