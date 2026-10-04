# K2 清单 DSL 规格（0.0.24 T1 第一步：现状冻结）

以代码为准：`exec/assemble.py`（下称 A）、`exec/finite_rules.py`（FR）、`exec/build/gen.py`（G）、`exec/facts/load.py`（L）。行号对应 982e3685。
统计范围：`exec/**/*-manifest.tsv` 132 个，`exec/facts/*.tsv` 210 个（T1a 起；A: 行号仍是 T1a 前的，A:187 以后减 5，A:599 以后减 6，A:617 以后减 12）。本文只描述现状，不改行为。

## 1. 清单头（G:7-15，解析 G:35-39）
`#! KEY V...` 行只由 G 在入口清单读取（子清单里的头无效）。

| 头 | 语义 | 出处 | 次数 |
|---|---|---|---|
| `base PATH` | 执行器模块（相对 exec/）作为 E；有 `executor()` 则 E=E.executor()；有 `check(E)` 则跑后调用 | G:7-9,43-49,56 | 10 |
| `flags A B` | 可接受 `--A`；flags[A]=是否给出 | G:10,40-42,52 | 12 |
| `start NAME` / `start $NAME` | 起始状态（缺省 START）；`$` 取 env | G:11,13,53-54 | 4 |
| `graph NAME` | E.g=E.GRAPHS[NAME]()（build/graph.py 显式登记 G、Delta） | G:12,50 | 2 |
| `domains STEM` | E.g.finish(start,{mode:values}) | G:14,57-59 | 1 |
| `extra KEY STEM` | 输出加 KEY=fact | G:15,64-65 | 1 |

顶层 env 即 `E.results`（G:51-53），执行器代码可读 export/result 写入的名字。

## 2. 行格式（A:3-31, A:217-224）
`op stem section when facts fresh seq bind opts`，tab 分隔，补 `-` 至 9 列；`#` 起头为注释；op 前每个 `.` 为一层 foreach 体深度（A:222）。块切分 A:231-240：深度大于当前者归入上一行的体；非 foreach 带体即断言失败（A:266）。

- **stem**：以 `@` 开头时按值求（A:262）。
- **section**：`@` 开头按值求，否则 `_section` 对 flags 格式化：`{FLAG?TEXT}` / `{!FLAG?TEXT}`（A:100-105, A:269）。
- **when**：`-` | `FLAG` | `!FLAG` | `fact:NAME`（非空）| 用 `&` 连接（A:88-97）。
- **facts**：`-` 或 `STEM+STEM`，可带 `load:` 前缀，自左向右合并（A:244-247）。
- **fresh**（A:132-153）：`-`→None；`none`；`split`（`OWNER_KIND`→OWNER 持有者，A:138-139）；`=NAME`（holder 行声明，A:141-144）；`P:NAME`；`U:NAME`/`S:NAME`（未登记持有者）；`tape:FILE@PART`（A:155-164）。先做 `{env}` 插值（A:133）。
- **seq / bind**：逗号分隔 `k=V`，V 走 §4 值求值，自左向右（A:207-214）。
- **opts**：`-` 或 JSON 对象（A:251），键见 §5。

## 3. op（A:264-352；统计：`tests/decisionledger.py --ops` 同口径）
| op | 语义 | 出处 | 行数 |
|---|---|---|---|
| rows | `install(g,root,stem,...)`：stem 旁四列表 | A:324-326, FR:227 | 872 |
| let | `exit` 则 SystemExit；否则 bind 与 seq 写入 env | A:337-341 | 794 |
| call | 子 `Run` 跑 `stem-manifest.tsv`；env=父 env+bind；opts.flags 覆盖（列表=任一）；`merge` 回流 env | A:332-336 | 419 |
| template | `install_template`（FR:443；模板语法 FR:3-78）；mode 缺省 r；overlay | A:319-323 | 263 |
| foreach | 对 over 每元素以 `as`（缺省 it）跑体，见 §6 | A:264-266, A:482-514 | 95 |
| table | `install_rows(g,root/stem,...)`；mode/skip/ordered | A:327-331, FR:212 | 54 |
| holder | 声明持有者 `P:NAME` 或 U；opts.cur；带 opts.cols 时为 fresh 表（A:374-386） | A:289-290, A:346-350 | 19 |
| assert-absent | 断言 stem 状态存在性 == opts.present | A:344-345 | 18 |
| label | stem 逗号名加入 g.labels | A:342-343 | 15 |

共 9 个（上限 12）。`fresh` 作为 op 名只在 cols 分支被接受（A:289），清单中 0 次。其它名字抛错（A:351-352）。

