# 核心层接口面与扩展应用层需求矩阵(2026-11-20,只读取证,未改仓,未提交)

基线:main 624f6f8cde,已装产品 0.0.85。统计脚本(.cx):/home/devali/.claude/jobs/2798e440/tmp/cem/usemat.cx(use 与裸 libc 记号扫描,跳注释与字符串,不跟符号链接)、gaps.cx(pkg-gaps 分类)。原始输出:同目录 usemat.tsv、gaps-sum.txt、gaps-open.txt。前序报告(layer-lanes、core-layer-marking、asm-acceleration)的数直接引用,不重做。

## 0 最重要的判断

核心层对 cx 使用者的供给基本够用,卡住人的不是"缺接口",是"有接口没人用"和"接口登记不全":sandbox-cx 11774 行、去打印后裸 libc 只有 3 处,证明现有 L0 足以写大型系统程序;而 tools/cxlib 19156 行里裸 libc 4369 处(每千行 228),125 个工具文件手工按制表符拆 TSV,用 tsvx 的只有 6 个文件。与此相对,0.0.87 到 0.0.96 十版几乎全给了外部 C 程序的需求(汇编 A1 到 A4、pthread),内部使用者要的十来个 L0 小件(各 0.5 单位量级)在版本序列里没有槽位。我的立场:汇编列车不撤,但每版核心列车固定切 1 到 1.5 单位给"L0 小件加模块解析",并把前端硬上限(SQLite、Lua 的真实卡点)插到 A2 之前。

## 1 取证原始数

### 1.1 使用矩阵原始数(usemat.tsv 摘录)

| 使用者 | 文件 | 行 | dock | use sys | use mem | use text | use sysx | use net | use crypto | use tsvx | use cxlib | 包内或自造模块 use | 裸 libc(去打印) | 每千行 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| ext/sysx | 15 | 2557 | 13 | 23 | 4 | 0 | 5 | 0 | 0 | 0 | 0 | 0 | 184 | 71 |
| ext/crypto | 15 | 3434 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 27 | 0 | 0 |
| ext/net | 6 | 739 | 6 | 18 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 6 | 0 | 0 |
| ext/tsvx(实体 3 个) | 3 | 477 | 3 | 2 | 4 | 4 | 0 | 0 | 0 | 1 | 0 | 0 | 26 | 54 |
| ext/testx | 1 | 158 | 0 | 3 | 0 | 1 | 1 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| ext/argx | 1 | 397 | 0 | 2 | 0 | 3 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| ext/fsx | 1 | 462 | 0 | 7 | 2 | 1 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| ext/memx | 1 | 63 | 1 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 2 | 31 |
| ext/textx | 4 | 920 | 4 | 0 | 5 | 1 | 0 | 0 | 0 | 0 | 0 | 0 | 7 | 7 |
| tools/cxlib | 86 | 19156 | 1 | 120 | 48 | 32 | 8 | 0 | 0 | 1 | 38 | 26 | 4369 | 228 |
| apps/csih | 141 | 36672 | 0 | 234 | 60 | 123 | 0 | 0 | 0 | 0 | 0 | 245 | 1281 | 34 |
| ujs | 170 | 18741 | 0 | 53 | 49 | 238 | 0 | 0 | 2 | 0 | 0 | 118 | 793 | 42 |
| devsvc(devsvc、devsvc-core、flow、tools/dv) | 7 | 6383 | 0 | 43 | 9 | 7 | 4 | 6 | 0 | 0 | 6 | 4 | 900 | 140 |
| facts-gen | 1 | 4343 | 0 | 0 | 1 | 1 | 0 | 0 | 0 | 0 | 0 | 0 | 36 | 8 |
| candidate-chain 加 chain-step | 2 | 5571 | 0 | 15 | 3 | 2 | 0 | 0 | 0 | 0 | 12 | 0 | 832 | 149 |
| affected | 1 | 2246 | 0 | 5 | 2 | 1 | 2 | 0 | 0 | 0 | 8 | 0 | 103 | 45 |
| release-exec、s9-nightly、precommit、commit-own | 4 | 6107 | 0 | 26 | 6 | 6 | 0 | 0 | 0 | 0 | 13 | 0 | 508 | 83 |
| sandbox-cx(仓外 moltbaby) | 8 | 11774 | 1 | 52 | 19 | 32 | 5 | 0 | 0 | 0 | 0 | 22 | 3 | 0 |

