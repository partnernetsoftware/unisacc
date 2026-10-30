# 加速分析计划（2026-10-30，cc）

主题只有一个：**加速**。本文是分析计划，不是施工记录；只记实测，未实测标 UNMEASURED。
与 `plans/perf-bottlenecks-2026-10-30.md`（下称 B 篇）分工：B 篇覆盖 unisacc.com 自身编译链路的 B1–B5 / G1–G6；本文**增量**补两条 B 篇没展开的线——**gcc_cx / clang_cx 提速**与**多模块复杂程序（csih）编译运行提速**，并给出统一的加速路线排序。已装产品 = 仓库根 `unisacc.com`（sha256 见 `unisacc.com.build.json`），Linux x86_64 本机，单次观测为主，热数据另注。

---

## 一、起点：一张实测总表

| 动作 | 工具 | 耗时 | 说明 |
|---|---:|---:|---|
| 空 `.c` 动态执行 | unisacc | 114 ms | 固定开销（包解析 + 模型解压） |
| 52 B `.cx` 动态执行 | unisacc | 54 ms | 最小模块 |
| 774 B `src/stream.cx` | unisacc | 286 ms | 规模线性 |
| 1278 B `tools/cxproc.cx` | unisacc | 295 ms | 规模线性 |
| 41 KB `tools/cxcc.cx` | unisacc | 1387 ms | 约 30 ms/KB |
| 41 KB `tools/cxcc.cx` `-S` | unisacc | 1387 ms | 与完整执行相同，时间在前端+推理 |
| csih `-cx-plan`（31 模块） | unisacc | 93 ms | 只规划，不编译 |
| **csih 完整编译（31 模块闭包）** | **unisacc** | **42 s** | 产出 1.9 MB ELF |
| **csih 完整编译（31 模块闭包）** | **gcc_cx** | **527 ms**（冷）/ 353 ms（热） | 系统 gcc，快 80 倍；因 `-Werror` warning 报错 |
| csih 单入口 `-S` | unisacc | 16.7 s | 不展开闭包，报 16 个 undefined |

**结论性事实：**
1. unisacc 动态执行/编译耗时随源码规模线性增长（约 30 ms/KB），基线约 54–114 ms。
2. `-S` 与完整执行耗时相同 → 时间几乎全在**前端（E2）+ 网络推理（E3/E4）**，后端 elf/lower 常数项小。
3. 多模块场景（csih 31 模块）：unisacc 42 s vs gcc_cx 0.53 s，**差距 80 倍**。这个差距的根因就是 B 篇 B1（每个模块重扫 cx.h 前导）和 B2（执行器代码质量），多模块把它放大了 31 倍。
4. 现有 `perf-bottlenecks-2026-10-30.md` 已经定位了 unisacc 侧的单项瓶颈（B1 整缓冲重扫、B2 执行器、B3 模型解压、B4 多单元重复、B5 构造自身），本文不重复，只补两条新线。

---

## 二、线 A：gcc_cx / clang_cx 的提速可能

`gcc_cx.com` / `clang_cx.com` 是普通 C 二进制（`tools/cxcc.cx` + `src/cxmod.c` 编成），不经过 unisacc 网络。它本身已经快（单模块冷 0.16 s，多模块热 0.072 s）。但它是**串行**的：`tools/cxcc.cx` 里每个单元串行 `cxcc_exec`（`posix_spawnp` + `waitpid`），无并行。

### A1　多单元并行编译（收益最大，只影响多模块）

- 现状：`cxcc_expand` 的 while 循环逐 unit 顺序编译，每个单元一次 `posix_spawnp`+`waitpid`。31 模块的 csih，编译阶段是 31 次串行 gcc 启动。
- 方向：把无依赖的单元并行 `posix_spawnp`（先全部 spawn，再逐个 waitpid），或按模块依赖分层并行。链接阶段仍串行（只有一个链接）。
- 预期：31 模块的编译墙钟从「31 × 单模块 gcc 启动」降到「最长单元 + 其余重叠」。单模块无收益。
- 约束：cxproc.cx 目前是单 spawn 单 wait 封装；需要新增"批量 spawn 再 wait"入口，或驱动里直接管理多个子进程。缓存写 `.o` 的并发安全要核对（不同 hex 名，天然无冲突）。
- 风险：并行度受 `CXCC_SRCS=32` 和文件描述符上限约束；需有并发上限（如 CPU 数或固定 8）。

