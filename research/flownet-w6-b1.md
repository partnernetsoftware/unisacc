# flownet W6 B1：机制与领域分离的收口——行为判据 + 内容钉，文本守卫降为启发式

> 需求：`plans/m4-flownet-cx-module.md` §9 B1 · 执行：cc（lane B）· 2026-10-23 · 全部命令在 scratch 副本里跑（`git archive HEAD` + root `unisacc.com`），每条 `timeout 58`。
> 结论一句话：B1 的判据从「flownet/ 文本里看不出消费方的表」改为两条可机检的事实——**(1) 扰动注入**：消费方金表逐键扰动，tdescprobe 的结果每次都变（判错或具名拒绝），46/46 键、196 次扰动 0 次不变；**(2) 内容钉**：flownet/ 的文件集与每个文件的 sha256 钉在 `tests/flownet-manifest.tsv`，任何字节改动都必须在同一个、经审阅的提交里改钉。`tests/flownetw6.cx b1` 文本守卫保留为启发式，下面 §3 是它被实测确认的极限 G1–G7。

## 1. 为什么文本守卫不能是判据

B1 core（0b3530f0、25060ad2、17fd38f2）把 TDESC 表搬到消费方 `tests/tdescprobe/`，经 `fn_label_source` 注入。守卫 `flownetw6 b1` 先后三版（只看 `.cx` 文本与契约名 → 扫全文与禁用词 → 按词比对、spread、数字规则），每一版都被验证方用改写格式的同一张表绕过：X-macro、enum + 矩阵、`A3->F` 箭头、网格表。根因不是规则不够多：**「flownet/ 里没有伪装的表」是一个关于不存在的命题，任何词法规则都只能枚举有限种写法**，而同一份数据的写法无穷。所以判据必须换成不依赖写法的东西：

- 如果 flownet 的答案来自它自己带的副本，那么改消费方注入的表，结果不会变——这是可以跑出来的（§2.1）。
- 如果 flownet 带了副本却从不用它，任何行为都看不出来；那就让 flownet/ 的每个字节都不能悄悄改——这是钉（§2.2）。

## 2. 判据

### 2.1 扰动注入（`tests/flownetb1.cx inject`，闸 `flownet-w6-b1-inject-1/-2`）

- tdescprobe 编译一次（旁边一个指向 ROOT/flownet 的链接，与它在树里的样子相同），在只装着 `tests/tdescprobe/`（contract.tsv + 契约所列各网金表，从 ROOT 读）的 scratch 目录里跑 `--oracle cc`；工作目录写相对路径，不同运行的输出里不会因路径不同而“变”。
- 对照：原表，必须绿（106 探针 0 不符；39 声明 23 合法，78 无尺寸，0 不符）。
- 扰动：每张金表的每个键行，逐个改成本表 class 行里的**每一个其他类**，再加**删掉该行**；一次只动一行。键数从表里数，不写死：今天是 adj 12、ap 20、sz 14 = **46 键**，150 次改类 + 46 次删行 = **196 次**，对半分两步（每步约 29 s，含约 9 s 编译；8 路并行，每次运行上限 30 s）。
- 每次扰动必须：与对照不同（rc、stdout、stderr 任一），并且是**判错**（rc 1，emit 钩子对 cc 报 `MISMATCH`）或**具名拒绝**（rc 2，一行 `tdescprobe: …`：flownet 的标签源拒绝如 `label source adj: key (P,P): the label source gives no label`，或 `fn_expect` 钉住的账目拒绝如 `books: row 1 is phase declare net adj hits 68; the consumer pinned declare adj 77`，或越域询问 `ask sz: field of got 'F:36', not in its domain`）。与对照相同 → 红，点名该键（「这个键的答案不来自注入的表」）；变了但既非判错也非具名拒绝（崩溃、超时）→ 红。
- 实测（scratch）：1/2 98/98 变（判错 1、拒绝 97），2/2 98/98 变（判错 51、拒绝 47），不变 0，无名 0。
- **关于「80 键」**：裁定写的是 80 个 adj/ap/sz 键。那是 W3 补 3 之前的旧域；W3 补 3 把域收紧到推导恰问的 46 键（旧 80 键里 34 键从不被问，扰动它们本来就不可能改变任何结果），这 34 键今天不在任何表里。本闸覆盖现存的全部键，且每键覆盖全部其他类与删行，没有缩小判据；键数从表里数，表扩大时闸随之扩大。