裸 libc 分类(去打印):tools/cxlib 字符串 2547、分配 989、fd 与文件系统 533、进程 247;devsvc 字符串 707、环境 77、分配 84;csih 字符串 590、fd 377、进程 154、stdio 111、套接字 15;ujs 字符串 402、stdio 243、分配 93。全部使用者线程类调用为 0,套接字裸调用只有 csih 15 与 ext/sysx 31。

注:csih 与 ujs 的归属是别的车道,本表只读统计,不建议 unisacc 侧去改。devsvc 的计数不含 tools/cxlib 下的 flow 与 rel 单元(它们计在 cxlib 行)。

### 1.2 ext 包的真实使用者数(tools、tests、apps 下 use 该包的文件数)

sysx 182(其中 sysx.process 126、sysx.poll 37、sysx.run 12、sysx.reaper 9)、crypto 16、net 13、fsx 8、testx 7、argx 7、os 7、tsvx 6、stream 3、path 1。对照:手工 strchr 制表符拆列的工具与测试文件 125 个。

### 1.3 已装产品 0.0.85 的三个实测(探针在 cem/p)

- `use text.text::tx_len as slen` 运行输出 len=5,rc=0。prd_cx_spec.md 3.2 写的"别名不可用"已过时。
- 模块里写 `#pragma unisa export pm_f`,被 use 后 rc=0。规范 3.5 写"被拒绝 unknown unisa pragma export"已过时。export 的名单语义是否真被强制未核(只测了接受)。
- 无 dock 模块写 `#include <stdio.h>`、`#if defined(__linux__)`、裸 strlen,被 use 后 rc=0,只多一行 conditional interface 提示。K2 尚未生效,与 core-layer-marking 的结论一致。

## 2 回答 (1):核心层接口面清单

| 类别 | 现有面(真源) | 覆盖度判断 | 已知缺口 |
|---|---|---|---|
| L0 宿主对接 sy_ | cx.sys 表 92 行、cx.os 表 138 行(含表头;build/layer-api) | 文件、目录、进程、环境、时间与截止、sha、fd、poll、信号观察、netraw 齐;够写 sandbox-cx | 通用 sy_syscall6(到期 0.0.86,仍 OPEN)、熵源 sy_random、mtime 纳秒、子进程 rusage、stderr 分离与带 cwd 带环境的 run、子进程等待集(pidfd 或 SIGCHLD)、超时路径接杀进程树、PDEATHSIG、Darwin sy_self_path、线程或工作者;std-surface 还有 11 行 status 为 new(定了没导出) |
| L0 内存 mm_ | cx.mem 表 63 行 | 堆、带界字节、grow、arena、vec、字符串键 map 齐 | 名字到整数的 idmap、mm_copy 目标容量易误用(约 90 处把拷贝长度当容量) |
| L0 文本 tx_ | cx.text 表 46 行 | 带界比较、查找、迭代、替换、数字、utf8、编码齐 | 行索引(偏移到行号)、无正则、printf 的 %.*s 对大缓冲极慢(运行库) |
| 邻接包 path、stream、os | 29、34 行;os 2 个文件 | 面在,几乎无人用(path 1 个使用者、stream 3 个) | 不是缺口,是没有被采用;stream 有已知双重释放风险未修 |
| L1 执行器与驱动 | cx.base 95、cx.runtime 368、cx.compile 528、cx.driver 49、cx.plan 16 行 | 内部环,应用不该 use;但 tools 与 tests 实际有 runtime 198 处、compiler 103 处、core 25 处 use | 无稳定的阶段化前端 API 与只检查不出码的语义查询入口;运行缓存不按内容取键;并行首次编译丢缓存 |
| 内置 libc 与 POSIX | include 下约 45 个头加 sys 子目录 20 个 | POSIX 17/29、asm 4/17(0.0.82 实测;0.0.83 修三项后未整体重测) | sys/epoll.h、sys/random.h 无文件;alarm、setitimer、sigsuspend、pause 缺;pthread 头在而链接不到;原子与 TLS 全拒;prd.md 写缺 regex.h 与 strings.h,但 include 目录里有 regex.h(未核实现程度) |
| cx 语言扩展 | module、use、use 符号级、as 别名、dock、export | 别名与 export 在 0.0.85 已被接受(规范未更新) | 门面重导出 D1b、通配 use、仓外脚本找不到包根、cxroot 无 net 前缀(靠 tools/net 链接)、单元 48 条 use 上限、dock 无强制(K2)、safe 与 own 流检查 9 条开口 |
| 外部 C 程序的前端 | C99 主体 | realprog 新体系 0 批次 | 结构体成员上限 64(SQLite)、定义前的不完整数组声明(Lua ltm.h)、诊断行号偏移、-funit 对象 ABI 5 条(SysV 传参、初值入 bss、weak、_start 重复、arm64 入口) |

