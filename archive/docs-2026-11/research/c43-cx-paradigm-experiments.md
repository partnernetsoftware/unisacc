# .cx 范式实验报告（2026-10-22，独立实验子代理，仓外 scratchpad/cx-exp/）

> 原文照录。实验文件（e1/ e2/ e3/ 下的 .cx、表、合同与结果）留在会话 scratchpad，未入仓；入仓的第一刀是 `tests/tdescprobe.cx`（E3 枚举器的 .cx 闸化）与基线 `tests/tdescprobe-{product,classic}.tsv`。主会话复核：产品对 `int (*x[3])[3]` 的 `sizeof((*x)[0])`、`sizeof(*x[0])` 给 4（gcc 12），实际跑过。

# cx-exp 报告：流/网/范式/模式 折叠实验（.cx 模块）

工作区：本目录（仓库只读，未改）。工具链：`sh unisacc.com`、经典导出 `tests/export_ref.sh`、系统 cc（仅作对拍 oracle）。无 Python。

## E1 前奏（prelude）与模块机制 —— 实际跑过

文件：`e1/cxgate.c`（≈80 行补丁：.cx 模块门 M0，两张有限表 CXG_VER/CXG_STD）、`e1/ref_cx.c`（经典导出 + 补丁）、`e1/ua_cx`（构建产物）、`e1/t/*`（正反例）、`e1/cxbuild.cx`（清单驱动的模块构建器，本身用严格 .cx 写、由 ua_cx 运行）、`e1/m/`（三模块：app.strbuf / app.wc / app.main + cx.mod 清单）、`e1/parity.sh`。

命令与结果：
- `sh tests/export_ref.sh e1/ref.c`；`sh unisacc.com ref.c -o ua_ref0`（42.9 s）；补丁后 `ref_cx.c -o ua_cx`（43.6 s）。
- 现状确认：`unisacc.com -run p0.cx`（无 #include）→ `undefined function 'cx_read'`；`unisacc.com -include cx.h -run p0.cx` → 正常。即产品驱动早已有 `-include`，"隐式前奏"只是驱动层一条晚绑定规则（后缀/版本 → 强制 include 表行），不需要动 E2 网。
- ua_cx 正例：legacy 无 include 的 .cx 跑通；`#pragma unisa cx 1` + `use std.ctype` 跑通；.c 不受影响。
- ua_cx 反例（具名拒绝）：严格模式 `#include` → `cx strict: textual #include refused ..., line 2`；`cx 2` → `unknown language version 2`；`use std.net` → `unknown standard module`；`#pragma unisa frobnicate` → `unknown unisa pragma`。
- 平价（ua_ref0 vs ua_cx，`-b lnx/x86_64` 镜像逐字节）：60 个仓库 .cx（tests/ + examples/cx-lab）：56 相同、1 两边同错、**3 不同**（headerchain/lifecycle/prevheaders —— 正是闭包里不含 cx.h 的三个）。原因实测：多出的前奏把 `write` 等库体拉进 tape（`-S` diff 多出 `write:` 函数）。结论：**头文件前奏不是闭包中性的**；要么 legacy 版本行映射到空前奏（字节平价），要么等 CXIF（只含声明、不带体）落地后再对 legacy 默认开前奏。
- 模块集（`ua_cx -run cxbuild.cx -- m ../ua_cx include`）：接口投影 `.cxi` → 每模块单独 `-c -b lnx/x86_64 -funit` → 链接 → 运行 `words=5 bytes=30 first=HELLO UNISA WORLD, C`；系统 cc（`cc -c -x c -include include/cx.h -include ctype.h -include app.strbuf.cxi wc.cx` 等）同输出。两次构建输出全部 sha 一致（DETERMINISTIC）。
- 模块反例：未声明依赖 → `REFUSE undeclared-dependency`；依赖序/环 → `REFUSE dependency-not-built-before`；身份不符 → `REFUSE module-identity-mismatch`；严格模块内文本 include → 编译器门拒绝。
- 接口稳定：只改 app.strbuf 函数体 → 三个 `.cxi` sha 不变、仅 strbuf.o 变 → 消费者无需重编（实际跑过）。

局限（诚实）：`.cxi` 仍是"声明头"，经 `-include` 文本注入，不是规范化类型图 CXIF；投影规则是单行函数头 + typedef 行（只是原型级）；std 表在编译器补丁与 cxmods.tsv 各一份（折叠目标是一张表两个消费者）；cc 路径的 -include 列表本应由门输出（`--print-uses`），此处手写。

## E2 范式即模块：一个通用流细胞，三条流复用 —— 实际跑过

