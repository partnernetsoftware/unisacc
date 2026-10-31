# 分层依赖事实盘点与最小检查设计（2026-10-31）

状态：提案与实测盘点，不改源码、不改 `cx/spec.md` 条款、不把拟议层号当已生效。
范围：仅 Linux x86-64 工作区；不改 apps/csih、ujs 或其他车道。

## 1. 现行规范依据

- 层就是目录：L0 `cx/sys`/`cx/mem`/`cx/text`；L1 `cx/core`/`cx/runtime`；
  L2 `cx/driver`/`cx/compiler`/`src`；L3 `tools`/`tests`/`apps`（CX-S-1…CX-S-4）。
- 调用方向只允许向下，或同层已 `use` 的公开符号（§13.1 末段）。
- `cx/cx0`、`cx/workspace`、`cx/crypto` 属上层但无层号（CX-S-6/CX-S-7）；`cx/*.cx`
  顶层文件（`check_memory.cx`、`check_owned.cx`、`owned.cx`）无层号。
- 精确例外只来自 `tests/cxraw-layers.tsv` 与 `tests/cxinclude-boundary.tsv`，
  且它们只管**裸 libc/POSIX 调用和 #include**，不授权模块反向依赖（CX-S-5）。

## 2. 实测事实（工作区 HEAD 82ca853dfd 之后的树）

方法：`sh ./unisacc.com tools/symindex.cx -- --index out/symindex-dep update`
（1889 文件，0 stale）取 `dump` 的 `U` 行与源码 `#pragma unisa use` 对照；
用 realpath 解析“使用方同级目录 + 点号变目录 + `.cx`”；层按 CX-S 前缀判定。
逐边结果：`out/depinventory.tsv`；汇总脚本 `out/depinventory3.py`（仓外一次性测量）。

- tracked `.cx` 路径 1632，physical 1601，其中 **31 个 `.cx` 是软链**（去重后才算边）。
- 直接 `use` 边 2317；其中编号层之间的边 2185：
  - 向下 1422：L3→L0 1161、L3→L1 131、L3→L2 95、L2→L0 13、L2→L1 4、L1→L0 18。
  - 同层 763：L3 738、L0 12、L2 7、L1 6。
  - **向上 0**。
- 涉及无层号目录的边 82：
  - `cx/crypto`：tools→crypto 36、crypto→crypto 27；
  - `cx/cx0`：tests→cx0 5、tools→cx0 4、cx0→cx0 1、cx0→L0 2；
  - `cx/workspace`：tools/tests→workspace 3、workspace→L0 2；
  - `cx/check_memory.cx`→`cx/core`（AMB→L1）1、`cx/check_owned.cx`→`cx/owned.cx`（AMB→AMB）1。
- CX-S 范围外目录（adapter、flownet、examples、exec、release）共 50 条边；不属本检查授权面。
- 未解析 `use` 5 条，全部是夹具/负例：
  `tests/cxcc_missing.cx`、`tests/cxpack/bad-use/main.cx`、`tests/cxstd_unknown.cx`、
  `tests/nets.cx`（`netcore`）、`tools/symindex/fixture/facts.cx`。

  > **事实更正（2026-10-31，原句“全部是夹具/负例”作废；上面的 15:18 epoch 数据保留不动）**
  > `tests/nets.cx` 不是负例：`tests/batches.tsv` 该批状态为 `PREPARED`，材料路径
  > `{nets}/nets.cx`，`tools/net-material.cx` 把 `netcore.cx` 与 `nets.cx`、`material.tsv`
  > 一起写进同一输出目录并打印 `PREPARED`；tracked 树内没有 `netcore.cx`，所以
  > `use netcore` 在未材料化时必然 ENOENT。它属于“材料化后才可解析的入口”。
  > 因此 5 条是 4 条负例/夹具 + 1 条 PREPARED 材料入口；白名单行数与配对未变
  > （`tests/cxlayers-unresolved.tsv` 5 行，改动只在该行理由文本，提交 `8762f242d1`）。

