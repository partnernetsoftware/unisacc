# cx 核心设计(定稿 2026-11-12)

本文是 cx 核心面(`cx.*`)与扩展面(`ext.*`)的逐区设计决定。事实表是 `build/std-surface.tsv`(v1,412 行),本文不改表,第 14 节给出本设计要求的行编辑清单。稳定性与版本策略在 [stability.md](stability.md),产品需求摘要在 [prd_cx.md](../../prd_cx.md) 第 0 节,条款在 [cx/spec.md](../../cx/spec.md)。

参照来源与引用写法:

- R = [ref-rust.md](ref-rust.md),N = [ref-node.md](ref-node.md),Q = [ref-quickjs.md](ref-quickjs.md),P = [ref-cpython.md](ref-cpython.md)。`R6(7)` 表示该文第 6 节第 (7) 条。
- 「从」表示采纳,「拒」表示不采纳。四份参照里自带 `[忆]` 标记的内容,本文引用时不升级为已核。
- `[未核]` 表示本文作者未用命令或原文核对,落实现前须实测。全部未核项汇总在第 15 节。

取数口径:表的计数来自 2026-11-12 对 `build/std-surface.tsv` 的只读统计(按 `# area` 分区、按第 9 列分状态)。本轮没有运行任何检查工具(发布尾部在跑,只读),所有「现状通过与否」均未实跑。

## 1 总原则

| 编号 | 决定 | 依据 |
|---|---|---|
| D1 | 核心面由事实表定义。不在 `build/std-surface.tsv` 且不在 `build/layer-api/PKG.tsv` 的名字不是承诺 | 从 P6(1)、R6(1);拒 Q5.4(3) 的「表就是源码,无差异检查」 |
| D2 | 两个高度加四个语言补位层。`cx.sys` 是系统调用高度,`cx.os` 是便捷高度,`cx.path`、`cx.text`、`cx.mem`、`cx.stream` 补 C 没有的内建 | 从 Q6(1)(2)、P6(4);修正主人假设「sys + os + net 即够」,见 Q6 判断 |
| D3 | 一个能力一种调用形态,全部阻塞式,靠 deadline 定界。不做回调、Promise、事件循环三套并存 | 从 N6(11);拒 N 的三形态并存 |
| D4 | 六目标同面同签名。平台做不到的函数仍存在,返回 `-ENOSYS`,不让符号消失 | 从 R6(7)、Q6(7);拒 Q4 与 P4 的「函数随平台消失、靠 hasattr 探测」 |
| D5 | 错误只有一种约定:返回值负码。见第 11 节 | 从 Q6(4)、N6(4);拒 Q3 的三种并存 |
| D6 | 每个可能无限期阻塞的入口在签名或句柄上带 deadline,首版定死 | 从 Q6(5);Q 至今补不进去 |
| D7 | 新 API 先进 `ext.*` 孵化,满足判据再升 `cx.*`。不设「随核心发出的 provisional」状态 | 从 P6(3);PEP 411 已被取代,声明不稳定挡不住依赖 |
| D8 | 核心保持小,进入比退出难 | 从 R6(12)、P6(12) |

## 2 包与归属

| 包 | 高度 | 内容 | 表内行数(v1) | 归宿 |
|---|---|---|---|---|
| `cx.sys` | 系统调用 | fd 读写、管道、轮询、信号观察、tty、平台识别、错误码 | 54 | 核心 |
| `cx.os` | 便捷 | 文件系统、整文件操作、进程派生、环境、时间与 deadline | 115 | 核心 |
| `cx.path` | 纯计算 | 词法路径,posix 与 win 两种风格 | 26 | 核心 |
| `cx.mem` | 纯计算 | 带长度的堆、带界字节操作、vec、map;本设计并入 `mg_*`、`ms_*` | 27,并入后 59 | 核心 |
| `cx.text` | 纯计算 | 带界文本、拆分、UTF-8、进制、hex/base64;本设计并入 `tx_snprintf` 两项 | 39,并入后 41 | 核心 |
| `cx.stream` | 组合 | reader/writer 句柄、拷贝、管道对;带回执的有界管线 | 29 | 核心 |
| `ext.net` | 系统调用 | 地址、解析、TCP、UDP、Unix 域 | 31 | 孵化,目标 `cx.net` |
| `ext.procx` | 便捷 | 输出采集、进程树、子收割 | 10 | 孵化 |
| `ext.memx` | 纯计算 | 并出 `mg_*`、`ms_*` 后只剩 arena | 36,并出后 4 | 孵化 |
| `ext.textx` | 纯计算 | 字符串构造器、旧式数字解析、deflate | 27,并出后 25 | 孵化 |
| `ext.crypto`、`ext.tsvx` | 纯计算 | 密码与 TLS 1.3 记录层;三方合并 | 17、8 | 扩展,不以进核心为目标 |

现状与目标的差距(事实,来自 `build/tlib-packages.tsv`):

