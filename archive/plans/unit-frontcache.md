# 单元前端(e2/e1)身份缓存键合同

状态:合同与工具侧原型(tools/check-cx-unitcache.cx);产品侧有私有克隆原型并已实测(见「实现设计」节),未落地,落地排在 0.0.55 P1。

## 实测依据(tools/cx-phase.cx,Linux x86_64,已装产品,单次墙钟,ms)

用例 uN 为 N 个 use 的空 main;单元数列为实测 e2 次数(模块闭包会多于 use 数)。

| 用例 | 单元(e2 次数) | 冷 wall | 半冷 wall | 热 wall | 冷 e2+e1 推理 | 占冷 wall |
|---|---|---|---|---|---|---|
| u1 | 1(1) | 394 | 351 | 8 | 148 | 38% |
| u2 | 2(2) | 1335 | 1343 | 11 | 393 | 29% |
| u6 | 6(6) | 3796 | 3701 | 29 | 1340 | 35% |
| u12 | 12(15) | 9433 | 9001 | 82 | 3741 | 40% |

冷 = UNISACC_RUN_CACHE=0;半冷 = 运行缓存未命中(源码带新注释)而模型缓存开且已预热(UNISACC_E3CACHE 隔离目录);热 = 同源码再跑(运行缓存命中,无相位行,记 UNMEASURED)。

## inflate 去重结论

半冷下模型缓存全部命中(u12 35 次 hit 0 miss),e2/e1 每次装载约 8 ms(含读文件与 sha 核验);u12 的 e2+e1 共 30 次装载合计 250 ms,占半冷 wall 9001 ms 的 2.8%;u6 为 96 ms/3701 ms = 2.6%。冷下 u12 为 384 ms/9433 ms = 4.1%。因此 inflate 重复在半冷下仍存在但只剩约 8 ms/次,去重至多省约 3%,不值得单独进产品。此前 146 ms 的数字是 UNISACC_RUN_CACHE=0 下测的,该开关同时关了模型缓存,故属冷上界。真正的大头是推理:e2+e1 推理占冷 wall 29%-40%,只有"单元结果缓存"能省。

## 键合同

键 = sha256 over(固定版本串 "UNIUCKEY1" + 单元源 sha + 强制头 sha 序列(顺序敏感,含个数)+ e2 模型 sha + e1 模型 sha + 目标 + ABI 标识)。其中:

- 单元源 sha:该模块单元预处理前的源字节 sha(含 `#pragma unisa` 行),不含其它模块;
- 强制头 sha 序列:驱动对该单元强制注入的头(隐式运行库、模块接口头)逐个 sha,按注入顺序;
- 模型 sha:e2/e1 解压后模型字节的 sha(与 modelcache 载荷 sha 同一身份),不用文件名或 mtime;
- 目标与 ABI:目标三元组加调用约定/类型宽度表的 sha。

### 键字段实测(tools/check-cx-unitkey.cx,批次 cx-unitkey;`stream e2|e1 --multi --unit N`,同单元逐项变化比输出字节)

| 字段 | e2 | e1(输入同一份 e2 输出) | 键处理 |
|---|---|---|---|
| 单元源字节 | AFFECTS | 经 e2 输出传递 | 入键(单元源 sha) |
| 二级头内容(h.h 包含的 h2.h) | AFFECTS | 经 e2 输出传递 | 须入键:传递头 sha 序列 |
| defines(-D 记录,含值) | AFFECTS | NO_EFFECT | 入键:defines 记录序列 sha |
| 源标签(\0cli/source) | AFFECTS(源含 __FILE__ 时) | NO_EFFECT | 入键:标签(或证明源无 __FILE__/诊断位置依赖) |
| 单元序号 --unit | NO_EFFECT(nunit=1) | NO_EFFECT | 不入键;但 nunit>1 时 RT-SHARED 宏随序号变(UNMEASURED,见下) |
| include-dir 路径(头字节相同) | NO_EFFECT | NO_EFFECT | 路径本身不入键,入键的是 e2 实际读到的头内容 |
| quote-dir(头字节相同) | NO_EFFECT | 未测 | 同上 |
| 前序属性表(\0cli/attributes,USLATTRT 载体) | 载荷 NO_EFFECT;载体被原样回带(不带 --raw 的输出含它) | NO_EFFECT | 载荷键不含前序表;但命中回放须把本单元新增属性记录按序号并入累积表,不能直接复用含载体的旧输出 |
| nunit / RT-SHARED(st_nunit) | UNMEASURED(stream 不设 st_units) | UNMEASURED | 多单元自链接时 \0cli/defines 带 RT-SHARED 宏,须入键:(nunit>1 标志 + 本单元 provider 序号) |
| hs_file_note 单槽 | UNMEASURED(运行期副作用,stream 无入口) | UNMEASURED | 缓存命中会跳过 e2 的读文件登记,runcache 依赖集将缺项;命中时须回放依赖 |
| e2/e1 模型、目标、ABI | 不在 stream 变量内 | 同 | 已在键(目标另有 --target 路径) |

