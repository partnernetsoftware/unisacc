# 晚绑定 2c2 第0步｜固定签名函数（fixed typed）最小下一刀（预稿，只是设计，2026-10-15）

**证据边界**：以 `research/c43-late-binding-cut2c-step0.md` 的拆分顺序为准（数据 → fixed typed → variadic → callback → 运行服务与上下文）。本页只定 2c2 的范围、文件域、探针与预算，未改实现、未跑验收。

## 1. 选择

**2c2 = fixed typed，不含 variadic、callback、运行桥。** 理由：
- 它是拆分顺序里紧随数据符号（2c1）的下一项，且不需要 callsite 帧、回调图或服务上下文，改动面最小。
- 只新增一个“签名类别”（`callkind=1`）与固定签名块（`USLSIG3`），E3 只输出计划需求与 ABI 证书，原生计划在绑定之后实例化。
- 前置假设（须在开工前核对）：2c1 的数据符号 kind=1 已落地，USLCON1 的版本化字段可以沿用。

## 2. 文件域（避开 cdx 在途文件）

- **合同与签名**：`exec/c/librarycontract.h`（合同字段与类别）、`exec/c/librarysignature-v3.md`（签名规范，文档）。
- **解析与计划**：`exec/c/libraryresolver.h:128–144` 目前在编译期用真实目标地址创建 plan/template；2c2 只改为“记录签名需求”，实例化推迟到绑定之后。
- **新探针**：新增 `tests/libraryfixedtypedcheck.py`（新文件，不与 `tests/libunisaccresolvercheck.py` 共用）。
- **不碰**：`exec/c/libunisacc.c`、`exec/c/libunisacc.h`、`exec/c/librarylatebind.h`、`exec/c/libraryrelocate.h`、`tests/libunisaccdatacheck.py`、`tests/librarydata*`、`unisa/librarydatalower.py`、`prd.md`、`spec.md`（均为 cdx 在途或不归本步）。

## 3. 探针组（每组一次 `tests/bound 58`）

- **P1 接受**：固定签名（整型参数、整型返回）的函数；合同生成 `callkind=1`、`nargs`、签名块与 ABI 证书；计划需求与证书一致。
- **P2 拒绝：实参个数不符**：`nargs` 与调用点不一致，以具名原因拒绝，不产生计划。
- **P3 拒绝：未知签名类别**：`callkind` 取未声明值，具名拒绝。
- **P4 拒绝：缺 `nargs`**：`callkind=1` 无参数个数，拒绝。
- **P5 未绑定不实例化**：绑定缺失时计划不得实例化，结果为拒绝，不回退到地址推断。
- **P6 无回调回归**：不含 typed 函数的程序，输出与改动前逐字节一致。

## 4. 预算与验收规则

- 每步 ≤58 秒（`tests/bound 58`），探针组各自独立计时；超时按失败记，不放宽。
- 验收遵循既有规则：合同从全部声明生成；分区只看 kind、callkind 与版本；**地址缺失不改归属**；新旧双归属或缺归属拒绝。
- 不得用 dlsym 成功与否或地址非零来判断 fixed typed 的归属。

## 5. 之后的顺序（只列，不在本步）

2c3 variadic（`callkind=2`，固定参数数与 promoted-tail 类型，callsite 帧）→ 2c4 callback（`callkind=3`，嵌套签名图与生命周期）→ 2c5 运行服务与上下文（exit/mmap/callable，与 mapping/argv 等实例状态分开，见 2c 第0步第3节）。

## 6. 待 cc 定

1. 确认 2c2 范围仅为 fixed typed（不含 callback、variadic）。
2. 确认 2c1 数据符号已落地，作为 2c2 的前置。
3. 确认 P6 的“逐字节回归”基线取自哪一个提交。
