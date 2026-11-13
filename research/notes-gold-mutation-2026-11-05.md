# 研究笔记：十八张 gold 表的变异实验（2026-11-05，供论文正文由项目所有者定稿）

方法：逐表改一键标签 → facts-gen 全量 + decide-gen → 42 个 recipe 全部重生网，与未变异基线比 sha256。
结论：
- 13 张 gold 表（pp lex type irsel enc reloc tyinfo regmap abi binsel prec opinfo peep）及 stmt、arity 两张决策表，改一键必改对应网——它们在七流出货路径上是真决策来源。
- 5 张（combo isel parse pfconv scope）改一键，42 张网全部不变——它们只是"决策网对照臂"：进 weights/built.json 与 kernel/*.inc，供 18 网经典批次与 gold 对照工具使用，不参与编译器管线。parse 的语句分派已由 stmt 决策表接替。
- 此前只读核对的判断（binsel 疑似死、irsel/reloc 死或半死）被实验推翻：三者都是活的。教训：表是否被读，要用变异定论，不能靠读引用。
- 实验暴露两个结构问题：regmap 改一个 x86_64 寄存器会让 x86_64 enc 网生成失败（生成器隐含假设寄存器映射是单射）；enc.tsv 的同一 form 被写进两份事实表。

论文里"十八张 gold 表"的表述需按此区分主脊与对照臂。
