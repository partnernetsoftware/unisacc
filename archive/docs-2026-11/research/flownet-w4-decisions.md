# flownet W4：决策网扩展（观测 → 决策）的口径、实测与可达性

> 需求：`plans/m4-flownet-cx-module.md` §4 W4、§8 裁定 4 · 执行：cc（lane A）· 2026-10-23 · 全部命令在 scratch 副本里跑（`git archive HEAD` + root `unisacc.com`），每条 `timeout 58`。
> 结论一句话：**type、shape、ic 升格**（ujs 适配器的前端消费其答案，自测变异探针 shift 皆拒收，EXPECT 随之更新）；**scope、irsel 维持 RESERVED**（不改 ujs 前端则不可达，可达性分析见 §4）；reloc 不在本波四网之内，维持 RESERVED。

## 1. 决策的测量口径（钉死）

沿用 m4 `selftest` 的口径，搬到 flownet 的注入面上，不新增 API：

- **变异**：shift（每个键的答案换成该网类序中的下一类，即 m4 `_Shift`）；trap（每个键都答 `trap`，即 m4 `_Trap`，只用于有 `trap` 类的网：isel、enc）。
- **变异怎么进网**：一个说谎的标签源 `lying_label`（包住 `.cx` gold 移植 `ujsgold_label`），经 `fn_label_source` 注入，`fn_build` 用它自建全部整数网。网对说谎源逐键精确，故 acc 看不出谎言；流只经 `fn_ask` 碰到它。这与 m4 包住 IntNet 的 `predict` 等价，且只用公开导出面。
- **判定**：某网在某变异下**决策**，当且仅当其余网诚实、该网说谎时，W1 记录的 33 个程序（28 语料 + 5 探针，m4 次序）中有一个输出与记录不同（字节数或 sha256），或被具名拒收（rc 1、stdout 空、无输出文件、stderr 一行带原因，即 D5 形状）。每次构建在 fork 出的子进程里跑、复用父进程构造的网（6 个一批并行，按程序次序判定，与逐个跑结论相同），每个子进程 20 s 上限（超时记 rc 124，不算拒收，红）。
- **参照**：同一 `fn_build` 用诚实源构造的网，33 个程序逐字节等于记录（`selftest` 第 1 份）。
- **EXPECT_W4**（`adapter/ujswasm/ujswasm.cx`）= m4 的 `EXPECT`（基线 `expect` 节，从 `m4/selftest.py` 直读）只改三行：type、shape、ic 由「nothing」改为「shift」。自测逐行核对：其余网与 m4 记录必须相同，被升格的网必须在 m4 记录里是 nothing。
- **choose 证明**（W2 留给本波的 isel 选择探针）：只让 isel 的 `(eq)` 键答 `f64_cmp`，`corpus/ternary.ujs` 的字节与 `i64_cmp` 不同（3227 / 3230 字节），`i64_cmp` 的字节等于记录，两者在 node 下都得 100。

## 2. 前端怎么消费（为什么是「合法/拒收」而不是「选择」）

m4 的 `note_binop` 每个二元运算问 type(`i64` OP `i64`) → shape(由 type 答案定) → ic(由 shape 答案定) → isel，前三个答案只做侧信道，被丢弃。ujs 适配器（`.cx` 移植，边界外）W4 的改法：

1. **问法不动**：问哪些网、键是什么、type → shape → ic 的链，与 m4 逐字相同，所以账目（相位、网、命中数）与字节都仍是 ujs 记录（D2 不退）。
2. **答案被消费**：`BINOP_LOWER` 按运算符钉死直接下降所实现的三样东西——`i64 OP i64` 的静态结果类型、操作数形状、内联缓存桩——每个答案在被问处经 `consume()`（唯一的消费点，`fn_gate`）核对，不符即在该运算符所在行具名拒收（例：`arith.ujs:1: operator *: type: net type answered 'f64', not a legal answer here (legal: i64)`）。`??` 的 type 询问（`null == str`）同样被消费：nullish 的 typeof 分派正是 type 网给不出静态类型（illegal）时前端的下降。
3. **为什么只能是闸**：被问的键对每个运算符是固定的，诚实答案只是运算符的函数；字节又被 ujs 记录钉死。任何让答案进入字节的改法都会在诚实网下改字节、破 D2。所以 type/shape/ic 的决策与 enc（`LEGAL_ENC`）、isel 的多数 op（`LEGAL_FORM`）同类：**合法/拒收**，答案不到达字节。本波不把它们说成「选择」。
4. **m4 的怪行如实保留**：`<= > >= != && ||` 的 ic 桩是 `add_i64`，因为 m4 的 ICOP 表把没有 IC op 的运算符映到 `add`；键按 m4 原样问，钉的是诚实答案。

合同随之修订（`adapter/ujswasm/tables.cx` 写 `tables/contract.tsv`）：`flow front parse type shape ic`，不再有 observed 行；`check` 比对「wired = m4 记录的 wired ∪ {type, shape, ic}，observed 为空」，reserved 仍等于记录（irsel, reloc, scope）。m4 自身（`m4/contract.py`、`m4/selftest.py`、基线的 `selftest`/`expect` 节）是 oracle，不改。

## 3. 实测（scratch，HEAD 0b3530f0 之上的工作副本）

`sh ./unisacc.com -run adapter/ujswasm/ujswasm.cx -- selftest . K/4`，四份各约 17–19 s（含约 14 s 编译，负载 12–16；不分份的全量自测编译后约 10 s）：

