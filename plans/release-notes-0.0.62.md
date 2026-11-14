# unisacc 0.0.62 -- 发布说明

状态:草稿,定稿于公开之前。沿用 plans/release-notes-0.0.61.md 的口径:只写已入库事实;未实跑的项一律写未跑,未证实的项不写 PASS;本文件不宣称 QUALIFIED。带 TBD 的字段由发布流程填写,不得在此臆测。本文件随提交 Y 入库,不含 Y 自身哈希,也不含公开之后才发生的读回结果。

- 冻结提交 F=9269dfa6f4883ad8dd3595de6c133d4d23d6f00c(标签 v0.0.62 与 v0.0.62-src 均指它;real6 起链所用 REV 为 `b2c557b1ba`,F 取其树)。F 的 release/candidate.tsv 行有缺陷(20 列、commit 列空),规范表由 X 携带。
- 提交 X=cccf51ada6ae2d23a664681452f35f7a7e3efb68;提交 Y1=73f861c674dec5611fbe6b881e6b56fe0c94b581;Y2:TBD(提交号定稿后补)。
- 同候选 CI run 43581179059:全部 10 作业 success,ci-verdict PASS all-jobs-success。
- 候选 APE `unisacc.com`(real6 链内产物,链内版本串 0.0.61,版本注入前):sha256=5dee23615a37181dcac3062e1404a139c2722ebfb45675c591e94f730f780100,7059300 B。公开 APE(注入 0.0.62 后):sha256=83b82c8dbd1a2426d29844d0ec1bb0db58246067110f02bc882c7bd688b4fd4d,7059300 B,`--version` 为 unisacc 0.0.62;公开后读回 sha 与 release/candidate.tsv 一致;SHA256SUMS 含七个资产。
- 公开:2026-11-14 09:41Z,GitHub Latest,https://github.com/partnernetsoftware/unisacc/releases/tag/v0.0.62 。装机已完成(本机根 sha 同上,productpkg-dir-identity 批次 PASS)。
- 裁决(release/decision.txt):NOT_QUALIFIED,unknown=1(s9 commit 档未出 PASS;公开后读回已 PASS)。公开前 decision.txt 记 unknown=2。
- git 范围 `v0.0.61..b2c557b1ba`:165 个提交(git rev-list --count 实测);此前起草时的 171 为 `v0.0.61..HEAD` 的旧读数,以 165 为准。

## 是什么

面向 C99 的编译器,自己写出 ELF、Mach-O、PE,可编译自身;产品形态为单文件 `unisacc.com`。0.0.62 的主题是封网(库包层的套接字与网络原语)、cxmod 三源收敛与整链/批次分治工具。上一发布版见 `plans/release-notes-0.0.61.md`。产品输入变更按 prd_mempalace.md 的决定归为同一类(库包与运行库),cxmod 三源作为缺陷修复并入;构造链与批次工具不属产品输入。

## 行为变化(产品输入)

库包与运行库(版本以 build/tlib-packages.tsv 为准:cx.sys 0.1.10、ext.net 0.1.8、cx.plan 0.1.3、cx.compile 0.2.7):

