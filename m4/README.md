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

## 自检结论（selftest，2026-10-19 实测）

| 网 | 决策种类 | 含义 |
|---|---|---|
| lex | shift | 说谎会改字节或拒收：真决定输出 |
| parse | shift | 同上 |
| isel | trap | 只决定拒收；字节不随其 form 变化 |
| enc | trap | 只决定拒收；字节不随其模板变化 |
| type, scope, shape, ic, reloc | 无 | 说谎不改变语料上的任何结果：未证明在决策 |

这是**方法轨的真缺口**：`emit_wasm` 的字节由写死的 op 处理生成，`isel`/`enc` 只做覆盖检查。
要让网真决定字节，需要让 emit 消费 form 与模板（表驱动的指令选择与编码），这一步尚未做。
`EXPECT` 把现状钉死：某网的决策种类一旦变化，闸就失败，逼着同步更新口径。

## 结构

| 文件 | 作用 |
|---|---|
| `contract.py` | 10 网名单、4 流绑定、保留网、接线自检 |
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
