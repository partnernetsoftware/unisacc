# K5-1i guard 真审回执（cdx；0844 裁定）

审查时间：2026-10-11T08:45:45.794149+08:00。依据同目录 `ruling-k5-1i-guard-true-review-0844.md`，限定只读核验；仅写本回执及材料 §4 勾选。未修改 prd、gatedeps、源码或裁定。

## 独立复算（实际跑过）

- `git rev-parse HEAD` 与本地 `origin/main` 均为 `c928581fd386b2da90cb05e6779c9b480c79cb50`；未 fetch，不作远端实时状态主张。
- 对 `git show c928581fd386b2da90cb05e6779c9b480c79cb50:exec/c/chain.sh` 原始字节以 Python hashlib.sha256 独立复算，得 `5f5d19bab8a0c9a531df4693777ebe3840237df60f80dbcd615f1b715685c0a0`，与候选全长相等。工作区文件逐字节等于 tip blob；blob ID 为 `82956486c71734e400ed700d65aab8c89c225d5b`。
- 对 `git show 495963b2:exec/c/chain.sh` 独立复算，得旧值 `ae17a9515fa5aed38619486621a9e1682f29e569f14dfb45e2f2954f75954587`。
- tip `tests/gatedeps.json` 与工作区逐字节相同；拒绝重复 JSON 键的解析通过。正文旧值恰 5 次，候选值 0 次；递归遍历全部值，旧值路径恰为下表，无旁路持有者。

| JSON 路径 | gatedeps 行 | 旧值核对 |
|---|---:|---|
| `suites.exec-chain-1.guards["exec/c/chain.sh"]` | 3646 | 全长相等 |
| `suites.exec-chain-2.guards["exec/c/chain.sh"]` | 3715 | 全长相等 |
| `suites.exec-chain-3.guards["exec/c/chain.sh"]` | 3784 | 全长相等 |
| `suites.exec-chain-4.guards["exec/c/chain.sh"]` | 3853 | 全长相等 |
| `suites.exec-chain-5.guards["exec/c/chain.sh"]` | 3922 | 全长相等 |

## 差集与夹带核验（实际跑过）

分别执行 `git diff --name-status BASE c928581fd386b2da90cb05e6779c9b480c79cb50 -- tests/gatedeps.json tests/gatequeue.py exec include kernel src unisa weights`，BASE 为 `40ec2903a5f79be248dbffbf320adae530cf0989` 与 `fd6aea3a`，两次均为空。此处“审核目录”指上述六个源码/数据目录，不指 research 文档目录。

仅在内存中对 tip JSON 原文模拟五处旧值→候选替换，未写回文件。递归比较解析前后对象：类型、成员集合、数组长度全部相同，变化路径恰为上述五项，每项旧值/新值准确；unified diff 恰五行删除、五行增加。`families.compilercheck.reviewed_trees` 整体相等，exec 仍为 `828a6f67417de1998627ef73d1346ff9709ef3cccd1cb7f2ebd904baf09a4775`；其它 guard 与其它键均相等。无 `REFERENCE_KEYS` 顶键，也未新增任何键。

## 材料 §4 勾选结果

- [x] 五键路径具名准确；旧值恰 5 次；无其它 REFERENCE_KEYS / reviewed_trees / 旁路 guard 夹带。
- [x] 候选新值 = tip 上 `exec/c/chain.sh` 内容 sha256 全长（独立复算，得 `5f5d19bab8a0c9a531df4693777ebe3840237df60f80dbcd615f1b715685c0a0`）。
- [x] 相对 40ec2903/fd6aea3a：审核目录+gatedeps+gatequeue 差集空；拟改后差集仅五字符串（内存模拟已核验）。
- [x] 材料页未暗示可直接写；未授 bump/Draft/跑门。
- [x] 真审只读；写入 gatedeps 要另外授权。

材料原件 §4 的五项亦已勾选；“本页不勾”表述属于重采时材料作者状态，本回执记录审查者后续勾选。以上是 guard 更新范围与哈希核验，未跑门，不构成产品覆盖或发布验收证据。未写键、未 bump、未开 Draft、未提交或 push；裁定原件保持原样。报告已写入仓内持久路径。

PASS 可进刷键裁
