# 同 SHA flaky 探针触发回执（1354）

- 授权：代裁 ruling-darwin-b139-next-1354.md
- 命令：`gh run rerun --failed 38111789528 -R partnernetsoftware/unisacc`
- 结果：gh_exit=0；run_attempt=2；SHA 不变 `82e1a9f0`
- attempt2 jobs：114402137149 (macos-14 queued)；114402137315 (macos-15 in_progress)；114402137352 (macos-15-intel in_progress)
- 未改源 / 未 bump / 未 Draft；原 attempt1 红保全
- 盯出口另轮；本回执只证「已触发一次」