身份事实：`src/{sys,text,core,runtime,driver,compiler}`、`tests/{sys,text,mem,workspace,...}`、
`tools/{mem,text,compiler}`、`cx/*/mem|sys` 等是目录软链；例如 `src/sys/fs.cx` 的
realpath 是 `cx/sys/fs.cx`（L0），只看词法路径会误判为 L2。31 个 `.cx` 文件软链同理
（`tests/cxlib/*.cx`→`tools/cxlib/*.cx` 等）。硬链接等价不在本盘点承诺内（CX-U-4）。

> **覆盖口径核实（2026-10-31，同一 epoch 只读对照；不改检查口径、不重跑已报告结果）**
> - **+7 的归因**：`13e6abf1a1` 的提交说明没有“软链使边数 +7”的说法。实测差额来自该提交
>   **新增工具自身**：`tools/check-cx-layers.cx` 在 15:18 明细里 0 行，其 shard 恰有 7 条 `U`
>   （`sys.fs`、`sys.io`、`sys.proc`、`sys.sig`、`text.text`、`text.fmt`、`cxlib.netpack`），
>   2317 + 7 = 2324。此前任何“软链导致 +7”的归因撤销。
> - **31 条是文件软链，不是目录软链入口**：`git ls-files -s '*.cx'` 中 mode 120000 的 `.cx` 共 42 条，
>   非 `archive/` 的 31 条与“1632 − 1601 = 31”精确一致。
> - **真实审计表面不含这 31 条**：`tools/symindex.cx`（`set_tracked`）显式跳过 `120000`，
>   故 manifest 的非 archive `.cx` 源为 1613 = tracked 1644 − 31；`cl_audit` 只遍历 manifest。
>   我此前“manifest 与 tracked 软链交集为 0”的实测正确，但当时未给出原因，且未说明后果：
>   工具头注承诺“每个词法条目都是源、不按物理源去重”，真实输入做不到这一点。
> - **被跳过的词法入口**：31 条，对应 38 条 `U`；31 个目标 realpath 全在 manifest 内，
>   所以依赖事实仍会以目标自身路径被审计一次，损失的是**词法条目的层归属**。
>   其中 2 条会改变分区：`tests/preprocess/bundle.cx`、`tools/cx0/bundle.cx`（词法 L3，realpath
>   `cx/cx0/bundle.cx` AMB）。
> - **未运行的推断（不是收据）**：若把这 31 条也当源，按同一规则应得 edges 2437、
>   same 793、down 1501、amb 84、out 59、up 0、unres 7——`tools/cx0/bundle.cx` 的
>   `mem.heap`/`mem.bytes` 在 `tools/cx0` 下无 `mem` 软链，会成为 2 条新未解析并使检查失败。
>   即：该词法条目当前**不可审计**，不是“已审计且合规”。
> - 夹具 `f-entries` 用自造索引证明解析器行为，不代表真实输入覆盖这 31 条。

交叉核对：`-cx-plan` 不能直接当直接边清单——它同时列直接 `use` 的完整 CXIF 和
传递 `use` 的类型 CXIF；例如 `src/drive.cx` 源码 3 条 `use`，`-cx-plan` 给 5 个
interface 列（含传递类型）。直接边应以源码 pragma（或 `symindex dump` 的 `U` 行）为准。

## 3. 现有覆盖与缺口

- `tests/cxraw.cx`、`tests/cxinclude.cx`、`tests/cxlibc.cx`、`tests/cxplatform.cx`：
  只查裸调用、`#include`、平台 `#if`，不建模块图，不能证明层向。
- `tools/symindex.cx` 的 `uses/users` 按 realpath 求传递闭包，`dump` 给直接 `use`；
  有身份与边，但没有层判定。
- `-cx-plan` 证明编译期解析与闭包，但不区分直接/传递列，也不判层。
- `tests/cx-unit-graph.cx` 只覆盖同程序闭包/分开链接，不涉层。
- 因此“模块反向依赖”目前**无边检查**；CX-S 的降级事实靠人工与目录约定。

## 4. 最小可执行切片（建议下一刀）

