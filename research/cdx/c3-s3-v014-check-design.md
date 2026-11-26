# v0.1.4 C3 S3 验收夹具规格 v2

事实表为同目录的 `c3-s3-v014-spec.tsv`。01 至 37 按 agyneon 第五版表的键和顺序一一保留，38 至 58 把回归和故障边界拆成独立断言。此前 37 个键中有 25 个未绑定，15 行夹具含额外探针。新版要求 58 键全部执行，`UNMEASURED` 上限为 0。B4 和 B5 另设提交门。这是待实现的验收规格，吸收 `research/ompgpt/c3-s3-spec-review.md` 的反向审查。

## 来源编号

| 编号 | 阻断与证据 |
| --- | --- |
| C0 | cdx 第二轮覆盖对账，25 个规格键无同名夹具。见同目录 `v013-mutation-evidence-census.md`。 |
| C1 | fatal poll 错被收尾成功或失败覆盖。旧手工变异 `/tmp/cdx-s3-r2-mut-poll.log` 存活。 |
| C2 | 截止后恢复阻塞 reap。旧手工变异 `/tmp/cdx-s3-r2-mut-blocking.log` 存活。 |
| C3 | 删 `st` 入口置零。旧手工变异 `/tmp/cdx-s3-r2-mut-zero.log` 存活。 |
| C4 | keep-root 路径抢收 root，随后持句柄收尾返回 `-ESRCH`。gatekeeper 的 `m3-verify.log`。 |
| C5 | 忽略 end 失败，伪造成功或 124。cdx 的 `/tmp/cdx-s3-r2-mut-end.log`。 |
| B1 | gatekeeper 的无截止 wait 和 run 基线约 601ms 成功，旧 S3 约 204/205ms 误超时。 |
| B2 | gatekeeper 的快收尾树/非树基线约 52/32ms，旧 S3 多付约 200ms。B1/B2 对照原行见 gatekeeper 会话的 `t1-gkc3.nOBEfy{,.s3}.log`。 |
| B4 | S1e 注入门必须有未启用、缺 PID、错 PID 和正确 PID 四条腿。 |
| B5 | keep-root 导出须同步层接口、接口摘要及适用的核心接口合同。 |
| R1 | keep-root 初探一般读错或格式错误不能报 residue=0。 |
| R2 | `kt_stat` 的 `-ENOENT` 与读取、权限、格式错误需要区分。确认身份前读不到 root 时向上报原错，已钉住身份后真实 ENOENT、代次改变或 Z/X 才能作为 root 已退出的观察。 |
| R3 | poll/read 的 `EINTR` 重试必须回外层检查同一绝对截止。 |

## 表与观测

(1) 解析固定 12 列 TSV。要求表头、全部键、状态、结果枚举、errno、位和正整数耗时合法。ID 唯一，`(slot_before, handle_before, event, outcome)` 唯一。夹具分派用固定 ID 映射，只接收对应的事件和场景。未知表行、孤儿夹具、重复执行、漏列或未认识的来源编号均红。01 至 37 另对照第五版的事件、场景键，行号保持稳定。

(2) 每格先建立真子进程和同步屏障，检查前态并绑定 `(slot,gen,pid)`。生产入口必须真实运行。通过默认关闭、显式启用且精确匹配 PID 的测试专用观察和注入接口控制单次故障，或在指定阶段持续注入。注入最底层操作和 errno，不能伪造待测 API 的返回值或代写状态。每格核注入命中次数、子进程退出可见性、调用及所有权事件顺序。关门、缺 PID 和错 PID 均不得注入。运行后采样状态再清理，清理不得影响表内断言。

(3) `release_by_api` 比较本次调用是否恰好释放该代槽一次。旧句柄、相同 PID 的不同代次、或夹具最终清理产生的释放均不算。19 由外部收尸者在表中前态采样后抢先释放，待测 API 不得再释。39 起步尚无槽，必须观察 API 启动、绑定、唯一释放。LOST 不减计数，除非同身份已确认收尸。未释放的格核下一责任方，并用夹具安全回收，末尾检查本格 FD 和槽无额外残留。

(4) 01 和 02 同时断言中间态与最终态。用握手让子进程保持 LIVE，并在实际 poll 被调用后才允许管道就绪。01 要求底层 poll 返回就绪数 1、槽与句柄仍 LIVE、未发生释放，然后允许子进程退出 7。02 在首次底层返回 `-EINTR` 后、第二次真正 poll 前采样相同状态和零释放，核两次剩余预算均来自同一截止，第二次预算随时间减少，然后核最终 `rc=0/PX7`。相位值单独输出，TSV 的 rc 始终表示 API 最终值。06 核保留全部读取字节。48/49 至少观察两次中断，越线后不得再进 poll/read 重试。