(1) cx.sys netraw(91a399dea0):套接字原语层,仅 Linux x86_64/arm64,osx/win 记 -ENOSYS;sys-syscalls.tsv 追加 snr_* 行并重生 sysnums;check-cx-sys-netraw 92 断言。
(2) cx.sys netpoll(959908611f):sy_sock_poll(基于 ppoll)与 sy_sock_optnum。
(3) cx.sys netmsg(832d52a8d3 及其前序):sendmsg/recvmsg 三参路径,不扩 `__syscall6` 内建,不改冻结的 netraw.cx;号表 NSYS 51 -> 53;check-cx-sys-netmsg 21 断言。
(4) cx.sys netnb(dfa159b9a1):sy_snb_get/sy_snb_set_nonblock,osx/win 为 -ENOSYS;check-cx-sys-netnb 15 断言。
(5) cx.sys sigwatch(9cdbf210ab):sy_sig_watch、sy_sig_taken,仅 Linux;cx.sys 0.1.10;check-cx-sys-sigwatch 14 断言。
(6) cx.sys refusals(e20f599456):拒绝原因集中表(exec/facts/refusals.tsv -> tools/refusals-gen.cx -> cx/sys/refusals.cx),先登记 10 个平台未实现点;check-refusals-gen 39 断言。
(7) ext.net 地址与阻塞 TCP(0.0.61 之后):addr 6 行、TCP 7 行(见 0.0.61 之后入库的 4eecb0e5fd、3e4b5bc5ec,冻结树之外)、sock 第三批 sy_sock_local/peer、sy_shutdown、sockopt、sy_tcp_accept(ms)、sy_wait_port 等。
(8) ext.net V6ONLY(c945e3a153、cff7b762a7):sy_tcp_listen_at 的 SY_LISTEN_V6ONLY 经 sy_sock_setopt 设 IPV6_V6ONLY;check-ext-net-sock 新增 9 条 V6ONLY 断言。
(9) ext.net unix 域流式(3502c3966d):ext/net/unix.cx,21 断言。
(10) ext.net UDP(40f8e7433b):sy_udp_bind/send_to/recv_from,走 netmsg,超长 -EMSGSIZE,到期 -ETIMEDOUT;24 断言。
(11) ext.net 截止时间 TCP 与数字地址解析(5000484c46):sy_tcp_accept_from、sy_recv_until、sy_send_all_until,到期 -ETIMEDOUT;resolve.cx 的 sy_resolve/sy_resolve4 只认数字字面量与内置 localhost,其余名返回 SY_EAI_BASE-2。
(12) ext.net 非阻塞 connect 与 dial(1d36f594f0、dfa159b9a1):sy_tcp_connect_to(非阻塞 connect -> poll 写就绪 -> SO_ERROR -> 恢复阻塞)、sy_tcp_dial(数字或 localhost);osx/win 记 refuse:ENOSYS。DNS 不做。
(13) 平台审计与 libc99posix 表扩面:build/libc-decls.tsv、build/enosys-sites.tsv 两张审计表;libc99posix.tsv 由 30 行扩到 129 行(cx-own,头内 static);ext 下 34 个无名 ENOSYS 点补原因注释(仅注释)。

cxmod 三源(cx/cx0/plan.cx 权威、src/cxmod.c 由 cx0-extract 生成、cx/compiler/cxmod.cx 手工对齐):

(14) 别名运行路径修复(4aee1dfb4a):改名后的模块体此前只进 -cx-plan 打印,运行与 -o 仍编原始源;现驱动编单元与取 CXIF 都落到改名体;未导入导出的唯一名编号修正;改名体文件名改内容摘要。
(15) 模块身份按规范路径判同(e576000457)与 cxm_deps_same、two lexical entries 拒绝对齐(4367e1b0e5)。
(16) CX-U-2 三条具名拒绝(57c7427f6f):`use NAME::*` 报 bad symbol name;导入模块未导出的符号报 does not export;符号级导入后调用未导入符号报 call to undeclared function。这是行为收紧,不是纯修复。
(17) T5 cxm_checkcalls 上移(68039f11f1):进权威源 cx/cx0/plan.cx,src/cxmod.c 重生成(EXACT 73271)。

以上第 (14) 至 (17) 项的提交说明均写明"待下一轮整链验证"或"旧产品 UNVERIFIED-需新产品";在新产品上的验证见已知缺口。

## 整链与构造改进(非产品输入,随本版带出)

