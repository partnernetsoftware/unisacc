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