(5) 14 和 41 使用真实 Linux 脱组后代，核 root 只由持句柄路径收尸一次，且后代零残留。15、56 至 58 各保持一个已知身份的存活后代，检查清点结果后再由夹具回收，root 的 ENOENT、代次改变或 Z/X 都不能直接返回零。16、54、55 要求调用层分别保留 `-EIO`、`-EACCES`、`-ETIMEDOUT` 到 residue，观测回退 root 信号后由持句柄路径唯一收尸。43 至 47、51 至 58 从 stat 的 open、read 或解析边界注入，禁止直接改 `kt_stat` 的返回。确认身份前，首次错误原样返回，零重试、零信号。已钉住身份后，44 强制连续 EIO，业务预算 200ms，至少两次实际有界非阻塞观察，首个 EIO 后仍继续观察，到期返回 `-ETIMEDOUT`，到期后不得再发起 stat，最后一次采样时间须在截止以前。检查零残留报告、错误和实际清点分别独立成立。

(6) 在夹具兜底清理前对账。05、09、12、13、18、26、29 在局部句柄消失前须看到 `LOST_ABANDONED/DETACHED`、FD 已关、同代未释，lost 计数相对转 LOST 后不减少。普通移交同样须看到 `ABANDONED/DETACHED` 与 FD 已关。正常局部收尸须看到 `FREE/REAPED`、缓存标记 `pid=-3`、同代唯一释放及 FD 关闭，最终外部句柄为 NONE。19 在调用 end 前单列 `FREE/STALE`，可重用槽分配新代，核旧借用的拒绝对新代零释放、零信号、状态不变。36 从实际 wait 边注入可上浮的 `-EIO`；真实 waitpid 的 EINTR 由 S1 内部重试，不能冒充这个上层错误格。

## 单独提交门

(1) B4 绑定现有 S1e 批次 `check-proc-own-sat`，核 `9e827883c` 已有具名原行。对 S3 新增注入入口再跑四条具名腿，分别为 `B4-unarmed-exact-pid`、`B4-armed-missing-pid`、`B4-armed-wrong-pid`、`B4-armed-exact-pid`。使用同一真实可就绪的管道，前三腿注入计数为 0，正常读取并退出 7；正确 PID 腿注入一次 fatal poll EIO，返回 `-EIO`。分别移除启用门、缺 PID 拒绝、PID 比较或正向注入，须触发对应具名失败。这些是注入门证据，不计入 58 行。

(2) B5 在固定提交运行 `sh ./unisacc.com tools/derive.cx -- --check --only layer-api,iface-hash` 与 `sh ./unisacc.com tests/coreapi.cx -- --check`，保存各退出码及 `LAYER_API_SUMMARY`、`IFACE_HASH`、`CORE_INTERFACE` 摘要原行。若引入 `rp_kill_tree_keep_root` 公共导出，确认实际签名、`build/layer-api/ext.sysx.tsv` 派生条目及接口摘要匹配，另核适用的 core-interface 登记；测试专用探针按测试接口登记。若保留为内部实现，取接口派生原行并核无新增公共声明。B5 须独立成立。

## 出参与耗时

| profile | 逐列预期 |
| --- | --- |
| PXERR | code、timed_out、sig、truncated、residue、wall_ms、outn、out_total、errn、err_total 全为 0，out 与 err 为 NULL。另用夹具时钟记录耗时，并对账分配与释放。 |
| PX7 / PX7ABC | code=7，timed_out=0，sig=0，residue=0，truncated=0。PX7 的双流为空，PX7ABC 的 stdout 恰为 `abc`，stderr 为空。成功返回的空流仍服从既有分配合同。 |
| PX124_R0 / PX124_R1 / PX124_REIO / PX124_RNEG1 / PX124_REACCES / PX124_RETIMEDOUT | code=124，timed_out=1，sig=SIGKILL，residue 分别为 0、1、-EIO、-1、-EACCES、-ETIMEDOUT，双流为空且 truncated=0。 |
| STZERO / ST7 | `(exited,code,sig,timed_out,raw)` 分别为 `(0,0,0,0,0)`、`(1,7,0,0,7<<8)`。 |
| ST_TIMEOUT_KILL / ST_TIMEOUT_EXIT7 | 分别为 `(0,0,SIGKILL,1,SIGKILL)`、`(1,7,0,1,7<<8)`。50 第一次传 NULL，随后同句柄读缓存，另比较 `sy_exit_code=124`。 |
| TREE_EMPTY / TREE_ONE | 128 字节报告缓冲预填 0xA5，前后各加哨兵。EMPTY 要求首字节 NUL；ONE 要求写入确切存活 `pid(starttime)` 且紧随 NUL。NUL 之后可写但不能越界；rc 与实际后代清点另核。 |

