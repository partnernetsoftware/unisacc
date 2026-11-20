# 核心层如何被区分与强制:取证与立场(2026-11-20)

只读取证,未改仓内文件,未提交。统计脚本 `/home/devali/.claude/jobs/2798e440/tmp/corestat.cx`(逐文件明细 `perfile.tsv` 同目录),实测探针在 `corelayer-probe-1120/`。输入:共享检出 main 22e3e32ef4,`git ls-files '*.cx'` 去掉 archive、ujs、research 后 2366 路径,其中常规文件 2315 个(符号链接不重复计)。

过程事故如实记:一次探针命令因目录名与已有文件撞名,cd 失败后把四个探针文件(main1.cx asm1.cx asm2.cx dockscript.cx)写到了仓根;已当场只删这四个自己的文件,`git status` 复核回到原状(仅剩原有的 release/state.tsv 未跟踪)。

## 0 最重要的判断

所有者要的"把核心层区别出来、只许它对接宿主"是对的,但缺的不是标记,是强制与权威定义。标记今天已经有了:`#pragma unisa dock` 三源都已解析,66 个文件在用。缺的是两件事:(1) 编译器对它什么都不做,无 dock 模块写 #include、裸 libc、平台 #if 实测照样编过;(2) 谁有资格写 dock 没有唯一的机器可读定义,现在是自我声明加事后审计表。换后缀同样是自我声明,不解决这两件事,成本却最高。所以我反对后缀(包括作为"可选第二步"),赞成主调度"层属性加编译器强制"的方向,但要改三处:层属性放 cxroot.tsv 而不是 tlib-packages;声明复用现成的 dock,不新造;1 到 2 单位只够拦 #include、平台 #if、__syscall、asm,拦不了裸 libc 调用。

## 1 现状

### 1.1 名词先对齐(容易说岔)

所有者说的"核心层"是对接宿主的那一层,仓里叫 L0(cx/sys、cx/mem、cx/text)。仓里叫 `cx/core` 的目录是 L1 执行器,规则上恰恰不许碰 libc(它现有 34 处裸 libc 债)。冻结门 L0-CORE-FREEZE-049 的前缀是 `cx/sys cx/core`,两者都含。后文"对接层"指所有者意思里的核心层,不用"核心"一词,免得与 cx/core 混。

### 1.2 现在靠什么区分与约束

| 机制 | 定义方式 | 强制点 |
|---|---|---|
| `#pragma unisa dock [include或libc]` | 文件头自声明;三源(cx/cx0/plan.cx 1325 行、src/cxmod.c 1273 行、cx/compiler/cxmod.cx 1297 行)只校验参数拼写,注释原文写明"a declaration only" | 无 |
| tests/cxraw-layers.tsv、tests/cxinclude-boundary.tsv | 路径前缀加精确例外(39 行、37 行) | 棘轮豁免 |
| 棘轮 cxraw、cxlibc、cxplatform、cxinclude | 逐文件计数基线(399、491、113、5 行),只降不升 | precommit 与批次;是计数不是拒绝 |
| tools/check-cx-layers.cx | 层前缀写死在工具里(L0 到 L3、EXT、AMB) | use 边方向 |
| tests/cx-module-freeze.tsv | 43 行冻结模块集 | 新增模块要改表 |
| build/freeze.tsv L0-CORE-FREEZE-049 | 前缀 cx/sys cx/core,截止 epoch 1795363200(2026-11-22 UTC) | precommit 拒提交;是时间窗冻结,不是层规则 |
| build/dock-audit.tsv 加 check-dock-audit | 每个 dock 文件一行 | 98 行里 89 行 UNAUDITED;`--write` 会给新 dock 文件自动补 UNAUDITED 行 |
| build/tlib-packages.tsv | 18 个包,有 roots、deps、opaque,没有层列 | 编译器不读(表头自述正规链不读 tlib) |

### 1.3 缺口(均有实测或源码行)

(1) 违规不被编译器拒绝。实测:带 `#pragma unisa module` 且无 dock 的模块里写 `#include <stdio.h>`、`#if defined(__linux__)`、`strcmp`、`malloc`,被脚本 use 后运行输出 r=1,rc=0。

(2) dock 是自我授权。cxraw、cxlibc、cxplatform 三个棘轮的 `declares_dock` 是对文件前 4095 字节做 strstr("#pragma unisa dock") 加 strstr("#pragma unisa module"),任何模块自己加一行就整文件免计;唯一的约束是审计表,而审计表九成未审。plans/cx-dock-pragma.md 规则 4"dock 只许出现在对接层语义的模块"没有机器检查(推断:未见对应断言;check-dock-audit 的 A1 到 A6 只核表与文件一致)。