新增 `tools/check-cx-layers.cx`（L3 工具，无参数）：
1. `git ls-files '*.cx'`，排除 `archive/`、`out/`；按 realpath 去重。
2. 逐物理文件解析 `#pragma unisa use NAME[::sym]`；按使用方目录解析、realpath。
3. 层判定：CX-S 前缀为 L0–L3；`cx/cx0`、`cx/workspace`、`cx/crypto`、`cx/<file>.cx`
   为 AMB；其他顶层目录为 OUT。
4. verdict：
   - 编号层之间的向上边 = 失败，诊断带 `FILE:LINE use NAME to_real`；
   - 未解析且不在夹具白名单（`tests/**` 负例、`tools/symindex/fixture/**`）= 失败；
   - AMB/OUT 只报计数与明细，不判失败（等层号/归属裁定）；
   - 向下与同层放行；同层不额外警告。
5. `--fixtures`：在临时目录构造
   - 正例：L0→L0、L1→L0、L2→L1、L3→L2、同层 L2→L2 全部接受；
   - 负例：`cx/sys/x use runtime.y`（L0→L1）、`cx/core/x use compiler.y`（L1→L2）
     必须按名拒绝；缺模块按名拒绝；`src/x use crypto.y` 只进 AMB 报告不计失败。
6. 默认同时跑仓库审计与 fixtures；仓库当前向上边为 0，故基线可立即绿。
7. 不建例外表、不放宽 CX-S；确需例外时再按 `tests/cxraw-layers.tsv` 的精确条目
   形式单列，并先盘点真实反向边。

登记：`tests/batches.tsv` + `tests/coverage.tsv` 各一行（id `cx-layers` /
`contract.cx-layers`），用 `tools/register.cx` 或 `tools/commit-own.cx --keyed`。
文档：`prd.md` 能力速查加一行由该文件维护者决定；`cx/spec.md` 不新增条款
（检查的是既有 CX-S/Y）。

路径归属：`tools/check-cx-layers.cx`、其 fixtures（建议 `tests/cases/cx-layers/`）、
注册两行归本刀；`plans/` 归属见各自编辑者；`cx/cx0`、`cx/workspace`、`cx/crypto`
层号裁定不在本刀。

验收：工具对当前树 rc0 且打印 `UP=0 SAME=763 DOWN=1422 AMB=82 OUT=50 UNRES=5`；
fixtures 全绿；负例诊断逐条命名；候选身份与实测日志落 `out/`；不重构造、不安装。

## 5. 附：cx/core/memory.md 现状

`cx/core/memory.md` 第 5 行仍写实现直接使用隐式 `calloc`、`free`、`memmove`；
实际 `cx/core/memory.cx`（`src/core` 是软链）第 2–3 行是
`#pragma unisa use mem.heap` / `use mem.bytes`，调用 `mm_zalloc`、`mm_free`、`mm_move`。
文档已过期，应改由 `cx/core` 的当前编辑者同步；本刀不改。

## 6. `#` 后空格的指令定位分歧（2026-10-31 只读证据；与 CX-G-3 条款裁定分开）

本节只记录分层工具的**指令词法**与编译器/索引不一致的事实和方案，不改判定、
不改 spec、不改白名单。CX-G-3「条件为真」与现行「任意嵌套即拒」的差异是另一
个裁定，本节不涉及、也不作为其依据。

事实（源码）：

- 定位点 `tools/check-cx-layers.cx:123` 的 `cl_src[a + 1] != ' '`：`#` 后紧跟
  一个空格时，该行被整行跳过——既不增/减 `cond`，也不给出诊断行号。
- 两个对照实现都跳过 `#` 后的空白再取词：`cx/cx0/plan.cx` 的 `cxm_id`
  （跳过 `<=32` 的字节）、其投影 `src/cxmod.c` 同形、`tools/symindex.cx` 的
  `directive()`（`skip_blank` 后 `dir_word`）。
- 因此 `#if`/`#ifdef`/`#ifndef`/`#endif`/`#pragma` 在 `#` 后带空白时：编译器与
  索引当作指令，分层工具当作普通行。