登记缺口(本次新发现):ext/testx、ext/argx、ext/fsx 不在 build/tlib-packages.tsv 也没有 layer-api 表,无版本、无接口哈希;argx、fsx、net 不在仓根 cxroot.tsv,靠 tools 下的符号链接解析。这三包已有 7 到 8 个使用者文件,接口一改没有任何机器能列出影响面。

## 3 回答 (2):需求矩阵

格子写法:深度(重、中、轻、无)加缺口。列:甲 L0 sy 进程与文件、乙 L0 mm 与 tx、丙 网络与事件、丁 线程原子、戊 语言扩展与模块解析、己 内置 libc 与 POSIX 头、庚 编译速度与缓存。

| 使用者 | 甲 | 乙 | 丙 | 丁 | 戊 | 己 | 庚 |
|---|---|---|---|---|---|---|---|
| ext/sysx | 重;13 个 dock 自己对接宿主,裸调用 184;linuxns 因缺 sy_syscall6 私带 __syscall6 | 轻 | 中;poll 在此,31 处裸套接字 | 无 | 被迫 dock 13 个;process 与 netpack 同名符号要靠双门面(缺重导出) | 4 个文件 include | 无 |
| ext/crypto | 无 | 无(纯计算,自带包内 27 条 use) | 无 | 无 | 干净,零 dock 零 libc | 无;缺位运算内建与宽乘只能纯 C 写(推断,未测速度) | 冷编 15 单元 |
| ext/net | 中;经 sy 包装,裸调用 0 | 无 | 重;阻塞加截止,无 epoll、无非阻塞事件循环、DNS 仅字面量与 localhost | 无 | 6 个 dock 声明过宽(裸调用 0);不在仓根 cxroot | 缺 sys/epoll.h | 无 |
| ext/tsvx | 轻 | 中;26 处裸 libc | 无 | 无 | 3 个 dock 不必要(推断) | 无 | 无 |
| ext/testx、argx、fsx | 轻到中 | 轻 | 无 | 无 | 无版本无接口表;argx、fsx 无 cxroot 前缀 | 无 | 无 |
| ext/memx、textx | 无 | 重(实现体);json 缺序列化,deflate 缺 inflate | 无 | 无 | 实体放 ext、身份算核心(layer-lanes 的 BS 类) | 无 | 无 |
| tools/cxlib | 重;sys 120 条 use,同时 fd 类裸调用 533、进程 247 | 重但绕开;字符串裸调用 2547、分配 989 | 轻 | 无 | cxlib 内部互 use 38;netpack 双名 | 当 libc 用 | 重;84 单元冷建 41 秒,贴 40 秒界 |
| apps/csih | 重;sys.io 69、sys.env 48、sys.fs 40、sys.proc 29 | 自造:zstr 55 处、json 19 处、fio 12 处,与 tx_、text.json、sys.fs 重叠 | 中;sys.net 11,裸套接字 15 | 无 | 10 个头面探针 include(有白名单) | 中 | 未测 |
| ujs | 中;sys.fs 44 | text.text 145、text.fmt 77;自造 g_math 89 处、g_geometry 24 处 | 无 | 无 | 模块只有 5 个,多为脚本 | stdio 243 | 未测 |
| devsvc | 重;sys.proc、sys.fs、sys.env 各 7 | 字符串裸调用 707、getenv 类 77 | 中;net.unix、net.tcp、net.sock、sys.netraw | 想要(FLOWEXEC-THREADS OPEN),现用多进程 | 用 sys.ffi(特权接口) | 轻 | 重;冷起 30 秒、固定版本冷构造撞 60 秒窗 |
| facts-gen | 无 | 几乎不 use,裸调用 36 | 无 | 无 | 无 | 轻 | 单文件 4343 行冷编 |
| candidate-chain、chain-step | 中 | 每千行 149 处裸 libc | 无 | 无 | cxlib 12 | 轻 | 中 |
| affected | 中;sysx 2 | 轻 | 无 | 无 | cxlib 8 | 轻 | 重;kit 准备冷编超窗 |
| release-exec 等四件 | 中 | 每千行 83 | 无 | 无 | cxlib 13 | 轻 | 中 |
| sandbox-cx | 重;sys.fs 8、sys.env、sys.proc、sys.posix、sys.sig、sys.sysinfo;sysx.linuxns、exec、poll、net | 重且规范;裸 libc 3 | 轻;sys.netraw 1 | 无 | 被迫在仓外自声明 dock(cagesys.cx 第 97 行 __syscall6);包根靠 r 链接指到 .claude/worktrees/v0.0.86-l1 | 无 | 未测 |

