# 本机快速加载器(local 车道):原始设计还原、现状差距与 0.0.69 起的落地方案

日期 2026-11-16,只读分析加测量,未改任何仓内文件(本文件除外)。本文件是派工指定的落点,主调度吸收后可归档。
口径:带"实测"的数字来自本轮命令或所引表行;带"估计"或"推算"的没有跑过;没有数据写 UNMEASURED;查不到证据写 UNVERIFIED。
本轮全部测量在 load1 10.7 至 19.8 下取得(他人作业在跑),一律 noisy,只作量级依据,不作基线,不写入 build/speed-baseline.tsv。

## 1 结论

(1) "unisacc.local" 这个字面名在仓内与全部历史里不存在(HEAD 上 git grep 无命中;git log --all -S 无命中,7.1 秒)。它对应三条真实存在、互相衔接的线:lab-loader 的 boot 加 manifest 合同(2026-11-03)、test-loader 显式绑定切片与模型包(2026-11-05 至 11-07)、本地重建产品 unisacc-local-0060 与启动器 ~/bin/unisacc-local(2026-11-13)。其中 0060 的 B 路线(routeB/unisacc-local-b.sh:已装切片加只换两张 E2 网的包)就是"快速 loader 加载快速迭代的切片与模型"的原型。
(2) 没有落成日常车道的原因不是被否决,而是三处断点:实验线被产品子命令 stream STAGE 取代后归档;test-loader 要五个绝对路径绑定加材料,实测"15 分钟窗口内未搭起";local-0060 按设计只用到 0.0.61 正规安装,之后产物已不在。没有任何一处把"改表 -> 重建受影响的网 -> 换包 -> 真实执行"做成一条命令。
(3) 机制本身今天仍然成立且本轮实测闭环:从工作树重建 errorparse 网,换进已装 0.0.67 的包,得到的包与已装包逐字节相同(sha256 c7cb3a8c 开头,1769476 字节);经真实切片编最小程序,镜像与已装产品相同(177ea4cb 开头)。换包 0.26 秒,真实编译 0.12 秒。
(4) 瓶颈在构造器而不在加载器:parse2 一张网的 gen 用产品默认档编出的构造器超过 55 秒未完成,-O2 档 38.7 秒,宿主 gcc -O2 为 10.4 秒;小网(prune、opt)产品编构造器 1 至 1.5 秒。sim 在 parse2 上每次启动约 7 秒(装载 76 MB 的 delta),prune 上 0.64 秒。
(5) 设计:devsvc 下的 local 会话。会话 = 一个基座切片加一个私有包,按 manifest 记录每个成员的来源与摘要;工作树来的成员标 local(记录而不信任);写事件经 net-deps 选出受影响的网,只重建并换入这些网。产物带 NOT_RELEASE 标记,结果一律标 LOCAL,不记 PASS,不进任何资格。
(6) 收益(估计):改 prune 或 opt 表到真实执行约 3 秒,lower 或 enc 约 12 秒,parse2 约 50 至 60 秒(现状没有一条命令的路径;历史数据点为约 8 秒、21.9 秒、15 分钟未搭起、约 2 小时)。
(7) 对三个版本的帮助不均:0.0.70 覆盖缺口与 0.0.71 生成码一期是改网表,第一期直接受益;0.0.69 的 X1 全在 src/runcache.cx、R1 的判定落在驱动读表处,都要换基座切片,第一期帮不上,要第二期(本机基座增量构造)。派工说明里"三项都是改事实表、网、编码表"对 X1 不成立,此处更正。
(8) 第一期只做:tools/local.cx(open、reload、run、status、close)、manifest 与 NOT_RELEASE 标记、flow 节点 local.run、tools/check-local.cx 与批次。工具线,在 main 上,不占版本槽。

## 2 A:原始设计还原

### 2.1 三条线与证据

