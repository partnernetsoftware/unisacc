# 两条「参考宽／产品严」分账（ledger 行，待裁；本文不代裁、不动参考）

2026-10-22 02:5x AWST。来源：com-extent-neg（e79ea3ab）与 com-bindings-neg（41fb49bb）两叶采证中发现，现状已被两叶按登记纪律写死（参考改口即叶红），本文供 ledger 线裁定走向。

## 1. extent ≥ 2^31（族 2）

**实测**（target lnx/x86_64，全部可复现）：
- 产品：单项 `.bss a 2147483648` 与累计 `2000000000+1000000000` 均 rc1、stdout 空、不写输出文件、具名 `not covered: tape data directive`。
- 模型两执行器（`exec/lower/sparsecheck.py` 在册断言）：同拒，诊断含 `tape data directive`。
- 参考（`python3 -m unisa compile --from-tape`）：**接受** rc0，镜像结构自洽——RW 段 `filesz 0 / memsz 2147483816`（累计件 `3000000168`），与 6.1 亿控制件同构；控制件产品/参考逐字节同（4097 B）。

**定性**：参考能真实表达 >2^31 虚区，产品/模型的 2^31 是**决策表上界**；按「不能精确匹配的必须拒绝并报未覆盖」，`not covered` 是诚实答案，不是缺陷。与 underscore（参考语法漂移）**不同类**。

**选项**：
- (B·现状) 保持登记式分账：产品 not covered，参考接受由叶写死。零改动。
- (C) 扩模型决策域到 64 位 extent 上界（与 Tape 整数 64 位字裁定同口径）：动 gold/表，属构造路径，需另授与穷举证据。
- (A) 收紧参考到 2^31：**不建议**——参考能力真实，收紧只是掩盖表的上界，还会使现有大额合法区间失去对照。

**建议：(B) 维持，(C) 列为可另授的扩表刀；不采 (A)。**

**追加（2026-10-22 03:4x，载体路由归属分叉）**：同一 extent 程序在两条路由上拒绝**归属不同**——文本路 `.tape` 归 lower 表（`not covered: tape data directive`），Tapebin 载体路在 ≥2^31 时被产品**解码器**先拒（驱动层 `invalid tapebin or origin target mismatch`），而参考 codec（LEB128 无界）与参考编译都接受该载体。可表示边界 2^31−1 两路同归 `tape data directive`（归属平价成立）。对照 hostaddr/bindings 两类 NEG：载体路归属、具名、行号与文本路完全一致——分叉只在 extent 的解码器界限上。prd 拒绝归属条（语言级拒绝归声明终态的阶段，不得改记）之下，裁向有二：(i) 产品解码器放行 LEB128 全域、让 lower 表以同一具名拒绝（归属两路合一，动 src/ 须另授）；(ii) 登记解码器 32 位界为载体格式承诺（参考 codec 向产品收紧，动 unisa/ 须另授）。现状已由 com-extent-neg 的 `neg-single-bin`/`neg-max-bin`/`neg-max-text` 三断言写死，com-hostaddr-neg 的 `neg-high-bin` 写死了归属平价的对照面。

**已落（2026-10-22 09:2x，机房主任 09:1x「待裁清单自排」）——取向 (i)**：`docs/tapebin-v1.md` 只说 `.bss` 记 ULEB128 长度、无 32 位承诺，Python codec 无界，没有任何套件断言该界，故 2^31−1 是三处 C 实现（`exec/c/tapebin.h:170` 驱动解码器、`src/tapebin.c:154` 经典解码器、`src/tapebin_encode.inc:178` 编码器）的实现界。三处改为 **2^63−1（非负有符号 64 位字）**：经典解码器以 `long` 返回 ULEB 值，`count<0` 天然守住该界，驱动与编码器取同值——三处 C 实现平价由构造保证；解码后把长度以十进制写进 target.text，extent 的拒绝回到 lower 的 `not covered: tape data directive`，与文本路**同归属**。≥2^63 两 C 解码器仍 `invalid tapebin`（载体可表示长度之外；py codec 独宽），而文本路对 2^63−1/2^63/2^64−1 一律由 lower 具名拒绝——登记在 com-extent-neg 的新断言 `neg-2p63-bin`；`neg-single-bin` 翻为 `tape data directive`。

## 2. 缺 bindings 的 `.librarycall`（族 4）

**实测**：
- 产品：`f:\n.librarycall r1, r0\n…` 无任何 bindings 输入时 rc1、stdout 空、不写输出文件、具名 `not covered: library import binding or signature`（绑定层）。
- 模型（`exec/lower/libraryimportscheck.py` bundle 路由）：缺/坏 `\0library/bindings` 一律 reject（在册断言）。
- 参考：**接受** rc0，无 bindings 通道照样下放出 4096 B 镜像（text 203 B、RW memsz 168；`.librarycall` 被下放成了什么语义本文未验，仅记录其存在）。