实跑证据（探针树 `/tmp/cxlprobe.o1YEsz`，真 `unisacc.com`，非模拟；探针源与
`tests/b.cx` 同目录，模块名等于文件主干名）：

| 探针源 | 分层工具 | 产品编译器 |
|---|---|---|
| `#if 0` + `use b` | `FAIL PARSE-CONDITIONAL :3` | `unisa pragma inside conditional` |
| `#ifdef NOPE` + `use b` | `FAIL PARSE-CONDITIONAL :3` | 同上拒绝 |
| `#ifndef NOPE` + `use b` | `FAIL PARSE-CONDITIONAL :3` | 同上拒绝 |
| `# if 0` + `use b` | **无 FAIL，`edges` 16→17，`failed=0`** | `unisa pragma inside conditional` |
| `# pragma unisa use b` | **无 FAIL，边照计，行号 0** | 接受（进入后续编译） |
| `#if 0` / `# if 1` / `use b` | `FAIL PARSE-CONDITIONAL :4` | 拒绝 `:4` |
| `#\tif 0` + `use b` | `FAIL PARSE-CONDITIONAL :3` | 拒绝 |
| `#if 0` 内写 `# pragma unisa use b` | **无 FAIL，边照计** | 拒绝 |
| `#iffy 0` … `#endif` | **`FAIL PARSE-CONDITIONAL :3`**（前缀匹配把 `#iffy` 当 `#if`） | `#endif without #if` |
| `/*` 块注释内 `#if 0` `*/` 后 `use b` | **`FAIL PARSE-CONDITIONAL :5`** | 接受（注释内不算指令） |

结论：两个方向都偏离，且只有**空格**能触发欠计（Tab 不触发，因为守卫只排除
`' '`）。

- 欠计（漏报）：`#` 后一个空格的 `if/ifdef/ifndef/endif`，其内的 `#pragma unisa
  use` 被当作合规边计入，`PARSE-CONDITIONAL` 消失，`rc` 仍 0。
- 过计（误报）：指令词用前缀匹配（`#iffy` 命中 `if`）；无块注释/字符串状态机，
  块注释里的 `#if` 会被计入 `cond`。
- 影响面：只影响 `PARSE-CONDITIONAL` 与条件内 pragma 判定；层向
  （same/down/up）仍由 symindex 的 `U` 行算出，不受影响。被漏报的 pragma 会
  额外产生 1 条边（探针 `edges` 16→17）。
- 覆盖边界：`cl_locate` 只对 `U` 行（`use`）调用；`#pragma unisa module` 位于
  条件内时工具完全不查（`M` 行不进入判定）；行接续 `\` 未处理、未测。

现网可达性（只读统计，HEAD 树）：

- `git grep -cE '^[[:space:]]*#[[:space:]]+(if|ifdef|ifndef|endif|pragma)\b' -- '*.cx'`
  命中 0 个文件；`if`/`ifdef`/`ifndef`/`endif`/`pragma` 五类空形各 0。
- 结论：当前树不可触发，且 16:56 的审计收据（`failed=0`）不受影响；`#if` 共
  254 行、162 个文件（非归档 141、archive 21）。manifest 收录的源里不存在
  「条件内 pragma」——由该收据 `failed=0` 反推，不是直接测量；archive 不在审计
  范围。「块注释内 `#if` + 后续 `use`」同理未直接测量。

方案（不施工，须 cdx/主人批准；三者均保持「任意嵌套即拒」，只对齐词法）：

- A 最小：`cl_locate:123` 改为「`#` 后跳空白再比关键词」，并把前缀匹配改为整词
  匹配；补夹具 `h-space-conditional`（`# if 0` 包 `use`，断言
  `PARSE-CONDITIONAL`）与 `h-comment-conditional`（块注释内 `#if`，断言不报）。
- B 在 A 之上加块注释/字符串状态机，与 `tools/symindex.cx` 的 `parse` 对齐。
- C 让 `symindex` 输出 `cond` 字段由工具消费（改索引格式，最大改动，本版不推荐）。

