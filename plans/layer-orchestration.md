# 核心层与扩展应用层团队编排(主调度自主编排,所有者 2026-11-20 要求)

状态:补全(2026-11-20,依据 fable 矩阵报告 plans/reviews/2026-11-20-core-ext-matrix.md)。相关:plans/reviews/2026-11-20-layer-lanes.md、plans/claude-md-layer-lanes-draft.md、plans/roadmap-0.0.83-0.0.99.md 修订注"核心层与扩展应用层矩阵与版本调整"。

## (1) 三组

| 组 | 范围 | 成员 | 车道 |
|---|---|---|---|
| 核心层组 | 编译器、核心层、内置 libc 与 POSIX、cx 语言扩展 | cdx、agyneon | 版本列车 v0.0.NN,重车道:整链加预跑加 CI |
| 边界与工具组 | 身份口径统一 lane-inputs、promote 按 A 键复用、包门工具、devsvc 与提交门、T1 T2 T3 | agy-flow,opus 夹具,cc 集成 | main(工具线) |
| 扩展与应用层组 | ext 包、tools/cxlib、apps、ujs、sandbox-cx | ccapi、cdx2、agy-cage | main 主干式轻车道,待包门工具就位生效 |

边界:扩展组可以改 ext 纯包(crypto、net、tsvx、testx、argx、fsx 与 sysx 的非别名文件);ext 里被 cx/sys、cx/mem、cx/text 链接的 14 个文件按核心层对待,只能由核心层组改。csih 与 ujs 的车道归属不变。

## (2) 核心层接口面(摘要,全文见报告第 2 节)

| 类别 | 现有面 | 已知缺口 |
|---|---|---|
| L0 sy_ | cx.sys 92 行、cx.os 138 行 | sy_syscall6、sy_random、mtime 纳秒、rusage、stderr 分离与带 cwd 的 run、等待集、杀树接超时、PDEATHSIG |
| L0 mm_ | cx.mem 63 行 | idmap;mm_copy 目标容量易误用 |
| L0 tx_ | cx.text 46 行 | 行索引、无正则 |
| 邻接包 path stream os | 面在 | 几乎无人用,不是缺口 |
| L1 执行器与驱动 | base runtime compile driver plan | 无阶段化前端 API;运行缓存不按内容取键;并行首次编译丢缓存 |
| 内置 libc 与 POSIX | 约 45 头加 sys 20 个 | epoll.h random.h 无文件;定时器信号缺;pthread 链接不到;原子 TLS 全拒;regex.h 实现程度未核 |
| cx 语言扩展 | module use as 别名 dock export | 重导出 D1b、仓外包根、cxroot 无 net 前缀、dock 无强制(K2) |
| 外部 C 前端 | C99 主体 | 结构体成员 64、不完整数组、-funit 对象 ABI 5 条 |

登记缺口:ext/testx argx fsx 不在 tlib-packages 也无 layer-api 表;argx fsx net 不在仓根 cxroot,靠 tools 下符号链接解析。

## (3) 需求矩阵摘要(报告第 3 节)

- 核心层对 cx 使用者供给基本够:sandbox-cx 11774 行去打印后裸 libc 只有 3 处;真问题是采用率与登记不全。
- 裸 libc(去打印)每千行:tools/cxlib 228、candidate-chain 149、devsvc 140、ujs 42、csih 34、sandbox-cx 0。
- 线程原子一列没有内部使用者被卡。
- L0 真缺口集中在进程一族(等待集、rusage、stderr 分离、杀树接超时、PDEATHSIG)加 sy_syscall6、熵源、mtime 纳秒。
- 模块解析(包根与前缀)是最廉价的高杠杆;编译速度与缓存卡住所有工具使用者。

## (4) 缺口归属条数(pkg-gaps 169 行含 OPEN,手工归类误差约正负 5,未逐条回源)

