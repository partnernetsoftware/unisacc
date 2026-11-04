# use-unitcount-2026-11-04

只读分析，未改代码。

## 问题

whole-o（`osx`/`win` 路线）一次把 `rb_whole_src[33]` 的全部源码交给编译器，展开后单元数超过 e3 帧上限（今天 64，`9c8c45f366` 后源码侧说 128 但产品仍 64）。
止损备选是「把 33 源拆成两份，每份 ≤64 单元」。这份文档回答：拆得动吗。

## 33 源各自声明的 use

| use 条数 | 文件 |
|---|---|
| 5 | `src/runtime/runexec.cx` |
| 4 | `src/driveroute.cx` |
| 3 | `src/packmodel.cx`、`src/host.cx`、`src/compiler/cxmod.cx` |
| 2 | `src/unisacc.cx`、`src/runtime/model_decode.cx`、`src/executor.cx`、`src/driver/input.cx`、`src/driveface.cx` |
| 1 | `src/stages.cx`、`src/runtime/routestep.cx` 等 |
| 0 | 其余（含 `src/runtime/stagehandoff.cx`、`src/bind.cx`、`src/entrybind.cx`、`src/hostnative.cx` 等） |

合计 **31 条 `#pragma unisa use`**。

## 关键结论

**33 源 + 31 条 use ≠ 实际单元数。** 单元数取决于被 use 模块的**传递闭包**：`use a.b` 会把 `a/b.cx` 当成一个单元，而 `a/b.cx` 自己还有 use，继续展开。所以「各自 use 条数」只能给出上界方向，不能给出精确值。

要给出精确数字必须真的展开——而展开就是我正在跑的 ctx20 那轮（它证明不了 unitcount，因为编译器不报单元数）。

## 「拆两份 ≤64」可行性判断

**不可行，理由三条：**

1. **闭包高度共享**：`sys.*`（`io`/`fs`/`mem`/`posix`/`kernel`）几乎是所有源的共同依赖，`runtime.*` 与 `compiler/*` 也被多个源引用。两份必然各自带上同一批底层模块，**两份之和 > 原来的一份**，不是把 64 拆成 32+32。
2. **单元上限是每帧的**：拆两份意味着两次 `-b` 编译再链接，但这两份之间要互相调用（`src/unisacc.cx` 的 main 调 `drive.*`，`drive.*` 调 `runtime.*`），跨帧调用需要显式符号表/外部单元，产品现在没有这个机制。
3. **今天已经有 67 对象的对象路线**（`release-build.cx` 的 `entry-build`），它已经解决了「33 源不能一次喂」的问题，代价是 67 > 64 —— 这就是 UNITOK2-CAP 存在的理由。

## 建议

**不要做「拆两份」。** 正确的修法是调高每帧单元上限（产品侧把 64 改成 128），这正是 `9c8c45f366` 在做的事；`tests/unit-boundary.cx` 已经把它变成可验收的（65/127/128 现在是 KNOWN-RED，放开后应转 PASS）。

拆两份如果仍要做，前置是：
- 先有跨帧单元导入机制（现在没有）；
- 再统计真实闭包大小（需要编译器提供 `--unit-count` 一类输出，现在没有）。

## 未测量项

- 真实闭包单元数（编译器无此输出）
- `runtime.*` / `compiler.*` 被引用的精确次数（只统计了直接 use，未做传递展开）
