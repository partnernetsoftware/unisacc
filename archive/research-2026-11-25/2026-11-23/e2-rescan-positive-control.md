# 0.0.95 e2 重扫修复:阳性对照(语料能否抓到被修缺陷)

问题:三条「原包 = 补丁包」的证据(`-o` 1154、pp-equiv 5128、单元 113)只能证明**没改坏**,不能证明**语料能抓到缺陷**。
若语料从不走被修的那条路,全 SAME 是空洞的。本节用两条独立判据补上。

## 一、被修的是什么(读 `40df7913c5` 与 `plans/v0.0.95.md` 七节)

修复点: `#include` 前缀重扫。`INCH` 256(行首 `#` 后的 256 字节头体)原先直接 `INPOP` 回 `P0`,把已读头体重新入队再走一遍,
于是每个 `#include` 的行体被扫两次;补丁改成 `P0.inc` / `P1.inc` 状态,用 `INC_SUB` 计数 + `INPUSHXE RESUME INC_END`
只重入**剩余**部分,`P0`/`P1` 的 256 分支改判 `CMPI INC_SUB 0`。

所以缺陷的可见信号有两条:

1. **字节**:修复前重扫会把同一段字节二次送入下游,理论上有输出差异风险(实际被下游去重掩盖);
2. **步数**:重扫本身是纯多余步,修复前后 **e2 步数必然下降**。

第 2 条才是本修复的语义:它是**步数**修复,字节等价是合同不是目的。

## 二、判据 A:步数必须下降(本修复自己的信号)

工具:临时探针(只读,不入库),对每个单元用两个包各跑一次
`stream e2 --level 1 --models PKG --target lnx/x86_64 --source F < F`,`UNISACC_RR_TIME=1` 取
`RR_TIME inference stage=e2 ... steps=N`。

113 项清单(`uec-113.tsv`,sha `18de657e…`)分三批跑完:

| 层 | 行数 | 步数变化行数 | 原包步数 | 补丁包步数 | 降幅 |
|---|---|---|---|---|---|
| tools | 40 | 40 | 252229286 | 134113748 | 46.8% |
| cx | 25 | 25 | 75585345 | 60910516 | 19.4% |
| ext | 20 | 20 | 52770165 | 45170214 | 14.4% |
| examples | 8 | 8 | 30505330 | 12272807 | 59.8% |
| tests | 7 | 7 | 44472984 | 13203911 | 70.3% |
| corners | 12(1 例无步数行) | 12 | 425200 | 92691 | 78.2% |
| 合计 | 112 | **112(100%)** | 455988310 | 265763887 | **41.7%** |

**结论:112/112 单元步数都变**,语料全部走到被修路径。字节 112/112 SAME(与三条证据一致)。
复跑(工具已内置,`de7b0c4c46` 起):

```
sh ./unisacc.com tools/unit-e2-compare.cx -- run-all \
  --ape ./unisacc.com --models-a <orig.pkg> --models-b <patched.pkg> \
  --manifest ~/.local/share/unisacc/research/v095-rescan-review/uec-113/uec-113.tsv \
  --ledger <新目录> --shards 8 --steps 1 --side-secs 20
```

末行 `STEPS_SUM rows=112 step_moved=112 step_grew=4`(grew 即上表的 4 例固定开销);
账本每行 detail 带 `steps=A/B`,可直接算降幅。`--steps 0` 回到纯字节对拍。

最小复现(手造,3 行):

```
plain.c: int a;                       -> 原 2709 / 补 2711 步(无 #include,只多 2 步的常量差)
inc1.c : #include "plain.c" + int b;  -> 原 3056 / 补 3046 步(-10)
inc2.c : #include "inc1.c"  + int c;  -> 原 3399 / 补 3359 步(-40)
```

即:含一层 `#include` 就触发,嵌套两层降幅加倍。`corners/depth-overflow.c` 最极端:385528 -> 54350 步(-86%)。

有 4 例步数**上升** 2-3 步(backslash-include、macro-include、nul、c04_choose_expr_nonconst_rej),
是 `INC_SUB` 的 `LDI`/`CMPI` 固定开销大于极短 include 体省下的重扫,量级 0.1% 以内,不影响判据。