### 2.2 自证（`inject ROOT proof K/3`，闸 `flownet-w6-b1-proof-1/-2/-3`）

三份 flownet 副本：把 `flownet/nets.cx` 里唯一的一处标签询问 `lab = label(ctx, N->name, N->nf, k);` 精确改一次，先照常问注入的标签源（所以消费方「每行恰被问一次」的核对照样过），再用自己带的副本覆盖答案：

| 份 | 副本 | 它自己的闸（对照 `--oracle cc`） | 每键一次改类后不变的键 | 结果 |
|---|---|---|---|---|
| 1/3 | 全部三张表，作数据（闸运行时从消费方金表写出） | 绿 | 46/46 | 红，逐键点名 |
| 2/3 | 验证方的 G7（adj 的 X-macro），原文照搬 | 绿 | 恰 adj 的 12 键 | 红，逐键点名 |
| 3/3 | 最后一张表的最后一键（今天是 sz `F:8`） | 绿 | 恰 1 键 | 红，点名 `sz F:8` |

闸要求「不变的键」**恰好等于**副本所带的键，多一个少一个都红。每份约 22 s。编辑点在 nets.cx 里找不到或不唯一 → 红（flownet 改了，变异要跟着改）。

### 2.3 内容钉（`tests/flownetb1.cx pin`，闸 `flownet-w6-b1-pin`，记录 `tests/flownet-manifest.tsv`）

- `file NAME SIZE SHA256`：flownet/ 的每个条目必须是普通文件、在钉里、大小和 sha256 相同；钉里的文件必须都在。
- `reach DIR`：消费方经由它们够到 flownet 的模块链接目录（`tests/flownet`、`adapter/ujswasm/flownet`）只许放链接，每个链接解析到同名的已钉文件，每个已钉 `.cx` 都有链接——所以消费方编译进去的 flownet 就是钉住的字节。
- 先查 ROOT（每处具名），再在 ROOT 的 scratch 副本上自证 9/9：原样绿；改一个字节、加文件、删文件、改名、flownet/ 里放链接、放目录、reach 链接改指副本、reach 目录里放普通文件——各自给出点名该条目与原因的发现。
- 改 flownet/ 的流程：改完跑 `sh ./unisacc.com -run tests/flownetb1.cx -- pin . write > tests/flownet-manifest.tsv`，逐行审阅差异，与 flownet/ 的改动同一提交落地。钉不判断改动好坏，它保证改动**不能不被看见**：任何字节变化都会出现在 manifest 的 diff 里。
- 实测：G1–G7 每一个放进 flownet/（`.md` 作 `flownet/notes.md`，`.cxpart` 接在 `flownet/fnx.cx` 末尾）都被钉点名（`flownet/notes.md: not in tests/flownet-manifest.tsv` 或 `flownet/fnx.cx: … bytes, sha256 …; … pins …`）。

## 3. 文本守卫的极限：验证方变异 G1–G7（实测）

`tests/flownetw6.cx b1` 仍在闸里（`flownet-w6-b1`），作启发式：它能便宜地挡住把表原样放进 flownet/ 的明显情形（表文件、目录、链接、消费方名字、整行金表、项目符号、箭头、散文、int 数组……，见其头注）。它**不是** B1 的判据。下面七个变异都是 adj（或 ap、sz）同一份数据的改写，放进 flownet/ 后守卫全部 PASS（scratch 实测，`b1` 每次约 6 s，rc 0）：

