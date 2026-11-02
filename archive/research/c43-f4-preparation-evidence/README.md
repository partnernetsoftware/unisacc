# F4有界准备和完整消费者摘要

原冻结F4源62aa49e5a37501368b6bb0741393dc9561626cf6，产品SHA256为4f0b493c7a83d73a4ae1b348c8400ea3b7eda0ea190051b54a84c712a6730eda。旧失败保留在原始记录中，不以这些专项结果声明完整F4或内存准入通过。

实际结果：E4原77源按39/38、另63源按32/31保持有序全集与严格断言，均在原41秒通过。E5正例30、拒绝16、两组运输各3、real/FP两子检查均在原41秒通过；完整FP实执行550例bad0。Decimal原生消费者保留166断言；原Python参考独立观察所得166个rc/stdout/stderr逐项相同，材料SHA与cache拒绝负例见保留结构收据。冷tools/harness各55秒、真实consumer41秒，不把prepare成功冒充consumer通过。

Warn模型JSON/table/net的C与未改参考字节相等，消费者argv/结果与材料SHA保留。PyPy只是独立参考实验，不能替代未完成的CPython45秒RSS协议。临时观察器、case bytes运输编码、逐命令日志和单字节status已外置，不再作为源码树中的过程材料。

原始逐过程材料保留在公开Git提交 `033a45f9467bc8b3331b51a59443c80cf4a7ab38` 的同路径，可独立恢复，不依赖私人备份：

```sh
EVIDENCE=$(mktemp -d)
git archive 033a45f9467bc8b3331b51a59443c80cf4a7ab38 -- research/c43-f4-preparation-evidence | tar -x -C "$EVIDENCE"
```

公开复核先使用仓内 `./tests/gate.sh --list` 找到本摘要对应具名闸，并按release/RELEASE-PIPELINE.md的独立scratch/现行预算执行；原始操作配方可从上面的公开历史文件恢复。