| 线 | 时间 | 内容 | 证据入口 | 终态 |
|---|---|---|---|---|
| 外置与内容寻址(架构方向) | 2026-11-02 项目所有者定 | manifest.tsv 六列;来源顺序 UNISA_HOME -> com 旁目录 -> 用户缓存 -> 内嵌包;命中必校验 sha256,不符具名拒绝,不静默回退;产品身份 = manifest 的 sha256;"单文件发布保留,外部目录形态用于开发";分步 B0 explode/implode -> B1 resolver -> B2 阶段测试台 -> B3 基座外置 -> B4 booter 头 -> B5 身份收敛 | prd.md 4.3 节 | B3、B4、B5c 为 DONE-PARTIAL;发布形态仍是单文件 APE;落地的是 resolver、p3-explode 与 stream STAGE |
| lab-loader 实验线 L1 至 L7 加 p3-swap | 2026-11-03 至 11-05 | boot 头加原生段加 manifest;单 NN 模板(换权重不换基座);网等于表的穷举核对;P3/P4 单模型替换 | tools/lab-loader/CONTRACT.md、README.md、experiments.tsv;提交 2e95b6ddcd、115033b45b、0f903dc99c | L1、L2、L7 于 2026-11-05 归档(a3d290657a),被 stream STAGE 取代(c413b7a1d0、a74d4bf047);LOADER-GRAD 按 stream 路线验收:改一行表到 e3 结论双向 21.9 与 21.8 秒(3bd18f6364);L3、L5、p3-swap、net-check 保留在用 |
| test-loader(TEST-LOADER-COM) | 2026-11-05 至 11-07,项目所有者令 0.0.46 立项 | 显式绑定 --slice 与 --models;--net 单网替换复用 p3-swap;逐模型身份收据;开发证据不是发布资格 | plans/test-loader-com.md;提交 3475daee75、ae98779c6c、ad34c04063、a5a7d9c4ae(DONE-LOCAL) | 入库在用,六个 loader 批次;覆盖 M0 至 M39 共 305 断言(c46-2,开发证据) |
| 本地重建 0.0.60(LOCAL-REBUILD) | 2026-11-13 | 以已装 0.0.59 为构造器跑 candidate-chain 到 gen1-lnx-x86_64,用 release-ape dev-lnx 打成 lnx-only dev 形态;启动头 unisacc-local-0060.com;持久位置 ~/.cache/unisacc/local-0060;启动器 ~/bin/unisacc-local;B 路线只换两张 E2 网 | 提交 f4a1b7d634、806119927e;plans/speed-hotspots-2026-11-02.md"本地重建 0.0.60"一节;archive/work/ledger-202611.tsv 的 LOCAL-REBUILD、LOCAL-0060-USE、LOCAL-0060-DEFAULT 三行;build/speed-baseline.tsv 中 note 以 LOCAL-REBUILD 开头的行 | 四批合计 939 秒;冷编译 -31.6%、8MB 排序 3.77 倍、首次启动 -13.4%;install-ape 拒绝(no-UNIPKG1-footer),未安装;本轮 ls 两处路径均已不存在 |

### 2.2 loader 合同(抄自 tools/lab-loader/CONTRACT.md,未改)

- boot 四步,顺序固定:(1) 识别宿主,只认自己编译目标支持的 os 与 arch;(2) 读同目录 manifest.tsv,按行的 kind 与 arch 选成员;(3) 在 boot 自己的原生代码里算 sha256 并与该行比对,不信任宿主外部工具,不符具名拒绝且不 exec;(4) exec 选中的成员,argv 原样透传。boot 不解析成员内部格式、不选阶段、不做路由、不做组合。
- manifest 字段:name、kind、arch、sha256、size、order;TAB 分隔,LF 结尾,井号开头为注释;order 从 0 连续,表达执行顺序。
- kind 列表:base(通用执行器,每宿主恰一行)、stage(NN 阶段权重,arch 为 any,可 N 行)、stage-classic(经典阶段可执行,逻辑与底座同体)、resource(具名资源,如头文件与事实表)。
- 拒绝口径七条:宿主不支持、manifest 缺失、无 base 行、base 行重复、成员缺失、sha256 不符、order 不连续或重复;一律非零退出加 stderr 一行具名信息。
- 实测约束:P2 只收明文网,P3 与 P4 只收 UNINETB1 二进制网;EOF 必须是终止动作;执行器切片与宿主绑定,换模型不换基座成立,换宿主必须换 base。

### 2.3 名词

- 切片有三个含义,原设计里指前两个。(a) ISA 切片:每目标一份原生机器码(L1 base 六片),test-loader 的 --slice 即本机这一片;已装产品解包后的本机片本轮见于 ~/.cache/unisacc/unisacc-c3ee5fb011428c61(2157694 字节,sha256 c3ee5fb0 开头)。(b) 单流切片:lab-loader L1 的 slice.cx 把七个流各做成单流产物并去重(已归档)。(c) 测试分片(shard)与此无关。原设计里没有"源切片"或"编译单元切片"的用法。
- 模型:包内的 M 记录(kind N 执行单元),P4 包现有 40 个(32 个流模型加 8 个公共);它们由 42 个构造网配方(tools/source-package.cx 的 recipes)经 gen -> tbl -> net 产出。另有 18 张 gold 网走 weights/gold -> kernel 的独立轨,编在基座里,不在包的 M 记录里。
- unisacc.local 的原意(按证据归纳):本机开发用;不经签名 APE、不经整链;基座用已装或本地 gen1 切片,模型与资源从外部包或工作树取;只在 lnx/x86_64 实跑;NOT_RELEASE,非 QUALIFIED。

### 2.4 为什么没有落成