文件：`e2/cell.cx`（≈170 行严格 .cx，`#pragma unisa cx 1`，模块 paradigm.cell）、表 `e2/lex.tbl` / `e2/decl.tbl` / `e2/tcheck.tbl`、合同 `e2/*.contract`（SC1 形：format/version/stage/table/record/resource KEY PRESENT|ABSENT）。
执行器 = 字节类上的下推转换器，动作原语有限且与流无关：`buf emit flush drop hold emithold push unwind lookup refuse redo`；加载时静态合同检查（未声明类/未定义下一状态/未知动作/表请求而合同未列的资源 = MISSING_RESOURCE）。结果类：ACCEPT / REFUSE<名> / NOT-COVERED<状态 类> / INTERFACE_ERROR(rc2)。

命令（均 `e1/ua_cx -run cell.cx -- CONTRACT INPUT`，每次 <1 s）与结果：
- 声明符流（decl.tbl，35 行表）：`int (**p)[3]`→`P.P.A3.int`；`int *a[3]`→`A3.P.int`；`int (*a)[3]`→`P.A3.int`；`int (*(*x)[4])(int)`→`P.A4.P.F(int).int`；`int (*p`→`REFUSE unclosed-(`；`int *p)`→`REFUSE unbalanced-(`；`int a[n]`→`REFUSE array-size-not-constant`。
- TDESC 校验流（tcheck.tbl，对上一流输出串联）：`F(int).A3.int`→`REFUSE function-returning-array`；`A3.F(int).int`→`REFUSE array-of-functions`；其余原样 ACCEPT。（"相邻性"放在第二条流里，因为在声明符流里它要依赖"unwind 是否弹出过星号"这种隐式状态——这正是折叠的要点：不让条件藏进寄存器，改成流的组合。）
- 词法流（lex.tbl）：`K int / I main / P ( ... N 42`；关键字集合是**资源**（keywords PRESENT）。
- SC1 反例：资源改 ABSENT → 合法空集（`int` 变 `I`）；删资源行 → `INTERFACE_ERROR MISSING_RESOURCE keywords` rc2；重复键 → `duplicate-resource-key` rc2；version 2 → rc2；表里写 `jump:` → `unknown-action` rc2。
- 执行器本身的 cc 对拍：`cc -w -x c -include include/cx.h cell.cx -o cell.cc`，两条流输出与 ua_cx 运行逐字节相同。
结论：同一执行器 + 同一合同检查器服务三条流，流专属控制代码 = 0 行（差异全在 .tbl/.contract）。

## E3 复发缺陷类：声明符 → TDESC + 枚举探针三方对拍 —— 实际跑过

文件：`e3/gen.cx`（枚举器，严格 .cx）、`e3/decls.in`/`want.tsv`（39 个派生）、`e3/probes.c`（合并探针）、`e3/pp/001..106.c`（单探针）、`e3/expect.tsv`（仅由 TDESC 一个递归函数 `sz()` + `ap()` 算出的期望）、`e3/runpp.sh`、`e3/res.{prod,classic,ref}`。

1. 枚举 {P, A3, F(int)} 深 1..3 = 39 个派生（`ua_cx -run gen.cx` → `derivations=39 valid=23 probes=106 model-refused-contexts=78`）。
2. 往返：39 个生成的声明符文本过 E2 的声明符流 + 校验流（同一 cell 执行器），结果与生成器的描述符/拒绝集合**逐字节相同**（`cmp got.tsv want2.tsv` → ROUNDTRIP 39/39；拒绝分布 6 array-of-functions / 5 function-returning-array / 5 function-returning-function）。
3. 系统 cc oracle：`cc -std=c99 probes.c && ./probes.cc` 与 TDESC 模型期望 106/106 逐字节相同。即 decl/sizeof/deref/subscript 八种上下文的结果全部是"一个规范描述符 + 两个纯函数（剥一层、求大小）"，没有任何按路径的状态。
4. 三方对拍（每探针单独编译运行，`sh runpp.sh NAME COMPILER`）：

| 编译器 | OK | REFUSE(具名 not covered) | WRONG(静默错码) |
|---|---:|---:|---:|
| 产品 `unisacc.com` | 86 | 18 | **2** |
| 经典 `ua_ref0`（本树导出） | 85 | 8 | **13** |
| `/tmp/ua_ref` | 85 | 8 | 13 |

产品的 2 个静默错：`int (*x[3])[3]; sizeof((*x)[0])` 与 `sizeof(*x[0])` 应为 12，产品给 4（"数组行经指针"丢一维——正是今天那类缺陷的新实例）。产品的 18 拒绝：`(**x)[3]` 声明整类拒绝（8 条）、`sizeof(**x)`/`x[0][0]` 等在 `*x[3]`、`**x[3]`、`(*x[3])[3]` 等上 "not covered: sizeof operand"（10 条）。经典 13 错含 `(*x)[3][3]` 整族 7 条（sizeof(x) 给 24）。
结论（实际跑过）：同一缺陷类在三个实现里以不同形状出现，且按路径各自"补"过；而 TDESC 表示下，"少一层指针/丢一维"不是可能的错误状态——描述符串本身就是类型，sizeof/deref/subscript 都只是对串的同一操作，没有第二个寄存器可以漏存。该生成器（39×8 上下文）就是这类缺陷的闸：任何实现只要过 106/106 + 78 个模型拒绝一致，即覆盖深度 ≤3 的全部派生。
未做：78 个模型拒绝上下文尚未逐条用 `cc -pedantic-errors` 证实为 C 约束违例（只是设计；`sizeof(function)` 在 gcc 下是扩展）；未接入产品 E3 网（只在仓外旁生）。