(18) candidate-chain 新增 cache-manager 相(5c4ec0e227、b54d7db899):链内用 tools/cache-retain.cx 自产 osx x86_64/arm64 两份,pack 默认用之,`--cache-manager-osx` 显式覆盖优先;check 加 6 条断言。该相在候选上的首次实跑结果(real6):链内自产 osx cache-manager(不再借用副本),pack 链内 37.8 s,FAIL/REFUSE 为 0。
(19) 起链前置门(9a97992301):拒绝 src 工作树 HEAD 与 `--rev` 不符、无 ident helper、pack 缺 `--cache-manager-osx`(rc 2,具名)。
(20) 整链墙钟:real5(g2/g3 linux 对并发,G_LNX_PAIR=2)3538 s,对照 real4 的 4759 s(-26%);只有这一组对照,未控制机器负载,不作结论(prd_mempalace.md 阻塞节整链一条)。real6(0.0.62 候选整链):REV `b2c557b1ba`(私有提交 70c31f0e9f),rc=0,墙钟 3752 s(与 real5 3538 s、real4 4759 s 各为一组未控负载对照,不作结论);g2/g3 linux 并发;g2/g3 对象各 18 项,FAIL/REFUSE 为 0;最长相 g3-lnx-x86_64-objects 734 s、g3-lnx-arm64-objects 734 s,g2 两个约 725 s。
(21) 其他:默认写仓根 out/ 的工具改写 TMPDIR(1ecb358822);ratchets 共享一次 `git ls-files` 清单与 coreapi 改 -run 查询,ratchets 墙钟约 12.8 秒降到约 6 秒(8c8067d764,单次观测);check-* 里 tx_len 返回 -1 的越界读修复(b4328f4381、cc22f4539d)。

## 工具层(非产品输入)

(22) tools/derive.cx 派生物总闸与 exec/facts/derive.tsv(919bfe812b):check 50 断言;`--write` 在生成器非 0 时还原本轮已写的登记 outputs。
(23) tools/status.cx 一条命令状态摘要(7cbc7b58bd),check-status 29 断言。
(24) tools/chain-ready.cx 起链前置条件一条命令(28459b3911,f87d3ee2c9 补 affected 退出码 3 判 UNMEASURED),check-chain-ready 32 断言。
(25) tools/cx0-diff.cx cxmod 三源差异表(62901f0fc7、3761234e57),落 build/cx0-three-source-diff.tsv;tools/check-cxmod-tri.cx 改行为级比较并含 CXM_MAXU 夹具(98e5adbc09、4a94e2b141)。
(26) dock-audit:build/dock-audit.tsv 与 tools/check-dock-audit.cx(d755923493、2fc15b0295);审计人、理由、裸调用符号数逐项未填,均 UNAUDITED。
(27) 批次选择:affected --iface-narrow(接口摘要未变时下游 use 批降级为 DEFERRED_BY_IFACE,3d7c610228,1420444e98 改按包判定)与 --symbol-narrow(符号级收窄,7c7124c12c);tools/iface-hash.cx 与 build/iface-hash.tsv(80e001c0b4)。实测选批数(各提交说明原文,基线不同):iface 段改 cx/text/text.cx 由 895 降到 757(3d7c610228,scanned=1061);符号级段改 tx_upper 体由 768 降到 4、延后 764(7c7124c12c);只作选批降级,默认与 --run 不带开关仍全量。符号级不识别函数指针与宏引用的间接调用。
(28) 其他:tools/phase-graph.cx、build/chain-phase-outputs.tsv(写集未经 strace,UNVERIFIED);cxinclude 棘轮新增 cx-wildcard-product(N9);guard-edit 钩子拦解释器与 stash/reset 等(a3fb9da2f4,59 断言)。

## 实测速度

来源 build/speed-baseline.tsv。0.0.62 候选产物未实测速度,候选的 first_e2e、冷编译:TBD(real6 产物出来后用 tools/run-startup-bench.cx 与 tools/coldcompile.cx 补测)。已有读数仅为已装 0.0.61 的参照:该表 6bcd7a98fb 追加行(startup 3 次、coldcompile 三文件各 3 次、gencode 一次),冷编译标 noisy,speed-probe 为 UNMEASURED。表内 2026-11-13 行(e43ef36832939c7f,n=3,首次 796.354/796.576 ms)为 0.0.59 基线。均非 OS 冷。0.0.62 不宣称提速;SPEED-11 运行库扫描族与 E3TRIM、-c 对象复用均不在本版(见下)。

## 已知缺口与未验证(如实)

按影响分三组:甲、会让用户踩到的缺陷与功能边界;乙、未验证的平台与环境;丙、未跑的验证范围。每条标注依据(实测/已复现/推算/读码推断/UNVERIFIED)与绕法。

### 甲、会让用户踩到的缺陷与功能边界