(3) "对接层"没有唯一定义,至少六处各说各的,且已经互相不一致:prd_cx_spec.md 3.4 说只有 cx/sys、cx/mem、cx/text 能 dock;实际声明 dock 的还有 cx/os 2、cx/path 1、ext/sysx 13、ext/net 6、ext/memx 1、ext/textx 4、ext/tsvx 3、tools/cxlib 1。dock-audit 把 cx/cx0/plan.cx 与 cx/compiler/cxmod.cx 记成 bare dock,而这两个文件行首并没有该 pragma(只是解析代码与注释里出现这串字),同时 cxlibc 基线又照常给 plan.cx 记 65 处债。同一事实三个工具三种判法。

(4) 内联 asm 现在对谁都不可用:.cx 与 .c 实测都报 `reject: not covered: inline asm`,rc=1;全部 2315 个 .cx 里 asm 记号出现 0 次。这是好消息:asm 的层规则今天定,迁移成本为零。

### 1.4 取数(固定 113 个 libc 名的记号扫描,跳过注释与字符串;与棘轮的 symindex 口径不同,数值不可与基线逐一对账)

| 桶 | dock | 文件 | #include(文件) | 裸 libc(文件) | 去掉打印类后 | 平台 #if | __syscall |
|---|---|---|---|---|---|---|---|
| cx/sys | 是 | 25 | 8(6) | 218(13) | 213 | 96(19) | 24(9) |
| cx/sys | 否 | 2 | 0 | 0 | 0 | 6(1) hostid.cx | 0 |
| cx/mem、cx/text | 是 | 10 | 0 | 15(3) | 15 | 0 | 0 |
| cx/os、cx/path | 是 | 3 | 0 | 0 | 0 | 1 | 0 |
| cx/core | 否 | 9 | 0 | 34(6) | 34 | 0 | 0 |
| cx/runtime | 否 | 87 | 0 | 156(4) | 156 | 0 | 0 |
| cx/compiler | 否 | 78 | 0 | 18(1) | 18 | 0 | 0 |
| cx/driver | 否 | 12 | 0 | 10(2) | 4 | 2(2) | 0 |
| cx/cx0 | 否 | 12 | 0 | 83(2) | 83 | 4(1) | 3(1) |
| src | 否 | 25 | 0 | 367(14) | 232 | 2(1) | 1(1) |
| ext/sysx | 是 | 13 | 12(4) | 168(9) | 165 | 22(10) | 8(5) |
| ext/net | 是 | 6 | 0 | 0 | 0 | 0 | 0 |
| ext 其它 | 是 | 8 | 0 | 35(5) | 35 | 0 | 0 |
| ext 其它 | 否 | 20 | 0 | 0 | 0 | 0 | 0 |
| tools/cxlib | 否 | 85 | 0 | 4399(67) | 4066 | 10(5) | 11(5) |
| tools 其它 | 否 | 1244 | 0 | 35072(983) | 26880(844) | 65(35) | 8(8) |
| tests | 否 | 489 | 0 | 10280(325) | 8053(261) | 72(67) | 13(9) |
| apps 等 | 否 | 175 | 40(10) | 2460(85) | 2076 | 6(3) | 0 |

读法:

- #include 已经干净:对接层之外只有 apps/csih 的 10 个头面探针(都不是模块,属 csih 车道,有白名单理由)。在"无 dock 模块"里出现次数为 0。所以"无 dock 模块写 #include 编译期拒绝"今天就能开,零迁移。
- 裸 libc 远没干净:产品上层约 670 处(src 最多,其中 host.cx、hostout.cx、hostres.cx 约 80 处是白名单内的引导层;cx/runtime/source_frame.cx 一个文件 150 处),tools/cxlib 这个被 use 的工具库 67 个文件 4399 处。
- "tools 允许 printf"的现状其实是 tools 用全套 libc:去掉打印类之后仍有 844 个工具文件、261 个测试文件在直接调 malloc、fopen、strcmp 一类。任何对脚本的编译期限制都会一次打红上千文件。
- ext/sysx 用 libc 是合法的:13 个文件全部声明 dock,且在两张白名单里;ext/net 声明了 dock 但裸调用为 0(经 sy_ 包装),属声明过宽。
- tests、tools 的平台 #if 与 __syscall 计数含检查夹具里的代码形状,未逐个分辨(未核)。

## 2 三个方案

### (A) 新后缀