**定性**：spec LB 系列要求库需求走显式合同/bindings 通道；参考在通道完全缺席时静默产像，疑属**参考漂移**（与 underscore 同类：参考比合同宽）。

**选项**：
- (A) 照 underscore 先例把参考向合同对齐：无 bindings 输入时具名拒绝（动 `unisa/`，需另授；落地时 com-bindings-neg 的 `neg-no-bindings-ref` 断言同步由「登记接受」改「具名拒绝」）。
- (B·现状) 保持登记式分账。
- (C) 放宽产品——与模型在册拒绝冲突，不可取。

**建议：(A)，另授后实施；未授期间 (B) 由叶守住。**

**已落（2026-10-22 07:4x，机房主任 07:30 放刀「从待裁清单落具名产品刀」）**：`unisa/lower.py` 的 `lower()` 在剪枝**之前**对任何 `.librarycall` 具名拒绝 `not covered: librarycall requires library bindings`（与同函数 `.libraryfn` 的既有检查同形；放在剪枝前是因为产品的合同检查也不看可达性——不可达的库调用仍是该 tape 的库需求，首次尝试放在剪枝后被 `prune_input=True` 剪掉了死函数而未生效）。bundle 路由（`\0library/bindings`）不变。com-bindings-neg 的 `neg-no-bindings-ref` 由「登记接受」翻为「具名拒绝」，同拍落。

## 2c. CRLF 行尾（第四条「参考宽／产品严」，2026-10-22 05:4x）

**实测**：`_start:\r\n  ret\r\n` 这类 CRLF 行尾的 tape——产品 tape 验域**接受**（`-S` 原样回显含 `\r`），但镜像路（`-b`/`-run`）拒绝 `not covered: an op outside the first encoder slice`（`ret\r` 作为 op 记号不可分类）；参考全路接受并建像。产品内部还有域/镜像两层口径不一。发射器从不写 `\r`。对照组：无尾换行、5000 字符符号名、UTF-8 BOM（双方都当普通标签字节）三者双实现接受且镜像逐字节同。

**选项**：(A) 域收紧——tapecheck 对 `\r` 给具名拒绝（动模型+参考同步，域/镜像口径合一）；(B) 双方剥 `\r`（CRLF 透明，动 src/+unisa/）；(C·现状) 登记分叉。**建议 (A)**：发射器不产 `\r`，域内合法性没有 CRLF 的根据，且能顺带消掉产品内两层不一致。现状由 com-tape-struct 的 `crlf-domain/identity/build/ref-accepts` 四断言写死。

**修正（2026-10-22 08:0x，撤回 (A)）**：`exec/tapecheck/README.md` 明写「CRLF counts as one line」——域对 CRLF 的接受是**授权规则**，参考 `splitlines()` 同口径；(A) 与之相抵，撤回。真正的分叉点已定位到 **lower 阶段的分词分隔符类**：`exec/lower/code-scan-byte.tsv` 的 `C.line 32,9,10`、`C.word 32,9,10,256`、`C.args 32,9,44,91,93,43`、`C.arg …,10,256` 与 `exec/lower/data-byte.tsv` 的 `LS/WORD/D.ws/D.name/D.value` 的 `32,9` 类都**不含 13**；tab(9) 在类内所以 tab 分隔的 tape 两目标都建像成功（实测），`\r` 不在类内就被并进记号（`ret\r`）原样传给 enc，enc 的 `x86-shell dispatch` / `armcontract fail` 具名拒绝（`an op outside the first encoder slice` / `ARM64 operand or instruction`）。同目录 `hostsymbols-byte.tsv` 已有完整 CR 处理（`LINE 13 → CR`）与 DEC/HEX/OCT/BIN 数字态——同树先例。

与 hex-A′ 同族：lower 把操作数拼写原样传给 enc，而 enc 的 `x86-operand AN` 只认 `-`/0–9，非十进制拼写在 enc 拒；两条都是「lower/enc 词法面比域窄」。

**正确修向 (B-lower)**：把 13 加入上述分隔符类（9 行表项，纯授权表编辑），数据记录行的 `COPY` 直通与 `.str` 字面量内的 `\r` 要单独核对（字面量内的 `\r` 是数据，必须保留）。**代价**：lower 模型变 → 候选重建 + 根对归位；`lower-seal`（streamseal 测量叶）与 `edge-lower-abi/regmap/reloc/enc-seal` 四边封件按「有模型变更实证」重封；arm/x86 两目标各加 CRLF 正例进 com-tape-struct 并翻转 `crlf-build` 断言。属产品闭包 + 重封，须具名派刀；本文不代落。

