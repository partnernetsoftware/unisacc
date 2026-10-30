# 0.0.43 完成门缺口草稿（cc，2026-10-28）

范围：六个MISSING大域（library、stream-e3、stream-elf、resources、ri、negative，共168格）逐格核对，以及全部39个BLOCKED格的裁定表。
性质：只读核对草稿。未编译、未实跑、未安装，没有改任何矩阵，也没有任何格记为PASS。分类是建议，换标仍须实跑批在同一已装候选上PASS。
输入身份：已装产品 unisacc.com SHA前缀79fb84e6；矩阵读于提交207195a597至de85b88a98之间，六域矩阵在此区间未变。
原始分节：out/c43-cc-gate-gaps-01/{library,streams,resources-ri,negative,blocked}.md。

## 一、汇总

分类口径：A可落地（入口已有、独立预期来源已有、可扩展现有批或工具、材料现成）；B需材料；C需新工具；D实为产品缺口（应改NEEDS_PRODUCT_CHANGE）；E需裁定。

| 域 | 格 | A | B | C | D | E |
|---|---|---|---|---|---|---|
| library | 34 | 23 | 3 | 0 | 2 | 6 |
| stream-e3 | 33 | 20 | 1 | 8 | 0 | 4 |
| stream-elf | 22 | 10 | 8 | 2 | 0 | 2 |
| resources | 31 | 17 | 0 | 12 | 2 | 0 |
| ri | 26 | 21 | 1 | 0 | 1 | 3 |
| negative | 22 | 18 | 1 | 1 | 0 | 2 |
| 合计 | 168 | 109 | 14 | 23 | 5 | 17 |

39个BLOCKED：R真需主人裁定18格，合并为4个问题；T技术前置可解除4格；F外宿主通道12格；S状态应改5格。

## 二、优先可落地（按收益排序）

1. **重跑negative-hostile一个批，可绑定21格。** 它覆盖negative的11个hostile格和resources的10个res.hostile格。唯一实跑在旧Root7a上，这21格断言当时全绿，整批红只因self-containing-struct的4项断言。证据不能迁移，需在当前已装候选上重跑。self-containing-struct见E项"incomplete"口径。
2. **登记并实跑tests/library-cells-lnx-x86_64.cx。** 文件已提交但未登记进tests/batches.tsv，可直接覆盖lib.ffi、lib.cb.wrapper-index-beyond-10、lib.layout.api-structs等格。
3. **扩展p3-library-members夹具，覆盖约20个library格。** 输入/编译/顺序拒绝都是字面串预期，运行、绑定、生命周期各加一个phase。生命周期和并发格需要在构建行加-lpthread并读/proc计数。
4. **ri的21个A格集中在三个已有测试上扩展。** 分别是tests/resources.cx、tests/ri-image-single.cx、tests/ri-route-refuse.cx。
5. **stream-e3的20个A格和stream-elf的10个A格，扩展现有stream批即可。** 明细见文末分域明细。
6. **negative loader的4格，扩展tests/negative-model-loader.cx。** 已装包头为`P 3 32 1479 74`，脚本硬编码的32/1479仍匹配，只需另解析74条资源行。

## 三、需cdx处理的产品缺口（D，5格）

- **resources res.steps.stream-malformed-budget：** exec/c/run.c:1061用strtol读预算，不检查结尾和范围，abc、-1、0会被静默转换。cdx工作区当前正在改run.c，请顺带核对。
- **resources res.disk.output-fsize：** 短写失败后不删除输出文件，会残留截断文件。
- **ri ri.id.lower-stage-cli：** lower阶段捕获只存在于独立run主程序，产品CLI只有e3和source两个入口。
- **library lib.in.us_add_file：** libunisacc.c:302把ENOENT、EACCES、ENOTDIR都报成同一句"cannot open source file"，违反只把ENOENT当不存在的规则。
- **library lib.compile.us_compile.invalid-options：** libunisacc.c:689只查NULL和优化级别，未知target要到:720 discard_image之后才失败，没有在入口具名拒绝。

## 四、需裁定项

- **39个BLOCKED合并后的4个主人问题：** 是否授权Actions作外宿主通道并拆分完成门；win/arm64的合同是仿真x86_64 PE还是原生ARM64 PE；步数和时延预算如何定合同；RI在-run和-c上的范围。每题的选项和推荐见文末BLOCKED表下的"需主人一次性裁定的问题"。
- **MISSING中的17个E格：** 多数是格的要求和现行代码或独立参考对不上。例子有negative的self-containing-struct要求出现"incomplete"一词，而独立参考写的是"a struct cannot contain itself"；还有ri的identity-malformed、ledger-path-too-long、source.EMPTY-ABSENT。这些多数可由矩阵主人改写要求解决，不一定上升到主人。

## 五、可直接解除的BLOCKED（不需裁定）

- **4格修复已装、待纪元实跑：** bootstrap E7.refusal-registry、negative loader.terminal-elf-stage、product.as-target-header-conflict、product.attr-strip-token-len。修复提交9df74fd151、48b5be2a4c都已在79fb84e6内，应改为MISSING/NOT_RUN并在下一纪元实跑。
- **negative loader.certificate-ape：** "签名禁止"已于10-28撤销，且该格只测证书表定位，可用合成证书夹具。prd.md:201和plans/v0.0.43.md:83仍写着签名禁止，需文档主人修正。
- **library lib.std.family-classification：** 主人已在research/libc-forward-handoff.md授权cc安排，可直接产出事实表、生成器和探针。

## 六、矩阵引用漂移（只记录，不改矩阵）

多处source列行号已漂移。例子：compiler.c:519/525/904实为596/602/1031，run.c:1046/1054实为1061/1069，spec.md的A-47/A-39/TP-8/TP-9实在784/776/840/842行。resources矩阵头注"run-batch把超时记成FAIL"已不成立：自97a3f4472a起超时输出UNKNOWN、rc3。逐格更正写在各分节里，由矩阵主人在换标时一并改。

---

# 分域明细

## library（34格）
小结：A可落地 23；B需材料 3；C需新工具 0；D实为产品缺口 2；E需裁定 6（均未实跑，无PASS）。

说明：C-API格唯一已登记载体是 `p3-library-members`（tests/p3-library-members.cx 以宿主GCC11+libffi构建 tests/cases/p3_library_members.c，非封闭测试设施）；下表"p3夹具"即指在该夹具加新phase、在批中加期望行。`tests/library-cells-lnx-x86_64.cx` 已提交（13d8cfe478）但未登记 tests/batches.tsv，也未见运行证据。归档 `archive/testing-c43-legacy/tests/` 的 liblatebind2b.c、liblatebind2c1.c、librarydata-cases.tsv、libunisacclifecycle.c 只能当迁移参考或输入材料，不能回调旧gate。