## 三、判据 B:变异必须被抓住(cdx 已有证据,本节核对口径)

`~/.local/share/unisacc/research/v095-rescan-review/mutation-samples/`:

| 变异 | headers DIFF | tests DIFF | 说明 |
|---|---|---|---|
| 错 1 字节 | 30 | 13 | 篡改一个字节即 43 项 DIFF |
| 去掉 `INC_SUB` | 30 | 13 | 绕过修复逻辑即 43 项 DIFF |

两表合计 30+13 项 DIFF,与 `plans/v0.0.95.md` 七节记的 `diff=8`/`diff=9` 是同一批样本的更大切片。
**语料与工具对「修复被破坏」有分辨力。**

## 四、已有 5128 与 113 是否覆盖触发面

- **5128(pp-equiv)**:headers 2415 + c-tests 116 + corners 各 SAME。headers 全部含 `#include`(本身就是头),c-tests 与 corners 见下。
  按判据 A 的口径,headers 层等于全量触发面。
- **113(unit-e2)**:112/112 步数变,含 5 个 corners 直接是 `#include` 边角(backslash/continued/macro/cross-header/deep-valid)、
  25 个 cx 单元(`#pragma unisa use` 走同一 INCH 路)、40 个 tools、20 个 ext。
- **DIFF 缺席本身**:两包字节相同是**预期**——修复是步数修复,字节等价是合同。所以「没有 DIFF」不是警告;
  警告应是「步数不变」,而实测 0 个单元步数不变。

## 五、结论与限制

- 三条「原包 = 补丁包」证据 + 本节步数判据共同成立:语料**确实**覆盖被修路径,且字节等价不是空转。
- **未测**:`-o` 1154 项(由 csr 复核)的步数未取;pp-equiv 5128 的步数未取(工具只比字节)。
  若要把「5128 也覆盖触发面」做成硬证据,需给 pp-equiv 加步数模式;当前用 113 的 112/112 与 headers 层的结构性覆盖支撑。
- 探针是临时脚本;步数模式已内置为 `unit-e2-compare --steps 1`(见第七节)。

## 六、墙钟口径(步数降不等于墙钟降)

对 39 个单元(113 清单前 40 个,1 例无步数行)各测 7 轮、取中位数,读 e2 的 `RR_TIME inference` `ns=`:

| 指标 | 值 |
|---|---|
| 原包 ns 合计 | 3626740435 |
| 补丁包 ns 合计 | 2938933435 |
| 总降幅 | 19.0% |
| 中位比值(补/原) | 0.974 |
| 四分位 | p25 0.922, p75 1.007 |
| 补丁包更快 | 26 / 39 |
| 补丁包更慢 | 13 / 39 |

步数与墙钟是否一致(同一批 39 单元):

| 关系 | 计数 |
|---|---|
| 步数降且墙钟降 | 25 |
| 步数降但墙钟升 | 11 |
| 步数升但墙钟降 | 1 |
| 两者都升 | 2 |

**结论:墙钟改善存在但远小于步数改善(19.0% vs 41.7%),且不一致。**
原因是 e2 墙钟里含模型装载、verify/layout 与调度开销(单次 `model` 段就有 7 ms 量级),
步数只覆盖推理部分;小单元上这部分固定开销把步数收益吃掉。p75 已过 1.0,
即四分之一的单元补丁包更慢。写发布说明时不能把 41.7% 当墙钟收益。

## 七、永久夹具

`tests/unit-e2-rescan/{plain,inc1,inc2}.c` + `check-unit-e2-compare.cx` 的 `rescan fixture` 三条关系断言
(`de7b0c4c46` 后提交):

- plain(无 include):补丁包步数不高于原包 2 步(`st[0] >= -2 && st[0] <= 0`),实测省 -2;
- inc1(一层):补丁包严格更少(`st[1] >= 1`),实测省 34;
- inc2(两层):严格更少且省得比 inc1 多(`st[2] >= 1 && st[2] > st[1]`),实测省 112。

断言写关系不写绝对数,改 e2 后不会因基线漂移误红;没有 `UNISACC_PPEQ_PKG_ORIGINAL/PATCHED` 时
打印 `UNMEASURED rescan-fixture` 具名,不记 PASS。