**B-lower 已实跑并落主（2026-10-22 08:3x 在私有冻结树实跑；08:39 机房主任代裁放刀，经 commitgate `--suite exec-core` 落主）**：改动 = `code-scan-byte.tsv` 4 行 + `data-byte.tsv` 5 行（把 13 加入 `32,9` 分隔符类）+ `src/tapebin_encode.inc` 1 行（行尾 CR 当行末空白；该编码器与经典参考共用，平价由构造保证）。实测：CRLF 与 LF 同程序在 lnx/x86_64、lnx/arm64、osx/arm64 建像**逐字节同**、`-run` 同退出码；数据记录行（`.str/.bss`）CRLF 直通镜像同；`.str "a\rb"` 字面量内的 CR 保留且与参考同像；载体发射 CRLF==LF==参考 codec、解码不动点；全部拒域叶绿（唯一翻转：com-tape-struct 的 `crlf-build` 由登记拒绝改为「建像且与 LF 孪生/参考逐字节同」）。封件影响：仅 `lower-seal` 转红；按新写的 `tests/reseal-lower.sh`（仿 reseal-elf.sh，traced 链 + prune.net fixture 生产 + 同输入断言）重封后，12 个 case 字节文件**全部不变**，只有 dependencies（两条 rule 的 sha）、identity、streams 与 model 名变；lower-seal/seal-modules 复验绿；候选重建全步绿（model steps 89s），`.com` 字节变（bd693565…，表入模型）。

## 2b. 另一类观察：双实现同宽于 C99（登记，不入前两类）

2026-10-22 04:4x 采证 com-cerr-hygiene 时发现：`int *p = 1.5;`（double 初始化指针，C99 约束违例，gcc error）被**产品与 Python 参考双双接受**（两边 rc0）。产品与参考互相一致（对拍纪律无违例），但共同宽于 C99；`int *p = 3;` 的整数→指针则是常见扩展（gcc 仅警告），不算。该类不属「参考宽/产品严」，单列为 conformance 面待裁：是否照 R13 缺陷账或 difftest 负域另立登记、以及双实现同步收紧的授权归属。本文不代裁，未入任何叶（叶里语义负例改用 unknown identifier 规避未裁面）。

**矩阵（2026-10-22 09:4x 实测；比较的是编译接受/拒绝，`unisa run` 的返回值是程序退出码、不作拒绝依据——首轮把 `int x=1.5` 的 rc=1 误读成参考拒绝，已纠）**：指针↔算术的相容检查在初始化、赋值、return、实参、指针→double 五处**双实现一律接受**（gcc 五处 error）——系统性缺失，归类型阶段设计面，非窄刀；`int *p = 3;` 同类。另一条真分歧：`struct s v = 1.5;` 参考**接受**、产品以 `not covered: struct assignment` 拒（诚实 not covered，非约束诊断）、gcc 拒——「参考宽」第五条，登记待裁，未入叶。

**第五条已落（2026-10-22 12:3x，自点名窄刀，参考向产品对齐）**：`unisa/front/parse.py` 两处具名拒绝 `not covered: struct assignment`——声明顶层 `struct s v = <非结构表达式>`（`local_init(top=True)`；嵌套成员的大括号省略照旧，`{1,2,3}` 进 `struct{struct i;int}` 实测仍为 9，与产品同）与赋值 `v = <非结构>`（原先从标量地址 blockcopy，属参考误编译）。产品不动。叶 com-cerr-hygiene 增 sema-sinit/sema-sasgn 两负例（60 检 0 坏）；111 个含 struct 的 tests/c、tests/c99、examples 源经新参考无一新增该拒绝。指针↔算术五处双宽仍待裁。

## 附：hex-A′ 基线项的实测范围修正（2026-10-22 07:2x，只记不动）

基线清单（v0.0.43-baseline-20261014.md）把该缺陷记为「十六进制立即数」。本次实测其真实范围是**全部非十进制拼写**：`0x…`、`0b…`、`0o…`、`-0x10` 在镜像路（`-b`/`-run`）一律 `not covered: an operand that is not a register or an integer`，十进制（含负）接受；而 tape 验域与 Python 参考对四种拼写全收（域拼写面已由 com-tapedomain 220 案例模锁定）。即缺口在镜像路的操作数分类面，不特属 hex。

派刀时的配套事实：下游 C 读器 `src/tapescan.h ts_integer`（被 `back_lower.c:387` 与 `tapeprune.c:46` 消费）只认十进制与 0x——若操作数分类面修到与域一致（含 0b/0o），这两处读器也得同拍跟上，否则缺口只是往后挪。修法无论取哪层，均属产品闭包，须具名授权。

## 3. 共同事项

- 两叶已把现状写死：裁定任一项落地时，对应叶的 ref 断言与注释须同一提交内改，不允许叶与裁定口径分叉。
- 本文不改产品、不改参考、不改模型表；win 口径（TP-7/8 待证）不在本账内。
