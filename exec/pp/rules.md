# E2 规则来源与构造边界

生产构造路径：这些声明 → `gen.py` 展开有限转移和动作 → `exec/c/tbl.py` →
`exec/c/net.py` 构造确定性网络权重 → 通用执行器运行网络。
Python 在这条路径上是离线构造工具；旧 C/Python 预处理器用于测试对照，不为生成器补答案。

| 声明 | 控制内容 |
|---|---|
| `text-*` | shebang、续行、注释、字面量保护 |
| `autoinc-*` | 自动头扫描、名字判定、按需输出模板 |
| `macro-*` | 定义记录、历史链和可见区间 |
| `cli-*` | `-D/-U/-include/-nostdinc` 资源输入 |
| `operators.tsv`、`expression-*`、`reduce-*` | 表达式扫描、栈调度、运算归约 |
| `hash-*` | 字符串化与拼接、参数字面量和空白处理 |
| `rescan-*` | 对象/函数宏、参数帧、跨帧续接、隐藏标记、`_Pragma` |
| `directive-scan-*`、`directive-action-*` | 指令识别、条件入口、决策之后的动作 |
| `directive-body-*`、`include-location-*` | 宏体读取、头文件搜索和读入 |
| `location-*` | UNIPP2 位置封装（完整 include 名及已解析的 `#line` 映射；布局见 `../pipeline/formats.md`） |
| `predefines.tsv` | 六目标的预定义宏名称与顺序 |

`weights/gold/pp.tsv` 的 schema 给出指令顺序，标签选择 `directive-action` 分节；
不在生成器里复制这18行决策。未提供的动作分节必须失败。
运算符编号和动作中的寄存器名是机器接口，不能只改编号而不改消费它的声明。

空白分类与 E1 共用 `weights/gold/lexcls.tsv`。`tools/facts-gen.cx --write . pp-gen`
派生 `classes`：`hspace` 为 HT/VT/FF/CR/space，`space` 另含 LF；宏内部
paint 标记的并集类仅供已有参数帧使用，不把其它源控制字节归为空白。
指令、宏调用、字符串化与自动包含行通过清单的 `classes` 绑定读这些事实；
LF 计数行保持独立，字符串/字符字面量和注释状态不做字节过滤。
`tests/c99-whitespace.cx` 保留原值、字符串化折叠和非空白控制字节拒绝边界。

`exec/finite_rules.py` 只展开互斥观察范围、默认值、动作序列、分节与显式绑定，
并登记返回标签；不解释 C 指令、宏或表达式。

## 仍保留的装配边界

- `include/*.h` 的导出名字提取、自动包含头的顺序和筛选。
- 自动头与预定义宏的逐项实例连接、输出文本构造。
- 布局常量、初始化数据、指令名到具名入口的连接。
- 通用转移容器、返回分派、序列去重和 JSON 序列化。

这些边界必须如实保留在迁移账中，不能称为所有规则都已脱离手写装配。
默认产品切换、完整编译器迁移与 C99 覆盖是另外的完成义务。

## 验证口径

迁移前冻结的六目标×两种位置模式逐状态、逐观察比较下一状态及完整动作，
不比较动作序号。关闭自动头的两种模式也保留对照。
实际网络由宏/位置检查与固定源码到 tape 链检验；隔离生成不提供旧 kernel、
`gold.py` 或产品 `src/`。临时修改 `pp.tsv` 的 `ifdef/0` 标签使实际网络输出
NO→YES，证明声明决策确实被读取。这些证据证明迁移保行为，不证明完整 C99 语义。

UNIPP2 的独立来源协议对拍入口是 `tests/check-unipp2-reference.cx`：
`sh ./unisacc.com tests/check-unipp2-reference.cx -- "$PWD" PP_RUN PP_NET NEW_ABS_OUT`。
`PP_RUN` 与 `PP_NET` 必须显式指定同候选冻结后的 E2 执行器和位置模型；检查器不构造或运行完整产品链。
它将 `archive/reference-c/src/front_pp.c` 作为“活参考，非历史归档”，经既有 `tools/sourceflat.cx`
显式构建参考程序，并逐字比较相同输入的 UNIPP2 全部字节，包括完整 UTF-8 长名、三层 include、
`#line` 默认 TU 哨兵、显式空名及继承。其他 `archive/reference-py` 检查器仍为历史归档，不参与此入口。
只读经典参考源、当前 kernel/include/共享声明、宿主 C 编译器以及 `tools/refshim.h` 都是显式参考依赖；
检查器保留导出输入清单和摘要，不宣称这些依赖或历史 Python/host C 债已经清零。

`LDRAW` 在 include 区域撤销前保存展开后 PP 文本的绝对物理行；`LDUSER` 才保存撤销
include、前奏及续行映射后的主文件用户行。PP 的逻辑名/行号选择与诊断均按同一物理坐标
选择 `#line` 记录，再按 `user - LDUSER - 1 + LDNUM` 还原逻辑行；头文件区域不应用主文件
的 `#line`。`tests/stream-e3.cx` 的独立手造帧保留 include 在指令前/后、三层 include、
以及 raw/user 不同的诊断断言；上述活参考对拍另保留同形源码与宏输出的一致性检查。

独立 C 参考入口直接初始化并调用既有 `splice/decomment/autoinc/preprocess/expandsrc` 和
`pp_origin_write`，不调用经典 `main/fe_units` 路由。字符和 UCN 判断真实定义来自只读
`archive/reference-c/cx/compiler/srcgate.cx` 与 `frontpp.cx`；检查器记录它们的摘要，
并通过函数/数据 section 和链接 GC 去除未使用的后端，不能用 stub 或忽略未定义符号代替。
这些宿主 C 参考依赖也以正式 `L` 行登记，不声称模型 PP 被用作自己的参考。

共享预定义模型的对拍通过 runner 现有 `--ri-snapshot/--upstream-stage` 接口显式输入完整
`cli/f1`、空 `cli/attributes`、实际 `cli/source`、`cli/target` 和目标预定义列表；二进制键均保留前导 NUL。
列表按 `predefines.tsv` 的目标行顺序派生为 NUL 分隔名字，不在宿主胶水硬编码宏串。
快照由既有 `exec/c/resourceledger.h` 写入器生成并验证同一 source-stage 身份，不用更简模型替换共享模型。
`cli/source` 的值按当前产品 `src/drive.cx` / `src/stages.cx` 的实际绑定包含末 NUL，
不是历史 standalone `archive/reference-c/exec/c/run.c` 的 `strlen(src)` 无末 NUL 形态。
检查器快照匹配产品合同；`cli/target` 则仍是无末 NUL 的目标字节。

include 记录保留稳定遇见顺序、`IRLN` 非降序；同起点外层在前，不能排序或去重。
`IRNL` 是该头插入当时的换行数加一，不是递归展开后的最终跨度，合法 child 可长于 outer。
不同实际目录的同名 `dir/x.h` 可产生完全相同 wire 记录；本布局不能证明同起点父子关系，
故只拒 `IRLN` 下降，不把同起点完全重复或不可证明非嵌套当负例。splice/line 仍严格递增。
独立对拍保留四层、child 更长、不同 IRPATH 同 wire 三类合法源码，E1/E3 保留下降负例；
0.0.49 的“布局带父深度”仅是设计债，本候选不新增字段、N 语义或容量。
