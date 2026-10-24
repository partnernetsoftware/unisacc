# Cxbin模块正文身份摘要

实际空格路径负控：direct-first/hit1，仅直接正文改动2；transitive-first3，叶正文改动4、hit4；missing-module2。状态为测试程序有意返回，runner成功；保留完整短结果表。

测试代码现行实现以-cx-plan和每个源单元完整SHA绑定模块正文，缺计划/缺文件/哈希失败明确拒绝。原probe源、bootstrap日志和重复manifest已外置；非本轮正式全量F4证明。

原始逐过程材料保留在公开Git提交 `033a45f9467bc8b3331b51a59443c80cf4a7ab38` 的同路径，可独立恢复，不依赖私人备份：

```sh
EVIDENCE=$(mktemp -d)
git archive 033a45f9467bc8b3331b51a59443c80cf4a7ab38 -- research/c43-cxbin-closure-evidence | tar -x -C "$EVIDENCE"
```

公开重放可在恢复目录中阅读并运行原cxbin-module-probe.sh；它调用仓内产品和实际模块变异，不需要另造观察器。原probe只在独立scratch中运行。