- (1) tlib 侧没有 `cx.sys`、`cx.os`、`cx.mem`、`cx.text` 这四个包,它们合在 `cx.base` 里,且 `cx.base` 还装着 `cx/core`(执行器)与 `cx/owned.cx`。稳定性按 tlib 包声明,所以声明前必须拆包:`cx.sys`、`cx.os`、`cx.mem`、`cx.text` 各成一包,`cx/core` 与 `cx/owned.cx` 归入不对外承诺的内部包。
- (2) `cx.stream` 现依赖 `ext.memx@0.1.0`(`xs_item` 含 `mg_buf`,`xs_read_line` 收 `mg_buf`)。核心包不得依赖孵化包,所以 `mg_*` 与 `ms_*` 并入 `cx.mem`。这两族的所有权合同已是规格条款(CX-O-1 至 CX-O-5,CX-A-2 称 grow 即 `Vec<u8>`、slice 即 `&[u8]`),满足 D8 的「别人要在其上搭建且需求不再变」。从 P5.8 的事实判据。
- (3) 函数名前缀按领域(`sy_`、`tx_`、`mm_`、`mg_`、`ms_`、`pt_`、`xs_`),不按包。迁包只改归属,不改函数名与签名(表头已写明)。因此拒 N6(9) 的「升入核心时换名并留别名」:换的只有模块路径,旧 `use` 路径留别名。

## 3 sys、fd、io

| 决定 | 内容 | 依据 |
|---|---|---|
| fd 是整数 | `cx.sys` 高度的 fd 保持 `int`,不包成对象 | 从 Q2.2、P1.3 的低层直通;拒 R3.2 的 `OwnedFd`,原因是 cx 没有编译期所有权,包一层并不带来静态保证 |
| 默认不继承 | 核心创建的每个 fd 默认 close-on-exec。继承只经 `sy_spawn_t` 的三个标准流与标志位 | 从 P6(7)(PEP 446,P 文标 `[忆]`) |
| EINTR 在 L0 重试 | 上层永不见 `-EINTR`,例外是 `sy_poll`、`sy_readable`、`sy_sleep_*`:它们按 deadline 重新计算剩余时间后重试 | 从 P6(7)(PEP 475,P 文标 `[忆]`) |
| 短读短写分名 | `sy_read_once` 一次系统调用;`sy_read_full`、`sy_write_all` 负责读满写完;流结束是返回 0,不是错误 | 从 R2.7 的 read 与 write_all 约定 |
| 偏移与大小用 64 位 | 文件偏移、文件大小、时间值、计数一律 `int64_t`。内存长度保持 `long`,在 win(LLP64)上限 2 GiB,超限返回 `-ERANGE` | 本仓事实:表已为此补了 `sy_seek64`、`sy_file_size`、`xs_receipt64`;win 上 `long` 为 32 位 |
| 轮询是唯一多路入口 | `sy_poll` 加 deadline 即全部「事件」设施,不单立事件循环模块 | 从 Q6(9);拒 N 的 libuv 循环 |
| 信号只观察不回调 | 可移植面是 `sy_sig_watch` 加 `sy_sig_taken`(置位后由调用者查询)。`sy_sig_handle` 这类回调式处置属 POSIX 能力,win 拒绝 | 从 Q2.2 的循环内分发与 P2.5 的「处理函数只在字节码间隙执行」;拒 N 的事件式信号 |
| 输出格式化 | `sy_printf` 族留在 `cx.sys`,失败合同改为 `-errno` | 第 11 节 |

win 上现为「本版未实现」的 sys 项(`sy_poll`、`sy_pollfd_t`、`sy_readable`、`sy_fd_nonblock`、`sy_fd_cloexec`、`sy_pread`、`sy_pwrite`、`sy_tty_size`)必须实现后才可声明稳定,因为 deadline 设计(第 8 节)依赖就绪等待。win 上对管道与套接字的就绪等待做法 `[未核]`。

## 4 fs

| 决定 | 内容 | 依据 |
|---|---|---|
| 整操作便利函数是首选形状 | `sy_read_file`、`sy_write_file`、`sy_append_file`、`sy_write_atomic`、`sy_mkdir_all`、`sy_remove_all`、`sy_copy` 不跨调用持有资源 | 从 R2.10(便利函数建在类型之上)、Q2.1 的 `loadFile`/`writeFile` |
| 存在性判定 | 只把 `ENOENT` 当不存在;`ENOTDIR`、`EACCES` 必须报告 | 本仓规则;P6(6) 同向 |
| 递归删除的安全线 | `sy_remove_all` 不跟随符号链接,拒绝空串、`/` 与 `.` | 表内既有合同,保留 |
| 目录遍历 | 流式 `sy_dir_open`/`sy_dir_next`/`sy_dir_close`,调用者拥有 `sy_dir_t`。`sy_dir_names`(一次写满缓冲)标 legacy | 从 R2.1 的 `ReadDir`、P2.2 的 `scandir`;Q 的一次性 `readdir` 在大目录下无界 |
| 元数据 | 一个 `sy_stat_t`,字段 `size`、`mtime`、`ctime` 改 `int64_t`。`sy_reg_size`、`sy_set_mtime`、`sy_lseek` 标 legacy,分别由 `sy_file_size`、`sy_utimes`、`sy_seek64` 取代 | 第 3 节 64 位规则 |
| 文件锁 | 整文件建议锁 `sy_lockf`/`sy_unlockf` 是可移植面;`sy_flock` 标 legacy | 一个能力一种形态(D3);win 的整文件锁做法 `[未核]` |
| 路径编码 | 路径是字节串,约定 UTF-8,六目标一致。win 的 L0 负责与系统原生编码互转 | 从 P4(PEP 529,P 文标 `[忆]`);拒 R4(5) 的 `OsString` 双类型,原因是 cx 没有类型层来承载 |
| POSIX 专属项 | `*at` 族五项标 internal,不进承诺面;`sy_mkfifo`、`sy_symlink`、`sy_readlink` 留在面内,win 按能力拒绝 | 第 12 节能力词表 |
| 不做 | 监视(watch)、ACL、扩展属性、挂载、配额 | 第 13 节 |

