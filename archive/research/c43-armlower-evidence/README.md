# ARM lowering两目标摘要

原41秒job在Linux完成后、Darwin处理中超时。固定原SHA的A..E adapter保留全部源码：Linux=A+B+C+D，Darwin=A+C+E；两个完整target各在原41秒内通过。fixture/immediate/regnames/hello/fib、125010 buffered mov和超过1000000 ARG槽断言保持。此为Linux宿主跨目标lowering，不是Darwin原生执行。

保留源码覆盖/身份、完整计划对比和未知target/源SHA/NUL/缺文件拒绝短收据。过程trace/运输b64/临时observer/status已外置。exec-e4self按现行TIMEOUT/H1为RULED_BASELINE；其原执行器独立55诊断48.48秒完整相等，不替代正式41红，也不引入本版产品性能重构。

原始逐过程材料保留在公开Git提交 `033a45f9467bc8b3331b51a59443c80cf4a7ab38` 的同路径，可独立恢复，不依赖私人备份：

```sh
EVIDENCE=$(mktemp -d)
git archive 033a45f9467bc8b3331b51a59443c80cf4a7ab38 -- research/c43-armlower-evidence | tar -x -C "$EVIDENCE"
```

公开复核先使用仓内 `./tests/gate.sh --list` 找到本摘要对应具名闸，并按release/RELEASE-PIPELINE.md的独立scratch/现行预算执行；原始操作配方可从上面的公开历史文件恢复。
