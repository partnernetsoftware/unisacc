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

## 1. 可改写面：常量操作数的 OUT 长序列

只有**操作数是 S 行下标常量**的 `OUT`（opcode 12）才能换成 S 行 + SBLOAD；
操作数是寄存器的 OUT（`regOUT`）不能换（值运行期才知道）。

| 指标 | 数值 |
|---|---|
| 42 网动作总数 | 829 026 |
| OUT 总数 | 331 550（常量 130 433 / 寄存器 201 117） |
| ≥16 连续 OUT 的段数（不分常量/寄存器） | 8 406 |
| ≥16 连续**常量** OUT 的段数 | 1 888 |
| 这些段里的常量 OUT 动作数 | 73 522(另有 8 406 段含寄存器 OUT,不可替换) |
| 这些段占用的网字节（zigzag varint） | 178 149 B |

最大三张：warnparse 618 段 / errorparse 616 段 / shared-e3 611 段，其余 39 张各 ≤11 段。

## 2. 替换形态与单价

参考内核 `cx/core/execute.cx` 的语义决定形态（**不能**用 SBBLOB：它写 scratch 不写 out）：

```
SBLOAD r5, idx     ; R[r5] = blob(S[idx])
INPUSH r5          ; 压一帧到该 blob 上
XLEN   r4          ; r4 = 帧尾
SPAN2  r4, r4      ; 把 [0, r4) 按当前 ot 写进 out
INPOP
```

二进制单价 **11 B**（每个操作码 1 + 每个操作数 1，寄存器号与下标都 <128）；
文本网（`.net`/tape）单价 **27 B**（`"56 5 0\n33 5\n46 4\n18 4 4\n36\n"`）。
一条新 S 行另外要 `2L+1` B（二进制，L 是段长）或 `2L+2` B（文本，hex + 换行）。

关键结论：**新建 S 行在 T=16 时基本不划算** —— 16 个常量 OUT 原来只占 32 B，
而替换要 11 B + 33 B 新 S 行 = 44 B，净亏。所以只有**该 blob 已经在 S 表里**的段才纯赚；
不在表里的段要段长足够大（≈ 文本口径下 L ≥ 14 就赚，因为文本 OUT 每个 5~6 B）才值得新建。

## 3. 字节差（两种口径，T≥16）

| 口径 | 改前 | 改后 | 净省 | 占该区域 |
|---|---|---|---|---|
| 文本网 `.net` / tape | 465 177 B | 125 536 B | **339 641 B** | 73.0% |
| 二进制包内 | 178 149 B | 94 438 B | **83 711 B** | 47.0% |

参数：**重写单元 = 一条 Q 行**（一条 Q 行里 ≥16 个连续常量 OUT 才算一段；跨 Q 行不能合并，
因为每行是独立的 query）。据此 1 888 段、新建 S 行 890 条。

> 早期一版把"跨 Q 行的连续 OUT"也算作一段，得到 1 784 段 / 822 条新 S 行 / 352 737 B，
> 那个口径**高估了**：跨行拼接的段在产品里并不存在，实施时必须按 Q 行切。上表是 Q 行口径。

> **给 cc 的选点**：T=16 在文本口径下已经是赚的（文本 OUT 单价 5~6 B），
> 在二进制口径下只有"blob 已在 S 表里"的段纯赚，新建 S 行在每段 16~32 个动作时是净亏
> （16 个常量 OUT 原本只占 32 B，替换要 11 B + 33 B 新 S 行 = 44 B）。
> 如果目标是**包字节**，建议 T=16 只替换命中已有 S 表的段（纯赚、零风险）；
> 若连新建一起做，实测 T=32 时二进制净省从 83.7 KB 降到 69.8 KB —— 说明**多新建 S 行是净亏**。

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

用这张表在 42 网 1 888 条可改写段上实测：

| 该条 Q 行里未被触碰的寄存器数 | 段数 |
|---|---|
| 32 | 1 554 |
| 31 | 315 |
| 30 | 19 |

**1 888/1 888 段都有 ≥2 个空闲寄存器**（最差也有 30 个），所以临时寄存器的分配不构成障碍。
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
本报告给出的是**上界**（Q 行口径：文本 339 641 B / 二进制 83 711 B），
与落地后必须复测的口径（包字节 + 装载耗时），以及唯一的改写点与替换形态。