- 候选与冲突:`.cx0` 与 cx/cx0 目录、cx0 环、cxroot 的 cx0 前缀撞名,排除;`.cxc` 易与 cx/core(L1)和 tools/cxcc.cx 混,且像"编译产物";`.cxs` 与汇编 .s/.S 以及账本前缀 CXS- 混,在汇编成为基本需求后尤其糟;`.cxk` 与生成物 kernel/ 混;`.cxd`(d 即 dock,与现有 pragma 同义)是唯一语义干净的。已核:仓内无 .cxc .cxs .cxk .cxd .cxl .cx0 后缀的跟踪文件,非归档文件里也无这些字样。
- 改动面:改名 44 个 L0 文件(cx/sys 27、mem 7、text 7、os 2、path 1)加 ext 里 27 个 dock 文件;模块解析要认两种后缀,三源同改(plan.cx 里拼 `.cx` 候选名至少 6 处,cx/driver/args.cx 270 行);派生表全动(tlib-packages 的 roots、layer-api 的 file 列、cx-module-freeze、dock-audit、symindex、tool-index);所有按 `*.cx` 取文件的工具与棘轮要改 glob;引用 .cx 路径的有 983 个 tsv、179 个 md,tests/batches.tsv 单表 1680 行含 .cx。
- 强制点:后缀本身仍是自我声明,谁都能把文件叫 .cxd,仍需路径或包白名单;等于在 B 之上再加一层改名。
- 对用户:多一个要解释的概念;编辑器高亮、`unisacc.com FILE.cx` 的入口习惯都要兼容。
- 与冻结门:cx/sys 在 L0-CORE-FREEZE-049 内,改名即撞冻结;产品输入变更,须整链重建并换身份。
- 成本与风险:估 4 到 6 单位(推断),风险是一次性波及面最大的机械改动,且 git 历史与 `git log -- 路径` 规则(新建改名前查历史)全部被触发。
- 收益只有一条:目录列表里一眼可见。这条 `tools/dock-map.cx` 已经能给。

### (B) 包层属性加编译器强制

- 层属性放哪:编译器解析模块时读的是 cxroot.tsv(CX-U-7),不读 tlib-packages。所以层列应加在 cxroot.tsv 的 R 行第四列(可选,缺省为 upper),tlib-packages 用一条 derive 检查保持一致,不作第二真源。
- 改动面:cxroot.tsv 17 行加列;三源各加"解析到的包根层"一个字段与一处判定;派生与文档各一处。
- 强制点:编译期具名拒绝。
- 弱点:单靠包粒度,文件里看不出它是不是对接单元;而且包里并非每个文件都对接(ext/tsvx 只有 3 个 dock)。

### (C) 文件头 pragma 加路径白名单

- 已经存在(dock),缺的只是强制与白名单的唯一化。单用 C 的弱点就是现状:自我授权。

### 结论:B 与 C 不是二选一,是同一机制的两半

- dock 是声明(文件里可见、可 grep、粒度到文件、kind 可审计);
- cxroot.tsv 的层列是授权(哪个包根下允许出现 dock);
- 编译器是裁判(不在授权包里写 dock 即拒;无 dock 模块用受限构造即拒)。

### 档位:需要三档加脚本,不是两档

| 档 | 谁 | #include | 平台 #if、__syscall | 内联 asm | 裸 libc | 可否写 dock |
|---|---|---|---|---|---|---|
| dock | cx/sys mem text os path;ext 的 sysx net memx textx tsvx | 许(须 dock) | 许 | 许 | 许 | 可 |
| upper | cx/core runtime compiler driver stream cx0 workspace、src、ext/crypto 等纯包 | 拒 | 拒 | 拒 | 目标拒;现为棘轮债 | 不可 |
| tool | tools/cxlib 与 tools 下的模块 | 拒 | 拒 | 拒 | 许(本机跑不跨目标,沿用所有者既有口径) | 不可,个别对接口单列 |
| 脚本 | 非模块(无 module 行且无人 use;2026-11-04 已有裁定) | 自由 | 自由 | 自由 | 自由 | 不需要 |

没有 tool 这一档,tools/cxlib 的 4399 处会被当成 upper 的债,要么永远红,要么逼着人给 67 个文件乱加 dock,把 dock 的含义冲掉。

### 汇编怎么承载

- .c、.s、.S 输入完全不受 cx 层规则管:所有者说的"C99 加 POSIX 的基本需求"在这条线上兑现,不设任何层门。
- .cx 模块里的内联 asm 只在 dock 档放行,理由与平台 #if 相同:asm 天然绑定架构,放进上层就破坏六目标。建议 dock 的 kind 增加 asm(`dock asm`),让审计表能区分"为 asm 而 dock"与"为 libc 而 dock"。
- 非模块脚本写 asm 不拦(与脚本自由一致;写了就只在该架构能跑,由作者自负)。
- 现在 asm 出现次数为 0,这条规则应与内联 asm 的首个实现同一补丁落地,先有门再有用法。

