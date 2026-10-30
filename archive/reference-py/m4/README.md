# m4：流×网表驱动的 `\0asm` 最小路径

合同：`research/m4-flow-net-contract-draft.md`。主张弧：gold → IntNet → 四流 → 字节。
旧的 ujs 路径（`ujs/`、`tests/ujs2wasm/`）保留为归档参考，本目录的 `check` 与它逐字节对比，不替换它。

## 命令

```
python3 -m m4 acc                 # 10 张网：构造 + 全键穷举 exact
python3 -m m4 build SRC -o OUT    # 源 → 四流 → \0asm，打印各流网命中
python3 -m m4 check               # tests/ujs2wasm/corpus 逐个对 ujs 的 gold 路径
python3 -m m4 selftest            # 变异测试：每张网是否真的决定输出（基线钉死）
./tests/m4.sh                     # 以上全部，逐步 60 秒看门狗
```

## 自检结论（selftest，实测）

10 张网全部构造并穷举验收（acc），但**只有 4 张真的决定输出或拒收**。合同按此分类（`contract.py`）：

| 网 | 类别 | 实测（变异） | 依据 |
|---|---|---|---|
| lex | 流 1 决策 | shift：改字节或拒收 | 词法分类 |
| parse | 流 2 决策 | shift：改字节或拒收 | 产生式选择 |
| isel | 流 3 决策 | shift、trap：拒收；另有选文本 | `emit.py` 按形态分派。多数 op 合法形态仍唯一。`eq`/`ne` 同时接受 `i64_cmp`（gold，与 ujs 逐字节）和 `f64_cmp`（另一段 WAT，Node 上 `ternary.ujs` 仍得 100） |
| enc | 流 4 决策 | shift、trap：拒收 | `emit.py` 每条指令按钉死的合法模板门控；合法模板唯一 |
| type、shape、ic | 观测（OBSERVED） | 无 | `front/compile.py` 的 `note_binop` 以侧信道发出，结果被丢弃 |
| scope | 保留（RESERVED） | 无 | `front.bind` 只检查 `illegal`，调用点 ctx 恒为 mod/block，该分支不可达 |
| irsel | 保留（RESERVED） | 无 | ujs 中无 `ask("irsel")` |
| reloc | 保留（RESERVED） | 无 | 直出路径从不询问，只有 `lower_wasm` 询问 |

**这是对草案“4 流 × 10 网”的实测修订：只有 4 张网在决策。** 口径是否改由政委定；本文件如实记录。

决策的性质：enc 仍是“合法/拒收”（每个 op 的合法模板唯一）。isel 对 `eq`/`ne` 已经在两个合法形态之间选文本（`contract.ISEL_CHOICE`）；selftest 的 choose 探针钉死“字节不同且都能跑”。其余 op 的 isel 仍是唯一形态，shift 落到别的类就拒收。

要让 type、scope、shape、ic 真正决策，需要改 ujs 前端，让它消费这些答案（例如用 type 的结果做检查、让 scope 的非 ok 分支可达）。这是 ujs 侧的设计决定，不在 m4 内解决。

`EXPECT`（selftest.py）把上表钉死：某网的决策种类一旦变化，闸就失败，逼着同步更新口径。

## 结构

| 文件 | 作用 |
|---|---|
| `contract.py` | 10 网名单、4 流决策网、保留网、观测网，划分自检 |
| `nets.py` | 复用 `ujs.seed.construct.gold.STAGES` 构造 IntNet（不登记进 unisa 默认 18）；`acc` 穷举 |
| `flows.py` | 源 → 前端 compile（流 1+2）→ 直出 emit（流 3+4）；运行时记账并检查 |
| `__main__.py` | CLI |

## 实测结果（2026-10-19，本地）

- `acc`：10/10 张网全部穷举 exact（lex 100、parse 330、type 1863、scope 48、shape 54、irsel 117、ic 975、isel 42、enc 144、reloc 4 键）。
- `check`：tests/ujs2wasm/corpus 28 个程序，m4 输出与 ujs gold 路径**逐字节相同 28/28**，0 差异，0 跳过。
- 运行时强制：每个阶段命中的网都属于接线集合，且任一网的 gold 回退计数为 0（即确实由网驱动）。

## 对合同草案的修订（由实测得出）

1. **ic 不是可选。** 前端 `front/compile.py:83` 会询问 `ic`，所以接入 front 流。草案 §3 的“可选”不成立。
2. **irsel 确实未接线。** `ujs` 里没有任何 `ask("irsel")`，保留（表核、不运行）与草案默认一致。
3. **阶段边界只是记账。** `front/compile.py:89` 在前端也会询问 `isel`（如 nullish），所以流 2 与流 3 的网并不互斥。运行时只强制“命中须在接线集合内”，不强制流间独占。

## 边界与未做的事

- 流 1+2 共用一次 `compile_src`，流 3+4 共用一次 `emit_wasm`。合同里四段的边界是逻辑上的；没有把它们拆成独立的函数调用。
- 这是**方法证明的最小路径**，不是产品：不替换 Pages 出货，不改 unisacc，不训练，不投稿。
- 只用了 ujs 的 28 个语料；未覆盖的程序会在 `build` 时明确报错，不静默回退。
