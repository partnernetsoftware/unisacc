# 原生测试目录与调度

这是一套统一的开发测试入口，完整保留原gate计划的命令和断言。catalog按七流、网、构造、集成、平台和基础设施归组；无法明确归属的项保留integration，不据名称推定测试覆盖或依赖完整。

这些工具都用unisacc.com默认-run，不调用旧Python队列：
- catalog.cx：严格导入原NUL计划，记录计划SHA和每项所属模块/层；--check核验完整一一对应。
- graph.cx：读取既有gateorder.json.after，输出计划绑定的依赖图；拒绝环，报告不在当前计划的旧名称。
- run.cx：按单元或模块选择，自动补全传递前置任务，按依赖并发；分别保存原argv、stdout、stderr、真实rc、耗时与输入摘要。
- prepdecl.cx：核验43组准备声明与197个成员参数完全一致，输出独立任务；reader typed拆为C、独立参考及原比较、表、网络四步，base reference拆为原JSON构造与原精确比较两步，保留兼容聚合入口和全部原断言。每步保留原看门狗，gateorder保留组内顺序及原组间依赖；旧聚合超时记录不作为新单元通过。

## 从完整计划选择

在仓库根执行，输出使用新路径：

```bash
mkdir -p out
sh tests/gate.sh --plan --com > out/suite.plan.nul
sh ./unisacc.com tests/suite/catalog.cx -- out/suite.plan.nul out/suite.catalog.tsv
sh ./unisacc.com tests/suite/graph.cx -- out/suite.plan.nul tests/gateorder.json out/suite.graph.tsv
sh ./unisacc.com tests/suite/run.cx -- out/suite.plan.nul out/suite.catalog.tsv - --module e4 --graph out/suite.graph.tsv --order tests/gateorder.json --list
```

必须先导入完整计划和图，再选择模块，不能先裁掉准备项。当前Windows消费拆片使完整计划增加5节点；实际节点／依赖边以重导出的完整计划和图为准；导入证明目录完整，实际测试覆盖与通过状态以回执为准。

Linux/x86_64 的 union16 direct1 按 is8、si8 两个自然 case 调度，完整保留各自四种压力、三种优化、sanitizer 和每种优化100次调用。配置真实 `UNISACC_FFI_X86_PROVIDER` 时，`lib-union16-runtime-prepare` 只准备内容核验的 runtime 材料，11个功能单元通过前置边等待并使用 `--require-cached`；不缓存测试结论。未配置 provider 时保留原无缓存调用路径，其它平台保留原参数。准备成功只记 PREPARED，材料缺失、损坏或锁忙返回75，不能记功能通过。

Linux 显式 provider 的原生选择器每次核验 manifest 与六件材料，并在操作后复核完整身份；未配置或空值保留既有选择规则。测试环境还须把已配对 provider 的 `include` 放到 `CPATH` 首位：核验头文件内容不等于编译器已选中该头。当前实际固定F4已用 `cc -E -H` 核过两个头的路径和SHA；最终环境须绑定自己的实际路径与内容。

执行示例：

```bash
sh ./unisacc.com tests/suite/run.cx -- out/suite.plan.nul out/suite.catalog.tsv out/suite.e4-run --module e4 --graph out/suite.graph.tsv --order tests/gateorder.json --jobs 1 --limit 55 --parent-container-unset
```

可用--unit ID替代--module；--layer prepare可与二者求交集，随后仍自动补齐跨层前置；只有--layer时保持默认list。开发定位加--fail-fast：首次非零结果后停止启动新任务，已启动子任务正常结证；真实失败前置的下游记BLOCKED，其余未启动项记NOT_RUN，rc/elapsed为-且没有argv/stdout/stderr进程材料。summary及completion保留not_run计数，覆盖未完成时不会返回成功。默认仅list；零选择或旧输出目录会失败。--jobs范围1..4，默认1；各个子命令由原tests/bound限时1..55秒。共享状态或重活的并发资格仍须按原合同检查，不因同属一个模块就自动允许并发。

