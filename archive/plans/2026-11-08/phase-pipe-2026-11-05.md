# lab-loader 转正为日常开发入口（设计一页，2026-11-05）

来源：主人方案（loader → 各阶段 .com 可管道串接 → 全过再整合）。cc 指出已有现成实验，不重做：
`tools/lab-loader`（L1 `slice.cx` 产 `slice/<s>/program.com` 七个单流，与 stream-pack 逐字节相同，三段样例七流管道 21/21 段同；
L3 boot 头带 sha 校验；L5 单 NN 模板；批次 slice-check 等在跑）与 `tools/cx-pipeline.cx`（显式串七流，限 lnx/x86_64，multi-unit 走 units）。
对应 0.0.45 的 C45-LOADER-01、C45-STAGEBENCH-01（plans/v0.0.45.md:15/17），本设计把 STAGEBENCH 提前并入 0.0.44 的 STAGE-LAYOUT/FAST-2。不插队 INSTALL44。

## 1. 为什么实验没成为日常入口（今晚查 65 源没人用，反而另写了 stagecase）

| 缺口 | 证据 |
|---|---|
| 入口不在开发路径上：要先手工 `slice.cx BASE_COM SOURCE_PKG OUT` 产出 program.com，输入是**已打好的包**；改一张表后得先走 pack 才有新 SOURCE_PKG——正是 956~1006 秒那条慢路 | slice.cx:3/109 用法；STAGECASE ledger 基线 |
| `cx-pipeline` 吃 CX0BND1 bundle 与 adapter 参数，不吃普通源文件列表与产品的真实阶段输入 | cx-pipeline.cx:1-3 |
| 没有拒绝定位：单流 rc1 时只有网的 bad 串，看不出状态/单元——今晚全靠 sim --trace | FAST-1 |
| 只覆盖 lnx/x86_64；不含 units 之外的多单元成帧与转发桩路径 | cx-pipeline.cx:2 |
| 实验线自述「不改正式产品、产品身份不经过这里」，没人被要求在开发时用它 | README.md:3 |

## 2. 转正后的入口

```
unisacc-phase <stage> [--target T] [--resource NAME=FILE]... < in > out       # 即 slice/<stage>/program.com
unisacc-phase --dev <stage> ...                                                 # FAST-2：网由工作树表现场生成（按表 sha 缓存）
```
- 阶段名取 `exec/pipeline/image-stages.tsv` 七行加 `units`；产物仍由 slice.cx 生成，与主产品共用 loader/网。
- `--dev`：不经 pack，按 stagecase 已实现的「tool-gen 现场产网、按 tool sha+argv+exec 树摘要缓存」装载当前表。这是 FAST-2 的 ≤60 秒指标的实现处。
- 拒绝 stderr 带 net/state/obs/cur/unit（FAST-1）；超时、读失败独立报错（BOUND-TIMEOUT-DIAG）。

## 3. 破坏纯管道的旁路（逐项显式化）

| 旁路 | 位置 | 显式化 |
|---|---|---|
| 13 个 `\0cli/*` 资源 | src/stages.cx:175-199 | `--target` 加 `--resource`，入口列出本阶段读取的资源 |
| USLATTR1 属性 side-car | src/stages.cx:363 `cxaf_strip` | 留在字节流里随 stdout 传下段 |
| 多单元成帧 | src/driveunits.cx `dv_units` + cx/compiler/frames.cx | `frame` 伪阶段，共用 frames.cx |
| 转发桩两遍 | src/drive.cx:765-787 | 管道不做；另列用例 |
| include/文件读取 | cx/runtime/runexec.cx:298 `ex_file` | `--include-dir`/`--resource`；用 SPEED-02 trace 登记实际读取 |

## 4. stagecase 与 lab-loader 重复部分的合并

- stagecase 现在在工具进程里用 sim 跑 e2/e1/units/e3、自拼帧；转正后改为**调用 slice 的 program.com**（真实执行器），sim 只留给 `--trace` 定位。
- 保留 stagecase 独有部分：用例目录约定（tests/stages/<stage>/<case>/ 的 source/expect/override）、覆盖树、按表 sha 的网缓存、三条防假绿断言——这些上移成 `--dev` 的实现与 STAGE-LAYOUT 的用例格式。
- 帧拼接已统一在 cx/compiler/frames.cx（03dea445b9），不再增加第三份。
- stagecase 第一版照常先落主树，不返工；合并在其后一个提交里做。

## 5. 等价检查（管道绿才有意义）

`check-phase-pipe`：固定夹具（单源、65 源、cap64 覆盖一例）跑 `unisacc.com -o A` 与 `e2|e1|frame|units|e3|e4|prune|lower|elf` 得 B，断言 `cmp A B`；不等则逐段落盘报首个不同阶段。复用 lab-loader 已有的 21/21 段比对做法。登记批次。