| 断点 | 证据 | 性质 |
|---|---|---|
| 实验线被 stream STAGE 取代并归档 | README"已归档"段;a3d290657a;prd.md 4.3 节现状句 | 路线变更,非否决 |
| 外置形态的启动成本 | experiments.tsv:boot 校验 1.18 MB 净开销 139 毫秒(纯 cx sha),改 AF_ALG 后约 16 毫秒 | 已缓解,未继续 |
| test-loader 使用门槛 | speed-hotspots 第 531 至 532 行:"绑定 5 个 TEST_LOADER 路径,15 分钟窗口内未搭起";loader6 等材料在 s9 daily 无生产者(prd.md 候选池 S 的 F4) | 可用性缺口 |
| 增量构网工具不收任意改动 | speed-hotspots 12b-2:net-incremental-rehearsal 的 mutate 是固定场景;a12-cycle 默认构造目录 out/c47-main-a8 已不存在(本轮 ls) | 工具陈旧 |
| local-0060 是过渡物 | 账本 LOCAL-0060-DEFAULT:"0.0.61 候选链产出完整 APE 并经全量验证后,用 install-ape 正规安装" | 按计划退场 |
| local-0060 产物与启动器的删除记录 | 未找到 | UNVERIFIED |
| 候选池 D(gen1 单目标开发候选,推算 2 至 3 分钟一轮) | prd.md 候选池加速方案 D,仅推算 | 是否试过 UNVERIFIED |

## 3 B:现状差距

### 3.1 已有基线(引用,未重测)

| 项 | 数值 | 出处 |
|---|---|---|
| 冷编译(三个基线工具) | 8081、9606、12746 毫秒(0.0.59);0.0.67 上 ratchets 5120、edit-span 2726 毫秒 | speed-baseline 第 6 至 8、184、185 行 |
| 运行缓存命中后重复启动 | 28.3 毫秒(中位);0.0.67 为 29.6 | speed-baseline 第 3、183 行 |
| 首次启动 first | 0.0.67 为 717.8 毫秒(负载偏高,不下结论);0.0.66 为 665.1 | 第 176、182 行 |
| 131 作业构网波 | 276.9 秒(0.0.49);42 网全键 net-check 7.2 秒 | phase-times 第 48、52 行 |
| routes 加 p3 | routes 29.7 至 40.5 秒;p3 域合计 1065.3 秒(48 次尝试) | phase-times 第 49、203、218 行 |
| gen1 lnx/x86_64 | 对象 49.9 至 54.8 秒,链接 24.7 秒 | speed-hotspots 本地重建节;phase-times 第 241 行 |
| 整链 | 3538 至 4625 秒 | phase-times 第 209、211、228 行 |
| 本地重建到 dev 形态 | 四批 939 秒 | speed-hotspots 本地重建节 |
| 快车道目标与首例 | 目标改到探针绿不超过 1 小时;首例约 2 小时 | plans/dev-methodology-2026-10-28.md 第 1、8 节 |
| 改一行表到 e3 结论 | 21.9 秒(stream 路线);prd 能力速查记"改表 -> p3-swap -> --models 实测约 8 秒" | 3bd18f6364;prd.md 第 235 行 |
| 单网构造(宿主 gcc -O2 构造器) | 每张 pp 网 gen 约 1.0、tbl 约 0.5、net 约 0.03 秒 | speed-hotspots 12b-2 |

### 3.2 本轮实测(2026-11-16,已装 0.0.67 APE a99ea71d,共享检出 main;load1 10.7 至 19.8,noisy;各 1 次)

| 步 | 产品默认档编的构造器 | 产品 -O2 编 | 宿主 gcc -O2 编 | 备注 |
|---|---|---|---|---|
| 编 seed/gen.c | 17.4 秒 | 35.8 秒 | 11.4 秒 | tbl.c 2.7 秒、net.c 1.8 秒(默认档) |
| gen parse2 --errors | 超 55 秒被截止(rc 124) | 38.7 秒 | 10.4 秒 | 输出 76199800 字节,-O2 与 gcc 两份逐字节相同 |
| gen pp --shared-predefines | 6.6 秒 | UNMEASURED | 1.1 秒 | 输出逐字节相同 |
| gen lower --full | 8.4 秒 | UNMEASURED | 1.3 秒 | 同上 |
| gen enc --elf | 4.6 秒 | UNMEASURED | 0.72 秒 | 同上 |
| gen opt --o2 | 1.5 秒 | UNMEASURED | 0.22 秒 | 同上 |
| gen prune | 0.99 秒 | UNMEASURED | 0.19 秒 | 同上 |
| tbl parse2 | 8.5 秒 | UNMEASURED | 1.3 秒 | prune 的 tbl 0.59 秒(默认档) |
| net parse2 | 0.66 秒 | UNMEASURED | 0.06 秒 | 两份 .net 逐字节相同;prune 的 net 0.035 秒 |

| 步 | 实测 | 备注 |
|---|---|---|
| net-deps --touch(选受影响的网) | 0.65 至 0.71 秒 | exec/prune/ 影响 1 张;exec/parse2/constexpr-result.tsv 影响 3 张(shared-e3、warnparse、errorparse);exec/facts/limits.tsv 影响 0 张 |
| sim 启动加最小程序(parse2 delta,76 MB) | 首次 14.6 秒(含冷编 sim.cx),再次 7.0 秒 | 覆盖 1907 个(状态,观测);装载与执行分相 UNMEASURED |
| sim 启动(prune delta,5.8 MB) | 0.64、0.66 秒 | 输入是畸形 tape,只量到启动 |
| p3-swap 把 errorparse 网换进已装包 | 0.26 秒 | 模型序号 4;输出正文 1769460 字节 |
| 换出的包对已装包 | 逐字节相同,sha256 c7cb3a8c3a6d8877 开头 | 说明工作树表、产品编的构造器、dz_deflate 三者与已装包一致;该前缀与 speed-baseline 记的 local-0060 包相同,原因 UNVERIFIED |
| 真实切片加已装包编最小 C 程序(UNISACC_RUN_CACHE=0) | 0.12 秒 | 经 test-loader,不带 --identity |
| 同上,带单网替换与 --identity | 3.8 秒 | 身份收据用软件 sha 过 7 MB 多次,是主要开销;镜像与不替换时相同(177ea4cb 开头),运行 rc 3 正确 |
| 编 p3-swap、test-loader | 3.6、12.2 秒 | 一次性 |

