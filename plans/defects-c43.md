# c43 已知缺陷登记（cc，2026-10-28 起）

## cxpack.fifo-open — 非普通清单阻塞（Linux 候选切片已复现，待修）

已装Linux修复：fifo-candidate-install的prepare/finish各重新核材料，备份旧e955产品/记录并暂存候选，finish校验后替换及fsync。共享已装8147f2e7/record c493b971，收据out/c43-fifo-install-01/installed.tsv SHA d932685226e4e4475a7da1ca2c682ee5e33ecb0c443ce3fabeeca243294a1df4。安装后smoke16/0，直接已装FIFO清单rc1、cannot read cxpack.tsv且stdout为空。Windows仍未改；外国宿主未运行；以下未安装描述为此前阶段历史。

FIFO新APE候选8147f2e72dafeb6a0c367398a3eafff2c1b8c5c555e00eee9018cabf86abd63e，record c493b97153c6a3f363bc5df2c0bb36e6c85a8f7a5364b259665fd8ed648d1be9，identity-helper check通过。候选格式out/c43-fifo-ape-formats-02.stdout为68/0；首轮ROOT错误触发installed-product-required，原list不覆盖，改以候选所在目录为ROOT并提供现有watchdog后实跑通过。直接sh候选、不传外部模型，清单五项实返0/1/1/0/1，过滤与缺席导出符合预期，超限/目录/FIFO具名拒绝且stdout为零；日志out/c43-fifo-candidate-{case}-01.*。仍未安装，不转授外国宿主执行资格。

FIFO冻结ab7fbdc810/SIDb2007a76的复用适用性：现有cxpack-source-delta新增显式fifo-v1配置，不改变默认旧配置的三个固定文件对。实核相对af8429d0cf共1932文件/49目录/8符号链接，仅src/main.c和exec/c/compiler.c两个固定哈希对变化，witness1f0a90a7ef74c19edfd26b452885844df3eb403f0508ef4e67c1d17914e063c5。反向根rc2具名拒绝；旧默认配置复跑witness仍ab107f14。证据out/c43-fifo-source-delta-01.*、reverse-01.*及out/c43-fifo-delta-legacy-01.*。模型/构造树字节未变，不重复生成权重；后续包绑定、切片及产品准入仍须实际核验。

宿主接续21ba1f4b52：按现有build-cxcc-host配方私有重链GCC/Clang至out/c43-fifo-{gcc,clang}-host.elf，两端五项-o均实返0/1/1/0/1；valid/absent生成程序均rc7，FIFO具名cannot read cxpack.tsv。证据out/c43-fifo-{gcc,clang}-{case}.{stdout,stderr}，根gcc_cx.com/clang_cx.com未替换。模型驱动osx/arm64切片out/c43-fifo-osx-arm64-01构造rc0，未执行外国平台，不授运行资格。已装e955仍未包含此修复。

经典参考接续：src/main.c 同样采用 POSIX 非阻塞打开及同fd普通文件检查。现有 tools/build_ref.sh（宿主C依赖）构造 out/c43-fifo-ref-01.elf，五项实返0/1/1/0/1，公开函数过滤/缺席兼容保持；超限、目录、FIFO均具名拒绝且stdout为0字节。日志out/c43-fifo-ref-{valid,oversize,directory,absent,fifo}-01.*。未验证Windows或经典自举。

POSIX模型驱动修复开发验证：cxm_rd改为O_NONBLOCK打开、同fd fstat拒非普通文件、EINTR重试及有界read，保留缺席/超限/I/O错误分类。新构造 `out/c43-fifo-driver-01.elf` 搭配显式ff696，`out/c43-fifo-fixed-01.stdout` 五项5/0；FIFO现在rc1具名拒绝且stdout为空。只证明Linux私有驱动，不代表已装e955已修；Windows仍原路径，经典与宿主对应改动尚待接续。

2026-10-28：`out/c43-e955-cxpack-fifo-01/` 实跑候选 slice-lnx-x86_64（冻结 af8429d0cf）及显式 PCff696，前四项通过；第五项真实无写端 FIFO 超过 10 秒，transport-error 为 TimeoutExpired，控制器 rc2。不是具名拒绝通过，不授其他驱动/平台结论。stdout/stderr/status.tsv 和超时原因均保留。下段为复现前分析。

### e955 CX0 正式批验收

`out/c43-e955-cx0-batches-01/`：cx0-unit 28/0、cx0-bundle 38/0、cx0-e2 17/0，三个 BATCH_RUN 均 PASS 且 candidate=e955714e。E2 reader 由 fresh workspace `out/c43-e955-cx-workspace-01/` 构造，SHA558ce95c；模型来自新提取 `out/c43-streams-e955-01/`，独立 e2-input-binding 正式44/0。日志 `out/c43-e955-{cx0-unit,cx0-bundle,cx0-e2,input-binding}-01.stdout`。严格清单 metadata_valid=1，234批/214覆盖/19 unresolved；清单校验不记执行资格。首次误用不存在的 tools/bound55 导致 rc127，保留 list-01；改用现有 tools/bound 55 后 list-02 rc0。有限模块合同不关闭完整发布或外国宿主资格。

