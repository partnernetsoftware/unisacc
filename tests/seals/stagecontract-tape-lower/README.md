# 第二边界夹具：Tape 验域 → lower（stagecontract-tape-lower）

- 产品：HEAD 干净导出 `unisacc-next.com`，sha256 `83e6e7e4…dafb`，包 `compiler.pkg` sha256 `71832a39…d67`（与 stagecontract-e1-e3 同一产品）。
- 捕获：`tests/middleflowrunner.c` 的 `capture-lower`，源 `tests/seals/middleflow-step0/accept-source.bytes`，期望 0（accept）。路由：E1 → e3 → prune（Tape 验域后的 UNITOK 帧）→ lower（lnx/x86_64/image/O0）。`input.bytes` 即 lower 的输入。
- 与 `middleflow-step0/lower/accept` 的比较：input、output、attributes、source-label 逐字节相同；本捕获多两行 absent（`data-latebind`、`latebind`），HEAD 产品请求了它们，旧的 lower 夹具没有。因此没有沿用旧夹具，而是用 HEAD 包重新捕获。
- 冻结模型 `lower.net`：由 `tests/modelstreamextract.cx` 从 HEAD 产品原样抽取，sha256 `55e70758…6be6883d`。根产品 `./unisacc.com` 的 lower 是另一个 `bbdb5695…`，所以本夹具只能与 HEAD 产品一起使用。
- 资源 ledger 只有 absent 行；边界键取 `library/module`（`006c6962726172792f6d6f64756c65`，absent，且捕获请求了它）。

## 检查结果（HEAD 产品，`stagecontractcheck.cx --fixture tests/seals/stagecontract-e1-e3 --fixture tests/seals/stagecontract-tape-lower`）

- S-POS-01、S-NEG-RESOURCE、S-NEG-RESET、S-NEG-DUP（rc 2，名 key）、S-NEG-FRAME（rc 2）、S-NEG-MODEL（rc 2）：全部通过。
- S-NEG-ABSENT：UNKNOWN，rc 0，stdout 1119 字节。
- S-NEG-EMPTY：UNKNOWN，rc 1，诊断 `reject: not covered: lnx/x86_64 lowering`。**与 ABSENT 不同**：lower 阶段区分了“存在且 0 字节”和“显式不存在”。这是 lower 阶段的拒绝（它不覆盖这种输入），不是回放适配器的错误，因此未判红，也未声称别的语义。

## 与 e1-e3 的关系

两个边界共用一份 `stagecontractcheck.cx` 和同一个回放器；差别只在 `boundary.tsv`（模型文件名、sha256、边界键、标签）和 `accept/`、冻结模型。e1-e3 的 `boundary.tsv` 同样只描述这些值。

## LDP 三边修复后重封（当前）

`de87f540` 将 LDP 的三个未定义目标改指既有 DEAD。用仓内 `networkformat.decode` 对真实旧/新产品网络逐行归因：3 个边目标变化，两个未引用空态删除，4397→4395 与 stack 上界4396→4394；S/Q/C、动作、字符串与其余状态不变。旧资料与 Python 另一构造路径的反事实记录保留，不以其物理字节作产品网络等价证明。

本次产品 SHA256 `305f5948802ebaef32ce866086f56d218b12ba2d0f6adc60e52d2d785670f303`；实际提取的新 lower SHA256 `9b7416b5b6ced06846af9c08544a505c0805fb449351bdfe306e4574d36451ea`。使用既有 `tests/modelstreamextract.cx` 提取产品原字节；`tests/middleflowrunner.c` 与 `exec/c/core.c` 按既有 capture-lower 入口重采同源包和 accept-source，六文件（input/output/status/attributes/source-label/resources）全与旧 accept 逐字节相同，故不重写它们。boundary 的 sha256 行由提取文件计算生成，不手填摘要。`middleflow-step0` 的旧模型是历史原件，不覆写。

复采步骤（从仓根运行，PRODUCT/PACKAGE 为同一次构建；OUT 为新目录，每条加 `python3 tests/bound.py 55`）：

```sh
cc -O2 tests/middleflowrunner.c exec/c/core.c -o "$OUT/runner"
"$OUT/runner" capture-lower "$PACKAGE" tests/seals/middleflow-step0/accept-source.bytes "$OUT/accept" 0
sh "$PRODUCT" -run tests/modelstreamextract.cx -- "$PRODUCT" "$OUT/models"
```

先创建 OUT/accept 与 OUT/models；比较 accept 全文件后，封存模型取 models/lower.net 原件，pin 从该文件的 SHA256 自动生成。复核仍用 `tests/stagecontractcheck.cx --fixture tests/seals/stagecontract-e1-e3 --fixture tests/seals/stagecontract-tape-lower --product PRODUCT`，不放宽任何正负断言。
- 2026-10-22 重捕：HEAD 59bf6258 的 lower 表刀（dff28ff6 CRLF B-lower、6f717a98 托管 .sys6 exit）之后冻结 lower.net 已陈旧（HEAD 干净导出包与工作树候选包抽出的 lower.net 同为 3f4c80e2500315ddf9db4086e7bd7bffab0ac6c5944186a1e391581d71ea3ad5，均≠旧冻结）；lower.net 经 modelstreamextract.cx 原样抽取，accept/ 以 middleflowrunner capture-lower 重捕，六个文件与旧捕获逐字节同。