取数命令(全部经 devsvc x --par 1,产物在 /tmp,测后已删):`sh ./unisacc.com seed/gen.c -o GEN`;`GEN parse2 OUT.json --errors`;`TBL OUT.json OUT.tbl`;`NET OUT.tbl OUT.net`;`P3SWAP ROOT/unisacc.com 4 BODY OUT.net`;`TESTLOADER --slice SLICE --models ROOT/unisacc.com [--net e3=OUT.net --swap-tool P3SWAP] -- t.c -o IMG`;`sh ./unisacc.com tools/sim.cx -- OUT.json t.e1.raw t.c ROOT --resource cli/target=F --cov COV`。

### 3.3 差距

- 现在改一个网表要试到真实执行,没有一条命令:要自己编三件构造器、查配方与模型序号、跑 gen/tbl/net、编 p3-swap 与 test-loader、找切片路径、设 UNISACC_RUN_CACHE=0。各件都在,拼装靠人。
- 快车道现实做法退化成"只在 sim 上证,产品运行值记 UNVERIFIED":提交 060447de54(常量表达式两入口)与 0974328548(units 网花括号诊断)的说明都写了"产品网未重建,新写法的运行值未经产品实跑",后者还写了"parse2 一级超 58 秒未跑完"。
- 改驱动或运行库源码(src、cx 下)要换基座切片:现状是整片 entry-build(本轮见他人对 v0.0.69 工作树以 290 秒上限在跑)或本地重建 939 秒;增量(只编改动单元加链接)的耗时 UNMEASURED。
- 构造器自身受生成码质量拖累:同一 gen,产品 -O2 档是 gcc -O2 的约 3.7 倍,默认档更慢。这是 local 车道与 0.0.71 生成码一期的直接交点。

### 3.4 仍 UNMEASURED 与建议取数(load1 低于 2 的静机窗口,各 5 次取最小与中位)

- 上表全部重测一遍;产品 -O2 档构造器对 pp、lower、enc、opt、prune 的 gen 与 tbl。
- sim 启动分相(读 JSON、建索引、执行):在 /tmp 放一个 sim 副本加打点,或给 sim.cx 加 --time 开关后量。
- 基座增量:`sh ./unisacc.com tools/entry-build.cx -- ROOT P3 OUT --target lnx/x86_64 --only KEY` 后接同输入 `--link-only`,量改一个单元的对象加链接。
- 一次 reload 重建 3 张 parse2 网并行时的墙钟与是否贴 55 秒线。
- 换包后运行缓存不命中的实证(RR_TIME 的 lookup_hit 为 0)。

## 4 C:设计

### 4.1 会话与 manifest

- 会话 = 基座切片一份加私有包一份,放在 sy_tmpdir 下的会话目录(形如 unisacc-local-WTHASH-PID),不在仓内,不在 out/。一个工作树至多一个活动会话,由 devsvc 常驻实例持有,带 ttl 与空闲自退。
- 会话 manifest(local-manifest.tsv):首行固定 `NOT_RELEASE<TAB>local-loader-v1`(沿用 dev-lnx-only 回执的首行约定);其后每成员一行,前六列与 CONTRACT 第 2 节完全相同(name、kind、arch、sha256、size、order),追加两列:origin(installed、worktree、built)与 trust(verified、local)。
  - origin=installed 的成员(从已装 APE 取的切片、未改动的模型与资源)照原合同校验 sha,不符具名拒绝,trust=verified。
  - origin=worktree 或 built 的成员(刚由工作树表生成的网、工作树里的头文件、本机构造的基座)只在装入时记录 sha,不与任何期望值比对,trust=local。这就是"记录而不信任"。
  - 只要有一行 trust=local,整个会话的 scope 就是 LOCAL;全部 verified 时 scope 为 LOCAL-BASELINE(仍是 NOT_RELEASE,只用于对拍)。
- kind 沿用四种,不新增。第一期只用 base 与 stage;resource 第三期;stage-classic 不用。
- 配方到模型序号的映射不写死:会话打开时,对受影响的配方先用"已装版本对应的干净树"(按已装产品 rev 用 git archive 解到会话目录,只读)构造基线网,压缩后与包内 M 记录逐字节比对,相同者即该配方的序号。本轮已实证 errorparse 对应序号 4 且逐字节相同。比不上的配方具名拒绝(REFUSE recipe-not-located),不猜。