验收（若批准）：`sh ./unisacc.com tools/check-cx-layers.cx -- --fixtures` 的
`checks` 增加且 `failed=0`；全量审计 `edges` 不变、`failed=0`、`amb/out/unres`
不变；探针 `# if 0`/`# pragma` 两种形状必须报 `PARSE-CONDITIONAL`。

## 7. 方案 B 精确设计：分层工具的 `use` 定位词法与 cxmod 同源（只读，不施工）

本节只给设计，不改 `tools/check-cx-layers.cx`。实施须由本刀按该文件路径归属进行，
并与 omp-go 协调批次登记（`tools/register.cx` / `tests/batches.tsv`）。

### 7.1 合同边界

- **镜像对象**：`src/cxmod.c` 的 `cxm_load`（164–193 行）与 `cxm_scan`/`cxm_directive`
  （368 行起）；权威源 `cx/cx0/plan.cx` 同形（214–243、425–472、470–500 行）。
- **判据不变**：保留现行实现「`cond > 0` 即拒」（`cxm_directive` 在有 `#if` 嵌套时
  直接报 `unisa pragma inside conditional`）。**不实现 CX-G-3「条件为真」**，不裁
  spec，也不把条款差异写进工具。
- **只判 `use`**：`cl_locate` 仅对索引 `U` 行进。`M`（`#pragma unisa module`）行
  在条件内**不查**（与今天一致），本方案不扩。
- **事实源不变**：`U` 行仍来自 `tools/symindex.cx`；本方案只替换「从源码取
  `line`/`cond`」这一步，不改边的发现方式。
- **同一 `use` 多次出现**：判定改为「任一出现 `cond>0` 即拒」，诊断取最小的
  条件内物理行。理由：编译器对任意一处条件内 pragma 都拒（实测 `dbl`：顶层
  `use b` + `#if 0` 内 `use b`，编译器拒 `:4`，现行工具只查首处以 `failed=0` 放过、
  仍计 1 条边）。

### 7.2 词法规格（逐条对齐 `cxmod_load`/`cxm_scan`）

1. shebang：文件首行以 `#!` 开头时整行改空格（保留 `\n`）。
2. 续行：`\` + `\n` → `0x01` + `\n`（保留换行字节）。
3. 块注释 `/* … */`：内容改空格，`\n` 保留；未闭合的块注释延伸到 EOF。
4. 行注释 `// …`：到行尾改空格。
5. 字符串与字符常量保留原样：`\` 跳过下一字节；遇 `\n` 终止（不跨行）。
6. 行首状态：遇 `\n` 置行首；`<= 32` 的字节（空格、Tab、`\r`、`\f`、`0x01`）不改变
   行首状态。
7. `#`：仅当处于行首且不在注释/字符串内。
8. 指令体边界：从 `#` 到第一个「前一字节不是 `0x01`」的 `\n`。
9. 指令词：跳过 `<= 32` 后取连续 `[A-Za-z0-9_]`，**整词精确比较**（`cxm_eqn`：长度
   相等且逐字节相等），不是前缀。
10. `cond`：`if`/`ifdef`/`ifndef` 加一；`endif` 且 `cond > 0` 时减一（下溢钳零）；
    其余词（`else`、`elif`、`define`、`pragma`）不改。
11. `pragma` 链：下一个词精确 `unisa`，其后为行尾或 `<= 32`；再下一个词精确 `use`；
    再一个词是模块名（取到下一个 `<= 32`），`::` 之前为主模块名。
12. 行号：`#` 所在的**物理行**（清洗只换字节、不动换行位置，故可用换行计数）。

与 cxmod 的两处已知差异（本方案**有意**不镜像，记为 UNKNOWN 待裁）：

- **续行指令后的行号**：cxmod 在 `#if \` 这类指令里吞掉续行换行且不增 `line`，故
  报行号偏小。实测 `spliceif.cx`（`#if \` 换行 `0`，pragma 在物理第 4 行）编译器报
  `:3`，物理行是 4。本方案报物理行（便于人读 `path:line`）。