模型/经典/cxcc三端cxm_rd使用阻塞fopen；无写端FIFO可能在进入fread前阻塞，
与CX0模块的O_NONBLOCK+普通文件检查不一致。尚未实跑，不能记已确认失败。
tools/cxpack-read-failures.cx新增可选--fifo，Linux/x86_64创建并核实实际FIFO；
保留原四项，加第五项必须rc1具名拒绝且无接口输出。子进程仍限10秒，超时不能判PASS。
待cc归还窗口后实跑；本次未修改产品输入或重建产品。

## cxpack.manifest-read-fail-open — 清单读取失败扩大公开接口（开放，未安装源码）

2c949ba577的cxm_listed把所有cxm_rd负值当成无清单，返回2并导出全部非static函数。
当前源码私有模型驱动e2292a76、PCff696上实际验证：正常清单只导出pack_mark；
65536字节以上清单、cxpack.tsv为目录两者均rc0且额外导出pack_extra。
原生回归tools/cxpack-read-failures.cx为3项/2失败，out/c43-cxpack-read-regression-01.stdout。
夹具与原始输出在out/c43-cxpack-review-01/；共享4c28尚未包含该公开过滤改动，不能移贴私有结果。
修复需区分ENOENT与读失败/超限，失败具名拒绝、无接口输出；保留无清单兼容行为。
cxm_rd存在模型驱动、经典参考和cxcc三个实现，需保持返回合同一致，不能仅改一端。