### 4.2 装载与执行(第一期不改产品)

- 执行沿用 test-loader 已证的路径:私有包 = 已装包正文按序号换入新 M 记录,补 UNIPKG1 尾;以本机切片加 UNISA_CONTAINER 与 --models 运行。产品代码不动,不引入第二套解析与执行。
- 每次 reload 写一个新包文件再原子改名,所以包文件的 inode 与时间必变;运行缓存的产品身份(cx/runtime/rcident.cx 的 rc_hex_ident:大小、设备、inode、mtime、ctime 的摘要)随之变化,旧条目不会命中。会话另设私有缓存目录(UNISACC_E3CACHE 指会话目录内),local 条目不进共享缓存。
- 每次 run 在 stderr 打一行:`LOCAL_RUN<TAB>scope=LOCAL<TAB>manifest=SHA<TAB>base=SHA<TAB>pkg=SHA<TAB>local_members=N<TAB>rc=N<TAB>ms=N`。

### 4.3 热重载

- 入口:`local reload [PATH...]`;不给路径时取"已装 rev 的干净树"与工作树(含未提交)的差异路径。
- 步骤:(1) net-deps --touch 得受影响配方;默认只取本机目标相关的配方(shared 各张、lnx-x86_64 的 lower 与 elf、object 的 x86_64 两张、公共模型),其余目标的配方记 NOT_BUILT,加 --targets all 才建;(2) 每张:gen -> table-lint(有 ERROR 即停,不构网)-> tbl -> net -> 转 UNINETB1;(3) 换入包;(4) 更新 manifest 并发事件 local.reloaded。
- 每步各自有界(不超过 55 秒),多张网并行,并发上限 2(机器级准入 P2 落地后改取令牌)。超界的那张记 DEFERRED 并具名,不延长时限。
- 构造器(gen、tbl、net)与 p3-swap、test-loader 的执行体走 devsvc 原生缓存,键含 seed 源内容;构造器用 -O2 档编(本轮 parse2 的 gen 默认档超 55 秒,-O2 为 38.7 秒)。宿主 gcc 只作测量对照臂,不作默认路径,不新增依赖。
- 生成物缓存:某配方的 .net 可按"配方名加输入闭包内容 sha 加构造器 sha"缓存(与 flow 节点 cache 列同一原则:只缓存生成物与输入身份);命中仍核 .net 的 sha。gen 产出的 delta JSON 同时保留给 sim 与 table-lint 用,不重复生成。
- 影响面分级:

| 改动落点 | 重载动作 | 期 | 量级 |
|---|---|---|---|
| exec 下某网的表 | 重建受影响配方,换 M 记录 | 一 | 小网约 3 秒,lower 与 enc 约 12 秒,parse2 约 50 至 60 秒(估计,由 3.2 节分项相加) |
| exec/facts 事实表 | 先跑对应生成器的 --write 到会话覆盖层再同上;生成器 --check 不过即停 | 一 | 同上加生成器耗时(UNMEASURED) |
| include 下头文件(包内 hdr 资源) | 换包内资源记录 | 三 | 需给 p3-swap 补资源替换,UNMEASURED |
| src、cx 下产品源 | 本机基座:entry-build --only 改动单元加 --link-only,新 entry.elf 作 base(trust=local) | 二 | 链接 24.7 秒加改动单元对象(UNMEASURED) |
| weights/gold(18 张 gold 网,进 kernel) | kernel-route 重出后同上重链 | 三 | UNMEASURED |
| seed 下构造器源 | 重编构造器(一次 17 至 36 秒),全部配方缓存失效 | 一 | 见 3.2 节 |

- 基座陈旧保护:若差异路径触及基座输入(src、cx、exec/c、kernel、weights/gold)而会话的 base 仍是已装切片,reload 打 BASE_STALE,其后每条 LOCAL_RUN 的 scope 改为 LOCAL-PARTIAL;第一期不提供绕过开关,第二期由本机基座解决。

### 4.4 与 X1、N2 的关系

- X1(运行缓存键归一,0.0.69):互补不重叠。X1 去掉键里的当前目录与源路径后,产品身份成为区分"已装"与"local 包"的唯一维度,所以 local 车道的两条负控(换包后同源必须不命中;local 条目不进共享缓存)同时是 X1 的回归用例,建议并入 X1 验收。X1 本身改的是 src/runcache.cx,要靠第二期的本机基座来快速试。
- N2(写事件预编译):共用触发器。同一条 f 写事件,若路径是 .cx 工具或库,走 N2 的 native_get 预构造;若路径在某配方的输入闭包里且该工作树有活动会话,走 local reload。去抖(2 秒内再次写入取消未开始的旧作业)、并发上限、作业机制(x --bg 同款)、事件只记 PREPARED 性质,四项与 N2 完全一致。N2 还负责在 seed 源变化时预热构造器执行体。
- N1(命中路瘦身):local run 每次都经 devsvc,固定开销约 400 毫秒会直接叠在 0.12 秒的真实执行上,N1 应先于或同期落地。

### 4.5 与 devsvc 的集成