实测修正原键字段:原键漏 defines、源标签(含 __FILE__ 时)、传递头内容(二级头);原键含强制头 sha 序列,实测里对应"头内容变则输出变",传递头与强制头同类,须由 e2 读文件登记取得而非预先枚举。

命中时依赖回放要求:
1. 传递头:缓存条目须附 e2 读到的全部头(路径加内容 sha)清单;命中前逐项核当前内容 sha,任一不符按未命中。头路径解析受 include-dir/quote-dir 影响,故清单记解析后的实际文件,不记目录。
2. hs_file_note 与 runcache 依赖集:命中必须把清单里的文件回放进 hs_file_note,否则运行缓存键漏依赖(UNMEASURED,产品落地时实测)。
3. 前序属性表:命中时按单元序号重放本单元的属性记录到累积表;缓存载荷存不含载体的载荷(--raw 形态)。
4. 源标签:含 __FILE__ 或诊断位置的输出依赖标签,标签入键。

文件格式沿用 modelcache 风格:"UNIUC1\n" + 载荷 sha(64 hex)+ "\n" + 载荷,文件名 uc-<键>;产品版另在头内记键本身并在命中时核对(夹具标 UNMEASURED,须产品落地时补)。写入先写 .tmp.<pid> 再用无覆盖发布(link 发布法)落位;并发写同键内容必然相同。

命中仍核内容:读出后核载荷 sha 与头内键;任何失败(无文件、头坏、sha 不符、截断、长度不符)都按未命中回退重算,不报错;重算后重新发布。缓存结果不替代执行结论,UNISACC_RUN_CACHE=0 同时关读写,路径与现状字节相同。

## 负例四类(tools/check-cx-unitcache.cx 已对原型实跑)

1. 命中:同键取回字节与首次相同。
2. 失效:源、头内容、头顺序、头个数任一变 -> 键变 -> 未命中。
3. 损坏:翻一字节、截断、魔数错、空文件 -> 判 corrupt,回退重算并可重发布。
4. 键变:e2 模型、e1 模型、目标、ABI 任一变 -> 键变 -> 未命中。

产品落地时另加:并发写(多进程同键)与 UNISACC_RUN_CACHE=0 字节等价,以及"产物字节与无缓存路径相同"的回归。

## 产品改动面(预计,0.0.55 55-7(b),触产品输入)

- cx/runtime/modelcache.cx:导出按键读写单元结果的 mc 风格接口(复用目录、预算 LRU、.budget.lock、条目名校验 mc_entry_name 需认新前缀 uc-);
- src/driveunits.cx:在每单元 e2 前算键、查缓存,命中跳过 e2 加 e1 推理,未命中后发布;
- src/runcache.cx:仅复用其 rc_off 与 RR_TIME 汇报(新增 result 行),不改运行缓存键。

前提:e2/e1 的单元输出须是可独立序列化的字节(55-5(b) 的 CX0BND1 单元身份先到位)。

## 实现设计(0.0.55 P1,2026-11-11 私有克隆原型实测;原型补丁 p1-proto.patch,未落地)

本节取代上文「键合同」「产品改动面」里与之冲突的预计写法;上文实测表保留。

### 三个 UNMEASURED 项的实测