路径编码一条有已知缺口:表内 win 单元格写的是 `GetFileAttributesExA`、`SetCurrentDirectoryA`、`MoveFileExA`、`CreateProcessA` 等 A 结尾入口,它们按系统代码页而不是 UTF-8 解释字节。本条据单元格文字推断,未读 win 实现源码,`[未核]`。声明稳定前须有 win 上非 ASCII 路径的实跑用例。

## 5 path

- (1) `cx.path` 是纯词法函数,不碰文件系统。碰文件系统的(`sy_realpath`、`sy_abspath`、`sy_exists`)在 `cx.os`。从 R2.2、P2.3 的 PurePath 与 Path 分族。
- (2) 双实现:无后缀名按目标平台风格,`*_as(style, ...)` 在任一宿主上给出 posix 或 win 的确定结果。从 N6(6)、P4。表内 26 行已全部 exists 且三平台 impl,是六个核心包里最接近稳定判据的一个。
- (3) 输出写调用者缓冲,返回长度,装不下返回 `-ERANGE` 且不截断。拒 R 的 `PathBuf` 拥有型返回,原因是核心纯计算层不分配。
- (4) 接口说明须写明:字面判断(`pt_is_abs`、`pt_has_prefix`)不构成路径穿越防护。从 N4。
- (5) 不做 glob、通配、`~` 展开、URL 与路径互转。

## 6 process

| 决定 | 内容 | 依据 |
|---|---|---|
| 唯一派生入口 | `sy_spawn_t` 选项结构加 `sy_proc_spawn`/`sy_proc_wait`/`sy_proc_try_wait`/`sy_proc_kill`/`sy_proc_close`,以及一步到位的 `sy_proc_run` | 从 N2 的 options 对象与 Q2.2 的 `exec(args, options)`;拒 R2.3 的 `Command` 构造器,原因是现结构已在三平台实现且 C 没有方法链 |
| 选项结构一次定全 | 字段:`argv`、`env`、`cwd`、`in_fd`、`out_fd`、`err_fd`、`flags`、`pgroup`、`deadline`。`deadline` 类型由 `long deadline_ms` 改为 `sy_deadline_t`;`sy_child_t` 同改 | 从 Q6(8);第 8 节 |
| 结构的扩展规则 | 调用者必须先 `sy_spawn_init`,不得用位置初始化。新的布尔选项只加 `SY_SPAWN_*` 标志常量(兼容)。稳定后需要新的非布尔选项时,新增一对「新结构加新函数」,旧的转 legacy,不改旧结构 | 从 R5.6:给全公开结构加字段算破坏;layer-api 以签名文本判类,改 typedef 即 X |
| 只收 argv | 不经 shell 拼命令行。`flags` 可选「按 PATH 查找」 | 表头 `# out popen-with-shell`;拒 N 的 `shell` 选项、Q 的 `popen` |
| 超时合同 | 到期先杀(有 `pgroup` 杀整组)再收尸,`sy_exit_t.timed_out` 置 1。超时不是错误返回,是退出状态的一个字段 | 表内既有合同;拒 P 的 `TimeoutExpired` 异常式 |
| 权限类选项 | `uid`、`gid`、`groups` 不进核心 | 从 Q6(8) |
| 旧入口 | `sy_run`、`sy_spawn`、`sy_spawn_inherit`、`sy_spawn_pipe`、`sy_wait_status`、`sy_wait`、`sy_kill` 标 legacy,由 `sy_proc_*` 取代;`sy_signal(pid, sig)` 保留,用于非子进程 | 从 P6(10) 软弃用作为合法终态 |
| 不进承诺面 | `sy_fork_session`、`rl_tmpfile_exec`、`rl_run_link` 标 internal | fork 回调在 win 无语义;后两项是产品运行装载的内部帮手。它们在产品内的调用点数 `[未核]` |
| 输出采集 | `np_exec`(两路采集加容量上限)留在 `ext.procx` 孵化,判据满足后升 `cx.os` | D7 |

## 7 os 与 env

- (1) 平台识别只有三个入口:`sy_os_name`、`sy_arch_name`、`tn_self`。能力判断用 `sy_cap`(第 12 节),不比较平台字符串。从 Q6(7);拒 Q4 的 `os.platform` 字符串比较与 P4 的 `hasattr`。
- (2) `sy_env` 返回借用指针,任何 `sy_setenv`/`sy_unsetenv` 之后失效。改环境只在单线程启动阶段做;给子进程传环境走 `sy_spawn_t.env`。从 R2.4(`set_var` 改为 unsafe,R 文标 `[忆]`)的方向。
- (3) 目录类取值(`sy_tmpdir`、`sy_homedir`、`sy_self_path`、`sy_cwd`、`sy_hostname`)写调用者缓冲或返回静态串,未设返回 `-ENOENT`,不猜默认值。`sy_tmpdir` 在 win 现有「都没有时为 `.`」的回退,保留并在行内注明。
- (4) `sy_uid`、`sy_euid` 在 win 恒 0,是有文档的差异,不是拒绝。
- (5) 不做用户与组数据库、主机网络接口枚举、负载与内存统计。N2 的 `os.cpus`、`networkInterfaces`、`loadavg` 一类不进核心。

## 8 time 与 deadline

