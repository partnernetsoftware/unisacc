# C99 构造器设计：清单与 TSV → δ JSON（B4）

状态：下文为 2026-10-04 的历史设计与推进顺序，不是当前完成率；当前构造入口与配方以 `tools/dev-run`、`tools/generator-json-compare.cx` 为准。本轮 c48-1b 诊断合同见文末，不能据局部定位检查宣称全网或自举通过。

## 目标与边界

`seed/gen.c STAGE OUT.json [--FLAG...]` 由**上一公开版** `unisacc.com` 编译。它读取仓内 TSV、清单与 facts，产生和 Python `exec/build/gen.py` **逐字节相同**的 δ JSON；随后可接 `seed/tbl.c` 与 `seed/net.c`。Python 构造器保留为独立逐字节参考，直到全部阶段及打包、自举闭包通过。`gen.c` 是构建期经典 C99 代码，属于可信基，不冒称网络中的编译决策。

“DSL 9 个 op”只是行首操作数；完整输入还包括清单头、9 列、`when`、fact 载入/覆盖、值前缀、`opts` 的映射与序列模板、`fresh` 的分配顺序、四列表、模板编辑和图的完备化。未实现的构造必须按名拒绝，不能忽略。每阶段的实际使用特性从清单闭包提取后开白名单，避免早期 C 版本假装支持全 DSL。

## 输入闭包和模块

- 入口 `exec/STAGE/gen-manifest.tsv`，递归 `call` 到子清单；按声明顺序读各行的 `facts`、四列 `*-byte.tsv`/`*-result.tsv`、`*-template.tsv`、fresh 表与清单头指定的域。文件都以相对仓根规范路径识别；缺文件、重复定义、非法 UTF-8/JSON、无效键范围和未支持操作显式失败。
- TSV 读取器保留物理行号，按现有注释和 tab 规则解析；不能用空白折叠。facts 采用“标量、对象、数组、字符串”有界树；字符串长度、节点数和加法/乘法都检查溢出。JSON 解析器负责输入动作与 facts，输出器固定 Python 当前 `json.dumps(..., separators=(",",":"))` 的字段顺序和转义。首片只接受 prune 实际出现的 ASCII；其它 Unicode 出现时拒绝，待通用转义实现后放开。
- facts-gen 身份注释（`# exec/facts/`、`# weights/type/`、`# input `）沿用其 `fg_body_start` 规则跳过，不是表的列声明。随后真正的 `# name`/`# value` 等 header 和 typed 行仍按原列数严格读取；身份头不允许少列、多列或缺少声明的正文通过。
- 图结构保留**状态首次创建顺序**、每状态的**观测键插入顺序**、动作序列的**首次出现编号**、标签集合。`on` 的已有边不覆盖；`finish` 按当前 G 的顺序建 RET、补 0..256 缺边、建 DEAD。最终 JSON 字段顺序固定 `start,states,seqs`，状态内容与序列均不排序；这几项是字节相等的必要条件。
- Tape 帧能力由清单安装的状态显式声明，不按阶段名字分支：`TF.check` 声明完整 payload 校验，`TF.prepare` 声明 E3 启动来源准备，`TF.current` 声明安全输出行来源发射，`TF.diagnostic` 声明后段拒绝定位。`finish` 在创建 RET 前注册所有新增 PUSH 续点；仅 JSON 外部入口切换为 `TF.entry`/`TE.entry`，原 START 及内部回边不改，避免回读时重复整流校验。
- 专用构造器保留原有指令装载及完整性计数；新增目录 `call` 行由薄桥传给同一个 manifest walker/visitor，使用原行来源、既有 flags 与环境。子清单的条件、绑定、环境复制/合并和目录递归不由专用分派另解释；这不是新的运行期 call 算子或子网语言。
- `TF.current` 只接在已知完整输出行起点（双空格指令、`.bss`/`.str`，或同序列已输出的 LF 之后），不接 token 读取或半条指令。共享 `TE.capture` 保存隐式观察值，续点先 RLD 恢复；来源子程序不得修改调用者的输入栈或 live SB。文件名按真实字节去重，不将元数据插入编译器符号 intern 表。带来源的原 REJECT 保留原 reason 与已有前置动作，经诊断子程序返回后执行；`unreachable`、`tape truncated` 和空拒绝不递归包装。
- 来源去重缓存只描述当前可见输出。仅安装 `TF.current` 的图在 OCUT/OCLR/SWAP 后失效；同一动作序列以 INPUSH+SPAN2 回放保存片段时，在 INPOP 后失效。真实代码片段捕获入口由既有表行显式失效，保证分离片段自含首来源；跨状态 COPY 回放在结束行失效。不把普通 SPAN2 或所有 OLEN 一律当作捕获，不改原语义动作、顺序或隐式观察值。
- 解释器按现有 9 op 分派，`opts`/值前缀由共享求值层处理；阶段专用 profile 只提供图种类、域、内建事实与有界 fresh 策略。任何 profile 钩子需单列并与 Python 同输入对拍，不能把某阶段的语言判断藏进 C 分支。C 版每落一项须保留一个故意改错 TSV 行会使对拍红的测试。