default-run会注入自身UNISA_CONTAINER，执行时必须声明调用前的状态：上例假定原本unset；显式值使用--parent-container-value VALUE（空值也有效）。运行器在每个子命令前恢复该状态，随后原命令自己的环境赋值仍优先。列表模式不要求这个参数。

## 结果合同

- 准备项成功记PREPARED，不计入test_pass。
- 前置非零时，下游记BLOCKED，rc为-，不启动进程。
- 实际rc1/2/77/142等原样保留为非通过；其中可能是拒绝、宿主未核或资源证据缺口，不能统称产品算错。
- 每项保存原argv的NUL字节和SHA，不用shell重新拼接命令。
- plan、catalog、graph和order在末尾重新核验。只有完整结证才产生completion.tsv；其中仍如实保留失败与阻塞计数，completion不等于全绿。
- 首版不自动复用历史结果，不把未知依赖当成可缓存。仅选受影响单元的开发循环与最终集成验证分开进行。
- 没有图时必须显式--independent，表示调用方提供的是已确认独立的计划；不得借此跳过完整套件的准备依赖。

## 第一批单元化

nativecheck.sh保留三次独立构建和compiler/network运行器逐字节相等断言。相同运行器字节、同显式输入且不查询自身路径的场景只执行两个不同内容类的代表；网络构建运行器的独有self路径和资源缺失负例仍实际执行。

阶段样例与全源self分开；链路样例、C self和network self分开。原stages/chain/all兼容入口仍保留。每单元保留原比较、输入和预算，准备/执行结果不混记。

Windows lower 使用winlowerphase.cx默认-run控制五个准备节点（共享run／hello／fib、各架构JSON及表）与七个显式消费scope：x86_64的cases／refusals-1／refusals-2，以及arm64的cases／refusals-1／refusals-2／large。cases完整保留fixture、regnames、hello、fib（arm64另含immediate），refusals-1／refusals-2按原Windows缺失导入顺序的偶／奇索引分区（当前每目标13＋12项），每片断言非空，联合完整覆盖原25项；显式refusals CLI仍遍历全量，large实际执行原arm64大输入检查。七scope联合覆盖旧两目标完整消费；每片只报告部分覆盖。默认check ARCH及wincheck.sh --arch仍执行all。共享缓存采用永久文件上的非阻塞flock：准备独占、消费共享，描述符随进程及其子树结束释放，绝不unlink锁文件；旧目录锁不自动转换。依赖图仍串行五准备及七消费；part不进入材料key，各片复用同一套完整核验材料。准备按固定产物列表、上游seal及内容身份核验，单写锁保护原子发布；消费缺失／损坏返回75，不隐式构造，各scope实际执行其原fullcheck断言，七scope联合保留全部比较、系统调用拒绝及arm64大输入检查。各叶仍须在原41秒预算验收，不继承旧聚合超时的通过状态。旧wincheck.sh无参／--arch兼容；旧独立对照由该入口继续持有，原生控制不代表其解释器依赖已消除。

callback lower 的完整off9／on5产物必须为非空常规文件，固定完整清单的实际SHA输出须与artifacts.sha256逐字节相等，生产入口才可命中；缺失、损坏、截断或空清单会回到原生产路径。两个check入口每次执行原比较与全域NN检查。消费part1缺少或损坏on模型时提前返回75，part2～4不要求on模型。尚无通用同身份续跑：一次模块执行中断后不能把旧输出目录当成续跑状态。

carrier import 用默认-run分开准备工具与模型：prepare-tools封存dump／run及三个现有C构造器，prepare的models-only只消费已核验工具。完整basis绑定构造命令、源码、系统头readset、环境及工具身份；阶段不进入材料key，缺失／损坏返回75。两个消费者仍执行原import／package断言；准备成功不算测试通过，原Python准备入口保留独立材料对拍。

