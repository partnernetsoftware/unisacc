# 分层与分车道:取证与立场(2026-11-20,只读,基线 c4c10e976f,分支 v0.0.86-l1)

## 0 最重要的判断

所有者的结构诊断成立,但主调度初判里有两处与实测不符,必须先改正再设计:

(1) "ext 改动拖慢了过去几版"不成立。0.0.75 到 0.0.85 十版 604 个非合并提交里,纯 ext(B 类)提交只有 1 个(ebdc15a0e7,linuxns 落库,405 行);A 类 32 个;tools tests apps(C 类)476 个。每一版都有 A 类提交(0.0.83 为 4,0.0.84 为 2,0.0.85 为 8),所以近三版没有一次整链是"只因 ext"而跑的。分车道的收益是面向未来的(0.0.87 脚本地板库、agy-cage 的原语全是 B 类),不是对历史的追偿。

(2) "L1 不需要新编译器,所以与产品身份无关"只对了一半。实测:ext/sysx/linuxns.cx 改一行,候选键变,源身份 SID 也变。原因是 src/sysx、cx/runtime/sysx、cx/driver/sysx、cx/workspace/sysx 四个目录别名指向 ext/sysx,seed/ident.c 按目录递归收集,把整个 ext/sysx 收进了 SID。A 层源码里没有任何一条 use sysx.*(已 grep 核),所以这是身份定义过宽,不是真实依赖。

(3) 真正的病灶是"产品输入"有四套互不一致的定义,见第 3 节。

## 1 取证方法与原始数

- 统计脚本:~/.claude/jobs/2798e440/tmp/lanes/lanes-stat.cx(.cx,git log --no-merges --numstat 分类)。
- 身份推演:git archive HEAD 到 ~/.claude/jobs/2798e440/tmp/lanes/snap(私有副本),宿主 cc 编 seed/ident.c 得 SID 助手,product-map-query --root snap --candidate-key 得候选键。未跑整链,未改仓内文件。

提交分布(一个提交可同时计入多类;lines 为增加加删除):

| 区间 | 提交 | A 提交/行 | B(纯 ext)提交/行 | tools/cxlib 提交 | C 提交/行 | 文档与表 D 提交 |
|---|---|---|---|---|---|---|
| 0.0.75 到 0.0.82 | 331 | 18 / 2913 | 0 / 0 | 4 | 241 / 57692 | 166 |
| 0.0.82 到 0.0.83 | 97 | 4 / 1881 | 0 / 0 | 0 | 81 / 19760 | 30 |
| 0.0.83 到 0.0.84 | 89 | 2 / 86 | 1 / 405 | 1 | 83 / 12772 | 17 |
| 0.0.84 到 0.0.85 | 87 | 8 / 297 | 0 / 0 | 2 | 71 / 10241 | 37 |
| 0.0.85 到 HEAD | 17 | 12 / 894 | 0 / 0 | 0 | 12 / 902 | 10 |

cx/path、cx/os、cx/stream、cx/workspace 在全部区间为 0 提交;被 cx/sys、cx/mem、cx/text 用符号链接引用的 ext 实体文件(下称 BS)也为 0 提交。版本间隔约 5.3 小时(标签时间 21:14、02:10、07:30、13:14)。

改一行的身份对拍(同一副本,逐项改后还原,还原后两值回到基线):

| 探针 | 候选键 | SID | 判读 |
|---|---|---|---|
| 基线 | 578de616 | 9d659fef | - |
| ext/sysx/linuxns.cx 加一行 | 变(858c96a3) | 变(dd96db76) | 两者都把它当产品输入;A 层无使用者,属身份过宽 |
| ext/crypto/sha256.cx 加一行 | 变(b211b387) | 不变 | 候选键误触发 |
| ext/net/tcp.cx 加一行 | 变(a619551b) | 不变 | 候选键误触发 |
| tools/cxlib/netpack.cx 加一行 | 不变 | 变(90c1921f) | 候选键漏触发(exec/c/cxlib 有三个符号链接指向 tools/cxlib) |

