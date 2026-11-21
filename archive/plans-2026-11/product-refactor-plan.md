# unisacc 产品整体结构小型重构计划(草稿)

状态:草稿,2026-11-18,只读调研产出,未改仓内文件,待 cc 审阅后入仓为 plans/product-refactor-plan.md。输入身份:共享检出 main 头 1c58acd2ad 的工作区;已装产品自报 0.0.71;v0.0.76 分支只经 git show 读取。未实测的数字标 UNMEASURED,引自他人证据的数字写明出处,本轮复核过的写"本轮复核"。

## 0 结论

- 最大的改善空间在表和网,不在 src 与 cx 的目录搬迁。产品源码总量小(src 5854 行受 srcshape 只降棘轮守,cx/driver 3464 行),而 exec 有 1102 张表、42 个网 137282 状态,其中结果表 196 张只有 8 张有规则等价校验,table-lint 没有接进任何批次或棘轮(本轮复核:仓内引用 table-lint 的只有它自己、tests/delta-loader.cx、tools/cx-dev.cx)。近三天的返工(W2a 被 check-literal 打回后搬 26 个常量、d0 与 d1 分叉、结果表直接编辑无生成器)全部发生在这一层。
- 默认做法分三档:(1) 立即做三张零风险先导单(只加检查与度量,工具类,在 main 上,不占候选槽);(2) 单开一个"表网卫生专版",只放字节等价的表重构(d0 与 d1 共用块、check-literal 改预算口径、真实未写寄存器补写),排在内存安全二段之后、"R2 收尾与对象产出缺陷"之前;(3) 对象产出合同(.data、.rodata、.init_array、weak、SysV 入口)先出设计单,它改变行为,不属于重构,不与重构同候选。
- 纯表重构有一个比 16 份样例更强也更快的 oracle:改前改后 42 网的 delta JSON 逐字节相同,则产品 sha 相同。W2a3 已用过(release/evidence/v0.0.76-r2-w2a-2026-11-18.tsv 的 w2a3-nets 行:两网 delta cmp-equal,开发产品 sha 5b1794d597db 不变)。凡能以此验收的单排最前。
- agy 的报告有帮助:它给出了表网这一层的第一份量化底数和三处静默缺陷线索;但差分部分测在落后五版的已装产品上,第三项定性有误,估时与部分统计要复核(见第 1 节)。
- 不建议做:网的拆分与合并、cx/compiler 与 cx/runtime 的 24 对同名生成模块合并、为 359 张规则表逐张补单测。理由见 3.3。

## 1 (A) agy 的 TSV 转 NN 审阅有帮助吗

结论:有帮助,主要贡献是把"表网体量大、校验薄"从印象变成了可排序的数字,并直接产出本计划表类与网类候选的大半;差分线索给出两处三版都在的静默缺陷。局限是统计脚本留在 /tmp/agy-nn-review 未入仓,仓内无法重算,所以先导单 P1 与 P3 的第一件事就是把它沉淀成带检查的工具。