方法一(取数全集):`UNISA_JOBS=1 UNISACC_RF_CAPTURE=DIR 候选 两单元.cx -o X`,账本 resources.tsv 按序列出每次 core_host_fetch 的键。e2/e1 两单元段实际取的键只有三类:(a) st_pub 发布的 `\0cli/*`(nostdinc、fno-trim-libc、includes、source、include-dir、target、defines、object、funit、undefines、quote-dir、f1);(b) 包内不可变资源(`\0kernel/*`、`\0hdr/*`、`\0predefines/<目标>`、`\0library/*`);(c) 经 hs_file 读的文件(`\0inc/q/`、`\0inc/a/`,含不存在的探测)。`\0cli/attributes`、`\0process/argc|argv`、`\0cli/error-limit` 在单元段没有被取,首次出现在 e3。

| 项 | 方法 | 结论 | 键处理 |
|---|---|---|---|
| nunit>1 的 RT-SHARED 宏 | `stream e2 --multi --unit 0|3 --res defines=`(无 / `RT_UNIT` / `RT_EXTERN`)对含 stdio.h 的单元;再把三份 e2 输出喂 e1 | e2 三种宏三种输出(AFFECTS);同宏下 unit 0 与 3 相同(序号 NO_EFFECT);e1 只随输入变,对宏与序号 NO_EFFECT。账本里单元 0 的 defines 为 `RT_UNIT`,单元 1 为 `RT_EXTERN` | 宏随 `\0cli/defines` 的发布值入键;序号与 nunit 本身不入键(它们只经 st_rt_shared 进 defines,代码上无第二条通路) |
| hs_file_note 回放 | 原型:同一程序先存储跑,再移走运行缓存条目做全单元命中跑,比两次写出的运行缓存条目(USE33 带与 USE34 .host,含 present/absent 依赖清单)逐字节;另做两个活失效探针 | 26 例中 25 例成功编译,条目逐字节相同(第 26 例是编译失败例,不写条目);全命中跑后新增遮蔽头(原 absent)与改 present 头,运行缓存均未命中且输出随之变 | 回放充分;登记改为可叠加后由命中路径逐项回放。未回放时缺项是按代码推断(rc_have 唯一来源是 hs_file 登记),没有单独造不回放的产品实测 |
| 模型 / 目标 / ABI | 目标:`stream e2|e1 --target` 五目标对含 `__aarch64__`/`__APPLE__` 条件的单元;模型:手头没有 e2/e1 模型不同的第二个包;ABI:驱动里没有独立于目标串的 ABI 开关 | 目标 AFFECTS(lnx/x86_64、lnx/arm64、osx/arm64、osx/x86_64 四种输出;win/x86_64 在该样本上与 lnx/x86_64 相同,换 stdio 样本则不同);模型 UNMEASURED;ABI 无独立变量 | 目标入键(路由串加 `\0cli/target`);模型测不出,按规则入键,且用整个产品身份覆盖模型、包内头、解释器与 ABI 表 |

### 最终键组成

键 = sha256("UNIUC3\n" 加下表各行)。原则:单元段能取到的一切,要么进键,要么进命中复核清单,要么被产品身份覆盖。

| 成分 | 取法 | 覆盖 |
|---|---|---|
| 产品身份 | 与运行缓存同一个 rc_prod(容器文件 size/dev/ino/mtime/ctime 的 sha) | e2/e1 模型、包内头与 predefines、kernel、解释器、ABI 表;换产品必不命中 |
| 单元路由 | `<目标>/unit` | 目标、阶段序列 |
| 发布态 sha | st_pub 为该单元发布的全部键值按序(LE32 键长、键、LE32 值长、值)的 sha;排除 `\0cli/attributes` 与 `\0process/*` | 源标签(`\0cli/source`,即 `__FILE__`)、defines 含值与 RT-SHARED 宏、undefines、强制头清单(includes,含 ua-cxif 接口头路径)、include-dir、quote-dir、nostdinc、target、error-limit、f1、object/funit/ccw 等附加项 |
| 单元源 sha | 交给 st_span 的字节(cxi_read 之后) | 单元源 |

