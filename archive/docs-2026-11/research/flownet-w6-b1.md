# flownet W6 B1：机制与领域分离的收口——行为判据 + 内容钉，文本守卫降为启发式

> 需求：`plans/m4-flownet-cx-module.md` §9 B1 · 执行：cc（lane B）· 2026-10-23 · 全部命令在 scratch 副本里跑（`git archive HEAD` + root `unisacc.com`），每条 `timeout 58`。
> 结论一句话：B1 的判据从「flownet/ 文本里看不出消费方的表」改为两条可机检的事实——**(1) 扰动注入**：消费方金表逐键扰动，tdescprobe 的结果每次都变（判错或具名拒绝），并且**每次都等于诚实 flownet 在同一扰动下的结局**（记录 `tests/flownet-b1-inject.tsv`，196 行，含整次运行的 sha256 前缀），改为表中所列类的扰动从不在标签装载时被拒；46/46 键、196 次扰动 0 次不变、0 次偏离记录；**(2) 内容钉**：flownet/ 的文件集与每个文件的 sha256 钉在 `tests/flownet-manifest.tsv`，任何字节改动都必须在同一个、经审阅的提交里改钉——钉是审阅控制，不是判据（§4）。`tests/flownetw6.cx b1` 文本守卫保留为启发式，下面 §3 是它被实测确认的极限 G1–G7。
> 收口补（验证方反驳第一版收口，§2.1 (c)(d)、§2.2）：第一版 (1) 只要求「变了且判错/具名拒绝」，验证方的 X1——带 adj 的 X-macro 副本、核对注入的标签、不一致就让 flownet 以「标签源不给标签」具名拒绝——每次扰动都「变了、具名拒绝」而过闸；与 W3 补 4 同类（交叉核对）。现改为逐扰动对记录，X1 作为第四个自证副本。

## 1. 为什么文本守卫不能是判据

B1 core（0b3530f0、25060ad2、17fd38f2）把 TDESC 表搬到消费方 `tests/tdescprobe/`，经 `fn_label_source` 注入。守卫 `flownetw6 b1` 先后三版（只看 `.cx` 文本与契约名 → 扫全文与禁用词 → 按词比对、spread、数字规则），每一版都被验证方用改写格式的同一张表绕过：X-macro、enum + 矩阵、`A3->F` 箭头、网格表。根因不是规则不够多：**「flownet/ 里没有伪装的表」是一个关于不存在的命题，任何词法规则都只能枚举有限种写法**，而同一份数据的写法无穷。所以判据必须换成不依赖写法的东西：

- 如果 flownet 的答案来自它自己带的副本，那么改消费方注入的表，结果不会变——这是可以跑出来的（§2.1 (a)）。
- 如果 flownet 拿副本核对注入的答案、不一致就自己拒绝，那么结果会变，但变得与诚实的 flownet 不同——把诚实者对每次扰动的结局记下来逐行比，也是可以跑出来的（§2.1 (c)(d)；第一版收口只查「变了且具名」，被验证方的 X1 绕过）。
- 如果 flownet 带了副本却从不用它，任何行为都看不出来；那就让 flownet/ 的每个字节都不能悄悄改——这是钉（§2.2）。

## 2. 判据

### 2.1 扰动注入（`tests/flownetb1.cx inject`，闸 `flownet-w6-b1-inject-1/-2`）

