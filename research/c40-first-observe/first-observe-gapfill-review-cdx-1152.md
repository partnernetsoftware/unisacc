# 首观察补证只读核（cdx；1152）

采样：2026-10-11T11:54:30.943542+08:00；已读1152限定补证裁定及cc补证回执，未代执行。

- 本哨在补证前先存11项原文件哈希（make日志、candidate日志、timing、identity前后、H1前后、UA日志/二进制、step日志、shim），补证后独立重算全部保持，没有发现原证被覆写。make仍恰1行、候选目录仍仅shared日志；未见重跑或续构造。
- t0和t_make从旧原件独立复算差值=66665890 ns（66.665890ms）；回执注明来自原采样，t_end明确UNKNOWN。该差值是monotonic间隔，不能作为墙钟端点或整轮耗时；cc“墙钟间隔”用词宜收窄为经过时间。
- 新增gapfill-ps-now.txt为1152事后快照，回执明确不能替代1147原ps表，PGID/TERM/bound残留未证明。1147原表、after ls-remote、失效键/seed/exittable和真make行为均具名UNKNOWN或未验，不补造。
- UA哈希独立吻合6a05c3c6…；当前ua-build.log为空，rc0来自作者1147回执，不是独立rc原件。H1前后输出INHERIT能追溯，rc0亦非文件内原始退出码；内部h1source成功由candidate到达shared可佐证。archive38前后sha仍是原回执缩写，不能冒称新完整前后sha日志已齐。补证没有把这些局限升格产品PASS。
- GitHub实际只读无0.0.40 release/Draft；当前Gitstatus=''。未见Draft、queue、B.2/公开/seal/bump越权；不将一次事后ps查询当重跑首观察。

可追溯旧原件保持，新增事后采样边界清楚；补证回执没有逐项给identity/H1等完整SHA256清单，故不判采集清单全部完成。后续是否接受UNKNOWN由授权人裁，不由本哨自动放行。

仅落本仓外短记，未写仓/产品、重跑、bump/Draft、提交推送。

PASS 补证范围：旧原件未变、事后采样标明、UNKNOWN具名；未见重跑/Draft/queue越权，非产品验收PASS
