# prd_hist.md —— 历史与证据记录（自 prd.md 迁出，逐字保留）

本文件收纳曾追加在 `prd.md` 里的**带日期的完成记录与实测证据**（原格式 `〔日期 任务ID〕…`）。它们是历史，不是现行规范：现行规则、能力与状态只看 [prd.md](prd.md)。新增完成记录请写 `agentwork/ledger.tsv`，**不要再追加到 prd.md**；需要长期保留的证据放这里并在 prd.md 的索引里加一行。

迁出时间：2026-11-09（cc）。原位置：prd.md §10「归档索引」第 505–673 行。顺序未改，内容逐字，仅在末尾追加了本说明。

---

〔2026-11-08 cc评审后续，C47-DRIVER-TARGET-OBJECT / C47-LOGIC-DISTRIBUTION〕先登记两项任务。修复已装0.0.47的--target拒绝与裸-c只出tape回退，保留默认-run最小复现，补六目标格式合同并重建核身份；独立普查表/生成程序/网/经典机制代码及六片解压模块字节，明确统计分母、重叠与未归因，为D1设计提供数据。当前均DOING，未借旧CI或发布资格。

〔C47-DRIVER-TARGET-OBJECT 定位/决定〕已装0.0.47默认-run最小main成功；--target lnx/x86_64 -o报option not migrated，裸-c输出_start tape（out/c47-driver-fix/*.before.log）。--target只选目标不覆盖-E/-S/-c；裸-c缺目标时取tn_self，非Linux沿现有-funit容器合同（Mach-O/COFF只载unit tape，由unisacc链接，不宣称系统链接器可用），保留-S与--tapebin显式文本路径。旧cxt_object低层拒绝合同保留，driver在展开前规范化。

〔C47-LOGIC-DISTRIBUTION，2026-11-08，实际实跑，尚未完成最终镜像模块归因〕新增 `tools/logic-distribution.cx`：`--sources ROOT` 按 tracked regular 文件、固定根 src/cx/exec/seed/weights/tools 做互斥的 TABLE(.tsv) / GENERATOR(*-gen.cx、seed/gen.c、seed/gold_*) / NETWORK_REPRESENTATION(.net/.tbl/.rows) / CLASSIC_MECHANISM(其余源码)分组，列每文件字节/物理行及分类分账；if/case/switch/? 跳过注释与字面量后只作词法代理，不等于语义决策或运行频率。当前2290文件、13,760,638B：表5,771,926B(41.9451%)，生成器1,450,565B(10.5414%)，经典机制6,538,147B(47.5134%)；tracked网表示0B仅因构造产物未跟踪，绝不表示产品不含网。事实表与其生成网有信息重叠，不能把以上字节份额当“以网换逻辑”的决策覆盖率。独立语义oracle仍未补齐。

同工具 `--ape FILE --package FILE --slice TARGET FILE` 核实际解压CRC/SHA、包内模型/资源/元数据，六片当前实测：lnx/x86_64=1,880,766B、lnx/arm64=1,884,814B、osx/x86_64=2,410,978B、osx/arm64=3,071,458B、win/x86_64外层PE=2,663,424B（含APE stub/version，不冒充原始裸片）、独立win/arm64=3,367,424B、sha889f36cd…（父提供公开候选资产）。包1,665,266B=模型存储1,315,916B+资源241,070B+目录/元数据108,280B；模型行以PACK_ROUTE绑定流与索引，资源列真实名字。证据 `out/logic-distribution-current.tsv`、`out/logic-distribution-slices.tsv`，输入为当时已装fde2db18…产品与公开win/arm64，父后续重建必须重新绑定身份。

**仍缺最终模块字节归属，目标保持未完成**：当前镜像无来源映射，经典路径tl_unit→prune→lower→elf丢单元身份且可跨单元融合；不能按对象.text比例或旧 `plans/unit-text.tsv` 分摊现候选镜像。工具每片明确 MODULE UNATTRIBUTED、归因0B并核守恒；这只验证未知账完整，不构成模块分账完成。下一步按 `plans/link-map-sidecar.md` 在构造链保留单元来源集合、融合记SHARED、最终布局输出同镜像SHA绑定的不重叠区间，再由普查消费；不能仅补节区或符号大小冒充模块。检查 `tools/check-logic-distribution.cx` 已装产品默认-run实跑6/0，覆盖词法正负例、分类、未知模块守恒、坏格式及无参拒绝；批次登记工具因既有inventory-root-layout拒绝需要父处理，不记完整批次PASS。

能力速查补充：逻辑/体积普查 `sh ./unisacc.com tools/logic-distribution.cx -- --sources . --ape unisacc.com --package unisacc.com [--slice TARGET FILE]`；检查 `sh ./unisacc.com tools/check-logic-distribution.cx`；缺最终机器码模块来源时明确UNATTRIBUTED，不按比例摊账。

〔C47-DRIVER-TARGET-OBJECT 测试入口修复，2026-11-08〕已装默认-run把argv[0]设为实际临时可执行文件，不是.cx源路径；tests/run.cx的inventory原先依赖argv[0]导致--list误拒inventory-root-layout。改为源身份__FILE__定位仓根，保留realpath失败、ROOT/tests/run.cx后缀、metadata-io/shape/schema的全部拒绝；修改前--list实际红，修改后须真实--list与smoke验，不用薄解析器代替入口资格。

〔C47-LOGIC-DISTRIBUTION 来源设计裁决〕仅设计评审建议：先做合并tape来源账本、诊断模拟器最终写出事件、绑定规则内容SHA的来源证书；只认领完整来源链已证明的模块区间，其他继续UNKNOWN。输入cursor/输出长度只证明执行关联，相邻标签差或字节cmp不能补来源证明；通用复制/切片可先证，融合需来源集合，控制依赖不得直接叫SHARED。私有测量不复制lower算法、不增加产品默认路径状态；先用两单元同名局部标签/共享BSS、缓冲覆盖、双遍布局负控证明最小接口，再量真实六片。报告在out/c47-driver-fix/provenance-design.txt，仅设计未实跑，不算模块分账已完成。

〔C47-DRIVER-TARGET-OBJECT 测试入口实证〕`tests/run.cx -- --list`修后实际metadata_valid=1（555批），smoke实际16/0，缺表仍两条metadata-io errno2；`tests/run-batch.cx`对logic-distribution实际PASS/6断言，绑定旧已装fde2db18…。既有tools/check-register.cx在55s看门狗处超时，前段断言绿但全检查未完成，原始日志保留、不记PASS。新增裸-c对象→产品链接镜像→执行返回23两条永久断言；对完整私有APE f069d4c9…实际cli 9/0（旧根产品仅作启动器，测试ROOT明确私有），不声称系统链接器兼容。证据out/driver-target-object-initial/{registry-fixed,smoke-fixed,registry-missing-tables,register-fixed,run-batch-logic-absolute,cli-final-object-link}.log。

〔C47-DRIVER-TARGET-OBJECT 身份工具缺陷〕本地新APE已安装并备份旧6215778B产品，但formal seed/ident写源记录后，migrate-build-record在原记录缺kind/digest两字段时产出缺逗号的无效JSON；已保留identity-migrate.log，不能记身份核验完成。定位为build_record的末字段判断及追加kind逗号条件均反向，须修生产者并用四种字段存在组合校验，再重生成当前记录；不手改资产摘要冒绿。

〔C47-LOGIC-DISTRIBUTION 诊断写出事件MVP，2026-11-08〕`tools/sim.cx --write-events NEW.tsv [--event-limit N] [--event-bytes N]` 只在显式启用时收集关联：state/key/action序号、frame实例/content SHA/cursor、COPY/SPAN输入范围、输出/诊断/scratch缓冲长度、SWAP前后输出代次。OFILL同长覆盖、OCUT/OCLR/ODROP显式记事件，不依赖长度差；非直接复制来源记-1/UNKNOWN，所有模块所有权仍UNKNOWN。事件上限默认100000、硬上限1000000；文件上限默认16MiB、硬上限64MiB，含身份与COMPLETE元数据。只在ACCEPT后以独占临时文件写完并原子无覆盖hard-link发布，拒绝/超预算不发布完整账本。delta/input/output绑定SHA；输入为冻结材料，资源/外部文件来源链未补齐，事件不是模块所有权证书。父安装产品sha=f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82，工具默认-run实跑独立小delta开关均输出B，首轮check13/0；帧实例与SHA缓存增强后按新输入重跑，不借首轮证据。真实六片最终模块分账仍未完成，下一步消费事件连接合并tape来源与最终布局，融合/共享来源需另证。

〔C47-DRIVER-TARGET-OBJECT 本机产品实证〕冻结3b41cc224的六片均已构造，mac/win whole-o内限50s、Linux独立对象每步<=55s；完整APE f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82 / 5818178B，单列win/arm64 fcfcc4135d9bd7fc71cd70e5dfae694c969047442d9594b2c0b49e0ac84792e6 / 3369472B。最终APE四批格式/对象20断言全绿（OS三批11、cli9，含对象经产品链接后实跑23），smoke5/0。本机已经install-ape安装，旧公开字节和侧车备份out/c47-driver-fix/before-install；未改远程release/tag/资产，原CI资格不迁移。身份生产者两处逗号判断已修，四存在组合及幂等8/0；源摘要9cc514d3…，新侧车配对，ROOT_IDENTITY9/0；plutil转换真实解析成功。侧车commit ac8f94dd7与冻结3b41cc224的产品输入src/cx/exec/include/seed/ext/kernel/weights零差异。完整棘轮/注册工具长检查仍未绿，tools功能扩额许可待答，不能称全仓通过。

〔C47-LOGIC-DISTRIBUTION 事件MVP冻结验证〕事件增加 `dst_lo/dst_hi`，OFILL取真实保留位置与宽度（非零起点1、同长覆盖[1,2)），追加取旧尾至新尾，截断/清空/丢尾/SWAP取被移除范围；文本key固定为本次edge选择前的观测值，不随同seq后续PUSH变化。帧实例递增、内容SHA缓存只用于诊断。最终已装产品f069d4c9…默认-run检查 `out/sim-write-events/check-final-bound.log`：SIM_WRITE_EVENTS_CHECK checks=14 failed=0，rc0；OFILL范围断言限制在OFILL同一事件行，防SPAN相同区间误绿。新批 `sim-write-events` 已写注册与覆盖行；14/0为直接检查证据，注册表全批执行由父集成，尚不借此宣称最终六片模块分账完成。无工具进程在途。

〔C47 两项同候选批次收据〕已装f069d4c9…下，driver-target-object-lnx/osx/win/cli、migrate-record-json、logic-distribution、sim-write-events七批真实run-batch均PASS/rc0/transport0，断言分别5/3/3/9/8/6/14。首轮父未先建ABS_OUT_PARENT，七次均canonical-root/output-parent errno2拒绝、未执行批次，原*.batch.log保留；创建目录后新*.registered.log绑定真实执行，未复用PASS。①回退修复的本机目标已验；②最终机器码模块分账仍UNKNOWN，事件MVP不是来源证书。tools行数扩额尚未收到答复，基线保持原值；不把七批局部通过称全量资格。

〔C47-LOGIC-DISTRIBUTION 合并边界下一步〕先复用产品compiler.tapelink的真实tl_unit emitter，保存有序对象SHA、各次调用的合并输出区间及合成__init范围；标记emitter-context而非最终机器码所有权，共享全局去重不凭首个emit调用归独占。新工具tape-origin与check只取独立对象（archives显式拒绝），保持名字改写/重复定义/目标不一致/缺入口/undefined等真实拒绝。后续须与编码器blob/记录传播补边组合；不把本层区间当六片最终模块账。

〔C47-LOGIC-DISTRIBUTION 新口径：生成程序与选择器分开〕现有GENERATOR仅统计构造器源码，未度量生成出的有限程序；不能据其份额替代产品逻辑分布。新增只读普查将内嵌模型先经正式decoder完整验证，再以现成model_number定位UNINETB1四段：header、S常量、Q/C action程序、T/N state/selector余段；kindT决策表与kindN网络选择器分别报告。raw四段字节精确守恒，压缩成员只报整体stored，不按比例摊section；actions区分编码的显式动作与C前缀展开动作。该统计仍为表示体积/结构代理，语义决策比例UNKNOWN。

〔C47-LOGIC-DISTRIBUTION 生成程序普查实测，2026-11-08〕新增 `sh ./unisacc.com tools/model-program-census.cx -- --package unisacc.com`（也支持 `--model FILE`，`--arity PATH` 默认core-ops事实表）。使用正式package/model decoder完整验证，codec解压并核CRC；另以现成model_number仅定位二进制跨度，Q/C与decoded nq/qlen/qa核一致，opcode arity从 `exec/facts/core-ops.tsv` 读取。40成员均kindN（kindT=0），**这层分母仅为解压模型4,833,789B，不能与上面的13,760,638B源码分母合并**：header797B、S常量189,035B、Q/C生成动作程序2,886,663B、state/网络选择器1,757,294B，四段精确守恒。总states116,728、显式rows143,590、sequences79,038、动作显式/前缀展开均774,497；rows是编码行/阈值数，不是语义决策覆盖率或完整输入域。Q/C动作程序与N选择器是不同的产品组成，“生成器源码”不能代表“生成程序”，模型也不全是选择器。压缩stored只逐成员原样报告，绝不按raw比例分摊四段。

本次输入APE sha `f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82`，arity事实表sha `7527789f1322b67e5f6b3fe66f64a994ae5e5b42ce24b5ea69779c5ce92ad458`，结果 `out/model-program-census-final.tsv` sha `2b445ea26f3629616dac245bdc9de24be36f453209512ca20770f5cf3a0797d3`，真实40成员命令rc0。独立最小UNINETB1 T/N、空S、Q/C前缀复用、截断/未知opcode/错prefix/错arity拒绝检查默认-run 9/0，证据 `out/model-program-census-check-final.log`；批次 `model-program-census` 注册实跑另列，不借直接check冒充注册门禁。此工具仅回答表示体积与结构数量，模块所有权与独立语义oracle仍未补齐。

model-program-census注册验收实跑终态：`out/model-program-census-register.log` 输出 `BATCH_RUN id=model-program-census status=PASS rc=0 marker=MODEL_PROGRAM_CENSUS_CHECK candidate=f069d4c9… scope=actual-one-batch-no-cached-result` 与 `REGISTER status=REGISTERED`；真实会话91311 rc0，无在途进程。仅本批通过，不表示整个普查/模块归因目标完成。

〔C47-LOGIC-DISTRIBUTION 合并来源账本实证〕`tools/tape-origin.cx --entry INVENTORY FROZEN_BUILDER OBJECT_DIR --out NEW_OUTDIR` 从已验证完整的slice-inventory与冻结entry-build编号重建真实链接顺序，不依赖收据完成顺序或文件名排序；复用compiler.tapelink，不复制链接算法。82对象（44根+38闭包）生成8,123,582B合并tape，SHA `d97a2288606b7012f13cf879420e41482ee4dcad814d2275d3beeb4f1ac7b1a3`；emitter上下文8,122,102B+合成初始化1,480B精确守恒。仅记planned_source，去重共享所有权仍UNPROVEN。输入收据复制到origin-material，P3路径仅重绑定到相同fde2db18…字节备份，原始收据未改；不是新构造证据。真实已装f069d4c9…重放该tape所得ELF SHA `2343d81333db0359fbba3336b157785b1485c0d0c798be97d62db466d42c9818` 与本轮lnx/x86_64/entry.elf cmp0，证明合并路径一致，不证明最终模块边界。

新增entry-order四项后检查与正式批次均实际16/0，最终批次日志 `out/c47-driver-fix/tape-origin-final.registered.log` 为PASS/rc0/transport0，绑定f069d4c9…；覆盖反序inventory、重复builder key与未验证inventory拒绝，保留原12项真实对象/同名局部/共享BSS及拒绝断言。最终元数据版 `origin-lnxx64-v2/merged.tape` 与前版cmp0；账本在同目录origin.tsv。能力速查：`sh ./unisacc.com tools/tape-origin.cx -- NEW_OUTDIR ORDERED_OBJECT...` 或上述--entry接口；`sh ./unisacc.com tools/model-program-census.cx -- --package unisacc.com` 分开报告生成动作与选择器。六片最终机器码模块归因仍未完成；下一步补blob创建/使用、缓冲代次与属性读写版本，连接编码器重排至最终文件。

〔C47普查同候选更新与D1结论〕已装f069d4c9…的六片/包普查实际rc0，`out/c47-driver-fix/current-candidate-slices.tsv`：lnx/x86_64=1,880,862B，lnx/arm64=1,889,006B，osx/x86_64=2,410,978B，osx/arm64=3,071,458B，win/x86_64外层PE=2,664,960B（含APE stub），独立win/arm64=3,369,472B。包内模型/资源/元数据仍1,315,916/241,070/108,280B；不得用旧fde2db18…两片Linux尺寸或旧win/arm64身份替代本轮。就解压模型分母而言，动作程序59.7184%、选择器36.3544%、常量3.9107%，只说明存储表示；生成器源码、已生成程序、表与网须分层记账，存在信息重叠。D1不能由这些份额声称逻辑消失或语义正确：经典机制仍需独立oracle与跨目标合同，第一项已补六目标格式回归，而完整同候选原生资格与最终模块贡献另需证据。

〔C47编码来源blob边实证〕显式write-events新增EDGE：OCUT/BLOBSAVE/SBSAVE记录新blob身份、内容SHA和源缓冲/代次/范围，INPUSH连接blob_id到独立frame实例，SWAP连接旧output代次到新frame，SBCLR递增scratch代次；相同内容的不同blob不靠SHA合并身份。所有EDGE与EVENT共用预算，仍仅ACCEPT后独占原子发布。独立小delta默认与开启日志均输出BQ，解析字段核BLOBSAVE的B子串、SBSAVE的Q、OCUT的BQ及INPUSH/SWAP连接；原14项断言保留，预算因新增两条真实边改为精确12/超界11，不放宽资源硬上限。直接检查20/0（真实会话7701退出0）与正式sim-write-events批次20/0/transport0（真实会话17974退出0）均绑定已装f069d4c9…；日志out/c47-driver-fix/blob-edges-check-final.log、blob-edges-final.registered.log。STX/LDX地址及最后写版本、SBBLOB等边仍缺，不能据本次称因果完整或最终模块已归属。

〔C47真实编码模型内容绑定〕冻结3b41cc224源的seed-gen `enc --elf` 实际生成JSON→seed-tbl→seed-net→现成nw_binary_net（不compactQ），64920B SHA `411198b71aa84eb74d48cdfdf5790181acb746fc24a7eee58e0e32f755b79be7` 与已装f069d4c9…包模型12解压内容逐字节相同，CRC/正式decoder通过；包route实际绑定lnx/x86_64/image/O0等elf阶段。前网络图 `out/c47-driver-fix/model-binding/elf.json` 可用于带状态名的真实编码诊断，不借旧out/model-com缓存。此小构造明确依赖宿主Apple clang21及系统头/链接器，不冒充完整context/PC/GEN或零宿主依赖；lower/model13与其他38模型未重造。报告out/c47-driver-fix/model-binding-report.txt（sha ac9793b5e367db3e5ebf13935f7f7ea9c11ee2d38ee881d344f138e246fdae15），全部真实会话已退出。下一增量在私有诊断补STX/LDX地址和值的最后写版本，避免BLB属性槽重用时追错旧blob。

〔C47属性版本及真实编码事件实证〕显式write-events的MEMORY行记录STX/LDX实际地址、值、frame/cursor与最后写版本；STX每次含零值都分配新版本并记previous_version，未写槽读取version0；诊断关闭时不构造版本map。独立delta先读未写槽，再B→Q→零覆盖同地址，LDX必须连接Q/零的实际最后写：默认/开启输出均Q。直接check及正式sim-write-events批次均25/0（会话68776、64531均exit0，日志memory-edges-check.log与memory-edges-final.registered.log），原20项保留。

冻结lower --full另已生成并绑定当前包model13 raw189100B SHA `8604703258355a176f850ec63ff55f4b78f43e09a2b1cf784e1b72e49e441b50`；产品-S真实main return7的tape经冻结lower得到minimal.target（24191B），当前带来源日志enc实际rc0产生8192B ELF SHA `ad5feb951792a7a8b4e2783061a5f2f9eb913bd82fc5fc5ec0bdd39e7d94643f`，40945个完整事件，日志out/c47-driver-fix/model-binding/minimal-current-events.tsv。手写ret.target曾因DEAD.reg拒绝，两次off/on rc1材料保留；换成真实lower输出有具体新输入理由。真实链仍待规则证书及合并单元到lower输出的来源证明，不认领模块字节。最新toolsloc实际红81046>80680，基线未改，扩额未获答复，不报全仓棘轮通过。

〔C47真实编码小例对照〕冻结sim的ELF、当前带事件sim的ELF、已装产品以同minimal.cx `--target lnx/x86_64 -o`写出的ELF三者实际cmp0，均8192B/SHA ad5feb95…；同源默认-run退出7是在Darwin宿主，未运行该Linux ELF，不冒称Linux原生证据。完整命令、终态与lower绑定见out/c47-driver-fix/lower-binding-report.txt（sha47757b8653b12ec5a72b654e7d1b57ac3319235d847b7b9c386321a0e1367c62）。三个相等证明日志未改变该例输出，模块归属仍需单独证明。

〔C47首个编码事件到最终字节的实证链〕只读核查固定事件2230的E.ret seq190=LDI195/OUTW→2231 OCUT/blob63→2233 BLB[68]版本437→24443 LDX同版本→24444 INPUSH/frame58→24445 COPY写重排text[336,337)→32745 LIB.imagebegin OCUT/text_blob995→32929 INPUSH/frame994→33266 EI.copy写file[512,513)，实读最终byte195，沿途无覆盖/截断失效。真实图被librarysymbols包装，OCUT状态是LIB.imagebegin，不能只据原始ELF.begin模板猜测。报告out/c47-driver-fix/ret-lineage-report.txt（sha b42a8bd09db3c85e0fa40668f4c7f3baf5336e603cb3bcd677e8f5ba329e700c）。这是特定编码事件到一个文件字节的链；输入ret完整行到事件尚缺INTERN/row_id显式边，原单元经prune/lower到target行也未证，模块仍UNKNOWN。下一步须补这两层并采用稀疏记录量真实切片，不能逐个手认字节冒充模块普查。

〔C47输入词身份下一增量〕私有诊断为INTERN/SBINTERN记录源帧/缓冲代次、精确半开子串范围、内容SHA及返回的intern身份；相同词在不同位置保留不同来源事件，不凭共享id合并模块归属。先以两行相同词和scratch复用的独立delta核位置与身份，再接真实图中的OPC属性读及E.ret规则；输入词所在完整行只能由同SHA不可变输入的行界核对，仍不把行位置直接当模块名。并行只读调查prune/lower实体来源合同及稀疏日志规模，暂不改产品。

〔C47 INTERN实际验证〕直接check29/0/rc0（会话59482），独立重复ret两行分别绑定[0,3)/[4,7)且scratch同id保持独立源事件；原25项保留。首轮29/1因新夹具留下ret输入使旧拒绝用例读到114而非65，修复为恢复其原始A输入，原失败日志intern-edges-check.log保留，不改拒绝期望。真实同输入/模型仅新增诊断，minimal-intern.elf与前版cmp0；44859事件/11405402B，INTERN在LW.i返回id59且精确子串[2224,2227)，内容SHA 85e4aea1…；并不靠cursor猜该词。注册批次尚待父本轮执行。

〔C47稀疏观察决定〕全写事件小例已约10MB，不能直接扩到8MB/439366行真实merged tape。新增仅私有sim的显式状态观察模式：配置列出实际状态与只读寄存器/指定地址槽，状态入口快照带frame SHA/实例/cursor、输出代次/长度及配置SHA；模式明确STATE_OBSERVATION_ONLY，未选择动作不称完整因果日志。现有完整write-events默认合同及1M事件/64MiB硬界保留。先用独立delta验证状态取值、坏配置拒绝与默认产物cmp，再为prune整行、lower实体/episode制定同模型内容SHA绑定的profile；不能按名字相似泛用证书。此模式是量测原语，来源解释另由固定规则合同检验。

〔C47 source/object身份缺口〕slice-inventory的VERIFIED只核字节/顺序计划，不独立证明源码等于对象。已实际用原冻结构造器fde2db18…、同snapshot cwd/source、原-O2 -c -b lnx/x86_64 -funit重编routestep.o，0.20s/rc0，输出SHA a5be9626…与候选对象cmp0。下一步用分批有界重放核82对象的原始优化参数及源码SHA；逐对象相等后才能把planned_source升级为该构造输入的实证来源，仍不替代模块到最终镜像的传播证明。不重建权重、不重跑全套测试。

〔C47 lower整行实体量测决定〕先以LINE入口前后输出长度及原输入完整行范围复原CL复制出的code缓冲，必须核复原字节SHA等于C.init所观测真实帧，不能靠词或标签相似。C.next/C.label将nc绑定该code帧完整行，再把GENERIC至C.advance的输出episode、PF/PL两行消耗分别绑定实体集合，其他规则/合成/数据仍UNKNOWN。实现独立消费检查，拒绝错input/delta/配置SHA、错帧、反序或重复实体以及不守恒复制；它只证明所登记规则的行来源，不把nc或intern身份当对象模块。当前真实小例watch已rc0/6264条，输入与原lower输出的cmp由消费校验接续。

〔C47稀疏观察实际交付〕`--watch-states CONFIG --write-events NEW.tsv`严格配置为STATE TAB REG,REG（当前只支持直接寄存器，不支持间接地址槽），最多64状态/每状态16不同寄存器/64KiB；未知状态/寄存器、重复/空字段及坏形均拒绝。仅选中状态入口记录frame实例/SHA/cursor/end、output/scratch代次和长度、指定寄存器值；关闭完整动作日志及属性版本map，header明确STATE_OBSERVATION_ONLY与config_sha，ACCEPT/预算/独占原子发布合同保留。新check13/0、原check29/0真实终态；sim-state-watch实际注册PASS/13/0/transport0，会话13804退出0。真实lower小例6,264条/1,632,524B，输出与原minimal.target cmp0；该规模不是全切片60秒资格。

冻结seed-gen prune→tbl→net另实际绑定包model8：raw23014B SHA33f2a9daec27e4f90d42781d7d01f1c76a975c1b9a04b18656b7e850591a98d7，完整memcmp=1/CRC通过；JSON sha048d415d42047197afddc2d1e1a3e8f80947efd753487e317680a58626134f48，可接真实整行prune量测，不信旧stamp。证据out/c47-driver-fix/model-binding/prune-binding.tsv，真实会话97920退出0。INTERN正式sim-write-events批亦29/0/transport0，会话96410退出0，前述“待执行”已由本收据闭合。

〔C47真实Linux切片诊断规模〕冻结8,123,582B/439366行merged经已绑定模型8→13→12的sim真实重放，各步rc0：prune2.83s、lower17.20s、enc17.93s；最终ELF SHA2343d813…与候选entry.elf完整cmp0。此轮不含全动作日志，未扩硬时限；prune输出逐字节等输入，不能冒称存在删行覆盖。来源观察下一步新增显式compact-v1：配置序号与数值列、每frame一次SHA/end声明，保留同样观测值、预算与限定claim，降低约89万状态观察的重复元数据，默认长格式不变；消费工具须同时验证帧表/序号/配置列，不可仅省掉身份字段。完整六片模块归因仍未达到。

〔C47冻结源码到82对象全部重放〕`tools/object-source-replay.cx` 将原构造器/receipt/inventory及各源SHA先记PREPARED seal，再在新输出目录按原目标/优化参数逐个实际重编并完整memcmp；每child8s、每批49s截止，不缓存PASS、不改原物。index0..81全覆盖：首批2，start2/18/34/50/66各16，六批真实exit0，82个全部BYTE_IDENTICAL；输入是冻结3b41cc224根和fde2db18…构造器，source-replay-seal.tsv只证明本次冻结，不冒充历史签名。源码/constructor编译后复核，receipt/inventory批末复核。独立真实最小源→对象→slice-inventory→seal→重编及篡改/路径/范围负控10/0，实际注册object-source-replay亦PASS/rc0/transport0，会话80794退出0。原首轮静态dup撞POSIX名、以及继承UNISA_CONTAINER拒绝均保留；后者修为子构造前明确清理override。可据本次确定性相等将该批对象与冻结源码关联，再与合并emitter上下文连接；共享数据独占性、优化传播和最终模块贡献仍不能从此直接推出。

能力速查：`sh ./unisacc.com tools/object-source-replay.cx -- --inventory INV --slice SLICE --sources NEW_SEAL --prepare` 只准备身份；随后同输入 `--sources SEAL --out NEWDIR --start N --count K` 实际重编（K<=32，父本轮用16），末行RECOMPILED_BYTE_IDENTICAL才是该批字节等价终态。证据out/c47-driver-fix/source-replay-{first-two-v2,batch-02,batch-18,batch-34,batch-50,batch-66}.log及register-object-source-replay.log。

〔C47紧凑观察与全片实体实际规模〕compact-v1独立check19/0、原完整check29/0，实际sim-state-watch批19/0；lower-row-origin长短统一解析检查21/0、实际注册PASS。真实全片前端观察27.20s/rc0，901415个FRAME+WATCHC记录、46,102,138B，未增加1M/64MiB硬上限，输出target与无观察full-lowered.target cmp0。全片实体消费已核完输入并开始输出，原逐行printf在20秒外限被杀，仅留下部分输出且无总结，**不能记全片消费PASS**；日志full-lower-rows.stderr保留。下一窄修将完整ENTITY输出按64KiB缓冲写出，保持全部行及原检查，不靠缩覆盖/summary模式规避规模。

〔C47完整实体账已生成〕64KiB缓冲输出后，原21项实际全绿（会话63129退出0）；同一完整901415记录的消费实际15.29s/rc0（会话69179退出0），生成48,248,191B完整ENTITY账，433975实体/7,956,487B code，code SHA d77c652bda755fc8761ceace35dc81c0864f9a8e00534c7d2c362a9326e215dc 与真实code帧相等，末行LOWER_ROW_ORIGIN rows=entities=433975闭合。未重跑原27秒观察、未删行、未改预算或验证合同；初次不完整full-lower-rows.tsv留存，正式新结果为full-lower-rows-buffered.tsv。独立消费工具接口 `tools/lower-row-origin.cx SOURCE DELTA CONFIG WATCH OUTPUT`，长/compact-v1共用SHA/帧/整行复制/nc检查；source-row-lineage-only仍不自动等于模块到最终字节。

本片prune另观察到FALLBACK在根frame1/cursor0、zero=0/size=8123582执行，输入/输出SHA都为d97a2288…；冻结图该规则明确OCLR→INPUSHXE(0,size)→SPAN2(0,size)→ACCEPT，因此本片是整帧身份传递，不是凭两份同字节猜内部行重排。实际日志full-prune-fallback-watch.tsv（1观察记录）及输出cmp0，4.65s/rc0；其它输入的prune删行路径仍须单独证。

〔C47 lower/encoder区间连接登记〕完整Linux lower episode真实消费20.08s/rc0：371917条已闭合GENERIC/PF/PL episode，1076644个target字节仍UNKNOWN，不能按最后模块兜底。独立check保留全部30断言，原合跑55s超时未记通过，拆--rows 21/0与--episodes 9/0实际通过。编码器新增encoder-record-origin/check，限定已内容绑定的model12静态Linux x86_64，将LW完整target行与WR.nx累计输出接成独占文件字节区间；最小实际1155记录/text4878B/image8192B，独立11/0。下一工具module-byte-map/check只连接经核SHA的origin→ROW→EPISODE→RECORD，模块贡献是发射上下文代码来源；多模块融合单记SHARED、合成初始化单列、其余lower规则/头/数据/对齐保持UNKNOWN。不能以lnx/x86_64证书外推其它五目标，六片仍DOING。

编码消费者全片首轮2351被50s外限终止（exit142），虽校验阶段已完成，输出只到q333668且无总结，full-encoder-origin.tsv不是PASS。与前序lower相同，setvbuf不足以降低运行时逐printf开销；下一窄修只改显式64KiB拼接/fwrite，所有RECORD保留，原超时材料保留，新文件full-encoder-origin-buffered.tsv再核。producer完整759328观察已23.98s/rc0，输出image与原候选相等；不能把producer成功替代consumer成功。

〔C47区间链消费闭合实证〕编码器显式缓冲窄修后，独立check11/0（21431退出0），完整consumer54329在原50s限内退出0；out/c47-driver-fix/full-encoder-origin-buffered.tsv SHA b74c57265d5652bf41edb549d0a25509747e756fc34fc8d8a18e23f9b606dd64，378536条RECORD、text1840773B，header[0,176)、code[176,1840949)、DATA_OR_ALIGN[1840949,1880862)，image SHA2343d813…与本轮候选一致。绑定target cf021fbf…/watch a1bcb30…，不能由同一SHA替代事务闭合校验。lower-row-origin注册批21/0、lower-row-episodes注册9/0、encoder-record-origin注册11/0均本机实际PASS/rc0/transport0，日志out/c47-driver-fix/{lower-row-origin-episodes-final.registered,lower-row-episodes-final.registered,encoder-record-origin.registered}.log；全部同候选f069d4c9…，不等于全仓门禁。下游module-byte-map尚在独立实现/核验，当前不报模块覆盖率，更不报六片完成。

〔C47其余目标来源前置〕lnx/arm64同为82对象entry-build路线，但现有slice-inventory/object-source-replay硬限lnx/x86_64，不能借该目标PASS贴到arm64。下一步在现有工具明确接收并绑定两个Linux目标，默认行为不变；receipt目标参与seal身份且重放命令使用该目标，source-plan仍经实际对象重编核。osx/win原whole-o路线需从真实单位发射阶段补源实体，不将另编独立对象的不同优化/链接结果冒充原片。六片分账目标不缩为两片。

〔C47五目标模型内容前置〕只读合同已定位真实路由：lnxARM lower/enc=9/2、osxARM17/16、osxx8621/20、winARM25/24、winx8629/28；ARM编码为两遍扫描且lnum未重置，不能沿用WR.nx证书。现先从冻结snapshot以既有native seed-gen/tbl/net重造这10模型，逐个与已装包raw完整memcmp/CRC核一致后才用其JSON制定观察；仍明确宿主工具依赖，不称产品重建或独立oracle。各相位内层有界，新路径在model-binding各目标子目录，所有原产物保留。whole-o sourceframe到tape传播尚待单独补证。

〔C47首片模块字节表实际接通〕module-byte-map/check已以SHA链连接真实origin→433975 ENTITY→371917 lower EPISODE/UNKNOWN→378536编码RECORD→候选image；独立一致性fixture8/0含2模块、融合SHARED单计、合成、UNKNOWN/zero、label/metadata空隙及错hash/重叠/ENTITY/count/缺尾拒绝。真实v3退出0，lnx/x86_64的1880862B分为82模块上下文代码1803567B、未覆盖代码36795B、合成初始化411B、HEADER176B、DATA_OR_ALIGN39913B，加总精确相等；SHARED0B只是本片命中情况，不证明融合永不同源。原v1把RECORD target跨度误要求连续的拒绝保留，最终仅要求target单调且file/text严格连续，因label/@sym行本来无编码RECORD。planned_source发射上下文贡献在82对象实际重编证据支持下可追冻结输入，仍不宣称共享数据独占。真实证据out/c47-driver-fix/module-byte-map-real-v3.log，当前仅首片，六片要求未完成。

已完成其余五目标10模型的gen/tbl/net→正式wire/decoder→包raw核对，各child10s、整个阶段55s内终态0；10个bytes_equal=1，模型索引与真实路由一致。为避免只留out临时材料，本轮把首片完整模块摘要及10模型身份表存research/c47-logic-distribution-20261108/，包含全部输入链SHA；这只是来源/体积证据，不是六片或语义验收。

module-byte-map正式注册实际PASS/8/0/transport0，证据out/c47-driver-fix/module-byte-map-final.registered.log；首次注册父写错marker MODULE_BYTE_CHECK，虽check8/0但门禁正确判FAIL并撤回两行，日志保留，未据此记通过。实际marker为MODULE_BYTE_MAP_CHECK，最终新调用重新执行而非复用缓存。首片贡献前五为asmtext223070B、cxmod119477B、source_frame92079B、host90856B、tapegate77950B；完整82模块与链身份见research/c47-logic-distribution-20261108/lnx-x86_64-module-bytes.tsv，其它五目标内容绑定表见同目录five-target-model-bindings.tsv。

〔C47 ARM编码事务实现登记〕新增arm-encoder-record-origin/check（名称全git历史查空），先只做已绑定lnx/arm64 model2静态ELF。依真实pass1完整行及OP.end输出事务建区间，pass0/REWIND OCUT严格隔离，lnum不假定重置；布局176B/EM_AARCH64核实际ELF。先独立fixture和真实最小target，再接全片；不是把x86 WR.nx改机器号。其它ARM格式与whole-o来源仍另需合同。

〔C47 ARM来源前置实证与whole-o捕获决定〕Linux ARM实际inventory=82/82、44roots+38closure、VERIFIED/COMPLETE；合并tape8122531B SHA4f565b8b…，prune根FALLBACK完整帧传递输出同SHA，未借x86材料替代。首次重编调用给相对out，child冻结cwd下正确拒cannot open output，终态2保留；使用绝对新目录重跑前32对象已实memcmp相等，不改工具放宽。另实比snapshot与原release-snap-3b41cc224f的src/cx/exec/include/seed共1890文件零差异。下一仅重放win/x86_64原whole-o诊断构造，保留真实绝对source路径，捕获原holdframe/资源，并与原bare片memcmp；各步<=50s，不修改产品或发布资产。RF_CAPTURE不自带stage全文，后续仍需实际阶段重放，不能以捕获目录存在认完成。

〔C47 x86其余格式编码证书登记〕已有encoder-record-origin/check保留Linux固定profile，下一增显式osx/x86_64与win/x86_64 profile，只接受各自已绑定delta与确切配置。共用LW/WRITE/WR.nx事务，分别核Mach-O1020B文本复制及二次整文件复制、PE1024B文本复制及同名copy的重定位用途；不得只替换header机器号。先独立格式fixture与产品真实最小链，完整whole-o来源仍另证。

〔C47两个Linux源码重编链实证〕arm64完整82对象六批0/16/32/48/64各16、80起2均真实exit0和BYTE_IDENTICAL；源seal/receipt/构造器批前后复核，原合并内容身份4f565b8b…不变。lower-front观察与433924实体消费也真实exit0，full-front.target现已生成，最终ARM编码镜像仍未闭合，不能提前记第二片贡献。slice-inventory/check泛化两个Linux目标，拒冲突/重复/非Linux，replay核inventory与receipt目标并实际传-b；seal-v1由receiptSHA绑定目标继续兼容。独立最小ARM和x64重编链各13/0；inventory原无参超55s，不删断言，拆--part1/2/3/4分别18/5/4/5，32全部实际通过，注册同样拆分，旧超时保留。

whole-o首轮捕获在编译前因默认UNISA_JOBS=8被资源严格合同拒绝（rc2，非超时）；原stderr保留。下一明确UNISA_JOBS=1并用已有PHASE A/B断点把单位准备与最终输出分开，各自50s；不提高时限、不把A成功当镜像成功。ARM编码窄profile以START及两pass OP.end取事务，省掉每字节/行重复LINE观察而保留全部指令记录；标签/元数据/空行无代码追加的源规则需消费检查核，1M事件硬限不变。

〔C47复制边观察的窄扩展决定〕Mach-O/PE纯COPY循环若逐字节观察，整片必超既有1M记录；新增watch配置可选第三列frame-edges，仅对明确登记状态在当前frame cursor=0或end时记录，默认两列观察不变。配置SHA绑定筛选策略，header仍STATE_OBSERVATION_ONLY，消费证书必须验证首/尾frame SHA/end及cursor/output变化，再以已内容绑定且固定COPY/ADV无分支改写的规则核完整复制；不以省掉动作审查换低记录数。profile未登记的状态不受此筛选。保留1M/64MiB硬限，扩展独立正负控后才用于整片。

正式inventory mapping批53515/95213并发中，target部分4/0通过，但mapping注册外55s终止，未记PASS（直接part4的5/0不冒充注册通过）。进一步按拒绝阶段拆原part4为unsafe/dupkey两项、claimed/missing两项、root外一项，各自保持原真实fixture和断言；旧part1/2/3不变，合计仍32，不加时限。

〔C47第二片区间与覆盖扩展决定〕ARM完整producer761808条观察、consumer379288条RECORD均真实exit0，最终1889006B ELF与原ARM候选entry.elf cmp0。当前已核GENERIC/PF/PL的模块账亦守恒，但代码UNKNOWN132612B（AF/AI等尚未纳入），不能用x86较高覆盖率替代。下一保留旧profile/schema，另增已审AF消费4、AI消费2的显式profile与每实体source字段；mapper按规则确切count逐一核nc及所有source0..3，跨模块只记SHARED一次，AI lookahead不计消费。独立正负控后再新观察ARM完整发射，禁止把原UNKNOWN直接按相邻模块填账。

〔C47 PE真实路径不能缩为static〕已装产品Win最小输入的host_dyn实际为1，现static消费者正确拒绝，不能换不含运行库的静态玩具当当前Win全片证据。新增明确forward profile将分别核PE forward头/导入布局与观察分支，文本仍需证明PE.text→PE.copy完整且逐q闭合；额外合成代码若存在单列，绝不贴给最近q。Mach-O最小1155 records/text4909/image33250已实际consumer通过，完整whole-o来源仍独立未闭合。

object-source-replay两目标的直接13/0仍有效，但注册并发三批时两份replay均在55s外限终止，不能把直接检查替代该两份注册PASS。下一保留默认13项，新增--part1/2/3，每份都实际准备源/对象/inventory/seal/重编，再分别覆盖目标错配、对象/构造器/新目录、源码/路径/范围三组；唯一断言集合不减，只重复必要准备以限制单批。不放宽55s。

frame-edges最初登记helper没有入口args参数，实际运行了默认19项，不能记新增policy通过；已把该batch args明确更正--frame-edges，正式新调用须核11项。新增独立edge11/0、原watch19/0、完整事件29/0均实际exit0；原edge夹具用非法'*'byte键的9项失败保留，换为真实97/98/99字节键后通过，不改生产拒绝逻辑。AF/AI新check7/0、mapper3/0与旧21/9/8亦均实际通过，注册以独立新批覆盖。

外格式最终独立检查Linux11/0、MachO5/0、PE static5/0、PE forward9/0均实际exit0；真实PE forward1479记录/text7892/image11776、imports[9216,10752)，consumer两次真实终态0。新profile配置SHA与首尾COPY帧内容绑定，两OTHER保持完整非text尾守恒桶，ID附已核imports区间。sim-edge和mapper-ARM独立check的marker与默认模式不同，首注册虽rc0仍按不匹配marker正确判FAIL；metadata改用SIM_STATE_WATCH_EDGES_CHECK与MODULE_BYTE_MAP_ARM_CHECK后重新执行，不以错门禁回执记通过。

〔C47两片模块最终本地账〕ARM新AF/AI完整观察774105记录、target与原front cmp0；消费372588 episode实际退出0，最后mapper亦退出0：1889006B=82模块上下文代码1793808B+未覆盖代码52860B+合成1324B+HEADER176B+DATA_OR_ALIGN40838B，SHARED0。新账research/c47-logic-distribution-20261108/lnx-arm64-module-bytes.tsv保留完整SHA链；旧未含AF/AI的132612B UNKNOWN账保留在out，不拿其覆盖新证据。Linux两片分别守恒且与本轮候选镜像cmp0；仍不是其它四片完成，亦不证明共享全局数据的排他所有权。

Win/x86_64正常whole-o原44roots路线实际重放成功，bare2663424B SHA76248e1fdcb152e260b9bb530179bb44295530ad448ee1c050475bdf720b2fec，与原片完整cmp0；原holdframe37029416B/82源帧/742B属性，SHA6aa39be72d1c0f8cf0c5183edc387e9df06b0a9d6cd5dd8b9c209c5e9a2e12b4。分段PHASE B确实在__ccw_0处lower拒绝，未冒充原片证据；存在固定临时forward源码名且多agent共享TMPDIR，后续捕获应先隔离TMPDIR再核，不先断言根因。旧RF_CAPTURE e3 companion的SHA字段空白不能作有效输入身份，后续用独立内容hash和最终image cmp补证，绝不把空身份填成PASS。

下一正常Win重放将TMPDIR设为本任务独占目录，保持原构造器/容器/44输入和正常whole-o路线，不用PHASE B、不启RF_CAPTURE；同时留存hf原帧及真实forward stub。须再次最终image cmp原片，才能把隔离捕获材料加入来源链；不会用不同输入产物冒充原片。

〔C47 whole-o关键分支已实测收窄〕正常重放的独占TMPDIR下image仍cmp原Win片0，且无forward stub文件；实际product stream units/e3/sidecar已全部退出0，USLFW1为hostslot0/ccw46。按dv_run_lower的cnt0真实分支可直接剥tape→prune/image，不需要第二e3；前述假设stub的计划被实证撤回。原PHASE A在cnt0也可能完成整片，不得只按环境名认“停在e3”。e3-first.tape13472921B/787224行，SHA cb9860332f975e901a6cbfb5da2f53db788e6bb421db502cbdb443561d61f583，82帧与742B属性保留。

模型4实际绑定的是parse2 --errors图（隐含locations），JSON SHA a985b1706b96659bd3951a44495e774d4eb3c969163845c7a63d9c225e6b1811、raw434447B SHA09ca74c5…，正式raw cmp=1；--locations-only的409740B不匹配图保留拒绝，不用它作来源。模型35 --locations已raw cmp=1。现source-package配方的shared-e3 --locations与本包事实不符，只记录缺口，不顺手修改构造链。

工具预算实测574文件82081行，对新合入cc基线81824多257；没有改基线，已异步提交具体本刀扩额问题。工具功能/来源验证继续，不把该棘轮红表述成全仓绿；另误选precommit自身夹具--loc整体55s超时保留，真正当前计量是tests/toolsloc.cx --check明确exit1。

〔C47 ARM其它格式证书决定〕保留现Linux ARM两pass/OP.end合同和默认11断言，新增显式osx/arm64与win/arm64-forward profile，各自绑定model16/24及新配置SHA；格式布局及完整复制边沿用已审x86格式事实，但机器码事务仍按ARM两pass验证。先独立格式正负控与真实最小target，完整whole-o分账未闭合前不记其它片完成。

〔C47 whole-o阶段链实证〕父真实product stream lower与elf均已终态exit0，win-stage-replayed.exe为2663424B，与正常whole-o原bare全字节cmp0；因此已捕获的82帧→units→model4首e3→cnt0 strip→prune→lower→elf链确为该原片材料。尚无逐函数来源证书，不能把镜像相等替代来源。下一对osx两架构及winARM各用同一原44绝对输入、冻结旧构造器/容器、独占TMPDIR正常whole-o捕获，各外限50秒，分别与自己的原片cmp；三路不共享输出或状态。

〔C47四片whole-o原材料闭合〕osx/x86_64、osx/arm64、win/arm64三份正常whole-o捕获会话62586/66780/24640均真实exit0，各自image与原候选slice cmp0；字节/镜像SHA分别2410978/42ce67ed…、3071458/e5f80fac…、3369472/fcfcc413…；原holdframe SHA分别6b80eb1a…、43ecf905…、1e9598aa…。首次WinARM cmp路径误指whole下不存在文件，保留拒绝，随后对正确win-arm64/slice-win-arm64实际cmp0；不把不存在路径当通过。所有材料为相同44原绝对输入及冻结构造器，四目标源帧各自捕获不互代。

〔C47 whole观察性能未闭合〕minimal真实e3输入198B上默认与-O2 sim均exit0，623B sidecar及940B完整12记录观察账逐字节相等；据此才单次完整-O2重试，仍在同50s退出142、无COMPLETE/sidecar0B，不记全片PASS。下一只做10秒私有诊断核新frame重复哈希是否热点，不凭猜测改性能或再完整重试；共享sim输入已移交写窗口。

〔C47 holdframe材料提取工具决定〕四目标whole-o已捕获，但原一次性Python提取有明确债，不继续复制算法。新增可复用.cx工具严格解析holdframe LE32 frn/atn、82源帧记录与原属性，验证路径/长度/重复单元/完整尾后输出USLATTRT units载体、attrs及逐帧SHA身份；仅准备材料，不代表功能或来源已证。配独立check并登记，原Python材料保留且新工具输出须cmp0。

〔C47 ARM外格式实际门禁〕新MachO ARM独立4/0、PE ARM forward5/0，原Linux11/0保留；真实最小producer/consumer均exit0（1349records/text6524/image33250及1765records/text9820/image13824）。两个新注册批最终均PASS/rc0/transport0/ARM_FOREIGN_CHECK，绑定现候选f069d4c9…。register无args，初次默认11只算注册准备，父改精确args/marker后正式新跑；另首次手动run-batch漏ROOT/OUT参数正确usage拒绝，已保留原日志，正确完整参数调用才记PASS。能力速查：arm-encoder-record-origin --profile osx/arm64|win/arm64-forward TARGET CONFIG WATCH IMAGE；配置SHA d64f933c…/edc2f841…。四目标whole-o材料身份已固化research/c47-logic-distribution-20261108/whole-o-material-identities.tsv，但函数来源/四片最终模块表仍未完成。

〔C47 holdframe原生材料能力已实跑〕新增whole-stage-material/check默认14独立断言，显式--real额外Win原Python载体cmp为15/0；实际工具Win提取82frames/82attrs且carrier/frames/attrs均cmp原0，正式注册批PASS/rc0/transport0。父对其余三片各自原holdframe调用也均exit0，全部只记PREPARED。能力速查：`sh ./unisacc.com tools/whole-stage-material.cx -- PHASE NEW_OUTDIR`，全校验后独占发布frames.bin/attrs.bin/units-input.carrier/material.tsv，ledger有完整SHA与原序路径，不覆盖旧目录。默认check不隐读out；旧Python提取债在这条新材料入口已替代，但不冒称全构造链Python清零。

〔C47函数来源下一窄合同〕采用已审输出快照方案作独立诊断原型：CT.afterfn output-mark保存同generation起点，FN.nx output-span对当时真实[A,B)哈希；最终bare tape同offset逐快照核，后改写者UNKNOWN、区间外UNKNOWN，不搜索平移，不称最后写者或排他所有权。单active mark、嵌套/无mark/未闭合/换代拒绝；配置SHA绑定新policy且新record显式区分旧WATCHC。先私有sim副本+独立check，HOSTCC在途观察继续冻结共享sim；原型通过后再明确集成。工具预算最终577文件82310行，比基线81824多486；先前异步请求441为快照前实测，新增快照运行时18行及跟踪后的新check27行合计再增45。当前toolsloc实际FAIL，尚无本刀扩额答复，不改基线。

〔C47目标分支继续收窄〕其余三片units/e3均真实exit0。Mach两架构sidecar各明确rc1报告5 hostslot，不能套Win cnt0路线；正常捕获的独占TMPDIR均留有2534B forwarding stub，后续需核真实增量merge来源。WinARM sidecar/prune/lower/elf均exit0，最终3369472B SHAfcfcc413…与原候选slice cmp0；这闭合第二条whole-o kernel材料链，不替代函数上下文账。共享sim尚f5810a66，新增snapshot只在私有原型验证中，尚未记生产能力。

〔C47输出快照诊断已集成〕私有原型首夹具参数错误rc1保留，最终14/0后父按原sim f5810a66与私有9c90c332双SHA断言、精确diff集成。共享源实跑旧watch19/0、完整事件29/0、frame-edges11/0均exit0，新snapshot正式注册批14/0/PASS/rc0/transport0；原1M/64MiB上限不变。第三列output-mark/output-span须各恰一状态，单active，记录OUTPUT_MARK/OUTPUT_SPAN与同generation/full-line[A,B)SHA；嵌套/无mark/未闭合/换代/换帧/边界/预算拒绝，后改写快照不等不冒充最终来源。能力速查：sim --watch-states CONFIG --watch-compact --write-events NEW；snapshot配置SHA与header snapshot_policy=single-active-output-v1显式绑定。当前只是观察器能力，还没全片producer/consumer，不报四片模块完成。

〔C47最后短采样诊断〕HOSTCC完整仍50s超时；一次15秒轻采样实际rc142，JSON解析281ms、712ms开始执行、约2.94秒完成UC前扫描、唯一39MB framehash141ms，至33M steps未再hash。3.17至14.905秒主要在RET/NX/IX/AU/DL.tokenunit/DL.offset路径且cursor持续前进、out0；排除了本样本JSON卡住/重复framehash说法，未泛化全程，不再无变更重试完整观察。另unit上下文前置报告已纠正：UC.finish可达SWAP，CT的hl_pos不能直接索引原units.payload；须取得实际normalized immutable frame并核其map/prefix才归属，否则UNKNOWN。

〔C47本轮材料里程碑及下一合同〕最后实质提交28249b986已pull --rebase/push，ls-remote与HEAD完全相等；工作树当时干净，本段仅补本轮最终事实。Mach两个sidecar实解5slots/ccw50，names为dlopen/dlsym/dlclose/dlerror/utimes；正常独占新TMPDIR的2534B stub仅可由dv_fwd_merge fast0回退写出，具体fast0拒绝原因仍OPEN。最小后继是只补stub e2/e1，按原生产splice插frame0之后并重标attrs成83帧，再正式units/e3 --res ccw且不含run-forward，捕获最终tape与原image对拍；无需重跑原82源前端。现尚无直接phase+stub splice CLI，不以设计当完成。六片机器码模块目标仍仅Linux两片闭合；本轮四片whole-o材料、两Win阶段镜像、ARM外格式最小证书、strict材料提取及snapshot观察能力均有真实局部证据，完整来源/跨平台CI不外推。
〔C47本轮决定与分工〕上轮有实质代码/证据进展，非阻塞等待。当前四片缺最终来源：一刀扩whole-stage-material严格83帧stub splice，复用原82帧；一刀给snapshot可选导出实际immutable frame，解决UC SWAP后hl_pos的来源边界；完整VM观察的5次不同诊断仍未过50s，符合两次自查未解/设计决策条件，另交自包含抽象简报做只读性能设计评审，禁止加时/减观察。三刀分域，父持有prd/注册/集成；不改产品模型或资产，原事件/属性/拒绝合同保留。
〔C47有界VM设计收窄〕只读强模型设计评审未跑实验，推荐诊断侧复用现fast integer核心，次选分块原生动作专门化。父实读可在私有run_arm64.S的Lrloop以x19=machine/w24=q/x23=steps作mask订阅hook，不改生产ASM；先out-only普通HOSTCC原型核模型解码/最小字节对拍/状态计数，再完整输入50秒一次，尚不视为命名全图映射已证。关键预算身份：生产ex_budget_now默认400000000000 steps，旧sim默认200000000；新引擎明确按生产预算执行并记录实际steps，绝不把旧模式默改后报绿。另从私有seed-tbl副本提取完整state/register名字，原表必须cmp已绑定tbl0，修正dl 1based reg和整数核0based reg的映射风险。所有构造仅PREPARED、HOSTCC依赖明确。
〔C47函数上下文消费决定〕下一可复用consumer将核material ledger/真实units地图→ACCEPT观察与导出的实际normalized frame→CT.afterfn入口unit_epoch/hl_pos前缀→mark/span快照→最终同offset tape SHA；仅同内容稳定上下文可归模块，改写/区间外UNKNOWN，stub独立SYNTHETIC，不凭标签猜归属。因UNITOK2经过UC SWAP，original与normalized两份地图/前缀须分别核并显式连接，禁止直接拿原offset。输出非连续CONTEXT与独立UNIT定义，后续mapper另扩显式格式，旧Linux连续UNIT协议不改。先独立fixture，真实全片另实核。
〔C47快核来源观察实际突破〕私有counter完整e3在生产4e11步预算下ACCEPT，382608138步/4.63秒，输出13472952B与生产sidecar cmp0。完整稀疏snapshot+frame observer随后真实9.97秒/exit0，11121事件、2780 mark/span对、1实际frame；output仍cmp0，导出frame与完整39268516B units.payload也cmp0，故本输入UC SWAP后内容相等由实际字节证实而非假设。minimal事件/manifest/frame与当前sim逐字节核等，动作wrapper保留SWAP/SBCLR/INPUSH/POP代次。数据仍out私有原型，Win4资源身份单列尚未header绑定；不是来源消费已绿。实际consumer接入rc2揭示新consumer把USLFW1 magic误写为NUL结尾，生产实际LF；由其owner修精确合同并补sidecar正负控，原拒绝保留。
〔C47 Mach最终镜像反证〕两片83帧splice、units/e3/prune/lower/elf均真实exit0，但stage-replayed.image对原slice均char217不同，未闭合最终identity。保留新材料作为失败对照，不把83帧守恒外推到原像；正在核生产fallback资源/阶段差异，不盲重跑whole。
〔C47函数消费与Mach窄修复〕真实whole-function消费已exit0：82单元、2780上下文全部survived=1，裸tape与原e3-first.tape cmp0，尾[13015177,13472921)保留UNKNOWN；sidecar LF magic正负控4/0、旧夹具7/0。Mach差异已收窄到stub独立stream漏生产RT_EXTERN：14项私有状态按实际D.align合计33320B，恰等两片BSS增量；补RT_EXTERN后stub e2/e1均exit0且声明变extern。下一仅用新stub接83帧后续链核原镜像，未闭合前仍记FAIL。另将私有快核参数化并测lower/encoder真实观察量，决定有界事件账设计；原1M/64MiB模式上限不改。
〔C47 Mach新资源链实证〕补RT_EXTERN后两Mach的splice/units/e3/prune/lower/elf链各步均exit0，最终image对各自原候选slice实际cmp0：x64 2410978B/42ce67ed…，ARM 3071458B/e5f80fac…。新产物分目录保留原失败对照，未重跑82源front。此证明阶段材料链身份，不替代未完成的函数与机器码模块来源证书。
〔C47显式双段账决定〕Win lower实际1598104选态、encoder选态3821476（含PE.copy字节循环，尚非有效事件数），原1M预算不足。后继采用独立watch-bundle-v1，最多两段、每段1M/64MiB、全部文件128MiB；旧单账预算不变，不自动迁移、不删观测。先共享严格容器reader与独立check；为了保持现有业务校验循环与不引入跨段游标错误，新模式首版在总128MiB硬界内重建规范compact视图，单次分配与峰值须实测，不能冒称流式常量内存。新证书watch_sha明确为manifest SHA；运行引擎/步预算/帧策略/资源身份保留独立execution.tsv并由manifest核SHA，不伪装旧日志。原frame/sequence/episode/两遍encoder业务状态一律贯穿双段。
〔C47双段真实规模与调度〕Win完整lower1598106事件/83.37MB、encoder1206286/66.15MB，两producer均exit0且输出cmp0；encoder原子发布窄修后独立复验2.91s、RSS470466560B，不称恒定内存。新reader默认14/0且两完整bundle均核SHA/序号/全尾exit0。12路consumer夹具与另4路lower夹具并发后，x64旧11项与ARM新5项在50s超时，均不记绿；其它10路已exit0。下一仅两失败作业改为受控少并发重核，不加时、不删断言，检查输入保持同SHA。
〔C47带参批次登记窄修〕现register.cx只登记无参数检查，迫使每个新带参模式先重复不相干默认检查再改行。为本刀双段/旧覆盖分批登记，补可选末尾--args（按现注册表分号编码）直接写入并实跑正确模式；原无参入口和事务回滚保持。独立私有表核两参数实传、空值/控制字符拒绝及表不变，不改原锁/并发负控。两失败夹具改少并发后分别旧11/0、新5/0终态exit0，原超时记录保留。
〔C47四片e3上下文闭合〕其余WinARM/Machx64/MachARM真实root观察及whole-function消费均exit0，前者82单元2780上下文、Mach各83单元2597上下文（含stub）；全部快照survived=1，e3输出各cmp对应产品0。资源capture残缺回执保留，实际文件与pack/生产st_pub覆盖逐项核后才观察；命名图未选部分仍不声称独立同构/语义oracle。Win x64完整ROW消费已781127实体/exit0、encoder消费603136记录/exit0；其它三片lower/encoder观察与完整证书仍DOING。
〔C47 Mach encoder实值与合同分叉〕两Mach完整观察的WRITE/LIB.imagebegin实际host_dyn=1，原最小静态profile限定0所以消费明确rc2，拒绝有效保留。新增独立osx/x86_64-forward与osx/arm64-forward profile，仅该显式模式要求host_dyn=1；原osx静态profile仍要求0，原PE forward元数据校验不泛用到Mach。模型/配置SHA与完整复制事务合同不变，另加静态/动态错配负控，不扩大旧profile接受域。Win x64 bare模块账已守恒2663424B；实际wrapper正文delta1024、头增长1024和版本资源512核后，outer PE模块账守恒2664960B，82模块及代码未知项逐行保持，未把整个APE尾算到该slice。

---

## 附：自 prd.md §2 能力速查迁出的进度/状态行（0.0.48 期间各窗口追加）

以下 27 行是 **任务进度记录**（非能力速查），原夹在 §2 表格第 89–115 行，逐字保留。表头为「能力 | 命令/用法 | …」。

| 0.0.48底子执行（cc续派） | BASE-1..6与C0.2登记于items；父集成BASE-1/BASE-6，独立子任务C0.2摘要、BASE-2耗时、BASE-5函数矩阵；BASE-4复用I2原型 | 先落实依赖/摘要及单网实测，再做真实pp扰动增量验证；仅冻结输入运行，不改产品输入。逐项提交同步并在计划§14填实际数字，全部完成后接X2；超过5分钟先分解，已有60秒子步骤看门狗不放宽 |
| 单网闭包内容摘要（C0.2） | `sh ./unisacc.com tools/net-digest.cx -- --root DIR [--closure TSV] --net NAME\|--all [--export-closure NEW]`；正式net-digest批15/0 | 三真实pp配方端到端；长度分帧绑定recipe参数、排序canonical路径/kind/内容与工具身份；闭包内变/外不变。缺输入、符号链接、权限、坏图、重复、逃逸、未知网均拒绝且不出摘要。旧tree_digest未找到定义，本次复用现SHA库实落地；显式闭包只适用于已冻结图，不证明运行读集完备或构造/全键验证通过 |
| 逐网构造耗时（BASE-2） | `tools/net-build-profile.cx -- --root ROOT --gen GEN --tbl TBL --net NET --out NEW --start N --count K`；`--summary PROFILE...` | 官方42配方三阶段实际构造累计26559ms，p50=483/p90=807/p95=2466ms，最长warnparse3138ms；7批全rc0，原始命令/输入/产物SHA见research/c48-base-build-profile/。检查9/0含重复配方/混身份拒绝；现将阶段子时限钳入45秒剩余预算，实测仍按原调用保存。不含工具编译/输入哈希等未分相位开销，不冒称全键验证 |
| 0.0.49共享依赖前置 | plans/dep-split.md及tools/dep-split-probe.cx；facts30/42=71.43%、generator42/42=100% | 首刀只在0794f90f5隔离archive迁AUT/AUD真实消费，新view touch3/42；shared-e3同flags JSON/TBL/NET完整memcmp0，独立check9/0，全图仍30。两高共享表按真实内部所有权拆分的条件预测max16/42，TK ABI独立性未证且公共生成器仍42，故未达全局门、不启用生产复用；冻结产品域无改动 |
| X3原生不动点与无Python预检 DONE | research/c48-native-selfhost/；G1→G2→G3由Mach-O arm64实际直接执行；chain-precheck正式批74/0 | 同44根/固定外部包，G2/G3各3071458B/e5f80fac…且memcmp0，30.269952/30.419658s；不是NN/宿主工具/六片APE全链N22。PATH仅git的G1 include/entry/use三probe实际0（生成Linux交叉输出，不称Darwin运行ELF）。ctor/job严格核宿主格式/CPU；树快照改mem.heap同族配对后完整74通过。单次strdup/free未复现，相关疑点和host-C夹具/系统库/外部NN/Python历史构造残留均保留，不称依赖清零 |
| X4分发实测与缓存核心DONE | 公开v0.0.47 carrier fde2db18…；本机26.5.1，载荷声明minos13.0 | gh真实下载0，curl/Safari直连GitHub504；同字节本地转存经Safari天然quarantine0083，首跑sh --version0/hello7且xattr原值保留。spctl载体/镜像均3拒绝，ad-hoc结构有效不等于Gatekeeper信任；13.0未实跑。默认正规入口真实六槽私有APE初版c959543e…，最终安全修正版a9c138ec…（管理器c3d1eaf2…），ARM/Rosetta x64实跑0；正式缓存批16/0。fresh lsof迁移136→4体，132删除及4完整SHA独立核对，释放91968648B，活跃项保护；最近4+live leases，未来旧launcher无标记仍需重新adopt。正常失败回退已核，异常信号边界未测；research/c48-macos-distribution/，未安装或动Release资产 |
| M8行数门插队修正 | 79c09c401重新引入逐文件toolsloc，与4607c928d及主人本次裁定冲突 | 保留提交档子集选择、选择器/估时、材料路径门禁和phase-budget；仅撤所有LOC拦截，历史表只存档。新增tool的check+批次门仍有效，重复/无调用者仅报告；主树24个受影响正式批全部真实终态0、PASS/rc0/transport0；652批注册表metadata_valid=1。历史表只加RETIRED头，其余字节不变；不复用原M8整档资格来冒充本次新树全绿 |
| 根身份按主机分账 | 本机实物f069/5818178B，共享ape-receipt及root-identity记录另机3d0e/6425630B；旧全局checker真实不匹配 | ROOT-RECORD-HOST-SPLIT已登记；本机实际记录plans/root-identity-osx-local.md，保留他机两文件不改，待显式host/ROOT检查接口，不把历史安装记录当本机实物 |
| P0 OBJ-C-PRUNES-EXTERNAL后续实证 | 保留02a后追加的历史验证与候选限制，不以其恢复已撤对象路由方案；CLI-only统一unit-object默认的out候选待cc集成，尚无新候选或发布资格 | 六目标(source/tape/unit各O0/O1/O2)正式批均12/0，bare5/0、oracle28/0、同次实编生成器7/0与288路线及四负控均PASS；最终13闸13/13。修复提交02a7a0d2d，正规冻结源SID9023ad22…、ARM848e0404…与私有件字节同；固定包仍f06940模型，非e586新候选资格。h/o真实分开编译后都有导出，但链接旧G3与新驱动都被.end缺失拒绝；当前源事实已发.end，等新42包复验，绝不补假end或放宽消费者。集成清单见plans/c48-2-binding-status.md#obj-c-prunes-external源修复集成通道；Linux同候选全批PENDING-REMOTE。cc新裁定不等3d0e实物：用本机osx-chain分别冻结02a7a0d2d^与02a7a0d2d作同固定包源对照；裸-c与显式Linux -c分别实核，不预设旧40包能重现原42包的osx红。根记录与实物按主机另记ROOT-RECORD-HOST-SPLIT，保留他机原记录。osx-chain前后两新RUN真实各0，32.255286/32.703603秒，before9083a6cc…/after848e0404…；最小ext_f(x)+main0四格实际终态0：Linux ELF前丢后留，osx裸-c前后均留，不能冒称本机复现了原3d0e的osx红。 |
| OSX-SENTINEL新目标 | origin/main持续前进；同步进main的安装记录称T1内部候选3d0e237d…，但本机实际hash仍f069d4c9…/5818178B；远程安装记录不能代替本机字节，哨兵显式冻结实测bootstrap/models身份。现日常入口实际是tools/osx-lane.cx（原14条，ae1e06快照动态24条），osx-chain是构造链，不混用 | 交付cc触发的一条可恢复命令，不后台轮询：每次≤60秒推进一个阶段（批次保留55秒完整窗口），fetch→冻结远端完整HEAD→独立git archive/私有git元数据与显式bootstrap/models→新源ARM DUT构造→原日常动态osx清单逐批运行；只读共享产品，不改他人代码。绿仅在新HEAD真实全批完成后加一行ledger；红用同一冻结材料在祖先区间二分，先证明base绿/HEAD红，未知不当红绿，首坏项附最小失败批次命令/日志/车道写OSX-REGRESS-sha。实现与检查先私有副本，公开接口/状态/并发锁/重启证据须可核。队列--step/--verify-queue已集成，源ed33f38f…与native1fac17ce…冻结；原default14、scheduler30、init12、完整真实PASS/FAIL各12及恢复重核各1均实际0，旧13项合并末步142保留并拆12+1，未抬55秒预算。主树正式osx-lane-queue=12/0/PASS；父ae1e06隔离快照24已实际LOCAL_PASS，末端31及只读verify40724均exit0，receipt9cdbd592…、executed=selected=24；固定根/包f069、历史DUT848/SID9023，不迁当前main30或新42同候选资格；sentinel二分控制器及其检查已集成：actual-image桥接controller SHA、冻结lane SHA/seed/harness/registry/selection比较，绿静默幂等ledger，原始收据双重验证，同材料先证明base/head再实际rev-list二分与firstbad items。最终私有六组51/0（16+10+10+5+5+5），主树六个正式批全部PASS/rc0/transport0，同版51/0且13闸13/13；UNKNOWN与无base红绝不假firstbad，partial保留证据需新STATE，不假称自动恢复。周期命令见plans/osx-sentinel.md。远程a3a32d8a0已用统一unit-object默认取代02a路由方案，按远程政策保留；P0检查新增Linux unit ELF导出section入口，原raw ELF定义检查保留，旧9023/848对照仅历史，不当当前候选。 |
| hardened-runtime真实ARM缺口 | 真实G3复制件加ad-hoc/runtime但空entitlements后ARM version0、hello动态kernel执行被SIGKILL；IPS为CODESIGNING/Invalid Page，kn_load现RW填service后mprotect RX且无MAP_JIT。x64 Rosetta同流程实际0，不能迁ARM资格 | 仅在私有ad-hoc验证profile显式加入com.apple.security.cs.allow-unsigned-executable-memory，再实测ARM动态kernel/hello与拒绝路径；保留空权限失败，不加disable-library-validation或disable-page-protection。发布方案需记录这项能力的必要性与风险；不改产品机制，不真签/notary submit。单例外真实ARM48342/x647823均exit0，version0/hello7/FFI0，外层错ENV未替换固定包；空ARM -9反证保留，spctl仍3/裸stapler65。独立check51940实际18/0 |
| macOS签名公证接入目标 | src版本仍0.0.47，e586内部件不作0.0.48发布件；两裸片codesign结构0但spctl3、无ticket。stapler只支持dmg/app bundle/signed-flat-pkg，不能给裸MachO或APE/zip冒staple成功 | 首真签绑定未来版本串0.0.48重建的owner-reviewed GitHub Draft，人工workflow_dispatch+受保护CI环境；Developer ID和notarytool密钥仅CI secrets，临时keychain，签前后SHA/重包APE/新收据分账，DMG承载ticket。本轮仅私有真实Mac片ad-hoc与dry-run，核hardened runtime/JIT必要entitlements、全步骤及失败路径；不真签/notary submit/切资产或测试性push |
| osx链宏探针夹具缺口 | 第二RUN六段构造真实0，两ISA G2/G3完整byteequal；随后宏夹具只定义probe，无main，产品具名no-main拒绝，52107真实exit2 | 将链内完整程序夹具改为main；另四个新DUT单源/多源实际11+6/0均已证明S修复。保留第二RUN失败及全部六段原证据，以新控制器/新RUN重跑，不跨RUN迁PASS |
| osx构造链首次引导spawn缺口 | 新链15155真实exit2，slice-arm64首次在4.112秒内rc1/release-build:spawn；原始argv与失败attempt保留。现whole-o默认argv0=sh却给SY_SPAWN_NO_PATH，Darwin execv不查PATH | 修正既有release-build默认shell为绝对/bin/sh，与NO_PATH合同一致；不改变构造根数/预算或借旧native候选绕过首次引导。更新控制器身份后新RUN重验全链，原失败不覆盖 |
| osx固定外包构造链新实证 | 当前T1源e586+显式旧外包0d569并非同源NN，但实际ARM新44根slice39729=0/30.979367s，自举63407=0/30.833197s，3120994B/57176856…完整memcmp0；hello实际7 | 采用显式--fixed-package的一条有界osx链：两Mac slice与各G2/G3/hello/宏探针，每子≤60、相位≤5min，冻结REV与外包完整SHA并记逐阶段耗时/失败日志；签名/公证仅前置读取。仅固定外包裸片不动点，不冒完整N22/NN同源或发布资格；同源P3另门不阻本机osx施工，工具对独立双CPU MachO/证据篡改等17/0，正式osx-chain-interface批36456真实PASS；第二RUN真实六段均0（32.130/31.476/31.254/31.917/39.002/41.433秒）且两ISA G2/G3完整相同，但后续夹具no-main失败；四个独立新DUT检查ARM与Rosetta均11+6/0。第三RUN26690真实exit0，八阶段全部完成：六段34.197/33.022/35.051/32.482/38.204/36.910秒，两ISA G2/G3完整memcmp0，hello两片7、bare/explicit S字节同。codesign结构0，spctl3，stapler65（无ticket）为具名信任/公证前置缺口，未签写/提交；Developer ID只读存在。冻结eec5fcfa685…快照SID实算a058与主树同，外包及根载荷仍f069；所有旧失败保留 |
| osx日常一键入口合同 | 起始platform含osx为7批，实际191/0/f069，新增selfpath后8批206/0；共有objects_osx材料父目录下必须含arm64与x86_64，不可把单profile叶绑定给会追加子目录的消费者 | 新工具动态读取tp_visit已验注册表，smoke加全部osx条目；显式prepare/verify两profile材料并预编BOUND_CACHE，首版串行后实测预算不足，现按最多4路独立TMPDIR只读并发施工；全部材料摘要合在一个固定20秒子进程测量（前后各一次），终核保留55秒完整余量；各批子≤55秒、整体相位≤5分钟；非空检查，材料缺失/计划/未跑/失败具名且非零，禁止未知PASS。记录实际候选/源SID/registry/harness前后身份、逐批原日志和耗时；不调Linux全会话、不迁旧PASS。首版真实run82620终态3：11条选中、5条真实LOCAL_PASS后按完整子窗口余量主动PENDING，余6条未授绿；原逐批verdict完整保留。第二轮15530真实exit0：动态13个osx条目+smoke全14/14 LOCAL_PASS，candidate=f069、sourceSID=a058、harness/材料前后稳定；typedMAP显式新ARM DUT9083a6cc…与固定包f069，未把根APE宣称已含S修复。主树持续scheduler30/0，默认14检查亦正式批PASS；输入变更后只按本轮冻结身份记账。夹具修正后第三轮30641亦真实exit0，当前全14条再次LOCAL_PASS且前后身份稳定；14条无未跑/不可用，本机四项目标已完成，Linux仍PENDING-REMOTE |
| DEFAULT-TARGET-S-C修复政策 | f069实测裸-S宏指纹lnx/x64，显式两osx正确；-c已修，早期赋target会把bare.s误分类为native asm而拒绝 | 仅两osx宿主裸-S预定义随当前slice；四非osx保留原lnx/x64。将tape_default政策写事实表，经既有target-gen生成，在cxt_object分类之后且mode2/无显式target/无既有origin时选择；保留bare.s tape成功、显式osx.s拒绝与generic路由/Linux旧合同。隔离补丁已合主树：事实四列读取器同步，5个target生成输出仅hostid/check-cx-target变化，pp-gen仅输入SHA元信息；定向生成器、pp-lines40、原CLI/route均实际0，主树target/lines及显式MODELS版产品check实编也0。旧DUT单源10项2红保留；全facts仍有nativeabi既有DIFF及隔离缺引导产品未完成。源a058的两Mac新候选已各单源11/0、多源6/0（含bare.s/显式osx.s拒绝）；根f069未替换。第三RUN26690整链真实0，两ISA G2/G3完整同；源修复已实证，不迁NN同源/发布资格 |
| 主人改向：本机只保osx线顺畅 | lnx远程为主力；暂停c48-2非osx与DEP-SPLIT后续实施，既有设计/隔离工具交lnx主线 | 本机清点并实跑platform含osx所有批次，逐个修或具名KNOWN-GAP；修DEFAULT-TARGET-S-C裸-S预定义；两Mac slice构造/自举/签名公证前置一条有界链及耗时；日常smoke+osx全批一键入口；状态回写计划§15。Linux一律PENDING-REMOTE，不等待 |
| osx loader自身路径能力缺口 | 兼容批实跑发现test-loader与special硬读/proc/self/exe；现sys.fs的sy_self_path在Darwin也走该路径，导致identity读取失败；另UNIPP2检查与本机旧f069载荷身份不合 | 保持T1源SID e586与产品输入不变，先用现L0 sys.ffi/sys.dlcall能力在工具域验证宿主自身路径接口，严格保留OS实际进程映像身份，不用argv0假冒；工具侧现L0 FFI桥接已实跑15/0，伪argv0与symlink负控保留；原special21/0、新launcher身份读取0、显式ISA claimed1/failed0，源SID未改。旧4红及UNIPP2对旧载荷差异完整保留，L0缺口已登记pkg-gaps；Linux格PENDING-REMOTE而不等待 |
| 登记器失败回滚锁缺口 | host-cell首次登记因既有commit-materials元数据不合法被拒，register在持锁分支又取回滚锁，外bound60终态142且留下本次两行 | 修复必须只在回滚前释放本调用持有的锁，再在回滚锁保护下删除本次精确两行；独立私有表负控8/0及正式register-metadata-rollback真实PASS；原args完整14列预期修正后5/0，原默认35项本机55秒超时仍未完成。保留旧DIRTY反证与超时，不放宽锁时限 |
| c48-2 loader材料消费绑定 | 0936a488a已有loader/swap/seal-loader与path登记；但consumer仍读ENV，未核登记目录manifest，可能登记A而用ENV-B | 扩现有units-prepare verify-loader与既有loadernets共用核验，先接ISA一批：显式登记路径必须决定所消费材料并校验receipt/候选及实际成员SHA，错路径/错身份/ENV替换拒绝。独立17/0及正式显式ISA新收据/错ENV实际0；仅ISA迁移，余五类保留原开发ENV合同。osx bridge后common10/0、special21/0、nets46/0，test-loader24中仅UNIPP2对旧f069载荷1红仍保留；不把旧79证据迁成本tipPASS |
| T1合入后的pin声明位置漂移 | 当前source-package-pack实SHA为e67953bd…且sourcepackage:117字面完全相同；声明仍a0f391…，另digest/selfcheck源码移至98/100但表仍96/98；新loader负控零SHA也需声明 | 仅同步已核真实调用位置、当前工具字节和标准SHA向量/明确负控literal；不改工具源码、断言、known-broken或产品pin。原6红保留；仅确证字段精确修表后90845真实exit0、failed0/stale30/hostdep6，宿主依赖变化仍报，不据此授新候选资格 |
| c48-2 host-cell显式身份 | 旧CLI可缺SHA回读侧车，registry未声明候选摘要，缺侧车便拒绝 | 改为ROOT+64位小写候选SHA必填，先核实际产品digest，仍仅Linux/x64可正向PASS；其他宿主明确NOT_RUN。登记candidate-sha:sha256与显式args，带材料的正批按现行元数据合同放daily，独立接口check仍commit；默认-run及正式host-cell-binding均19/0，Linux/x64分支实际交叉编译0但未原生执行，Mac直接CLI实际NOT_RUN/exit3；保留首编foreign宏错误和无效登记原证据，不把Mac控例算Linux资格 |
| DEP-SPLIT与c48-2新目标 | facts30/42、generator42/42仍超60%；旧s9全批23 NOT-AVAIL、10 INFRA未代表本轮资格 | 先定位当前候选/会话与材料身份；DEP-SPLIT新增具体行/函数切口及预测，隔离副本试拆一处用net-deps复核，目标facts≤25/42。c48-2逐项施工至NOT-AVAIL≤8，INFRA实跑转绿或登记串行域，未知不记PASS；不触冻结产品输入，不把旧PASS迁给新候选。远程T1合入后，本机65994实算源ident=e586721c7767275e1b5af84fe5a3cd199600e2550fc665c26cec642c6f4c9325，原声明已对应当前源；APE仍f069开发载荷，Linux格按cc裁定登记PENDING-REMOTE（items C48-2-LINUX与s9列ROOT/SESSION/MAP清单），本机不做不等，继续osx原生与材料接口；不把f069绿迁给e586候选 |
| osx车道最终安全门DONE | X5已push3b126fd8f且ls-remote等HEAD；远程R1已收敛全仓静态红，显露本车道此前X3/X4剩余安全债 | cache-retain/check与check-chain-precheck补真格式化/复制边界，不扩基线；3份已执行研究源按R1模式原字节归档archive并核SHA/引用，避免把历史快照当活跃工具。新源码按影响重跑缓存16/链74和私有两Mac载体；最新precommit变更后25相关批真实复验。产品域冻结不触碰，旧证据仍绑定原源，不迁移。最终三源静态债清零、25相关批及链74/缓存16复验均绿；13门聚合12ok、coreapi55秒ERROR且独立55秒仍超时，定位为每26模块重复编译symindex。仅测试协调器复用本轮一次实编helper，查询域/夹具/时限不减不扩：正常26模块/194导出13.874s，原8夹具14.322s均真实0。最终13门汇总session59831实际0、13ok/0fail/54.080s；research/c48-final-safety/。非全658功能批或发布资格，既有Darwin fault5红仍单列 |
| X5 osx unit-container合同DONE | 当前osx producer只写MH_OBJECT单__UNISA,__unisa_tape元数据，ncmds=1，无LC_SYMTAB且nreloc=0 | 不把空字段称实际机器码符号/重定位覆盖；X5区分unit-container正合同与完整系统ABI对象尚未支持的具名边界。保留旧版本/混目标拒绝，Linux/Win路径不碰；正式两正+四负共6批均真实0/PASS/transport0，独立7畸变负控、两目标产品链接7、旧版本/混目标精确拒绝及恢复保留；x64仅Rosetta。初6红为oracle误写S_ATTR_DEBUG=2，真实字段及writer均0x02000000，精确修后重验，原红保留。runner新增hostid许可本机Darwin，独立正式17/0；原Linux标签保持。既有runner-faults本机5红仍保留，旧/新harness超时与信号对照一致，非本刀回归；不称全harness/全批或all-waves Darwin已绿。research/c48-osx-object-contract/。提交前静态新债已用checked mm_copy/cx_fmt消除，基线不改；新tool/tests身份与新两材料下六正式重跑仍174/0，runner正式17/0。当时三静态门全仓旧红及本刀四源无超基线的原结果保留out/c48-x5-final；后续R1与本车道安全收口已使当前13静态门全部实际绿，不迁移旧结果 |
| osx车道接续（X3/X4→X5） | X3原生arm64 gen2==gen3；去PATH Python实跑chain-precheck；X4 Release隔离首跑/最低macOS/缓存最近N/README四步；之后X5仅osx两格 | 本轮产品f069…与X2固定包不刷新；X3/X4及X5 unit-container合同已交真实证据，父集成；每轮均pull--rebase保留远程main。残留host-C/Python逐条记录，不宣称清零；Release只下载验，不改资产/签/Latest。0.0.49首候选新增共享facts/生成器依赖拆分前置：实测30/42和42/42超60%，只读方案先行，C0.3复用后置 |

## 迁入：0.0.47 逻辑分布普查结论（原 prd.md §9.6，2026-11-08，逐字保留）


产品身份仍为 `f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82`，5,818,178 B、0.0.47、NOT_RELEASE。driver 四个正式批次20/0，六目标最小镜像格式、裸-c、链接并执行23、默认-run23均有实际证据；本次统计工具变化未重建或刷新产品。公开版本同名不迁移CI资格。

源码测量冻结于 `ce710d583`（合入远程 net-deps 后；随后 `99e24cd8a` 仅 PRD 变化）。`logic-distribution --sources` 重新实跑：2320个已跟踪普通源码，根为src/cx/exec/seed/weights/tools，总14,096,270 B。表5,771,926 B（40.9464%）、生成器源码1,450,565 B（10.2904%）、经典机制源码6,873,779 B（48.7631%）；该口径的已跟踪网络表示源文件为0，不等于产品没有网络。工具也在此分母，不能把源码字节或branch token冒充语义决策占比。

当前产品包另实跑40个kind N模型，raw总4,833,789 B：生成动作程序2,886,663 B（59.7184%）、选择器1,757,294 B（36.3544%）、常量189,035 B（3.9107%）、头797 B；精确守恒。压缩模型1,315,916 B、资源241,070 B、元数据108,280 B合为包1,665,266 B，未按raw比例虚摊压缩段。源码/模型/压缩包是三个分母，不相加；生成器源码也不等于生成动作程序。

六片逐模块明细和SHA链见[普查结果](research/c47-logic-distribution-20261108/summary.md)。下表为字节；MODULE是发射上下文来源，非排他数据所有权；各片SHARED均为0，只代表本次没有核到多源融合区间。

| 目标 | 解压映像 | MODULE | UNKNOWN代码 | SYNTHETIC | HEADER | DATA/ALIGN | RESOURCE |
|---|---:|---:|---:|---:|---:|---:|---:|
| lnx/x86_64 | 1880862 | 1803567 | 36795 | 411 | 176 | 39913 | 0 |
| lnx/arm64 | 1889006 | 1793808 | 52860 | 1324 | 176 | 40838 | 0 |
| osx/x86_64 | 2410978 | 2177228 | 148248 | 6941 | 1020 | 77541 | 0 |
| osx/arm64 | 3071458 | 2811856 | 169396 | 10160 | 1020 | 79026 | 0 |
| win/x86_64 外PE | 2664960 | 2379225 | 235958 | 0 | 2048 | 47217 | 512 |
| win/arm64 | 3369472 | 3074840 | 246224 | 0 | 1024 | 47384 | 0 |

六个mapper均真实exit0；独立默认-run探针逐行重算、核原映像SHA也exit0。Win x64先核bare 2663424 B，再验证正文整体平移1024 B、HEADER增加1024 B及版本资源512 B，得到外PE 2664960 B；整个APE余尾不算该切片。Mach83单元中的forward stub单列SYNTHETIC。后改写、函数区间外和未覆盖lower规则保留UNKNOWN，不把守恒等同100%归因。

D1数据结论：模型大半raw字节是生成动作程序，选择器只是其中一部分；这些表示计数没有证明“以网消除逻辑”。机制代码、跨目标合同与独立于参考C的语义oracle仍须分别验证；本次逐字节对拍和来源一致性不是独立语义oracle。whole观察的快核目前依赖out私有HOSTCC/ASM诊断原型及实际execution身份，不声称已迁入共享sim或形成可移植产品能力。

全部新增工具及最终参数模式均已正式登记实跑，包含8个encoder双段profile；静态和动态错配拒绝保留。C47最终测量585文件83,279行；2026-11-08主人最终裁定工具行数门退役，C47据已有功能证据DONE。先前13项实测9绿、cxapis/cxtrunc/outdep红及coreapi ERROR仍保留，统计制度改变不使其余门变绿。局部功能和登记通过不称全仓棘轮/--all/六宿主CI通过；首次超时、静态profile拒绝与未知状态均保留。

## 0.0.49 收尾（2026-11-10 自 prd.md §10 末尾逐字迁出；0.0.48.1 macOS 车道记录与 0.0.49 发布事实）

迁出说明：以下 OSX-LAUNCH-SLOW / OSX-SIGN-ROSETTA 段落原在 prd.md「§10 子文档与归档索引」之后（提交 bb3e43d218 时第 476–525 行），顺序未改、内容逐字；箭头符号按 AGENTS.md 规则改写为 `->`，其余未动。0.0.49 发布事实本身（APE 2d3852a8、冻结源 979b56e4、CI 42661106290 QUALIFIED 42/0、s9 提交档 96/96、提交 X e3fc485793 / Y 35cfdd1e3a）见 [plans/release-notes-0.0.49.md](plans/release-notes-0.0.49.md) 与 `agentwork/ledger.tsv` 2026-11-10 `RELEASE-0.0.49 PUBLISH_DECIDED` 行；教训见 [prd_release.md](prd_release.md) §3b。

### OSX-LAUNCH-SLOW（0.0.48 发布回归 P0）

cc报告发布件559705热启动每次创建并执行新manager inode，Darwin安全评估使--version/-run约1秒。本机实际根仍f069，需下载并核559705，不以安装记录代字节。首版manager内容缓存真实未过性能：隔离缓存仍高CPU重算MB摘要；原失败日志保留。改用新解压/marker异常才运行manager，热路径匹配内置完整slice SHA的原marker行、原子新增PID lease并touch recency，不启动manager；冷路径仍完整摘要校验/最近4清理/隔离fallback。manager保存在私有.managers内容哈希目录，冷构造原子发布。验收两Mac架构各热version/run r7逐次10次与中位数≤50/300ms，首解压仍清理旧缓存；新候选按发布流程绑定独立身份，未实测不记PASS。

OSX-LAUNCH-SLOW能力入口：`tools/osx-launch-profile.cx APE arm64|x86_64`显式候选逐次测量与10次中位；配对check独立15项。`tools/check-ape-pack.cx APE ARCH`验证冷清理/热manager不执行/lease/recency及symlink隔离；c869历史候选两格各45/0。五个osx批次已登记，真实候选性能不借工具夹具PASS；ARM15/224ms已过，Rosetta70/423ms未过；裸x64 run388.5ms、旧f069同入口405.5ms，保留300ms门继续核验。

OSX-LAUNCH-SLOW最终wrapper开发冻结07a90387…、候选01b16ae0…：热合一uname并仅缺目录才mkdir，ARM12.5/253.5ms、Rosetta48/512.5ms；后者run仍FAIL，旧f069 Rosetta同测405.5ms，裸c48 slice388.5ms。阈值不放宽，未发布或授QUALIFIED。测量check15/0及13棘轮13/13；工具库存由现git index完整重算，核补已有cache-retain/check、toolselfpath、precommit-freeze遗漏并剔除误列tests路径，现651源/82分片，明确只登记未全编。下一步查r7具体输入/运行相位并推进新候选CI。

OSX-LAUNCH-SLOW宿主分账：Rosetta两批是在ARM宿主强制arch x86_64的跨架构探针，platform须标osx/arm64；此前误标osx/x86_64，被run-batch既有宿主门拒绝，原4份REFUSE日志保留，未改宿主门或把拒绝当功能失败。Terminal.app责任应用已有Developer Tools豁免，不改系统设置；同01b16ae0在该上下文x64独立10次中位48/246ms过门，非豁免Codex上下文run512.5ms失败单列，根因另有既有-run临时程序新inode；源码runcache也只启用lnx/x64。正式ARM批在Terminal已15/101.5ms PASS，配对接口15/0、cache ARM45/0；修正platform后再跑Rosetta正式批，不迁原独立结果。

OSX-LAUNCH-SLOW完成审计补漏：01b16候选CI42551819720已全六目标成功，但热shell每次创建lease而仅cold清理，会在长期热调用后积累死lease触4096扫描预算，故不是最终交付。修tools/cache-retain新增轻量--touch全SHA模式，复用同inode但不digest MB载荷，严格marker/root/lock/parentPID并清本tool全部dead lease、保留active/EPERM、更新recency和新lease；full mode仍cold做完整摘要/四载荷清理。健康新缓存不积累；既有超预算目录不宣称自动恢复，保留隔离fallback。双manager将新构造，必须新候选SHA及新CI，不迁01b16绿。

OSX-LAUNCH-SLOW cc协同新裁定：最终目标改为0.0.48.1可发布候选；本车道负责version事实/配套站点、热lease维护、固定四Mac成员与重包接口，cc独占.github/workflows/macos-sign.yml、tools/macos-signing.cx、plans/macos-signing.md，本车道不改。草稿附members.tsv五角色及四Mac独立资产，最终tag/candidateSHA/member清单SHA写release/candidate-osx-launch.tsv并交cc触真签。版本必须实际新源构造，不把0.0.48 wrapper候选冒称0.0.48.1；签后四成员显式替换重包保留字节，新签后身份重新资格，不沿用未签CI。旧01b16已六目标CI成功但lease缺口/版本不满足最终裁定，仍仅历史对照。

OSX-LAUNCH-SLOW签名交接防误用：旧01b16/0.0.48六目标42/0候选表移到release/candidate-osx-launch-rc1.tsv，当前release/candidate-osx-launch.tsv仅在新0.0.48.1完整六片/四固定Mac成员/members.tsv实际就绪后重建并通知cc，避免把旧版本或lease缺口候选误绑首真签。暂缺当前表不是新候选身份，不写零SHA冒名。

OSX-LAUNCH-SLOW新源Linux构造入口缺口：两目标首批尚未构造对象即被entry-build覆盖UNISA_CONTAINER拒绝；显式constructor仅认bare ELF，Mac裸片被错误unset包。正规entry接口已声明显式constructor直接执行，现补Mach-O64识别保留同显式P4，APE/薄头仍清外部包；不换产品源/NN/SID，不以shell包装掩盖构造器身份。失败OUT保留，新native工具与新冻结源根另开批次，最终完整44根/closure/link逐项核。

OSX-LAUNCH-SLOW最终53ff候选首测：ARM性能过门，两架构缓存各108/0且四Mac成员/P4严格字节同；Rosetta version54.5ms FAIL、run262.5ms PASS，材料仅PREPARED不授可发布。热wrapper仍外部uname+chmod，下一小步用bash只读BASH_VERSINFO[5]识别Darwin架构(实测ARM/Rosetta及覆写HOSTTYPE/OSTYPE/MACHTYPE不影响)，不支持的POSIX shell/未知平台回退原uname，不信可覆盖环境变量。保留chmod私有目录与全部cache安全合同；新wrapper必出新SHA再独立性能/功能/CI，不迁53ff结果。

OSX-LAUNCH-SLOW新候选bdd52246c4ba52becff3c1404b20976a51f955e8c3e2218443123d1b464adaa3/6429038B：实际两架构逐次10次性能ARM14/101.5ms、Rosetta43/257.5ms通过50/300ms门，Terminal.app既有责任应用上下文；53ff失败完整保留，不改安全设置。release-ape新接口28/0、members独立21/0与最终13棘轮13/13。将用独立草稿v0.0.48.1-osx-launch-rc1上传七标准资产+四Mac成员+五行无header members.tsv，交cc触真签；签后重包新SHA须重验，不迁未签资格。对外动作回滚为删除本草稿和本tag(gh release delete v0.0.48.1-osx-launch-rc1 --cleanup-tag)，不覆盖已发v0.0.48/Latest/根。本车道不改cc三签名路径。

OSX-LAUNCH-SLOW最终正式五批补验：冻结f09969d9d0efb81976b3eaf486731f120df22cbe的独立ROOT，精确bdd52246副本，Terminal.app既有上下文。interface15/0、两cache各108/0、ARM热version/run12/99ms、Rosetta42/246.5ms，各mode10次中位，五个BATCH_RUN均PASS/rc0/transport0，candidate完整SHA一致。正式入口未拒绝/未迁旧数据，平台同osx/arm64含Rosetta，非原生Intel资格由同候选CI补；详out/osx-launch-slow/v0481/formal-*.log。

OSX-LAUNCH-SLOW交接身份已实际固定：草稿v0.0.48.1-osx-launch-rc1、source_commit=f09969d9d0efb81976b3eaf486731f120df22cbe、APE=bdd52246c4ba52becff3c1404b20976a51f955e8c3e2218443123d1b464adaa3；七标准资产+四Mac+members五角色共12资产上传全actual0。members SHA4e1a51b8bc0d2dbdf8aefad6bca25cf615556fb572044abc3b89e008565225d1，正式清单/表均在release/candidate-osx-launch*.tsv。新CI42555350465实派发且fetch/ffi成功，正在原生格前工具准备；完成前不授42/0。cc按tag/source_commit/membersSHA绑定首真签，四成员签后重包须新CI；不改cc签名路径。

OSX-LAUNCH-SLOW草稿回读完成：12资产独立重新下载，严格layout与members check真实exit0；metadataAPI核draft=true/tag/targetf099/12唯一，独立CX审计逐文件完整SHA/bytes与远端digest全部一致，members仍4e1a51b8…，保留初始EOF/notfound日志。六个native格现均success，汇总report尚在途；不提前授42/0。证据out/osx-launch-slow/v0481/draft-readback-{report.md,audit.tsv}。

OSX-LAUNCH-SLOW未签候选最终资格：CI42555350465实际completed/success，下载新artifact表绑定bdd52246全SHA+f099精确提交，六目标七检查=42 PASS/0FAIL/0NOTRUN/0INVALID；Windows portable子集范围以原notes为准，不称完整--all或全POSIX。本车道目标已到可真签交接：0.0.48.1草稿12资产和members4e1a51b8…经远端回读完全一致，正式candidate表固定CIID，四成员签后替换入口28/0。首真签归cc，未触发真签/公证/Latest/根替换，签后新身份须重新资格。热性能仅Terminal既有责任应用上下文通过，非豁免旧失败保留。证据out/osx-launch-slow/v0481/ci-evidence-bdd/table.md与draft-readback-report.md。

OSX-SIGN-ROSETTA P0接续：cc首真签run42556245736公证Accepted/arm64签后可跑，但DeveloperID+hardened runtime x64 Rosetta --version挂起，同源ad-hoc同entitlements正常。先核run2原始字节/签名/同包环境，建立最小RW->RX内存执行对照，MAP_JIT/pthread_jit_write_protect/allow-jit仅为待证路线，不把Rosetta采样位置当根因。cc三签名路径不改；L0/core冻结继续，若产品修复必要只给明确单路径候选补丁并实际验证，不覆盖远程车道。所有复现子≤8秒/外≤60，留原hang并清进程；未过团队签名+Rosetta真实复验不得宣称修复完成。

OSX-SIGN-ROSETTA最小裁判：先比较完全无动态执行内存的return7程序，由unisacc与系统clang各生成x64，私有副本仅本机现有DeveloperID/runtime/无timestamp与ad-hoc签名对照；目的是区分Mach-O加载/AOT与kn_load执行，不上传、不公证、不碰原件或签名workflow/证书配置。当前任务明确团队签后差异，必要私有签对照在本任务范围内；不以本机签字节代替后续cc正式签验收。若无JIT程序也挂，停止把MAP_JIT当既定修法。

OSX-SIGN-ROSETTA反证实测：原路径签后3679a611在统一cleanenv+P4 fffa两次8s超时142；同字节cp -p新副本DeveloperID/同runtime/同唯一entitlement正常version0，原路径sample全Rosetta routines/约200K无应用符号，原材料SHA不变。独立systemclang return7内存执行23子实测：RW->RX在全部ad-hoc profiles正常，无hang；只MAP_JIT无allow-jit被EACCES拒绝，allow-jit MAP_JIT正常，supported_np=0。尚不认kn_load/RW->RX为根因，不加泛权限或动冻结L0。下一步从公证DMG私有读出四Mac已签成员，严格原字节重包到新内容key/新inode，核真实Rosetta和冷/热两路径；若通过给无源变更的签后修复候选。另本机DeveloperID私有最小样本签名尝试因errSecInternalComponent失败，仅ad-hoc旧字节，不计团队签名测试。

OSX-SIGN-ROSETTA调用链收窄：所绑src/unisacc main->dv_main->dv_parse_args/entrybind --version分类->dv_parse直接ex_ri_say->sy_printf->return，未到kn_call/core_run，查无静态constructor触发kn_load；因此正常version控制流的这次挂起不能由后续kn_load RW->RX修复，sample单独不证明何时停。公证DMG四签成员复制保留quarantine，直exec8s挂与无xattr副本成功不得混为同变量。继续用正常sh APE原字节解压新内容key复验；不删任何xattr/不关策略，不扩entitlements。

OSX-SIGN-ROSETTA签后原字节候选实测：release-ape显式替换四公证DMG内DeveloperID成员，APE5e110594033cbaa9e81d59c8677eef4c2727efa79269d35366ff556b37ec177c/6450974B。不改kn_load/entitlement/冻结源，不覆盖原未签候选。正常sh用户路径两格冷version0、cold/hot -run r7均7，四缓存strictverify0；两独立cache检查各108/0且改写fixture也过，无残留。原裸路径两次142与带quarantine私有直exec142保留。此为LOCAL-CHECKED签后重包候选，不转移bdd原六格CI，也不声称新APE已公证或原宿主Rosetta状态已根治。

OSX-SIGN-ROSETTA当前交接结论：只读syspolicyd日志11:29/11:38对团队L2/id osx-x86_64/NOT_A_BUNDLE明确Prompt shown/waiting for response，支持相关裸成员启动阻塞在Gatekeeper评估，不能因path散列把全部8s超时唯一归到同prompt。正常--version链不触kn_load，团队签名新APE实际-run7与FFI0两格又证现RW->RX可工作；无理由改冻结L0/allow-jit。候选out/osx-sign-rosetta/candidate/unisacc.com SHA5e110594…/6450974B，members96ab2e58be8e5a3fd6cb63c3ce7f686d652c45f7aafb7056aa4423c36615af81，两格cache108/0、热10样本ARM12/96ms及Rosetta45/254.5ms通过(既有Terminal上下文)。四签成员原字节/签名/原P4不变，原DMG只读卸载，无残留；LOCAL-CHECKED不迁旧CI、不冒新DMG公证，cc三签名路径未动。完整最小对照和集成边界research/c48-osx-sign-rosetta/result.md，后继cc将签后APE与同字节成员入最终DMG并按新身份走CI/签公证。原裸CLI/私有quarantine直exec142保留不涂绿。

OSX签后重包runner交接：用户要完整自举命令写research/c48-osx-sign-rosetta/repack-cmd.md；完整checkout冻结f099并把核SHA的草稿bdd APE放runner根unisacc.com，因release-ape内层固定sh ./unisacc.com再运行ape-pack。需要6裸片+完整footer P4；现draft缺Winx64/Linux2三辅助原裸片，将按既有完整SHA补充(不改5角色成员/pins)。不可逆PE资源回注不能把从APE剥出的带.rsrc Win片直接再当pack输入。固定首签四bytes回放应5e11；新的timestamp签名原字节变，新APE SHA必须实算，不硬码历史输出。

OSX签后重包命令交付已实证：research/c48-osx-sign-rosetta/repack-cmd.md三bash代码块经CX提取，在独立完整f099检出/草稿bdd根bootstrap下回放；step1同draft5输入远端重下载SHA全OK、step2实际0输出精确5e110594…，四签后源与原输入前后SHA均同、receipt输出SHA核同。补充Winx64/Linux2三辅助资产上传actual0并远端digest/下载核同，不改原5members/pins。文档给DMG接线并区分历史5e11回放与未来timestamp新SHA，不改cc工作流。

0.0.48.1 macOS发行附件裁定：cc确认run42561596269完成签->重包->DMG->公证Accepted->staple，本机Gatekeeper/ARM及Rosetta -run7与热0.22s验收通过。公开0.0.48.1时必须附该run的macos-stapled-dmg内最终DMG，并明确Intel实机/macOS13未测。本车道先取artifact核staple后完整SHA、验证签名/票据，将同字节DMG及独立SHA附件准备到现0.0.48.1草稿，发布说明写入待发布文案；不据本通知提前公开/改Latest，不改cc三签名路径。签后APE身份与原未签bdd42/0继续分账。

0.0.48.1 macOS附件已就绪：从run42561596269/macos-stapled-dmg下载并核post-staple DMG6e25e7c9f9efec717c1a0cf33bc7a35ad8c2bfccc2a0b2ab9e807ff2b40f0cc0/5475910B，stapler validate0、Gatekeeper accepted(Notarized Developer ID)。以unisacc-0.0.48.1-macos.dmg及独立sha256资产附到现v0.0.48.1-osx-launch-rc1草稿，上传真实0且远端重新下载SHA核同。发布说明plans/release-notes-0.0.48.1.md与release/macos-0.0.48.1.tsv固定run/DMG/内嵌签后APE988a528d…身份，明确Intel实机/macOS13未测，原bdd未签42/0不迁用。公开仍待按原发布流程执行，不修改Latest/cc三路径。

---

## 2026-11-11 prd.md 重整时删去的段落（索引，非逐字；原文以 `git show 6a4fa1fa6f:prd.md` 为准）

本次没有把旧段落逐字搬来，因为它们与各版发布说明重复或已被后续事实推翻。逐项列出原位置与去向：

| 原位置（6a4fa1fa6f 的 prd.md 行号） | 内容 | 去向 / 为什么删 |
|---|---|---|
| 77-79 行（§2 开头三行） | release-members 夹具 21/0、库存 652 -> 654、Mac daily 批收尾实证 | 进度流水，不是能力入口；入口一行留在 §2 |
| 236-252 行（§3） | 0.0.47 APE `fde2db18` 的 com-tree 取数 | 换成 2026-11-11 对 0.0.52 APE 的实测（§3.0） |
| 260-270 行（§4.0 七流表）、274 行（§4.1） | 流表与十八网清单 | 并入 §3.3、§3.4（唯一权威），§4 只留引用 |
| 288-293 行（§4.3 图） | booter / 基座 / 阶段权重 / 资源 四分法（L0-L3） | 2026-11-07 起被结构五层取代（plans/structure-5-layer.md） |
| 305-341 行（§5） | cx 树、包层表（2026-11-08 行数）、ext 符号链接已核事实 | 迁入 prd_cx.md §3（包层表换成 2026-11-11 字节实测；已核事实段近乎原文迁入） |
| 369-379 行（§7.3） | 2026-11-08 覆盖层行数（合计 462、批次 550） | 换成 §3.8 的 2026-11-11 实测（662 / 831） |
| 426、428、430 行（§9.1） | 0.0.51、0.0.50、0.0.49 三段发布身份 | 与 plans/release-notes-0.0.49 / 50 / 51.md、prd_release.md 重复 |
| 432-439 行（§9.1） | 「当前版本 0.0.52」、已上 main 列表（L 两步、G1 第一步、0.0.49 工具总补丁）、在途 | 0.0.52、0.0.53 已发布，旧进度不再是现状 |
| 526-528 行（§9.1） | G0 六片 g1 复现、Y 设计结论（Y-0 -> Y-A -> Y-B -> Y-C）、D / R / S 在途 | G0、Y-0 已随 0.0.51 发布；Y 设计在 plans/v0.0.50-unisacc.md §2 |
| 530-533 行（§9.1） | 三个最大阻塞（冻结到可发布十余小时、Mac/Win 贴线与方案 X、全批资格与第二真相源） | 改写为 2026-11-11 的三项；旧文所引 0.0.49 数字见 phase-times 与 0.0.49 发布说明 |
| 584 行（§10） | 2026-11-10 子文档存在性核对 | 一次性核对记录 |