不入键:单元序号、nunit(只经 defines 起作用)、前序属性表(实测单元段不取)、进程 argv 指针、cwd(相对路径的头由复核清单按当前 cwd 核内容)。同键只有一个条目,后写覆盖。

### 命中复核与回放

1. 传递头逐项 sha:条目带未命中时 hs_file 读到的全部文件(解析后的实际路径)。present 项的 sha 在 hs_file 把字节交给阶段的那一刻由 `uc_note(path, 1, bytes, n)` 就地算出(编译用的就是这些字节),存储时不重读文件;同一单元内同一路径两次读到不同字节则放弃存储。命中前逐个读当前文件核 sha256,absent 项要求仍是 ENOENT(ENOTDIR/EACCES 算失效)。任一不符按未命中重算并覆盖条目。
2. 条目自核:魔数、条目 sha(整个文件,sha 槽填 '0';覆盖各计数、帧标志、键、依赖清单与正文)、头内键与当前键相同、长度;任何失败按未命中,不报错。
3. 文件登记可叠加:登记表在 cx/runtime/unitcache.cx(最多 4 个回调),`uc_note_set` 幂等追加,`uc_note_drop` 摘除,hostres 的 hs_file 调 `uc_note` 逐个通知。未命中时单元缓存在 `uc_note` 内直接收集,与 runcache 的 rc_on_file 并存;命中时把清单逐项再送进 `uc_note`,运行缓存依赖集与无缓存时相同。
4. 前序属性重放:条目存本单元各阶段剥下的 5 字节属性记录和「是否见过 USLATTR1 帧」标志。命中时按当前序号 cxaf_merge 进累积表;零记录但见过帧且表为空时补 4 字节零计数头,与 cxaf_strip 的提交行为一致。载荷是剥帧后的字节。
5. 源标签:已在发布态 sha 内。

### 存储格式与原子发布

文件 `<缓存目录>/uc-<64 hex 键>`,与模型缓存、运行缓存同目录、同 `.budget.lock`、同 UNISACC_E3CACHE_MAX_BYTES 预算与 LRU(mc_entry_name 认 `uc-` 前缀,mc_complete 核魔数与最小长度 160)。

布局(现行 `UNIUC3`,头 160 字节):`"UNIUC3\n\0"`(8) + LE32 载荷长 + LE32 记录数 + LE32 present 数 + LE32 absent 数 + LE32 帧标志 + LE32 stderr 长 + 键(64 hex,偏移 32) + 条目 sha(64 hex,偏移 96) + present 项(LE16 路径长、路径、64 hex sha) + absent 项(LE16 路径长、路径) + 记录(5 字节一条) + stderr + 载荷。条目 sha 是 sha 槽填 64 个 '0' 时整个文件的 sha256。

发布:持预算锁写 `uc-<键>.tmp`(同目录,名字固定),fsync 后 rename 落位再 trim;tmp 只在持锁期间存在,所以 trim(持锁执行)见到的 `uc-<键>.tmp` 必是崩溃写入者的残留,当场删除。并发同键串行各自 rename,读者只见完整文件。实测条目 0.35-0.47 MB/单元,存储约 4-8 ms,命中约 5 ms(含逐头 sha 与条目 sha)。

### 开关

- `UNISACC_RUN_CACHE=0`:rc_open 不置 rc_on,单元缓存不算键、不读不写、不打印;26 例产物与改动前候选逐字节相同,缓存目录条目数不变。
- `UNISACC_UNIT_CACHE=0`(新增):只关单元缓存,运行缓存与模型缓存照常;RR_TIME 下每单元打印 `unitcache result=off`。
- 沿用运行缓存的准入:warnings、tape、deps、资源流捕获/strict、非 lnx/x86_64 目标时不启用。
- 观测:`RR_TIME<TAB>unitcache<TAB>result=hit|miss|store|off<TAB>ns=N`;`UNISACC_RR_DEBUG` 下复核失败打印 `RR_UNIT_MISS present|absent 路径`。

### candidate-chain 内强制关闭及断言