未核项:APE 字节是否随 linuxns 变化没有做实编比较(要跑构造,超出只读与时限)。推断:不变,因为 A 层无 use sysx.*,且 use 按 cxroot.tsv 读磁盘源(本次脚本放在仓外即因找不到 cx/sys/posix.cx 被拒,旁证模块源不在 APE 内);但 SID 变了,链身份与 build.json 的 sources_sha256 会变,现行规则下等同于"新候选"。

## 2 回答 (1):分层事实与路径边界

SID 的实际收集范围(seed/ident.c 第 262 到 268 行,list-current 实测 1712 个文件):seed、include、cx 全树、src、exec、weights、kernel、typekw.tsv,加 13 个 tools 支撑文件(bound、bound.c、cxbin、safedel.cx、lib.sh、build_ref.sh、export_ref.sh、refshim.h、reffoot.h、sourceflat.cx、ape-pack.cx、release-ape.cx、cxlib/releaseversion.cx),加经符号链接进入的 tools/cxlib 的 codec、netpack、pytempfile,加经别名进入的 ext。

逐目录归类(立场):

| 路径 | 归类 | 依据 |
|---|---|---|
| src、cx/core、cx/runtime、cx/compiler、cx/driver、cx/cx0、exec、seed、weights、kernel、include、typekw.tsv | A | 编译器与执行体本体 |
| cx/sys、cx/mem、cx/text 的实体文件 | A | L0,A 层直接 use(mem.bytes 29 处、sys.fs 16 处等) |
| ext/memx/arena.cx、ext/textx 的 build deflate json num、ext/sysx 的 childdepth fd guard net poll process process_group process_group_osx tty(共 14 个,BS) | 现状属 A | 是 cx/mem、cx/text、cx/sys 的符号链接目标,在 SID 内;文件放在 ext 只是历史摆放 |
| cx/stream | A | cx.compile 依赖 cx.stream,A 层 use stream.pipe、stream.item |
| cx/os、cx/path、cx/workspace | 现状在 SID(整棵 cx 被收集);A 层无 use(已 grep);语义上是 B | 建议移出 SID 或迁到 ext,见第 4 节 |
| ext/sysx 其余(linuxns、exec、processcore、reaper、run、trace) | 语义 B,现状被目录别名拖进 SID | A 层无使用者 |
| ext/crypto、ext/net、ext/tsvx、ext/testx、ext/argx、ext/fsx | 纯 B | 不在 SID,只在候选键与 product-map 读集里 |
| tools/cxlib | C 的库层(tlib 包 tools.cxlib 0.3.44),其中 codec、netpack、pytempfile、releaseversion 四个文件属 A | 被 exec/c/cxlib 链接或被 ident 强制收集 |
| tools、tests、apps、ujs、examples | C | - |

所以"ext 五个包是否都纯 B"的答案是否:ext.memx 全部、ext.textx 四分之四(fmt 已不在其列,build deflate json num 四个都是链接目标)、ext.sysx 九个文件实为 A。纯 B 的是 crypto、net、tsvx、testx、argx、fsx 与 sysx 的六个文件。

ext 是否进 APE:pkg.bin 是 P3 模型包(网与权重),PP_APE 布局下它在 APE 文件内(tools/cxlib/productpkg.cx 头注释),不是 ext 源码包;tlib 包表明写"发布链与 gen1 g2 g3 从源冷构造,从不读 tlib"。ext 源不作为源码进 APE(推断,旁证见上);BS 文件是否以编译结果进 APE 取决于隐式运行库前缀是否含它们(process_group 定义 prelude 的 setpgid,表内有注),本次未逐单元核。

## 3 回答 (3):候选键与链的分层

四套"产品输入"定义并存,互不生成:

| 定义 | 位置 | 内容 | 用途 |
|---|---|---|---|
| 候选触发键 | tools/candidate-trigger-inputs.tsv | src cx ext include exec weights seed kernel 八棵树加 typekw | 只被 tools/dvreact.cx(devsvc react)用,不是门 |
| 源身份 SID | seed/ident.c | 见第 2 节 | 链身份、pack 回执、promote --check |
| 开发车道新鲜度 | tools/dev-lane.cx | include src cx exec seed 五棵树 | 本地产品是否过期 |
| 冻结路径 | build/freeze.tsv 第五列 | 每次冻结手写 | precommit freeze_gate |