## 4. 值前缀（Run.value, A:166-205；先做 `{NAME}` env 插值 A:167, A:370-371）
按检查顺序：
| 前缀 | 语义 | 出处 | 清单次数 |
|---|---|---|---|
| `@rej:T` | E.rej(T) | A:168 | 117 |
| `@bytes:T` / `@bytes:=PATH` | SBOUT 字节 | A:170-173 | 40 |
| `@str:T` | 字符串，`{name}` 取 facts，反斜杠转义 | A:174-175, A:47-54 | 2276 |
| `@out:T` / `@out:=PATH` | OUT 字节；`{name}`、`\xHH`、转义 | A:176-177, A:110-117 | 220 |
| `@fmt:F` | 最小插值 _fmt：`{NAME}` 或 `{NAME[KEY]...}`（全数字 KEY 取下标，否则取 dict 键），`{{`/`}}` 为字面括号；无转换、无格式说明符（T1b，不再用 str.format） | A:178-179, A:473-492 | 231 |
| `@ref:F` | 格式化后取 fact 路径 | A:180-181 | 49 |
| `@seqmap:LIST:TMPL` | 对 LIST 每元素实例化动作模板 | A:182-183, A:584-596 | 5 |
| `@acts:PATH` | JSON 列表转元组 | A:184-186 | 107（含 T1a 的 18 个 stack_*，facts k2-stack） |
| `$$D:K` | env[D][K]，K 含 `{}` 时格式化 | A:192-194 | 75 |
| `$NAME` | env[NAME] | A:195-196 | — |
| `@textf:PATH` | E.O(fact) | A:197-198 | 52 |
| `@text:JSON` | E.O(json) | A:199-200 | 221 |
| `fresh:SCOPE:KIND` | self.fresh(SCOPE)(KIND)，SCOPE 可格式化 | A:201-204 | 2439 |
| 其它 | fact 路径 `a.b.0`（列表用整数下标） | A:205, A:81-85 | — |

注：`@text:` 必须在 `@textf:` 之后检查（A:197-200 已如此）。mapseq 内另有动作级形式 `"$N"`、`["@out",T]`、`["@bytes",T]`、`{f}`（A:393-412, A:655）。

## 5. opts 键（43 个，按 A 中出现处；括号内为清单中次数）
- 行控制：`once`(3) A:252-257；`let`(314) A:258-261；`with`(115) A:287；`exit`(14) A:338；`result`(38) A:353-354；`export`(110，列表或 dict) A:292-294；`merge`(11) A:335；`flags`(50) A:333。
- 域与类：`classes`(216) A:271；`domain`(46，`[lo,hi]` 或 `"@labels"`) A:273-276；`domain_keys`(14) A:277；`domain_at`(5) A:279；`tokens`(5) A:281, A:633-638；`classmap`(82) A:283-286。
- 安装参数：`mode`(28)、`overlay`(4) A:321-322；`skip`(1)、`ordered`(1) A:329-330。
- 序列：`textrows`(16)、`bufrows`(1)、`msgrows`(1) A:596-615；`seqfact`(46) A:299；`seqlist`(162) A:301-307；`outseq`(12) A:309-314；`mapseq`(102) A:315, A:388-431, A:653-667；`seqenv`(40) A:317。
- 绑定（A:517-570）：`accumulate`(199)、`keep`(11)、`bindmap`(195，路径/列表/dict)、`freshrows`(209，`FILE@PART` 或规格列表：over/file/key/owner/kind/where/lookup/holder)、`cellsfirst`(87)。
- foreach（A:482-514）：`over`(95)、`as`(63)、`where`(13)、`join`(1)、`chain`(14：entry/start/next/result)、`pre`(38)。
- holder / fresh 表：`cur`(1) A:349；`cols`(8)、`where`、`scope`(1)、`owner`(1) A:374-386；`present`(2) A:345。

## 6. 求值顺序与作用域
1. 每行 facts = `dict(env)` ← 各 facts 表 ← foreach 外层绑定 `extra`（A:244-248）。env 为父 Run 的 env 本体（写入即全局可见）。
2. when 不成立即跳过（A:249）；once 已见则 `done=True` 结束本清单余下行（A:252-257, A:233）。
3. opts.let 写入行 facts（A:258-261）→ stem `@` 求值 → foreach 分派 → section → classes/domain/tokens/classmap → with（A:262-287）。
4. bindings（A:291, A:517-570）顺序：accumulate 旧值 → bindmap(路径) → freshrows(FILE@PART) → cellsfirst 时 bind 单元 → freshrows 列表 → bind 单元 → bindmap(dict)；accumulate 存回，keep 写 env。
5. export 立即写 env（A:292-294），先于 seq，所以同行 seq 可见。
6. seq 合并优先级：行 seq 单元 > seqrows > seqfact > seqlist；outseq、mapseq 覆盖其上；seqenv 最低（A:295-318）。
7. 执行 op；`result` 把返回值写 env（A:353-354）。call 的子 env 是拷贝，只有 merge 回流（A:333-336）；顶层 env 即 E.results（G:51-53）。
8. foreach：over 为 `$NAME` 时遍历 env dict（key/value，可 join）；否则 fact 路径（可格式化）；where 过滤；每元素先 pre 依序、再 chain.next，然后跑体；chain.result 写 env（A:482-514）。