### A2　缓存 `.o` 二次拷贝可省（单模块热路径，收益小）

- 现状：`cxcc_expand` 缓存命中时 `cxcc_copy_file(cached, objs[nunit])` 把缓存 `.o` 拷到临时目录，链接阶段再传给 gcc。多一次 4 KB 块拷贝 + unlink。
- 方向：链接阶段直接用缓存 `.o` 的绝对路径，跳过临时拷贝。链接器不修改输入 `.o`，安全。
- 预期：单模块热从 0.16 s 降到约 0.13 s（省一次拷贝，收益微小，但零风险）。

### A3　header fold 结果缓存（每次运行的固定开销）

- 现状：`cxcc_cache_open` 每次重新 fold `cx.h`、`unisa_sha256.h`、`unisa_safedel.h`（及 Windows 的 hostinc 头）。即使编译器 tag 已缓存（`cxcc_cid_set`），header 字节未变，仍重复 SHA256。
- 方向：按（路径, mtime, size）缓存这些固定头的 fold 结果，命中跳过重新读+hash。
- 预期：这些头不大，收益是常数项（微秒级），优先级低；但对频繁 `-run` 的小工具累积有少量意义。

### A4　gcc_cx 与 unisacc 的定位差异（不优化项，记一笔）

- gcc_cx 的意义是「用系统 gcc 快速编出普通 C 可执行，作开发/CI 的宿主入口」，不授予已检查身份。它不是 unisacc 的替代，也不追求与 unisacc 等价。把 gcc_cx 优化得再快，也解决不了「unisacc 自身编译慢」这个主问题——那要靠 B 篇的 B1/B2。

**线 A 结论**：gcc_cx 可优化空间主要是 A1（多单元并行），A2/A3 是微小常数项。A1 只对多模块（csih 这类）有意义。

---

## 三、线 B：csih 这类多模块复杂程序的编译运行提速

csih：`apps/csih/`，72 个 `.cx` 文件，`csih.cx` 入口的模块闭包 31 个，合计 22,671 行。`csih.sh` 三态：

1. `prepare`：一次编译成 `out/csih-e2e-bin/csih`（实测 42 s，脚本注释 `~1min`），按源 digest 判增量。
2. 默认：`CSIH_BIN` 直接 exec 已编译 ELF（即时，无编译开销）。
3. `CSIH_COLD=1`：强制 `-run`（脚本注释 `~47s/module graph`）。

### B1'　编译期：31 模块 × 每模块重扫 cx.h 前导（= B 篇 B1 的放大）

- 现状：csih 42 s 编译里，每个模块的 E2 都重跑同一份 `cx.h` 前导（`-include include/cx.h`）。B 篇已实测：空 main `.cx` 一次 E2 共 213 次 SWAP、累计换入 5000 万字节；31 模块就是 31 倍。
- B 篇已把「已处理前缀不再进入后续 SWAP」列为 B1 方向（预期换入量 5000 万 → 200 万字节）。csih 是这个优化的**最大受益者**：42 s 里的 E2 占比会从「13%（单对象构造观测）」在多模块场景大幅上升。
- 补充方向（多模块特有）：**前导结果跨模块缓存**。cx.h 前导的 E2 产物只依赖（目标、预定义宏、前导字节），与具体模块无关。31 个模块可以共享一次前导 E2 结果，而不是各跑一次。B 篇 B4 已提「前导结果按（目标、预定义、前导字节）缓存」，csih 是直接验证场景。

### B2'　编译期：多单元重复工作（= B 篇 B4）