(1) DNS 未做(功能边界,既定):sy_resolve/sy_resolve4 只认数字字面量与 localhost,其余名返回 SY_EAI_BASE-2;sy_tcp_dial 同。绕法:调用者自行解析后传数字地址。
(2) std-surface 仍有 2 行 new:`sy_res_live`(cx.sys,表内平台格为 refuse:ENOSYS,本版未实现)与 `np_exec`(ext.procx);pkg-gaps 已记。B2 六参系统调用不扩 `__syscall6`,走 sendmsg/recvmsg 三参。封网在本版即此状态。
(3) sy_unix_connect(ext/net/unix.cx)截止时间只在调用前检查一次,之后是阻塞 connect:监听方积压满且无人 accept 时越过截止时间不返回(已复现,外层 timeout 杀掉)。绕法:监听方保持 accept 循环,或调用方自行在线程/子进程里带外部超时。0.0.63 修复补丁草稿已备(非阻塞 connect,-EAGAIN 按截止时间重试,到期 -ETIMEDOUT),见本节末补丁说明。
(4) sy_tcp_accept_from(ext/net/tcp.cx)带 peer 时只认 inet4/inet6:对 sy_tcp_listen_at(SY_AF_UNIX) 的监听 accept 成功后返回 -EAFNOSUPPORT 并关掉已接受的连接(已复现)。绕法:unix 监听上用 peer=0 调用,或不用 listen_at 的 unix 族。补丁草稿已备(accept_from 内加 unix 分支),见本节末补丁说明。
(5) poll 就绪后的阻塞调用不保证截止时间:udp 的 recvmsg(数据报被内核丢弃时)、accept(连接在 poll 与 accept 之间被复位)、阻塞 fd 上的 send 可能越过截止时间(读码推断,未复现)。绕法:对这些 fd 先 sy_snb_set_nonblock,或在外层加超时。
(6) syt_wait 与 netpoll 把 EINTR 原样上抛:sy_tcp_connect_to、sy_recv_until、sy_send_all_until、sy_udp_recv_from、sy_wait_port 在等待期间收到已处理信号就返回 -EINTR(sy_send_all_until 可能已发一部分;sy_wait_port 总等待变短)(读码推断,未复现)。绕法:调用者对 -EINTR 按剩余截止时间重试(send 类需自行记已发字节),或不对同一线程装计数信号。
(7) netmsg 的 recvmsg 入参 salen 为负时被当大无符号 namelen 交给内核,内核取走数据报后才报 -EINVAL(数据报丢失);回写的地址长度可大于入参余量(读码推断,未复现)。绕法:salen 恒传非负且不小于 SY_SA_MAX(udp.cx 现状已满足)。
(8) sy_sig_taken(cx/sys/sigwatch.cx)读后清零非原子,读与清之间到达的信号计数会丢(读码推断,未复现)。绕法:只把计数当"是否非零",不依赖次数。修法需 L0 原子交换或屏蔽,冻结面不能做,UNVERIFIED。
(9) cxm_checkcalls 误拒短名函数指针调用写法:单元里「标识符后跟 (」与程序中某个未被本单元 use 的模块的导出函数同名时,报 `call to undeclared function NAME (declared in M, which X does not use)`,包括结构成员 `o.bf()`、局部/形参/文件级函数指针、typedef 返回类型的原型、`#include` 头里的原型(已复现)。已装 0.0.61 发布件同样报错,是既有缺陷,不是 0.0.62 回归。绕法:把函数指针调用写成经本单元声明的间接名,或重命名导出函数。0.0.64 修;补丁草稿已备,仅在审查副本验证(误拒夹具转 RC=0,原拒绝例仍拒绝),未入库,未装新产品。
(10) cx 模块缺口:同一文件经软链(tools/cxlib/sys -> cx/sys)被当两个模块(`sy_env exported by both`),使 tools/osx-sentinel.cx 等编不过,未修;`use m::sym as alias` 通过规划但运行报 `no host function slen`(rc 127,修复见上文行为变化第 (14) 项 4aee1dfb4a,新产品上未验证);`#pragma unisa export` 未实现;`use sys.*` 未设计;仓外脚本找不到包根。
(11) -c 对象路线无最终产物复用:现状是 artifact_hit 标记戳丢失(只丢戳,既有状态,每次全跑约 280 ms),丢戳的提交未二分(候选 a3a32d8a09、9b107888db 均未实证);phase-timing-stamps 断言已改为真实行为(dfe2b5fa75);真复用为 0.0.63 任务 N15。
(12) osx/win 上 netraw、netpoll、netnb、netmsg 的 -ENOSYS 点没有进 exec/facts/refusals.tsv(现仅 fs 与 mem 的 10 行),按具名拒绝口径是缺口;tools/enosys-sites.cx 对账未跑(读码核对,UNVERIFIED)。绕法:无,osx/win 本版即不支持这些接口(见 (14))。

补丁说明((3)(4) 共用):补丁草稿 $CLAUDE_JOB_DIR/tmp/sx.patch.txt 在 git archive 副本上实跑:r-unix-block 复现由"越过截止时间不返回"转为各次 -110 且 300 ms 返回;r-unix-peer 复现由 -97 转为 accept 成功、连接保持;check-ext-net-unix 24 项与 check-ext-net-tcp 69 项全 PASS(各新增 3 项)。仅 lnx-x86_64 本机,未入库,未装新产品,未在 arm64/osx/win 跑。

### 乙、未验证的平台与环境

(13) 平台:arm64 的 msghdr 布局(netmsg)只按 LP64 同为 56 字节推断,未实跑;lnx/x86_64 探针 `sendmsg=4`、`recvmsg=4` 为本机一次实跑(prd_mempalace.md B2 证据行)。
(14) osx/win 无证据:netraw、netpoll、netnb、sigwatch、netmsg 在 osx/win 为有名 ENOSYS 拒绝,没有对应主机实跑;osx/win 平台批次需对应主机。
(15) osx cache-manager:借用副本(x86_64 452d9240、arm64 3f6e4cbd)不能由现有输入重现,来源未证实;自产件(x86_64 89d0b937、arm64 5096089c)在 macOS 上的运行与签名未验证,需 osx 主机。
(16) wt-gate-probe 为 PLANNED(链路超 55 s 批次上限),6 条断言未验证;wt-gate-clean-win 为 PLANNED/NOT_READY,无证据。wt-gate-clean 已 PASS。

### 丙、未跑的验证范围

(17) 新产品验证:CX-U-2 三条拒绝、T5 cxm_checkcalls 上移与别名运行修复需要新产品验证:tests/cx-namespace.cx 的两条别名运行断言在已装 0.0.61 上输出 UNVERIFIED needs-new-product,不计 PASS 也不计 FAIL。已在 real6 候选(sha256 5dee2361...,链内版本串 0.0.61)上验证:tests/cx-namespace failed=0 unverified=0,CX-U-2 三条拒绝、别名运行、通配拒绝转真;候选上 V0 身份 PASS、V1 release-ape smoke PASS、V3 module units PASS、V4 module check PASS(V4 内部实际验证的是已装产品,不作候选证据);V5 装机后跑,V6 s9 commit 档进行中,均未出结果。起链前的 P1 扫描(HEAD b2c557b1ba,1190 个含 use 的 .cx,cx0 与产品各 1175 通过 15 被拒,新拒绝数 0)是已装产品上的规划扫描,不等于候选产品验证。build/cx0-three-source-diff.tsv 仍有语义分叉行;cxmod 三源中产品件是手写 cx/compiler/cxmod.cx。check-cxmod-tri 有具名 GAP(CXM_MAXU:97 单元 plan.cx 拒绝、产品通过;128 边界未测)。
(18) 批次复验面:lnx IMPLEMENTED 约 1040 批至 1076 批(口径随登记变化,以 `tests/run.cx -- --list` 为准;2026-11-14 实测 PLAN 行:batches=1111,implemented_batches=1089,planned_batches=22,executed_batches=0,该口径为全平台合计,未单独拆出 lnx 数),在快照根复验的累计只有 45 批(PASS 43,FAIL 2,为两项修复之前的口径,修复后两项在共享检出上各自复验为 PASS,但没有在快照根上重跑);其余未复验。FAIL 两项均已修:ri-image-pem(duplicate-key-row,a1bf73b583)与 phase-timing-stamps(断言期望改为真实行为,dfe2b5fa75);两项修后的复验计数以 prd_mempalace.md 阻塞节为准,本文不据此改写累计数。共享检出另有 15 批(PASS 14,FAIL 1 即 tsv-lint,见本条后段)。约 80 个 FTRIM-bootstrap 红批装新产品后复验,未验证。全集复验必须分片(推算,非实测):单批固定开销约 27 s,慢批 pp-cli-includes、pp-header-locations 超 37 s 被杀(须单跑改 heavy),snapshots-cx 临界(批内约 30 s);light 约 742 批约 93 波、heavy 313 批每波 1 批。tsv-lint 同日三次实跑 PASS、FAIL、PASS,疑与并发写表有关,未定因(实测);冻结窗口单跑 `affected.cx --ids tsv-lint --run -j 1` 为 PASS rc=0 22 s。affected 选批降级不替代全量复验。
(19) s9 未跑:按决定只有 commit 档计划卡发布,daily 与 release 档夜跑,结果如实入说明,不卡发布。如实状态:commit 档只有 0.0.53 的实测(3 步 252.7 / 87.3 / 12.7 s),0.0.62 候选上未跑;daily 档(预测约 16111 s,条件值)与 release 档至今未跑通一次,墙钟 UNMEASURED。aggregate 到 release/evidence.tsv 第 4 行的映射靠人工判读,没有工具(缺口,待记 build/pkg-gaps.tsv)。0.0.61 的第 4 行为 UNKNOWN。候选 s9 判定:TBD。
(20) cache-manager 相在候选上的首次实跑与 real6 整链结果:已实跑,首链不带 `--cache-manager-osx`,链内自产 osx cache-manager,未借用副本,rc=0(见行为变化 (18)、(20))。
(21) 同候选 CI、本机全批资格:TBD。以上未验证项不得在发布页写成已验证。
(22) 其他未覆盖:check-stdio-buffer 不覆盖 --link-only 与子集编译的缓存 identity,tty/pty 未测,六目标未跑;build/speed-baseline.tsv 未更新,run-startup-bench、check-bench-gencode、speed-probe 对新产物未跑。
(23) 审计与派生登记不全:dock-audit 全部 UNAUDITED;derive 总闸登记项仅 6 至 7 项。
(24) 0.0.63 项不在本版:E3TRIM(modelcache.cx 每次发布全量扫描修剪,补丁未入库)、-c 对象路线真复用(N15,见 (11))、L1/L2/L4 驱动与运行缓存加速、SPEED-11、cx.stream/tlib 重构、export/通配语法(D1/D2)均不在 0.0.62。

## 撤回与回滚

按运行手册(release/rollback.txt 形态,0.0.62 版:TBD,由发布流程按 0.0.61 样式生成并填 RUN_ID 与 X):撤回公开用 `gh release edit v0.0.62 --draft=true` 并把 v0.0.61 重设为 Latest;main 上提交只用 `git revert`,不 force push;本机根回退用 `tools/install-ape.cx --rollback`。产品输入侧回滚:cxmod 三源改动可撤对应提交(4aee1dfb4a、e576000457、4367e1b0e5、57c7427f6f、68039f11f1)回到 0.0.61 行为,库包新增文件为只增,撤提交即回现状。

规则:发布页正文必须与本文件逐字相同,公开后读回核对;读回结果记账本。

## 待核对

以下为整理时发现、未自行改动的点,请对仓内事实核对:

(1) 缺口 (2) 与旧 (18) 合并后,"std-surface 仍有 2 行 new"沿用原文,未对 build/std-surface 当前表重算。
(2) 缺口 (18) 中"共享检出另有 15 批(PASS 14,FAIL 1 即 tsv-lint)"与同条"tsv-lint 同日三次 PASS、FAIL、PASS"并存,未核对其时序与最终状态。
(3) 行为变化 (20) 与"已知缺口 (20)"编号相互独立(前者是整链墙钟,后者是 real6/cache-manager 未跑),正文引用均已按新编号核过;缺口 (10) 引用的"第 (14) 项"指行为变化节。
(4) 缺口 (3)(4) 的补丁文件 $CLAUDE_JOB_DIR/tmp/sx.patch.txt 位于作业临时目录,发布前是否应入库待定。

公开后待补:候选速度基线(first_e2e、冷编译)、s9 全批(commit 档候选实跑结果及 daily/release 档)、Y2 提交号。
