# 冷计划原生metadata修复摘要

原K2b冷计划因构建完整seed CLI超过10秒。轻量原生reader只读同一数据/派生常量，完整冷计划实际4.82秒；规约ROOT并扣除新增seed-metadata闸后原NUL计划逐字节相同，原10秒预算不变。无效数据返回2，原K2b在40秒内完成。过程日志已外置。

45个part/manual数据行与44个自动名称仍由tests/seedconstruct-parts.tsv、原生metadata模块和派生一致性闸维持，不修改构造/参考路径。缺失/空/重复/非法flags/有序行或原字节漂移都须拒绝。正式F4仍须新身份全量。

原始逐过程材料保留在公开Git提交 `033a45f9467bc8b3331b51a59443c80cf4a7ab38` 的同路径，可独立恢复，不依赖私人备份：

```sh
EVIDENCE=$(mktemp -d)
git archive 033a45f9467bc8b3331b51a59443c80cf4a7ab38 -- research/c43-seed-metadata-evidence | tar -x -C "$EVIDENCE"
```

公开复核先使用仓内 `./tests/gate.sh --list` 找到本摘要对应具名闸，并按release/RELEASE-PIPELINE.md的独立scratch/现行预算执行；原始操作配方可从上面的公开历史文件恢复。