另有事实:bg-candidate 与 chain-run 以提交号 rev 为候选身份(BASE/rev,promote 要求身份指名 REV),不是按键。所以"只改 ext 触发整链"在现实里走的不是候选键,而是两条规则:起链即内容截止(产品输入变更须重起链)与 promote 绑 rev。

改法(立场:以 SID 为唯一事实源,其余由它生成):

(1) seed/ident.c 的 list-current 输出作为 A 集合的唯一事实;新生成器 tools/lane-inputs-gen.cx(带 --check)从它生成 build/lane-inputs.tsv(列:path、lane、owner_pkg、via),candidate-trigger-inputs.tsv、dev-lane 的五棵树、freeze.tsv 的路径列都改读它。
(2) product-map-query 增加 --candidate-key-a(只含 lane=A 的文件)与 --pkg-key PKG(只含该包 roots 加 deps 的 api_sha 加 include/cx-flat.h 的 sha);旧 --candidate-key 保留一版作对拍后退役。
(3) 去掉四个 sysx 目录别名(src/sysx、cx/runtime/sysx、cx/driver/sysx、cx/workspace/sysx;最近一次改动是 05424b6932),使 ext/sysx 非链接目标文件离开 SID。这是 A 类改动(别名拓扑进 SID),跟随一次正规整链落地,不单独起链。动手前须核兄弟解析是否有隐性使用者(本次只 grep 了 use sysx,未核其它解析路径)。
(4) bg-candidate 的 owner.tsv 增记 A 键;promote --check 在"rev 不同但 A 键相同且 tools 支撑文件相同"时放行并打印 REUSED_FROM rev。release/candidate.tsv 不必加列:包与 APE 的绑定仍是 package_sha256;B 类包身份另记 build/pkg-release.tsv(pkg、version、api_sha、src_sha、min_product)。
(5) chain-phases-gen 的 inputs_of 里 src:cx/** 实际含链接目标,改为引用 lane-inputs.tsv 的 A 集合,避免第五套口径。

先红后绿(登记为批次 lane-key-probes,即本报告四个探针):

| 探针 | 现状 | 目标 |
|---|---|---|
| 改 ext/crypto 一行 | A 键变(红) | A 键不变,ext.crypto 包键变 |
| 改 tools/cxlib/netpack.cx 一行 | 键不变(红,漏触发) | A 键变 |
| 改 ext/sysx/linuxns.cx 一行 | 键与 SID 都变(红) | 去别名后 A 键与 SID 都不变,ext.sysx 包键变 |
| 改 ext/sysx/fd.cx 一行(cx/sys 链接目标) | 变 | 仍变(保持) |
| 同一 A 键、不同 rev 的已完成候选 | promote 拒 | promote 放行并留 REUSED_FROM |

## 4 回答 (2)(4):轻车道定义与规则文字

轻车道适用集合:build/lane-inputs.tsv 中 lane=B 的路径。在第 3 节 (3) 落地前,只含 ext/crypto、ext/net、ext/tsvx、ext/testx、ext/argx、ext/fsx;ext/sysx 的非链接文件在去别名后加入。

最小验收(全部用已装 Root,不建候选):
(1) 包内每个改动单元对六目标 -c 实编(现有 check 已对 win_x86_64、osx_x86_64 做);
(2) tools/layer-api.cx --write 得 bump 类别(加法升 Y,破坏升 X),build/tlib-packages.tsv 版本与 api_sha 同步;
(3) 该包自己的 check 与 affected 选出的批次全绿(实测:改 linuxns 选 7 批,改 crypto/sha256 选 11 批,对比整套预跑 84 分钟);
(4) 包键写入 build/pkg-release.tsv;
(5) 兼容检查:用上一公开 APE 再编一遍(见第 6 节)。
目标时长:改到可用不超过 15 分钟,无整链、无预跑、无 CI。证据级别如实标 LOCAL lnx 实跑,其余目标 COMPILED 未实跑。

交付形态(立场):不做 submodule,不单独发 tar。use 读磁盘源,下游(agy-cage 的 sandbox-cx)直接跟 main 的提交号即可;包级"发布"就是 main 上一行 build/pkg-release.tsv 加一个轻量 tag pkg/ext.sysx/0.1.9。等仓外 use 解析(0.0.87 已列)落地后,再让 cxroot.tsv 支持 R 行指向仓外目录与 pin 的包键。现在先做 tar 发布是过度设计。

CLAUDE.md 建议改写稿(供所有者批准,未改文件):

> 分车道(项目所有者 待批):产品输入按 build/lane-inputs.tsv 分两类。A 类(编译器与核心层,即源身份 SID 收集到的全部文件)只上版本分支 v0.0.NN,冻结时合入 main,起链即内容截止。B 类(扩展层库包,不在 SID 内的 ext 路径与今后迁入的库包)可直接提交 main,条件是:过包门(六目标实编、layer-api 版本与 api_sha 同步、affected 选批全绿、上一公开产品兼容实编),只提交自己的路径,在 build/pkg-release.tsv 记一行。B 类提交不改变候选身份,不触发整链与预跑,冻结窗口内照常允许。一个提交不得同时含 A 类与 B 类路径。B 类单元不得被 A 类路径 use 或符号链接引用,由 check-lane-boundary 守。
>
> "产品输入变更先出补丁给集成者核"改为:"A 类产品输入变更先出补丁给集成者核;B 类过包门后由作者直接提交"。
>
> "版本分支"条首句后加:"B 类库包改动是本条的第二个例外(第一个是 devsvc),在共享检出的 main 上改与提交。"

## 5 回答 (5):与 A 类提速的叠加关系

- 推测链 T3 与预跑发车:第 3 节 (4) 的"A 键相同即复用候选"是它们的前提放大器。现在 promote 绑 rev,链起之后任何提交(哪怕 C 类或文档)都使 rev 不同;按 A 键复用后,近三版 273 个提交里只有 14 个会使候选失效。这一条的收益大于整个 B 车道。
- 整链内容寻址增量:相位输入改读 lane-inputs.tsv 后,B 类与 C 类变化天然不使任何相位失效。
- 周节奏:B 车道不占版本槽,A 类可以拉长节奏而不饿死库迭代。
- 不叠加的部分:预跑 84 分钟主要由 C 类批次量决定,B 车道不减它。

## 6 回答 (8):隐性耦合与拦截

已有实证的耦合:0.0.83 预跑 12 格 netregen 失败,原因是 include 头小修经 cx-flat.h 进入 .cx 单元,改了模块镜像(prd_mempalace.md 第 149 行)。即 A 的 include 变化会改变 B 的编译结果。

拦截:
(1) 包键含 include/cx-flat.h 的 sha 与 deps 的 api_sha;A 类合入 main 时所有包键失效,包门在版本预跑里自动重跑(本来就在全批里)。
(2) 兼容契约放旁表 build/pkg-compat.tsv(pkg、version、min_product、features),不改 tlib-packages.tsv 的列。包门用"已装 Root"与"上一公开 APE"各实编一遍;只在当前 Root 通过的,min_product 写当前版本,下游据此拒绝旧产品。多版本 CI 矩阵暂不做,两点对拍足够,等出现第一个真实不兼容再扩。
(3) check-lane-boundary:A 集合内不得有指向 B 路径的符号链接或目录别名;A 路径不得 use B 包;B 提交不得带 A 路径。
(4) lane-key-probes 批次守第 3 节五个探针,防口径再次漂移。
(5) 最大的残余风险:B 直接进 main 后,main 在两次发布之间含未经六目标实跑的库代码。对策只有如实标级别,不宣称 QUALIFIED;下一次版本 CI 才把它升级。

## 7 回答 (6):应用层与 ext 的边界

立场:ext 不要"合并给应用层"。按所有权分三档:
- 进 A 的:14 个 BS 文件应把实体搬回 cx/sys、cx/mem、cx/text(现在是 cx 链到 ext,方向反了),ext 里留反向链接或删除。A 类改动,随一次正规整链。
- 留 B 的平台与协议库:sysx(进程、命名空间)、net、crypto、tsvx,由 unisacc 侧持有,因为它们对接系统调用号与六目标差异,应用车道无力维护。
- 归应用的:只被单一应用使用的库(判据:tests/tools-compile-sources.tsv 与 use 关系里使用者只在一个 apps 子目录)。目前未发现此类 ext 单元(未逐包核)。
注册表就用现有 build/tlib-packages.tsv 加 pkg-release.tsv,语义化版本规则表里已有(X 破坏、Y 加法、Z 实现)。对 ujs 与 apps/csih:它们是别的车道,B 车道使它们可以按 main 提交号取库而不等版本;tools 受益最小,因为 tools 本来就直接进 main。

## 8 回答 (7):版本编排与 L1 试点

| 项 | 版本 | 估时(单位) | 负责人 | 类别 |
|---|---|---|---|---|
| P1 lane-inputs 生成器加 lane-key-probes 批次(先红) | 0.0.86 槽前工具线 | 1 | agyneon | C,直进 main |
| P2 包门工具 tools/pkg-gate.cx(实编、layer-api、affected、兼容对拍、记 pkg-release) | 0.0.86 | 1.5 | agy-flow | C |
| P3 CLAUDE.md 条文改写与 freeze_gate 改读 lane-inputs | 0.0.86 所有者批准后 | 0.5 | cc 主调度 | 文档加 C |
| P4 promote 按 A 键复用 | 0.0.87 | 2 | cdx | C,配合 T3 |
| P5 去四个 sysx 别名,BS 实体搬回 cx | 0.0.87 A 线,随整链 | 1.5 | agyneon | A |
| P6 设计复核与探针口径审查 | 0.0.86 | 0.5 | opus | 只读 |

合计约 7 单位,其中 0.0.86 占 3.5。与 0.0.86 现有任务:W2、D2、T1 到 T3 不受影响;P4 是 T3 的直接前置,应并入 T3 的验收。0.0.87 的脚本地板库是 B 车道的第一批真实负载,P1 P2 必须在它之前到位;plans/v0.0.87.md 在本工作树不存在(未核其它分支)。

L1 是否立刻走轻车道:不能照搬。linuxns.cx 现在在 SID 内(实测),按现行口径它是产品输入。立场与步骤:
(1) L1 的验收按轻车道做:check-ext-sysx-linuxns 全绿、affected 的 7 批全绿、layer-api 记 ext.sysx 0.1.8 到 0.1.9 加法升级(工作区已有 build/layer-api/ext.sysx.tsv、iface-hash.tsv、tlib-packages.tsv 三个未暂存改动,需一并提交)。
(2) L1 的落点仍是 v0.0.86 版本分支,在 0.0.86 起链之前合入。0.0.86 已有 12 个 A 类提交,整链反正要跑,L1 不增加任何整链或预跑代价。
(3) 若 L1 赶不上起链:不为它重起链,推到 0.0.87,agy-cage 先从 v0.0.86-l1 分支的提交号取源(use 读磁盘,功能上立即可用)。
(4) L1 作为试点记录两个数:改到包门绿的分钟数、包门选批数,作为 B 车道的基线。
(5) 不建议现在把 L1 直接提交 main:那会在 P5 之前制造一个"main 的 SID 与最近公开候选不同"的状态,并破例于尚未批准的条文。

## 9 未核与限制

- APE 字节对 ext 改动的不变性只有推断,无实编对比。
- BS 文件是否进隐式运行库前缀未逐单元核。
- 四个 sysx 目录别名是否有非 use 形式的使用者未核。
- "整链 27.8 分钟、预跑 84 分钟"取自任务简报,本次未复测。
- 0.0.82 到 0.0.85 的整链重起次数未从 speed-ledger 取数;本报告只证明每版都有 A 类提交,因此首次整链不可归因于 ext。
- 统计把合并提交排除;分类以路径前缀为准,BS 名单取当前符号链接状态,历史上名单可能不同。
