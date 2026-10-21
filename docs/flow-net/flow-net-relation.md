# 流×网关系总表

- tip `70f61992` · 观测 2026-10-21 10:59 AWST
- 真源：`image-stages.tsv`、`gold.STAGES`、`tests/STAGE-IDENTITIES.md`、经典 ask/`inf` 位点、`compilerpack` 路由
- **勿混**：7 流 = 字节管道；18 网 = ask 事实表；同名不可互换。

## 图例

| 符号 | 含义 |
|------|------|
| **✓** | 主消费（出货主脊该段主要承载/查询） |
| **◐** | 次要/旁路（跨段 ask、构造旁路、或物理未拆的次归属） |
| **·** | 不相关 |
| **⊘** | 有金表但不在出货主脊 ask（对照臂） |

## 1) 关系总表（行=7流，列=18网）

| 流＼网 | pp | lex | parse | type | scope | irsel | enc | reloc | tyinfo | pfconv | regmap | isel | abi | combo | binsel | prec | opinfo | peep |
|--------|:--:|:---:|:-----:|:----:|:-----:|:-----:|:---:|:-----:|:------:|:------:|:------:|:----:|:---:|:-----:|:------:|:----:|:------:|:----:|
| **e2** | ✓ | · | · | · | · | · | · | · | · | · | · | ⊘ | · | ⊘ | · | · | · | · |
| **e1** | · | ✓ | · | · | · | · | · | · | · | · | · | ⊘ | · | ⊘ | · | · | · | · |
| **e3** | · | · | ✓ | ✓ | ✓ | ✓ | · | · | ✓ | ✓ | · | ⊘ | · | ⊘ | ✓ | ✓ | · | · |
| **e4** | · | · | · | · | · | · | · | · | · | · | · | ⊘ | · | ⊘ | · | · | ✓ | ✓ |
| **prune** | · | · | · | · | · | · | · | · | · | · | · | ⊘ | · | ⊘ | · | · | · | · |
| **lower** | · | · | · | · | · | · | ◐ | ✓ | · | · | ✓ | ⊘ | ✓ | ⊘ | · | · | · | · |
| **elf** | · | · | · | · | · | · | ✓ | · | · | · | · | ⊘ | · | ⊘ | · | · | · | · |

## 2) 白话：每流主要网

| 流 | 主要网 | 可能旁路 | 白话 |
|----|--------|----------|------|
| **e2** | pp | — | 预处理管道；指令/条件决策查 pp 表。 |
| **e1** | lex | 构造期旁看 parse.tsv 的 tok 域（非运行时消费 parse 网） | 词法管道；最长匹配/类别查 lex。 |
| **e3** | parse, type, scope, irsel, tyinfo, pfconv, binsel, prec | — | **一大段未拆**：tokens→tape；语法/类型/作用域/IR/优先级等 ask 全嵌此物理网。 |
| **e4** | opinfo, peep | — | tape 上 peephole / 属性驱动改写。 |
| **prune** | （无 18 网） | — | 按导出/根剪函数；**不查金表**，政策在代码。 |
| **lower** | abi, regmap, reloc | **enc**（经典 lowering 问 form；产品编码段在 elf） | ABI/寄存器/重定位；enc form 经典侧顺带问。 |
| **elf** | enc（+ image 写出，无独立金网名） | — | 编码+容器；STAGE-IDENTITIES 把 enc/image 钉在 elf。 |

## 3) 边级「有无夹具」（流×网 主边）

口径：夹具 = `tests/seals/` 点名覆盖该**流段**或该**网**的原件；流级封≠网级叶封。有封 ≠ 已冻；可标 STALE。

| 边（流→网） | 关系 | 有无夹具 | 路径简述 |
|-------------|------|----------|----------|
| e2 → pp | ✓ | **有（网级叶 + 流侧）** | `tests/seals/exec-pp-literals/`；STALE风险（头/autoinc） |
| e1 → lex | ✓ | **无** | stagecontract-e1-e3 不含 e1；earlystageseal 不验 E1/lex |
| e3 → parse/type/scope/irsel/tyinfo/pfconv/binsel/prec | ✓（嵌） | **有（流级，非按网点名）** | `stagecontract-e1-e3/`、`middleflow-step0/e3/`、`librarycallbacke3/`；金网无一单独叶封 |
| e4 → opinfo, peep | ✓ | **无** | 无 e4/opt 点名封 |
| prune → ∅ | · | **无** | 无网可封；无 prune.net 夹具 |
| lower → abi, regmap, reloc | ✓ | **有（流级，非按网点名）** | `stagecontract-tape-lower/`、`middleflow-step0/lower/`、`library*lower/`；STALE风险；abi/regmap/reloc 无网级叶封 |
| lower → enc | ◐ | **无（enc 无点名）** | 经典 ask 在 lower；身份合同属 elf |
| elf → enc | ✓ | **无** | enc/image 集成于 elf；无 elf/enc 点名封 |
| * → isel / combo | ⊘ | **无** | 对照臂；research `seal-combo-*` ≠ 金网夹具 |

### 脚注

1. **流级封 ≠ 网级封**：e3/lower 有阶段合同封，只证明该段管道原件存在，**不**等于嵌在内的每张金网已点名冻结。
2. **唯一网级叶封**：目前仅 **pp**（`exec-pp-literals`）。
3. **isel/combo**：矩阵列上对所有流出货主脊标 ⊘（对照/不问或 off ship）；不单独为每行再写夹具行。

## 4) 物理嵌套（勿读成路由扇出）

- **e3 ⊃** {parse, type, scope, irsel, tyinfo, pfconv, binsel, prec} — 一段流嵌多张网。
- **elf ⊃** {enc, image} — image 不是 18 金网之一。
- **e4 ⊃** {opinfo, peep} — 一段两网，仍未拆独立 image-stage。
- **近 1:1**：e2↔pp，e1↔lex；**prune → ∅**；**lower** 对 abi/regmap/reloc 为一对多路由式。
