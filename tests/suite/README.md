# 原生测试目录与调度

这是一套统一的开发测试入口，完整保留原gate计划的命令和断言。catalog按七流、网、构造、集成、平台和基础设施归组；无法明确归属的项保留integration，不据名称推定测试覆盖或依赖完整。

三个工具都用unisacc.com默认-run，不调用旧Python队列：
- catalog.cx：严格导入原NUL计划，记录计划SHA和每项所属模块/层；--check核验完整一一对应。
- graph.cx：读取既有gateorder.json.after，输出计划绑定的依赖图；拒绝环，报告不在当前计划的旧名称。
- run.cx：按单元或模块选择，自动补全传递前置任务，按依赖并发；分别保存原argv、stdout、stderr、真实rc、耗时与输入摘要。

## 从完整计划选择

在仓库根执行，输出使用新路径：

```bash
mkdir -p out
sh tests/gate.sh --plan --com > out/suite.plan.nul
sh ./unisacc.com tests/suite/catalog.cx -- out/suite.plan.nul out/suite.catalog.tsv
sh ./unisacc.com tests/suite/graph.cx -- out/suite.plan.nul tests/gateorder.json out/suite.graph.tsv
sh ./unisacc.com tests/suite/run.cx -- out/suite.plan.nul out/suite.catalog.tsv - --module e4 --graph out/suite.graph.tsv --order tests/gateorder.json --list
```

必须先导入完整计划和图，再选择模块，不能先裁掉准备项。原1142项计划的实测导入为1142节点、782条依赖边；这证明导入完整，不表示1142项已执行通过。

执行示例：

```bash
sh ./unisacc.com tests/suite/run.cx -- out/suite.plan.nul out/suite.catalog.tsv out/suite.e4-run --module e4 --graph out/suite.graph.tsv --order tests/gateorder.json --jobs 1 --limit 55 --parent-container-unset
```

可用--unit ID替代--module。默认仅list；零选择或旧输出目录会失败。--jobs范围1..4，默认1；各个子命令由原tests/bound限时1..55秒。共享状态或重活的并发资格仍须按原合同检查，不因同属一个模块就自动允许并发。

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

该入口尚未替代正式发布验收，也不表示全部旧oracle已经原生化。原断言中仍必需的Python如实保留；新增目录、依赖图和调度逻辑均为.cx。
