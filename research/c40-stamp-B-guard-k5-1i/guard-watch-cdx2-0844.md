# cdx2 0844 guard 真审旁观短记

时间：2026-10-11T08:46:03.148125+08:00。只读核验实际跑过；仅续本短记与 prd 上下文，不代真审、不代写键裁。

- cdx 回执已落仓内 `guard-true-review-cdx-0844.md`，末行为 `PASS 可进刷键裁`；材料 §4 五项已勾。
- 独立复算 HEAD=c928581fd386b2da90cb05e6779c9b480c79cb50，tip 与工作文件 chain.sh sha256 均为 5f5d19bab8a0c9a531df4693777ebe3840237df60f80dbcd615f1b715685c0a0；旧值恰五次，guard 持有路径仅 exec-chain-1…5 的 exec/c/chain.sh。两基点 40ec2903/fd6aea3a 的限定路径差集均空。
- 回执具备旧哈希溯源、拒重复 JSON 键、递归全值路径核验、内存五字符串模拟差集、reviewed_trees/其它键不变的具名证据；本旁观未重跑所有模拟，不冒充第二份完整真审。
- 回执落盘后独立确认 gatedeps 工作文件仍与 HEAD 同字节，tracked diff 为空（续记前）；未见越权或限定真审漏证。仅本地 origin/main 核验，不代证远端实时状态；未跑门，不代证产品覆盖/发布通过。
- 0837 更正与 0844 裁仍有效：PASS 只可进另一次刷键裁，当前无写键/bump/Draft/全量门授权。未改裁定，未提交/push，未访问或改动 /tmp/unisacc-cdx2/plans/ 0.0.41 合并稿。

收口：本次旁观未见越权或漏证；写键继续搁置至另裁。

## 0847 五键写键旁观续记

核验时间：2026-10-11T08:48:01.155154+08:00；实际只读核验。依据 `ruling-k5-1i-guard-write-0847.md`，写键已获另裁；本段更新前段的“搁置”状态。

- cc 提交 `1e2f7b51b899453788573997f5c712f13b1c3889` 的父提交恰为指定基点 c928581fd386b2da90cb05e6779c9b480c79cb50；commit 路径仅 tests/gatedeps.json，未夹带 research/prd。
- 独立逐字节断言：写后文件恰等于基点文件的五处旧值→新值替换；旧值 0 次、新值 5 次，unified diff 恰 -5/+5 行。解析对象独立比较也恰五键，reviewed_trees、其它 guard、成员集合及其它值全等。chain.sh 实际 sha256 与新值全长一致。
- 只读 git ls-remote origin refs/heads/main（20 秒超时）rc0，远端 main 为上述完整提交；父子关系支持快进结果。cc pane 与写回执记录普通 FF 推送、未 force；旁观未重放推送。
- 写回执已在 /tmp/cc40-prep/stamp-B-guard-0822/write/receipt.md，含 commit SHA、五键核表和边界说明；该回执当前为 /tmp 路径，不冒充已持久入仓。
- 未见五键写入越权或夹带；没有源码/版本变更证据。未跑门，不代证产品覆盖或发布验收；bump/Draft/全量门仍未获本裁授权。
- 本轮只续此短记，不改 prd.md，不接触 /tmp/unisacc-cdx2/plans/ 0.0.41 稿，未代提交或推送。

收口：五键写入及远端落地核验吻合，未见越权/夹带；research/prd 卫生仍属另案。
