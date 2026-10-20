# 0.0.43 tip 字节差集（r1，2026-10-20 13:37）

对照：基线产品 = 根包 `unisacc.com`（sha256 `0e0531e3…f8`，构建元数据 commit `dffc5eb2`）；候选 = 当前 tip `fac46869` 的干净副本构建（sha256 `ffdc4622…49`）。两者都只做只读编译：`-E`、`-S`、`-c`、`-run`，记录返回码与输出/stderr 的哈希。

## 结果

| 集合 | 行数 | 差异 |
|---|---|---|
| 语料 `tests/c` + `examples`（305 个 .c × E/S/c） | 915 | 0 |
| 修复刀输入（33 个 .c，含 14 个 -include 头，× E/S/c/run） | 132 | 0 |

## 为什么两者在语料上一致

`dffc5eb2..fac46869` 之间，C 产品源码（`src/`、`exec/c/`、`include/`、`seed/`）没有改动。改动的只有：

- 模型表 `exec/parse2/*.tsv`、`exec/facts/*.tsv`（生成的网络输入）；
- Python 参考 `unisa/front/parse.py`；
- 测试与门禁声明文件。

模型表改动是两条"拒绝"修复，而不是字节层面的功能变化。语料与修复刀输入没有覆盖它们的分支，所以语料一致并不证明这两条修复无副作用，只说明它们没有影响这些输入。

## 实际差异（语料之外单独验证）

1. **空声明（b5cded8d）**：文件作用域 `int ;`：基线接受（rc0），候选拒绝 `not covered: declarator`（rc1，具名）。宿主 `cc -pedantic-errors` 拒绝（"useless type name in empty declaration"）。候选正确；拒绝诊断文字与参考 C 实现的 `empty declaration` 不同，是诊断措辞差异。
2. **裸 return（b4d00269）**：`int f(void){ return; }`：基线接受（rc0），候选拒绝 `not covered: return with no value`（rc1，具名）。C 参考实现 `src/front_parse.c:5814` 一直拒绝同一情形。候选与参考一致，基线的接受是缺陷。

## 仍存在的缺口（两者都接受，非回归）

- 块作用域 `int ;`（函数体内）：基线与候选都接受（rc0）。宿主 `cc -pedantic-errors` 拒绝。这是既有缺口，候选刀的下一项。

## 局限

- 只比较了返回码与输出哈希，没有比较目标文件以外的全部中间形式；
- 模型表的其余行（`global3-common` 的新分支）没有对应的单独输入；
- 未测其它目标平台。

## 候选刀：块作用域空声明（未实施）

位置（只读定位，见 `/home/box/c43/0043-diff-r1/block-empty-decl.md`）：
- C 参考：`src/front_parse.c:5326-5329`（块作用域）与 `:6135-6137`（文件作用域）都在 `bare_decl_ok()` 为假时报 `empty declaration`；
- 模型：`exec/parse2/control-result.tsv` 的 `local0-common` 段，第 1665 行把裸类型后的 `;` 直接送到 `S.empty`，没有检查；
- 文件作用域的对应修复（b5cded8d）在 `global3-common` 段把 `FN.semi * NEXT` 拆成四行，检查 `type_enum` 与 `rb == SBB`，拒绝经 `global_reject6`。

不能直接改 `S.empty`：第 227 行的空语句 `;` 也用它。需要的是第 1665 行改指向新的检查状态。

未验证：`type_enum` 与 `tb` 在该状态下的取值；typedef、union、`enum E;`；匿名结构体（文件作用域修复会接受 `struct {int a;};`，块作用域会继承该行为）；`tests/gatedeps.json` 与 `tests/seedmemory.py` 的哈希刷新；`exec/prune/prune-result.tsv` 与 `exec/modelsignature-result.tsv` 是否需要重生成。

本窗口不实施：需要表改动、产品重建、门禁套件、gatedeps 刷新，以及与 `JOBS=1` 同样的 commitgate 声明问题。
