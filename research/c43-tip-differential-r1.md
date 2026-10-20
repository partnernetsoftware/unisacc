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