| 决定 | 内容 | 依据 |
|---|---|---|
| 两种钟分开 | 单调钟 `sy_mono_ns`(`int64_t` 纳秒)只用于间隔与 deadline;墙钟 `sy_wall`(`sy_ts_t`,`sec` 改 `int64_t`)只用于时间戳 | 从 R2.6 的 `Instant` 与 `SystemTime`、P4(PEP 418、564,P 文标 `[忆]`) |
| 一个 deadline 类型 | `sy_deadline_t { int64_t at_ns; }`,绝对单调时刻。构造只有 `sy_deadline_in(ms)` 与 `sy_deadline_never()`,组合用 `sy_deadline_min` | 从 Q6(5);绝对时刻使一串调用共用一个界,避免每次调用重新起算 |
| 谁带 deadline | 内部会循环或等待的组合操作按值收 `sy_deadline_t`。叶子级一次等待(`sy_poll`、`sy_readable`、`sy_sleep_ms`)保留相对毫秒 | 一处定义、处处同型 |
| 到期的返回 | 统一 `-ETIMEDOUT`。进程等待例外,见第 6 节 | 第 11 节 |
| 无 deadline 的阻塞例外 | 名字解析 `sy_resolve`(宿主解析器无法定界)与普通文件读写。例外清单封闭,只此两类 | 从 R2.5(`ToSocketAddrs` 含阻塞式解析) |
| 日历只做 UTC | `sy_utc_split`/`sy_utc_join`/`sy_utc_fmt`/`sy_utc_parse` 是纯计算。本地时区、时区库、locale 格式不做;`sy_local_fmt` 标 legacy | 拒 R2.6 的「完全没有日历」(构建工具需要写时间戳);拒 P 的 `datetime`/`zoneinfo` 进核心 |
| 旧取时入口 | `sy_now_ms`、`sy_wall_s` 返回 `long`,在 win 上会溢出,标 legacy | 第 3 节 64 位规则 |
| 不做 | 定时器对象、间隔定时器、取消令牌 | 拒 N3.5 的 `AbortSignal`;deadline 即取消的唯一形式 |

## 9 net

Q 没有套接字(Q 文已实查两仓),所以本区从 R 与 N 推导。

| 决定 | 内容 | 依据 |
|---|---|---|
| 面的范围 | 地址类型、名字解析、阻塞 TCP(监听、接受、连接、收发、半关)、UDP、Unix 域套接字、少量套接字选项 | 从 R2.5 的有意小面 |
| 地址是值类型 | `sy_addr_t` 一个结构覆盖 IPv4、IPv6、Unix 路径;解析与格式化是纯计算 | 从 R2.5 的 `SocketAddr` |
| IPC 并在 net 下 | Unix 域套接字与 TCP 同一组收发函数 | 从 N2(net 覆盖 TCP 与 IPC);拒 N 在 win 上换成命名管道的做法,只用 AF_UNIX,系统不支持时拒绝 |
| 套接字是 fd | 与文件 fd 同一整数空间,可交给 `sy_poll` 与 `cx.stream` | 从 Q 的 fd 模型;win 上经句柄表映射(表内已注) |
| deadline | 连接、接受、UDP 接收按值收 `sy_deadline_t`;流式收发新增带 deadline 的 `sy_recv_until`、`sy_send_all_until`;`sy_tcp_dial` 增 deadline 参数 | 第 8 节;拒 R2.5 的 `set_read_timeout`(状态挂在套接字上、每次调用重新起算) |
| 解析错误 | `SY_EAI_*` 并入统一负码空间的独立号段,归入 NAME 类 | 从 N3.3 的 `ENOTFOUND`(Node 自造码并明说非 POSIX) |
| 选项 | `sy_sockopt_set`/`get` 只认封闭枚举,枚举外返回 `-ENOPROTOOPT` | 封闭枚举可增长,新增常量是兼容变更 |
| 归属 | 31 行留在 `ext.net` 孵化,目标包 `cx.net`。核心六包的稳定声明不等 net | D7;表内 30 行为 new,实现在 layer-api 的排除单元 `ext/sysx/net.cx`(libc-forward),离判据最远 |
| 测试用便捷项 | 按端口号操作回环的 `sy_tcp_listen`、`sy_tcp_connect`、`sy_tcp_accept`、`sy_wait_port`、`sy_resolve4` 标 legacy,不随迁 `cx.net` | 由 `*_at`、`*_to`、`*_from` 取代 |
| 不做 | TLS、HTTP、URL、DNS 记录查询、原始套接字、组播、非阻塞连接状态机、事件驱动服务器 | 第 13 节 |

`csih` 车道依赖 `sys.net` 别名(prd_cx.md B4 已记)。本区任何签名变更先发信封给该车道,不直接改其文件。

## 10 stream、text、buffer、mem

**stream**

- (1) 核心流就是两种句柄:reader 与 writer,来源是 fd 或内存;外加 `xs_copy` 与 `xs_pipe_pair`。从 R2.7 的 `Read`/`Write`/`BufRead` 加 `io::copy`、P2.4 的 raw 与 buffered 分层。
- (2) 拒 N 的事件式流与背压(`highWaterMark`、`drain`)。阻塞式写本身就是背压。
- (3) 每个读写入口带上限:`xs_read_line(r, line, max)`、`xs_copy(r, w, limit, copied)`、`xs_writer_buf(w, dst, limit)`,超限返回 `-EMSGSIZE`。无界读取不提供。
- (4) deadline 挂在句柄上:新增 `xs_reader_deadline`、`xs_writer_deadline`,设绝对时刻,到期返回 `-ETIMEDOUT`。这是「每次调用带 deadline」的唯一例外,原因是流的读写入口多,且绝对时刻不存在重新起算的问题。
- (5) 关闭不吞错:`xs_writer_close` 先刷出再关,刷出或关闭失败都返回负码。拒 R3.2 的析构吞错。
- (6) 带回执的有界管线(`xs_open`/`xs_add`/`xs_tap`/`xs_run`/`xs_receipt*`)留在 `cx.stream`,它是编译器包的既有依赖。其失败合同由「非 0」改为负码。`xs_receipt`(32 位计数)标 legacy,由 `xs_receipt64` 取代。