| 格 | 现有API/入口 | 独立预期来源 | 可复用批/工具 | 分类 | 下一动作 |
|---|---|---|---|---|---|
| lib.ctx.us_new.invalid | libunisacc.c:266 us_new只拒NULL，路径惰性到:192 library_package | 手推NULL；/proc/self/fd计数 | p3-library-members | E需裁定 | 裁定：坏包在us_new拒绝（产品改）还是在compile/contract具名拒绝（改要求） |
| lib.ctx.us_free | libunisacc.c:273；discard_image :244 | NULL无操作；/proc/self/fd、maps计数前后相等 | p3-library-members（参考归档libunisacclifecycle.c） | A可落地 | p3夹具加free phase：us_free(NULL)，并在前后计fd/maps |
| lib.ctx.concurrency | _Thread_local状态 libunisacc.c:92-98,173 | 字面答案1027/2037（同compile phase） | p3-library-members | A可落地 | p3夹具加pthread双线程phase重复N次，构建行加-lpthread |
| lib.ctx.reentry | :1253 execution during compilation…；:337 recursive query；:691 recursive compilation | 字面us_error串 | p3-library-members | A可落地 | 注入宿主函数在导出内回调us_compile/us_binding_contract，断言具名串 |
| lib.in.us_add_source | libunisacc.c:291 "missing source"；us_error(NULL)="null context" :330 | 字面串 | p3-library-members | A可落地 | p3夹具加NULL ctx/name/source三拒绝断言 |
| lib.in.us_add_file | libunisacc.c:302；ENOENT/EACCES/ENOTDIR同为"cannot open source file" | CLAUDE.md文件存在性规则 | p3-library-members | D实为产品缺口 | 标NEEDS_PRODUCT_CHANGE：按errno区分错误串后再测 |
| lib.in.us_add_tape | libunisacc.c:313只拒NULL/混用；坏tape在relocate才拒 | 无独立来源（库tape≠CLI -S，去进程包装） | p3-library-members | E需裁定 | 裁定tape来源（CLI -S还是us_tape往返）及坏tape拒绝点 |
| lib.in.us_define | libunisacc.c:318 "invalid definition"（NULL或含\n） | 手推宏值字面答案（不用CLI -S字节） | p3-library-members | A可落地 | 改判据为us_define(N=v)后导出返回v，并加含\n拒绝 |
| lib.in.us_include_path | libunisacc.c:325 "invalid include path"（仅NULL；单路径替换） | 手写头内常量字面答案 | p3-library-members | A可落地 | 夹具在out写头，us_include_path后导出返回常量，并加NULL拒绝 |
| lib.in.us_set_signature_version | libunisacc.c:478 "invalid signature version configuration" | 字面串；改值后us_sym报library is not relocated | p3-library-members | A可落地 | 断言2/3接受、0/4拒绝，改值后us_sym为NULL直到重编 |
| lib.compile.us_compile.invalid-options | libunisacc.c:689只查NULL/级别；未知target在:720 discard_image后才失败 | 字面串invalid compilation options | p3-library-members | D实为产品缺口 | 标NEEDS_PRODUCT_CHANGE：未知target须在discard前拒绝；越界O级可先测 |
| lib.compile.us_compile.language-error | :207-210把模型错误写入us_error | tests/negative-diagnostics.cx手钉file:line:col | p3-library-members+negative-diagnostics语料 | A可落地 | 取一条negative语料喂us_add_source，断言位置串（格式未核） |
| lib.compile.us_tape | libunisacc.c:331；const ctx无法写错误 | 无独立来源（库tape≠CLI -S，归档靠Python去包装） | p3-library-members | E需裁定 | 裁定us_tape字节判据与编译前"具名错误"要求是否成立 |
| lib.compile.us_relocate.order | :1120 "no compiled tape"；:1340 "library is not relocated" | 字面串 | p3-library-members | A可落地 | 夹具加编译前relocate与relocate前us_sym两条拒绝断言 |
| lib.compile.cross-target | :1121 "library execution target differs from host"；:695,:712 resolver分支（prd.md:112引用错误） | 字面串 | p3-library-members | A可落地 | 以非宿主target编译后relocate断言具名拒绝，修正引用 |
| lib.run.us_run_main | libunisacc.c:1396 "invalid main arguments"/"no main function in library" | 手推argc/返回字面 | p3-library-members | A可落地 | 加main返回argc+exit路径phase，并加argc<0拒绝 |
| lib.run.us_sym.invalidation | libunisacc.c:1339→us_exports_symbol_frame | 未知名有字面串；旧指针"不再返回"无安全观测量 | p3-library-members | E需裁定 | 裁定可观测判据，未知名NULL部分先落地 |
| lib.run.us_sym_typed | libunisacc.c:1370；"invalid concrete export declaration" | exec/c/librarysignature-v2.md记录格式手编 | p3-library-members | B需材料 | 按v2文档手编USLSIG2 ALL_STACK记录并审查后接入夹具 |
| lib.run.us_call_status | libunisacc.c:1391 | 手推状态对(0,-)/(1,exit码) | p3-library-members | A可落地 | 导出正常返回与exit(3)两例断言状态对 |
| lib.err.us_error-catalogue | libunisacc.c:262 error()；串散在全文件 | 无独立来源：头文件不含任何错误串 | p3-library-members | E需裁定 | 裁定"文档化失败"清单（建事实表），再由各格汇总 |
| lib.bind.us_add_symbol.invalid-and-data | libunisacc.c:485 "invalid symbol parameters"/"invalid data symbol declaration" | 字面串；手推数据值 | p3-library-members | A可落地 | 加count>1024拒绝、kind1数据注入读取、注册后us_sym为NULL断言 |
| lib.bind.us_declare_import | libunisacc.c:503 | libc abs/strlen手推结果；字面串 | p3-library-members | A可落地 | 声明abs后脚本调用断言结果，加失败保持旧代断言 |
| lib.bind.injected-import-fragments | libunisacc.c:514,546-547；us_late_query librarylatebind.h:343 | spec.md LB-4/LB-5具名错误；UNILOW1魔数 | p3-library-members（参考归档liblatebind2b.c） | A可落地 | 迁移2b用例到夹具：无provider时relocate具名、片段魔数/NULL+0 |
| lib.bind.injected-data | libunisacc.c:523,531,544-545；librarylatebind.h:104-111 | spec.md LB-6/7/8具名串 | p3-library-members（材料：归档librarydata-cases.tsv、liblatebind2c1.c） | A可落地 | 迁移2c1数据用例：type/extent/missing-provider/readonly-write具名断言 |
| lib.bind.typed | libunisacc.c:549,555；2c2默认产品未启用（research/c43-latebind-status.md） | exec/c/librarysignature-v2.md、librarybinding-v3.md | p3-library-members | B需材料 | 手编USLSIG2/3记录（含1024/1025边界），核PC包是否含typed模型 |
| lib.bind.us_load_library | libunisacc.c:560 "invalid library load" | 宿主libm.so.6函数手推值；maps计数 | p3-library-members | A可落地 | 加载libm后调用cbrt断言结果，加坏路径拒绝及free后maps计数 |
| lib.bind.us_binding_contract.failures | libunisacc.c:335-341 "unknown target"；"required model is missing"（librarycontract路径） | spec.md LB-3字面串 | p3-library-members（已构造M30翻转包） | B需材料 | 构造缺M30成员的PC包；未知target部分可直接测 |
| lib.ffi.unisacc_ffi | include/unisacc_ffi.h:74-253 | 文件头手推：strlen=12、abs=7、snprintf、RFC1950 adler32、glibc dlerror | tests/library-cells-lnx-x86_64.cx（未登记） | A可落地 | 将library-cells登记入batches.tsv并实跑 |
| lib.cb.host-to-script-api | us_sym :1339 返回原生可调指针 | 手推排序结果字面 | p3-library-members | A可落地 | 脚本导出比较器经us_sym交宿主qsort，断言排序输出 |
| lib.cb.wrapper-index-beyond-10 | 产品-run回调包装（tests/callback-wrapper-index.cx仅到10） | 手推"CBW101\t203" | tests/library-cells-lnx-x86_64.cx（未登记） | A可落地 | 登记library-cells并实跑101比较器用例 |
| lib.layout.api-structs | exec/c/libunisacc.h:29-40 | x86-64 psABI手推48/96及偏移 | tests/library-cells-lnx-x86_64.cx（未登记） | A可落地 | 登记library-cells并实跑布局断言 |
| lib.layout.host-headers | include/sys/stat.h、include/dirent.h:10-14 | glibc bits/struct_stat.h公开偏移 | tests/library-cells-lnx-x86_64.cx（dirent仅OBSERVED） | E需裁定 | 裁定携带dirent（无d_off/d_reclen）是否须等于glibc；stat/timespec先跑 |
| lib.own.borrowed-snapshots | libunisacc.h:64-75,107-110；:331,:335,:343 | 事件前后字节副本比较 | p3-library-members | A可落地 | 夹具复制合同/片段/tape字节，于失效事件前后比对 |
| lib.own.leak-cycles | libunisacc.c:273 us_free | 预热后fd/maps计数相等 | p3-library-members（参考归档libunisacclifecycle.c） | A可落地 | 先测单轮耗时定N以守55s预算，再加循环计数phase |

### 优先可落地
- 登记并实跑 `tests/library-cells-lnx-x86_64.cx`（已有，未登记batches.tsv）：lib.ffi.unisacc_ffi、lib.cb.wrapper-index-beyond-10、lib.layout.api-structs（另附host-headers中stat/timespec部分，dirent待E裁定）。
- 扩展 `p3-library-members`（tests/cases/p3_library_members.c 新phase + tests/p3-library-members.cx 期望行）：
  - 输入/编译/顺序拒绝（纯字面串）：lib.in.us_add_source、lib.in.us_define、lib.in.us_include_path、lib.in.us_set_signature_version、lib.compile.us_relocate.order、lib.compile.cross-target、lib.compile.us_compile.language-error。
  - 运行：lib.run.us_run_main、lib.run.us_call_status、lib.cb.host-to-script-api。
  - 绑定：lib.bind.us_add_symbol.invalid-and-data、lib.bind.us_declare_import、lib.bind.us_load_library、lib.bind.injected-import-fragments、lib.bind.injected-data。
  - 生命周期/并发（需构建行加-lpthread并读/proc）：lib.ctx.us_free、lib.ctx.concurrency、lib.ctx.reentry、lib.own.borrowed-snapshots、lib.own.leak-cycles。

## stream-e3（33格）

分类：A可落地 20 / B需材料 1 / C需新工具 8 / D实为产品缺口 0 / E需裁定 4（无一格记PASS；“换标”仅指已有批的实跑断言可直接引用）