- tdescprobe 编译一次（旁边一个指向 ROOT/flownet 的链接，与它在树里的样子相同），在只装着 `tests/tdescprobe/`（contract.tsv + 契约所列各网金表，从 ROOT 读）的 scratch 目录里跑 `--oracle cc`；工作目录写相对路径，不同运行的输出里不会因路径不同而“变”。
- 对照：原表，必须绿（106 探针 0 不符；39 声明 23 合法，78 无尺寸，0 不符）。
- 扰动：每张金表的每个键行，逐个改成本表 class 行里的**每一个其他类**，再加**删掉该行**；一次只动一行。键数从表里数，不写死：今天是 adj 12、ap 20、sz 14 = **46 键**，150 次改类 + 46 次删行 = **196 次**，对半分两步（每步约 29 s，含约 9 s 编译；8 路并行，每次运行上限 30 s）。
- 每次扰动必须同时满足四条：
  - **(a) 变了**：与对照不同（rc、stdout、stderr 任一）。与对照相同 → 红，点名该键（「这个键的答案不来自注入的表」）。
  - **(b) 判错或具名拒绝**：**判错**（rc 1，emit 钩子对 cc 报 `MISMATCH`）或**具名拒绝**（rc 2，一行 `tdescprobe: …`：flownet 的标签源拒绝如 `label source adj: key (P,P): the label source gives no label`，或 `fn_expect` 钉住的账目拒绝如 `books: row 1 is phase declare net adj hits 68; the consumer pinned declare adj 77`，或越域询问 `ask sz: field of got 'F:36', not in its domain`）。变了但既非判错也非具名拒绝（崩溃、超时）→ 红。
  - **(c) 改类不在装载时被拒**：改成表中所列的类时，结局不得是 `flownet: label source …` 拒绝。诚实的 flownet 对标签源给的任何所列类照单全收（实测 150 次改类 0 次装载拒绝；装载拒绝只出现在 46 次删行上：「不给标签」），所以装载时拒一个合法类，就是拿副本核对了注入的答案 → 红，点名该键。
  - **(d) 等于记录**：本次运行的结局行 `NET KEY FROM>TO KIND H LINE`（KIND 判错/拒绝，H 是整次运行 rc+stdout+stderr 的 sha256 前 16 位，LINE 是 MISMATCH 首行或拒绝行）逐字等于 `tests/flownet-b1-inject.tsv` 里同序号的那一行；行数必须等于从表里数出的扰动数（表变了而记录没重录 → 红，指明重录命令）。
- 记录由诚实的 flownet 写出（`inject ROOT record 1/2 > tests/flownet-b1-inject.tsv`，`record 2/2 >>`，各一步；(a)–(c) 不成立时不写、rc 1），逐行审阅后与改动同一提交落地——与 W3 补 4 的 `tests/flownet-tdesc-sweep.tsv` 同一机理。确定性：记录与随后两次检查逐字节相同（工作目录写相对路径，输出里没有时间与路径）。
- (a) 抓「从副本作答」；(b)–(d) 抓「拿副本核对、不一致就自己让运行失败」：这样的副本每次都「变了、具名拒绝」，但它的失败不是诚实 flownet 的失败——(c) 抓它在装载时拒合法类，(d) 抓任何其他方式（另一条拒绝路径、另一个 MISMATCH、输出里任何一个字节）。全部 196 次都等于记录，意味着 flownet 在这些扰动上的行为与诚实者**完全相同**，即它的答案就是注入的表的答案。
- 实测（scratch）：1/2 98/98 变（判错 1、拒绝 97），2/2 98/98 变（判错 51、拒绝 47），不变 0，无名 0，装载拒合法类 0，等于记录 98/98 + 98/98，偏离 0。
- **关于「80 键」**：裁定写的是 80 个 adj/ap/sz 键。那是 W3 补 3 之前的旧域；W3 补 3 把域收紧到推导恰问的 46 键（旧 80 键里 34 键从不被问，扰动它们本来就不可能改变任何结果），这 34 键今天不在任何表里。本闸覆盖现存的全部键，且每键覆盖全部其他类与删行，没有缩小判据；键数从表里数，表扩大时闸随之扩大。

### 2.2 自证（`inject ROOT proof K/4`，闸 `flownet-w6-b1-proof-1/-2/-3/-4`）

四份 flownet 副本：把 `flownet/nets.cx` 里唯一的一处标签询问 `lab = label(ctx, N->name, N->nf, k);` 精确改一次，先照常问注入的标签源（所以消费方「每行恰被问一次」的核对照样过），再用自己带的副本——前三份覆盖答案，第四份核对答案：

| 份 | 副本 | 它自己的闸（对照 `--oracle cc`） | 每键一次改类后偏离记录的键 | 其中与对照相同 / 具名拒绝 | 结果 |
|---|---|---|---|---|---|
| 1/4 | 全部三张表，作数据（闸运行时从消费方金表写出），作答 | 绿 | 46/46 | 46 / 0 | 红，逐键点名 |
| 2/4 | 验证方的 G7（adj 的 X-macro），原文照搬，作答 | 绿 | 恰 adj 的 12 键 | 12 / 0 | 红，逐键点名 |
| 3/4 | 最后一张表的最后一键（今天是 sz `F:8`），作答 | 绿 | 恰 1 键 | 1 / 0 | 红，点名 `sz F:8` |
| 4/4 | 验证方的 X1，原文照搬：G7 的行核对注入的 adj 标签，不一致即「无标签」，flownet 具名拒绝 | 绿 | 恰 adj 的 12 键 | 0 / 12 | 红，逐键点名 |