矩阵读法(立场):

- (1) 丁列全空:没有一个内部使用者今天被线程、原子、TLS 卡住,唯一的愿望单是 devsvc 的进程内并行,工作者进程池加等待集就能满足。
- (2) 乙列的问题是采用,不是供给:tx_ 与 mm_ 齐全,sandbox-cx 做到了零 libc;cxlib 与大工具的 4000 多处是历史写法。K2 第三步(裸 libc 编译期拒绝)对 tool 档不适用的判断成立,不要为此造新核心接口。
- (3) 甲列的真缺口集中在进程一族(等待集、rusage、stderr 分离、杀树接超时、PDEATHSIG)与 sy_syscall6、熵源,每项都小,合计约 4 到 5 单位,被 devsvc、cxlib、sandbox-cx、sysx 四家同时需要。
- (4) 戊列是最廉价的高杠杆:包根与前缀(net、argx、fsx 进 cxroot;仓外脚本的搜索路径)只动表与驱动,却卡住每一个写仓外分析脚本的代理和 sandbox-cx。
- (5) 庚列卡住所有工具使用者,pkg-gaps 里约 12 条 OPEN 是冷编与缓存;执行器内核产品化(0.0.88)对内部使用者的价值高于任何一片汇编。

## 4 回答 (3):缺口归属

pkg-gaps.tsv 实测:213 个数据行,含 OPEN 的 169 行(其中 168 行为六列;另有 15 行只有 5 列、2 行只有 3 列,属表格式破损)。按 need 与 target 手工归类(误差约正负 5,未逐条回源核实):

| 归属 | 条数 | 内含 |
|---|---|---|
| 甲 核心层必须做 | 约 69 | 编译器前后端与对象 ABI 22;语言扩展与模块解析 15(其中 safe 与 own 流检查 11);L0 宿主对接 16;内置 libc 与 POSIX 运行库 4;驱动、运行缓存与环境变量语义 12 |
| 乙 扩展层自己做 | 约 17 | json 序列化、inflate、clex、排序、图(bitset、可达、拓扑)3 条、目录树快照与差异与 glob 与树哈希 3 条、procx 带界捕获、DNS、md 扫描、配方枚举、行索引、超时保留输出尾部 |
| 丙 应用与工具层自己做 | 约 72 | devsvc 门与作业 12 上下、dvqualify 与资格布局 12 上下、release-exec 与版本槽 10 上下、register 与批次声明 6、derive 与派生表 5、其余检查与速度台账 |
| 丁 文档、账本或环境问题 | 约 11 | 已交付仍挂 OPEN:sigprocmask、environ、wchar(prd.md 记 0.0.83 已修)、tsvx 行读取 4 到 5 条(tv_row_next 已在)、目录列举(sy_dir_ 已在)、非阻塞套接字(sy_snb_set_nonblock 已在);本机无 ffmpeg 1 条;另加规范里别名与 export 两处过时 |