## 3 推荐与步骤

立场:不做 A;做 B 加 C 的合体;分三步,每步先红后绿。与主调度初步意见的差别见第 0 节三处。

第一步(工具侧,不动产品输入,约 1 单位):唯一化定义并堵自我授权。

- cxroot.tsv 加层列(编译器当前按列解析 R 行,多一列是否被现产品拒绝须先实测;若拒,则第一步先放在一张 tests 侧表,第二步再并入 cxroot,未核)。
- dock-map 改为只认行首 pragma(修掉把 plan.cx、cxmod.cx 误判为 dock 的问题);四个棘轮的 `declares_dock` 改读 dock-map 一份结果。
- 新断言:dock 文件必须落在层为 dock 的包根下,否则红;check-dock-audit 的 `--write` 不再给授权外文件补行。
- 验收命令:`sh ./unisacc.com tools/check-dock-audit.cx`(预期 failed=0);夹具先红:在 upper 包根下放一个带 dock 的模块,预期 rc=1 且文案含包名与层;`sh ./unisacc.com tools/ratchets.cx`(预期 13 项不变);遍历消费者一致性:四个棘轮与 dock-map 对同一文件集给出的 dock 判定逐文件相等。

第二步(产品输入,三源同改,约 2 单位):编译期具名拒绝零债的四类。

