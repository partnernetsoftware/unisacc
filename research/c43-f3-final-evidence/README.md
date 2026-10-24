# F3完整CONTRACT结果

构建上下文1a26da092bc56c5b88c38cac16d59bf9c20799d6，根与候选SHA256均4f0b493c7a83d73a4ae1b348c8400ea3b7eda0ea190051b54a84c712a6730eda。完整187项实际完成：186 PASS、0失败、1 UNVERIFIED，原队列rc4。唯一未验证为bound的Darwin Terminal归属子项；不能改称PASS，也不覆盖其他失败。

保留权威逐项results.json、当时env-pin/input-pins和出口分类exit-table.json。十四窗口过程日志与日志副本已移出工作树，原结果没有修改。本记录只结证该历史输入下F3，不替代新树F4或外部CI。

原始逐过程材料保留在公开Git提交 `033a45f9467bc8b3331b51a59443c80cf4a7ab38` 的同路径，可独立恢复，不依赖私人备份：

```sh
EVIDENCE=$(mktemp -d)
git archive 033a45f9467bc8b3331b51a59443c80cf4a7ab38 -- research/c43-f3-final-evidence | tar -x -C "$EVIDENCE"
```

公开复现配方见仓内release/RELEASE-PIPELINE.md及tests/release.sh；F6实际最后证明命令另保留于final-proof.txt。环境/前置产物须匹配；不能直接把历史路径当当前输入。