闸要求「偏离记录的键」**恰好等于**副本所带的键，多一个少一个都红。4/4 那一列说明了第一版收口为什么漏：X1 的 12 次扰动全都「变了、具名拒绝」，只看 (a)(b) 它是绿的；它在 (d) 上偏离记录（记录里是账目拒绝 `books: row 1 … hits 68 …` 或 MISMATCH，X1 给的是 `label source adj: key (P,P): the label source gives no label`），在 (c) 上是装载时拒合法类。每份约 22 s。编辑点在 nets.cx 里找不到或不唯一 → 红（flownet 改了，变异要跟着改）。

另在 scratch 里把验证方的 X1 `nets.cx` 原样放进 flownet/ 跑真闸（inject 闸不读钉，等同验证方 `pin . write` 之后的情形）：`inject 1/2` 红 12，逐键点名 adj 的 12 键（「a relabel to a class its table lists, refused at label load」）；去掉 (c) 只留 (d) 再跑，同样红 12（「the run is not the one tests/flownet-b1-inject.tsv records」）——两条各自都抓得住 X1。

### 2.3 内容钉（`tests/flownetb1.cx pin`，闸 `flownet-w6-b1-pin`，记录 `tests/flownet-manifest.tsv`）

- `file NAME SIZE SHA256`：flownet/ 的每个条目必须是普通文件、在钉里、大小和 sha256 相同；钉里的文件必须都在。
- `reach DIR`：消费方经由它们够到 flownet 的模块链接目录（`tests/flownet`、`adapter/ujswasm/flownet`）只许放链接，每个链接解析到同名的已钉文件，每个已钉 `.cx` 都有链接——所以消费方编译进去的 flownet 就是钉住的字节。
- 先查 ROOT（每处具名），再在 ROOT 的 scratch 副本上自证 9/9：原样绿；改一个字节、加文件、删文件、改名、flownet/ 里放链接、放目录、reach 链接改指副本、reach 目录里放普通文件——各自给出点名该条目与原因的发现。
- 改 flownet/ 的流程：改完跑 `sh ./unisacc.com -run tests/flownetb1.cx -- pin . write > tests/flownet-manifest.tsv`，逐行审阅差异，与 flownet/ 的改动同一提交落地。钉不判断改动好坏，它保证改动**不能不被看见**：任何字节变化都会出现在 manifest 的 diff 里。
- 实测：G1–G7 每一个放进 flownet/（`.md` 作 `flownet/notes.md`，`.cxpart` 接在 `flownet/fnx.cx` 末尾）都被钉点名（`flownet/notes.md: not in tests/flownet-manifest.tsv` 或 `flownet/fnx.cx: … bytes, sha256 …; … pins …`）。

## 3. 文本守卫的极限：验证方变异 G1–G7（实测）

`tests/flownetw6.cx b1` 仍在闸里（`flownet-w6-b1`），作启发式：它能便宜地挡住把表原样放进 flownet/ 的明显情形（表文件、目录、链接、消费方名字、整行金表、项目符号、箭头、散文、int 数组……，见其头注）。它**不是** B1 的判据。下面七个变异都是 adj（或 ap、sz）同一份数据的改写，放进 flownet/ 后守卫全部 PASS（scratch 实测，`b1` 每次约 6 s，rc 0）；验证方后来的 X1（G7 作核对，§2.2）同样过守卫：

| 变异 | 形式 | 放在 | 文本守卫 | 钉 | 扰动注入 |
|---|---|---|---|---|---|
| G1 | adj 作 Markdown 网格（行 OP、列 HOLDS） | `flownet/notes.md` | PASS | 红 | 只是文档、不被用：行为看不到，归钉 |
| G2 | adj 作注释 `may[A3][F] = bad` | `flownet/fnx.cx` | PASS | 红 | 同上 |
| G3 | adj 作 `- A3->F: bad` 列表 | `flownet/notes.md` | PASS | 红 | 同上 |
| G4 | ap 作 `* A3.P[index] = next` | `flownet/notes.md` | PASS | 红 | 同上 |
| G5 | sz 作 `* A3:36=108` | `flownet/notes.md` | PASS | 红 | 同上 |
| G6 | adj 作 enum + 3×4 矩阵 `may_tab` | `flownet/fnx.cx` | PASS | 红 | 未单独实测；若被用作答案，与 proof 2/4 同机理（注入的表改了答案不变），若用作核对，与 proof 4/4 同机理 |
| G7 | adj 作 X-macro `MAY_TABLE` | `flownet/fnx.cx` | PASS | 红 | 被用作答案即 proof 2/4、被用作核对即 X1 = proof 4/4：皆红，恰点名 12 个 adj 键 |

