# F1产品修复与重封摘要

实际基线8b82f76d，旧产品4fbfdad2…；修复后产品SHA256为4f0b493c7a83d73a4ae1b348c8400ea3b7eda0ea190051b54a84c712a6730eda。657项语料结果628 SAME、29 DIFF；非零与动态输入边界见保留的corpus-results/difference-review/nonzero-review短表。固定输入及其独立运行结果、PP/E3重封摘要保留，尤其pp-reseal.tsv仍被tests/seals说明引用。

这是本轮修复归因，不是全量F4、跨平台CI或发布通过。已知双边非零/崩溃不因本摘要获得豁免，release/rulings.tsv仍是套件裁定来源。

原始逐过程材料保留在公开Git提交 `033a45f9467bc8b3331b51a59443c80cf4a7ab38` 的同路径，可独立恢复，不依赖私人备份：

```sh
EVIDENCE=$(mktemp -d)
git archive 033a45f9467bc8b3331b51a59443c80cf4a7ab38 -- research/c43-f1-evidence | tar -x -C "$EVIDENCE"
```

公开复核先使用仓内 `./tests/gate.sh --list` 找到本摘要对应具名闸，并按release/RELEASE-PIPELINE.md的独立scratch/现行预算执行；原始操作配方可从上面的公开历史文件恢复。