- **`#` 后直接换行**：cxmod 的 `cxm_id` 跳过 `<= 32` 但受指令体 `e` 限制，两者实测
  都不把 `#\nif 0` 当指令；未穷举，标 UNKNOWN。

### 7.3 结构：复用与新增

- 复用：`cl_read`（含 `CL_CAP` 上限）、`cl_isid`（与 `cxm_idch` 位集一致）、
  `cl_process_use` 的解析/白名单/`cl_edge` 路径完全不动、索引 `U` 行作为事实源。
- 复用缓冲：`cl_src` 原地清洗（`cl_locate` 与 `cl_wl_load`/manifest 读取的生命周期
  不重叠），**不新增 1 MiB 静态缓冲**。
- 新增：
  - `static void cl_clean(long n)`：镜像 `cxm_load`。
  - `static int cl_uses(long n, const char *mod, int *l, int *c, int cap)`：镜像
    `cxm_scan` + `cxm_directive`，收集该模块名全部 `use` 出现的 `(物理行, cond>0)`，
    返回出现数；`cap` 取 8，超出返回负值（→ UNKNOWN，不静默截断）。
  - `cl_locate` 改为薄封装：读文件 → `cl_clean` → `cl_uses` → 选出最小条件内行。
- 三态返回：`1` 已定位、`0` 源中无该 `use`、`-1` 读失败或超长。

### 7.4 计数与诊断边界

| 条件 | 行为 | 计数影响 |
|---|---|---|
| 任一出现 `cond > 0` | `FAIL PARSE-CONDITIONAL path:line`，`line` = 最小条件内物理行 | `cl_fail++`；该 `use` **不计边**（与现行 `return` 一致） |
| 全部出现 `cond == 0` | 无诊断 | 边分类（same/down/up/amb/out/unres）不变 |
| 索引有 `U` 行但源中定位不到 | 新 `UNKNOWN PARSE-LEX path:0 use MOD` | 新计数 `cl_lex_unknown`；不计 `cl_fail`；汇总追加一行 `UNKNOWN` |
| 源不可读 / 超过 `CL_CAP` | 同上 UNKNOWN 类 | 同上（不得静默放行） |
| 同模块出现超过 8 次 | `UNKNOWN PARSE-LEX-MANY` | 同上 |

- 边数：仅「新判 FAIL」的 `use` 少计（每条 1），受引力是条件内 pragma 的数量；
  当前审计域该数量由新工具首跑确认（设计预期 0）。
- 行号：物理行；`UP` 诊断沿用 `cl_cur_line` 的现有语义不变。

### 7.5 夹具（每项 1 条 `fx_note`，用 `fx_dump` 触发一次审计）

| id | 源形状 | 断言 | 覆盖规格 |
|---|---|---|---|
| `h-space-if` | `# if 0` 包 `use b` | `cl_fail >= 1` 且诊断行 = 2 | 规则 9（`#` 后空白） |
| `h-space-pragma` | `#if 0` 包 `# pragma unisa use b` | `cl_fail >= 1` | 规则 9 + 11 |
| `h-comment-if` | 块注释内 `#if 0`，注释外 `use b` | `cl_fail == 0 && cl_edges == 1` | 规则 3（现行误报点） |
| `h-comment-pragma` | 块注释内 `#pragma unisa use zz`，注释外 `use b` | `cl_fail == 0 && cl_unres == 0` | 规则 3（索引已过滤，回归） |
| `h-string-if` | `#define S "#if 0"` 后 `use b` | `cl_fail == 0` | 规则 5 |
| `h-char-hash` | `#define C '#'` 后 `use b` | `cl_fail == 0` | 规则 5 |
| `h-splice-cond` | `#if \` 换行 `0` 后 `use b` | `cl_fail >= 1` | 规则 2 + 8 |
| `h-splice-use` | `#pragma unisa \` 换行 `use b` | `cl_fail == 0 && cl_edges == 0`（现状：索引无 `U` 行） | 规则 2，见 7.6-1 |
| `h-double-use` | 顶层 `use b` + `#if 0` 内再 `use b` | `cl_fail >= 1` 且诊断行 = 后者 | 7.1 多次出现 |
| `h-iffy` | `#iffy 0` … `#endif` 后 `use b` | `cl_fail == 0` | 规则 9 整词（现行误报） |
| `h-unclosed` | `#if 0` 无 `#endif` 后 `use b` | `cl_fail >= 1` | 规则 10 |
| `h-tab` | `#\tif 0` 包 `use b` | `cl_fail >= 1` | 规则 6（现行已正确，回归） |

