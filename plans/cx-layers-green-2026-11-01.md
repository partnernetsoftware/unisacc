# cx-layers 批次转绿：移除过期 A-INCOMPLETE

## 现象

`tests/run-batch.cx ... cx-layers` 返回 `status=FAIL rc=3`，但同一收据是 `up=0 failed=0`，四个 `undecided_*` 中只有 `undecided_lex=1`。这个 1 不是审计发现，而是 `cl_audit` 开头无条件打印并自增的：

```c
UNDECIDED A-INCOMPLETE lexical-12 and named-fixture increment not implemented
cl_und_lex++; cl_und_total++;
```

`lexical-12` 与 U/manifest 对账已经在同一个工具里接线，`--fixtures` 当前 `checks=40 failed=0`。该自增把“历史未完成文案”变成每次审计的假负债，是否 rc3 与真实边/对账结果无关。

## 修复

- 删除这条过期标记及其自增；不删除任何 `fx_note` 断言，不改 `up` 拒绝、白名单、分区和 `undecided_*` 的真实计数规则。
- `undecided_*` 仍由真实词法/U/manifest/IO 发现驱动；任一非零仍 rc3。
- `AMB/OUT` 仍打印 `UNKNOWN amb=... out=... not-a-pass`，不把无编号层边冒充通过；B 的 tracked/symlinks 仍 `UNKNOWN/NOT-DONE`。

## 证据

| 命令 | 结果 |
|---|---|
| `tools/check-cx-layers.cx` | rc0，`up=0 failed=0`，`amb=87 out=50`，`undecided_*=0` |
| `tools/check-cx-layers.cx --fixtures` | `checks=40 failed=0`，rc0 |
| `tests/run-batch.cx ... cx-layers` | `status=PASS rc=0 marker=CX_LAYERS_SCOPE` |
| `tools/affected.cx --files tools/check-cx-layers.cx --run` | `pass=1 fail=0` |
| `tools/ratchets.cx` | `ok=7 fail=0` |

## 为什么 gcc/clang 套件没有这个问题

宿主 `cxcc` 套件只负责“把单个 `.cx` 及其直接/传递 use 图展开、编译、运行”；它不是全仓分层审计，不读取 `tests/symindex` 的 manifest，也没有 `cx-layers` 这条收据。`cx-layers` 是 unisacc 侧对 1631 个 manifest 源文件的静态直接边审计，属于另一条合同；宿主套件通过并不能替代它，反过来它失败也不是宿主编译回归。

## 仍未声称

- A 合同 §14.2 的六个扩展夹具（digest/aba、missing/orphan shard、manifest escape/path-too-long 端到端、`.git/index` 自检）**未新增**；本次没有把 `checks` 数改成 46，也不声称它们已覆盖。
- `coverage=manifest-only`、`symlinks=UNKNOWN`、`tracked_reconciliation=NOT-DONE(B)` 原样保留。
- rc0 只表示现有编号层断言通过，不授予全仓/tracked 分层资格。
