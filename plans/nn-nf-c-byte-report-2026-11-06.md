# NN-NF-C 字节差报告（2026-11-06，候选 7266862b08，42 张网）

口径：同 rev 网字节 cmp 等价 + 字节差报告。cmp 部分是可执行的批次
（`tests/check-nn-nf-c.cx`，登记 `nn-nf-c-byte-report`），本文件只放**测量结果**，
每个数字都能用下面的命令重算。

## 0. 同 rev 网字节等价（cmp 的门禁）

```
sh ./unisacc.com tests/check-nn-nf-c.cx -- "$PWD" out/nncm2 out/nncr2
NN_NF_C_REPORT nets=42 equal=42 diff=0 missing=0 runs_ge16=8406 actions=336439 bytes=883488
NN_NF_C assertions=10 failed=0
```

- 42/42 张 `net-<name>/result.net` 与包内 `models/<name>.net` **逐字节相同**（diff=0、missing=0）。
- 反例有牙齿：改一个字节 → `DIFF` 且退出码非 0（`refusal.exit-nonzero` / `refusal.one-diff-detected` 两条都 PASS）。
- 材料 `nnc-nets` 由 `tests/nnc-nets-material.cx` 生成，`--verify` 逐网重算两 sha256：`rows=42 failed=0`。

## 1. 可改写面：连续 OUT 的长序列（全部可改写）

**关键事实**：`case CORE_OUT: ... core_put(&s->out, (int)a[1], s->ot)` ——
`OUT` 的操作数 `a[1]` **就是那个字节本身**，不是寄存器也不是 S 行下标。
所以 42 网里 **331 550 个 OUT 全部是编译期常量**，每一个都可改写
（早期一版以为"只有下标常量才能改"，把可改写面砍成了 1/5，那是错的）。

| 指标 | 数值 |
|---|---|
| 42 网动作总数 | 829 026 |
| OUT 总数（全部常量） | 331 550 |
| ≥16 连续 OUT 的段数 | **6 978** |
| 这些段里的 OUT 动作数 | 292 691 |

最大三张：warnparse / errorparse / shared-e3 各约 600+ 段，其余 39 张各 ≤11 段。

## 2. 替换形态与单价

参考内核 `cx/core/execute.cx` 的语义决定形态（**不能**用 SBBLOB：它写 scratch 不写 out）：

```
LDI    r6, 0       ; 起点必须显式清零：regs 只在整次 run 起始 calloc，跨 query 保留
SBLOAD r5, idx     ; R[r5] = blob(S[idx])
INPUSH r5          ; 压一帧到该 blob 上
XLEN   r4          ; r4 = 帧尾
SPAN2  r4, r6      ; 把 [0, r4) 按当前 ot 写进 out（SPAN2 的起点是 R[a[1]]）
INPOP
```

两处容易错的细节（都已在 `cx/core/execute.cx` 里核对过）：

1. `SPAN2` 是 `s0 = R[a[1]], s1 = R[a[2]]`，**起点取自寄存器**。必须先用 `LDI r6,0` 把它清零，
   否则会从上一段残留的值开始写（`s.regs` 只在 `core_run` 开头 `calloc` 一次，跨 query 保留）。
2. `SPAN2` 写 out 时用 `(op == SPANT || !F->at) ? s->ot : F->at[j]` —— 在新压的 blob 帧上
   `F->at` 为 NULL，所以逐字节用当前的 `s->ot`，与 `OUT` 的 `core_put(..., s->ot)` **完全一致**，
   属性也等价。属性只可能被 `SETOT`/`XATTR` 改动，而它们会打断 OUT 段，所以段内属性恒定。

单价：二进制 **14 B**、文本网 **33 B**（`"3 4 0\n56 5 0\n33 5\n46 6\n18 4 6\n36\n"`）。
一条新 S 行另外要 `2L+1` B（二进制）或 `2L+2` B（文本，hex + 换行）。

## 3. 字节差（两种口径，T≥16，重写单元 = 一条 Q 行）

| 口径 | 改前 | 改后 | 净省 | 占该区域 |
|---|---|---|---|---|
| 文本网 `.net` / tape | 1 908 214 B | 536 504 B | **1 371 710 B** | 71.9% |
| 二进制包内 | 758 067 B | 401 709 B | **356 358 B** | 47.0% |

