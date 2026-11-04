# C44-REGALLOC-01 设计（最小方案：表达式临时值进寄存器）

status: DRAFT（2026-11-04 立，omp-bdy）
预算：240 min

## 1 事实（实测，不是推测）

同一份 `tools/bench-gencode.c`，8 MB 工作负载：

| workload | unisacc | gcc -O0 | gcc -O2 | unisacc/O0 |
|---|---|---|---|---|
| sha256（纯 C） | 1012.5 ms | 108.3 ms | 47.5 ms | 9.4× |
| sort（qsort 2M int） | 3930.8 ms | 470.5 ms | 310.8 ms | 8.4× |
| memcpy | 11.9 ms | 7.4 ms | 7.5 ms | 1.6× |

`unisacc -S` 出 312 744 B / 18 457 行，`gcc -O2 -S` 出 13 077 B / 约 400 行（24×）。
指令构成：`store64` 2248 + `load64` 2609 = **4857 次内存操作**，gcc 约 184 次 mov。
`memcpy` 只慢 1.6×，说明内存带宽与调用开销不是瓶颈——**瓶颈是每条表达式结果都写栈再读回**。

## 2 为什么不做窥孔

严格安全窗口（禁标签/call/写 `[addr]`/写源寄存器，窗口 1）实测可省 89 行 / 18 457 = **0.5%**。
窗口放宽到 6 也只在 `sort` 这类大函数里略有增长，且 `.frame` 对消与死 `imm` 被标签阻断。
窥孔上限远低于早先记的 12%（那条是未校验的宽口径，见 ledger 2026-11-04T12:20 CORRECTION）。
**0.5% 不值得一次产品重建**，故放弃窥孔，直接做寄存器直通。

## 3 落点（关键约束）

- tape（72 种 op）由 **lower 网推理**产出，网本身不改。
- `src/stages.cx:354` 的 `ex_ri_exec(route, stage, ...)` 之后、`rt_strip` 之前，
  是 cx 代码拿到整个 tape 字节的地方——**这里是唯一可插的产品级落点**。
- 在 `stage == "lower"` 且目标是 object/tape 路线时，对 `ob[0..on)` 做一遍**局部改写**：
  保持指令流语义等价，只把「刚 store 到栈槽、下一两条指令又 load 同一槽」换成寄存器直通。
- 必须与六个目标共享（tape 是目标无关的中间层），所以放在 `cx/` 而不是某个 target 的 encoder。

## 4 最小方案（三档，按收益递增，每档独立可停）

### 档 1：单条直通（store→load 紧邻/隔一条算术）
    store64 [A], rS
    ...（无写 [A]、无写 rS、无标签、无 call）
    load64  rD, [A]
    ↓
    store64 [A], rS     ；保留写回（栈槽仍是最终值，后续可能仍从栈读）
    mov     rD, rS      ；load 换成寄存器搬移

- 安全性：`[A]` 的写回保留，所以任何后续从 `[A]` 读的路径都不受影响；
  只要求窗口内 `rS` 没被改写。
- 收益上界：把 2609 次 load 中的一部分换成 mov。mov 在六个目标上都是单周期寄存器操作。

### 档 2：栈槽消除（可选，档 1 无收益时不做）
若窗口内 `[A]` 只被这一对 store/load 访问，且窗口结束前没有逃逸（不取地址、不跨块），
可删除 store，只留 `mov`。收益更大但需要槽级活跃性分析，风险也更大。

### 档 3：临时值不进栈（真正的寄存器直通）
在单个基本块内维护「哪些寄存器持有哪个临时值」，让 lower 之后的 tape 上
相邻的两个表达式不落栈。这等价于一个**局部线性扫描分配器**，工作量最大。

**先只做档 1**，用 gate 量出收益再决定是否做档 2/3。

## 5 验收（gate）

1. `sh ./unisacc.com tools/check-bench-gencode.cx -- . 7`
   → `BENCH_GENCODE invalid=0`，且 8 MB `sha`/`sort` 的 unisacc 中位相对 gcc -O0
   从 **9.4×/8.4× 降到 ≤4×**（`memcpy` 不变或更好）。
2. 字节等价回归：不改开关时 `-o` 产物与改前逐字节相同；
   `host.c -c -funit` 仍是 `c67d3a9a 230264 B`（LIBCUNIT gate a 的锚点）。
3. 六目标产物可运行：`lnx/x86_64`、`lnx/arm64`、`osx/x86_64`、`osx/arm64`、
   `win/x86_64`、`win/arm64` 各编一个 hello 并跑通（能跑的执行/env 覆盖不到的目标至少产物可解析）。
4. 批次全绿：`tools/ratchets.cx` ok=9 fail=0；`affected --files src/stages.cx --run` 全 PASS。

## 6 风险与已知边界

- tape 改写要**逐条解析 72 种 op 的操作数宽度**；宽度表在 `cx/compiler/tapeshape.cx`
  （`ts_names[72]`）与 `cx/core/execute.cx` 的 `CORE_ACTION_ARITIES`，两处必须一致。
- 只处理 `store64`/`load64`（宽 8 字节）；`store32`/`.st`/`.ld` 等先不动。
- 标签边界、call 边界一律放弃改写（保守）。
- 改写发生在 `stage == "lower"`，所以 `e3`/`elf` 阶段的 tape 不受影响；
  `tapecheck` 阶段如果也在同一 tape 上跑，需要确认不改写它的输入（只在 lower 后改一次）。

## 7 下一步（本条目第一刀）

1. 写 `out/regalloc-proto/`（临时原型，不入 tools/）读一份 lower 产出的 tape，
   统计「档 1」模式命中数与预期 mov 数——先量收益，不改产品。
2. 命中率若 ≥ 20%（相对 load 总数），在 `src/stages.cx` 实现档 1 并跑 gate。
3. 命中率不足则记 CORRECTION，转档 2 或放弃本条。