| 变异 | 形式 | 放在 | 文本守卫 | 钉 | 扰动注入 |
|---|---|---|---|---|---|
| G1 | adj 作 Markdown 网格（行 OP、列 HOLDS） | `flownet/notes.md` | PASS | 红 | 只是文档、不被用：行为看不到，归钉 |
| G2 | adj 作注释 `may[A3][F] = bad` | `flownet/fnx.cx` | PASS | 红 | 同上 |
| G3 | adj 作 `- A3->F: bad` 列表 | `flownet/notes.md` | PASS | 红 | 同上 |
| G4 | ap 作 `* A3.P[index] = next` | `flownet/notes.md` | PASS | 红 | 同上 |
| G5 | sz 作 `* A3:36=108` | `flownet/notes.md` | PASS | 红 | 同上 |
| G6 | adj 作 enum + 3×4 矩阵 `may_tab` | `flownet/fnx.cx` | PASS | 红 | 未单独实测；若被用作答案，与 proof 2/3 同机理（注入的表改了答案不变）|
| G7 | adj 作 X-macro `MAY_TABLE` | `flownet/fnx.cx` | PASS | 红 | 被用作答案即 proof 2/3：红，恰点名 12 个 adj 键 |

为什么词法规则抓不住它们：网格把一行金表拆到行头、列头和格子三处，没有一行含整行的词序；`may[A3][F]`、`A3->F`、`A3.P[index]`、`A3:36=108` 把键字段粘成一个词，词切分后与金表的词不相等；enum + 矩阵把类名换成标识符、把键换成下标位置；X-macro 每行的词序与金表行相同，但 `X(` 与 `\` 夹在中间，且出现在宏体里而非字面量或注释。每补一条规则，下一种写法又在规则之外——这正是 §1 的结论。

## 4. 覆盖边界（如实）

- 扰动注入证明的是 **tdescprobe 的 46 个键**：flownet 对每个键给出的答案都来自注入的表。ujs 适配器的网由它自己的表经同一个 `fn_label_source` 注入；W4 的 selftest（`flownet-w4-selftest-*`）以「说谎的标签源」逐网证明 lex/parse/type/shape/ic/isel/enc 决策，是**网粒度**而非键粒度；scope、irsel、reloc 维持 RESERVED、不决定任何输出，flownet 若带它们的副本，行为上看不见，只由钉兜底。
- 带了副本但从不用（G1–G5 那样的文档、死代码）：行为不可见，由钉保证它进不来而不被审阅。
- 钉只看 flownet/ 与两个 reach 目录。消费方自己的代码（如 tdescprobe.cx 的推导）是否偷偷带表，是 W3 扫荡（`flownet-w3-flow-*`、`tests/flownet-tdesc-sweep.tsv`）的事，不是 B1 的。

## 5. 闸步

| 闸 | 命令 | 约时 |
|---|---|---|
| flownet-w6-b1-inject-1 | `tests/flownetb1.cx -- inject . 1/2` | 29 s |
| flownet-w6-b1-inject-2 | `tests/flownetb1.cx -- inject . 2/2` | 29 s |
| flownet-w6-b1-proof-1 | `tests/flownetb1.cx -- inject . proof 1/3` | 22 s |
| flownet-w6-b1-proof-2 | `tests/flownetb1.cx -- inject . proof 2/3` | 23 s |
| flownet-w6-b1-proof-3 | `tests/flownetb1.cx -- inject . proof 3/3` | 22 s |
| flownet-w6-b1-pin | `tests/flownetb1.cx -- pin .` | 7 s |
| flownet-w6-b1（启发式） | `tests/flownetw6.cx -- b1 .` | 6 s |

scratch 目录由 `cxlib.pytempfile` 的 `ptf_mkdtemp` 建在临时目录下，只在名字是本次所建时才由 `ptf_rmtree` 删；ROOT 只读。