修复源码已统一三端返回合同：仅ENOENT返回-1，超限返回-2，其他打开/读取/关闭失败返回-3。
公开接口生成遇后两者具名拒绝，不再退回全部公开；缺少清单仍保留兼容行为。
私有模型驱动driver-fixed.elf配正规PCff696实跑4/0，输出在
`out/c43-cxpack-read-regression-02.stdout`，包含正常、超限、目录及无清单四项。
经典参考reference-fixed-02.elf同四项实际结果为0/1/1/0；两拒绝stdout为空，
stderr分别为cxpack.tsv too large和cannot read cxpack.tsv，见
`out/c43-cxpack-review-01/ref-{valid,oversize,directory,absent}.*`。
第一次参考构造因errno声明缺失失败，原日志保留；refshim补显式errno头后构造成功。
cxcc宿主首次重建发现目录仍被当空清单：fseek/ftell得到零长度，零字节fread跳过实际读取。
保留两宿主首轮directory/*-build.*及错误产物；改为有界fread并检查ferror/fclose后，
GCC与Clang各四项-o实跑：正常/缺席编译rc0且执行rc7，超限/目录编译rc1并具名拒绝。
二轮证据在`out/c43-cxpack-read-regression-02/{valid,oversize,directory,absent}/*-build-02.*`；
独立宿主驱动为`out/c43-cxpack-review-01/{gcc,clang}-fixed-02.elf`，未覆盖根目录宿主产品。
本次仅Linux/x86_64；共享4c28未安装此修复，完整缺陷仍开放。

回归断言进一步收紧为拒绝码必须1，超限必须报告`cxpack.tsv too large`、目录必须报告
`cannot read cxpack.tsv`，且stdout为空。此断言修订尚待重新执行，不继承旧4/0为新测试结果。
工具现逐例独占保存stdout、stderr、status.tsv；传输失败额外保存transport-error，保留现场后拒绝。
此证据留存改动也尚待实跑，不补造旧回归中未保存的逐例输出。

权威源审查发现cx/cx0/plan.cx未含2c949ba577的公开清单过滤；直接提取将丢失后续修复。
已将同一cxm_listed及函数导出过滤同步回plan.cx，compat-prefix同步当前说明与错误返回合同。
src/cxmod.c及冻结根不改；`tools/cx0-extract.cx -- --check`逐字复验、CX0路径回归尚待交窗执行。
交窗后实际--check退出0：EXACT 45858B，SHA c0a385b6ff5f1aa3e9067142b91b033747134718afa7af62bd9236a6a49c7ce6，
与冻结src/cxmod.c相同。证据out/c43-cxpack-extract-check-01.*。
独立CX0默认-run回归新增清单公开列表、目录IO拒绝、超限拒绝、无清单恢复四项，
总28/0，out/c43-cxpack-cx0-01.*；仅本机CX0模块，不授已装产品新行为或完整E2束资格。

构造接续：af8429d0cf已git archive到仓外`/home/devali/repos/unisacc-epoch-cxpack-af8429d0cf`，
启动产品逐字核为4c2831c3，来源记录一并保存。prepare-context默认-run实返0，
初始产品观察与新宿主identity-helper观察均为SID
`6e7375f9aba84aba36c5ccbc5d705a911a1134e4660776ad22345c75b3602250`；
context.tsv SHA `cce326a849ec5d73f3e2e5700bfa6f4c232abb011b0f63b0dec55150f4190623`。
证据在冻结根out/epoch-context-01与out/cxpack-context-01.*。
仅PREPARED，材料作业未开；与cc流批出现重叠后停止续派，不能把该准备记作候选或资格。

后续现场无在跑批时完成context复验/list（rc0），再实际执行tool-gen、gen-shared-e2、
gen-shared-e1、gen-shared-e3四项，全部rc0并生成source-package-job-v1收据。
各输出在冻结根out/cxpack-{tool-gen,gen-shared-e2,gen-shared-e1,gen-shared-e3}-01.*，
材料及收据在out/epoch-context-01/jobs/。尚余31项生成JSON未执行；未启动tbl/net，
应先完成生成内容比较，再按依赖及复用合同决定重建范围，不把四项材料计作功能PASS。

首三份JSON与`unisacc-epoch-sizeof-e5bade1851/out/epoch-context-01/jobs`同名材料
实测SHA256一致：E1=5392af7e38e4e22062664105563fce385fbfbf338716d0fa9bbe82b6867dd396，
E2=087a3220858653b2a30590c365f03f6fff64aa8f648eff1ab1293389a60a73e7，
E3=6dd90ec5c6f799a50617cab36d9c39c595d022e44d8474c29365d7e99e7fc168。
该局部比较不替代剩余31项及完整复用准入。

后续cc父进程2637955终止后续作业：shared-e4/shared-o1/shared-prune/shared-nativeabi、
lnx-arm64与lnx-x86_64各lower/elf、osx-arm64的lower/elf共十项生成全部rc0并有收据。
累计tool-gen 1项、生成JSON 13/34项完成，余21项；tbl/net未启动，包/准入未做。

`tools/cxpack-source-delta.cx`是待实跑的有限差异检查器，方向为冻结link-896888f424到
cxpack-af8429d0cf；只允许src/cxmod.c、src/main.c、exec/c/compiler.c三对固定哈希，
其余八棵产品输入树核目录、类型/模式、字节与符号链接字面值。正向及反向拒绝测试尚未跑，
不能据代码存在授予复用；生成JSON完整比较与候选身份检查仍独立必需。

后续已完成全部34项生成（加tool-gen共35项实构），每项独立bound55、rc0并有job收据。
source-model-compare对sizeof冻结纪元完整实测34 SAME/0 DIFF；报告
out/c43-cxpack-model-compare-01.tsv SHA5e0a71b2a2cb897d7d8b116038dda15d4656be994259455331e5ba0738b0177d。
cxpack-source-delta正向rc0，核1932文件/49目录/8符号链接/三处固定差异，
witness ab107f143caae14f4c336e6a2a73378f3cc31dad26db8cb7e8cfc9d7788ff6af；反向rc2具名拒绝src/cxmod.c。
证据out/c43-cxpack-source-delta-{01,reverse-01}.*。这些证明内容与差异范围，不是候选准入或功能PASS。

冻结根out/candidate-01已实际构造lnx/x86_64和win/x86_64两个切片，均bound55内rc0。
命令为冻结根`sh ./unisacc.com -O2 -I <冻结根>/include -I <冻结根>/exec/c -b TARGET exec/c/asmcompiler.c -o out/candidate-01/slice-TARGET`；
构造者4c2831c3，显式两项-I为本次实用flags（不是旧product-build的-iquote命令）。
lnx切片751192B，SHA435c75687470c0e65254c2f425646ce695ad776813dd0ef4c9c9c57aad95bd25。
两次构造后identity-helper均复测SID6e7375f9不变，stdout/stderr保留在同目录。
其余三片、APE封装、候选记录和准入尚未完成，未替换已装产品。

后续三片lnx/arm64、osx/x86_64、osx/arm64均在原bound55内rc0，五片已齐。
使用sizeof正规纪元ape-wrap-host.elf、来源相同seed/ape.c（SHAad3c21cf）、PCff696封装rc0。
冻结根out/candidate-01/unisacc.com为3014754B，
SHA e955714e766f89e324c76812b02624bf0a6960270507c2feba802a977eacfd0f；构造后SID仍6e7375f9。
直接`sh CANDIDATE -cx-interface SOURCE`未挂外部模型包，正常/超限/目录/缺席四项rc为0/1/1/0；
正常只导出pack_mark，缺席同时导出pack_extra，两拒绝具名且stdout为空。
证据out/c43-cxpack-read-regression-02/{valid,oversize,directory,absent}/candidate.*。
新APE仍未准入/安装，外国宿主未运行；候选记录与更完整产品验证继续。

候选e955格式批实际68/0（out/c43-cxpack-ape-formats-01.*），测试ROOT为独立candidate-01，
其unisacc.com为被测真实APE；六目标写出/重复字节及APE结构不授外国宿主原生执行资格。
收紧断言及逐例输出留存版本实际4/0（out/c43-cxpack-read-regression-03.*），
对象为新lnx切片435c7568配显式PCff696，与前述APE内嵌包直接检查分别记证。
冻结根补af8429d0cf提交元数据（Git对象借用共享objects，只在仓外写index/ref），tracked diff空。
原生identity-helper write/check均成功，候选记录SHA0fe8abb8b337825fc1b9f927adf668622528cbb10f94bd40f7ae14c11ecb8541；
记录在冻结根out/candidate-01/unisacc.com.build.json。仍未安装，下一步有限材料准入检查及安装。

`tools/cxpack-candidate-check.cx`有限复核已实跑rc0（out/c43-cxpack-candidate-check-01.*）：
固定启动/旧产品4c28与记录820b、新候选e955与记录0fe8，逐字比较内嵌PCff696；
固定原生helper核两冻结源前后身份，实跑三文件差异检查和34模型比较，核工具前后字节。
两context固定cce326a8/ab98a6ae，比较真实读取job收据与JSON，不复用旧测试结论。
错误候选传入共享旧4c28时具名REFUSE candidate、rc2（out/c43-cxpack-candidate-wrong-01.*）。
结果仅CHECKED finite-material-applicability；不冒称kind4重建、安装或发布资格。下一步两阶段安装器接此检查。

两阶段安装已完成：tools/cxpack-candidate-install.cx prepare/finish均rc0，分别重新执行完整有限检查，
核安装器/检查器字节、前后产品/记录、冻结身份、暂存与备份后替换并fsync。
共享现e955714e/记录0fe8abb8，旧4c28备份在out/c43-cxpack-install-01/previous.com及.build.json。
installed.tsv SHA f1a20e2e330cb663a7086c5474c65924729a8d557477c6c9944eada07ac30ad5。
安装后实际默认-run smoke16/0（out/c43-e955-smoke-01.*）。这是有限材料路径开发安装，非原kind4重建或发布资格。
证据沿用冻结根out/cxpack-gen-*-01.*与epoch-context-01/jobs；当前无所属在跑作业。

## link.undef-function-refusal — 未定义外部函数链接成崩溃镜像（开放）

2026-10-28 cdx 实测更新：共享3a5b完整原批为1 PASS/4 FAIL，收据
`out/c43-3a5b-link-undef-01/results.tsv`；object/archive/reversed三个错误镜像rc139，direct-C为rc127，不能统称SIGSEGV。
本刀`tl_check_undef`检查所有未解析extern，仅豁免随后生成的`__init`；诊断有界截断，不再跳过长名。
修改源码经现有build_ref构造的独立参考`out/c43-link-undef-fix-01/reference.elf`已实测：
main.o、main.o none.a均rc1并具名used；m2.o B.a A.a具名inner且rc1；三者未产生镜像。
正序m2.o A.a B.a仍成功且运行rc106。首次归档试命令误写lib.a，得到cannot open，不计通过；随后正确none.a实测如上。
直接C路径仍生成动态ELF并rc0，说明仅修tapelink尚未闭合整个合同；继续查动态外部符号解析边界。
当前仅源码及参考验证，未重建/安装产品，登记批仍红，不授修复完成。

后续复核纠正直接C归因：`src/main.c` R21-4a'明确让写出的镜像也转发宿主函数；
`exec/c/compiler.c`的fwdwant明确覆盖mode0但排除linking；`src/fwdstub.c`生成的缺符号分支具名exit127。
新参考实际执行direct.elf为rc127、stderr=`unisacc: no host function used`；合法abs(-7)的直接C镜像仍rc7。
因此direct-C不是同一静态链接缺陷；现登记批要求编译期拒绝与既有迟绑定合同冲突。
保留该断言及原红，未通过删断言或限制所有未知宿主名来造绿；需将两个合同明确区分后再登记验收。
长名探针250字符在前端按63字符上限拒绝，未到链接器，不能算长名链接负控通过。
相关文件在`out/c43-link-undef-fix-01/`；这不扩授产品或跨平台资格。

模型驱动验证：共享3a5b以-O2编译当前`exec/c/asmcompiler.c`成功，私有ELF
SHA `e2292a760d559dd9c733aca6a39f8bac896fdb04a5c448382f0f67218514ed9a`，
显式挂载原正规包ff696（未重造模型），未安装。薄包装在`out/c43-link-driver-subject-01/`，
不作为已装产品身份。原缺陷批`out/c43-link-driver-test-03/`实为4 PASS/1 FAIL：
对象、缺定义归档、逆序归档三拒绝均正确，正序实际rc106；唯一保留红为直接C合同冲突。
`out/c43-link-driver-archive-02.stdout`原归档互操作批17/0（独立/usr/bin/ar）。
测试夹具首次包装把--models放在ar前，造成运输失败；修复后第二次遗漏watchdog的bound.c，
导致执行控制失败；两轮原收据均保留。补全watchdog源码后才产生上述最终实跑结果，不把夹具失败误归产品。

接续正规构造：896888f424已git archive到仓外`/home/devali/repos/unisacc-epoch-link-896888f424`；
tar SHA af06942e0334e96e81757c5ad04a1fb4569fa1d7daf9a0ce78123b2b9574fe70。
sizeof纪元固定identity-helper实读新源SID为
`8390b60b0b8f40138c1da649a4cd55afa97edefc7a6eb0e8e88cb369d60f6549`（rc0）。
相对e5bade1851，src/exec/include/kernel/seed内仅src/tapelink.c变化；旧mixed包收据仍绑定353c，不能直接改标成8390。
私有模型驱动对象格式批`out/c43-link-driver-formats-01.stdout`21/0；其中打印的installed-product字样是工具固有标签，
本次实际对象始终是显式私有wrapper+e229驱动+ff696包，不是共享安装，也不是跨平台运行资格。

正规复用前置实测：新增薄入口`tools/nn-inputs-compare.cx`直接调用既有ni_compare，
对8390与353c冻结根、固定原生reader5c6a7756实际返回rc2
`necessary-data-tree-mismatch`，证据`out/c43-link-undef-fix-01/nn-inputs.stderr`。
同根负控亦rc2。原因明确：现有nninputs整树读取exec/unisa/src/weights，src/tapelink变化必然拒绝。
这只证明现有保守复用合同不适用，不证明模型需要重造；下一步核实构造器实际输入闭包，
在保留独立原包验证与新源身份的前提下建立适用证明。未删现有校验、未改旧包收据、未授准入。

复用拒绝现已具名定位：nninputs的等值检查新增首个不同规范树行的路径诊断，
不改变任何相等/拒绝条件。新实跑仍rc2，明确`path=src/tapelink.c`，见
`out/c43-link-undef-fix-01/nn-inputs-located.stderr`。五个宿主构造器的既有引用闭包
在nninputs的ni_deps/ni_quotes中列举，不含tapelink；这只是编译依赖边界，
还不能替代运行时数据输入闭包证明，故未据此放宽复用准入。

运行时读入口审查进展：seed/tbl.c仅以argv[1]读JSON，seed/net.c仅以argv[1]读表；
seed/pack.c读显式manifest、manifest指定模型和mount文件。source-package-pack.cx的正式pack命令
仅mount include、kernels、declaration/predefines，未mount src。gen.c的事实加载器固定前缀
exec/facts，通用stage清单固定exec前缀，但模板路径来自清单展开，不能只凭fopen文字搜索宣称闭包已证。
读入口列表保存在`out/c43-link-undef-fix-01/constructor-read-sites.txt`；正式gen命令与展开路径
仍需逐项绑定，下一步重点是这部分，不重复审已确认的tbl/net单输入路径。

生成器实际依赖观察已完成：从sizeof纪元34份gen command.argv.tsv取stage/flags，
保留原工具与40秒预算，仅把工作目录切到8390冻结根、输出写入新out目录；
外层strace -f trace=%file记录每次实际文件访问，34命令全rc0。
执行工具SHA5145075e08da56a5e4e25cf90d4eecf0366b322b49938b1cfd8889f04496818b。
34份JSON与353c原result.json逐份SHA相等，34 SAME/0 DIFF，未运行tbl/net重造权重。
`out/c43-link-gen-trace-01/COMPARE.tsv` SHA90953f766402415c697e8387c04dd01f396b6c1c57eb82e0839337f0572dd80e；
同目录保存每命令trace/stdout/stderr/JSON。trace未出现tapelink或直接src路径；
默认strace字符串截断使它不是完整规范路径清单，34输出逐字身份比较另有独立证据，不能混称通用依赖证明。
下一步将这份实际34同结果与原包来源链接入显式复用合同；当前ni_compare默认拒绝仍保留。

原生独立内容复验入口`tools/generator-json-compare.cx`已落地：枚举原gen目录、逐字比较新旧JSON、
绑定原命令文件SHA，输出规范witness；不从文件存在推断历史退出码或完整作业资格。
默认-run实读34份为different=0、rc0，witness9f1d7ec3076f65d812af21da912a192c5834caf46614070e06dc5386b1f118f4。
证据native-compare-02.stdout；首次纯CX SHA超55秒rc142保留，改用既有kernelsha后在原预算内完成。
独立JSON内容差异夹具rc1（out/c43-json-compare-negative-01），未放宽相等条件。

私有完整产品进展：冻结8390源用共享3a5b、显式冻结include/exec-c路径、-O2，
依次构造win/x86_64、lnx/arm64、osx/x86_64、osx/arm64、lnx/x86_64五切片，各40秒内rc0。
既有sizeof纪元host APE wrapper（seed/ape.c源同SHA ad3c21cf）挂载未变ff696包，封装rc0。
`out/c43-link-ape-01/unisacc.com` SHA4c2831c3fa1c871f5abf9549bcfd1f98f46f61554639706f698ed3ebd680e5b0。
真正APE上的原formats-elf-ape批68/0，out/c43-link-ape-formats-01；先前薄包装59/1保留，不冒充APE。
完整APE上的原链接缺陷批仍4/1，唯一失败direct-C迟绑定合同冲突，其余静态拒绝与正序rc106均通过。
这是未准入私有候选，不替换共享3a5b，不授外国宿主运行或完整发布资格。

有限源码适用证明入口`tools/link-source-delta.cx`已实跑：对两冻结根逐项核对
src/exec/unisa/include/weights/kernel/seed/iterate-kernel的完整目录名、类型/模式、普通文件字节、符号链接字面目标，
只接受src/tapelink.c固定b9af154f→16925f64这对哈希，其余差异拒绝。
实读1932文件/49目录/8符号链接，唯一允许差异1；witness
a064ffa55512277cc8f51e7f0739ad08ef808a206b39aee87969a80458b07d94，rc0。
交换根顺序实测rc2 unapproved-link-source-pair；证据source-delta*.stdout/stderr。
该证据不是原子文件系统快照或完整仓库身份，也不单独授准入；默认ni_compare仍保持原拒绝。

统一入口`tools/link-candidate-check.cx`已实际跑通：固定旧产品3a5b/record0523、固定新候选4c28，
直接提取两APE尾包并逐字比较为原PCff696；固定identity-helper新读两源身份，默认-run调用源码差分
及34份JSON复验（绑定两工具源码和子输出SHA），结束再次核两源/启动产品/旧记录/候选身份。
material-check-02.stdout实际rc0，原55秒预算不变；旧已装产品冒充候选的负控rc2 candidate。
证据out/c43-link-ape-01/material-{check-02,wrong-candidate}.*。只输出有限材料适用CHECKED，
尚未写产品build record、未两阶段安装；这不是完整发布资格或原始构造执行收据的重建。

随后已由原固定identity-helper实际write并check候选构造记录（两命令rc0），
记录产品4c28/3007570字节、源8390、提交896888f424fa2b2a98862cd3316063a22dc2dc35，settings为空。
冻结根原archive补本地Git元数据：首次短提交fetch失败，完整40位fetch成功；update-ref/read-tree仅写元数据和索引，
tracked diff为空，原始源身份校验通过。记录位于out/c43-link-ape-01/unisacc.com.build.json，
provenance{,-check}.stdout/stderr保留；这仍是构造身份记录，不冒称kind4原材料准入或已安装。

实际两阶段开发安装完成：tools/link-candidate-install.cx每阶段重新调用有限candidate-check，
固定旧产品/记录及新产品/记录SHA；prepare独占创建旧备份、暂存pair和绑定工具/检查输出的prepared.tsv；
finish重核同一上下文、备份与暂存字节后两次rename并fsync，成功后写installed.tsv。
prepare9653/finish78640均rc0，错误候选finish拒绝时共享仍3a5b，暂存仍4c28。
共享现4c2831c3/record820b9ae4，installed.tsv SHA13a9f8102b7c018cffbce91a592fcd248fca0f392ca39306017a872ae57a67d2。
旧备份out/c43-link-install-01/previous.com及记录保留。安装后缺陷批4/1（仅direct-C合同冲突），
smoke16/0。源码身份指冻结8390；共享源码另有cxmod与stream-runtime后续改动为e4bf22ba，未混称包含。
此新有限路径不是kind4原构造流程；没有补造历史107作业，没有签名或发布资格。

4c28已安装后追加回归：library-abi实际55/0、negative-diagnostics211/0（known_gaps1、archived17保留）。
日志out/c43-4c28-abi-02及out/c43-4c28-negative-01；ABI第一次误用三参数入口rc2，修正为其ROOT单参数后通过，原失败保留。
coverage中旧1/4缺陷描述改为本候选实际4/1及明确合同冲突，没有移除原direct-C断言。

- 现象：`int used(void); int main(void){return used()+2;}` 经 `-c -b lnx/x86_64 -funit` 得 main.o 后，`unisacc -o img main.o`（或加只定义 `unused` 的归档、或直接 `-o img main.c`）rc 0 并写出镜像；运行 SIGSEGV（rc 139）。预期：`link: undefined reference to used`，rc≠0，无镜像。
- 定位：src/tapelink.c:351 `tl_check_undef`（R20-3）只检查名字以 `g_` 开头的未定义外部（全局数据对象），函数外部不检查。
- 同现：经典参考 REF 0ed46192 行为相同（共享缺陷，非回归）；产品==REF 字节差分仍有效，不阻塞候选 f3705dfa。
- 复现入口即批次：`tests/batches.tsv` 行 `link-undefined-function`（tools/link-undefined-function.cx），当前红 0/3，预期不放宽；日志 out/c43-cc-reg-02/link-undefined-function.log；另有 out/c43-cc-link-undef-repro-01/REPRO.md。
- 拟议修复（由 src 域所有者 cdx/grk 在本纪元合入；cc 不动 src/）：`tl_check_undef` 去掉 `s[0]==103 && s[1]==95` 前缀条件，对所有 `tl_ext` 中未在 `tl_def` 出现的名字（除链接器自身生成的 `__init`）发 `link: undefined reference to NAME`。需同步 REF（tools/build_ref.sh 源）以保持差分；下一慢车道纪元重建后批次转绿。
- 裁定：fable 2026-10-28（选项 B+C 修复部分）：拒绝条款不属 archive.product-ar 格合同，归独立批次。

### 拟议补丁（2026-10-28 17:4x，cc 起草，未应用；src/tapelink.c 由 src 域所有者合入）

事实核对（冻结导出 f3705dfa，`-c -b lnx/x86_64 -funit` 单元 tape 的链接属性）：函数外部记为裸名 `.extern used`；全局数据记为 `.extern g_shared`；宿主库调用（printf/malloc/free）**不产生** `.extern`；链接器自身提供的 `__init` 以 `.extern __init` 出现，须白名单。

```
--- a/src/tapelink.c
+++ b/src/tapelink.c
@@ int tl_check_undef(void) {
-        if (n > 2 && n < 250 && s[0] == 103 && s[1] == 95 && !tl_inset(tl_def_at, tl_def_len, tl_ndef, s, n)) {
+        if (n > 0 && n < 250 && !tl_is(s, n, "__init") && !tl_inset(tl_def_at, tl_def_len, tl_ndef, s, n)) {
             pre = "link: undefined reference to "; k = 0;
             while (pre[k]) { msg[k] = pre[k]; k = k + 1; }
-            n = n - 2; s = s + 2; while (n > 0) { msg[k] = *s; k = k + 1; s = s + 1; n = n - 1; }
+            if (n > 2 && s[0] == 103 && s[1] == 95) { n = n - 2; s = s + 2; }   /* print data objects without the g_ tag */
+            while (n > 0) { msg[k] = *s; k = k + 1; s = s + 1; n = n - 1; }
```

验收：批 `link-undefined-function` 三行转绿（`link: undefined reference to used`，rc≠0，无镜像）；`objects-link`、`formats-objects`、`formats-archive-ar`（17）、`formats-elf-ape`（68）不变；REF 需同步同一补丁后 `optimization-corpus-*` 六目标字节差分保持 0 DIFF。风险：若某单元对 `_start` 或 `__init_u` 发 `.extern`（本次夹具未见），需把它们加入白名单。

## 观察（非缺陷）：相同字符串字面量的指针相等性（2026-10-28 ccdiff-4 探针）

- `const char *s1="dup",*s2="dup"; s1==s2`：宿主 gcc 11 输出 1（字面量合并），产品 f3705dfa 输出 0（不合并）。C99 6.4.5p6 明示"相同内容的字面量是否为同一对象未指定"，两者都合规；不入差分语料，不改产品。

## 观察（诊断质量，低）：无 main 的链接拒绝文本不具名（2026-10-28 link-edge-cases）

- `unisacc -o p a.o`（a.o 无 main）：rc 1、无镜像（合同满足），但 stderr 为 `reject: not covered: address or target declaration`，未说明"undefined reference to main"。建议 src 域所有者在 tl_check_undef 补丁同批加 `_start`/`main` 缺失的具名诊断。批 `link-edge-cases` 以 INFO 行记录现文本，不断言。

### 同根第二现象：跨归档顺序（2026-10-28 17:5x，cc 探针）

- `m2.o A.a B.a`（A 含 outer，B 含 inner，outer 调 inner）rc 0 且退出 106；`m2.o B.a A.a` 也 rc 0，镜像运行 SIGSEGV。原因：exec/c/compiler.c:401 / src/tapelink.c:446 归档按给定顺序各自扫到不动点，不回扫前面的归档（与 GNU ld 单遍语义相同——ld 在此情形报 `undefined reference to inner`），而未定义函数又不被 tl_check_undef 拦截，于是静默链接。上述补丁合入后此例转为具名拒绝（与 ld 一致）；是否改为跨归档回扫属产品合同选择，未要求。
- 复现入口：批 `link-undefined-function` 行 `cross-archive-order.reversed-refused`（工作顺序行 `working-order` 断言 106，保证非空洞）。

## driver.diagnostics — 驱动与 ar 的四处不具名/错因拒绝（开放，诊断类；exec/c/compiler.c，所有者 cdx/grk）

批 `driver-diagnostics`（tools/driver-diagnostics.cx，7/11，红行即复现）：
1. `unisacc -run u.o`（单元对象）：rc 1，**stderr 为空**。预期具名拒绝（如 `cannot run a unit object; link it first`）。
2. `unisacc -run ok.a`（仅归档）：文本 `these objects were compiled for another target; running needs this machine's: `（目标名为空，且原因错误：问题是没有可运行的单元/main）。
3. `unisacc -o p ok.a`（仅归档链接）：文本 `model driver: tape domain: required tapecheck model is missing`（错因）。预期与无 main 一致的具名拒绝。
4. `unisacc ar zz empty.a`（空归档上的非法操作）：rc 0 无输出；product_ar 只在遍历成员时校验 op（compiler.c:445），空归档不触发。预期 `ar: operation must be r, q, t or x` rc 2（非空归档已如此）。
附：`ar q2`/`rcsX` 等后缀不校验（op[0] 判定），低优先。