| 归属 | 条数 |
|---|---|
| 核心层必须做 | 约 69 |
| 扩展层自己做 | 约 17 |
| 工具与流程自己做 | 约 72 |
| 文档或账本陈旧 | 约 11 |

表有 17 行列数破损,2026-11-20 已保守修复并把明确已修仍挂 OPEN 的行改状态(不删行)。

## (5) 前 10 杠杆项摘要(报告第 4 节)

- (1) 仓外脚本包根与 cxroot 前缀不全,约 0.5 到 1 单位。
- (2) 子进程等待集加杀树接超时加 PDEATHSIG,约 2 到 3 单位。
- (3) sy_syscall6 特权入口,K2 前置,约 0.5 单位。
- (4) 运行缓存按内容取键加并行首写不丢,约 2 单位。
- (5) 前端结构体成员上限 64 与不完整数组声明,约 3 单位(推断)。
- (6) 门面重导出 D1b,约 2 单位。
- (7) tsvx 采用率(125 个手工拆列文件对 6 个使用者),约 1 单位,零核心成本。
- (8) ext 三包入 tlib-packages 与 layer-api,0.5 单位,影响评估流程的前提。
- (9) sys/epoll.h 加非阻塞事件循环,约 2 到 3 单位。
- (10) 熵源 sy_random 与 sys/random.h,约 0.5 单位。

单位均为推断估值。版本落位见 build/version-seq.tsv 第 22 到 32 行(0.0.88 核心层库面小件包与模块解析登记)。

## (6) 当前派工快照(2026-11-20 16:00)

- cdx:D2 只剩 implicit-function。
- agyneon:W2 默认开关判定。
- agy-flow:T1 v5 加 T2 v4 加 affected 残留清理加 T3。
- L1:复核通过,整改中。
- fable:矩阵已出(plans/reviews/2026-11-20-core-ext-matrix.md)。

## (7) 需求单流程(扩展层 -> 核心层)

- 需求单复用 build/pkg-gaps.tsv,先修表再加列。字段:使用者(路径加调用点数,用命令给出)、缺口(一句话加最小默认运行复现路径)、不满足的后果(现在的绕法及代价)、建议接口(签名草案、错误合同、三目标列)、验收(一条命令加预期末行)、归属类与层。
- 核心层每版开槽前对每条需求四选一响应:接受并给版本号;改派扩展层(给出现有接口名);拒绝(写理由,不留 OPEN);要求补复现。
- 已交付的单由交付者关单;边界组每周扫一次"能力已在仍 OPEN"。

## (8) 影响评估流程(核心层变更 -> 扩展层)

- 接口变更前用 tools/layer-api.cx 判类并给签名差。
- 仓内使用者:refactor.cx sites 逐符号列调用点。
- 仓外使用者:登记在 build/external-users.tsv(使用者、依赖的包与接口、取源方式);首行 sandbox-cx 经 cxroot.tsv 指工作树,风险是工作目录发布后删除会使其编不过,v0.0.86 合入 main 后改指主检出。未登记的仓外使用者不受保护。
- 拒绝合同变更(如 K2)同样先列受影响使用者再定生效版本。

## (9) 每周节拍

- gcc 差分普查(46 探针,更新 POSIX 与 asm 分数)。
- 矩阵差分(重跑使用矩阵,看裸 libc 每千行与 ext 采用数升降)。
- 陈旧缺口扫描(pkg-gaps 过期与"能力已在仍 OPEN")。
- realprog 一项。

## (10) 规则

- 一人一个工作树。
- 一个候选只含一类大改。
- 派工后确认送达。
- 任务板与督导定时器每 15 分钟。
- 未验证问题不派落库与改配置。

## (11) 未核

归类条数未逐条回源;include/regex.h 实现程度未测;工作者进程池可替代线程未做原型;工具裸 libc 是习惯而非缺口只有 sandbox-cx 一个样本;信号定时器槽位;export 名单是否强制。