**text**

- (1) 文本是字节加长度 `(ptr, len)`。带零结尾的入口只留给字面量比较。从 CX-A-3;R 的 `str` 与 `CStr` 之分。
- (2) 编码只认 UTF-8:校验、逐码点、写码点、计数。不做 locale、宽字符、大小写折叠表(`tx_lower`/`tx_upper`/`tx_ieq` 只处理 ASCII)、规范化。拒 P2.7 的 `codecs` 注册机制。
- (3) 可失败转换不做静默替换:`tx_to_i64` 对空串、非数字、溢出分别返回 `-EINVAL`、`-ERANGE`。从 R3.1 的严格版本;旧的 `tx_atol`、`tx_strtol`、`tx_strtoul`、`tx_strtod` 留在 `ext.textx` 标 legacy。
- (4) 输出一律写调用者缓冲,装不下返回 `-ERANGE` 且目标不动。`tx_copy`、`tx_field` 的「非 0」合同改为 `-ERANGE`。
- (5) `tx_snprintf`、`tx_vsnprintf` 并入 `cx.text`。原因:`sy_printf` 族(核心)依赖格式化,语义即 C99,需求不再变。字符串构造器 `sb_*` 留在 `ext.textx`(与 `mg_buf` 重叠,去留未定)。
- (6) 查找未命中返回 -1,不是错误。

**buffer 与 mem**

- (1) `mm_alloc` 族失败返回空指针,`mm_grow` 失败时原块仍有效。`mm_alloc_n` 检查乘法溢出。从 R 的 alloc 层。
- (2) 带界字节操作 `mm_copy`/`mm_move`/`mm_fill` 的越界合同由「非 0」改为 `-ERANGE`。
- (3) `mg_buf`(拥有型可增长缓冲)与 `ms_slice`(借用视图)并入 `cx.mem`,失败合同改为 `-ENOMEM`、`-ERANGE`。
- (4) 容器只有两个:带元素宽度的 `mm_vec` 与字符串键的 `mm_map`。不做泛型容器族。表头 `# out generic-collections`。拒 R2.9、N、P 的集合全家。
- (5) arena(`ar_*`)留在 `ext.memx`。

## 11 错误模型

**选定:返回值负码,一个整数。不做带载荷的错误对象。**

| 候选 | 取舍 | 原因 |
|---|---|---|
| 负的 errno 风格码(选定) | 采用 | (1) 规格 CX-Y-4 已定「跨模块错误只走返回值,L0 失败返回 `-errno`」,表内 102 行已是此合同;(2) 不分配,失败路径本身不会再失败;(3) 无全局状态,不依赖 `errno`;(4) 六目标可在 L0 归一 |
| 带路径、系统调用名、原始码的错误对象 | 不采用 | 从 N3.3 的 `SystemError` 与 R3.1 的 `io::Error` 看,载荷需要分配与所有权;cx 无异常无析构,每个调用点都得释放错误对象。上下文(哪个路径)调用者自己有,由调用者拼进诊断 |
| 三种约定并存 | 不采用 | Q3 是反例 |

配套规则:

- (1) 码表唯一。L0 的 `_os` 帮手把宿主错误(含 win 系统错误号)归一到一套 `E*` 名字与负数值,上层只见这一套。从 N6(5)、P3.1。名字与数值互查用 `sy_errno_name`、`sy_errno_of`,人读文本用 `sy_strerror`。
- (2) 分类谓词。新增 `sy_err_kind(int e)`,把码归到按「调用者如何处理」划分的类:NOTFOUND、EXISTS、PERM、NOTDIR、ISDIR、NOTEMPTY、AGAIN、PIPE(对端断开)、REFUSED、TIMEOUT、INVAL、BADH(坏句柄)、RANGE(容量不足)、NOMEM(资源耗尽)、UNSUPPORTED、NAME(名字解析)、OTHER。从 R6(6) 的 `ErrorKind`、P6(6) 的 PEP 3151。
- (3) 类清单可增长,调用者必须有兜底分支;新增一个类或一个码是兼容变更。从 R3.1 的 `non_exhaustive`、N3.4。
- (4) 码是合同,消息不是。测试不得断言 `sy_strerror` 的全文。从 N6(4)。
- (5) 每行的 `errors` 列用封闭文法写(stability.md S3),不再出现「非 0 表失败」这类自由文本。
- (6) 「不支持」只有一个码:`-ENOSYS`。
- (7) 非错误的返回不占负码:流结束是 0,查找未命中是 -1(仅限返回下标的纯计算函数),谓词是 0/1,超时的进程等待见第 6 节。
- (8) 失败不留半成品:不留半开 fd(CX-Y-4),`sy_write_atomic` 失败时目标不变,输出缓冲在 `-ERANGE` 时不动。
- (9) 空指针、空路径、负长度返回 `-EINVAL`(CX-Y-4)。

