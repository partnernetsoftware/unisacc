# 共享 facts / 生成器依赖拆分：C0.3 前置设计

结论：先把整表读取、单体生成器和打包阶段的身份边界分清，再启用复用。BASE1 的 facts30/42=71.43%、generator42/42=100%仍保留，不改分母、不放宽基线。本方案只完成只读分析与设计；未拆表/生成器、未启用C0.3、未新构造模型。复用[BASE5设计](seed-generator-split.md)及[293函数×42配方矩阵](seed-generator-functions.tsv)，不重做调用图；其静态候选/UNKNOWN及inspect根污染边界全部沿用。

## 实际核对与证据

- `tests/net-deps.tsv`：pp3、lower12、generator42、facts30，universe42。`plans/v0.0.48-unisacc.md` §13规定多数改动波及>60%时先拆共享依赖；若把60%变为单项整数门，42网应≤25，不能把26/42写成≤60%。当前只有最大值，不能推出真实开发改动频次分布。
- 本轮实际默认图session14549 rc0、summary54981 rc0、oracle67014 rc0、touch63346 rc0；均外bound50，只读。summary显示915非生成器输入、863被≥2网共享；最高facts为`parse-constants.tsv`30，`numeric-names.tsv`27。30度的`parse/numeric-byte.tsv`及result也是共享热点，不能只拆facts把它们隐藏。
- oracle对既有固定42配方观测集`tools/affected-model-inputs.tsv`，`obs_not_static_total=0`；但shared-e3静态452/观测337、shared-o1 32/17、win/x86_64-lower135/81。静态有保守余量，不等于未观察到的文件不可能被读，更不证明其它flags/失败路径完备。
- 证据：`out/c48-shared-facts-deps.tsv` SHA256 `ab584f5a38da8f639f79c365268f0e4f3f4220c8cdb358f2c88d12fc7749ee5f`；旁有`-summary.tsv`、`-oracle.tsv`及stderr。touch同时给两个输入，结果为两输入受影响并集，不拿它冒充单文件30实测。
- 冻结源SHA：net-deps `1c7923858560a9fad8f00ebe6d941e7bc1fd6cdd303e9811019859dfa83c1a05`；net-digest `97094299b96249cbb3003f3b9ee1283a52234c9291b9dcb9e8366e7a58db03e7`；parse-constants `76a801015e4bc00c2a9d8f217e22cc95acfdae8cfe499d4a9fc864c89ca465ec`；BASE5矩阵 `c290358f4bc41d939aba6adfc349256267be380bd280385d8f8ba24081c1ecb7`。其它seed/gen/sourcepackage身份见BASE5；本次未跑模型构造或全定义域验证。

重算入口（只读；每条各bound50）：

```sh
./tools/bound 50 sh ./unisacc.com tools/net-deps.cx -- --summary
./tools/bound 50 sh ./unisacc.com tools/net-deps.cx -- --oracle
./tools/bound 50 sh ./unisacc.com tools/net-deps.cx -- --touch exec/facts/parse-constants.tsv
./tools/bound 50 sh ./unisacc.com tools/net-deps.cx -- --touch seed/gen.c
```

## 哪些是真共享，哪些只是保守边

| 来源 | 当前事实 | 处理 |
|---|---|---|
| net-deps:145–212的manifest递归call、rows/table/template、第五列facts及domains/extra | 为每配方取闭包；不求值when/flags，候选兄弟表存在即加入 | 只能在解释when/flags语义、闭包与实际读取均获证后缩边；未知表达式继续保守，缺文件/路径失败仍拒绝 |
| net-deps:253–258的parse2额外tokens2/parse-tokens/diagnostics | seed硬编码读取不在call图；不是可删“脏边” | 先把实际引用声明成显式构造profile输入，仍保留完整依赖 |
| seed/gen.c:384–456 load_fact | 实际读完整facts文件、校全表、缓存按stem；name/value表同时导出列表、顶层键与`stem!`map | parse-constants30现在是实际输入依赖，即使某调用只用2键，也不能仅hash2键而继续读整文件 |
| parse/numeric-manifest.tsv:3–5，parse/prn-manifest.tsv:4，lower/data-manifest.tsv:4 | 数值输出多路径消费DIG、TK_FNUM等；numeric-names的freshrows还影响名字和分配顺序 | 公共数值ABI是真共享，不按前端/后端名字伪分；先证明哪些键/段可独立，再改实际消费 |
| parse2/unitmode-manifest.tsv及gen2/units入口 | 同一parse-constants含HEADER/FOOTER、token code、银行/类型布局等不同目的数据 | 物理/逻辑拆分的首选，但须保持既有bind/别名与插入顺序 |
| net-deps:215–263 shared_gen全42 | gen/json/facts、tbl/json、net/core属于当前构造链；compilerpack/pack属于包层却一起放入每个net | 原图是全构造支持上界。可拆“raw模型→包→切片”阶段边，不得从最终产品依赖图删除pack |
| gen.c单体和共享Value/Graph/manifest引擎 | 每个当前配方都用同一生成器可执行身份，公共实现确会影响多网 | 只有实现真正分模块/分入口并核闭包后才能缩，不可直接按BASE5词法矩阵排除函数 |

## 具体切口与接口（以下均为设计名，未创建）

**facts先按用途投影，再迁实际读取。** 首批从parse-constants中分出`parse-startup`（HEADER/FOOTER）、`token-codes`（被共享的token编码）、`parse-bank-layout`（标记/银行/类型布局）、`numeric-constants`（DIG/TK_FNUM等数值输出真公共键）。numeric-names先按section保留显式有序视图prn/numout，再观察是否适合按目标拆。键成员清单需从实际bind/opts/模板求值读取证明产生；以上仅候选切口，不是已枚举完的键集合。

