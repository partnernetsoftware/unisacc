# 原生测试目录与调度

这是一套统一的开发测试入口，完整保留原gate计划的命令和断言。catalog按七流、网、构造、集成、平台和基础设施归组；无法明确归属的项保留integration，不据名称推定测试覆盖或依赖完整。

这些工具都用unisacc.com默认-run，不调用旧Python队列：
- catalog.cx：严格导入原NUL计划，记录计划SHA和每项所属模块/层；--check核验完整一一对应。
- graph.cx：读取既有gateorder.json.after，输出计划绑定的依赖图；拒绝环，报告不在当前计划的旧名称。
- run.cx：按单元或模块选择，自动补全传递前置任务，按依赖并发；分别保存原argv、stdout、stderr、真实rc、耗时与输入摘要。
- prepdecl.cx：核验原43组准备声明与193个成员参数完全一致，输出独立任务；每步保留原看门狗，不再让4～9个操作共用一个外层额度。gateorder保留组内顺序及原组间依赖。

## 从完整计划选择

在仓库根执行，输出使用新路径：

```bash
mkdir -p out
sh tests/gate.sh --plan --com > out/suite.plan.nul
sh ./unisacc.com tests/suite/catalog.cx -- out/suite.plan.nul out/suite.catalog.tsv
sh ./unisacc.com tests/suite/graph.cx -- out/suite.plan.nul tests/gateorder.json out/suite.graph.tsv
sh ./unisacc.com tests/suite/run.cx -- out/suite.plan.nul out/suite.catalog.tsv - --module e4 --graph out/suite.graph.tsv --order tests/gateorder.json --list
```

必须先导入完整计划和图，再选择模块，不能先裁掉准备项。当前完整计划为1309节点、944条依赖边；导入证明目录完整，实际测试覆盖与通过状态以回执为准。

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

Windows lower 使用winlowerphase.cx默认-run控制五个准备节点（共享run／hello／fib、各架构JSON及表）与两个完整消费节点。准备按固定产物列表、上游seal及内容身份核验，单写锁保护原子发布；消费缺失／损坏返回75，不隐式构造，每次完整执行原fullcheck全部比较和系统调用拒绝，arm64的大输入检查亦保留。各叶仍须在原41秒预算验收，不继承旧聚合超时的通过状态。旧wincheck.sh无参／--arch兼容；旧独立对照由该入口继续持有，原生控制不代表其解释器依赖已消除。

callback lower 的完整off9／on5产物必须为非空常规文件，固定完整清单的实际SHA输出须与artifacts.sha256逐字节相等，生产入口才可命中；缺失、损坏、截断或空清单会回到原生产路径。两个check入口每次执行原比较与全域NN检查。消费part1缺少或损坏on模型时提前返回75，part2～4不要求on模型。尚无通用同身份续跑：一次模块执行中断后不能把旧输出目录当成续跑状态。

libraryfunctionimports依次登记--prepare-gen/c-json/p-json/tbl/net/runner六阶段；消费入口使用--require-cached。各阶段核对完整前置seal，重复准备只复用已核验产物；缺失或损坏返回75要求准备，消费不偷偷重建。构造、C/Python比较和20案例消费断言仍保留。实际Python对照阶段约35秒，旧20秒限时会超时，仅此阶段定点调整为40秒，外层41秒；其余构造20秒、消费35秒不变。

suite-tools-native通过默认-run运行check.cx：原声明恒等、依赖顺序、BLOCKED零执行、准备与测试分计、真实77/142失败、容器状态恢复、错误绑定拒绝等7场景28断言。独立tools-suite-failfast-run入口验证check.cx --fail-fast的3场景86断言：首红停止、已启动任务结证、NOT_RUN与跨层依赖补齐；两个入口各自限时55秒。按精确脚本路径确认的E3/lower单元归入对应模块，跨流或未知项保守integration。

该入口尚未替代正式发布验收，也不表示全部旧oracle已经原生化。原断言中仍必需的Python如实保留；新增目录、依赖图和调度逻辑均为.cx。
