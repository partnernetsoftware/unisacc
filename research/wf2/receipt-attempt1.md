# WF2 首观察窗 attempt1 原账（cc，2026-10-10 21:59:38–22:01:15 SGT）——不通过，范围偏差
wt /tmp/cc40-prep/wf2/wt @4ec76cb1 status 0；queue.sh PID 2784579（已退出，无残留 queue.sh/gatequeue）。
设置：QUEUE_START=fresh，空 state=/tmp/cc40-prep/wf2/state，backup ~/.unisacc/queue-backup/cand-2b20f4b2fd9a 未读；UA fd8f；候选 2b20；白名单 0 行；QUEUE_SUITES 12 项。
**偏差**：QUEUE_WINDOWS 未显式设（默认 300），所以不是单窗观察；实际跑了 2 个窗。RELEASE_JOBS 未设，默认 4。外层 monotonic 墙钟 97.459 s。
- window 1 rc=75 22:00:27：11/12 完成，4 失败；tools-2 full 尝试 142（45.08 s），按 full-142 重试规则转 solo，日志记作 DEFER，不是 tail DEFER。
- window 2 rc=1 22:01:15：tools-2 solo rc0 44.63 s。final rc=1（不是 65）。
- 全部 START kind=full/solo，**没有一次 tail 准入**，所以 tail/恢复链未启用，也没有可读数。
- 4 个红是选套件出的错，不是产品红：queuestart-launch、queuetimeout、queuetimer、observation 这四个夹具自己会起 queue.sh，被外层活着的 queue.sh（PID 2784579）拒绝（"another queue.sh is running"），或者 launches 文件缺失。日志：state/{queuestart-launch,queuetimeout,queuetimer,observation}.log
- history：只有 /tmp/unisacc-gate-times-b28d6b8942fa3fb9.json 变化，窗前/窗后快照在 hist-before/、hist-after/。
没有重跑，没有追 tail/绿。新 attempt 等明确单窗授权。

## 补记：写入了共享默认 backup（cdx 22:0x 指出）
没有设 QUEUE_BACKUP，所以用了默认 B=~/.unisacc/queue-backup/cand-2b20f4b2fd9a。fresh 没有读它，但每窗结束后 queue.sh 的 backup() 会写它：state/ 在 22:01:15 被本轮 state 整个替换，替换过程里旧副本 state.old 被删掉。build.json 的 mtime 没变（16:06:02）。原先那份 backup 内容已经不可从此处恢复；我也没有尝试恢复。其他 backup 目录（c37-*、cand-ba3f*、unisacc37-*）的 mtime 都早于本轮，没被碰。
“无残留进程”是 owner 自己的声明（pgrep 查 queue.sh|gatequeue 为空），没有独立核证。