| 格 | 现有API/入口 | 独立预期来源 | 可复用批/工具 | 分类 | 下一动作 |
|---|---|---|---|---|---|
| e3.id.same-candidate-model | streams.json e3条目(模型4)+候选P3 | 候选P3成员SHA(包解码器) | tools/stream-model-identity.cx(批stream-model-identity) | A | 新登记units=e3参数行，在da4e材料+候选上实跑 |
| e3.det.file-vs-stdin | stream-runtime INPUT“-”；tools/streams.md:41-43(原引:33-35错) | 两传输字节相等 | interfaces-lnx-x86_64 已断言 case0/1.e3.stdin-route.edge-bytes-equal-file-route | A | 以interfaces@da4e 283/0实跑证据换标，与elf同格同式 |
| e3.det.bare-vs-framed | stream-runtime attr_strip；formats.md:20-29 | 字节相等 | interfaces 已断言 case0.e3.framed-and-bare-unitok1-same-tape | A | 直接换标(引interfaces-lnx-x86_64.cx:160-161) |
| e3.det.label-independence | argv[2] SOURCE_LABEL；tools/streams.md:41-42、formats.md:129(原引:54错) | 两标签tape字节相等；诊断含标签 | tools/stream-io-cases-gen.cx 加e3模式 | A | e3模式：good.tok用两标签跑比tape，错例比诊断文件名 |
| e3.in.unitok2-multi | 多单元路由e3同为模型4(=stream3)；formats.md:251-276(原引:176-200错) | 手构双单元UNITOK2+执行结果(串行器同unitlocationcheck.cx:353-411) | tests/stream-e3.cx 或 stream-semantics 加e3 | A | 手构UNITOK2经stream3→4..7执行；未核stream3接受UNITOK2及static后缀约定 |
| e3.in.unitok2-frame-refusals | E3 UNITOK2帧检查(formats.md:270-272) | 坏帧清单 unitlocationcheck.cx:425-437；名字未声明 | stream-e3.cx | E | 裁定名字：units-reject.tsv属units阶段，E3仅有tokenlocations-reject名 |
| e3.variant.warnings | warn路由e3=模型26，不在stream3；streampack只选image/LEVEL | 归档参考诊断文本(未核位置) | tools/cxlib/streampack.cx | B | streampack加warn路由变体打容器，再配-Werror资源钩子 |
| e3.pos.return7-exact-independent | stream3 | interfaces tape_oracle 592B按front_parse.c手推 | interfaces 已断言 case0.e3-e4.equals-reference-derived-literal-tape(+:158手构输入) | A | 换标引interfaces:159；或补REF -O0 -S字节对拍 |
| e3.pos.decl-scope | stream3(prd.md:711-721) | 手推执行结果；REF -O0 -S tape | tools/stream-semantics.cx 加e3模式；opt-corpus-gen语料 | A | e3模式：源→stream2/1/3→6/7执行比字面值 |
| e3.pos.integer-arith | 同上 | C99手推执行值 | stream-semantics e3模式 | A | 加有/无符号提升与转换程序及手算退出码 |
| e3.pos.pointers-arrays | 同上；rank≤8(shape-reject.tsv) | 手推执行值 | stream-semantics e3模式 | A | 加指针算术/多维数组/sizeof程序 |
| e3.pos.aggregates | 同上 | 手推执行值 | stream-semantics e3模式 | A | 加struct/union/enum/位域/柔性数组程序 |
| e3.pos.control | 同上 | 手推执行值 | stream-semantics e3模式 | A | 加if/循环/switch/goto程序 |
| e3.pos.calls | 同上 | 手推执行值 | stream-semantics e3模式 | A | 加函数指针/变参/intrinsic程序 |
| e3.pos.initializers | 同上 | 手推执行值 | stream-semantics e3模式 | A | 加标量/聚合/静态/字符串初始化程序 |
| e3.pos.vla | 同上；exec/parse2/vla-reject.tsv | 手推执行值 | stream-semantics e3模式 | A | 加VLA维度程序(sizeof与下标) |
| e3.pos.float-literals | E3 limb解码(prd.md:716) | IEEE-754手算位型(含次正规) | stream-semantics e3模式 | A | 程序memcpy取位型按字节输出，比手算常量 |
| e3.gold.parse-340 | weights/gold/parse.tsv(340键在) | parse.tsv y | 无；仿tools/peep-cases-gen.cx | C | 写parse-cases-gen：每键构造输入，接受/拒绝对表，不可实现记UNREALIZABLE |
| e3.gold.type-4275 | weights/gold/type.tsv(4275键) | type.tsv y+tyinfo | 无 | C | 写type-cases-gen分片批(sizeof/符号执行探针) |
| e3.gold.scope-30 | weights/gold/scope.tsv | scope.tsv y | 无 | C | 写scope-cases-gen，每(ctx,kind)一程序 |
| e3.gold.irsel-420 | weights/gold/irsel.tsv | irsel.tsv y(tape op名) | 无 | C | 写irsel-cases-gen，查tape含对应op |
| e3.gold.tyinfo-16 | weights/gold/tyinfo.tsv | tyinfo size/uns | 无 | C | 写小生成器：sizeof与回绕执行值 |
| e3.gold.pfconv-9 | weights/gold/pfconv.tsv | pfconv y+打印字节 | 无 | C | 生成printf用例；未核流链镜像是否带printf运行库 |
| e3.gold.binsel-32 | weights/gold/binsel.tsv | binsel y+边界操作数执行 | 无 | C | 生成有/无符号div/mod/比较/移位边界程序 |
| e3.gold.prec-19 | weights/gold/prec.tsv | prec y | 无 | C | 生成括号敏感表达式对，比执行值 |
| e3.lim.capacities | prd.md:719；shape-reject.tsv:reject0/5 | spec限值；token跨度2^26=64MiB超60s | 无 | E | 裁定拆分：rank/struct池可做，2^26跨度不可在时限内实跑 |
| e3.ref.attr-count-overstated | run.c适配器；formats.md:52-58 | 文档定为`run: malformed USLATTR1 side-car` rc2 | interfaces 已断言 attribute-count-overstated(ATTR_SIZE_REJ) | E | 需求写的'malformed constructor attributes'与合同冲突，裁定改需求后换标 |
| e3.ref.diag-stream-boundary | stream3错误路线；formats.md:278-293(原引:212-214错) | negative-diagnostics.cx内归档diag字面 | tests/stream-e3.cx 复用negative-diagnostics夹具 | A | 同源label跑stream3，比完整stderr、rc1、空stdout |
| e3.ref.unmapped-limitation | formats.md:289-291(原引:213-216错) | exec/parse2/*-reject.tsv声明名 | tests/stream-e3.cx | A | 选shape/vla拒绝输入，断言定位+not covered名+无tape |
| e3.ref.hostile | spec.md:784 A-47 | spec；1000层括号值可手算 | stream-e3.cx；移植negative-hostile.cx夹具 | A | 加深括号/自含struct/截断三例；自含struct措辞可能成产品发现 |
| e3.ref.missing-input | run.c:157-178 io_open_reason | ENOENT/EACCES/ENOTDIR运行合同 | tools/stream-io-cases-gen.cx 加e3模式 | A | e3模式跑四类坏路径；未核da4e容器底座含io_open_reason |
| e3.rec.after-refusal | stream3；interfaces已覆盖帧恢复(PARTIAL) | 592B字面tape | stream-e3.cx/stream-io-cases-gen e3 | A | 每个语义/诊断拒绝后跑good.tok比592B字面tape |
| e3.host.lnx-x86_64 | 汇总格 | 本矩阵 | 聚合 | E | 汇总；受上列E/C格阻塞，余格闭合后按聚合证据换标 |

优先可落地（A类，按批/工具）：
- 直接换标（interfaces-lnx-x86_64@da4e 已实跑断言）：e3.det.file-vs-stdin、e3.det.bare-vs-framed、e3.pos.return7-exact-independent。
- tools/stream-model-identity.cx（加units=e3行）：e3.id.same-candidate-model。
- tools/stream-io-cases-gen.cx（加e3模式，夹具=stream-e3.cx hand_input，oracle=592B字面tape）：e3.ref.missing-input、e3.det.label-independence、e3.rec.after-refusal（部分）。
- tests/stream-e3.cx（stream-e3-independent）：e3.ref.diag-stream-boundary、e3.ref.unmapped-limitation、e3.ref.hostile、e3.rec.after-refusal、e3.in.unitok2-multi。
- tools/stream-semantics.cx（加e3模式，语料借opt-corpus-gen字面值，可选REF -O0 -S对拍）：e3.pos.decl-scope / integer-arith / pointers-arrays / aggregates / control / calls / initializers / vla / float-literals。

## stream-elf（22格）

分类：A可落地 10 / B需材料 8 / C需新工具 2 / D实为产品缺口 0 / E需裁定 2（无一格记PASS）

| 格 | 现有API/入口 | 独立预期来源 | 可复用批/工具 | 分类 | 下一动作 |
|---|---|---|---|---|---|
| elf.id.same-candidate-model | streams.json elf条目(lnx/x86_64模型12) | 候选P3成员SHA | tools/stream-model-identity.cx | A | 与e3同一新行units=e3:elf实跑 |
| elf.pos.exact-image-independent | stream7 | REF(make ref)同源-O2镜像；或手推ELF字节 | interfaces case0链镜像+{ref}材料(批stream-e4-o2已用) | A | 加REF镜像逐字节比对；未核REF与网络ELF字节相同，不同则转手推解码(C) |
| elf.fmt.zero-tail-trim | E6共享动作(prd.md:745) | 手推p_filesz<p_memsz范围 | tests/stream-elf.cx；ELF解析借formats-elf-ape.cx | A | 手构@data非零前缀+大@data_len，比PT_LOAD filesz/memsz |
| elf.pos.relocations | @relocs(formats.md:79,96-99) | 手推：槽值+数据基址 | tests/stream-elf.cx | A | 手构@data槽+@relocs，从phdr取基址比补丁字节 |
| elf.pos.x86-branch-shortening | x86长分支后缩短(prd.md:739) | SDM手推EB/E9/E8字节 | tests/stream-elf.cx | A | 手构近/远jump与call，比rel8/rel32编码 |
| elf.pos.windows-args-template | WINARGS_BODY；exec/facts/enc-x86win-winargs.tsv | 模板字节事实表 | tools/formats-macho-pe.cx PE解析 | B | 用当前候选重打win/x86_64流材料后检查模板字节与计数 |
| elf.gold.enc-lnx-x86_64 | weights/gold/enc.tsv(522键) | enc.tsv+src/asmtext.c at_dis/tests/asm-golden/x86_64.tsv | 无；仿peep-cases-gen | C | 写enc-cases-gen：每op构造target.text行，解码比对 |
| elf.gold.enc-foreign-targets | enc.tsv其余435键 | enc.tsv+arm解码(tests/asm-golden/arm64.tsv) | 无 | C | 同上按目标分片，并需各目标流材料 |
| elf.target.lnx-arm64 | lnx/arm64 elf模型；旧材料out/c43-cdx-stream-pack-lnx-arm64-01(包4d9b8a3a) | 独立ELF解析+ARM ARM手推MOVZ/MOVK/BL字 | formats-elf-ape.cx ELF解析 | B | 在当前候选重打lnx/arm64材料，手构fixture比字 |
| elf.target.osx-x86_64 | osx/x86_64 elf模型18；旧材料cdx-osx-x86_64-01 | Mach-O布局+cx_sha256页哈希重算 | tools/formats-macho-pe.cx 加流输入模式 | B | 重打材料后让formats-macho-pe解析stream7输出 |
| elf.target.osx-arm64 | osx/arm64 elf模型；旧材料cdx-osx-arm64-01 | 同上 | tools/formats-macho-pe.cx | B | 同osx-x86_64 |
| elf.target.win-x86_64 | win/x86_64 elf模型22；旧材料cdx-win-x86_64-01 | PE/COFF规范(导入/cookie/DIR64排序去重分页) | tools/formats-macho-pe.cx:137-145 | B | 重打材料后对stream7输出复用PE检查 |
| elf.target.win-arm64 | win/arm64 elf模型20；旧材料cdx-win-arm64-01 | 同上 | tools/formats-macho-pe.cx | B | 同win-x86_64 |
| elf.pos.memory-unimem1 | memory路由=同模型12；PACKAGE.md:180-203 | 手推UNIMEM1头(魔数+4×u64) | stream-runtime UNISA_STREAM_RESOURCE仅单资源 | B | 钩子扩为多资源(memory/text+data或process/argc+argv)重打stream7 |
| elf.ref.label-twice | exec/enc/gen-manifest.tsv:14 | 声明名'a label defined twice' | tests/stream-elf.cx | A | 手构重复标签TINS，断言rc1/空stdout/名+恢复 |
| elf.ref.meta-twice | exec/enc/gen-manifest.tsv:14,21 | 声明名'a meta key given twice' | tests/stream-elf.cx | A | 手构重复form=键TINS，断言名 |
| elf.ref.scratch-alias | exec/enc/gen-manifest.tsv:64 | 声明名'.zero with the scratch r11 as its base' | tests/stream-elf.cx | A | 手构.zero以r11为基址；未核.zero行语法 |
| elf.ref.range | prd.md:741；exec/enc无范围拒绝名 | 无独立来源(无声明名) | tests/stream-elf.cx | E | 先探针越界位移/立即数；裁定名字，静默接受则转D |
| elf.ref.arm64-operand | exec/enc/arm-manifest.tsv:58 | 声明名'ARM64 operand or instruction' | tests/stream-elf.cx 目标参数化 | B | 需当前候选lnx/arm64流材料后加坏操作数例 |
| elf.ref.missing-input | run.c:157-178 | ENOENT/EACCES/ENOTDIR运行合同 | tools/stream-io-cases-gen.cx 加elf模式 | A | elf模式夹具=exit7 TINS，oracle=执行7+ELF头 |
| elf.rec.after-refusal | interfaces已有帧拒绝后恢复(比干净边字节) | 原生exit7+ELF64头 | tests/stream-elf.cx / stream-io-cases-gen elf | A | 每个新拒绝后跑exit7 TINS并执行；arm64部分依赖B |
| elf.host.lnx-x86_64 | 汇总格 | 本矩阵 | 聚合 | E | 汇总；受range(E)与各B/C格阻塞 |

优先可落地（A类，按批/工具）：
- tools/stream-model-identity.cx（units=e3:elf）：elf.id.same-candidate-model。
- tests/stream-elf.cx（stream-elf-independent，手构TINS）：elf.fmt.zero-tail-trim、elf.pos.relocations、elf.pos.x86-branch-shortening、elf.ref.label-twice、elf.ref.meta-twice、elf.ref.scratch-alias、elf.rec.after-refusal。
- tools/stream-io-cases-gen.cx（加elf模式）：elf.ref.missing-input、elf.rec.after-refusal（I/O部分）。
- tests/interfaces-lnx-x86_64.cx 或 stream-elf 接{ref}材料：elf.pos.exact-image-independent（先核REF镜像字节相同）。

附注（只读核对所得事实）：
- 已装产品P3中 lnx/x86_64 的 image/O2、multi/image/O2、multi/tape/O0 路由的e3均为模型4，memory 路由的elf与image路由同为模型12；warn路由e3为模型26（grep -a 已装 unisacc.com 的 D 行）。
- tools/cxlib/streampack.cx:28 只构造 `<target>/image/<LEVEL>` 路由；tools/stream-runtime.c:19-21 的 UNISA_STREAM_RESOURCE 仅注入一个资源。
- out/c43-cdx-stream-pack-*-01 六目标材料来自包4d9b8a3a，不是da4e纪元(50cc5a96)，按规则不迁移。

说明：resources / ri 域 MISSING 格收口审计（只读，2026-10-28）

范围：`tests/resources-domain-matrix.tsv` 31 个 MISSING 格、`tests/ri-domain-matrix.tsv` 26 个 MISSING 格。只读代码/日志，未编译、未运行、未改任何受控文件；以下全部未记 PASS。
共性发现：
- 矩阵引用的行号普遍已漂移：run.c:1046/1054 实为 1061/1069；compiler.c:519/525/904 实为 596/602/1031；run.c:978 实为 995（严格取键 >512）/721（捕获键长）；tools/bound.c:20-23 实为 29（CLEANUP_EPSILON）；prd.md:605 现为流表，内存预留实现在 exec/c/memory.c:52-71。
- resources 矩阵头注 “Current gap: run-batch 把超时折成 FAIL” 已过时：97a3f4472a 起 run-batch 超时输出 status=UNKNOWN、rc3（tests/run-batch.cx:37-40），np_run 用独立进程组并 kill(-pid)（netpack.cx:763,807）。尚存的缺口：计时用的是 1ms 节拍计数，不是单调钟；被 tools/bound 包住的孙进程在 setsid 后的新会话里，kill(-pid) 杀不到。
- res.hostile.* 十格：已登记的 negative-hostile 批在 Root7a（candidate 7a0cf28b…）上的实跑（out/c43-cc-n1p1-registry-01/negative-hostile.log）里，这十格的断言全 PASS；整批 FAIL 只是因为 self-containing-struct 的 4 项断言失败，而这 4 项不属于 resources 域。tests/negative-domain-matrix.tsv 中同一组 hostile.* 也仍是 MISSING。另：当前已装 unisacc.com 的 sha256 前缀是 79fb84e6…，与该日志的 candidate 不一致（是否属于同一身份口径，未核）。

## resources（31格）

分类计数：A可落地 17；B需材料 0；C需新工具 12；D实为产品缺口 2；E需裁定 0

| 格 | 现有API/入口 | 独立预期来源 | 可复用批/工具 | 分类 | 下一动作 |
|---|---|---|---|---|---|
| res.runner.timeout-is-unknown | tests/run-batch.cx:37-40（超时→status=UNKNOWN rc3） | 矩阵词表 UNKNOWN；夹具睡眠时长可手推 | 无自测批；all-aggregate-controls 未登记 | C | 新建 runner-controls：假根下单行 registry 加睡眠子进程 |
| res.runner.wall-deadline | netpack.cx:807 只累计 1ms 节拍，非单调钟 | 单调钟 elapsed ≤ budget+0.25s（bound.c:29） | 同上 | C | np_run 改按单调钟截止，再用持续输出的子进程自测 |
| res.runner.descendant-cleanup | netpack.cx:763 setpgid + kill(-pid)；bound 子进程 setsid 后逃出该组 | 孙进程记 pid，/proc/PID 应不存在（POSIX） | 同上 | C | 自测“np_run→bound→sleep”嵌套，核孙进程被回收 |
| res.runner.signal-is-fail | run-batch 对信号死只输出 rc=-N status=FAIL，不写信号名 | SIGSEGV=11（POSIX） | 同上 | C | run-batch 在状态行写出信号名，并加 raise(SIGSEGV) 夹具 |
| res.runner.bound-rc142 | bound.c:459 置 142；仅 allsession.cx:299 归为 UNKNOWN | bound 合同注释（rc142=中断/截止） | allsession；各测试仍把 142 记成断言 FAIL | C | 在 netpack 中加共用 142→UNKNOWN 判定，并用 bound 1 sleep 夹具自测 |
| res.steps.stream-exhaust | run.c:1061 getenv+strtol，1069 报文，rc3 | run.c:14 用法注释定 rc3；预算值 1 可手推 | interfaces-lnx-x86_64 / stream-e4（streams 材料） | A | 在流批中加 UNISA_MAXSTEPS=1 例，断言 rc3、stderr 原文、stdout 为空 |
| res.steps.stream-recovery | 同上，默认预算 | 流批已钉的 sha 材料字节 | stream-e4-independent 等 | A | 耗尽例之后同输入默认预算重跑，与已钉字节比对 |
| res.steps.stream-malformed-budget | run.c:1061 strtol 无 endptr/范围检查 | 无独立来源（拒绝名未定） | 流批 | D | 读码可知 abc/-1/0 被静默强转；登记 NEEDS_PRODUCT_CHANGE |
| res.steps.product-env-ignored | asmcompiler.c:6 UNISA_PRODUCT_BUILD；run.c:1059 用构建常量 | 与不设变量的同命令逐字节差分 | negative-hostile 同型 runv | A | 已装产品带/不带 UNISA_MAXSTEPS=1 编译，比对 tape/image 逐字节 |
| res.mem.compile-as-limit | run.c:44 xrealloc die("out of memory") rc2 | RLIMIT_AS 语义（POSIX）；充分档与无限档差分 | 无：np_run/bound 都不能设 rlimit | C | 给 np_run 加 pre-exec setrlimit 选项，或新建 rlimit 启动器 |
| res.mem.run-reserve | memory.c:52-71 预留 2147467264 B，报“cannot reserve native memory” | 预留常量可手推，AS 限低于它即失败 | 同上 | C | 有了 rlimit 启动器后，以 AS=1GB 跑 -run 断言具名拒绝 |
| res.mem.user-malloc-null | -run 在进程内执行（compiler.c:1449 memory_enter） | C99 7.20.3：malloc 返回 NULL，退出码由程序自定 | 同上 | C | 先定能过预留、但让用户 malloc 失败的 AS 档，再加夹具 |
| res.mem.recovery | 同上各格 | 无限档基线字节 | 同上 | C | 内存诸格落地后，同批内做无限档重跑比对 |
| res.fd.compile-nofile-limit | compiler.c:72 “cannot open %s”；run.c:176/463 | RLIMIT_NOFILE 语义（POSIX） | 需 rlimit 启动器 | C | 有启动器后，以 NOFILE=4/8 跑编译和 -run，断言具名拒绝或正确接受 |
| res.fd.no-leak-to-program | -run 进程内执行，程序可见编译器自身的 fd | POSIX：只应有 0/1/2，计数时扣除 opendir 自用的 fd | negative-hostile accept7 型 | A | 夹具扫描 /proc/self/fd，计数为 3 时 return 7；须先核 np_run/bound 自身不泄漏 fd |
| res.disk.output-full | compiler.c:1468-1474（1474）→ “model driver: short output write” rc1 | /dev/full 写返回 ENOSPC（POSIX/Linux） | negative-hostile 同型 | A | -o /dev/full 断言 rc≠0、具名、stdout 为空；矩阵所写“cannot write output”名已过时 |
| res.disk.output-fsize | compiler.c:1466 O_TRUNC 打开，短写后不 unlink | EFBIG 语义；应无残留截断文件 | 需 rlimit 启动器 | D | 读码确认失败后残留部分文件；登记 NEEDS_PRODUCT_CHANGE（短写须删除输出） |
| res.disk.output-eacces | compiler.c:1469 “cannot open output” rc1 | chmod 0500 目录的 EACCES（POSIX）；euid0 时记 NOT_RUN | negative-hostile 同型 | A | 对不可写目录 -o 断言具名拒绝，且不报为“不存在” |
| res.time.user-infinite-loop | bound 截止 rc142；-run 在进程内 | 程序写下 getpid，事后 /proc/PID 不存在；正常 -run 返回 7 | negative-hostile recover() | A | 夹具 bound 2 跑 for(;;)，打印 UNKNOWN 分类行，核 pid 已消失后再跑恢复例 |
| res.time.cpu-limit | 无 rlimit 设施 | SIGXCPU 语义（POSIX） | 需 rlimit 启动器 | C | 有启动器后以 CPU=1s 编译重例，归为 UNKNOWN |
| res.hostile.truncated-file | negative-hostile:truncated-file{,-mid-token}.* | rc1、有界、无映像，恢复例 exit7 | negative-hostile | A | 在当前候选重跑后，证据绑定 negative-hostile:truncated-file.* |
| res.hostile.unterminated-comment | 同批 unterminated-comment.* | rc1、无映像（spec A-47） | negative-hostile | A | 重跑后绑定证据 |
| res.hostile.unterminated-string | 同批 unterminated-string/char.* | rc1（C99 6.4.4.4/6.4.5） | negative-hostile | A | 重跑后绑定 string+char 两组证据 |
| res.hostile.include-cycle | 同批 include-cycle-self/mutual.*（4000 splice 上限） | 有界 rc1；guarded 对照例 exit7 | negative-hostile | A | 重跑后绑定证据；“not covered”措辞是否可接受另记 |
| res.hostile.long-identifier | 同批 identifier-5000.*（Root7a 实为接受 7） | 3+4=7 可手推 | negative-hostile | A | 重跑后绑定证据 |
| res.hostile.deep-nesting | 同批 paren-depth-1000.* | 10-3=7 可手推 | negative-hostile | A | 重跑后绑定证据 |
| res.hostile.binary-garbage | 同批 binary-garbage.* | 固定字节规则，rc1 | negative-hostile | A | 重跑后绑定证据 |
| res.hostile.embedded-nul | 同批 embedded-nul.* 加无 NUL 对照例 | rc1；对照例 exit7 | negative-hostile | A | 重跑后绑定证据 |
| res.hostile.self-expanding-macro | 同批 self-expanding-macro（-run=7） | C99 6.10.3.4 手推：2+1+f(4)=7，-E 输出可手推 | negative-hostile | A | 补一个手推 -E 字面输出断言，再绑定证据 |
| res.hostile.directory-input | 同批 directory-input（rc2 “run: cannot read adir.c”） | 与 ENOENT（rc1）区分；errno 名未输出 | negative-hostile | A | 重跑后绑定证据；若要求点名 EISDIR，转产品缺口 |
| res.recovery.after-budget | 各格各自的恢复例；negative-hostile 前后核候选 SHA | 基线字节 + 候选 SHA 不变 | 计划中的 full-resources（tests/pending/resources.cx 不存在） | C | 新建汇总的 resources 产品批，每个拒绝/UNKNOWN 后跑基线 |

## ri（26格）

分类计数：A可落地 21；B需材料 1；C需新工具 0；D实为产品缺口 1；E需裁定 3

| 格 | 现有API/入口 | 独立预期来源 | 可复用批/工具 | 分类 | 下一动作 |
|---|---|---|---|---|---|
| ri.frame.identity-malformed | resourceledger.h:110-123 只查前缀/长度/hex，不查 stage 语法 | hex/前缀可手推；stage 坏实际报 identity-mismatch | resources（refuse/appended） | E | 裁定 stage 语法错是否必须报 malformed（是则 D），hex 类先落地 |
| ri.frame.row-field-too-long | resourceledger.h:150-151 f[5][4096]，宽≥4096 拒 | 无独立来源（界值 4096 只在源码） | resources appended() | A | 追加 key 字段 4095/4096 字符两行，断言接受/拒绝 |
| ri.frame.key-length-bound | 严格 run.c:995 “resource key too long”；捕获 run.c:721 | tools/ri.md:29 定键长 1–512 B | ri-image-single（有 -I/引号头） | A | 用 7+505/7+506 字节引号头名，捕获并严格重放 |
| ri.frame.bytes-length-mismatch | resourceledger.h:173 | 手设声明长度与实际字节差 1 | resources appended() / ri-image-single | A | 新增单一理由断言，或把 mutated-frontend 例收紧为此名 |
| ri.frame.ledger-dir-absent | resourceledger.h:130：ENOENT→absent，其他→read-failed errno=N | POSIX errno：ENOENT/ENOTDIR=20/EACCES=13 | resources | A | 三个夹具：不存在目录、普通文件当目录、0000 目录（euid0 记 NOT_RUN） |
| ri.frame.ledger-path-too-long | compiler.c:596 在 rl_load 之后；resourceledger.h:106 先报 ENAMETOOLONG | 无独立来源 | resources | E | 此名经 CLI 不可达（实报 read-failed errno=36），须裁定以哪个名为准 |
| ri.frame.strict-needs-ledger | compiler.c:602 原文报文 rc2 | rc2 与空 stdout 可手推 | resources | A | 加不带 --resource-ledger 的严格例 |
| ri.capture.cli-dir-preconditions | run.c:742-764：dir-not-empty/ledger-not-regular/session-busy | tools/ri.md:24-28 合同文本 | resources（UNISACC_RF_CAPTURE） | A | 三个夹具，核目录清单前后不变、stdout 为空 |
| ri.capture.parallel-jobs | compiler.c:1031（默认 JOBS=8 即拒） | 报文点名 UNISA_JOBS=2 可手推 | resources --multi | A | --multi 加 UNISA_JOBS=2 捕获和严格两例 |
| ri.id.target-bound | resourceledger.h:215 stage mismatch；标签含 target | 标签语法手推：e3:lnx/arm64/tape/O0 | resources wrong-stage 型 | A | 用 x86_64 账本带 -b lnx/arm64、osx/win 重放 |
| ri.id.optimisation-bound | 同上；标签 O<digit>（resourceledger.h:267） | 手推 O0≠O2 | resources | A | 用 -O0 账本带 -O2 重放，断言 mismatch 且无输出 |
| ri.id.e3-companion-file-faults | run.c:828-851,905-929 各单名 | 各故障一一对应一个名（run.c） | ri-image-single | A | 重签行使账本可加载后，四种故障各断言单一理由、stdout 为空、无映像 |
| ri.id.front-blob-mutation | resourceledger.h:175；现测试把 blob 截成 1 字节 | 独立 SHA-256 | ri-image-single / ri-image-pem | A | 改为同长度翻字节，断言 hash-mismatch、无映像 |
| ri.id.lower-stage-cli | lower 捕获只在 standalone run main（run.c:1239） | 无（产品 CLI 无 lower 门） | ri-flow-e3-to-lower（适配器） | D | 产品 CLI 只有 e3/source 门，登记 NEEDS_PRODUCT_CHANGE |
| ri.class.cli-target.EMPTY-ABSENT | compiler.c:914 \0cli/target；严格时由账本回答 | prd 不变量 7（目标须显式） | resources（重签行） | A | 重签 EMPTY/ABSENT 两例；若被静默接受则转 D |
| ri.class.cli-option.ledger-authority | compiler.c:841 \0cli/defines；run.c 严格只信账本 | 与普通 -D 调用的字节差分 | resources | A | 从 -DFOO=7 捕获取行，重签进无 -D 账本后差分 |
| ri.class.cli-option.EMPTY | compiler.c:841/858 | 与无 -D/-include 普通调用差分 | resources | A | 重签 EMPTY defines/includes 后与 plain 调用比对 |
| ri.class.source.EMPTY-ABSENT | \0cli/source 是源路径名，不是源内容（compiler.c:889） | 无（“空文件差分”与键语义不符） | ri-image-pem | E | 裁定 \0cli/source 为 EMPTY/ABSENT 时的预期 |
| ri.class.inc-quote.EMPTY | run.c 默认取：\0inc/q/NAME 对应 cwd 相对文件 | 与磁盘空 helper.h 的差分（C99 6.10.2p3） | ri-image-single（helper.h） | A | 重签 EMPTY 后与磁盘空头编译的映像比对 |
| ri.class.inc-quote.ABSENT | 不存在时返回 0，由 E2 报定位诊断（R13-0b #07） | C99 6.10.2p2；file:1:col 可手推 | ri-image-single | A | 重签 ABSENT，断言 rc1、定位诊断、无映像 |
| ri.class.inc-quote.MISSING | run.c:1004 MISSING_EXPLICIT_RESOURCE <hex> | hex(\0inc/q/helper.h) 可手推 | ri-image-single | A | 删去该行后严格重放，断言 hex 键名 |
| ri.class.inc-angle.states | 同 inc-quote，\0inc/a/ 加 -I | 同上四个 oracle | ri-image-single | A | 新增 <x.h> 夹具，覆盖四态 |
| ri.class.hdr-bundled.states | \0hdr/ 键，INCDIR=-I（compiler.c:693）；-nostdinc 在 663/861 | 映像相等、hex 键、捕获事件表 | ri-image-single | A | stdio.h 夹具覆盖四态，并核 -nostdinc 时无 hdr 事件 |
| ri.class.attributes.cli | compiler.c:911 ATTR_SLOT；严格时由账本回答 | 构造器程序退出码 0/7 可手推 | ri-image-single | A | 构造器夹具重签 EMPTY/ABSENT 断言 exit0，删行断言 MISSING |
| ri.class.package-RES-precedence | run.c rf_default_fetch：RI→RES→磁盘 | 各提供者字节可独立计算 | ri-consumers 仅合成 RES | B | 须构造带同键 RES 的包材料，否则已装产品无法注入 |
| ri.route.preprocess-E | -E 即 mode=1，compiler.c:1136 进门后报 route-unsupported（读码推断，未核） | ri.route 既有拒绝合同 | ri-route-refuse | A | 加 -E 严格和捕获两例，断言拒绝名、账本和输出均不存在 |

## 优先可落地

- **negative-hostile**（证据绑定加一处补强）：res.hostile.truncated-file、unterminated-comment、unterminated-string、include-cycle、long-identifier、deep-nesting、binary-garbage、embedded-nul、self-expanding-macro（补 -E 字面输出断言）、directory-input。在当前候选重跑一次后，把证据写成 `negative-hostile:<label>.*`；negative-domain-matrix 中同名格同步处理。
- **negative-hostile 同型产品批**（建议拆为独立的 resources 产品批）：res.steps.product-env-ignored、res.fd.no-leak-to-program、res.disk.output-full、res.disk.output-eacces、res.time.user-infinite-loop。
- **流批**（interfaces-lnx-x86_64 / stream-e4-independent，streams 材料）：res.steps.stream-exhaust、res.steps.stream-recovery。
- **tests/resources.cx**（refuse/appended/重签基础设施，加 --multi）：ri.frame.row-field-too-long、bytes-length-mismatch、ledger-dir-absent、strict-needs-ledger、ri.capture.cli-dir-preconditions、ri.capture.parallel-jobs、ri.id.target-bound、ri.id.optimisation-bound、ri.class.cli-target.EMPTY-ABSENT、cli-option.ledger-authority、cli-option.EMPTY。
- **tests/ri-image-single.cx**（helper.h/-I/e3 伴随文件）：ri.frame.key-length-bound、ri.id.e3-companion-file-faults、ri.id.front-blob-mutation（亦可绑定 ri-image-pem）、ri.class.inc-quote.EMPTY/ABSENT/MISSING、inc-angle.states、hdr-bundled.states、attributes.cli。
- **tests/ri-route-refuse.cx**：ri.route.preprocess-E。

## negative（22格）

分类计数：A可落地 18 ／ B需材料 1 ／ C需新工具 1 ／ D实为产品缺口 0 ／ E需裁定 2（合计22；均未实跑，无一记PASS）

只读核对口径：已装 `unisacc.com` sha256 前缀 79fb84e6（本次只读算得），内嵌包头 `P 3 32 1479 74`（PM=32、PS=1479、NR=74，与 tests/negative-model-loader.cx 硬编码常量一致）。
既有 negative-hostile 唯一实跑是 Root7a（7a0cf28b）181断言4失败（out/c43-cc-n1p1-registry-01/negative-hostile.log）：11格行全绿，仅 self-containing-struct 的“incomplete”措辞红。该证据绑定旧候选，不能迁移到当前候选。
来源位置更正：矩阵写的 spec.md:766/758/822/824 现在实为 spec.md:784(A-47)、776(A-39)、840(TP-8)、842(TP-9)；prd.md:611/661/74/605-607 已漂移，包格式应引用 exec/c/PACKAGE.md:44,78-98,118,281-292 和 exec/c/run.c:415-458，UNIMEM1 应引用 prd.md:687,753（工作树）和 exec/c/memory.c:21-26。
开放缺陷关联：22格都没有被 pp.negative、c.negative、driver.diagnostics、link.malformed-input、link.undef-function-refusal 直接阻塞。loader.other-routes-targets 若翻掉 elf 行，会撞上 BLOCKED 格 loader.terminal-elf-stage（exec/c/bind.c:65 静默成功）。

| 格 | 现有API/入口 | 独立预期来源 | 可复用批/工具 | 分类 | 下一动作 |
|---|---|---|---|---|---|
| hostile.truncated-file | E3解析 file:line 诊断（negative-hostile.cx 钉精确文本） | C99 6.9/5.1.1.2 语法完整性 | negative-hostile | A | 在当前已装候选上实跑 negative-hostile，然后绑定该格证据 |
| hostile.unterminated-comment | exec/lex/literal.tsv:38、exec/pp/text-byte.tsv:42 | C99 6.4.9p1、5.1.1.2p1(3) | negative-hostile | A | 同上，在当前候选上重跑 negative-hostile 并绑定证据 |
| hostile.unterminated-string-char | exec/lex/compare.py:88 所列 lex REJECT 文本 | C99 6.4.4.4、6.4.5 | negative-hostile | A | 同上（string 与 char 两例一起） |
| hostile.include-cycle | exec/pp/directive-body-result.tsv:16（4000拼接上限） | C99 5.2.4.1 #include 嵌套下限；src/front_pp.c:1190 | negative-hostile（含 guarded 对照） | A | 同上；措辞带“not covered”，可另请裁定 |
| hostile.identifier-5000 | E1/E3 标识符上限（63字符具名拒绝或接受） | C99 5.2.4.1 有效字符下限 | negative-hostile（accept7 可拒） | A | 同上，并记录 OUTCOME 是接受还是具名上限拒绝 |
| hostile.paren-depth-1000 | E3 表达式解析 | 字面期望值7；C99 5.2.4.1 括号63层 | negative-hostile | A | 同上 |
| hostile.binary-garbage | exec/lex/entry.tsv:12 `unexpected character` | 固定字节规则 (i*37+11)&0xFF；C99 5.2.1 | negative-hostile | A | 同上 |
| hostile.embedded-nul | exec/lex/entry.tsv:12 | 固定字节；C99 5.2.1 基本字符集 | negative-hostile（含 NUL→空格对照） | A | 同上 |
| hostile.self-expanding-macro | E2 pp 宏展开 | C99 6.10.3.4p2，期望值2+1+4=7 | negative-hostile | A | 同上 |
| hostile.directory-input | run.c readfile `run: cannot read` rc2 | 无独立来源（仅 spec A-47 文字） | negative-hostile | A | 同上 |
| hostile.missing-input-file | 驱动 `cannot open X: ENOENT` rc1 | spec.md:776 A-39 | negative-hostile | A | 同上 |
| hostile.self-containing-struct | exec/parse2/control-reject.tsv:24 reject22 `not covered: a struct member of struct type` | C99 6.7.2.1p2；参考 src/front_parse.c:4456 写的是“cannot contain itself” | negative-hostile | E | 裁定“须写 incomplete”是否成立：参考编译器也不写，产品却用 not covered 掩盖 |
| tape.neg001-unknown-opcode | compiler.c:1137 tape_domain→tapecheck；tapecheck-result.tsv:26 | spec.md:840 TP-8 | 移植 archive/.../tapedomaincheck.cx 的 ?\n 例 | A | 新增 -S/-c 两模式 `?\n` 用例：rc1、stdout空、输出路径不变；仍 rc0 则改记 NEEDS_PRODUCT_CHANGE |
| tape.integer-range | tapecheck 模型 A64/C64U 界检（exec/tapecheck/README.md TP-9节） | spec.md:842 TP-9 字面界 | properties-cases-bound（只测 -b 路线） | A | 在当前候选上重跑 properties-cases-bound，并把证据交叉绑定到本格；可加 -S 路线 |
| tape.integer-underscore | tapecheck `invalid integer literal` | spec.md:842 TP-9 列表 | properties-cases-bound（缺 8_0） | A | 补上 8_0 与 -S 路线后重跑，并核 stdout 为空、含 integer 与行号 |
| loader.resource-table | run.c:421 `bad resource count`、452 `bad resource extent`、454 `duplicate resource name` | exec/c/PACKAGE.md:78-98；packageformat.py:44-50 | negative-model-loader（新增 resource 集） | A | parse() 扩展解析74条F行，分别翻 NR、键长、内容长和重名，钉住具名 rc2 |
| loader.trailing-package-data | run.c:458 `trailing package data` | PACKAGE.md:44“无尾随字节”；packageformat.py:51 | negative-model-loader | A | 在 footer 前插 k 字节，同时把 footer 长度 u64 加 k，断言 rc2 具名且无镜像 |
| loader.m-clen-raw-fields | run.c:443（raw≤0）、446 `bad package model extent`、1126 inflate | PACKAGE.md:281-292 M头算术 | negative-model-loader（M行 f[1]/f[2]） | A | 翻 M13 的 CLEN、RAW 数字，按实测钉住具名原因并保留恢复检查 |
| loader.semantic-illegal-action | run.c:276 `bad opcode`、307/313/327 bank/unit/row 检查 | PACKAGE.md P3 + UNINETB1 语法（exec/c/networkformat.py） | 原语：tools/p3-member-consume.cx、exec/c/qprefixcheck.cx | C | 写私有重编码器：inflate→改 opcode→存储块 DEFLATE+CRC→改 M 头和 footer |
| loader.other-routes-targets | 同 core 各 die 点；路由由 D 表选（run.c:425-436） | 包 D 表路由行 | negative-model-loader（flipcase 参数化） | A | 给 flipcase 加 argv 维度，覆盖 -run、-S、-c -funit 和其余5个 -b 目标；避开 elf 行 |
| runtime.stream-bad-table | tools/stream-runtime.c 调 package(argv[0])；run.c:39 PANIC exit2 | run.c:13-14 退出码合同；PACKAGE.md | stream-io-cases-gen 加 bad-table 集 | B | 先为当前候选 PREPARED 七个 stream 容器，再逐个翻 M 头/CRC，要求 rc2 具名且 stdout 空 |
| runtime.run-memory-bind-refusal | exec/c/memory.c:22 `bad memory image`、25-26 `bad memory image bounds`；compiler.c:1386起 | prd.md:753 UNIMEM1 布局 | exec/c/memory-contract.c 宿主替身（需宿主cc） | E | 裁定三选一：加产品测试钩（转D）、接受宿主替身单元证据，或改 memory 模型 |

### 优先可落地

- **negative-hostile 批**（tests/negative-hostile.cx，已登记）：truncated-file、unterminated-comment、unterminated-string-char、include-cycle、identifier-5000、paren-depth-1000、binary-garbage、embedded-nul、self-expanding-macro、directory-input、missing-input-file，共11格。在重测试窗口内对当前已装候选实跑一次，逐格绑定证据。不要求先改代码；若钉住的文本漂移，按“红即保留”处理。
- **negative-model-loader 批扩展**（tests/negative-model-loader.cx）：resource-table、trailing-package-data、m-clen-raw-fields、other-routes-targets，共4格。新增 `resource`、`trailer`、`mfields`、`routes` 四个集并登记批 id；PM/PS 常量现仍匹配（32/1479），NR=74 需要解析。
- **properties-cases-bound 批**（tools/properties-cases-gen.cx）：tape.integer-range、tape.integer-underscore，共2格。先补 8_0，再在当前候选上重跑，矩阵引用该批行。跨矩阵复用证据需主刀确认口径。
- **tape 域负例（移植）**：tape.neg001-unknown-opcode，1格。移植 archive/testing-c43-legacy/tests/tapedomaincheck.cx 的 `?\n` 与 -S/-c 模式断言，作为新 negative 集登记。只移植夹具，不回调归档 gate。

## BLOCKED 39格裁定表

审计口径：只读（2026-10-28，HEAD 9be80f8876；已装产品 79fb84e6 / 冻结 207195a597，见 prd.md:157）。行号按 `grep -n` 原文件行；未实跑任何命令，所有格均未记 PASS。

- R真需主人裁定：18（win/arm64 原生格 11、CI 通道 2、RI 范围 3、资源预算 2）
- T技术前置可解除：4
- F外宿主通道：12
- S状态应改：5
- 合计：39（dev-methodology §5 所说"6 个具名 BLOCKED"= library 1 + resources 2 + ri 3；其中 library 一格经核实主人已授权 cc 安排，判 T 而非 R）

| 矩阵 | 格 | 阻塞原因(摘) | 阻塞性质 | 依据 | 建议 |
|---|---|---|---|---|---|
| bootstrap | lnx/x86_64 chain.fresh-sid-rebuild | HEAD SID != 4fe5; ctx05 abandoned; waiting for epoch product fixes | T | tests/bootstrap-domain-matrix.tsv:14；理由文本已过时：产品修复已陆续装入（prd.md:157 已装 79fb84e6/冻结 207195a597），真正缺的是当前 SID 的完整 context/PC/GEN+kind2 准入，以及链格重跑 | 下一次慢车道纪元（CLAUDE.md:32）跑完整 107 作业 context/PC/两代 GEN/kind2，再重跑 hostseed 三代链各格；先把 note 改成"待慢车道纪元 <日期>"，不再引用 SID4fe |
| bootstrap | lnx/x86_64 E7.refusal-registry | conflict fix 9df74fd151 not in a candidate | S | tests/bootstrap-domain-matrix.tsv:35；`git merge-base --is-ancestor 9df74fd151 207195a597` 成立（已装冻结源含该修复，改动在 exec/c/compiler.c、src/asmtext.c） | 改为 MISSING（理由：修复已在已装 79fb84e6，拒绝登记批尚未写/跑）；按慢车道纪元实跑具名拒绝批（含 `-b` 与头冲突 rc1 无产物）后再换标 |
| candidate | lnx/x86_64 id.next-candidate | Root7a evidence does not transfer; rebuild pending epoch fixes | T | tests/candidate-domain-matrix.tsv:16；Root7a 已多代被替换（prd.md:157-161 的 79fb84e6/cb743c27/b6f15e82…），"pending epoch fixes"已不是阻塞 | 在下一慢车道纪元的准入候选上重跑全部 id.* 格；note 改为纪元身份绑定，矩阵脚本迁移证据（dev-methodology §7.3） |
| candidate | lnx/x86_64 ci.producer-artifact | no producer workflow; push/dispatch and public artifact need owner authorization | R | tests/candidate-domain-matrix.tsv:19；"no producer workflow"已过时：.github/workflows/candidate-raw-export.yml 于 f6a824fc79（2026-10-28）提交，从已存在 release 资产导出两文件工件。剩余阻塞是授权：CLAUDE.md:60"不为了测试push"与 CLAUDE.md:61"禁令已撤销、按当前任务授权执行" | 归入问题1（CI 是否作为外宿主/同候选通道并授权 Draft release + dispatch） |
| candidate | lnx/x86_64 ci.remote-lnx-x86_64 | needs producer artifact and authorized dispatch | R | tests/candidate-domain-matrix.tsv:20；candidate-native.yml:63 已列 ubuntu-24.04 | 归入问题1；获授权后与 producer 同批一次 dispatch |
| hosts | lnx/arm64 version-0.0.43 | no-native-arm64-guest-on-builder | F | tests/hosts-domain-matrix.tsv:8；本机 `uname -m`=x86_64，无 limactl/qemu-system-aarch64；矩阵头 :3 规定仿真/容器不得 PASS | 移入外宿主门（dev-methodology §5:37）；通道见问题1（ubuntu-24.04-arm runner）或真实 arm64 机按 plans/foreign-host-runbook-cc.md §1 |
| hosts | lnx/arm64 product-sha-stable | 同上 | F | tests/hosts-domain-matrix.tsv:9 | 同上 |
| hosts | lnx/arm64 smoke-return7-subset | 同上 | F | tests/hosts-domain-matrix.tsv:10 | 同上 |
| hosts | osx/arm64 version-0.0.43 | no-darwin-host-on-builder | F | tests/hosts-domain-matrix.tsv:11 | 移入外宿主门；macos-15 runner 或真机；须 arm64 进程（非 Rosetta） |
| hosts | osx/arm64 product-sha-stable | 同上 | F | tests/hosts-domain-matrix.tsv:12 | 同上 |
| hosts | osx/arm64 smoke-return7-subset | 同上 | F | tests/hosts-domain-matrix.tsv:13 | 同上 |
| hosts | osx/x86_64 version-0.0.43 | no-darwin-host-on-builder | F | tests/hosts-domain-matrix.tsv:14 | 移入外宿主门；macos-15-intel runner 或真机 |
| hosts | osx/x86_64 product-sha-stable | 同上 | F | tests/hosts-domain-matrix.tsv:15 | 同上 |
| hosts | osx/x86_64 smoke-return7-subset | 同上 | F | tests/hosts-domain-matrix.tsv:16 | 同上 |
| hosts | win/x86_64 version-0.0.43 | no-windows-host-on-builder | F | tests/hosts-domain-matrix.tsv:17；另有技术前置：Windows JobObject watchdog + 原生路径适配未完成（tests/candidate-domain-matrix.tsv:24 note；plans/v0.0.43.md:83） | 移入外宿主门；本机可先完成 watchdog/路径适配（T 部分），宿主观测走 windows-2025 runner 或真机 |
| hosts | win/x86_64 product-sha-stable | 同上 | F | tests/hosts-domain-matrix.tsv:18 | 同上 |
| hosts | win/x86_64 smoke-return7-subset | 同上 | F | tests/hosts-domain-matrix.tsv:19 | 同上 |
| hosts | win/arm64 version-0.0.43 | needs-native-arm64-PE-not-x86_64-emulation | R | tests/hosts-domain-matrix.tsv:20；产品 APE 只有五切片，win/arm64 走 x86_64 PE（prd.md:859 同构；plans/v0.0.43.md:83"remains a product/platform blocker"）；candidate-native.yml:68 把 windows-11-arm 设为 process: x86_64；交付物只许 unisacc.com 与 dmg（prd.md:89，主人 2026-09-30） | 归入问题2（win/arm64 原生 ARM64 PE 是否属 0.0.43 合同） |
| hosts | win/arm64 product-sha-stable | 同上 | R | tests/hosts-domain-matrix.tsv:21 | 问题2 |
| hosts | win/arm64 smoke-return7-subset | 同上 | R | tests/hosts-domain-matrix.tsv:22 | 问题2 |
| interfaces | win/arm64 if.host.win-arm64 | class=host; needs native ARM64 PE product | R | tests/interfaces-domain-matrix.tsv:47 | 问题2 |
| library | lnx/x86_64 lib.std.family-classification | no-forward-keep-refuse-table-for-43-include-headers | T | tests/library-domain-matrix.tsv:57；主人已裁定方向且"具体怎么做由 cc-unisacc 安排，不由本会话拍板"（research/libc-forward-handoff.md:21-25）；原则见 prd.md:847；缺的正是"转发/保留/拒绝"权威表（handoff :55-57）。include/ 实有 48 项（含 sys/ 18） | cc 交付：事实表 `tools/` 下 family→forward/keep/refuse（带 `--check` 生成器）+ 每族代表探针；表落地后此格转 MISSING→按批实跑。无需主人裁定 |
| negative | lnx/x86_64 loader.terminal-elf-stage | FAIL 15/7 on Root7a; fixed in source 48b5be2a4c; needs rebuilt admitted candidate | S | tests/negative-domain-matrix.tsv:40；48b5be2a4c 是 207195a597 祖先（已装含修复，改动 exec/c/bind.c、run.c）；批 negative-model-loader-terminal 在后续 wave 日志均为 NOT_RUN reason=not-scheduled（out/c43-1621-installed-wave-02/all.log） | 改为 NOT_RUN/MISSING（修复已装，待实跑）；把 negative-model-loader-terminal 排入下一慢车道纪元批；原 Root7a 15/7 红保留 |
| negative | lnx/x86_64 loader.certificate-ape | signing prohibited; no signed candidate exists | S | tests/negative-domain-matrix.tsv:47；签名禁令已由主人 2026-10-28 撤销（CLAUDE.md:61；prd.md:201 与 plans/v0.0.43.md:83 仍写"禁止"，属过时文本）。且合同只是格式定位：exec/c/run.c:410-413 注明"format handling, not a signature trust decision"，exec/c/packagefooter.h:79-128 只核 security directory 偏移/长度/8 对齐 | 改为 MISSING；用 cx 生成器构造合成 WIN_CERTIFICATE 证书表夹具（无需真实证书/私钥）测定位与畸形拒绝；真实签名候选上的复核另列为发布期格（未核是否已有签名证书材料） |
| negative | lnx/x86_64 product.as-target-header-conflict | fixed in source 9df74fd151; needs rebuilt candidate | S | tests/negative-domain-matrix.tsv:50；9df74fd151 已在已装冻结 207195a597 内 | 改为 MISSING/NOT_RUN；慢车道纪元实跑 rc1 无对象断言，oracle ELF e_machine 不变 |
| negative | lnx/x86_64 product.attr-strip-token-len | fixed in source 48b5be2a4c; needs rebuilt candidate | S | tests/negative-domain-matrix.tsv:51；48b5be2a4c 已在已装冻结 207195a597 内 | 同上；USLATTR1 token_len 不符具名拒绝 |
| optimization | lnx/x86_64 corpus.c-testsuite | corpus not in tree (archive has baselines only) | T | tests/optimization-domain-matrix.tsv:54；旧取法 archive/testing-c43-legacy/tests/corpus.sh:16,27（`git clone --depth 1` c-testsuite）；基线 corpus.baseline.list 216 行、knownfail 7 行；仓内无 corpus/ | 材料前置：把 c-testsuite 钉到固定提交 SHA 取到仓外或忽略的 out/（记 PREPARED + 来源 SHA），写 `.cx` 运行器（六 flag、外部 .expected 为 oracle、wrong=0），旧基线只作对照不作 PASS。网络可用性未核 |
| resources | lnx/x86_64 res.steps.product-exhaust | product-budget-4e11-steps-unreachable-within-60s | R | tests/resources-domain-matrix.tsv:25；exec/c/compiler.c:7-9 `UNISA_DEFAULT_MAXSTEPS 400000000000LL`；exec/c/run.c:1059"shipped: a build constant, the host environment has no say"。粗估 E2 约 2e7 步/秒（dev-methodology §9：3.04e7 步≈1.5 s），4e11 步约 5–6 小时（估算未核） | 归入问题3 |
| resources | lnx/x86_64 res.time.end-to-end-budget | no-current-numeric-latency-ruling | R | tests/resources-domain-matrix.tsv:44；现有测量：冷 1.796 s / 热 0.090 s（plans/v0.0.43.md:143），header-free .cx 冷 1.94 s（dev-methodology §8:62）；spec B-5 是历史 Python 数字 | 归入问题3 |
| ri | lnx/x86_64 ri.class.memory | owner-ruling-run-route-RI-scope | R | tests/ri-domain-matrix.tsv:70；`\0memory/reserve` 只在 -run 路线注入（exec/c/compiler.c:1399），而 strict/capture 对 -run 现为具名拒绝（exec/c/run.c:777；ri.route.object-run-refusal 已 COVERED_LOCAL，:75） | 归入问题4 |
| ri | lnx/x86_64 ri.class.library | spec-gap-library-keys-vs-CLI-ledger | R | tests/ri-domain-matrix.tsv:71；spec.md:853-870 LB-3..LB-8 均未规定 CLI 账本能否供给 `\0library/*` | 归入问题4 |
| ri | lnx/x86_64 ri.route.run-object-supported | owner-ruling-whether-run-object-RI-replay-is-required | R | tests/ri-domain-matrix.tsv:77；现合同是拒绝（run.c:777"strict and capture cover tape and image routes only"） | 归入问题4 |
| stream-e1 | win/arm64 e1.host.win-arm64 | needs native ARM64 PE product | R | tests/stream-e1-domain-matrix.tsv:56 | 问题2 |
| stream-e2 | win/arm64 e2.host.win-arm64 | needs native ARM64 PE product, not x86_64 emulation | R | tests/stream-e2-domain-matrix.tsv:77 | 问题2 |
| stream-e3 | win/arm64 e3.host.win-arm64 | needs native ARM64 PE product | R | tests/stream-e3-domain-matrix.tsv:66 | 问题2 |
| stream-e4 | win/arm64 e4.host.win-arm64 | needs native ARM64 PE product | R | tests/stream-e4-domain-matrix.tsv:43 | 问题2 |
| stream-elf | win/arm64 elf.host.win-arm64 | needs native ARM64 PE product and host | R | tests/stream-elf-domain-matrix.tsv:55 | 问题2 |
| stream-lower | win/arm64 lower.host.win-arm64 | needs native ARM64 PE product | R | tests/stream-lower-domain-matrix.tsv:47 | 问题2 |
| stream-prune | win/arm64 prune.host.win-arm64 | needs native ARM64 PE product | R | tests/stream-prune-domain-matrix.tsv:45 | 问题2 |

### 需主人一次性裁定的问题

**问题1（2 格：candidate ci.producer-artifact、ci.remote-lnx-x86_64；并决定 12 个 F 格及外宿主门的通道）——是否授权 GitHub Actions 作为同候选外宿主通道，并接受完成门拆分？**
背景：CLAUDE.md:60"不为了测试push"，CLAUDE.md:61 已撤签名/Draft/Latest/公开禁令但"按当前任务授权执行"；producer（candidate-raw-export.yml）与六 runner 矩阵（candidate-native.yml:63-68）已写好未运行；dev-methodology §5 的"本机门/外宿主门拆分"待主人确认。
- A. 授权：为准入候选建 Draft release（含 unisacc.com + build.json）→ dispatch raw-export → dispatch candidate-native；外宿主格只在该通道回流，本机门与外宿主门拆分生效，`--all` 本机子集 rc0 作为日常完成信号。
- B. 不用 CI：只接受主人提供的真实机器（按 plans/foreign-host-runbook-cc.md 导出跑），CI 两格删除或改为 not-required；门拆分生效。
- C. 维持现状：外宿主门与本机门不拆，CI 两格继续 BLOCKED。
- 建议：A。理由：工作流已为同候选、哈希绑定设计，可一次性覆盖 lnx/arm64、osx×2、win/x86_64 四类宿主；授权只限 dispatch 与 Draft，不推送测试专用提交，不选 Latest。

**问题2（11 格：hosts win/arm64 ×3、interfaces、stream-e1/e2/e3/e4/elf/lower/prune 各 1）——win/arm64 的产品合同是"原生 ARM64 PE 进程"还是"x86_64 PE 在 Windows ARM64 仿真下运行"？**
背景：APE 只容一个 PE，产品现为五切片；公开资产只限 unisacc.com 与 dmg（prd.md:89）；plans/v0.0.43.md:83 把它定为 product/platform blocker；CI 已按 process: x86_64 编排。
- A. 0.0.43 合同改为"win/arm64 宿主上运行 x86_64 PE（仿真）+ 用户镜像为原生 ARM64 PE"；这 11 格改写 requirement 并转 F（外宿主门），原生 ARM64 产品进程延到后续版本。
- B. 维持原生要求：另出原生 ARM64 PE 产品工件（违反"只两件公开资产"，需同时改交付裁定），转 NEEDS_PRODUCT_CHANGE + F。
- C. 删除这 11 格，win/arm64 只保留用户镜像格（formats/optimization/properties 等已有的 NEEDS_FOREIGN_HOST）。
- 建议：A。理由：与现有交付物裁定和 APE 结构一致，不新增公开资产；原生用户镜像义务仍保留，不缩覆盖。

**问题3（2 格：resources res.steps.product-exhaust、res.time.end-to-end-budget）——资源预算类格如何定合同？**
- 3a 产品步数耗尽（4e11 步，估算 5–6 小时，远超 60 秒规则；运行期不允许环境覆盖）：
  - A. 接受"同源仅改 `UNISA_DEFAULT_MAXSTEPS` 编译常量的变体构建"作为该格证据（批内核对与已装产品源差异仅此常量，rc3 + 字面消息），并另保留 product-env-ignored 格证明已装产品不受环境影响。
  - B. 给此格单列重活窗口例外（如 8 小时 bound、每纪元一次）在已装产品上实跑。
  - C. 删格，只以 res.steps.stream-exhaust + 代码审查覆盖。
  - 建议：A（不放宽产品、不改运行期合同，证据可在 60 秒内复现）。
- 3b 端到端时延预算：
  - A. 定绝对上限（例如 `hello.c` 默认 -run 冷 ≤3 s、热 ≤0.3 s，header-free `.cx` 冷 ≤3 s；以现测 1.796/0.090/1.94 s 为基线）。
  - B. 定相对回归预算：不得比上一慢车道纪元慢超过 20%（冷热分别计），越界即红。
  - C. 改为非必选的测量报告格（只记 OBSERVED/UNMEASURED，不断言）。
  - 建议：A+B 合并为一条合同（绝对上限兜底 + 纪元间相对回归），由主人给出具体数值。

**问题4（3 格：ri.class.memory、ri.class.library、ri.route.run-object-supported）——RI（具名资源输入）在 0.0.43 的路线与键范围？**
- A. 现行拒绝即合同：strict/capture 只覆盖 tape 与 image 路线，-run/-c 维持 `resource-route-unsupported` 具名拒绝（已 COVERED_LOCAL）；ri.route.run-object-supported 删格；ri.class.memory 改写为"-run 下 `\0memory/*` 不进入 RI 账本且 strict 拒绝"；`\0library/*` 键归 libunisacc 所有，CLI 账本供给即具名拒绝（追加 spec LB 条款）。
- B. 要求 -run/-c 支持 RI 重放：两格转 NEEDS_PRODUCT_CHANGE（run.c 路线扩展 + memory 键 presence 语义），library 键仍按 A 拒绝。
- C. 推迟到 0.0.44：三格在 0.0.43 标 DECLARED 不计。
- 建议：A。理由：与已覆盖的拒绝合同一致，无需产品改动即可闭合；不允许静默绕过账本。

### 可直接解除

**动作甲：改状态并排入下一慢车道纪元实跑（S，修复已在已装 79fb84e6/冻结 207195a597）**
- bootstrap E7.refusal-registry（9df74fd151）
- negative loader.terminal-elf-stage（48b5be2a4c；批 negative-model-loader-terminal 至今 NOT_RUN）
- negative product.as-target-header-conflict（9df74fd151）
- negative product.attr-strip-token-len（48b5be2a4c）
- 做法：矩阵主人把 BLOCKED 改为 MISSING/NOT_RUN 并写明"修复已装、待纪元实跑"；纪元批 PASS 后再换标。原红证据保留。

**动作乙：禁令已撤 + 合成夹具（S）**
- negative loader.certificate-ape：改 MISSING；写 cx 生成器构造合成 WIN_CERTIFICATE 夹具测证书表定位与畸形拒绝；同步修正 prd.md:201、plans/v0.0.43.md:83 中"签名禁止"过时文本（属文档主人操作）。

**动作丙：慢车道纪元前置（T）**
- bootstrap chain.fresh-sid-rebuild、candidate id.next-candidate：在下一慢车道纪元完成完整 context/PC/两代 GEN/kind2，然后重跑 hostseed 链与 id.* 全部格；先把过时 note（SID4fe/Root7a/"pending epoch fixes"）改为纪元绑定说明。

**动作丁：材料与表（T）**
- optimization corpus.c-testsuite：钉 c-testsuite 固定提交、取到忽略目录、写 `.cx` 六 flag 运行器（外部 .expected 为 oracle），记 PREPARED 后实跑。
- library lib.std.family-classification：cc 按主人已授权（research/libc-forward-handoff.md:21-25）产出 forward/keep/refuse 事实表 + 带 `--check` 生成器 + 每族代表探针。

**附：F 格（12）**——hosts 的 lnx/arm64、osx/arm64、osx/x86_64、win/x86_64 各 3 格：移入外宿主门，通道取决于问题1；win/x86_64 另有本机可先做的 JobObject watchdog 与原生路径适配。