- 用独立开关而不是 RUN_CACHE=0:链内大量 `sh ./unisacc.com tools/*.cx` 靠运行缓存热启动,整体关掉会把每次工具启动变成 3-10 s 冷编。
- candidate-chain 的 main 入口先 `sy_setenv("UNISACC_UNIT_CACHE","0",1)`,读回不是 "0" 即 REFUSE,子进程全部继承;`--plan` 首行打印 `ENV UNISACC_UNIT_CACHE=0`(原型已做,`--plan` 实跑通过)。
- 落地时补两条断言:(1) check-candidate-chain 的 plan 用例加 `has(out, "ENV\tUNISACC_UNIT_CACHE=0")`;(2) 链的 warmup 相位对一个两单元夹具带 RR_TIME 跑被测产品,要求 `unitcache result=(hit|miss|store)` 行数为 0(0.0.54 引导产品没有该行也算通过)。
- 结构性事实:产品对象走 `-c -funit` 单源路由,不经 dv_units,本就碰不到单元缓存;开关防的是多源链接步与以后的路由变化。

### 原型改动面与实测

改动 410 行增、10 行删,共 7 个文件(不含本文档):src/runcache.cx +239(键、查、存、复核、回放),src/driveunits.cx +70 -2(dv_unit_cached 包住 dv_units 里的 st_span),src/stages.cx +60 -1(st_unit_blob、st_strip_count),src/hostres.cx +25 -4(可叠加登记),cx/runtime/modelcache.cx +6,tools/candidate-chain.cx +4 -1,tools/cx-phase.cx +6 -2(`--product`、first 模式)。没有新模块,不动 hostseed/cx-driver-build 的对象清单。

耗时(tools/cx-phase.cx `--product`,entry-build 候选,单次墙钟 ms,机器有其它负载,约 10% 噪声):

| 用例 | 关缓存(RUN_CACHE=0)基线/原型 | 首跑(缓存开全空)基线/原型 | 换根命中(模块不变改根)基线/原型 | 换根 e2 次数 基线/原型 |
|---|---|---|---|---|
| u1 | 375 / 376 | 514 / 445 | 354 / 347 | 1 / 1 |
| u2 | 1281 / 1291 | 1352 / 1357 | 1393 / 1161(-17%) | 2 / 1 |
| u6 | 3786 / 3840 | 3749 / 4382 | 3713 / 2701(-27%) | 6 / 1 |
| u12 | 9351 / 9218 | 9362 / 9547 | 9556 / 6108(-36%) | 15 / 1 |

u6 首跑 4382 属噪声:同机单元缓存关闭的 u6 三模式为 4204/4305/4154。最终候选复测 u12 为 9262 / 9345 / 6309。