接口建议为有版本的`FACT_VIEW`：view_id、schema/version、原键及别名映射、确定顺序、定义摘要、实际消费文件SHA、导入view身份；单一权威定义经现有“事实表→生成器→产物”生成。调用者只读所声明view，不再暗读整parse-constants。保留`parse-constants!.DIG`等旧路径所需兼容绑定由显式adapter提供，迁移完成后删除adapter前再核没有隐式读取。load_facts_expr按从左到右合并、同名覆盖、数组插入序、fresh编号、类型宽度和失败合同均须逐条保留。

不能让一个新view生成器的“整主表源SHA”无条件进入每个网摘要，否则只是换名字，仍全部失效。可分两层：全主表校验/投影生成收据先独立执行；每个网输入身份取其实际读取view字节+view定义/解释协议+实际构造模块闭包。主表任意修改都重新验证并生成视图，只有被证明内容/定义不变的视图才可继续用于网级复用。全局校验失败绝不能通过命中旧视图绕过。若“无关坏行以前使本配方拒绝、现在不再拒绝”改变了输入拒绝边界，必须先裁定新schema和阶段职责，不能悄悄称纯机械等价。

**生成器按机制层、族入口、下游阶段拆，沿BASE5顺序。**

| 拟接口 | 实际消费者/边界 | 必须进入身份 |
|---|---|---|
| value/fact reader、Graph/sequence writer、manifest interpreter | 真公共机制；保留共享，不复制到42份 | 源实现和传递模块/ABI、错误与序列化协议；公共改动仍可42/42 |
| pp/lex/opt/nativeabi薄入口与parse2、lower/enc、generic manifest profile | 以BASE5候选为调查起点，实际回调/动态profile/全局状态另核；generic含foreign unitcontainer，不按enc名字硬分 | profile代码+显式facts/table引用+参数/flags+manifest解释版本+公共机制闭包 |
| gen→tbl→net阶段收据 | 每网各stage独立；gen变化先重gen，相等后是否允许复用下游须有受控策略 | 各阶段实际exe SHA、源闭包、recipe和输入产物SHA、输出SHA、验证器身份 |
| compilerpack/pack→切片/APE | 不再冒充单网rawnet的直接读文件，但仍依赖全部实际成员 | 包布局/资源/成员顺序、压缩协议、打包exe与源闭包；不丢最终产品依赖 |

第一步可以仅模块边界清晰而仍链接一个exe；这时exe级generator42依赖不变，不能先宣称波及门下降。若按族构造exe，必须声明全部导入模块并证明未隐式链接其它族；同一真实共享引擎修改仍全量失效。不要通过删除exe身份、只hash“看起来被调用”的函数或复制共有代码来规避42。

`tools/net-digest.cx:27–29`还把摘要工具/依赖分析器/SHA实现身份纳入每网；它们改变时重新确认全部摘要是合理协议迁移，不应为了图好看去掉。生产输入schema应区分实现身份与构建会话身份，不能把宿主路径或其它网生成产物混入本网投影，也不能漏真实编译器/flags/导入代码。

## 顺序、风险与C0.3门

1. 固化42原配方/flags、BASE1图和BASE5矩阵；把每条边记为显式读取、运行观测、保守条件或人工硬编码补边。先工具层完善配置条件/隐式引用说明，未证明的边不删。
2. 选parse-startup vs numeric用途做最小facts迁移，保留数值公共ABI；原主表与视图重构各自产物和拒绝合同先对拍。随后再按section拆numeric-names。不可同时大改解析语义。
3. 按BASE5分公共机制/族入口；消除rd缓存、fresh/global状态与回调的隐藏所有权；再分gen/tbl/net/pack身份。所有独立模块依赖先完整，再量实际波及。
4. 新图与现有读集oracle比较，按42配方及目标flags覆盖做定向扰动：闭包内常量/字节变化必须失效，真实未依赖输入变化摘要必须稳定；条件未知保持保守。放入遗漏依赖、错view/错schema/换exe、坏表、顺序变化等负控，不能只测“注释不影响”。
5. 每一刀对42配方及受影响完整定义域做表↔网全键；原/新JSON、tbl、net逐字节对等、重复构造确定性，状态/序列编号和错误拒绝不减。全键表=网不是独立语言语义oracle；功能语义/六目标产品等价仍走候选慢车道。
6. 重新公布facts/table/generator的原始max、分布和按用途改动样本。普通族内编辑目标≤25/42，pp≤3、lower≤12是当前参考，不预留涨额；真正共享的DIG/类型编码/公共解释器可能仍30或42，必须原样列为真实全局依赖。若global max仍超过60%，不得把这类文件排除出分母或另起类别冒充达标。需先证明§13“多数改动”收益条件，或由主人明确裁定全局核心变更必须全量重建的例外策略；本设计不代行修改该门。

**C0.3不得抢跑：** 上述结构切口尚未实施/全域字节等价与拒绝未核、真实读集遗漏未归零（在声明覆盖域内）、共享核心高波及策略未裁定、独立输入/工具/验证器身份收据不完整，任一项存在即不启生产复用。BASE6仅证明一次pp位置表变更3构造/39复用与42全量对照相等，不证明任意facts/生成器变更可以复用。首个0.0.49候选应先收这些前置；未过则候选任务是补底子，不能把“新图PREPARED”当复用资格。

届时复用仍须同schema/输入闭包/实际构造exe及验证器身份命中、产物内容校验和强制抽样重建；共享核心变更按真实闭包全量，不能靠旧PASS继续。分片stat证书只适用于BASE6同一冻结窗口，不能直接拿来当跨run缓存认证。收益需分别报告cache prepare/摘要/验证/比较/调度和真实端到端墙钟；累计3网构造时间与42网时间之比不是总流程加速比。