- 子命令(调用者只表达意图,不碰锁、路径与序号):`devsvc local open [--cwd WORKTREE]`、`local reload [PATH...] [--targets all]`、`local run -- PRODUCT_ARGS...`、`local status`、`local close`。open 自己定位已装 APE、取本机切片、建会话目录;切片取法以产品启动器的实际解包规则为准,实施第 0 步先读 seed/ape.c 与 ape-pack 的启动器文本确认(本轮只观察到解包路径,规则 UNVERIFIED)。
- flow 节点:local.reload(输入为表路径集合,输出为会话包;生成物可缓存)与 local.run(字段 session、argv、bound_s、expect;永不缓存,每次实跑;输出行带 scope)。写事件触发的自动 reload 是第三期,第一期由调用者显式 reload。
- 服务守铁律:会话有 ttl 与空闲自退;不缓存任何 run 的结论;会话目录由服务按作业清理;服务从已提交的固定版本运行。
- 写域归属:devsvc 源码(tools/devsvc 系、tools/cxlib/flow 系)只由主调度及其子代理改;tools/local.cx 与 check 同属工具线,在 main 上开发。

### 4.6 sim 与 loader 的分工

| | sim | local loader |
|---|---|---|
| 输入 | 构网前的 delta JSON(带状态名) | 构网后的二进制网,在包里 |
| 执行者 | tools/sim.cx(执行器语义的 .cx 移植) | 产品的真实切片(汇编核) |
| 范围 | 单个阶段 | 整条七流加驱动,出真实产物 |
| 回答的问题 | 哪个状态哪一行处理了这个字节;在哪里被拒;两次运行覆盖差集 | 真实产物字节、退出码、诊断文本、耗时是否如预期 |
| 何时用 | 定位表行、构网前 | 定位之后确认行为;对拍镜像与诊断 |
| 结论效力 | 诊断,不授 PASS | LOCAL,不授 PASS |

顺序固定为 gen -> table-lint -> sim(可选)-> tbl、net -> 换包 -> 真实执行;sim 与 loader 共用同一份 delta JSON。sim 在 parse2 上 7 秒的启动先分相取数,再决定是否由会话持有已解析的 delta(这是工具车道的常驻,不是产品常驻;持有的是按内容 sha 标识的输入,不是结论)。

### 4.7 与六目标和铁律

- 只在 lnx/x86_64 实跑。其余五目标的网可以构造、可以交叉出镜像,但不执行,报告里写 NOT_CLAIMED(与 loader-isa-start 的口径一致)。
- 不可发布:会话目录首行 NOT_RELEASE;包不落仓内;导出环境变量 UNISACC_LOCAL=1 给子进程。chain-ready 把 UNISACC_LOCAL 加入残留环境拒绝清单(与候选池 N 同一处);install-ape 对会话包与 dev 形态的拒绝写成 check-local 的实跑断言。release 链的 pack 只取链自己检查点里记了 sha 的包(pack-inputs.tsv),会话包没有链回执,进不去;这一条在 check-local 里以负控实证,不靠推断。
- 不缓存测试结论:local.run 永不缓存;缓存的只有构造物(构造器执行体、.net、基线树)。
- 不放宽时限:每步不超过 55 秒;超界记 DEFERRED。
- 报告标注:local 车道只产生 LOCAL_RUN、LOCAL_RELOAD 行与 LOCAL_OK、LOCAL_FAIL 汇总,不出现 PASS 字样;run-batch 与聚合器遇到候选身份以 local: 开头的记录必须拒绝(第一期 local.run 不经 run-batch,这条先写成 check-local 的负控)。提交说明与发布说明里,"本机通过"写作"local 车道实跑(LOCAL,非候选)",与"候选通过"分列。快车道的绿不授 PASS,红可以阻止合入(沿用 dev-methodology 第 1 节)。

## 5 D:收益估计

| 场景 | 现状 X | local 车道 Y(估计,由 3.2 节 noisy 分项相加) | 依据 |
|---|---|---|---|
| 改 prune 或 opt 表到真实执行 | 无一条命令;历史点约 8 秒(工具已备时)、21.9 秒(stream 路线) | 约 3 秒(选网 0.7、gen 1.0 至 1.5、tbl 0.6、net 0.04、换包 0.26、执行 0.12) | 本轮实测分项 |
| 改 lower 或 enc 表(只本机目标) | 同上,且六目标配方多 | 约 12 秒(gen 4.6 至 8.4;tbl UNMEASURED,按 2 至 3 秒估) | 同上 |
| 改 parse2 表 | sim 上证,产品值记 UNVERIFIED;等下一次慢车道 | 约 50 至 60 秒(三张网并行,单张 gen 38.7 加 tbl 8.5);贴 55 秒线,是本方案最弱处 | 同上 |
| 首次搭环境 | 15 分钟未搭起(2026-11-13) | open 一条命令;首次编构造器与工具约 1 分钟(17 至 36 秒加 3.6 加 12.2 秒) | speed-hotspots 第 531 行;本轮实测 |
| 改驱动源到真实执行 | entry-build 整片(他人现用 290 秒上限);或本地重建 939 秒 | 第二期:约 30 至 80 秒(链接 24.7 秒加改动单元,推算) | 本地重建节 b3 |