| 类别 | agy 的数据 | 本轮或独立复验结果 | 评价 |
|---|---|---|---|
| 可信可用 | d0.s23 44 行对 d1.s24 40 行 | 本轮复核:main 上 44 对 40;v0.0.76 分支已是 48 对 40,d0.s17 58 对 d1.s18 49,分叉在扩大 | 直接成立,是候选 T2 与先导单 P2 的依据 |
| 可信可用 | check-literal files=189 total=3019 | nn-verify S-a 行复现;tests/literal.tsv 142 行 | 成立;"几乎零余量"的结论被 W2a2 实例印证(elfobject 58 大于 55、objectplan 227 大于 172 被拒) |
| 可信可用 | tokenlocations rel impact 537/1476 批 | nn-verify S-b 行复现 | 成立;"32/42 网"未复算 |
| 可信可用 | 结果表仅 8 张有 decide 等价检查 | 本轮复核:tests/check-decide.cx 恰有 8 条 one() 规则 | 成立 |
| 可信可用 | 全局初值与 const 进 .bss 无 .init_array;weak 发射为 GLOBAL | nn-verify V-01、V-02:0.0.71、v0.0.75、v0.0.76 候选三个产品都仍存在 | 成立,且是静默错误;已登记 NN-VERIFY-ELF-INIT、NN-VERIFY-WEAK |
| 要修正 | -funit 非 main 序言污染为"确认缺陷" | V-03b:序言在 v0.0.75 已修;残余缺陷是带参对外函数首参取自 %rax 而非 SysV 的 %rdi | 现象真实但定性过期,测在 0.0.71 上;修法归属从"序言"改为"调用约定" |
| 要复核 | 42 网 states=137282、unwritten=52、maybe_unwritten=9976、sb_read=46 | 脚本在 /tmp,未入仓,本轮未重算;旁证:W2a2 对 object-enc-x86_64 单网实测 2531 到 2540 状态、warnings 307 到 310,量级吻合 | 量级可用,精确值待 P1 复现后入基线 |
| 要复核 | 955/1102 张表无直接单测;41 条粗 exec/ 前缀 | 本轮粗查 tests/batch-product-inputs.tsv:含 exec/ 的行 52,口径不同未对齐 | 方向成立,数字待 P3 复现 |
| 要复核 | "5 组真实未写"(of_unit、ms_blob、mg_*、ps/pe、nl/tf_output) | 未逐例读表;agy 自己说明 36/52 是故意读 0 | 逐组定性后才能改产品表,不能批量补写 |
| 不一致 | 余量为零的表数 188/189 与 132/133 | nn-verify S-a 指出报告内自相矛盾 | 结论方向不变,数字不引用 |
| 不采纳 | R-06"1 天为 359 张表补单测";R-05 把三类对象缺陷并成 1 到 2 天一单 | 与每单不超过 40 分钟、一个候选一类大改的铁律冲突 | 估时不采纳,拆法见第 2、3 节 |
| 未测 | 所有 arm64 运行行为 | agy 与复验均未在 arm64 实跑 | 一律 UNMEASURED |

对重构决策的贡献:(1) 把重构重心从目录搬迁改判到表网层;(2) 证明"先收窄 exec/ 触发前缀"是错的顺序(先补覆盖再收窄);(3) 指出 maybe-unwritten 噪声占告警 72.8%,所以 table-lint 不能直接 --strict 接门,要先建只降基线;(4) Top10 的 R-01、R-03、R-04、R-07、R-08、R-09 被本计划采纳为 N1、N3、T2、N1、N2、T3,排序做了调整。

## 2 (B) 小型重构候选清单

