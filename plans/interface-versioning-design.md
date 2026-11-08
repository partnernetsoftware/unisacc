# 接口版本化设计(I2)——7 阶段接口摘要与消费者机查

状态:设计+原型。原型 `tools/interface-digest.cx`、`tools/check-interface-digest.cx`、`tools/interface-registry.tsv`(未登记批次,未进 prd 能力速查,未触 src/cx/exec/include/seed/kernel/weights)。

## 1. 7 阶段接口清单
阶段表唯一来源 `exec/pipeline/image-stages.tsv`(stage/in/out/model/official)。格式文本规范 `exec/pipeline/formats.md`;tape 另有 `docs/tapebin-v1.md` + `.shape.tsv` + `.records.tsv`。

| 阶段 | 输入格式 | 输出格式 | 生产者(模型) | 规范位置 |
|---|---|---|---|---|
| e2 pp | src.c | pp.text | e2.net | formats.md `- **src.c**` / `- **pp.text**`、`## Preprocessor diagnostic envelope` |
| e1 lex | pp.text | tokens.typed | e1.net | `- **tokens.plain/typed**`、`## E3 token trailer`、`## Token offsets` |
| e3 parse/type | tokens.typed | tape.text | e3.net | `- **tape.text**` + tapebin-v1 三件 |
| e4 opt | tape.text | tape.text | e4.net | 同上(同格式进出,摘要相同=合同「不改格式」) |
| prune | tape.text | tape.text | prune.net | 同上 |
| lower | tape.text | target.text | lower.net | `## target.text (lower -> elf input)` |
| elf | target.text | image.bytes | elf.net | **无文本规范**,仅 `keep-elf.txt`(薄弱点,见 §6) |

消费者 = 读该格式的阶段 + 非阶段读者。机扫实测(`--consumers`,排除 plans/archive/out/.git):src.c 19 文件、pp.text 16、tokens.typed 15、tape.text 39(33 scan + 1 已列 + 5 推断)、target.text 28、image.bytes 13。大头是 tests/ 矩阵、cx/{compiler,runtime}/stream/*.cx、tools/ 链接与链路工具、formats.md/prd.md 自身。

## 2. 接口摘要
- **定义**:`sha256( 对该格式每条规范项 "SPEC\n"+抽出文本+"\n" 依登记顺序拼接 )`。规范项 = 整文件 | `FILE@NAME`(`- **NAME**` 起的缩进块)| `FILE#TITLE`(`## TITLE` 节)。规范文本+语法表(tapebin-v1.shape/records.tsv)都进摘要;不含实现、不含注释以外的 prd/plans。
- **粒度**:每格式一个摘要,不是全树一个。改 `Warning CLI routes` 节,6 个格式摘要全不变(检查 c);改 tape 块一字节,仅 tape.text 变(检查 b)。
- **何时升版本**:摘要变即「接口变」。语义分级:①只补充说明/措辞(摘要变但字段集合不变)→ 登记 `minor`,消费者无需动,仅刷新其钉;②增删改字段/标记/拒绝语句 → `major`,消费者必须复核。分级是人判,摘要只负责「别漏报」。版本号不另存,摘要即身份;`IFACE1` 登记里可后续加 `V fmt N digest12` 行记人工版本号(原型未实现)。

## 3. 消费者声明依赖(沿用 CXROOT1/CXWORK1 风格,不另起格式)
`tools/interface-registry.tsv`:首行魔数 `IFACE1`,`#` 注释,TAB 列,行首字母分类(同 CXWORK1 的 P/I/C/D/T/G):
- `S FMT SPEC` 规范片段(摘要输入);`C FMT FILE KIND NOTE` 手工消费者,KIND=listed|inferred。
- **拟扩(未实现)**:`U FILE FMT DIGEST12`——消费者文件钉住自己读的接口摘要;`--stale` 比对现算摘要,不符=该文件未复核。可并入各自包的 CXWORK1 `D` 行或 cxroot 邻接清单,避免新建分散文件。
- 与 cxroot 区别:cxroot 解析 `use` 名到文件(源级依赖);IFACE1 声明「数据格式」依赖,二者正交,不互相替代。

## 4. 改一个字段波及哪些消费者(机查)
`sh ./unisacc.com tools/interface-digest.cx -- --consumers tape.text`:输出 `CONSUMER fmt file source`。source:`scan`(全文含格式名,机扫事实)、`scan+listed`、`listed`(人确认读它)、`inferred`(按名字/路径推断,**未核实**)。
流程:改 formats.md/tapebin-v1.* → `--table` 看哪个格式摘要变 → `--consumers` 列文件 → 把文件集交给 `tools/affected.cx --files` 选批次、`tools/net-deps.cx --touch` 选网。T1 类改动(tape 加 TAPE-END/`.unit` 帧)实际波及:e3/e4/prune/lower 4 阶段 + 上述 39 文件 + C 参考端 seed/gen.c(推断)。
局限:扫描只认格式名字面,**不认字段名**;只引用 `tape` 而不写 `tape.text` 的文件会漏(tapebin 工具多半如此)。精度改进=字段级索引(§6)。

## 5. 与源 SID、分治身份(v0.0.48 §13 C0)的关系
- 源 SID(全树单摘要)≈ 所有输入的并集,接口摘要是其**子集的子集**:只含格式规范。接口摘要不替代 SID,用于「分治」时作**边界哨兵**。
- C0.2 单网摘要 = 闭包表+生成器+seed 的内容摘要。拟组合:`网摘要 = H(闭包摘要, 输入接口摘要, 输出接口摘要)`。网内实现改但接口摘要不变 → 下游网可复用(这是降耦合的收益来源);接口摘要变 → 上下游网都要重验。
- 约束:接口摘要变而网闭包摘要未变,应判为「文档先行,实现未跟」,门禁红(规范与产物脱节)。目前阶段表显示 e4/prune 输入输出摘要相同,可机验「不改格式」。

## 6. 风险与不做什么
风险:①formats.md 块抽取靠 `- **NAME**` 前缀与缩进,格式重排会 `section not found`(安全失败,拒绝而非静默);②摘要按文本字节,空白改动也会升;③image.bytes 无文本规范,摘要只覆盖 keep-elf.txt,等于未覆盖,**不能当作 elf 接口已受控**;④消费者 `inferred` 行是猜测,需逐个核实后升 listed;⑤`--consumers` 对 prd.md/tests 矩阵会过报(只提及不依赖)。
不做:不改 exec/ 与产品;不引入新文件格式解析器(摘要只读文本);不把摘要写入产物/缓存当 PASS;不做字段级语义 diff(留 §4 局限);未登记批次/ratchet、未改 prd 能力速查(合并时再做);版本号自动分级不做。