参数：6 978 段，**没有一段的 blob 已在 S 表里**（每张网有自己的 5084 条 S 行，
但 16 个连续 OUT 组成的 blob 从未预先存在），所以全部要新建 S 行。

> **给 cc 选点用**：目标若是**包字节**，只做"净收益为正"的段（即 `2L > 14+33`，约 L ≥ 24）：
> 4 984 段可做、1 994 段太短跳过（新建 S 行会亏），
> 二进制净省 **306 185 B**、文本净省 1 181 260 B。
> 若把 1 994 个短段也强行改，二进制反而会因为这 1 994 条新 S 行回吐约 50 KB，
> 所以**改写器必须按净收益筛，不能只按 T≥16 一刀切**。

## 4. 同笔删 OLAST / ODROP

cc 要求同笔删掉 `OLAST`(19) 与 `ODROP`(20)。实测：两者在 42 张网的 Q 行里**出现 0 次**，
所以"同笔删"天然满足 —— 不需要删任何网内容，只需在实施时从事实表与八份副本里摘掉定义，
并把 19/20 之后的序号前移（`OP_COUNT` 57→55，`SBLOAD` 的 index 56 也要跟着挪）。

## 5. 复算命令

```sh
# 1) cmp 门禁
sh ./unisacc.com tests/nn-nf-c-report.cx -- "$PWD" out/rtl3/ctx/jobs out/rtl3/routes/models
sh ./unisacc.com tests/check-nn-nf-c.cx  -- "$PWD" out/wnncm2 out/wnncr2

# 2) 造材料（会话自己的克隆里）
sh ./unisacc.com tests/nnc-nets-material.cx -- prepare "$PWD" out/rtl3/ctx/jobs out/rtl3/routes/models out/nncm2
sh ./unisacc.com tests/nnc-nets-material.cx -- --verify "$PWD" out/nncm2
```

## 6. cc 裁定与实施位置

cc 2026-11-06 裁定：
1. **T ≥ 16**（原语审计与 Q 行口径都支持；T≥8 只多几 KB 但改写面更大，风险不值）。
2. 验收按**包字节（deflate 后）与装载耗时前后**，不按 wire 字节；装载没改善就降优先级。
3. **不进 rc2**（rc2 已固定为 NATIVE-RUN-HOST-TARGET / WARN-PROBE-USED / 帮助文字），
   放 0.0.45 开头、rc2 候选冻结之后；可以在仓外克隆里先做 patch 与 42 网 cmp 报告。

改写点只有一个：**生成链的 net 阶段**（`seed/net.c` 写 Q 行处，`for (j = 1+nstr; j < 1+nstr+nq; j++)
fprintf(out, "%s\n", lines.v[j]);`）。在那里按 Q 行扫连续 OUT（opcode 12）常量动作，
≥16 个就把该段替换成
`SBLOAD r5,idx / INPUSH r5 / XLEN r4 / SPAN2 r4,r4 / INPOP`，
并把 blob 追加进 S 表（或复用已有同字节的 S 行）。这属于改产品输入，必须等 R 解冻。

### 两个临时寄存器：已用实测种类表验证过

替换序列要占用**两个**空闲寄存器（`r5` 存 blob、`r4` 存帧尾）。它们必须在**该条 Q 行**里
既不被读也不被写，否则会改掉后续动作看到的值。

判定靠**每操作码的操作数种类**。我先试过"把每个操作数都当寄存器"，那个估算无效：
多数操作数是立即数（`LDI` 的值、`ADV` 的目标、`OUT` 的常量下标、`CMPI` 的比较值……）。
之后改为从 `cx/core/execute.cx` 的 `case` 体机械推导：`R[a[k]]` 的位置是寄存器、
裸 `a[k]` 的位置是立即数。解析覆盖 56/57 个操作码（只差 `BYTE`，它用宏名 `CORE_BYTE`，
已手工补；`OUT` 是 `CORE_OUT`，其 `a[1]` 是常量字节不是寄存器，也已手工更正）。

用这张表在 42 网 **6 978** 条可改写段上实测（替换要 3 个寄存器：`r4` 帧尾、`r5` blob、`r6` 起点零）：

| 该条 Q 行里未被触碰的寄存器数 | 段数 |
|---|---|
| 38 | 3 822 |
| 37 | 2 046 |
| 36 | 509 |
| 39 | 397 |
| 35 | 164 |