## 首片：prune

只支持 `exec/prune/gen-manifest.tsv` 的 `rows prune main` 与 `label CLASS.r8`，四列表 `prune-byte.tsv`、`prune-result.tsv`，以及 G 型图的 `load/install/finish`。本片不用 `facts`、`fresh`、`template`、`call`、`foreach`；它们在 C 入口应报 `unsupported DSL op`。实现顺序：C99 行/JSON 读取与动作序列 intern → 四列表显式/default 键展开 → G 完备化 → 与 Python JSON 对拍 → 再抽象为完整 9 op 的解释器。预计首片 600–900 行 C99（含 JSON/TSV/图和错误处理），后续通用 DSL 能力另计；这是规模估计，不是验收数字。

当前冻结输入上，Python prune 输出 **5,164,719 字节、888 状态、467 动作序列**，SHA-256 `068a6a1484c61a67dcdbe32f7dbc6d280fc0ee650f7fd518b8bc7e3c76595585`。首片用同一输入执行 C/Python 两版，`cmp` 完全相等；再各跑两次证明确定性，删一条规则、交换相邻规则及破坏动作 JSON 时按名拒绝。产物 `.tbl` 与 `.net` 由现有 C/Python 两条下游路线各自对拍，但不以网络等同代替 δ JSON 等同。

## 推进与切换

先 prune，再按实际清单特性分层加入 `let`/facts、模板、fresh、递归 call/foreach 和其它图 profile；每新增一阶段：δ JSON 字节相同、平表与网络字节相同、`--check-net` 全域通过。B1–B3 的 C 链与 B4 合流后，最后核对 P3 包、seed、N22 三阶段和六目标出货字节。只有这些义务全部满足，才把默认种子构造入口从 Python 切到“上一版 `.com` 编 C99 构造器”；切换前 Python 仍是正式构造路径。

## 输入诊断定位（c48-1b / T1.1）

- 有输入上下文的生成失败输出 `seed-gen: PATH:LINE: 原拒绝原因`，行号来自物理输入行，不是生成器 C 源码行。尚未进入任何输入的参数/资源失败不伪造文件位置。
- 文件来源在流关闭后仍有效；嵌套清单返回恢复父上下文。延迟处理的清单、规则与模板行保留各自来源，不能被后续读入的行覆盖。输入流注册按需增长，不新增固定 32 流上限。
- `seed/facts.h` 与构造器共用定位拒绝合同；`seed/json.h` 的可选 `JFAIL` 仅增加绑定客户端的诊断来源。未绑定客户端保留原 JSON 诊断与退出 2，绑定后的语法拒绝仍退出 2；不能把无效 JSON 当作正常值。
- `sh ./unisacc.com tools/check-generator-location.cx -- ROOT ABS_GEN FRESH_OUT` 实跑十类中后段拒绝、嵌套/延迟行来源及两项 JSON 边界。OUT 须新建，每例留输入、输出、状态与预期位置；不钉死附带错误措辞。
- 基线 `state3` 被改为未分配的 `state3z` 时，实际 `parse2 --locations` 应在 `exec/parse2/control-result.tsv:4` 拒绝。正常图的字节同、全部配方及六目标机器产物同属于后续 T1.3，不能由这项拒绝检查替代。