libraryfunctionimports依次登记--prepare-gen/c-json/p-json/tbl/net/runner六阶段；消费入口使用--require-cached。各阶段核对完整前置seal，重复准备只复用已核验产物；缺失或损坏返回75要求准备，消费不偷偷重建。构造、C/Python比较和20案例消费断言仍保留。实际Python对照阶段约35秒，旧20秒限时会超时，仅此阶段定点调整为40秒，外层41秒；其余构造20秒、消费35秒不变。

suite-tools-native通过默认-run运行check.cx：原声明恒等、依赖顺序、BLOCKED零执行、准备与测试分计、真实77/142失败、容器状态恢复、错误绑定拒绝等7场景28断言。独立tools-suite-failfast-run入口验证check.cx --fail-fast的3场景86断言：首红停止、已启动任务结证、NOT_RUN与跨层依赖补齐；两个入口各自限时55秒。按精确脚本路径确认的E3/lower单元归入对应模块，跨流或未知项保守integration。

该入口尚未替代正式发布验收，也不表示全部旧oracle已经原生化。原断言中仍必需的Python如实保留；新增目录、依赖图和调度逻辑均为.cx。

union native/scalar共享原生libffiprovider模块：Linux按既有LIBRARY_PATH选非空常规libffi_pic.a，实际path/SHA进入原报告及输入读集，并在构建前后复核；仅ENOENT可跳，目录／空文件／ENOTDIR／权限等错误拒绝，其它host或无匹配保留原-lffi。scalar准备和无cache两条构建路径一致；v3缓存basis明确绑定provider path/SHA及含sanitize／soname／arch的原命令recipe，旧nonPIC缓存不复用。所有原普通／sanitizer断言、loader核验和准备／消费者区分保持；新准备不代表18消费者已通过。

scalar原六case×三pressure准备分为三个显式group（int8/float4、double8/mixed2、mixed3/mixed-reordered），均保留三pressure和sanitize。group1先产同一runtime，group2/3依赖group1；原full18 --prepare完整argv保留为验证barrier并依赖全部三组，原18消费者仍只在barrier后执行。各原40秒上限不变，准备仅记PREPARED，不冒充消费者PASS；分组并不借旧部分cache命中声称冷准备已通过。

composite普通／cached路径同样复用libffiprovider，实际provider路径／SHA进入完整报告、runtime输入读集及v2 cache basis；原arch、sanitize及完整link recipe绑定，旧v1 key不复用。构建前后、缓存校验及拷贝后复核实际provider；原nested struct/array/union、direct/callback、三pressure及所有opt/counter/extent/alignment断言和flags不改。该族六noncached＋准备一项＋十四cached是二十一个独立旧义务，准备成功不冒充功能通过。

Linux/x86_64另设composite-runtime-prepare-ready原生准备前置；六个旧完整功能单元仅改为require-cached，保留全部case、pressure和独立断言，旧prepare仍完整校验材料，十四cached不合并。其它host不列新前置、六条原argv不变；缺失前置按既有图合同忽略。准备只记PREPARED，全部二十个功能单元仍须实际执行。

`lib-callback-source-graph`默认-run门面固定原shell SHA，仅免编未使用的ref可执行文件（仍由build_ref --source-only生成相同wrapped C），并从既有exec-chain-model-prepare的完整身份／产物SHA合同materialize e3.json。原cc -O2 runner、dump源码变换、完整sim／C／host解码断言和cleanup不变。门面先恢复原parent UNISA_CONTAINER unset；缺缓存75不隐式构造。显式前置只依赖准备节点，不复用任何测试verdict；原shell聚合CLI兼容，仍诚实保留独立Python裁判依赖。

`target-package-1/2/3`默认-run门面分别选择Linux/macOS/Windows两架构，固定生产targetpackage.py源码SHA与完整六目标域；三片联合覆盖原六目标闭包。每片仍运行完整源重组、坏／缺分片、bad-target及缺另一target负例，wanted过滤始终使用全局六目标，只有目标执行循环选片。原Python独立裁判仍保留，原无参／单路径全六CLI兼容，分片输出准确2/6；这不是Python依赖消除。