- 无 dock 的模块出现 #include、平台 #if(预定义平台宏出现在条件指令里)、`__syscall`、内联 asm:拒绝,文案给出修法(加 dock 仅限对接包,否则 use sys.*)。
- dock 出现在未授权包根:拒绝。
- 先清的存量(平台 #if 与 __syscall 在无 dock 模块里):cx/driver/input.cx、output.cx 各 1,cx/cx0/plan.cx 7,cx/workspace/driver.cx 2,src/host.cx,cx/sys/hostid.cx 6 与 runlaunch.cx 1(这两个补 dock 即可,但在冻结前缀内,须等 2026-11-22 窗口过或走解除),tools/cxlib 5 个文件。
- 验收:`sh ./unisacc.com tools/cx0-extract.cx -- --check` 报 EXACT;本报告探针目录里的 main1.cx 加 upmod.cx 由 rc=0 变 rc=1 且具名;负控:同一模块加 dock 并放进授权根后通过;L0 全部模块照常编过、两代字节相同;`tools/affected.cx --files 三源 --run` 全绿。

第三步(另排,规模大,先出设计):裸 libc 名的编译期拒绝。

- 思路(推断,可行性未核):按层给不同的隐式前导,upper 档的前导不带 libc 原型,于是裸调用自然落到"隐式函数声明"这条拒绝上,正好接 0.0.86 的 D2 implicit-function 负例。前提是 upper 的约 670 处债清零(source_frame.cx 150、drive 系列约 300 为大头)。
- 在此之前裸 libc 继续由 cxraw、cxlibc 棘轮守,不假装已强制。

## 4 对 cx spec 的影响

应该写,而且应写成一张"构造乘以档位"的表(即第 2 节那张),放进 prd_cx_spec.md 3.4 取代现在的五行层表,并改掉其中已经不符事实的一句"只有 cx/sys、cx/mem、cx/text 能"。cx/spec.md 相应加条款:dock 的授权来源、四类受限构造、拒绝文案。

防上层借道滥用,有四个口子要点名:

(1) use 不传递(CX-U-5)已经挡住"use 一个 dock 模块顺带拿到 libc"。

(2) 通用逃生口 sys.dl、sys.ffi、sys.dlcall:上层只要能 dlopen 就能调任何东西。已核现有使用者只有 src/host.cx 与 tools/cxlib/toolselfpath.cx。应把这三个模块标为特权接口,upper 档 use 它们即红(可并进 check-cx-layers 的断言)。

(3) 直通包装:dock 包对外接口里"只是把 libc 改个名"的函数会把边界掏空。tlib-packages 里已有 `libc-forward` 的排除先例(cx/sys/dlcall.cx、ext/sysx/net.cx),建议把它变成 layer-api 的一条检查:dock 包新增公开函数时须带长度或截止或错误码合同之一,否则列入人工审。属判断标准,不易全机械(推断)。

(4) 接口里的宏与内联函数:若 dock 模块公开的宏展开后含 libc 名,上层单元里就出现了裸调用。三源里有宏登记(cxm_nmac),是否随接口导出我没有核到(未核);第二步验收应加一个负例覆盖它。

## 5 会不会妨碍"用 cx 打赢 bun、node、python"

按上面的档位设计,不会,理由是事实而不是愿望:

- 用户随手写的脚本是非模块,完全不受限。并且现在脚本根本不需要 #include,隐式前导已经给了 libc 原型:tools 与 tests 的 1733 个文件 #include 出现 0 次,而 1308 个文件在直接用 libc。"不能随手 #include"对脚本不是门槛,因为没人需要写它。
- 受影响的只有用户自己写的库模块(带 module 行):想 #include 要加一行 dock,并在自己的 cxroot.tsv 里给自己的包根标 dock。用户的包根归用户,这是一行声明的摩擦,换来"哪些文件碰宿主"可 grep。这是对外行为变化,发布说明须写。

真正的门槛在别处,层规则帮不上也不添乱:

(1) 仓外脚本找不到包根:prd_cx_spec.md 3.3 实测要自己建符号链接加一份 cxroot.tsv 才能 use sys.fs。这比 #include 的问题大得多,应由产品内置默认包根解决。

(2) 库面缺口:build/pkg-gaps.tsv 213 行里 169 行含 OPEN。与脚本语言对比最显眼的缺项(据目录与规范文字判断,未逐项实测):HTTP 客户端与 DNS(net.resolve 只有字面量与 localhost)、正则与通配、目录遍历与递归操作(ext/fsx 只有 fs.cx 与 path)、JSON 仅在 ext/textx 未进核心六包、use 别名在运行期不可用(规范 3.2 已记)、ext/net 不在仓根 cxroot。

(3) 内联 asm 现在全拒。

sy_、tx_ 要多完整才够:判据不是接口数,而是 tools/cxlib 那 4399 处裸调用里出现频次最高的名字能否逐个被带界接口替掉。建议把 corestat.cx 的按名字频次版登记成工具,作为 L0 补面的需求清单来源(本次未做按名字分布,未核)。

## 6 版本、估时、负责人

- 第一步进 0.0.86,约 1 单位,只动 tools 与 tests 与表,与 W1、W2、M2 无文件相交(W2 动 run 路的 unitobj、objlink 与三源;第一步不碰三源)。但 0.0.86 已估 17.4 到 20.4 单位超容量,所以列为储备项,不挤 W2、M2;若 D2 未排满则顶上。负责人建议:opus 做先红夹具(它在 0.0.86 的既有分工就是夹具),cdx 做四棘轮与 dock-map 的判定统一(它的角色是提缺口与复核,与"消除三种判法"对口);agyneon 已被 T1 T2 T3 占满,不派;agy-flow 的职责我没有核到,不建议。
- 第二步进 0.0.87,约 2 单位,与内联 asm 的首个实现绑在同一个版本、同一个三源改动窗口:两者都改 plan.cx 的指令与记号处理,分两次改就是两次 EXACT 投影与两次整链。必须排在 W2 接线合入之后(同一冲突域),并在 L0-CORE-FREEZE-049 于 2026-11-22 到期之后(hostid.cx、runlaunch.cx 要补 dock)。负责人:谁做内联 asm 前端谁做这一步,由集成者 cc 核三源补丁;不建议拆给两个人。
- 第三步不早于 0.0.89,先出设计与债务清单,规模待估(UNMEASURED)。
- 汇编基本需求的衔接:汇编报告(asm-acceleration.md)定出内联 asm 首批范围时,把"cx 模块内 asm 须 dock asm、脚本与 .c 不限"作为其验收的一条写进去;.s 与 .S 混编、gcc .o 混编属驱动与链接,与层规则无交集,不受本提案影响。

## 7 未核与推断汇总

- cxroot.tsv 多一列是否被现产品解析器接受:未核(决定第一步层列先放哪)。
- dock-map 把 plan.cx、cxmod.cx 记成 dock 的原因:据"行首无该 pragma、文件内有该字串两处"推断为文本匹配误判,未读 dock-map 判定代码全文。
- plans/cx-dock-pragma.md 规则 4 无机器检查:推断(只读了 check-dock-audit 头注与 check-cx-layers 头注)。
- 各方案单位数:推断,无实测工时。
- tests 与 tools 的平台 #if、__syscall 计数里夹具占比:未核。
- 宏是否随模块接口导出:未核。
- agy-flow 的职责、cdx 当前负载:未核。
- 我的 libc 计数用固定 113 名单,少于棘轮的 symindex 口径(例:source_frame.cx 我数 150,cxlibc 基线 198);分布结论可用,绝对值以棘轮为准。