字节等价回归(分析脚本 ucreg.cx,每例四跑:基线关缓存、原型关缓存、原型存储跑、原型全命中跑):26 例全过,合计 204 次单元命中、0 次未命中;四份 `-o` 镜像逐字节相同,基线与原型的 stderr 相同。语料:tools/*.cx 14 个(含 release-build 26 单元)、tests/*.cx 8 个、tests/*.c 1 个、手造多源 C 3 组(含 constructor 属性与换序)。其中 1 例是编译失败例,四跑 rc 与 stderr 一致。

-run 模式抽查(tools/cx-phase.cx `--parse`):基线、存储跑、全命中跑(12 命中 0 未命中)的程序输出相同。

负例(手造 a.c、b.c、头):头内容变、源变、-D 变、换序(RT_UNIT/RT_EXTERN 角色互换)均未命中;只改 a.c 时 b.c 命中;翻一字节与截断按未命中重算并重新发布。

### 产品化修订(同日第二轮,补丁 p1.patch;与上文冲突处以本小节为准)

- 模块位置:缓存逻辑全部在新模块 cx/runtime/unitcache.cx(L1,只 use L0 与同层的 rcident、modelcache、attribute_frame;check-cx-layers 无新增上行边)。src 只留调用点:driveunits 一处 `uc_span(st_span, st_pub, st_attr_exchange, ...)`,stages 的 st_key 改走 `uc_cli_put`、剥帧改走 `uc_strip`、st_pub 去掉 static,runcache 的 rc_open 调 `uc_open`、rc_arm 改用 `uc_note_set`,hostres 的登记改调 `uc_note`,executor 的阶段 stderr 改走 `uc_err`。十三项棘轮全过,srcshape 5854 -> 5849 行(只降)。模块不按名字上调驱动,st_span 等以函数指针传入;ex_flow_pinned 经 `use runtime.runexec` 取得。rc_open 里是单独一句 `if (uc_open(product, target) || warnings) return 0;`,uc_open 在资源流被钉住时返回 1。
- stderr 存储与回放:条目带 stderr 段(头 160 字节,六个 LE32:载荷、记录数、present 数、absent 数、帧标志、stderr 长度;正文为 记录、stderr、载荷),格式经独立审查后现为 `UNIUC3`。未命中时 ex_exec 写阶段 stderr 的那一处经 `uc_err` 同时写出并留存;命中时在该单元位置原样写回 fd 2。RR_TIME 行不经此路,不入条目。
- 启用条件与 warnings 解耦:单元缓存在 warnings 打开时也启用(运行缓存此时仍关)。实测警告与错误诊断都出在 e3(`-Wall` 的 unused variable、语法错误的位置信息均在 e3 相位行之间打印),e2/e1 在成功路径上对 #warning、宏重定义、未知 pragma 都不出字节;命中后 e3 照跑,警告不丢。真实输入里没有找到成功且带 e2/e1 stderr 的单元,回放路径用伪造条目(塞入 stderr 并重算 sha)实测:每个命中单元回放恰好一次。
- 目标:只在已验证的宿主目标 lnx/x86_64 启用,写死在 uc_open;lnx/arm64、osx/arm64、win/x86_64 实测不出任何 unitcache 行、不写 uc 条目。键与复核逻辑本身不含目标特判(目标经路由串与 `\0cli/target` 入键),放开其它目标只需改 uc_open 一行并补该目标的等价回归。tape、deps、资源流捕获/strict 时不启用。
- 并发写同键(实测):2 个与 4 个进程同时从空目录编同一三单元程序,全部 rc 0,镜像与关缓存相同,目录无残留 .tmp,每单元恰一个条目,随后一跑全命中。写入在预算锁内完成 tmp 写与 rename,同键后写覆盖先写,内容相同。
- 预算淘汰(实测):不设限时三单元程序留 1906243 字节、3 个 uc 条目;`UNISACC_E3CACHE_MAX_BYTES=300000` 时编译成功、镜像不变、留存 286440 字节(uc 条目因单个超预算被当场淘汰);设 0 时不留任何完成条目;淘汰后再跑按未命中重算,镜像不变。
- 回归工具:tools/check-unit-frontcache.cx,十一个模式各一批次(unit-frontcache-equiv0..4、-neg、-race、-stderr、-switch、-concurrent、-budget,先登记 daily,产品安装后再升 commit),已登记 tests/batches.tsv、tests/coverage.tsv、tests/tools-compile-sources.tsv 与 prd 能力速查。对审查修订后的候选实测:五片等价 17-27 s(26 例、147 次命中、0 未命中),neg 27 s,race 15 s,stderr 1 s,switch 5 s(带 `--base` 时另核与改动前候选逐字节相同),concurrent 5 s,budget 7 s。产品不带单元缓存时打印 DEFERRED 并返回 3。
- 换根耗时复测(第二轮候选 u12):关缓存 9794,首跑 9804,换根命中 6378 ms;审查修订后未重测耗时。

### 独立审查后的修订(同日第三轮)

- 编译中途改头存入陈旧条目(审查复现):原实现在编译结束后重读头算 sha,旧载荷会配上新头的 sha。改为 sha 取自 hs_file 交给阶段的字节。race 模式(编译开始后 0.05-0.30 s 改头,六次)对修订前候选 6 次中 4 次产物错,对修订后候选全过。运行缓存自身的 rc_store_kind 仍是事后重读(既有行为,本补丁未动)。
- 条目 sha 覆盖不全(审查复现):原 sha 只管正文,改帧标志或依赖清单路径仍命中。现为整文件 sha(sha 槽填 '0'),neg 模式加两条:「翻帧标志」,以及「把清单里的 absent 路径 inc1/br.h 改名后再新建该遮蔽头」(a.c 必须未命中、b.c 命中、产物随新头);两条对修订前候选都 FAIL,对修订后候选通过。
- candidate-chain:入口强制 `UNISACC_UNIT_CACHE=0` 外,正式运行在建好 --out 后先跑 uc_guard(用链的产品带 RR_TIME 编两单元夹具,出现 unitcache result=hit|miss|store 即 REFUSE);`--uc-guard DIR` 为夹具入口。check-candidate-chain 加两项:调用方设 `UNISACC_UNIT_CACHE=1` 时 plan 仍打印 `ENV<TAB>UNISACC_UNIT_CACHE=0`,守卫输出 `UC_GUARD ok lines=0`;守卫目录建在批次 `{out}` 下(candidate-chain 批次行的参数由 `-` 改为 `{out}`),守卫自删所写文件,检查以 rmdir 成功断言无残留。已变基到 bf21dd197c(与 install_if_changed 的参数解析并存)。用带单元缓存的候选冒充链产品实跑守卫:2 行 result=off,0 行命中/未命中/存储。
- executor 的三行 fputs/fputc 已改回原样;给 use 行腾出的一行来自把 `if (result.err.n > 0 && result.err.b)` 的判断并进 uc_err(函数内同样判断)。
- 崩溃残留:mc_trim 清 `uc-<键>.tmp`(budget 模式种一个残留、下一次存储后消失)。

### 未做与落地前必须处理

- 十一个批次走材料约定:batches.tsv 的 materials 列声明 `unitcache-product`(带单元缓存的候选产品可执行文件),args 为 `{out};--product;{unitcache-product};<模式>`。不给材料时 affected 报 `DEFERRED needs-materials=unitcache-product`(实测 11 个 deferred、0 fail);给材料时 `tests/run-batch.cx ROOT PARENT ID unitcache-product=/abs/candidate` 实跑,自建候选作材料十一个全 PASS(3-32 s)。检查本身在无特性产品上仍打印 DEFERRED 并返回 3,不记 PASS。档位 daily,产品安装后由集成者决定是否去掉材料并升 commit。
- 宿主 C 引导链(hostseed-step)、cx-driver-build、candidate-chain 全链未跑:新模块经 `#pragma unisa use runtime.unitcache` 进闭包,entry-build 实测能构造,其它构造路径是否自动带上 unitcache 对象是 UNMEASURED,须慢车道核。
- tests/tools-compile 末片在重定基后的 main(648b7fd9cd)上 REFUSE missing registered shard:main 已有 746 个源、每片 8 个需要 94 片,只登记到 092;本补丁再加 1 个源不改变片数,093 片的登记留给集成者。check-cx-layers 的唯一 FAIL(holdframe -> compiler.frames)为基线既有。
- check-cx-unitcache(工具侧旧夹具)仍是最初的 UNIUC1 布局,未改;与产品格式 UNIUC3 不对应,建议退役或改写。本文「键合同」一节里的 UNIUC1 与 .tmp.<pid> 写法是该旧夹具的合同,不是产品格式。
- UNISACC_PHASE_IN/OUT 相位检查点与单元缓存共存:UNMEASURED。
- 产品身份是 stat 身份,不是内容 sha;换产品文件即全失效,跨版本不复用(有意保守)。
- 运行缓存的依赖 sha 仍在编译后重读(既有窗口):编译中途改头时,那一次编译自己写的运行缓存条目可能陈旧;单元缓存已无此窗口。
- 每次存储触发一次 mc_trim 目录扫描;条目上千时的代价 UNMEASURED。
- race 模式靠时间窗撞缺陷,对修订前候选 6 次中 4 次,不是每次必中;修订后的正确性来自「sha 取自所读字节」的构造,不依赖撞窗。

## 预期收益(以实测为依据)

仅在同一单元在不同根程序间复用时有收益(A8:h5b 换根仍 3570 ms 即无复用)。命中时每单元省 e2+e1 推理:约 150-250 ms/单元(u12 冷下 3741 ms/15 单元 = 249 ms/单元;u1 为 148 ms),即冷 wall 的约 30%-40%;u12 级程序约 9.4 s -> 约 5.7 s。e3 及以后(约 55%-60%)不在本缓存范围,留给 56-2。inflate 去重不计入(见上,至多 3%)。
