# 模型对象原生验证摘要

保留最终current-parts、objects和original-21-bytecmp短表：八份有界准备、21个实际产物与原构造逐字节相等。whole.pkg SHA256为ab04343d9c29521937a77862fc2dd325e475bbfced7efb09236133f487c7185c，unit.pkg为1547593aa4c8a5d4a221334fd5852d0c8bc169ac59dcce15babc455e8c49bb86。

原生消费者、头readset/工具/environment身份和缺失/损坏/路径负控仍由tests/modelobjectprepared.cx与相应闸执行；准备HIT不是测试PASS。多代索引日志、rc、临时runner与重复manifest已外置，不能将旧身份结果用于新树。

原始逐过程材料保留在公开Git提交 `033a45f9467bc8b3331b51a59443c80cf4a7ab38` 的同路径，可独立恢复，不依赖私人备份：

```sh
EVIDENCE=$(mktemp -d)
git archive 033a45f9467bc8b3331b51a59443c80cf4a7ab38 -- research/c43-modelobject-evidence | tar -x -C "$EVIDENCE"
```

公开复核先使用仓内 `./tests/gate.sh --list` 找到本摘要对应具名闸，并按release/RELEASE-PIPELINE.md的独立scratch/现行预算执行；原始操作配方可从上面的公开历史文件恢复。