- 现状：每个 `.cx` 单元各自重跑同一份 cx.h 前导 E2（B1' 的成本乘单元数）。B 篇 B4 已记录「产品实测 `tools/dev-run.cx` 冷 8.7 s 中 units+e3 因 fwd 重启跑两遍（约 1.8 s）」。
- 方向：同 B1' 的前导缓存；另核对 csih 的 fwd（转发）模块是否触发 e3 重跑。

### B3'　运行期：`prepare` + `CSIH_BIN` 已经是正确模式（无优化必要，记一笔）

- csih 的日常运行已经通过 `prepare` 把编译成本前置到一次性构建，之后 exec 原生 ELF，运行期零编译开销。这是**多模块程序加速的正确范式**：把「冷 -run 47 s」换成「一次性 prepare 42 s + 每次 exec 即时」。
- 唯一残留：`prepare` 的 42 s 仍受 B1'/B2' 支配，是 unisacc 编译链路的固有成本，不是 csih 脚本能优化的。

### B4'　编译期：gcc_cx 作为 csih 的快速迭代入口（横向）

- 实测 gcc_cx 编 csih 只 0.53 s（虽因 `-Werror` warning 报错）。若 csih 的开发/测试迭代需要「改一个模块 → 快速重编验证」，gcc_cx 是 80 倍于 unisacc 的路径。
- 前提：gcc_cx 产物是普通 C，不授予已检查身份；只能作开发期行为验证，不能作发布/验收证据。
- 约束：csih 模块用了 `-Werror=implicit-function-declaration` 之外的 gcc 警告（`snprintf` 截断），需要 gcc_cx 驱动加 `-Wno-` 或 csih 源码消警。这是工程决策，不在本文展开。

**线 B 结论**：csih 编译提速的根子还是 B 篇 B1（前导重扫）+ B4（多单元重复），csih 是这两个优化的**标准验证载体**。运行期已经用 `prepare` 做到最优。gcc_cx 可作开发期快速迭代入口（横向 80 倍），但定位是宿主工具不是验收路径。

---

## 四、统一加速路线排序（合并 B 篇与本文两条线）

按「收益 × 可行性 ÷ 风险」排，标注车道（快 = 快车道，慢 = 慢车道，见 `plans/dev-methodology-2026-10-28.md`）：

| 序 | 项 | 车道 | 收益 | 依赖/风险 |
|---|---|---|---:|---|
| 1 | B1：E2 前导不重扫（前缀已处理部分不再进 SWAP） | 慢 | 最大；csih 42 s → 预计秒级 | 需带位置属性的截存/回放，模型规则+构造 |
| 2 | B4+B1'：cx.h 前导跨模块共享缓存 | 慢 | csih 31 模块直接受益 | 依赖 B1，或独立做前导级缓存 |
| 3 | A1：gcc_cx 多单元并行编译 | 快 | gcc_cx 编 csih 0.53 s → 预计 0.2 s | 新增批量 spawn；cxproc 扩展 |
| 4 | B3：模型解压缓存（run-cache 已部分落地） | 快 | 空 main 冷 330 ms → 热 58 ms | 已交 grk，run-cache 移植中 |
| 5 | B2：执行器代码质量（G1–G6） | 慢 | 自编热循环 3–4 倍 | 有限表+构造，逐项私有包对拍 |
| 6 | A2/A3：gcc_cx 缓存 `.o` 免拷贝 + header fold 缓存 | 快 | 微小常数项 | 零风险，顺手做 |

**下一步动作（本文不施工，只定方向）：**
1. 先用相位计时把 csih 42 s 的构成拆开（E2/E3/E4/elf 各占多少，多模块是否 31×E2 前导），验证 B1' 是主因还是 B2'（执行器）才是主因。现有 `perf-bottlenecks` 相位表只覆盖单对象构造，csih 多模块的相位分布仍 UNMEASURED。
2. 确认后，B1（前导不重扫）优先于 gcc_cx 并行，因为它同时救 unisacc 自身和 csih。
3. gcc_cx A1（并行）作为独立的快车道小项，可先行，因为它不碰模型、不改验收合同。

---

## 四之补：csih 42 s 的相位实测（2026-10-30，决定性数据）