## 12 资源所有权与平台抽象

**资源**

| 资源 | 表示 | 谁关 | 关闭后使用 | 性质 |
|---|---|---|---|---|
| 裸 fd | `int` | 打开者调 `sy_close` | 不可检测(系统会复用号) | 未声称任何保护 |
| reader、writer、map | 带世代的整数句柄 | 创建者调对应 `*_close`/`*_free` | 确定返回 `-EBADF` | 运行期检查 |
| 子进程 | 调用者拥有的 `sy_child_t` | `sy_proc_wait` 收尸后 `sy_proc_close` | `-EBADF` 或 `-ESRCH` | 运行期检查 |
| 目录 | 调用者拥有的 `sy_dir_t` | `sy_dir_close` | 重复关闭 `-EBADF` | 运行期检查 |
| 堆块、`mg_buf` | 指针;调用者拥有的结构 | `mm_free`;`mg_free`(之后清零,再 free 是空操作) | 不可检测 | CX-O-1 至 CX-O-5 的调用方合同 |
| `ms_slice` | 借用视图 | 不关 | 所有者释放后不得使用,不强制 | 未声称 |

- (1) 每个返回或填充资源的行,在表的 `own` 列写明种类与配对的关闭函数(第 14 节 v2 列)。从 R6(9) 的文档纪律、P6(8)。
- (2) 句柄型资源重复关闭是确定的 `-EBADF`,不是幂等成功。拒 P3.3 的 `close()` 幂等,原因是重复关闭在 cx 里几乎总是所有权错误,应当暴露。
- (3) 泄漏可见:新增 `sy_res_live(kind)` 返回句柄型资源的存活数,检查在收尾处断言为 0。从 P3.3 的 `ResourceWarning` 思路,改为可断言的计数。
- (4) 首选形状是不跨调用持有资源的整操作函数(第 4、6 节)。不做「打开、回调、必关」的回调式作用域入口:拒 R6(9) 后半,原因是 C 没有闭包,回调加 `void *` 反而模糊所有权。
- (5) 如实声明:以上全部是运行期检查或调用方合同。没有编译期所有权与借用检查(CX-W-4 未落地),句柄可伪造可复制(CX-O-6),不声称内存安全与线程安全。

**平台**

- (1) 六目标:`lnx`、`osx`、`win` 乘 `x86_64`、`arm64`。事实表按操作系统三列记语义,架构差异只出现在 L0 的系统调用号表与个别常量(例 `sy_page_size` 在 osx arm64 为 16384),用 `impl:说明` 标注。架构维度的证据来自同候选六目标实跑(stability.md S7)。
- (2) 公开函数体内没有平台分支。每个公开函数调同名的 `NAME_os` 静态帮手,`#if` 与 `__syscallN` 只出现在帮手与表里(CX-Y-3)。从 R4(2) 的 `sys` 模块、P6(5)。
- (3) 平台单元格只有四种写法:`impl`、`impl:说明`、`refuse:原因`、`unverified`。本设计把拒绝再分两类:
  - 按能力拒绝:该平台没有对应语义。写成 `refuse:ENOSYS cap=NAME`,NAME 取自表头 `# cap` 封闭词表。
  - 未实现:写成 `refuse:ENOSYS 本版未实现` 一类。这类单元格阻塞稳定声明。
- (4) 能力词表(首批 12 个,其中 `flock` 只对应已标 legacy 的 `sy_flock`):`at`、`fifo`、`symlink`、`pgroup`、`termios`、`posix-signal`、`mmap-exec`、`dup2`、`socketpair`、`proc-tree`、`memfd-exec`、`flock`。运行期查询用新增的 `sy_cap(int cap)`,返回 0/1。从 Q6(7) 的能力表;与 R6(8) 的「平台专属另立包」相比,选能力拒绝而不另立 `ext.os.<目标>` 包,原因是另立包会让同一函数有两个身份,且现有 32 行已是同面拒绝。
- (5) 目标分级不引入。六目标同级,任一目标的面批失败都阻塞稳定声明。拒 R4(6)、N4、P4 的三级制,原因是 cx 的产品定义就是六片同候选。

## 13 核心之外(OUT)

表头已有 11 条 `# out`,本设计保留并增补 6 条。进核心先改表头。

| 范围 | 去向 |
|---|---|
| 线程、Worker、互斥、原子(已有) | 不做;并发靠进程与 `sy_poll` |
| 事件循环、EventEmitter、Promise 式定时器(已有) | 不做 |
| TLS、HTTP、URL(已有) | `ext.crypto` 既有单元,不扩面 |
| deflate 之外的压缩、sha 之外的密码(已有) | `ext.*` |
| 泛型容器族、locale 与浮点格式选项(已有) | 不做 |
| 经 shell 的 popen、`FILE*` 缓冲流、Windows ACL、ptrace/dlopen/ffi(已有) | 不做或仅构造工具可用 |
| 文件系统监视(新增 `fs-watch`) | 不做;N 的 `fs.watch` 各平台语义不一 |
| 本地时区、时区库、日历运算(新增 `calendar-tz`) | 不做;只有 UTC |
| 正则、glob、通配(新增 `regex-glob`) | `ext.*` |
| 用户与组数据库、主机统计(新增 `user-host-db`) | 不做 |
| 原始套接字、组播、DNS 记录查询(新增 `raw-net`) | 不做 |
| JSON、TOML、命令行参数解析等数据格式(新增 `data-formats`) | `ext.*`;TSV 已在 `ext.tsvx` |