**6 978/6 978 段都有 ≥3 个空闲寄存器**（最差也有 34 个），所以临时寄存器的分配不构成障碍。
落地时仍需在改写器里按同一张表逐行判定，不能假设全局可用。

> 注意：这张种类表目前是**从 execute.cx 推导**的，不是独立事实表。
> 落地时应把它做成 `exec/facts/` 下的一张表并由生成器校验，避免推导脚本与切换语句脱节。

### 等价性 oracle 的现状（重要）

要证明"改写后 42 网逐字节等价"，需要把**改写后的文本网**跑起来比输出。我写了一个
`tests/nnc-run.cx`（把文本网的 S/Q 行装成 flat Model 后调 `cx_run`），但**这条路走不通**：
这 42 张网是 `isnet=1` 的阈值网，`CoreModel` 需要 `mode/keys/next/seq/lo/hi/base_next/base_seq`
这些**从 H 行（阈值表）解析出来的字段**，而 `src/executor.cx:44 ex_model` 是从 `CxModel`
（`cxmd_decode` 的产物）映射的 —— 也就是说必须走 `cxmd_decode` 解码整张网，不能只解 S/Q 行。
我撤掉了那个半成品（它只能处理 flat delta，对 isnet 网会 segfault）。

**所以实施时的等价性验证应该这样**:用产品自己（`sh ./unisacc.com` + 新包）编同一个源，
比对**产物字节**（ELF/对象），而不是比对网的中间输出 —— 这正是 `nn-nf-c-report.cx` 的 cmp
在做的事（网字节等价），而"改网后产物不变"要靠 `tools/core-run-diff.cx` 那一套 +
新包重跑 42 个构造作业。清单与耗时已写进 `/tmp/wB/rc2-materials-plan.txt`。

### 为什么本次没落地

改写 42 张网 = 改生成链 = 改**产品输入**。cc 裁定它不进 rc2、且 R 冻结期间不动产品输入。
本报告给出的是**上界**（Q 行口径、全部 OUT 可改写：文本 1 371 710 B / 二进制 356 358 B），
与落地后必须复测的口径（包字节 + 装载耗时），以及唯一的改写点与替换形态。

> 口径更正史（留档，避免后来的人复用旧数）：
> - 最早一版把"跨 Q 行的连续 OUT"也算作一段（1 784 段 / 352 737 B），跨行拼接在产品里不存在，已废。
> - 第二版以为"只有 S 下标常量的 OUT 才可改"（1 888 段 / 339 641 B），
>   但 `CORE_OUT` 的操作数就是字节本身，所有 OUT 都可改，已废。
> - 当前版本：全部 OUT、按 Q 行切、6 978 段、单价 14 B（含 `LDI` 清零起点）。

## 7. 按 cc 口径实测（deflate 后包字节 + 装载耗时）：上界与可改写面

口径：42 张文本网（`out/rc2-8a3f028d57/ctx/jobs/net-*/result.net`，与
`routes/models/<name>.net` 逐字节相同），改写只在**一条 Q 行内**把 ≥16 个连续 `OUT`
换成 `LDI r6,0 / SBLOAD r4,idx / INPUSH r4 / XLEN r5 / SPAN2 r6,r5 / INPOP`，
blob 追加进 S 表（同字节复用）。测量脚本是仓外的只读 Python，输入身份见上。

| 口径 | 改前 | 改后 | 差 |
|---|---|---|---|
| 42 网原始字节合计 | 11 941 135 | 10 581 257 | −1 359 878（−11.4%） |
| 拼接后 zlib-9 | 1 691 693 | 1 675 179 | −16 514（**−0.98%**） |
| 拼接后 zlib-6 | 1 756 505 | 1 736 963 | −19 542（−1.11%） |
| 逐网 zlib-9 之和 | 1 683 327 | 1 667 384 | −15 943（−0.95%） |
| `cxmd_decode` 每网均值（10 次重复） | 12 251 902 ns | 10 169 022 ns | **−17.0%** |

装载耗时用仓外 `loadtime`（`cxmd_decode` 单一入口，`reps=10`，42 网全解码
`failed=0`）测；两轮重复稳定在 −15.6% ~ −17.0%。

