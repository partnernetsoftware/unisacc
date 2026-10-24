# F6候选三阶段固定点

构建上下文1a26da092bc56c5b88c38cac16d59bf9c20799d6。实际seed、stage2、stage3均2670104字节，SHA256均4f0b493c7a83d73a4ae1b348c8400ea3b7eda0ea190051b54a84c712a6730eda；第二/第三阶段候选完整build.json也相同。

保留stages.json、candidate.build.json、accept-inputs.sha256、实际证明命令final-proof.txt及官方fixedpoint/双ident最终输出comboot-final-proof.log。逐窗口日志、24个中间marker/proof与其压缩副本已外置；没有把超时窗口改写为成功。该固定点不代表完整F4、Darwin/Windows本机执行或发布验收通过。

原始逐过程材料保留在公开Git提交 `033a45f9467bc8b3331b51a59443c80cf4a7ab38` 的同路径，可独立恢复，不依赖私人备份：

```sh
EVIDENCE=$(mktemp -d)
git archive 033a45f9467bc8b3331b51a59443c80cf4a7ab38 -- research/c43-f6-final-evidence | tar -x -C "$EVIDENCE"
```

公开复现配方见仓内release/RELEASE-PIPELINE.md及tests/release.sh；F6实际最后证明命令另保留于final-proof.txt。环境/前置产物须匹配；不能直接把历史路径当当前输入。