`UNISACC_RR_TIME=1` 实测 csih 完整编译（31 模块闭包），推理耗时合计约 42 s：

| 相位 | 耗时 | 占比 | 说明 |
|---|---:|---:|---|
| **e3** | **22.28 s** | **53%** | 两遍：11.27 s + 11.03 s |
| lower | 5.53 s | 13% | |
| units | 3.74 s | 9% | 两遍：1.84 s + 1.84 s |
| elf | 3.26 s | 8% | |
| tapecheck | 2.32 s | 5.5% | |
| prune + symbols | 0.78 s | 1.8% | |
| e2 | 0.18 s | 0.4% | **前导重扫已被 cx-flat 解决** |
| e1 | 0.014 s | ~0% | |

阶段顺序（关键）：
1. `construction` → `units`(1.84s) → `e3`(11.27s)  ← **第一遍**（fwd 探测，无 e2/e1）
2. `construction` → `e2`(0.19s) → `e1` → `units`(1.84s) → `e3`(11.03s)  ← **第二遍**（fwd 重启，完整重跑）
3. `tapecheck` → `prune` → `symbols` → `lower` → `elf`

**三个推翻/修正先前假设的事实：**
1. **e2 只有 0.18 s，不是主因**。`include/cx.h` 已展平成 `include/cx-flat.h`（B 篇 B1 的前导部分已在结构层解决），csih 多模块没有「31×前导重扫」的问题。B1' 假设不成立。
2. **主因是 e3 占 53%，且跑了两遍**。第二遍是 `dv_fwd_replay()`（`src/drive.cx`）触发：`mode2==3`（`-run`）或 `image_fwd` 时 `dv_run_lower` 探测到需要转发桩，插入 stub unit 后 `cxt_text` 完整重跑 e2+e1+units+e3。csih 用了进程类调用（`posix_spawn`/`pipe`/`dup2` 等），必触发。
3. **e3 单遍 11 s 本身也偏大**（31 模块合计约 22,671 行，e3 约 0.5 ms/行），受 B 篇 B2（执行器代码质量）支配；但**先消除 fwd 重启的第二遍（省 11 s）比优化 e3 单遍更直接**。

**据此修正 ROI 排序：**

| 序 | 项 | 车道 | 收益 | 说明 |
|---|---|---|---:|---|
| 1 | **消除 fwd 重启的 e3 重跑** | 快/中 | csih 42 s → 31 s（省 11 s） | 复用 `dv_hold_fr`/`dv_hold_at` 已保存的第一遍 units 帧，重启时不再重跑 e2/e1/units/e3，只 splice stub + 续跑 lower |
| 2 | e3 单遍加速（B2 执行器 + G 系列） | 慢 | e3 11 s 每遍，多遍/多模块累积 | 受有限表+构造约束 |
| 3 | lower 加速（B2） | 慢 | 5.53 s | 同上 |
| 4 | elf 加速 | 慢 | 3.26 s，常数项重 | 每字节约 800-1400 ns |
| 5 | gcc_cx 多单元并行（A1） | 快 | 只对 gcc_cx，不救 unisacc | 独立线 |

**下一步（待验证 fwd 重启可消除性）：**
- 确认 `dv_hold_fr`（第一遍保存的 framed units）能否在第二遍直接复用，跳过 `cxt_text` 的 e2/e1/units/e3 重跑。代码注释已写「does not compile the original units again」，但实测 e3 两遍说明当前实现仍重跑了 e3——这是可消除点，需读 `dv_fwd_replay` / `dv_fwd_splice` 确认边界。

---

## 五、未实测 / 待澄清

- csih 42 s 编译的**相位分布**（E2/E3/E4/elf 各占多少）——UNMEASURED，是第 1 步要补的。
- gcc_cx 编 csih 的 warning 根因与是否可加 `-Wno-` 开关——待定。
- B1' 前导共享缓存的键设计（目标/预定义/前导字节）是否足够——待设计。
- gcc_cx A1 并行的并发上限与文件描述符约束——待实测。
