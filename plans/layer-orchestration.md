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

## (6a) 流程总纲:按环节分割

各流程(需求单、影响评估、每周节拍)按 plans/v0.0.87.md"总纲:按环节分割"的环节分割表执行:每个环节写清输入身份、输出身份、等待时做什么;合入、复扫、发布列车、复核、集成者单点五个切法;门是唯一裁判,门过即合,冲突才升级给人。

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
- 证据入口一律写 ~/ 或仓内相对路径,不写 /home/<用户名>/、/Users/<用户名>/ 这类带系统用户名的绝对路径(本仓有公开发布);ratchet no-home-path 与 tools/no-home-path.cx --staged 守,已写的用 tools/scrub-home-paths.cx --write 改写。工具生成的证据在生成处输出规范化路径。

## (11) 未核

归类条数未逐条回源;include/regex.h 实现程度未测;工作者进程池可替代线程未做原型;工具裸 libc 是习惯而非缺口只有 sandbox-cx 一个样本;信号定时器槽位;export 名单是否强制。

## (12) 流程:主调度(cc)自身的事实批评与改进规则(2026-11-20,依据 plans/reviews/2026-11-20-fast-iteration.md 第 5 节)

事实:路线图一天改排 5 次(两次相隔 2 分钟);当天 12 份分析报告;0.5 单位的 T1 评审四轮跨 3 小时 18 分;load1 门当天加当天用 4 个提交撤;消费者清扫放在七条规则之后。

改进规则:
- 派工前先判问题是否成立。
- 分析报告每天不超过 3 份,且必须带决策。
- 评审超过两轮就削减范围,先落核心。
- 清扫左移:消费者清扫在每个产品提交后自动后台跑,不放到版本末尾(见 plans/v0.0.87.md S3)。

## 工作树清单 2026-11-20(清洁执行者实测,只列不删)

归属按名字与分支推断;"合入"指 HEAD 是 origin/main 祖先;最后活动均为 2026-11-20;共享检出 5.2G 未列。

| 工作树 | 分支或 HEAD | 归属(推断) | 合入 | 占用 | 建议 |
|---|---|---|---|---|---|
| d2-v086-private | detached e75c162dff | D2 窗口(cdx) | 合入 | 197M | 可删 |
| tools-s4 | tools-s4 | 工具 S4 窗口 | 合入 | 199M | 可删工作树,分支保留到 0.0.87 公开 |
| tools-s5 | tools-s5 | 工具 S5 窗口 | 合入 | 199M | 同上 |
| ujs | ujs | 论文 B 车道 | 合入 | 200M | 归论文 B,问维护者,unisacc 侧不动 |
| v0.0.84-prep | v0.0.84-prep | 0.0.84 预跑 | 未合入 | 215M | 0.0.84 已公开,核差异后可删 |
| d2-086-sync | cdx-d2-086-integrated | D2 窗口 | 未合入 | 207M | 0.0.86 冻结后复核 |
| d2-v086-const、func、ptr、final-scan | detached | D2 窗口分项 | 未合入 | 各约 198M | func 与 ptr 同一提交 29503ed611,可并一个 |
| tools-s2、tools-s3 | 同名分支 | 工具 S2、S3 窗口 | 未合入 | 各 199M | 待合入后删 |
| ujs-v085-integration | detached de422f82a2 | 论文 B | 未合入 | 199M | 问维护者 |
| v0.0.86、v0.0.86-d2、-l1、-w2d | 同名分支 | 0.0.86 版本车道 | 未合入 | 各约 198M | 在用,不动 |

分支(只列可清理):已合入且无工作树的 bench-v083、cdx-s3-land、r0-v082、v0.0.82-s2、v0.0.83-r2、v0.0.83-r2-identity、v0.0.83-release-flow、v0.0.84-s3 可删本地分支;v0.0.65 到 v0.0.85 版本分支均已合入,按规则保留到下一版公开后可删到最近两版;未合入的 cdx-s3-verified-snapshot、rehearsal-v082、ujs-d4-paused、v0.0.84-f1-meter 需各自归属窗口确认。

README 陈旧实测数字(停在 0.0.84,未在 0.0.85 重跑,未改):README.md 与 README_zh.md 第 22 行(0.0.84 公开件、APE 8417722 字节,POSIX 与 asm 面为 0.0.82)、第 33 行"实跑(0.0.84)"、第 37 行 `unisacc 0.0.84` 示例输出。