结论:169 条里真正要核心层动手的约四成;最大的一类(约 72 条)是工具与流程自己的债,不该进核心层排期。

前 10 项最高杠杆缺口(跨归属):

- (1) 仓外脚本包根与 cxroot 前缀不全(甲,语言与驱动)。卡住:全部代理的分析脚本、sandbox-cx、tools/dv。造价约 0.5 到 1 单位。
- (2) 子进程等待集加杀树接超时加 PDEATHSIG(甲,L0)。卡住:devsvc flowexec、dvqualify 孤儿进程、sysx.process 的 126 个使用者文件间接受益。约 2 到 3 单位。
- (3) sy_syscall6 特权入口(甲,L0)。卡住:linuxns 与 sandbox-cx 的 dock;是 K2 的前置。约 0.5 单位。
- (4) 运行缓存按内容取键加并行首写不丢(甲,驱动)。卡住:所有工具冷编、资格布局、公开读回。约 2 单位。
- (5) 前端结构体成员上限 64 与不完整数组声明(甲,编译器)。卡住:SQLite、Lua,即 realprog 重新接回。约 3 单位(推断)。
- (6) 门面重导出 D1b(甲,语言)。卡住:sysx.process 与 cxlib.netpack 双门面、任何想收拢子模块的包。已被让位,约 2 单位。
- (7) tsvx 采用率(丁加丙):125 个手工拆列文件对 6 个使用者。不需要新接口,需要 cx-lint 提示与一次 refactor sweep。约 1 单位,零核心成本。
- (8) ext 三包(testx、argx、fsx)入 tlib-packages 与 layer-api(丁,登记)。0.5 单位,是影响评估流程的前提。
- (9) sys/epoll.h 加非阻塞事件循环(甲的头加乙的 ext/net)。卡住:https 端到端、devsvc 多连接。约 2 到 3 单位。
- (10) 熵源 sy_random 与 sys/random.h(甲)。卡住:crypto 的密钥生成、mkstemp 质量、TLS 握手。约 0.5 单位。

## 5 回答 (4):核心层 backlog 前 10 与版本对照

排序口径:被卡使用者数 x 解锁价值 / 造价(单位沿用路线图的单位;全部为推断估值)。

| 序 | 任务 | 被卡使用者 | 估单位 | 建议版本 | 现编排 |
|---|---|---|---|---|---|
| 1 | L0 小件包甲:sy_syscall6 特权入口、sy_random、mtime 纳秒、rusage 回填、stderr 分离与带 cwd 的 run | sandbox-cx、sysx、devsvc、cxlib、crypto | 2 | 0.0.87(2026-11-22 L0 冻结到期后) | 无槽位;sy_syscall6 原定 0.0.86 到期未做 |
| 2 | 模块解析:cxroot 补 net、argx、fsx;仓外搜索路径;三包入 tlib 与 layer-api | 全部脚本作者、sandbox-cx、tools/dv | 1 | 0.0.87 | 包根与门面被让位 |
| 3 | 执行器内核产品化(冷编与运行提速) | 全部使用者 | 3 到 4 | 0.0.88 | 一致,保留 |
| 4 | 进程等待集、杀树接超时、PDEATHSIG、工作者进程池 | devsvc、dvqualify、sysx.process 使用者 | 2 到 3 | 0.0.88 库侧加 L0 小件 | 无槽位 |
| 5 | 运行缓存内容键与并发首写 | 全部工具、资格布局 | 2 | 0.0.88 或 0.0.89 | 无槽位(0.0.86 W2 相邻) |
| 6 | K2 dock 强制 | 核心层边界本身 | 1.5 到 2 | 0.0.89 | 一致;但必须排在第 1 项之后 |
| 7 | epoll 头、sys/random.h、信号定时器(alarm、setitimer、sigsuspend、pause) | ext/net、https、外部 C 程序;POSIX 面 17/29 可到约 23/29 | 3 | 0.0.88 到 0.0.89 | epoll 在 v0.0.87.md 标"待定";信号定时器原排 0.0.86,在 version-seq 第 22 到 31 行与 v0.0.86、v0.0.87 计划里搜不到(未核是否在别处) |
| 8 | 前端硬上限(结构体成员 64、不完整数组)加 realprog 接回 | SQLite、Lua、外部用户 | 3 | 0.0.90,排在 A1b 之前或与之换位 | 原前端覆盖二期三期被汇编让位 |
| 9 | A1a 位运算内建与符号属性;A1b 基本 asm | 外部 C 程序;内部 0 处 asm | 3 加 2.5 | 0.0.87、0.0.90 或 0.0.91 | 一致 |
| 10 | A2 到 A4、A3 原子与 TLS、pthread | 外部 C 程序;内部无人被卡 | 约 20 | 0.0.91 到 0.0.96 | 一致;A2 应把 -funit 对象 ABI 5 条 OPEN 列为前置 |

