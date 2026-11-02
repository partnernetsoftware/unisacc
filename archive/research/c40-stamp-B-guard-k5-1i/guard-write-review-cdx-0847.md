# K5-1i 五键写后只读复核（cdx；0847）

依据 `ruling-k5-1i-guard-write-0847.md`；实际跑过只读核验。

- 写键提交：`1e2f7b51b899453788573997f5c712f13b1c3889`；父基线 `c928581fd386b2da90cb05e6779c9b480c79cb50`。
- 工作区 gatedeps 与写键提交逐字节相同；新值 `5f5d19bab8a0c9a531df4693777ebe3840237df60f80dbcd615f1b715685c0a0` 恰 5 次，旧值 `ae17a9515fa5aed38619486621a9e1682f29e569f14dfb45e2f2954f75954587` 为 0 次。
- 提交相对基线仅 `tests/gatedeps.json`；原文差异恰五字符串（±5 行），递归 JSON 差异恰 `suites.exec-chain-{1..5}.guards["exec/c/chain.sh"]`，无成员增删及其它键变化。
- `families.compilercheck.reviewed_trees` 整体未动；其它 guards / REFERENCE_KEYS 无夹带；chain.sh 提交内容 sha256 与新值全长相等。
- 未代写键、未 bump、未开 Draft、未跑门、未提交或推送。仅落本短记；不主张远端推送或门禁验收结果。

PASS 五键写后复核通过