为什么词法规则抓不住它们：网格把一行金表拆到行头、列头和格子三处，没有一行含整行的词序；`may[A3][F]`、`A3->F`、`A3.P[index]`、`A3:36=108` 把键字段粘成一个词，词切分后与金表的词不相等；enum + 矩阵把类名换成标识符、把键换成下标位置；X-macro 每行的词序与金表行相同，但 `X(` 与 `\` 夹在中间，且出现在宏体里而非字面量或注释。每补一条规则，下一种写法又在规则之外——这正是 §1 的结论。

## 4. 覆盖边界（如实）

- 扰动注入证明的是 **tdescprobe 的 46 个键、每键全部单行扰动**：在这 196 次扰动上 flownet 的整次运行（rc、stdout、stderr 的 sha256）与诚实的 flownet 相同。由此，flownet 带的副本无论用来**作答**（proof 1/4–3/4：运行等于对照）还是**核对**（X1，proof 4/4：运行变成另一种具名拒绝），只要在某个单行扰动上起作用，就偏离记录而红。
- 行为看不见的副本，只能由钉兜底，如实列出三类：(i) 带了但从不用（G1–G5 那样的文档、死代码）；(ii) 只在单行扰动造不出来的输入上起作用（例如只在两行以上同时不一致、或在表里多出一行时才核对/作答）——证明的输入集合是单行扰动，这之外的行为不在记录里；(iii) 不进 tdescprobe 的网：ujs 适配器的网由它自己的表经同一个 `fn_label_source` 注入，W4 的 selftest（`flownet-w4-selftest-*`）以「说谎的标签源」逐网证明 lex/parse/type/shape/ic/isel/enc 决策，是**网粒度**而非键粒度；scope、irsel、reloc 维持 RESERVED、不决定任何输出。
- **钉是审阅控制，不是判据。** 它不判断改动好坏，分不出 X1 与一次正当的修改：验证方 `pin . write` 之后 X1 的钉就是绿的。它保证的只是 flownet/ 的任何字节变化都以 manifest 的 diff 出现在同一个经审阅的提交里——(i)–(iii) 的副本要进来，必须被人看见。判据是扰动注入 + 记录；钉不替它下结论。
- 记录本身由诚实的 flownet 写出、经审阅提交；一份带副本的 flownet 若被用来重录记录，记录也会跟着「说谎」。所以 `tests/flownet-b1-inject.tsv` 与 `tests/flownet-manifest.tsv` 一样只能在审阅过的提交里改；而 (c)（装载时不拒合法类）不依赖记录，重录时就把 X1 这一类挡在记录之外（`record` 在 (a)–(c) 不成立时不写、rc 1）。
- 钉只看 flownet/ 与两个 reach 目录。消费方自己的代码（如 tdescprobe.cx 的推导）是否偷偷带表，是 W3 扫荡（`flownet-w3-flow-*`、`tests/flownet-tdesc-sweep.tsv`）的事，不是 B1 的。

## 5. 闸步

| 闸 | 命令 | 约时 |
|---|---|---|
| flownet-w6-b1-inject-1 | `tests/flownetb1.cx -- inject . 1/2`（对记录 `tests/flownet-b1-inject.tsv`） | 28 s |
| flownet-w6-b1-inject-2 | `tests/flownetb1.cx -- inject . 2/2`（同上） | 28 s |
| flownet-w6-b1-proof-1 | `tests/flownetb1.cx -- inject . proof 1/4` | 22 s |
| flownet-w6-b1-proof-2 | `tests/flownetb1.cx -- inject . proof 2/4` | 22 s |
| flownet-w6-b1-proof-3 | `tests/flownetb1.cx -- inject . proof 3/4` | 22 s |
| flownet-w6-b1-proof-4 | `tests/flownetb1.cx -- inject . proof 4/4`（X1） | 22 s |
| flownet-w6-b1-pin | `tests/flownetb1.cx -- pin .` | 7 s |
| flownet-w6-b1（启发式） | `tests/flownetw6.cx -- b1 .` | 6 s |

重录记录（表或 flownet 的行为有意改变时；各一步，逐行审阅 diff）：`tests/flownetb1.cx -- inject . record 1/2 > tests/flownet-b1-inject.tsv`，`… record 2/2 >> tests/flownet-b1-inject.tsv`。

scratch 目录由 `cxlib.pytempfile` 的 `ptf_mkdtemp` 建在临时目录下，只在名字是本次所建时才由 `ptf_rmtree` 删；ROOT 只读。