## 14 对 build/std-surface.tsv 的行编辑清单

本文不改表。以下由集成者按「事实表 -> 生成器或迁移工具(带 `--check`)-> 表」落地,并同步 `tools/check-std-surface.cx`。行编辑合计 129 行:改 122 行,增 7 行。各组行数相加为 134,扣除重叠 12 行(E8 与 E6、E7 重叠 11 行,`xs_receipt` 同在 E3 与 E6)得 122。

**表级变更(不计入行数)**

- 首行改 `# std-surface v2`,每行追加四列:`stab`、`since`、`own`、`test`。
  - `stab`:`incubating`(ext 包默认)、`pre`(cx 包默认)、`stable`、`legacy`、`internal`、`deprecated:CXDEPnnnn`。
  - `since`:该行进入所属包时的包版本。
  - `own`:`none` 或 `open:KIND close:FN` 或 `borrow`。
  - `test`:登记在 `tests/batches.tsv` 的批次名。
- 表头增 `# cap NAME 说明` 12 行、`# out` 6 行(第 13 节)、`# removed` 墓碑段(初始为空)。
- `errors` 列按封闭文法机械归一(例:`-1 未找到` 写成 `miss:-1`,`0 表失败` 写成 `null`,`不返回` 写成 `noreturn`)。这是拼写归一,不改合同,由迁移工具一次完成,不逐行列出。

**E1 迁包 `ext.memx` -> `cx.mem`,同时把失败合同改为负码(32 行)**

- `mg_buf`、`mg_init`、`mg_reserve`、`mg_push`、`mg_append`、`mg_free`、`mg_move`、`mg_clear`、`mg_pop`、`mg_truncate`、`mg_insert`、`mg_remove`、`mg_take`(13)。
- `ms_slice`、`ms_slice_mut`、`ms_none`、`ms_is_none`、`ms_span`、`ms_of`、`ms_len`、`ms_at`、`ms_eq`、`ms_find`、`ms_rfind`、`ms_contains`、`ms_starts_with`、`ms_ends_with`、`ms_mut_of`、`ms_mut_len`、`ms_mut_at`、`ms_mut_fill`、`ms_mut_copy`(19)。
- 其中 14 行的 `errors` 由「非 0」改为负码:分配失败 `-ENOMEM`,越界或长度不符 `-ERANGE`,空缓冲 `mg_pop` 为 `-ENOENT`。

**E2 迁包 `ext.textx` -> `cx.text`(2 行)**:`tx_vsnprintf`、`tx_snprintf`。

**E3 核心内失败合同改为负码(20 行)**

- `cx.stream`(10):`xs_item_set`、`xs_open`、`xs_add`、`xs_tap`、`xs_run`、`xs_receipt_count`、`xs_receipt`、`xs_receipt64`、`xs_receipt_id`、`xs_close`。坏句柄 `-EBADF`,句柄用尽 `-EMFILE`,形状不符 `-EINVAL`,`xs_run` 透传步函数的负码并仍写 `why`。
- `cx.mem`(3):`mm_copy`、`mm_move`、`mm_fill` 改 `-ERANGE`。
- `cx.sys`(5):`sy_printf`、`sy_eprintf`、`sy_vprintf`、`sy_veprintf`、`sy_puts` 改 `-errno`。
- `cx.text`(2):`tx_copy`、`tx_field` 改 `-ERANGE`。

**E4 签名改 64 位(4 行)**

- `sy_stat_t`:`size`、`mtime`、`ctime` 改 `int64_t`。
- `sy_truncate(int fd, int64_t size)`。
- `sy_utimes`:四个时间参数的秒改 `int64_t`。
- `sy_ts_t`:`sec` 改 `int64_t`。

**E5 签名改用 `sy_deadline_t`(7 行)**

- `sy_spawn_t`:`long deadline_ms` 改 `sy_deadline_t deadline`。
- `sy_child_t`:`long deadline_at_ms` 改 `sy_deadline_t deadline`。
- `sy_tcp_accept_from`、`sy_tcp_connect_to`、`sy_udp_recv_from`、`sy_unix_connect`:末尾 `long ms` 改 `sy_deadline_t d`。
- `sy_tcp_dial`:增第三参数 `sy_deadline_t d`。

**E6 `stab=legacy`(25 行)**

- fs、time、stream(9):`sy_reg_size`、`sy_lseek`、`sy_set_mtime`、`sy_now_ms`、`sy_wall_s`、`xs_receipt`、`sy_dir_names`、`sy_flock`、`sy_local_fmt`。
- process(7):`sy_run`、`sy_spawn`、`sy_spawn_inherit`、`sy_spawn_pipe`、`sy_wait_status`、`sy_wait`、`sy_kill`。
- net(5):`sy_tcp_listen`、`sy_tcp_connect`、`sy_tcp_accept`、`sy_wait_port`、`sy_resolve4`。
- textx(4):`tx_atol`、`tx_strtol`、`tx_strtoul`、`tx_strtod`。

**E7 `stab=internal`(12 行)**:`rl_tmpfile_exec`、`rl_run_link`、`sy_fork_session`、`sy_map_none`、`sy_unmap`、`sy_protect_rw`、`sy_protect_rx`、`sy_at_fdcwd`、`sy_openat`、`sy_mkdirat`、`sy_unlinkat`、`sy_linkat`。

