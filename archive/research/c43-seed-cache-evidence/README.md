# Seed reference缓存key修复摘要

原gen使用内容串SHA1前缀28a01f2255ab8b0d，内存准入使用路径/内容readset SHA256 key1aba99c3d52243aa。现入口调用已有原生tests/seedmemorykey.cx gen，校验family与16位十六进制并在key错误时拒绝，原策略/配方不变。

隔离实测：同源E4材料在准入key下命中；修改exec/finite_rules.py一个换行使key变化且MISS；移除源输入则非0且stdout空。小过程日志已外置。这不是全套seedgen通过、内存采集补全或新准入常量授权。

原始逐过程材料保留在公开Git提交 `033a45f9467bc8b3331b51a59443c80cf4a7ab38` 的同路径，可独立恢复，不依赖私人备份：

```sh
EVIDENCE=$(mktemp -d)
git archive 033a45f9467bc8b3331b51a59443c80cf4a7ab38 -- research/c43-seed-cache-evidence | tar -x -C "$EVIDENCE"
```

公开复核先使用仓内 `./tests/gate.sh --list` 找到本摘要对应具名闸，并按release/RELEASE-PIPELINE.md的独立scratch/现行预算执行；原始操作配方可从上面的公开历史文件恢复。