callback lower part1由`exec-librarycallbacklower-model-1-off`与`-1-on`两个默认-run单元联合覆盖：固定原脚本SHA，off保留原fixture生产及12行off verdict，on保留8行on verdict；两者均保留原12组off及on模型seal检查与cleanup。原`other11 on targets UNVERIFIED`完整保留，不代表其它目标已执行，也不声称完整lower ABI通过。原脚本part1聚合CLI兼容保留；模型key、原producer与seals不变，verdict每次真实执行。
门面按固定原7939字节的prefix7497/off294/on148直接取块，并验证完整SHA及逐字节重建；C源不重复原命令，原脚本中的三个真实解释器调用及依赖仍保留。`--inspect`额外报告两段实际生成脚本SHA，可与旧H3门面逐字节对拍；这不替代新H6两个原40秒完整执行。

source-to-ELF原第一片`ELF_SHARD=1/3`分为`exec-srcelf-1a`（1/6）与`exec-srcelf-1b`（4/6），两个有序子集不重叠且并集完整等于原第一片。第二、第三片不变；所有入口仍运行原check-elf.sh、原完整七阶段与独立C/Python参考比较，不复用verdict，不改变模型key或原40秒验收界。分组只提供联合覆盖，旧第一片超时保留。

`lib-context`经`librarycontextcheck.cx`默认-run门面选择Linux既有LIBRARY_PATH内的常规libffi_pic.a并打印实际SHA；仅ENOENT可略过，其它stat错误或非文件拒绝。无匹配或其它host仍用原-lffi。原Python独立裁判的全部并发、多单元优化、include、错误恢复及参考断言保持，仅链接单参数由门面传入；这未消除该oracle的Python依赖，不改hosttools/env-pin。
`lib-typed-host`的Linux严格C99探针声明`_GNU_SOURCE`，普通与ASan/UBSan两次构建仍保留原六行为及USBIND3完整断言；两项完整资格待独立H5实跑。

DRAFT callable门面：lib-callable-model仍完整独立执行原wire检查；var聚合三个原checker分别由-promoted、-wire、-native三scope完整执行，重复wire义务不合并。每片恢复parent UNISA_CONTAINER unset，仅source-only免编未用ref，并按既有exec-chain完整身份合同materialize相同parse2 JSON；原runner flags／dump变换／checker／cleanup保留，缺缓存75。原两sh聚合CLI不改；这只是待独立原40实跑的调度候选，不是消除Python依赖或完整通过声明。

九个原生共享runtime scope（typed/sig2共用libnative、variadic、callable-variadic、callback native/forward/outcome及两exports）复用cxlib.libffiprovider选择Linux LIBRARY_PATH内实际PIC archive，并在shared构建前后复核路径/SHA；JSON绑定provider及runtime，传递模块body列入files/guards。host executable链接、Darwin -arch和sanitizer及全部原公共调用/ownership/loader/exports/并发断言保留；五个仍活跃Python入口本刀不改。此为待独立H13原40资格的接线，help/依赖轻检不代表完整通过。

五个Python oracle（bindings/data/resolver/lifecycle/artifacts）现由libraryoracleprovider.cx固定白名单门面选择实际PIC link/SHA，覆盖UNISACC_TEST_LIBFFI_LINK并恢复parent UNISA_CONTAINER unset；POSIX直接fork/exec/wait继承原stdout/stderr、argv和完整后代看门狗，执行前后复核provider，其他平台保原exec路径。只替原shared-runtime一个链接参数（artifact传现build ldflags）；原算法/循环/断言/参考/超时/host executable链接不变，Python运行依赖尚未迁移，完整H14资格未做。

union16仅在Linux且CLI/环境均未显式配置FFI flags或qualified provider时选择实际PIC archive，前后核路径/SHA并记入source_closure；原qualified3.5.2完整manifest分支保留，普通archive不伪装该协议。原is8,si8四压力、O0/O1/O2和sanitizer全部断言不变。实际共享链接已0，但首次原40资格在is8/gp5fp7的O0值/复制/canary断言仍rc1，不能记整项通过。