**E8 拒绝单元格改写为 `refuse:ENOSYS cap=NAME`(32 行,其中 11 行与 E6、E7 重叠)**

| cap | 行 |
|---|---|
| `at` | `sy_openat`、`sy_at_fdcwd`、`sy_mkdirat`、`sy_unlinkat`、`sy_linkat`(win) |
| `fifo` | `sy_mkfifo`(win;osx 一格是未实现,不属本类) |
| `symlink` | `sy_symlink`、`sy_readlink`(win) |
| `pgroup` | `sy_setpgid`、`sy_getpgid`、`sy_setsid`、`sy_killpg`、`sy_tcgetpgrp`、`sy_tcsetpgrp`(win) |
| `termios` | `sy_tty_t`、`sy_tty_get`、`sy_tty_set`、`sy_tty_raw`(win) |
| `posix-signal` | `sy_sig_handle`(osx、win)、`sy_sig_default`、`sy_sig_ignore`、`sy_raise`(win) |
| `mmap-exec` | `sy_map_none`、`sy_unmap`、`sy_protect_rw`、`sy_protect_rx`(win) |
| `dup2` | `sy_dup2`(win) |
| `socketpair` | `sy_socketpair`(win) |
| `proc-tree` | `sy_kill_tree`、`rp_kill_tree`(osx、win) |
| `memfd-exec` | `rl_tmpfile_exec`(lnx) |
| `flock` | `sy_flock`(win) |

**E9 新增(7 行)**

| pkg | 签名 | errors | 说明 |
|---|---|---|---|
| `cx.sys` | `int sy_err_kind(int e);` | `none` | 返回 `SY_K_*` 类,未知码归 OTHER |
| `cx.sys` | `int sy_cap(int cap);` | `0/1` | 能力查询,未知能力为 0 |
| `cx.sys` | `long sy_res_live(int kind);` | `-EINVAL` | 句柄型资源存活数 |
| `cx.stream` | `int xs_reader_deadline(int r, sy_deadline_t d);` | `-EBADF` | 之后的读到期返回 `-ETIMEDOUT` |
| `cx.stream` | `int xs_writer_deadline(int w, sy_deadline_t d);` | `-EBADF` | 同上 |
| `ext.net` | `long sy_recv_until(int fd, void *buf, long n, sy_deadline_t d);` | `-errno;-ETIMEDOUT` | 0 表对端关闭 |
| `ext.net` | `int sy_send_all_until(int fd, const void *buf, long n, sy_deadline_t d);` | `-errno;-ETIMEDOUT` | 写完或失败 |

这 7 行是面封口前的一次性增补。面封口(本清单落表并升 v2 的那个提交)之后,D7 严格生效:新函数只能先进 `ext.*`。

**不属行编辑、但阻塞稳定声明的单元格(由实现与实跑改写)**

- `unverified` 71 格(osx 3、win 68):逐格实跑后改为 `impl` 或拒绝。
- 未实现的拒绝,核心 21 行:win 上 `sy_access`、`sy_link`、`sy_remove_all`、`sy_truncate`、`sy_fdatasync`、`sy_dir_t`、`sy_dir_open`、`sy_dir_next`、`sy_dir_close`、`sy_utimes`、`sy_pread`、`sy_pwrite`、`sy_fd_nonblock`、`sy_fd_cloexec`、`sy_pollfd_t`、`sy_poll`、`sy_readable`、`sy_tty_size`、`sy_lockf`、`sy_unlockf`;osx 上 `sy_mkfifo`。扩展 1 行:win 上 `sy_tcp_dial`。
- `status=new` 108 行:实现并由 layer-api 导出后改 `exists`。

## 15 未核实清单

- (1) 本轮未运行任何工具:`check-std-surface`、`layer-api --check`、`check-tlib-frozen`、棘轮的当前结果均未实跑。「现有导出名里没有表行的有多少」未知。
- (2) win 的实现细节全部未读源码:A 结尾入口的编码行为、整文件锁、句柄继承标志、管道与套接字的就绪等待、AF_UNIX 的系统版本门槛(表内写 Win10 1803,来源未核)。
- (3) `sy_fork_session`、`rl_*`、`*at` 族、映射四项在产品与工具里的调用点数未查,标 internal 的依据只是它们的平台形状。落表前用 `tools/refactor.cx sites` 核对;若有 `ext` 之外的广泛调用者,改标 legacy。
- (4) 可移植的打开标志常量(`SY_O_*`)是否已存在未核;`build/layer-api/cx.base.tsv` 里只查到 `SY_SPAWN_STDERR_SPLIT`、`SY_STDIO_INHERIT` 两个 `SY_` 枚举名。若不存在,`sy_open` 的 `flags` 须先定一组封闭常量。
- (5) `sy_deadline_t` 按值传参与返回在六目标 ABI 上的一致性:表内 `sy_deadline_in`、`sy_deadline_min` 已按值返回且三平台记 impl,按值入参未单独核。
- (6) E1 的迁包对 `cx/mem`、`cx/text` 与 `ext/memx`、`ext/textx` 之间互指符号链接(prd_cx.md 第 3 节已记)的影响未核。
- (7) 四份参照文档里标 `[忆]` 的条目,本文引用处仍是未核:PEP 446、475、418、564、529;Rust 的 `set_var` 改 unsafe、析构吞错、`OwnedFd`;Node 的 `AbortSignal` 与资源模型。
- (8) 第 9 节「绝对时刻优于每次调用超时」是设计判断,没有在本仓做对照实验。