## E4 内存安全检查器 —— 只是设计（未跑，时间用尽）
设想：复用 E2 cell，把 cx_read 契约 `returns_owner` 写成 resource（契约表 PRESENT），对 E1 的 strbuf/main 模块的 token 流（E2 lex 流输出）跑一张 owner 状态表：`ALIVE→(free)→FREED`、`FREED→(use)→REFUSE use-after-free`、`FREED→(free)→REFUSE double-free`、EOF 时 `ALIVE` 且无 consumes → `REFUSE leak`。单函数、无别名、无分支汇合时这是有限表；汇合/别名/循环需要 spec §5.1 的格与不动点，不能硬塞进单次有限分类（spec §6 已警告）。

## 折叠目标架构（证据支撑的部分）
1. **一个执行器**：字节类下推转换器 + 有限动作原语（E2 cell；与 exec/c/core 同构：表控制 + 原语，不认识语言）。流 = (表, 合同)；流的组合 = 串联（声明符流 → TDESC 校验流），替代"在寄存器里藏条件"。
2. **一个合同**：SC1（format/version/resources PRESENT|ABSENT/MISSING/重复键 + ACCEPT/REFUSE/NOT-COVERED/INTERFACE_ERROR），加载时静态检查表（未声明类/状态/动作/资源）——对应 R1 regcheck 的"寄存器必须声明"，但从结构上做到。
3. **一个类型描述符**：TDESC 串（P./A<n>./F(sig)./base）由唯一的声明符流产出；sizeof/deref/subscript/晚绑定的类型比较（LB-8）都消费它。今天消失的：vt/vb/rkok/rk/vid/szrow 这些"类型侧影寄存器"、`fp_more`/`declfpp`/经典 `cursize` 78 处等按路径补丁。
4. **一个模块门 M0**：后缀/版本 → 前奏，`use` → 接口，严格模式拒绝文本 include；两张有限表（版本表、std 模块表）；驱动层晚绑定（产品驱动已有 `-include` 资源 `\0cli/includes`，无需改 E2 网）。接口工件先是投影 `.cxi`，再升级为 CXIF。

## 迁移路径（保持逐字节平价）
- 第 0 步（仓外，已具备）：`e3/gen.cx` + `runpp.sh` 作为新闸的原型，与 stagechain.cx（Python-free E2→E1→E3）并列；先把 2 个产品 WRONG 登记为 `tests/c/candidates/` 探针。
- 第 1 步：TDESC 旁生流——产品 E3 不变，额外在 tape 旁输出每个声明的 TDESC（或由 cell 声明符流在仓外从源码产出），与 gen 模型对拍；产物字节不变（examples/ 与 tests/c 用 stagechain.cx 逐字节对拍）。
- 第 2 步：sizeof/deref/subscript 的消费者逐个改读 TDESC；每迁一个消费者，跑 106 探针 + tests/c + examples 全平价；红即回退，不加补丁。
- 第 3 步：删除被 TDESC 取代的寄存器与路径补丁（regcheck 证明无读者）。
- .cx 侧：M0 门 legacy 行先映射为"无前奏"（实测 3/60 个 .cx 镜像会变），严格版本行（`cx 1`）才开隐式前奏与 include 拒绝；CXIF（不带函数体）落地后再讨论 legacy 默认开前奏。

## 第一刀（建议立即落）
把 `e3/gen.cx` + `runpp.sh`（改为 .cx 驱动）入仓作为 `tests/tdescprobe.cx` 闸（只读、对产品零字节影响），记录现状基线 prod 86/18/2，两条 WRONG 作为已知红；随后修复时以"让 TDESC 唯一来源接管该上下文"为准，不允许路径补丁——闸的度量是 OK 数单调上升、WRONG 归零。

## 风险与不确定
- 产品 E3 是构造网络；把 TDESC 引入网会改变状态序、牵动 .tbl/.net 与封存（R3 已标"高成本"）；状态数增长、60 s 构造上限未测。
- E2 cell 只是玩具规模（字符串比较查表，每字节线性扫描）；产品执行器是阈值网络/汇编内核，cell 只证明"范式可以模块化"，不证明性能或网化可行。
- 声明符流只覆盖 `int` 基类型、`[常数]`、`(int)`；限定符、typedef 名（需资源）、抽象声明符、多参数签名未做。
- E1 门是经典补丁，不是产品驱动；`.cxi` 不是 CXIF；投影规则粗糙（单行函数头）。
- 三方对拍的 cc 是 gcc（x86_64 Linux）；`sizeof` 值依赖 LP64。