**结论一（对 cc 的口径）**：deflate 后只省 **0.95% ~ 1.11%**（约 16 KB），
显著低于 report 里 wire 口径的 73%。装载耗时省 17%，是这条路线的主要收益。

**结论二（改写面）**：13 915 段连续 OUT 中 6 932 段可改（291 125 个动作），
筛掉的 6 983 段是 <16；另有 46 段在被 `OSEL 1` 切到 stderr 的上下文里（不能拿 SPAN2 顶），
以及 r4/r5/r6 在该条 Q 行内被读或写的段。

**结论三（安全性前提已被测试钉住）**：
- 寄存器跨 query 保留（`cx/core/execute.cx:195` 只在 `core_run` 开头 `calloc`），
  所以 `LDI r6,0` 清起点是必需的。
- `SBLOAD` 每次执行都 `core_blob_add` 追加一个 blob（`bytes.cx:15`，不去重），
  即改写会在运行期按**执行次数**增长内存 —— 这是 deflate 之外必须复测的第二项成本，
  本报告未测（需要跑整网）。
- `tests/check-sbload.cx` 已证明 `OUT×5` 与 `SBLOAD+INPUSH+XLEN+SPAN2+INPOP` 输出一致。

**结论四**：改写的正确性门槛是"每条 Q 行里两个寄存器既不读也不写"，
这只能由 `exec/facts/core-opkinds.tsv` 判定；该表与校验器已进主树
（`01ff20d262` / `b5c4835122`，批次 `core-opkinds` PASS）。

## 8. 前置表与校验器的严格化（2026-11-06）

`exec/facts/core-opkinds.tsv`（57 行）与 `tests/check-core-opkinds.cx` 已进主树，批次 `core-opkinds` PASS。

**为什么不能只按 arity 判空闲寄存器**：多数操作数是立即数（`LDI` 的值、`ADV` 的目标、`OUT` 的常量、
`CMPI` 的比较值……）。把立即数当寄存器会既掩盖真被用的寄存器、又把立即数 4/5/6 误判成占用。

**共享 case 体的陷阱（上一版校验器的盲区）**：
```
case CMP: case CMPI: { I u = R[a[1]], v = op == CMP ? R[a[2]] : a[2]; ... }
case A64: case A64I: { int z; R[a[2]] = alu64((int)a[1], R[a[3]], op == A64 ? R[a[4]] : a[4], &z); ... }
```
同一位置对不同 opcode 是不同种类。上一版把整个 case 体一律看待，于是把
`CMPI` 的 a[2]、`A64I` 的 a[4] 记成寄存器也能 390/390 全绿；新版按
`op == NAME ? A : B` 切分支，只比对本条 opcode 实际走的那一支，
同一张错表报 2 条 FAIL、修正表 0 失败。这是收紧，不是放宽。

**改写面的最终数字**（42 网，按上述种类表判安全性）：
| 类别 | 段数 |
|---|---|
| 连续 OUT 段总数 | 13 915 |
| < 16（不改） | 6 983 |
| 被 `OSEL 1` 切到 stderr（不能拿 SPAN2 顶） | 46 |
| r4/r5/r6 在该条 Q 行内被读写（不改） | 0 |
| **可改写** | **6 932**（291 125 个动作） |

实测三组取值下 r4/r5/r6 都没有被读写的段，但这仍是逐段判定的结果，不是"某组寄存器永远空闲"的假设。

## 9. deflate 基线（不改造的原始量级）

第一节的表给的是**改造写后**的差值。这里补一个不改造的基线，用树内
`text.deflate`（`ext/textx/deflate.cx`，LZ77 32 KiB 窗口、懒匹配，gzip -6 同级）
在报告工具内部算，不依赖外部 `gzip`：

| 量 | 值 |
|---|---|
| 42 网原始字节合计（nettotal） | 11 941 135 |
| 42 网 deflate 后合计（deflated） | 1 716 572 |
| 压缩比 | 14.4% |

这是**当前**的包内形式。改写后重测得到 1 675 179（zlib9）量级，
也就是说改写节省的 1.6% 是相对于这个 1.72 MB 的基数，而不是相对于 11.9 MB 的 wire 基数。

报告的汇总行现在同时给出 `nettotal` 与 `deflated`；`tests/check-nn-nf-c.cx`
断言两者都存在且 `deflated < nettotal`，所以报告退化成"只测 wire"会立刻变红。