现有 `h-conditional`（`#if 0` 包 `use`）保留不动；`h-conditional` 的
`h-compiler-refuses-conditional` 交叉核对同样保留。

### 7.6 未知与不作保证（UNKNOWN，非 PASS）

1. **索引漏行**：`#pragma unisa \` 续行 → `symindex dump` 无 `U` 行（实测
   `tests/splice.cx`：只有 `M`/`D`，无 `U`），而编译器接受该文件。该 `use` 在分层
   审计里**完全无边**。方案 B 无法修（事实源在 symindex）；需要另案：symindex 的
   `directive()` 对齐 cxmod，或工具自治扫描。**未测量该形状在现网的数量。**
2. **续行指令后的行号**：cxmod 少算，本设计报物理行；是否镜像待裁。
3. `M` 行条件嵌套：不查（范围外），现状也无法判定。
4. `#` 后直接换行（`#\nif 0`）：两实现实测都不是指令；未穷举其他空白组合。
5. 超长源（> 1 MiB）：编译器与工具的上限同为 1048576，原则上不出现单边可读；
   只作 UNKNOWN 兜底。
6. 忽略文件里的 `.cx`（工作树实测 2 个）与 `out/` 生成源：不在任何检索域内。

### 7.7 检索域与身份（更正上一节的措辞）

以下都绑定提交 `4df9cd59ed` 的工作树，数字均实测：

- **`git ls-files '*.cx'`**：1943 个 tracked `.cx`；其中 `#[[:space:]]+(pragma|if|
  ifdef|ifndef|endif)` 命中 **0 个文件**。
- **工作树非忽略 `.cx`**：同为 1943（无未跟踪 `.cx`）；另有 2 个被忽略的 `.cx` 不在
  检索域。
- **索引 manifest**：`out/cx-layers-index/manifest.tsv` 共 **1902** 行（全部 `.cx`）
  = 1943 个 tracked − 42 个 mode 120000 软链（`symindex` 的 `set_tracked` 跳过）+
  1 行生成 `out/` 源。
- **实际审计域**：manifest 去掉 `archive/`（288）与 `out/`（1）后为 **1613**，
  即上节收据的 `edges=2401` 那一轮所扫描的源集合。
- 因此「当前不可触发」必须限定为：**在 1943 个 tracked `.cx` 的词法检索**
  与 **1613 个 manifest 审计源的条件检查**两个域上成立；对任何新引入空格形指令的
  文件不成立。上一节 §6 未写域的那句"当前树不可触发"以本段为准。

### 7.8 批准后的实施与验收顺序（不在本次执行）

1. 改 `tools/check-cx-layers.cx`：新增 `cl_clean`/`cl_uses`，重写 `cl_locate`，
   加 12 条夹具与 `cl_lex_unknown` 汇总输出。
2. `sh ./unisacc.com tools/check-cx-layers.cx -- --fixtures`：`checks` = 原值 + 12，
   `failed=0`。
3. 全量审计：`edges` 与 `amb/out/unres` 与上一收据逐项比对；预期 `edges` 不变
   （新判 FAIL 数 0）、`failed=0`、新增 `lex_unknown` 计数为实测值而非假定。
4. `tools/affected.cx --files tools/check-cx-layers.cx --run`；`tools/ratchets.cx`
   确认 `srcshape` 只降不升。
5. `tools/commit-own.cx --whole tools/check-cx-layers.cx` + 批次/覆盖表各一行。
6. `cx/spec.md` 与三源**不动**；CX-G-3 仍另案。