对三个版本:

- 0.0.69(R1 加 X1):第一期帮助有限。R1 的恒等率普查是工具;条件事实表若以包内资源或网的形式被驱动读取,第三期资源替换可用;驱动里读表的一处路由与 X1 的 src/runcache.cx 都要换基座,靠第二期。建议第二期与 0.0.69 开发同期做,先取 3.4 节的基座增量数。
- 0.0.70(前端覆盖缺口):全部是 parse2 与 units 表,第一期直接适用。两条已有提交里记为 UNVERIFIED 的产品运行值,可在 local 车道约 1 分钟内得到 LOCAL 实跑值(仍不是候选资格)。parse2 的 gen 贴线是主要风险。
- 0.0.71(X3 生成码一期):opt、lower、enc 表,第一期直接适用,3 至 12 秒一轮;G1 同时动 e4、lower、enc 三层,一次 reload 换多张网正合用;六目标编码只有本机可执行,其余 NOT_CLAIMED。反过来,gen parse2 的"产品 -O2 对 gcc -O2 约 3.7 倍"可作为 X3 的一个真实工作负载基准(建议静机复测后入 speed-baseline)。

迭代出更快的 unisacc.com:性能类改动在 local 会话上用四个测速工具与已装产品做 ABAB 对拍(先例即 speed-baseline 的 LOCAL-REBUILD 各行),数字标 local、非 QUALIFIED;确认有效后才进版本分支走慢车道。

## 6 E:最小可用版本与阶段

### 6.1 第一期(MVP):模型网的本机车道

| 段 | 内容 |
|---|---|
| 目标 | 一条命令完成:改网表 -> 重建受影响的本机配方 -> 换入私有包 -> 真实切片执行;结果标 LOCAL |
| 只做 | manifest 指向工作树生成的网文件并标 local;NOT_RELEASE 标记;open、reload、run、status、close;flow 节点 local.run;一个 check 与一个批次;prd.md 能力速查一行 |
| 不做 | 自动事件触发、资源替换、基座构造、gold 网、其余五目标执行、sim 常驻 |
| 写域 | tools/local.cx、tools/check-local.cx、tools/cxlib/localsession.cx(新名,已查 git log --all 无历史);devsvc 侧 tools/devsvc-core.cx 的 local 子命令转发与 tools/cxlib/flow 系的 local.run 算子、tools/devsvc-repo.tsv 配置行;登记经 register;prd.md 经 devsvc doc |
| 依赖(已就绪) | test-loader 与 p3-swap(P4 可用,压缩字节与产品一致,本轮实证);net-deps --touch;seed 三件可由已装产品编出且输出与 gcc 版逐字节相同(本轮实证);table-lint、sim;devsvc 的 x、--bg、jobs、events、原生缓存 |
| 依赖(未就绪) | 本机切片的正规取法(第 0 步确认);已装 rev 干净树的物化(可先用 git archive,N4 落地后换);N1 命中路瘦身(不阻塞,影响体感) |
| 截止建议 | 分三单,每单一个子代理不超过 15 分钟:(a) localsession 与 open、status、close;(b) reload;(c) run、flow 节点、check 与登记 |

验收命令与预期值(预期为设计值,实现后实跑才算数):

- `sh ./unisacc.com tools/check-local.cx`,末行 `CHECK_LOCAL assertions=N failed=0`,至少含:
  - (1) 基线往返:干净树上 open 后对 errorparse 与 shared-prune 两配方 reload,会话包 sha256 等于已装包 sha256(本轮值 c7cb3a8c 开头,以实现时已装产品为准),scope 为 LOCAL-BASELINE。
  - (2) 产物对拍:local run 编最小 C 程序,镜像 sha256 等于已装产品直接编出的镜像。
  - (3) 消费实证:在夹具根(git archive 副本,不动工作树)里改 prune 表一行,reload 报 nets=1 且名字为 shared-prune,包 sha 变化,manifest 该行 origin=built、trust=local,scope 变为 LOCAL;同一输入的 stream prune 输出与基线不同或得到具名拒绝。
  - (4) 缓存负控:reload 后同一源再跑,RR_TIME 行 lookup_hit 为 0;共享缓存目录条目数不变。
  - (5) 不可发布负控:manifest 首行为 NOT_RELEASE;install-ape 对会话包干跑被拒;子进程环境含 UNISACC_LOCAL=1;带该环境跑 chain-ready 末行 READY=no(此条依赖 chain-ready 加一行拒绝,属主调度写域,未加之前记 UNVERIFIED 不记通过)。
  - (6) 拒绝合同:无会话时 run 具名拒绝;table-lint 有 ERROR 时不构网不换包;定位不到序号的配方 REFUSE recipe-not-located;触及基座输入时打 BASE_STALE。
  - (7) 输出里不含独立的 PASS 字样;其余五目标行是 NOT_CLAIMED。
  - (8) 耗时行 `LOCAL_RELOAD nets= gen_ms= lint_ms= tbl_ms= net_ms= swap_ms= total_ms=` 各列非空,未测相位写 UNMEASURED;静机(load1 低于 2)下 prune 一张网 total_ms 目标不高于 5000(估计值,首测后改为实测基线,不作门)。