错误优先级保持 poll、read、OOM、wait、end。03 至 13 及 27 至 29 固定互异的主操作错误与收尾错误。所有成功 `px_result` 同时比较 `wall_ms` 的非负与上限，以及输出长度、累计长度、字节和所有权。42 在入口初始化之前由测试钩子毒化局部 `sy_exit_t`，以 timed_out=77、code=99 等非零值覆盖所有字段，在第一次把地址交给 wait/end 之前只读采样并逐字段比零。随后照常等待真子进程退出 7 并比较公开结果。这样即使完整退出解码后来覆盖全部字段，删除初始化也会具名变红。

成功的 PX 双流均须各自返回有效可释放的 NUL 结尾缓冲，空流的长度和累计长度为 0，PX7ABC 的 stdout 长度和累计长度均为 3。夹具在采样后通过 `px_result_free` 各释放一次，确认指针和当前长度清零。

所有 `max_ms` 均从待测 API 入口到返回，排除夹具准备、预热与事后清理。14 至 19、54/55 分别记录树处理预算 2000ms 与持句柄收尾预算 200ms，以及全程 2250ms 上限。31/33 的真子进程由独立看门狗守护，在实际 waitpid 边观察标志，允许变异真正进入阻塞并报对应格，不能用不区分标志的替身；同时核收尾绝对预算 200ms。38/39 显式输入 `deadline_ms=0`，握手后约 350ms 由子进程自报并正常退出 7，实测 300 至 750ms，期间须无 SIGTERM/SIGKILL、无超时清理。40/41 的业务截止 30ms，从 `px_run` 入口计全程 150/170ms；记录首次确认收尸、最后一次 reap 和 API 返回时刻，确认收尸后无 sleep。48/49 的业务截止为 50ms，全程 250ms。其它故障格可推进测试单调时钟，仍须以独立看门狗核真实墙钟。超时记具名 FAIL，保存已达阶段原行。

## 必红对照

| 单点变异 | 预期具名红格 |
| --- | --- |
| M5 将实际 fatal poll 改为 continue，或吞掉原错 | 03 至 05；单次注入后允许正常退出，靠原 EIO、错误优先级和零重进断言杀死 |
| M1 截止后由非阻塞 reap 改成阻塞 wait | 31、33；实际 waitpid 标志或具名看门狗 |
| M2 删 px_run 的 st 初始化 | 42；验证初始化前预填和首次 wait/end 前采样各命中一次 |
| keep-root 恢复抢收 root | 14、41 |
| 忽略 end 失败 | 17、21、24 至 26 |
| keep-root 初探任一错误返回 0 | 43、45、46、53 |
| 把 stat 的权限、读取或格式错误冒充 ENOENT | 43、44、46、53 |
| 把真实 ENOENT 折叠成一般错 | 45、47 |
| 调用层把树的一般错误清成 residue=0 | 16、54、55 |
| 看到 root 消失、换代或变僵尸就跳过后代清点 | 56、57、58 |
| poll 或 read 的 EINTR 重试跳过绝对截止 | 对应 48 或 49 |
| 无截止 wait 套 200ms 收尾预算 | 38、39 |
| 已收尸仍等完固定 200ms grace | 40、41 |
| kill 返回 ESRCH 后保留旧错误，跳过正常收尸观察 | 32 |
| 缓存 timed_out 依赖 st 非空 | 50 |
| 篡改任一列、删行、加未分派行、重复键、漏观测、跨代释放 | 对应行或具名表结构错误 |

每个中间态输出 `PROC_S3_STEP id=ID phase=PHASE rc=RC slot=SLOT handle=HANDLE release=BIT remaining_ms=MS`。每个最终态输出 `PROC_S3_ROW id=ID event=EVENT rc=RC slot=SLOT handle=HANDLE release=BIT ms=MS status=STATUS`，失败带预期和实际列。各分片按固定 ID 范围互斥覆盖，核并集和执行次数，目标为 `CHECK_PROC_S3 rows=58 asserted=58 failed=0 unmeasured=0`。历史 M1/M2/M5 存活，M3/M4 有旧红；新版按固定输入逐个重跑并留原行。只有达到目标具名失败才记杀死，编译失败或未执行目标格单列。冷跑超出单批预算时按真实树、时间回归、快速故障分片，每片目标小于 25 秒。
