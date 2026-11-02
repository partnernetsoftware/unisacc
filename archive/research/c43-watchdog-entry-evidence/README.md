# 监督清理与准备入口摘要

Linux x86_64隔离实测：原modelobject内部bound30，外原生监督32.25覆盖既有清理余量；父/setsid孙进程忽略TERM负控均被回收，实际30.081099秒。未放宽原命令或套件预算，未改变通用bound/seedprep。

本树模型对象八份独立bound55准备通过，21实际输出与原配方全SAME，保留对应bytecmp短表；chain独立冷准备11输出相等。原五分片/模型消费者仍需具名闸实跑，不能将准备收据当PASS。过程trace、runner、日志/status已外置。

原始逐过程材料保留在公开Git提交 `033a45f9467bc8b3331b51a59443c80cf4a7ab38` 的同路径，可独立恢复，不依赖私人备份：

```sh
EVIDENCE=$(mktemp -d)
git archive 033a45f9467bc8b3331b51a59443c80cf4a7ab38 -- research/c43-watchdog-entry-evidence | tar -x -C "$EVIDENCE"
```

公开复核先使用仓内 `./tests/gate.sh --list` 找到本摘要对应具名闸，并按release/RELEASE-PIPELINE.md的独立scratch/现行预算执行；原始操作配方可从上面的公开历史文件恢复。