冲突与调整建议:

- (1) K2 与 sy_syscall6 的次序冲突:K2 生效后 dock 只许出现在授权包根;sandbox-cx 的 cagesys.cx 在仓外自声明 dock,要么被拒,要么因为它自带 cxroot.tsv 而自我授权,等于留后门。必须先给 sy_syscall6(标特权接口,与 sys.dl、sys.ffi 同列),并在 K2 设计里写明仓外包根的层列缺省为 upper。
- (2) "一个候选只含一类大改"与 L0 小件:L0 在源身份内,任何 sy_ 增量都要整链。建议把 L0 小件定义为每版核心列车的固定附挂额度(不超过 1.5 单位、只增不改、layer-api 判为 Y 类),不算第二类大改;超出就单独成版。此口径需要主调度写成决定,否则 0.0.87 A1a 加 L0 小件会被判违规。
- (3) pthread 排 0.0.96 与 devsvc 的并行需求:不要等。工作者进程池加等待集满足内部需求,pthread 留给外部 C 程序。
- (4) 汇编十版对内部使用者的收益为零(2315 个 .cx 里 asm 记号 0 次,矩阵丁列全空)。所有者已定汇编为基本需求,我不主张撤,但主张 A4(带约束内联 asm,8 到 10 单位)在 A1 到 A3 对拍通过后重新过一次采用门,用 realprog 的真实卡点数据决定先做 A4 还是前端硬上限。
- (5) 0.0.83 修复后 46 探针未整体重测,POSIX 17/29 是旧数;每周 gcc 差分普查应立即成固定项,否则本表第 7 项的基数不可信。

## 6 回答 (5):团队编排建议

### 6.1 分组、容量与节拍

| 组 | 成员 | 每版容量(单位,推断) | 节拍 |
|---|---|---|---|
| 核心层组 | cdx(表、NN、前端与诊断、A 系列主刀)、agyneon(执行器、L0 小件、gcc 对拍) | 主题 3 到 4 加 L0 小件附挂 1 到 1.5 | 跟版本列车,一版一类大改;每版开槽前收需求单,冻结前一刻不加项 |
| 边界与工具组 | agy-flow(lane-inputs、包门、devsvc、影响面查询工具)、opus 子代理(夹具与独立复核)、cc(集成与验收,不写主题代码) | 工具线每版 2 项(现行规则)加登记与影响面 1 | main 上连续提交;每版公开后到下次起链前交付 |
| 扩展与应用层组 | agy-cage(sandbox-cx 与 ext/sysx 的需求方兼 sysx 实现)、ccapi 与 cdx2(ujs 车道)、fable 子代理(分析,不落库) | ext 包内实现每版 2 到 3 | 库快车道,不等整链;包门工具就位前仍按版本分支规则 |

边界:扩展组可以改 ext 纯包(crypto、net、tsvx、testx、argx、fsx 与 sysx 的非别名文件);ext 里被 cx/sys、cx/mem、cx/text 链接的 14 个文件按核心层对待,只能由核心层组改。csih 与 ujs 的车道归属不变。

### 6.2 需求单(扩展层 -> 核心层)

不新开文档,复用 build/pkg-gaps.tsv 并先修表:补齐 15 加 2 行破损行,加四列,由 tools 侧一个带 check 的小工具写入(经 devsvc 持锁)。字段:

