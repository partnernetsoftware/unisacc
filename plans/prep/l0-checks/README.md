# plans/prep/l0-checks:L0 小件包先红检查草稿(0.0.89 预备)

状态:只作草稿参考,不登记批次,不是现行检查。其中 6 个已被正式 tools/check-l0-*.cx 取代并归档(killtree/mtime-ns/random/rusage/syscall6/waitset);仍留 capture 与 pdeathsig(尚无同名正式夹具)。文件后缀为 .cx.draft。原 8 个草稿在已装 0.0.85 上因缺接口先红。计划见 archive/plans-pre090/v0.0.89.md。

| 草稿 | 对应项 | 0.0.85 红原因 |
|---|---|---|
| check-l0-syscall6.cx | L0b-1 sy_syscall6 | SY_NR_GETPID unknown identifier |
| check-l0-random.cx | L0b-2 sy_random | undefined function sy_random |
| check-l0-mtime-ns.cx | L0b-3 mtime_ns | sy_write_file 参数不符(草稿本身待按现接口修) |
| check-l0-waitset.cx | L0a-1 等待集 | sy_wait_any unknown identifier |
| check-l0-rusage.cx | L0a-1 rusage | sy_rusage_t unknown identifier |
| check-l0-capture.cx | L0a-2 双管捕获 | sy_capture_t unknown identifier |
| check-l0-killtree.cx | L0a-3 杀树接超时 | SY_SPAWN_KILL_TREE unknown identifier |
| check-l0-pdeathsig.cx | L0a-3 PDEATHSIG | SY_SPAWN_PDEATHSIG unknown identifier |

注:check-l0-mtime-ns 的红是草稿调用写错而非缺 mtime_ns 本身,搬正前须先改为现有 sy_write_file 签名再确认红因(未核)。原始红运行输出在预备作业目录 ~/.claude/jobs/2798e440/tmp/prep-l0/red-run.txt,不入库。