## 7. facts 表格式
- 段式（A:57-78）：`=NAME<TAB>TYPE<TAB>VALUE` 标量；`@NAME<TAB>col:TYPE...` 表头，随后 `<TAB>v...` 行；TYPE ∈ str|int|json（A:47-54）。
- 表头式（L:1-25）：首行 `# col<TAB>col`，单元能 JSON 解析即解析；单列 `value` 返回值列表。经 A:460-476 载入为 `{STEM: rows}`；若列恰为 name/value，另给 `STEM!` dict 并把各 name 作为缺省 fact（A:471-475）。判别：首个非注释行不以 `=`/`@`/tab 开头即表头式（A:460-464）。
- 缓存按路径（A:59, A:76）。

## 8. 待删除或待改的特性（T1 下一步清单）
计数用 `re.findall` 对全部 132 个清单与 210 个 facts 文件逐文件统计（脚本见提交说明的复现命令）。

| 项 | 代码 | 清单 | facts |
|---|---|---|---|
| 其它 `.format` 路径（经 _fmt 的 `$$` 键、`fresh:` scope、foreach over/allowed/where、freshrows 已随 T1b 改用最小插值；仍直接调 str.format 的：foreach join default、mapseq、fresh 表 owner） | A:193, A:203, A:486-500, A:536-560, A:400-412, A:655-665, A:381 | 随宿主键计 | — |
| `=vN` 绑定（翻译器生成的匿名标量 fact，以裸路径 `k=vN` 引用） | A:68-69, A:205 | 2085 / 17（bitfields 350、functiontypes 276、librarydata 180、enumtypes 116 …） | 489 / 15（printfcontrol 282、k2-librarydata 61、bitfields 40、functiontypes 36 …） |
| 隐式顺序：`fresh:` 单元按行序/单元序分配 | A:201-204, A:207-214 | 2439 / 59 | 0 |
| 隐式顺序：`freshrows` 按行序分配 | A:524-561 | 209 / 41 | 0 |
| 隐式顺序：`once` 截断余下行 | A:252-257 | 3 / 3 | 0 |
| 隐式顺序：`ordered` | A:330 | 1 | 0 |
| facts 里残留的动作序列（单元含 `[["OP",…` JSON） | — | — | 2489 处 / 27 文件（pp-autoinc-gen 1096、lex-gen 432、printfcontrol 274、k2-control 213、k2-gen2 142、k2-call 92 …） |

T1e：上述冻结统计中的 `pp-autoinc-gen` 已由 1096 处迁到 0；动作模板位于 `exec/pp/autoinc-manifest.tsv` 的 `mapseq` 声明。facts 只带函数/宏/头文件名、依赖槽偏移和有序列表的序号/区间/末项标记，状态名和动作不再由 facts 生产者给出。13 个 pp 图哈希保持原值；未扩展 op、opts 键或值前缀。lex-gen 的 432 个输出动作序列也已降为 0，连同入口与 NSTART 的转义 JSON 动作副本迁回 `exec/lex/output-manifest.tsv` / `gen-template.tsv`。facts 保留 token 名字、拼写属性、字节分类与前缀树。五种 lex 图哈希不变。动态命名 mapseq 复用同一已有动作展开器，未增加 op、opts 键或值前缀。其余表仍待逐片迁移。
| facts 里的 `"exit"` 动作 | — | 15 / 5 | 9 / 3（lower-code 6、k2-gen2 2、returnwarnings-text 1） |
| 死数据：无清单或 py 引用的 facts 表 | — | — | 0（`nativeabi-ordered-regs.tsv` 已于 T1f 删除） |
| 死代码：`tape:` fresh、`fresh` op 名 | A:150, A:289 | `tape:` 0；op `fresh` 0 | — |

下一步建议顺序：getattr 已在 T1a 去掉（@stack 8、stackrows 2 改成 export.py 写出的 facts `k2-stack` 加 `@acts:`；graph 头改为 build/graph.py 的 GRAPHS 显式表），下一步把 `=vN` 换成具名 fact，再把 fresh 分配改成显式序号，最后把 facts 动作序列迁回模板表。

- 2026-10-04 T1b2：assemble.py 里剩下的 7 处 str.format（fresh owner、mapseq、foreach join default）也改用 _fmt，assemble.py 不再调用 str.format。

T1e 第三片：两架构 hostbridge guards、k2-errors、k2-gen2、k2-stack 的 facts 动作序列 197 → 0；动作归入既有模板/mapseq，facts 保留领域数据。E3 八变体与编码十变体整图哈希不变，DSL 仍为 9 op。