## pp.negative — 预处理器两处未拒绝的约束违反（开放；E2/E1 模型域，所有者 cdx/grk）

批 `pp-negative`（tools/pp-negative.cx，8/10，红行即复现；宿主 gcc 11 对两者均 rc 1）：
1. `#define 3x 1`：产品 `-E` rc 0、无 stderr。C99 6.10.3："The identifier immediately following the define…"——宏名须为标识符，须诊断。
2. `#include <stdio.h`（缺 `>`）：产品 rc 0 且**静默包含了 stdio.h**（stdout 56860 字节），无 stderr。C99 6.4.7 header-name 必须闭合；须拒绝。
诊断质量（INFO，不断言）：`unterminated conditional directive`、`argument count`、`#if expression`、`unterminated macro call` 四类拒绝文本无 `file:line`，而 `#error`、`#else without #if`、`no such file for #include` 有。
非缺陷：`#foo bar` 为 6.10 非指令（产品接受合规，gcc 选择报错）；`#define __STDC__ 2` 产品与 gcc 都接受（gcc 仅警告）。

## c.negative — 七处 C99 约束违反被静默接受并产出对象（开放；E3/parse2 模型域，所有者 cdx/grk）

批 `c-negative`（tools/c-negative.cx，10/17，红行即复现；宿主 gcc 11 `-std=c99 -pedantic-errors` 对七者均 rc 1；产品 rc 0、无 stderr、写出 .o）：
1. `const int k=1; void f(void){ k=2; }` — 6.5.16p2 可修改左值。
2. `int j(void){ case 1: return 0; }` — 6.8.1p2 case 须在 switch 内。
3. `struct S{int a;}; struct S{int b;};` — 6.7.2.3p1 同作用域重定义。
4. `int v(void){ int a[3]; return a; }` — 6.8.6.4p3/6.5.16.1p1 指针→int。
5. `int *p=0; char *c=p;` — 6.7.8p11/6.5.16.1p1 不兼容指针初始化。
6. `typedef int T; typedef char T;` — 6.7p3（C99 禁止 typedef 重定义）。
7. `void vv(int a, int a){}` — 6.9.1p9/6.7p3 重复形参。
前四项影响语义（接受错误程序），后三项为类型系统漏检。十项已正确拒绝的里有 5 项诊断带 file:line（INFO）。