- (1) 使用者:路径加调用点数(用 refactor.cx sites 或 usemat 一类命令给出,不凭印象)。
- (2) 缺口:一句话,加最小默认运行复现的路径。
- (3) 不满足的后果:现在的绕法及其代价(裸 libc 处数、被迫 dock 的文件、超时秒数),或"无法实现"。
- (4) 建议接口:签名草案、错误合同(返回负 errno)、三目标列(lnx、osx、win 各写 impl、refuse 或 unverified)。
- (5) 验收:一条命令加预期末行。
- (6) 归属类(甲乙丙丁)与层(L0、语言、libc 头、驱动),由边界组初判。

核心层的响应(每版开槽前一次,cc 主持,不超过 30 分钟):每张单四选一:接受并给版本号;改派扩展层(给出可用的现有接口名);拒绝(写理由,不留 OPEN);要求补复现。接受的 L0 单进当版附挂额度,超额按"使用者数 x 后果"排队。两版无人响应的单自动升级到主调度。已交付的单由交付者关单,边界组每周扫一次"能力已在仍 OPEN"。

### 6.3 影响评估(核心层变更 -> 扩展层)

接口变更(layer-api 判 X 类,或 Y 类但改了错误合同)合入前必须附一张受影响使用者表:

- (1) tools/layer-api.cx 给出变更类与签名差。
- (2) 仓内使用者:refactor.cx sites 逐符号列调用点,按 1.1 的使用者行汇总。
- (3) 仓外使用者:新建一张登记表(建议 build/ext-consumers.tsv:使用者、路径、取源的提交号、联系窗口),现至少 sandbox-cx 一行;未登记的仓外使用者不受保护。
- (4) X 类变更须先发弃用期一版(旧名保留为 alias-of),受影响车道的维护者确认后再删;csih 与 ujs 只发通知,不替它们改。
- (5) K2 这类"拒绝合同变更"同样走本流程:先跑一遍全仓加已登记仓外使用者的预检,列出会变红的文件数再定生效版本。

### 6.4 每周节拍

- 每版(约 5 到 6 小时一版):开槽前需求单评审 -> 核心列车与库快车道并行 -> 冻结 -> 整链与 CI -> 公开后工具线两项。
- 每周一次:慢车道正规一轮;46 探针对 gcc 差分普查并更新 POSIX 与 asm 两个分数;重跑 usemat 出矩阵差分(看裸 libc 每千行与 ext 采用数是升是降);pkg-gaps 过期与陈旧扫描;realprog 重跑一项。
- 每周一个判定点:A 系列下一片与前端硬上限、L0 小件之间的顺序,凭当周普查与 realprog 数据重排,不凭计划惯性。

## 7 回答 (6):风险与不确定

- (1) 未核:pkg-gaps 的四类条数是手工归类,误差约正负 5;"已交付仍 OPEN"的 11 条只核了接口名存在,未逐条跑验收。
- (2) 未核:export 在 0.0.85 只测了被接受,名单是否被强制、与 cxpack.tsv 是否互校未测;别名只测了 lnx 默认运行。
- (3) 未核:include/regex.h 存在但 prd.md 写缺 regex.h,实现程度未测;信号定时器是否排在本次未搜到的文件里。
- (4) 未核:裸 libc 计数是固定名单的记号扫描,会把同名的自定义函数与宏算进去;与棘轮基线不可逐一对账。csih 141 个文件是不跟符号链接的口径,与 find -L 的 421 不同。
- (5) 风险:sandbox-cx 的包根链接指向 .claude/worktrees/v0.0.86-l1,而规则是发布后删工作目录;删了它就编不过。仓外使用者需要一个稳定的取源方式(已装根加版本,或按提交号 git archive)。
- (6) 风险:L0 小件附挂额度若不写成决定,会与"一个候选只含一类大改"相撞;写成决定后又可能被当作塞大改的口子,需要 layer-api 的 Y 类判定作硬门。
- (7) 推断:所有单位估值;"工作者进程池可替代线程"基于 devsvc 现在就是多进程,未做原型。
- (8) 推断:tools 的裸 libc 是习惯而非缺口,依据是 sandbox-cx 一个样本;cxlib 里是否有 tx_ 无法表达的用法(如 sscanf 类)未抽样。
- (9) 口径限制:本次没有量 csih 与 ujs 的编译与运行时间(庚列写未测);devsvc 的数不含 cxlib 下的 flow 与 rel 单元。
