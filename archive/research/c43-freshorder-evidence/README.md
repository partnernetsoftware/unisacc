# Fresh allocation基线修正摘要

同一8377aee0bc源码和输入重放八模式，新增bit-field promotion所需BC_b查询，未删除既有键或改变其答案。保留summary/inputs/insertions和独立compare.c；16份旧/新完整回放压缩副本已外置。

真正测试基线在tests/中，仍须由原fresh-order各模式验证；本次证据不是全量F4或新的豁免。

原始逐过程材料保留在公开Git提交 `033a45f9467bc8b3331b51a59443c80cf4a7ab38` 的同路径，可独立恢复，不依赖私人备份：

```sh
EVIDENCE=$(mktemp -d)
git archive 033a45f9467bc8b3331b51a59443c80cf4a7ab38 -- research/c43-freshorder-evidence | tar -x -C "$EVIDENCE"
```

公开复核先使用仓内 `./tests/gate.sh --list` 找到本摘要对应具名闸，并按release/RELEASE-PIPELINE.md的独立scratch/现行预算执行；原始操作配方可从上面的公开历史文件恢复。
