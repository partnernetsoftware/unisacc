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

**33 源 + 31 条 use ≠ 实际单元数。** 单元数取决于被 use 模块的**传递闭包**：`use a.b` 会把 `a/b.cx` 当成一个单元，而 `a/b.cx` 自己还有 use，继续展开。所以「各自 use 条数」只能给出上界方向，不能给出精确值。精确值用 `-cx-plan` 实测，见下节。

## 实测：33 根展开后 = 72 单元（2026-11-04 补）

权威口径是产品自带的 `-cx-plan`，不用造工具、不改产品：

```
sh ./unisacc.com -cx-plan <rb_whole_src[33]>
```

`src/driveplan.cx:31` 的 `dv_cx_flags` 走 `-cx-plan` 时调 `cxm_plan` + `cxm_planprint`，与
`dv_expand`（`driveplan.cx:89` 拿 `cxm_count()` 和 `DV_MAXU` 比的那个 `n`）是**同一套展开**。
输出 72 行、唯一路径 72 个 → **33 根展开后 72 个单元**。72 > 旧 `DV_MAXU=64`，`9c8c45f366`
提到 128 后 72 < 128 可过。72 已跨过 65，65 源修好前 osx/win whole-o 必然红（INSTALL44 关键路径）。

逐根单独 `-cx-plan` 的闭包单元数（顺序同 `rb_whole_src`）：

| # | 根 | 闭包 | # | 根 | 闭包 | # | 根 | 闭包 |
|---|---|---|---|---|---|---|---|---|
| 1 | unisacc.cx | 42 | 12 | deps.cx | 1 | 23 | executor.cx | 9 |
| 2 | host.cx | 4 | 13 | input.cx | 3 | 24 | transition.cx | 1 |
| 3 | stages.cx | 2 | 14 | args.cx | 1 | 25 | codec.cx | 1 |
| 4 | routestep.cx | 8 | 15 | path.cx | 1 | 26 | model_decode.cx | 2 |
| 5 | stagehandoff.cx | 1 | 16 | entrybind.cx | 1 | 27 | model_number.cx | 1 |
| 6 | runexec.cx | 8 | 17 | memlayout.cx | 1 | 28 | packmodel.cx | 5 |
| 7 | attribute_frame.cx | 1 | 18 | driveface.cx | 3 | 29 | modelload.cx | 1 |
| 8 | bind.cx | 1 | 19 | driveroute.cx | 8 | 30 | bindstage.cx | 1 |
| 9 | cli.cx | 1 | 20 | cxmod.cx | 1 | 31 | entryexec.cx | 1 |
| 10 | compat.cx | 1 | 21 | tapegate.cx | 2 | 32 | hostnative.cx | 1 |
| 11 | route.cx | 1 | 22 | fwdstub.cx | 1 | 33 | tapelink.cx | 1 |

逐根闭包之和 = 117。**不要把逐根并集当权威值**：逐根时 use 解析到 `src/runtime/*`、`src/sys/*`，
33 根一起时解析到 `src/driver/*`（同 basename 不同目录），逐根唯一路径并集是 79，与 72 口径不同。
要拿去和 `DV_MAXU` 比的只有 72。

**定量否决「拆两份 ≤64」**：最大单根 `unisacc.cx` 闭包就 42；`routestep`/`runexec`/`driveroute`
各 8、`executor` 9，且这些闭包高度共享、解析随根集合变化，没有跨帧单元导入机制 → 切不出两个
自洽且各自 ≤64 的闭包。67 对象的对象路由同理超旧 64。另两条定性理由仍成立：

1. **闭包高度共享**：`sys.*`、`runtime.*`、`compiler/*` 被多源共用，两份之和 > 原来一份，不是 64 = 32+32。
2. **单元上限是每帧的**：拆两份要两次 `-b` 再链接，跨帧互调需显式符号表/外部单元，产品现在没有。

## 建议

**不要做「拆两份」。** 正确的修法是调高每帧单元上限（产品侧把 64 改成 128），这正是 `9c8c45f366` 在做的事；`tests/unit-boundary.cx` 已经把它变成可验收的（65/127/128 现在是 KNOWN-RED，放开后应转 PASS）。

拆两份如果仍要做，前置是：

- 先有跨帧单元导入机制（现在没有）；
- 真实闭包大小已可测（`-cx-plan`，72 单元）。