### c.negative 第二轮（2026-10-28 18:0x）：再添八处静默接受（批现 19/34）

8. `int *p = 5;` — 6.7.8p11 int→指针初始化。
9. `int k2(int); k2(1,2)` — 6.5.2.2p2 原型存在时实参过多。
10. `int n2(int); char n2(int);` — 6.7p4 函数类型冲突。
11. `enum E2{A2,B2}; enum F2{A2};` — 6.7p3 枚举常量重声明。
12. `int t2[0];` — 6.7.5.2p1 数组大小须大于零（gcc 扩展接受，`-pedantic-errors` 拒绝）。
13. `x = (void)0;` — 6.5.16.1p1 void 值被使用。
14. `const char *c="x"; c[0]='y';` — 6.5.16p2 经 const 指针写入。
15. `int i; i += "str";` — 6.5.16.2p1。
诊断质量：`int s2[-1]` 被拒但文本为 `reject: not covered: tape data directive`（错因，INFO）。

### c.negative 第三轮（2026-10-28 18:0x）：再添六处（批现 20/41，漏检累计 21）

16. `char *p; int *q; p == q` — 6.5.9p2 不兼容指针比较。
17. `int x = 1; int x = 2;`（同一块作用域）— 6.7p3。
18. `case 1: case 1:` — 6.8.4.2p3 重复 case。
19. `a[1.5]` — 6.5.2.1p1 下标须为整型。
20. `int a19(void){ return a18(); }`（a18 返回 void）— 6.8.6.4p1。
21. `const int c; int *p = &c;` — 6.7.8p11 丢弃限定符。

