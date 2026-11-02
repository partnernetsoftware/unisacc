# F1头边和E3边界重封依据

保留12边edges/added-keys/input-pins与旧头Git blob清单。旧头回放与旧封件相同，新头回放只有新增查询，共有键答案零变化；这解释F1头引入的指纹变化，不是放宽比较或负例。

产品4f0b493c…提取E3为2bbfe160b7d843e3f8081a4e4abd9b43dc65c6b3c50fd71810e31ffc38411449，与同代errorparse压缩前编码逐字节相等；完整accept材料仍位于tests/seals，不能随本目录日志清理删除。旧/新重放压缩副本与审计日志已外置，原正负闸及UNKNOWN边界保持。

原始逐过程材料保留在公开Git提交 `033a45f9467bc8b3331b51a59443c80cf4a7ab38` 的同路径，可独立恢复，不依赖私人备份：

```sh
EVIDENCE=$(mktemp -d)
git archive 033a45f9467bc8b3331b51a59443c80cf4a7ab38 -- research/c43-edge-boundary-evidence | tar -x -C "$EVIDENCE"
```

公开复核先使用仓内 `./tests/gate.sh --list` 找到本摘要对应具名闸，并按release/RELEASE-PIPELINE.md的独立scratch/现行预算执行；原始操作配方可从上面的公开历史文件恢复。
