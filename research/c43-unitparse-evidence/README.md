# Unit-mode原生消费者摘要

实际同源scratch：三份原构造各bound55成功；原生consumer在原bound40成功，五个unit案例reference/simulator/native逐字节相同。whole fib仍只比simulator/reference，未声称whole native。candidate-raw/tapes.tsv保留23份输出的长度、状态和SHA，重复原始tapes/tokens二进制副本已外置。

参考尾字节变异使consumer拒绝，恢复后通过；损坏net被原校验拒绝。原Python消费者独立原预算通过；源码、identity、索引专项和完整新树F4不是同一份通过证据。

原始逐过程材料保留在公开Git提交 `033a45f9467bc8b3331b51a59443c80cf4a7ab38` 的同路径，可独立恢复，不依赖私人备份：

```sh
EVIDENCE=$(mktemp -d)
git archive 033a45f9467bc8b3331b51a59443c80cf4a7ab38 -- research/c43-unitparse-evidence | tar -x -C "$EVIDENCE"
```

公开复核先使用仓内 `./tests/gate.sh --list` 找到本摘要对应具名闸，并按release/RELEASE-PIPELINE.md的独立scratch/现行预算执行；原始操作配方可从上面的公开历史文件恢复。
