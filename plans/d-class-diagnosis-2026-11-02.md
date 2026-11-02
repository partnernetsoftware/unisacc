# D 类 KNOWN-DEFECT 诊断缺口只读分析（c-negative 23 + pp-negative 2）

来源：材料 P0 会话（out/aw-m4-mat）D 类 2 个批次。产品对 C99 约束违反静默接受、
产出对象；宿主 gcc `-std=c99 -pedantic-errors` 全部 rc1。这些是真实产品缺陷（不是回归），
记 KNOWN-DEFECT，defects-c43.md「c.negative」开放，所有者 cdx/grk，领域 E3/parse2 模型域。

## 一、缺口清单与归属

### c-negative（23 处漏检，defects-c43.md 四轮累计）

类型系统 / 声明约束（parse2 事实表域，慢车道）：
- assign-to-const（6.5.16p2 可修改左值）、write-through-const-pointer、discards-const
- incompatible-pointer-init、int-to-pointer-init、distinct-pointer-compare、return-pointer-as-int
- conflicting-typedef、conflicting-function-types、duplicate-parameter、void-parameter-named、multiple-storage-classes
- struct-redefinition、local-redefinition、enumerator-redeclared、duplicate-case
- void-value-used、void-call-as-value、int-plus-assign-string、float-subscript

控制流 / 语法约束（parse2 事实表域，慢车道）：
- case-outside-switch、too-many-args-prototype、zero-array-size

### pp-negative（2 处，预处理域，慢车道）
- macro-name-not-identifier（宏名须为标识符）、header-name-unclosed（include 头名未闭合）

## 二、车道判定

全部 25 处属 **慢车道**（事实表 → 生成器 → 网），证据：
- 前端诊断域在 `exec/parse2/*.tsv`（body-manifest、control-reject、conversion-result、
  functiontypes、enumtypes、parse2-extra 等）+ `exec/facts/*.tsv`，经 `tools/facts-gen.cx`
  生成 `cx/compiler/p2ini.cx` 等代码；`cx/compiler/declname.cx` 也是 `tools/declname-gen.cx`
  生成的（「Generated … Do not edit by hand」）。
- 无一属可直接改 .cx 代码的快车道 bug：已正确拒绝的 20 项走的是同一 parse2 事实表域，
  漏检项是「约束行缺失/未覆盖」，不是「现有代码分支写错」。

## 三、成本排序（由易到难）

1. **pp-negative 2 处**（预处理）：宏名标识符、头名闭合，属 pp 模型域，约束最孤立、最便宜。
2. **类型一致性约束**（incompatible-pointer-init / int-to-pointer-init / distinct-pointer-compare /
   conflicting-typedef / conflicting-function-types / return-pointer-as-int）：functiontypes/
   enumtypes 事实表已有类型 token，加「赋值/初始化/比较双方类型兼容」约束行。
3. **const 族**（assign-to-const / write-through-const-pointer / discards-const）：需要类型表
   带 const 限定符位 + 可修改左值判定，依赖类型 token 已有 qualifier 位（functiontypes.tsv
   FPS_VAR/FPS_PARAM 等），成本中等。
4. **声明作用域族**（duplicate-parameter / local-redefinition / struct-redefinition /
   enumerator-redeclared / multiple-storage-classes / void-parameter-named）：需要作用域
   符号表重复判定，跨阶段（declname + parse2），成本较高。
5. **控制流/语法族**（case-outside-switch / duplicate-case / too-many-args-prototype /
   zero-array-size / float-subscript / void-value-used / void-call-as-value / int-plus-assign-string）：
   分散，逐个加控制约束，成本最高、最碎片。

## 四、修复属慢车道的前提

- 事实表只经「事实表 → 生成器（带 --check）→ 表」修改；交付物（生成器/规则/提案）进
  plans/ + tools/ 并提交。
- 快车道（改探针+sim --cov）不适用：这些是前端模型行为，不是运行库/接口 bug。
- 修复后需重跑 c-negative/pp-negative（hostcc 材料同源，无材料不同源干扰），并核对
  defects-c43.md 的四轮记录逐项转绿。

## 五、建议第一步（最小）

先做 pp-negative 2 处（最孤立），验证「事实表 → facts-gen → p2ini → 产品」这条慢车道
改动链完整走通、且 c-negative/pp-negative 能实跑验证；再按成本顺序推进 c-negative 的
类型一致性族（第 2 组），因为那一族覆盖的漏检最多、事实表已有基础。