### c.negative 第四轮（2026-10-28 18:0x，声明层）：声明层覆盖良好，再添两处（批现 20/43，漏检累计 23）

22. `void b15(void x){ }` — 6.7.5.3p10。
23. `static extern int b16;` — 6.7.1p2 多个存储类。
位域宽度/类型、VLA 存储、register 取址、函数数组/返回数组、`long long long`、`unsigned float`、不完整类型、union 多初始化器等 18 项均已正确拒绝。

## 观察（有效程序被拒，低）：`int vla_fs[(int)1.0 + 0];`

- 文件作用域数组大小为整型常量表达式（C99 6.6p6 允许把浮点常量强制转换为整型），gcc 接受；产品拒绝（`-c -b`，rc 1）。属 E3 常量折叠对强制转换的覆盖缺口；非安全问题。

## link.malformed-input — 畸形输入：两处容器不校验、一处产品自身崩溃（开放；exec/c/compiler.c tl_tape 定位 / 运行器）

批 `link-malformed-input`（tools/link-malformed-input.cx，8/11，红行即复现）：
1. **S2 产品崩溃**：`_start:\n  imm r0, 7\n`（无 ret，落出末尾）`-run noret.tape` → 产品进程 SIGSEGV（rc -11，无 stderr）。预期具名拒绝（"control falls off the end of _start"）或运行期有界终止。`-o` 镜像路线 rc 0 并写出镜像（INFO）。
2. S1 容器不校验：ELF 魔数被覆盖为 `XXXX` 的单元对象仍被链接成镜像（rc 0）；截断到前 2000 字节（无节头表）的对象同样被链接。tl_tape 直接搜索单元 tape 负载而不校验 ELF 头/节表。预期：`not an ELF unit object` / `truncated object` 具名拒绝。
绿：截到 100 字节对象、tapebin 翻字节/截断/坏 minor、未知操作码 tape、二进制垃圾 tape 均具名拒绝。

### 验收（2026-10-28 18:5x，已装 Root 4c2831c3 —— prd.md:159 链接修复产品，含 896888f424；红批复跑时共享产品已由 3a5b76ab 换为 4c2831c3，日志 IDENTITY 为准）

- 批 `link-undefined-function` 在 4c2831c3 上 4/1：object-only、archive-without-definition、cross-archive-order（逆序拒绝 + 工作顺序 106）均转绿——`link: undefined reference to NAME` 已具名；**仅余 direct-c-route**（`-o img main.c`，直接 C 路线不经 tapelink 的未定义检查，cdx 提交注记 "retain direct-C gap"），仍 rc 0 + 崩溃镜像。triage #14 由 S2 降为"direct-C 路线一例开放"。
- 其余红批在 4c2831c3 上计数与 f3705dfa 相同：driver-diagnostics 7/11、link-malformed-input 8/11、pp-negative 8/10、c-negative 20/43（日志 out/c43-cc-reg-3a5b-01/）。