| 网 | m4 EXPECT | 实测（flownet） | 首个被拒/改变的程序与原因 |
|---|---|---|---|
| lex | shift | shift | `arith.ujs:1: not a number where lex answered num` |
| parse | shift | shift | `arith.ujs:1: expected break got return` |
| type | nothing | **shift** | `arith.ujs:1: operator *: type: net type answered 'f64' … (legal: i64)` |
| scope | nothing | nothing | 33 个程序无一改变或被拒 |
| shape | nothing | **shift** | `arith.ujs:1: operator *: shape: net shape answered 's_f64' … (legal: s_i64)` |
| irsel | nothing | nothing | 33 个程序无一改变或被拒 |
| ic | nothing | **shift** | `arith.ujs:1: operator *: ic: net ic answered 'div_f64' … (legal: mul_i64)` |
| isel | shift/trap | shift/trap | shift：enc 门控拒 `const`（form br）；trap：`isel trap for op const` |
| enc | shift/trap | shift/trap | shift：`enc for const … answered 'op_slot'`；trap：`enc trap form=const` |
| reloc | nothing | nothing | 33 个程序无一改变或被拒 |

lex、parse、isel、enc 的结果与 m4 记录一致（移植没改它们的决策）；诚实构造网 33/33 逐字节；choose 证明通过。

**测量本身不是摆设**（`sh ./unisacc.com -run tests/flownetw4.cx -- unconsumed .`，约 21 s）：把适配器复制到 scratch，只删 `consume()` 里那一行 `fn_gate`（须恰好命中一次；`consume(` 须恰被 4 处询问调用），问法与链原样保留（即 m4 的侧信道），该副本的 `selftest . type,shape,ic` 红：三网各「none of 33 programs changed or refused」→ `CHANGED (expected shift)`。即：只问不用，量出来就是 nothing。

W2/W3/W6 闸在改后复跑（scratch）：parity 33/33 逐字节 + 迹同、acc 10/10、gold 回退 0；refuse 22、negative 7/7、accepts 11、tables 与 tables-gold 一致、d1 + bounds PASS、d4-adapter 无 python 进程、w6-b1 PASS（`flownet/` 未动）；另 glossarycheck、zeropython（0 升）、script-inventory、inventory-derived、gate-layers、flownet-baseline-file 皆绿。

## 4. scope 与 irsel：可达性分析（裁定 4：不可达即 RESERVED）

**scope。** 前端唯一的询问点是 `bind()`：键为 (`nscope > 1 ? block : mod`, `bind`)，答案只在等于 `illegal` 时拒收（`cannot bind NAME`）。

- 诚实答案：`scope_label` 对 bind 只在 ctx ∈ {switch, expr} 时答 `illegal`；前端的 ctx 只由作用域深度定（`mod` 或 `block`，与 ujs `front/compile.py` 相同），参数、let、for 变量都经同一个 `bind()`。所以 `illegal` 分支对任何程序都不可达——这是 m4 记下的结论，W4 复核成立。
- 变异：shift 把 `ok` 换成 `shadow`（类序 ok, shadow, unbound, readonly, illegal），前端把它当 ok，故 shift 下 nothing（上表）。另做一次不入闸的实验：scope 对每个键都答 `illegal`，`corpus/arrow.ujs` 第一个被拒（`arrow.ujs:1: cannot bind f`）——说明分支在说谎时被消费，但没有诚实输入能到达它。
- 要让它可达，前端须以 fn/for/switch/expr 等真实上下文发问（并对 param/rest/store/label/break/continue 各类问），即改 ujs 前端的绑定模型；把 `shadow`/`unbound`/`readonly` 也改为拒收只会让变异变红，不改变「无诚实程序可达」。按裁定 4，二者都不纳入本需求：**scope 维持 RESERVED**，未虚报升格。

**irsel。** ujs 前端与 m4 的直出路径里没有任何 irsel 询问（适配器同样没有）。要让它决策，须新增询问点；新询问会在账目里多出 (相位, irsel, 命中) 行，与 W1 记录的 m4 迹不同（D2 迹相同即破），且等于改 ujs 前端/发射路径的设计。**irsel 维持 RESERVED**。

**reloc**（不在 W4 的四网内）：直出路径从不询问，维持 RESERVED。

## 5. 闸步（D6：每步 ≤60 s 看门狗）

| 闸步 | 命令 | 证明 |
|---|---|---|
| `flownet-w4-selftest-1` | `ujswasm.cx -- selftest . 1/4` | 诚实构造网 33/33 == 记录；lex、parse、type 的决策种类 == EXPECT_W4；EXPECT_W4 与 m4 记录只差三网 |
| `flownet-w4-selftest-2` | `… selftest . 2/4` | scope（nothing）、shape（shift） |
| `flownet-w4-selftest-3` | `… selftest . 3/4` | irsel（nothing）、ic（shift）、isel（shift/trap） |
| `flownet-w4-selftest-4` | `… selftest . 4/4` | enc（shift/trap）、reloc（nothing）、isel 的 eq/ne 选择证明（node 下两者皆 100） |
| `flownet-w4-unconsumed` | `tests/flownetw4.cx -- unconsumed .` | 删掉唯一消费点的副本被自测量为三网 nothing、红 |

如实记下的边界：(a) 三网的决策是「合法/拒收」，答案不到达字节（§2-3）；(b) 测量是逐网的（m4 口径），不是 W3 的逐键扫荡：逐键扰动在适配器上不可用（适配器的拒收是进程退出，扫荡要求流在进程内具名失败，W3 补 3 边界 (c)）；(c) 适配器的 front 询问 isel（`note_binop`、`??` 的 typeof）答案仍被丢弃——isel 已由 select 流决策，本波不动；(d) `flownet/` 未改一行，导出面未加入口（lane B 的 CXIF v1 不受影响）。