- `printf` 一个只含 local.run 节点的流喂给 `sh ./unisacc.com tools/devsvc.cx -- flow -`,输出含一行 LOCAL_RUN 且 rc=0。
- `sh ./unisacc.com tools/ratchets.cx` 15 项全过;`sh ./unisacc.com tools/affected.cx -- --files tools/local.cx tools/check-local.cx tools/cxlib/localsession.cx --run` 末行 fail=0。

风险与回滚:

| 风险 | 缓解 | 回滚 |
|---|---|---|
| local 结果被当成候选通过 | 输出无 PASS;scope 标注;聚合器拒 local 身份;发布说明分列 | 工具线,删三件文件与批次行即可,产品与已装根不受影响 |
| 工作树与已装版本相差多张网,已装切片与新网不匹配 | 差异路径驱动重建全部受影响配方;触及基座即 BASE_STALE | close 会话 |
| parse2 的 gen 贴 55 秒线 | 构造器 -O2;三张并行;超界记 DEFERRED 不放宽;静机复测;长期靠 0.0.71 | 该配方回到"只在 sim 上证" |
| 与前台抢核 | 并发上限 2;P2 令牌 | 配置行把上限设 1 |
| 换包后误命中旧缓存 | 新文件新 inode;会话私有缓存目录;负控 (4) | 会话内强制 UNISACC_RUN_CACHE=0 |
| 配方到序号定位错 | 只认逐字节相同;否则拒绝 | 无需回滚 |

### 6.2 后续阶段

| 期 | 内容 | 前置取数 | 主要受益 |
|---|---|---|---|
| 二 | 本机基座:entry-build --target lnx/x86_64 --only 改动单元加 --link-only,新 entry.elf 进会话作 base(trust=local);须过"用引导编译器多单元编译"之外的全部本机检查后才可用于对拍,仍是 LOCAL | 3.4 节基座增量数 | 0.0.69 的 X1 与 R1 驱动侧;候选池 D 的落地形态 |
| 三 | 资源替换(头文件);gold 网经 kernel-route;写事件自动 reload(与 N2 共用触发器);sim 分相后决定是否会话持有 delta | sim 分相;资源记录格式核对 | 库与头文件改动;体感 |
| 四 | 日用启动器:对性能类改动,local 会话可作为日常跑工具的编译器(local-0060 先例:冷编译 -31.6%),始终标 LOCAL,不替换 ROOT/unisacc.com | 第二期稳定;ABAB 对拍流程 | 工具运行与整仓开发提速 |

## 7 F:与已否决、应停项的关系

| 已否决或应停 | 本方案是否触及 | 说明 |
|---|---|---|
| L4 零解码外部映像 | 否 | 会话包仍是压缩 M 记录加 CRC,不存解码后映像,不新增映像校验 |
| CX0 规划缓存 | 否 | 不缓存规划结果 |
| 方案 B(收据承接,已并入 R2) | 否 | 不承接任何测试结论;local.run 永不缓存 |
| lower 前缀复用与属性入键、elf 前缀模型步、L3 流并行 | 否 | 不改流内复用与并行 |
| 产品侧常驻服务(SPEED-14) | 否 | 产品不驻留;驻留的是 devsvc 工具实例,持有的是会话目录与构造物 |
| e3 增量、全局寄存器分配、-O2 设为默认 | 否 | 仅构造器执行体用 -O2 编,属工具侧选档,不动产品默认 |
| 再细化 affected 闭包 | 否 | 选网用 net-deps 的静态上界,多报不漏报 |
| boot 层按 inode 缓存已校验结果(L3 设想,未实现) | 否 | 不提;installed 成员每次照常校验 |
| 已排期项(R1、R2、R3、X1、X3、N1 至 N6、候选池 C、D、I、L) | 只引用 | 第二期是候选池 D 的落地形态,不另立项;触发器与 N2 共用 |

## 8 未核实与局限

(1) 全部本轮数字 noisy(load1 10.7 至 19.8),各 1 次。
(2) 换出的包与 local-0060 记录的包 sha 前 16 位相同,含义未查。
(3) 本机切片的解包路径规则只凭运行中进程观察到,未读启动器源码。
(4) 正规链 job_command 里的 cc 实际指向哪个编译器未核;本文把 gcc 只当对照臂。
(5) install-ape 对会话包的拒绝文案、release pack 对无回执包的拒绝,均为按现有合同的推断,未实跑。
(6) R1 的生成表以什么形式被驱动读取(编进切片还是包内资源)未定,决定它落在第二期还是第三期。
(7) prd_mempalace.md 的版本流水线图仍把 0.0.69 写作 prune 原样判定、0.0.70 写作 cxmod 内存安全乙期,与 prd.md 版本表(0.0.70 为前端覆盖缺口)不一致;本文按 prd.md 版本表,未动记忆宫殿。