规模口径:单 = 一张不超过 40 分钟的工单。等价方式缩写:DELTA = 42 网 delta JSON 改前改后逐字节相同;BIN16 = 至少 16 份 examples/apps/*.c(现有 17 份)整程序 -o 产物 sha256 相同;FIX2 = 两代不动点;ENT3 = 三入口一致;DIFF = 与 gcc 加系统 ld 对拍(行为有意改变时)。

### 2.1 功能类

| 号 | 现状(证据) | 痛点(已发生) | 做法(最小步) | 等价或对拍 | 风险 | 收益 | 规模 | 依赖 |
|---|---|---|---|---|---|---|---|---|
| F1 对象产出合同 | -c -funit 对象无 .rodata、.data 为 0,初值全进 .bss 由 __init_u 赋值且不登记 .init_array(nn-verify V-01,三产品);.bss 对齐固定 16(agy DIF-05,未复验) | gcc 写的 main 链接后读到 0,期望 51 得 0;静默错误 | 先出设计单:写一页对象合同表(节、符号绑定、对齐、初始化登记),定最小改面是"__init_u 登记 .init_array"还是"初值写 .data";再拆 3 到 4 张实现单 | DIFF(nn-verify 的 p03、p10 作正控);整程序路 BIN16 必须不变(-o 不经对象路) | 中:改 objectplan 与 elfobject,x86 与 arm64 都要改 | 外部 C 互操作可用;量化 UNMEASURED | 设计 1 单,实现 3 到 4 单 | T2 先做可省一半 |
| F2 -funit 对外函数 SysV 入口 | 带参对外函数首参取 %rax(nn-verify V-03b,v0.0.76 候选 rc 17,期望 11) | 只能导出无参函数 | 设计并入 F1 的合同表;实现为 lowering 调用约定表加入口搬运或外部包装桩 | DIFF(p02c 正控);BIN16 不变 | 中:arm64 UNMEASURED | 同 F1 | 2 单 | F1 设计 |
| F3 weak 绑定 | weak 发射为 STB_GLOBAL(V-02,三产品) | 与强定义链接报 multiple definition | 前端属性到符号绑定列传递,elfobject 发 STB_WEAK | DIFF(p07 正控) | 低到中 | 同 F1 | 1 到 2 单 | F1 设计 |
| F4 -c 与 -c -funit 语义拆分 | lnx 上两者同字节(简报所列,本轮未复测);多单元经系统 ld 报 __init_u 重复定义与 __init 未定义(w2a 证据 ld-multiunit BLOCKED,既有) | 用户无法分辨"标准对象"与"单元对象";系统 ld 只能链单个单元 | 合同表里写清两种产物;先加检查固定现状(同字节即登记为已知),再决定 -c 是否走标准对象 | 检查先行;行为改变时 DIFF | 中:改 CLI 语义 | UNMEASURED | 设计含在 F1,实现 2 单 | F1 设计 |
| F5 链接单元统一 | src/drivelink.cx 300 行走 tape 重降低;cx/driver/objlink.cx 693 行纯链接,头注释自述"Not wired into drive yet";另有 cx/compiler/tapelink.cx 762 行 | 链接路慢(W3 设计实测跳过段约 198 毫秒);两套符号解析与未定义检查各写一遍 | 不合并实现;先抽一张"链接合同"检查:同一组单元两路的运行输出与 rc 相同、重复强定义与未定义的拒绝文案一致;接线仍按 R2 计划 | 两路只比运行输出与 rc(W3 第 2 节已定不比字节);各路自身两次 sha 相同 | 低(只加检查);接线风险归 R2 | 防两路语义漂移 | 检查 1 单 | 无 |
| F6 环境变量与暂定定义 | modelcache 把 UNISACC_E3CACHE=0 当目录名,在仓根写出 ./0(pkg-gaps MODELCACHE-ENV-ZERO-WRITES-RELATIVE-DIR);_unisa_env 暂定定义成强定义(简报所列) | 仓根出现垃圾目录;冷缓存口径被误用 | 环境变量语义入一张事实表(名字、取值域、0 与空的含义),modelcache 读表判定 | 未设变量时 BIN16、FIX2 不变;新增负控批 | 低 | 减少误用;UNMEASURED | 1 到 2 单 | 无 |

### 2.2 结构类

| 号 | 现状(证据) | 痛点 | 做法(最小步) | 等价或对拍 | 风险 | 收益 | 规模 | 依赖 |
|---|---|---|---|---|---|---|---|---|
| S1 src 到 cx 的最终归属 | src 40 个受追踪文件,其中 7 个是指向 cx 与 ext 的符号链接;src/*.cx 合计 5854 行(tests/srcshape.tsv TOTAL);drive 族 11 个文件,drive.cx 817 行最大;prd.md 第 5 节自述"结构层是功能划分,不是已分开的文件" | 驱动逻辑分在 src/drive*.cx 与 cx/driver 两处(funit 判定在 cx/driver/route.cx 的 cxt_object,接线在 src/drive.cx 496、754 行) | 每单只搬一个无状态纯函数族到 cx/driver(先 driveargs 64 行、driveutil 26 行这类),srcshape 行数随之下降 | BIN16、FIX2、ENT3;srcshape 只降 | 中:与驱动类版本写域重叠 | src 继续变薄;速度收益 UNMEASURED | 每文件 1 单 | 与 R2 接线错开 |
| S2 驱动特判改模型步 | CLAUDE.md 定"编译行为差异写成模型步,不在 drive.cx、host.cx 写特判";srcshape 注记 ifs 仍 107/57 未再降 | 新增路线(R2 单元路)每次都要改 dv_main 分支 | 先度量:列出 drive.cx 中按目标或模式分支的 if,分类为"可入表"与"必须留";只对可入表的出单 | BIN16、FIX2、ENT3 | 中 | 减少接线单改面;UNMEASURED | 度量 1 单,其后每组 1 单 | S1 之后 |
| S3 模块私有状态普查 | 设计要求模块私有状态与生命周期;0.0.77 计划已有 unsafe 计数与 dock 审计(89 个 docked 模块) | 无量化的"谁持有全局可变状态"表 | 并入内存安全线的普查工具输出,不另起重构单 | 不改产品 | 无 | 为后续接口化提供清单 | 0(并入) | 0.0.77 |

### 2.3 表类

| 号 | 现状(证据) | 痛点 | 做法(最小步) | 等价或对拍 | 风险 | 收益 | 规模 | 依赖 |
|---|---|---|---|---|---|---|---|---|
| T1 结果表的可执行参照 | 196 张结果表只有 8 张受 check-decide 规则等价校验;objectplan-result 与 elfobject-result 无生成器、无 --check(w2a 证据 w2a2-apply 行) | W2a 的表改动只能靠人读补丁加样例验证 | 不追求给全部结果表造生成器;按近期改动频度选前 5 张(objectplan、elfobject 起),各配一个结构对拍检查(用 tools/formats-*.cx 核产物结构) | 只加检查,不改表 | 低 | 改表后分钟级得到反馈;UNMEASURED | 每张 1 单 | 无 |
| T2 d0 与 d1 共用块 | objectplan-manifest 80061 字节 91 行,d0(x86_64)与 d1(arm64)各写一份;配对节多数行数相同(s27/s28 各 130,s31/s32 各 18,s2/s3 各 17),s23/s24 与 s17/s18 已分叉 | W2a 只改了 d0,arm64 的 -funit 对象没有具名进程槽;以后每个对象合同改动都要写两遍 | 第一步只加一致性检查(先导单 P2);第二步把行数相同且归一化后相同的配对节抽成一份共用 rows,d0 与 d1 引用它;分叉节留到 F1 设计后处理 | DELTA(共用块抽取不改网);BIN16、FIX2 | 低到中 | F1 到 F4 的实现单数减半(估算) | 检查 1 单,抽取每组节 1 单 | P2 |
| T3 check-literal 改预算口径 | 189 个文件 total=3019,基线只降,新文件上限 0(tests/check-literal.cx 头注释) | W2a2 因 +3 与 +55 个立即数被拒,返工一轮把 26 个常量搬进 exec/facts/enc-objectplan.tsv,manifest 涨到 80KB | 保持"只降"不变;把计数口径从"按文件"改成"按网总量加按文件",并增加"事实常量绑定数"度量,防止把立即数换成百个绑定参数来过门;口径变更前后对全仓各算一遍并入证据 | 不改产品表;新旧口径对同一棵树的计数并列 | 中:改门口径,须独立复核,不得借机抬高任何上限 | 减少为过门而写的绑定膨胀;UNMEASURED | 2 单 | 无 |
| T4 高扇出表接口化 | tokenlocations 改一处波及 537/1476 批(nn-verify S-b);net-deps:928 个非生成器输入中 876 个被两个以上网共享(agy RSK-01,未复算) | 前端改动循环约 500 秒(简报所列,本轮未复测) | 先设计:度量 tokenlocations 各列被哪些网读,能否拆成按消费者分区的小表;设计单产出"拆后每网闭包"的预测并用 net-deps --touch 验证 | DELTA | 中高:拆错会让选批漏选 | 前端改动选批数下降;预测值待设计单给出 | 设计 1 单,实现 UNMEASURED | N4 |
| T5 手写事实表登记与清理 | 243 张 facts 只有 45 张带生成或校验标记(agy INV-03);exec/facts/derive.tsv 16 行;约 15 个孤立标量、102 张零闭合表(agy,未复算);exec/README.md 仍写 python3 gen.py(agy INV-14) | 新人与子代理按过期 README 找入口 | (1) 加 facts-unused 检查并建只降基线;(2) README 重写随下一个实质提交;(3) 零闭合表只列候选清理表,不自动删 | 只加检查;删表时 DELTA | 低 | 减少误导;UNMEASURED | 2 单 | P3 |

### 2.4 网类

| 号 | 现状(证据) | 痛点 | 做法(最小步) | 等价或对拍 | 风险 | 收益 | 规模 | 依赖 |
|---|---|---|---|---|---|---|---|---|
| N1 网体检进门 | table-lint 未登记进任何批次或棘轮(本轮复核);librarycontract 与 librarypartition 被 delta 拒载,完全跳过体检;lex 三网 44 个常驻 ERROR 源于 domain 口径不一(agy INV-05、INV-06) | W2a 证据里 table-lint 一栏先是 UNMEASURED,补测才得到 +3 告警 | 先导单 P1 建全网体检基线(只降);其后两单分别消除 2 个拒载与 44 个口径 ERROR | P1 不改产品;补 DONE 空状态行用 DELTA 之外的 BIN16、FIX2(网会变) | 低 | 表改动自带体检差值 | P1 加 2 单 | 无 |
| N2 入参寄存器标记降噪 | maybe-unwritten 9976 例占告警 72.8%;of_unit 在 object-lower 报 maybe、在 6 个 lower 网才退化为 unwritten(agy INV-08) | 真实退化被噪声淹没 | manifest 增加入参标记字段只供 table-lint 读,不进网;降噪后把 maybe-unwritten 也纳入只降基线 | DELTA(标记不进网则 delta 不变,这一点须在单内实证) | 中:若标记进了 delta 则变成产品改动 | 告警可读 | 2 单 | N1 |
| N3 真实未写与未清缓冲修复 | 5 组疑似真实未写;exec/pp/directive-body-result.tsv 85 到 86 行 INC2.f 进 KEY.S 前未 SBCLR(agy INV-07、INV-09,均未复核) | 潜在静默身份错误 | 每组先读表定性(故意读 0 的登记进允许表并写理由),确属漏写的每组一单补写 | 网会变:BIN16、FIX2、ENT3 加 sim --cov;预期产物字节不变,若变即说明此前读到了非零残值,按缺陷处理并留最小复现 | 中 | 消除静默风险 | 定性 1 单,修复每组 1 单 | N1 |
| N4 affected 按表路径选批 | exec/pipeline/table-deps.tsv 已有 5214 行 46 个模型的静态闭包,affected 已按它取 via=table 边;batch-product-inputs.tsv 70 行,大量批次保守声明整个 exec/;register 不写也不核产品输入声明(pkg-gaps REGISTER-NO-PRODUCT-INPUT-DECL) | 七批到运行时才报 BATCH_PRODUCT_INPUT_UNDECLARED;改一张表选中的批过多 | (1) P3 度量每张表的直接覆盖;(2) register 登记时写并核 id 行;(3) 只对已有直接覆盖的表逐条收窄声明,每条收窄附新旧选择集对比 | 不改产品;新旧选择集差集逐条列出并说明 | 中:收窄过头即测试真空,必须先有 P3 基线 | 改表后重跑批数下降;UNMEASURED | 3 单 | P3 |
| N5 跨目标一致性自动化 | lower 与 elf 各 6 目标一网;object-enc-x86_64 为 2540 状态,arm64 为 2542(w2a2-table-lint 行) | 目标间差异靠人记 | 工具输出"同阶段各目标网"的状态数、边数、寄存器集差集,差集入允许表带理由;P2 是它在 objectplan 上的特例 | 不改产品 | 低 | 新分叉当场可见 | 1 到 2 单 | P1、P2 |

## 3 (C) 排序与分期

### 3.1 前八项(按"小、安全、立刻减少返工"排序)

| 序 | 项 | 理由 | 版本槽 |
|---|---|---|---|
| 1 | N1(含 P1) | 一切表改动的体检底座;不改产品 | 即刻,main 工具类 |
| 2 | T2 第一步(P2) | 分叉仍在扩大(44 到 48),越晚越贵 | 即刻,main 工具类 |
| 3 | N4 第一步(P3)与 register 核声明 | 已有七批返工实例 | 即刻,main 工具类 |
| 4 | T3 | 已造成一轮返工与 80KB manifest | 0.0.77 并行,工具与测试类 |
| 5 | T1(objectplan、elfobject 两张起) | 为 F1 到 F4 备 oracle | 0.0.78 并行,工具类 |
| 6 | F1 设计单(含 F2、F3、F4 的合同表) | 只读设计,不占候选 | 0.0.78 并行 |
| 7 | T2 第二步、N2、N3 | 字节等价表重构,需要候选 | 表网卫生专版 |
| 8 | F5 检查、F6 | 小,防漂移 | 随 R2 收尾版 |

### 3.2 与已排版本的关系

说明:build/version-seq.tsv 在 main 上仍写"0.0.76 内存安全乙期、0.0.77 netraw",与当前实际(0.0.76 发布进行中,0.0.77 与 0.0.78 为内存安全一段、二段)不一致,入仓前应先更新该表;下表按当前实际序列写。

| 版本槽 | 已排内容 | 本计划插入 | 写域 | 是否冲突 |
|---|---|---|---|---|
| 0.0.77 内存安全一段 | 只改 cx/cx0/plan.cx,经 cx0-extract 投影到 src/cxmod.c 与 cx/compiler/cxmod.cx | P1、P2、P3、T3、register 核声明,全部在 main 上作工具类 | tools/、tests/ | 不冲突:不触产品输入,不占候选;共享表用 commit-own --keyed 只带自己的行 |
| 0.0.78 内存安全二段 | 同上三源 | T1、F1 设计单、N5 | tools/、tests/、plans/ | 不冲突,同上 |
| 新增:表网卫生专版(建议排在内存安全二段之后) | 无 | T2 第二步、N1 的两张清零单、N2、N3 | exec/enc、exec/lower、exec/pp、exec/lex、exec/library*、tools/table-lint.cx | 与内存安全线写域不相交(后者不碰 exec/);一类大改 = 字节等价表重构 |
| netraw、诊断二期 | 各一类 | 不插入 | 无 | 无 |
| R2 收尾与对象产出缺陷 | 驱动类加对象产出 | F1 到 F4 实现、F5、F6;建议把"对象产出合同"从 R2 收尾里拆成单独一类,R2 收尾只留接线 | exec/enc、exec/lower、src/drive*.cx、cx/driver | 须拆成两版,否则一个候选含两类大改 |
| 链接单元二期 | 驱动类 | S1、S2 随行 | src/、cx/driver | 同类,可同槽 |

为什么单开专版而不是塞进现有槽:T2、N2、N3 都改 exec 产品输入,需要一条完整慢车道;与内存安全同候选会违反一类大改,与对象产出同候选会让"字节不变"的验收失效(对象产出本来就要变字节)。专版的验收恰好最简单(DELTA 加 BIN16 全等),预计开发关键路径不超过 4 张单 160 分钟,整体不超过 2 小时线的风险点只在链尾(0.0.75 实测链尾 83.5 分钟),超线即把 N3 拆到下一版。

### 3.3 不建议做

| 项 | 理由 |
|---|---|
| 网的拆分与合并(例如拆 parse2 的 450 张表) | 无返工实例指向网粒度;42 网是构造链与 chain-facts 的既定基数(tools/check-chainfacts.cx 断言 cf_net_count()==42),改基数波及构造链全部相位 |
| 合并 cx/compiler 与 cx/runtime 的 24 对同名生成模块 | 本轮复核:24 对同名、逐字节相同的 0 对,均由 tools/axis-gen.cx 自同一事实表生成;重复由生成器承担,合并要改层向规则,收益小于风险 |
| 为 359 张规则表逐张补单测 | 规模不可控;改为 N4 的"按改动频度补",先补近三十天改过的表 |
| 给全部 189 张结果表造生成器 | 结果表是决策与机制本身,多数没有更上游的事实;T1 用结构对拍替代 |
| src 目录整体搬进 cx | 一次性大搬迁违反分治时限;按 S1 逐文件做 |

## 4 (D) 重构单验收模板

以下可直接并入 plans/briefs 的模板,作为"重构单"口径块;每项写实测值,未跑写 UNMEASURED 并不得记通过。

| 号 | 项 | 命令或做法 | 通过判据 |
|---|---|---|---|
| R1 | 输入身份 | 记开工提交号、已装产品 sha256、开发产品 sha256 | 三项齐全 |
| R2 | 网字节等价(DELTA) | 改前改后对受影响网导出 delta JSON 后 cmp;受影响网用 `sh ./unisacc.com tools/net-deps.cx -- --touch 改动表` 列出 | 全部相同;不同即转 R3 到 R5 全做并在单内说明哪些状态变了 |
| R3 | 整程序字节等价(BIN16) | 17 份 examples/apps/*.c 各 -o lnx/x86_64,改前改后 sha256 并列入证据 TSV;改动涉及其它目标时加该目标 | 17/17 相同 |
| R4 | 两代不动点(FIX2) | 开发产品自编一代,再自编一代,比 sha256 | 两代相同 |
| R5 | 三入口一致(ENT3) | 同一源经 -run、-o 后执行、-c -funit 加链接,比 stdout 与 rc | 三者相同 |
| R6 | table-lint 前后 | 受影响网逐网记 states、edges、registers、errors、warnings 前后值 | errors 不增;warnings 增量逐条解释 |
| R7 | check-literal 前后 | `sh ./unisacc.com tests/check-literal.cx -- --check` 前后末行 | total 不增,不改基线上限 |
| R8 | 选批覆盖前后 | `sh ./unisacc.com tools/affected.cx --files 改动路径` 前后选中批次集合 | 改后集合包含改前集合,或差集逐条说明 |
| R9 | 冷缓存批次门 | 新增或改动的批在全新布局、UNISACC_RUN_CACHE=0 UNISACC_UNIT_CACHE=0 下实跑 | 不超过 33 秒 |
| R10 | 棘轮与 sim | `sh ./unisacc.com tools/ratchets.cx`;改网时加 sim --cov | 全过;覆盖不降 |
| R11 | 行为有意改变时 | 不适用 R2、R3;改用 DIFF 正负控(gcc 加系统 ld),并列出"必须不变"的范围(例如整程序 -o 路仍 17/17 相同) | 正控通过、负控被拒、不变范围全等 |
| R12 | 回滚 | 写明撤回哪一个提交即恢复 | 一条命令可回滚 |

禁区沿用现模板:不删断言、不缩覆盖、不放宽时限、产品输入变更补丁先行、不提交他人路径。

## 5 (E) 风险与防护

| 风险 | 表现 | 防护 |
|---|---|---|
| 重构吞掉产品线 | 版本槽被重构占满,功能版顺延 | 只开一个专版;其余全部是不占候选的工具类单;专版开发关键路径封顶 4 张单,超出即拆 |
| 静默行为变化 | 表改动后网变了但样例没覆盖到 | R2 的 DELTA 作第一判据;DELTA 不同就必须过 R3 到 R6 全套并解释状态差;N3 这类预期"网变而产物不变"的单,产物一旦变化按缺陷处理 |
| 表网体量大难评审 | 137282 状态无法靠人读 | 评审对象从"表"改为"差值":每单附 table-lint 前后差、delta 的状态与边增减数、net-deps --touch 的受影响网清单;单次改动只动一个配对节或一组寄存器 |
| 收窄选批造成测试真空 | 改表后没有任何批被选中 | P3 基线先行;N4 每条收窄必须附新旧选择集;禁止收窄尚无直接覆盖的表 |
| 门口径变更被用来放水 | T3 改口径时总量上限被抬高 | 新旧口径并列计数入证据;核心路径独立复核;任何上限数值不得上调 |
| 与 devsvc 速度线互相影响 | 新检查拖慢提交门;dvqualify 每次重建布局(pkg-gaps DVQUALIFY-REBUILDS-LAYOUT-PER-CALL) | 新检查一律登记为批次由 affected 选跑,不进提交门常跑集合;42 网体检分片并行,每片不超过 55 秒;P1 输出同时提供给 devsvc 作 flow 节点的候选 |
| arm64 行为全程未实跑 | T2 抽共用块后 arm64 对象变化无人验证 | T2 第二步只抽 DELTA 相同的节;arm64 运行行为在证据里保持 UNMEASURED,不写通过 |
| 证据脚本不可复现 | agy 的统计脚本在 /tmp | P1、P3 把统计沉淀成仓内带检查的工具,基线数字以工具重算为准 |

## 6 (F) 三张零风险先导单

三张单都只加检查与度量,不改 src、cx、ext、include、exec、weights、seed、kernel;都在 main 上;工具名为建议名,开工第一步先跑 `git log --all --oneline -- tools/NAME` 查名字历史,有历史就另选名;每个新工具带 check、登记批次、在 prd.md 能力速查补一行。预期值中的数字来自 agy 报告或本轮复核,以工具首次实测为准,不符时如实记录差异,不得为对上数字改口径。

| 单 | 目标 | 写域 | 验收命令与预期 |
|---|---|---|---|
| P1 网体检基线 | 把 42 网 table-lint 汇总做成只降基线,替代 /tmp 里的一次性脚本 | tools/net-health.cx、tools/check-net-health.cx、tests/net-health.tsv(基线)、tests/manifests/net-health.tsv、prd.md 一行 | `sh ./unisacc.com tools/net-health.cx -- --check` 末行形如 `NET_HEALTH nets=42 errors=44 unwritten=52 sb_read=46 load_err=2 status=PASS`(数字待首测);负控:临时副本里给一张表加一个未写寄存器读,--check 必须 rc 1 并点名;分片后每片不超过 55 秒;冷缓存全新布局不超过 33 秒(超出即按网分批登记) |
| P2 d0 与 d1 一致性检查 | 固定 objectplan 两目标配对节的现状,新分叉当场报 | tools/check-objectplan-pair.cx、tests/objectplan-pair-allow.tsv(已知分叉,每行带理由与到期版本)、tests/manifests/objectplan-pair.tsv、prd.md 一行 | `sh ./unisacc.com tools/check-objectplan-pair.cx -- --check` 末行形如 `OBJECTPLAN_PAIR pairs=N same=N known_diff=2 new_diff=0 status=PASS`;main 上已知分叉为 d0.s23/d1.s24(44 对 40)与 d0.s17/d1.s18;负控:副本里删 d1 某节一行必须报 new_diff=1 |
| P3 表级覆盖度量 | 每张 exec 表算出进几网、有几批直接声明、rel impact 批数,建立"无直接覆盖"只降基线 | tools/table-cover.cx、tools/check-table-cover.cx、tests/table-cover.tsv(基线)、tests/manifests/table-cover.tsv、prd.md 一行 | `sh ./unisacc.com tools/table-cover.cx -- --check` 末行形如 `TABLE_COVER tables=1102 in_nets=N zero_direct=955 status=PASS`(955 待首测复现);`--touch exec/facts/tokenlocations.tsv` 输出的批数与 `devsvc rel impact` 的 537 一致;负控:基线里把某表的覆盖数改大必须 rc 1 |

三张单互不依赖,写域不相交,可同时派发;每张不超过 40 分钟,子代理单按 15 分钟一段拆。

## 7 未验证与待办(入仓时并入 prd_mempalace.md 的未验证项)

- agy 的全网统计(137282、9976、955、102、15)未在仓内重算,以 P1、P3 首测为准。
- "前端改动循环 500 秒""sim 两侧耗时""-c 与 -c -funit 在 lnx 同字节""_unisa_env 强定义"引自任务简报,本轮未复测。
- unit-frontcache equiv0 的 deps=DIFF 是既有缺陷(pkg-gaps UNIT-FRONTCACHE-EQUIV0-CX-DEPS-DIFF),不属于本计划任何重构单;它使 R5 三入口项在 @cx 单元上有一条已知失败,模板执行时须单列,不得掩盖。
- arm64 的所有运行行为 UNMEASURED。
- build/version-seq.tsv 与当前实际版本序列不一致,入仓前更新。
