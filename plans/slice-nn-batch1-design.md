# 切片迁 NN 批次 1:生成器设计与重量分工(0.0.104,只设计,不动代码)

状态:设计稿。依据 `plans/slice-nn-migration.md` 批次 1(5 单元)与 `archive/docs-2026-11/plans/rtsharedname-nn-pilot.md`。
写域约束:`cx/compiler/rtsharedname.cx` 与 0.0.101 共享状态接线写域相交,接线落地前不改任何代码;本页只定重量口径与生成器合同。
重量数据取 omp-grk 那份,本页不另造数;对不上的列标 UNMEASURED。

## 1. 批次 1 五单元的实际可迁性(读源后的修正)

清单把 5 个都标 FINITE,读源后只有前三个键域真有限:

| 单元 | 源 | 询问 | 键域 | 结论 |
|---|---|---|---|---|
| rtsharedname | `cx/compiler/rtsharedname.cx`(由 `rt-shared-gen.cx` 生成) | count / at(i) / covers(i,name) / used(i,name) | i 与表内名全集 | 有限,但 name 是串键,需定长垫 |
| tapeshape | `cx/compiler/tapeshape.cx` | name(op) / shape(op),op 0..71 | 72 个 op | 有限,最干净 |
| cellname | `cx/compiler/cellname.cx` | count / at(i),i 0..5 | 6 个 cell | 有限,最小 |
| driveargs | `src/driveargs.cx` | `dv_word(word,is_object)` 调 `bx_tool`/`bx_cli_word`/`bx_more_src` | 命令行词:无界串 | 不是有限键;`dv_parse_args` 还持 opts/compat 指针与回调,属 MIXED |
| model_number | `cx/runtime/model_number.cx` | tag / hex / budget / number | 字节流解析,budget 十进制任意长 | 不是有限键;是解析器,hex 一个小函数可表化 |

决定:批次 1 先落前三个(合计 .text 约 9405+8435+3976);driveargs 的 `dv_word` 等 entrybind 的三个分类函数先迁表后再谈,`dv_parse_args` 留壳;model_number 只迁 `cxmn_hex`,其余按批次 2 的"拆分后迁"。此裁剪写回 `slice-units.tsv` 显式覆盖行,不改批次号表里的数字,由 `slice-migration-gen --check` 复核。

## 2. 通用"表到网"生成器合同(`tools/table-net-gen.cx`,待建,带 `--check`)

输入:
- 事实表(现有 `exec/facts/*.tsv`:`rt-shared.tsv`、`tape-shape.tsv`、`pp-targets.tsv`)。
- 一份询问规格 `exec/facts/ask-<unit>.tsv`:每行 `ask<TAB>in_cols<TAB>out_cols<TAB>reject_code<TAB>reject_when`,把 count/at/covers 这类询问写成"输入字节 -> 定长输出字段 + 状态码"。

输出(三份,缺一不可):
1. 网权重:一热阈值网,输入 1 字节索引(串键先按表内最大名长定长垫,垫长写进规格),输出定长字段加 1 字节状态码。
2. 金标全键穷举表:由事实表枚举出的键域加越界哨兵(i=2、255;缺指针;表外名),每键一行 `key<TAB>expected_bytes`。
3. C 参考实现(现生成物原样保留为 oracle):迁移后不进切片,只在检查里与网逐字段 cmp。

`--check`:重算三份输出与库内逐字节比;事实表或规格改动而生成物未重算则非零。

拒绝合同照抄现函数,逐条进规格:空指针 4、索引越界 2、坏索引计数 -1;网侧以状态码字节返回,薄壳把状态码映回原整数返回值,壳不超过一个函数一行。

## 3. 验收(沿用试点页,不放宽)

- 全键穷举与 C 参考逐字段、逐返回码相等,键数口径:rtsharedname 约 80,tapeshape 72*2 + 越界,cellname 6*1 + 越界。
- 判定门不变:仅当 `Δpkg < Δslice` 且 `Δslice > 0` 才合入;任一量缺失标 UNMEASURED,保留生成 C 表。
- 回收上界写对象 `.text`,不写 APE 下降;网增量待权重构造后量。
- 执行器合同未门禁(OOM 为 panic)的事实写进每份收据。

## 4. 分工与顺序

| 步 | 内容 | 谁 | 前置 |
|---|---|---|---|
| 1 | 重量数据(三单元的网权重、`Δpkg`) | omp-grk | 无 |
| 2 | `table-net-gen.cx` 设计细化到规格列、`--check` 断言清单 | omp-sonnet(本页) | 无 |
| 3 | tapeshape 与 cellname 先落(不碰 rtsharedname 写域) | 待派 | 步 1、2 |
| 4 | rtsharedname 落地 | 待派 | 0.0.101 共享状态接线落地 |

对齐点(与 omp-grk 核):规格列的输出定长字段宽度、串键垫长、状态码字节编码,三项以他的权重为准,本页随之改。

## 5. 风险

- 把 driveargs、model_number 硬塞"有限键"会得到不等价的网(无界串键);已在 §1 排除。
- rtsharedname 的 `rts_covers` 现用逐字符比较循环,网侧键为定长垫串,表外名与表内名前缀相同的情形必须进金标(如 `_unisa_atexit` 对 `_unisa_natexit`)。
- 切片体积下降是结果不是判据;`Δslice` 以同候选五片分列量取。

## 6. 规格列实测口径(omp-grk 对齐后,事实表量名长)

| 单元 | 输入 | 垫长 | 状态码 | 备注 |
|---|---|---|---|---|
| rtsharedname | i 1 字节;covers/used 名 | 15(表内最长 `_unisa_sigflags`,但现生成源中的最长字面量是 14 的 `_unisa_envheap`,`_unisa_sigflags` 不在 covers/used 字面量里,多垫 1 字节无害,以事实表为准) | 0/2/4 | 输出定长字段宽度 UNMEASURED |
| tapeshape | op 1 字节整数 0..71(不是名) | 输出名垫 12(`.librarycall`);输出 shape 串垫 7(源里 `rrii` 等多字母类,不是 1) | 0/2/4(空指针不在此单元合同,越界返回 0 指针,状态码 2) | 输出定长 UNMEASURED |
| cellname | i 1 字节 0..5 | 输出 cell 串垫 10(`lnx/x86_64`) | 0/2/4 | - |

更正:omp-grk 来信写 shape 串垫长 1 与 op 名作输入,实测 shape 最长 7、输入为 op 序号;`tape_shape*` 现函数越界返回空指针而无状态码 4,状态码语义由壳映回空指针。
Δpkg、Δslice、.text 基线均 UNMEASURED,待 099 预跑后另量。
