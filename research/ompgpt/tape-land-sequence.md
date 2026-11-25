# tape/m0 落地主序

## 合并边界

基线 `fd3994e9a659ac76cf397e04a8421785d6634ad4`（批次容量 4096）。已通过 devsvc 合入 `tape/m0`，`GIT_MERGE rc=0 commit=e311594540a7776f2f7d7edc13851c58f52da2e6 branch=tape/m0`。agy-flow 已把写窗交出；合并前按其授权备份草案、移开旧清单与撤回旧登记行，保留两份原始源码。旧登记备份在本轮私有目录 `/tmp/tape-land.sAgVCco4/`，源码另见 `~/.local/share/unisacc/research/agy-flow/`。

用同一个 seed identity helper 对基线归档树与合并后的工作树实算，均为 `8e48f954438681f1bd5e21b0708228d3890dd42af2954397dc7daa9b64889831`。实际差集只有 `tools/`、`tests/`；`seed/ident.c:281-294` 的支持路径白名单未包含 `tools/tape/asm/`、`tools/tapepack.cx` 或 `tools/tapemode-run.cx`。也未在基线产品树、候选触发清单或核心闭包表找到对这三项的引用。`research/agy-h/toolline-series/0002-gate23-pik1.patch` 草案不把这些路径归入 PIK1；正式 `tools/chain-ready.cx:1610-1615` 尚无这一豁免，仍拒绝落后于 `origin/main` 的分支。这是工具线，不需重构产品，但只能在无发布冻结时由 cc 集成，不改在途车的祖先约束。

## 拆笔与落地命令

(1) 保留已审内核与容器合约历史：包括三表、启动台与现有操作码对拍至 `4852f20a1`。不把产品路径、r1 二进制私有语料或速度结果附进合并。

(2) tapepack：提交 `49592cc39af4c94f4c4eeb30dd794b93d20636e9`。两份原稿、`prd.md` 能力行、覆盖、库存和批清单共七路径；`tools-compile-195` 增一行。干净资格树 `f4035b0ec68aefdccab32ae8b92f44e63f3c2ac2` 实跑 `REGISTER_LAYOUT_SELFTEST id=tapepack status=PASS ms=14219`；此前 `BATCH_RUN id=tapepack status=PASS rc=0 assertions=19`。编译库存共 1565 源、196 片。新版 source、容器或检查都不是缓存结论。

(3) 速度轻量合同：提交 `51023e8a4aac30b0ea0aacd46a9cad12ba4c25db`，五路径。冷资格树 `a03a852f44d79a45b24225acfde3e892bc797a27` 实跑 `REGISTER_LAYOUT_SELFTEST id=tape-tldce-speed-contract status=PASS ms=13430`，最终私有索引 `PRECOMMIT staged_cx=1 blocked=0`。登记 argv 只含 `--self-test`，覆盖固定清单顺序、改字节拒绝、精确 15 倍界线、探针和高负载不报资格通过。正式模式对未绑定的预构造材料复用具名拒绝，CLI 实跑 `T14C_REUSE_NEG_RC=2`；正式测速不进提交门。回执见同目录 `tape-land-evidence/`。

(4) `tape-tldce-lite16` 未登记：历史 16 件清单所绑的 g2 对象目录不存在；恢复的 r1 是另一份私有字节语料，不能代替历史输入，也没有干净资格布局的材料生产者。私有三方对拍证据保留，不用玩具 ELF 或历史 PASS 代替。恢复入口须先提供审批后的固定语料（清单顺序、逐件 SHA 和来源均可验），准备材料再用真实三方 cmp、SHA 和 `cut>0` 自动冷跑小于 25 秒方可登记。详见同目录 `t14b-lite-batch.md`。15 倍实测未过（16 件 94.037 倍，64 件 114.316 倍；102 件未完成）。

(5) 仅 cc 在当前主线无冻结并收到 cdx 复核后整合；不要把未提交工作树作为合并对象。每笔只归属相应路径；若需重新在私有集成树提交，则先预热 `./tools/bound 55 sh ./unisacc.com tools/affected.cx -- --registry-check`，再用 `sh ./unisacc.com tools/devsvc.cx -- git commit -m '…' --whole 路径1 路径2 …`。共享表选键而不是带其他窗口行。最终从 main 跑 `sh ./unisacc.com tools/devsvc.cx -- git merge <已审提交>`，同 helper 比较合并前后 SID，核派生和实际批，不推未审工件。若整合批次有问题，用前向回退提交，不重写远端，保留本分支证据。

(6) 联合私有索引 `PRECOMMIT staged_cx=3 blocked=0`；`lint(report-only) R1-product tools/check-tapepack.cx:262` 是检查器调用检出内产品，不是阻断。`derive --check --only tables-gen,table-index` 为 `ok=2 drift=0 refused=0`。已知 `origin/main` 从 `fd3994e9a` 前移至 `772b6e9eb` 时仅改 devsvc 三个工具，不覆盖 tape 或登记；落地当天仍应以实时远端祖先、实际路径差集、最新合并索引验收为准。

## 集成命令

以下仅由 cc 在共享主线执行。两笔已经在 tape 分支提交，不需在 main 重提交或重新登记。若实际主线差集与上述记录不同，先在私有合并索引重新核对，不覆盖别的窗口的登记。

```sh
set -eu
target=51023e8a4aac30b0ea0aacd46a9cad12ba4c25db
helper="$HOME/.cache/unisacc/prep-cache/identity-helper-b9ed85e8f3890fa8.bin"
test "$(git branch --show-current)" = main
test -z "$(git status --porcelain)"
git merge-base --is-ancestor origin/main HEAD
before=$("$helper" "$PWD")
./tools/bound 55 sh ./unisacc.com tools/affected.cx -- --registry-check
./tools/bound 55 sh ./unisacc.com tools/devsvc.cx -- git merge "$target" -m '合入 tape 容器工具与轻量回归'
test "$before" = "$("$helper" "$PWD")"
./tools/bound 55 sh ./unisacc.com tools/derive.cx -- --check --only tables-gen,table-index
```

逐笔复核需重放冷资格时，从 tape 工作树分别运行：

```sh
./tools/bound 55 sh ./unisacc.com tools/register.cx -- --qualify-tree 49592cc39af4c94f4c4eeb30dd794b93d20636e9 tapepack
./tools/bound 55 sh ./unisacc.com tools/register.cx -- --qualify-tree 51023e8a4aac30b0ea0aacd46a9cad12ba4c25db tape-tldce-speed-contract
```

## 分相表交接

(1) 重建：`~/.local/share/unisacc/research/ompgpt/quiet-speed/phase-times-v010-g2.tsv`。

(2) 正式静默测速：`~/.local/share/unisacc/research/ompgpt/quiet-speed/v010-r1-quiet-evidence/phase-times.tsv`。由 cc 单写合入 `build/phase-times.tsv`，不与 tape 批一并写共享表。
