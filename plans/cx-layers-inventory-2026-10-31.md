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

## 8. 方案 B 修订：状态门禁、全源对账、上限与行号（2026-10-31，未施工）

本节覆盖 §7 中被指出的四处缺口；§7 其余条款（词法 12 条、复用、判据不裁 G-3）不变。
实施仍归本刀路径，批次登记与 omp-go 协调。

### 8.1 状态模型：UNKNOWN 必须是非零退出，不能靠文本

现行批次门禁链路（实测读码）：

- `tests/batches.tsv:332` 的 `expect=rc0:CX_LAYERS`；
- `tests/run-batch.cx:39`：`ok = !transport && rc==0 && candidate 未变 && expect 前缀 rc0:
  && marker 恰好出现 1 次`；`ok` 才写 `status=PASS`，否则 `FAIL`（只有 transport 超时才
  `UNKNOWN`），进程返回 `ok?0:timed_out?3:1`。

因此只打印文本 `UNKNOWN` 仍会 `rc=0` → 被记 PASS。改为三档退出：

| rc | 条件 | 批次判定 |
|---|---|---|
| 0 | `failed==0 && undecided==0` | PASS |
| 1 | `failed>0`（层向、白名单、PARSE-CONDITIONAL、I/O） | FAIL |
| 3 | `failed==0 && undecided>0`（词法覆盖不完整 / 对账不一致） | FAIL |

`rc=3` 与 `rc=1` 在 `BATCH_RUN` 行的 `rc=` 字段可区分，原因由 stdout 的
`UNDECIDED` 行给出；**不给 `status=UNKNOWN`**，因为 `run-batch` 只在超时时写它，
与其冲突会制造假 UNKNOWN。`--fixtures` 模式同一规则。

新增必打行（审计与 `--fixtures` 都打）：

```
CX_LAYERS_SCOPE  sources=<审计域源数> manifest=<manifest 行> tracked_regular=<数> symlink_skipped=<数> archive_excluded=<数> out_excluded=<数> reconcile=OK|MISMATCH|UNDECIDED
UNDECIDED  <KIND>  <path>:<line>  <detail>          （每条形一条，逐条命名）
```

`CX_LAYERS` 行追加 `undecided=<数>`，保持既有字段顺序。

### 8.2 全源双向对账（可执行，不用 U 行当唯一事实）

问题：`#pragma unisa \` 续行时 `symindex` 实测无 `U` 行（`tests/splice.cx` 的 `dump`
只有 `M`/`D`），编译器却接受该文件 ⇒ 该 `use` 的边与条件检查全部缺失，且**只消费
`U` 行永远发现不了**。

做法：把 `cl_locate` 的「按 `U` 行查源码」改为「每源一次扫描，按模块名建表」，再与
`U` 行做双向多重集合对账。

每源一次扫描 `cl_scan(abs)`（词法同 §7.2）：

- 产出 `modtab[k] = {name, occurrences, min_cond_line}`，`k < 128`；
- 溢出（同文件不同模块数 > 128）→ `UNDECIDED TOO-MANY-MODULES`，不静默截断。
  上限依据实测：非归档 tracked `.cx` 中单文件 `use` 行最多 49
  （`tools/install-fixes/uses48/main49.cx`），候选每单元上限 48；128 留两倍余量。

对账（`cl_shard_file` 内，用同一份 `U` 行）：

| 情形 | 输出 | 边计数 |
|---|---|---|
| `U` 行在表中有对应 occurrence | 正常，按 §7.4 判 `cond` | 不变 |
| `U` 行次数 > 表中次数（EXTRA-U） | `UNDECIDED EXTRA-U` | **仍按现行计边**（不缩覆盖），但 `undecided++` |
| 表中 occurrence 多于 `U` 行（MISSING-U） | `UNDECIDED MISSING-U path:line use M` | 该次出现无边（现状如此），`undecided++` |
| MISSING-U 且该次出现 `cond>0` | `UNDECIDED MISSING-U-CONDITIONAL path:line use M` | 同上 |
| 源不可读 / 超过 `CL_CAP` | `UNDECIDED UNREADABLE` | `undecided++` |

对账域与身份（`4df9cd59ed` 工作树，全部实测）：

- tracked `.cx` **1943**；其中 mode 120000 软链 **42**；regular **1901**。
- `manifest.tsv` 行 **1902** = 1901 regular + 1 行生成物 `out/` 源。
- regular 中 `archive/` **288**；审计域 **1613** = 1901 − 288 = 1902 − 288 − 1。
- 软链 42 个不在 manifest，也不在审计域；它们是 `cx/sys`、`cx/mem` 等的别名视点，
  目标文件本身已被审计。工具须在 `CX_LAYERS_SCOPE` 里把它们记为 `symlink_skipped`，
  不得表述为「全仓 `.cx` 已覆盖」。
- 另有工作树中 2 个被忽略的 `.cx`（不在 `git ls-files`），明确排除并计入
  `UNDECIDED IGNORED-SOURCE`（若实现选择扫描）。

盲区（明确不保证）：若 `symindex` 与词法扫描**同时**漏掉同一 `use`，对账仍一致。
词法镜像的是编译器（`cxm_load`/`cxm_scan`），故该盲区等价于「编译器自己也看不见」，
但**不能**由此宣称完整对齐；资格状态取决于 `reconcile=OK` 这一实测项。

### 8.3 上限：显式拒绝，不静默少扫

废除 §7.3 的「同模块 >8 次返回负值」。改为：

- 每个 `(源, 模块)` 的出现次数不设上限（扫描是流式的，只累计计数与最小条件行，不存列表）；
- 只有**不同模块数**有 128 上限，超限 → `UNDECIDED TOO-MANY-MODULES` + `rc=3`；
- 模块名长度上限仍为 128（与 `cl_cur_mod` 一致），超长 → `UNDECIDED LONG-MODULE-NAME` + `rc=3`。

### 8.4 重复 `use` 与别名的边合同不变

- 同一模块出现多次：`U` 行多次；边仍按 `seen[]` 的 `realpath` 去重（现行
  `cl_shard_file` 行为），**不改**；
- `#pragma unisa use m::sym as alias`：`U` 行与词法扫描都取 `::` 之前的主模块名；
- 唯一新增判定：**任一** occurrence 处于 `cond>0` 即 `FAIL PARSE-CONDITIONAL`，
  诊断行取最小的条件内物理行。理由见 §7.1（`dbl` 实测：编译器拒 `:4`，现行工具
  `failed=0` 且计 1 边）。

### 8.5 行号未决项（独立，不改编译器）

记为未决条目 `OPEN-LEX-LINE`：`cxm_directive` 在含续行的指令里不增 `line`，实测
`#if \` 换行 `0` 的 pragma 编译器报 `:3`、物理第 4 行。本方案报**物理行**，与编译器
诊断差 1。两条路都不在本次范围：镜像编译器（改工具）或改 `cxm_directive`（改三源，
禁止）。当前处置：工具报物理行，并在 `plans/` 记该差异；谁消费行号谁必须知道差 1。
不修编译器、不改 spec。

### 8.6 夹具补充（在 §7.5 的 12 条之外）

| id | 形状 | 断言 |
|---|---|---|
| `k-missing-u` | `#pragma unisa \` 换行 `use b`（索引无 `U`） | `undecided>=1`、`failed==0`、rc==3 |
| `k-extra-u` | 构造函数里 `U` 行有、源码注释内才有的模块 | `undecided>=1`、rc==3、`edges` 不缩 |
| `k-missing-u-cond` | 续行 `use` 位于 `#if 0` 内 | `UNDECIDED MISSING-U-CONDITIONAL`、rc==3 |
| `k-many-modules` | 129 个不同模块的 `use` | `UNDECIDED TOO-MANY-MODULES`、rc==3 |
| `k-scope` | 任一正例 | `CX_LAYERS_SCOPE` 的六个计数与实际目录一致 |

`--fixtures` 的 `checks` 由「+12」改为「+17」，`failed=0`，且至少一条正例断言
`reconcile=OK`。

### 8.7 验收（更新 §7.8）

1. `--fixtures`：`checks` = 原值 + 17，`failed=0`；`k-*` 五条各自 rc==3 或断言命中。
2. 全量审计：`edges`/`amb`/`out`/`unres`/`failed` 与上一收据（`4df9cd59ed` 工作树，
   `edges=2401 … failed=0`）逐项比对；`undecided` 打印**实测值**（预期 0，但不预写）；
   若 `undecided>0` 则本次交付**不是** PASS，须先解释每条形。
3. `CX_LAYERS_SCOPE` 六项计数须与本节的 1613/1902/1901/42/288/1 一致才算对账成功。
4. `tools/affected.cx --files tools/check-cx-layers.cx --run`、`tools/ratchets.cx`。
5. 提交：`tools/commit-own.cx --whole tools/check-cx-layers.cx` + 批次/覆盖两行。
6. spec 与三源不动；CX-G-3 另案；`OPEN-LEX-LINE` 保持未决。

## 9. 方案 B 第二次修订：绑定索引实际合同、域等式动态推导、输入身份（未施工）

回应 cdx 三点：数量不得钉死、先实证索引是否去重、源/manifest 同一输入且漂移须
UNDECIDED。**§8.7 验收里的 1613/1902/1901/42/288/1 一律降级为 `1f4129d306` 那次
收据，不再是断言值。**

所有探针在仓外固定快照 `/tmp/pinH`（`git archive 1f4129d306`）上跑：`cx/{sys,mem,text,
core,runtime}` 与 `tools/{symindex,check-cx-layers,cxlib/netpack}.cx` 全部来自快照，
不使用当前工作区（工作区 `cx/sys/fs.cx` 有他人未提交变更，已按 cdx 指示不再触碰、
不重跑该失败路径）。探针树 `/tmp/cxlprobe.o1YEsz`，每个文件先 `git add`/`commit` 再让
工具看见。

### 9.1 索引实际合同（实测，决定对账怎么比）

`tools/symindex.cx` 的 `directive()` 逐行处理，**不做任何去重**；只在行首（跳过
`<= 32` 字节后）遇到 `#` 才进入指令解析。实测：

| 探针 | 源 | `dump` 的 U 行 | 编译器 `-cx-plan` / 编译 |
|---|---|---|---|
| m1 | `use b` 两行相同 | `U b` ×2 | 含 `tests/b.cx` |
| m5 | `use b::b_x as ax`、`use b`、`use b::b_y as ay` | `U b` ×3 | 含 `b.cx` |
| m2 | `use b::b_x as ax`、`use b::b_y as ay` | `U b` ×2 | 含 `b.cx` |
| m4 | `use b`、`use b::b_x as ax` | `U b` ×2 | 含 `b.cx` |
| e1 | `use b as bb`（无 `::`） | `U b` | 含 `b.cx` |
| e2 | `use b::b_x` | `U b` | 含 `b.cx` |
| f5 | 三行完全相同的 `use b` | `U b` ×3，**shard 内也是 3 行** | 含 `b.cx` |
| f1 | 块注释与 `//` 内各一条 `use b` | **0 行** | 不含 `b.cx` |
| g1 | 行首 `/* start`，其内 `#pragma unisa use b` | 0 行 | 不含 |
| g4 | 行中 `int zz; /* start` 跨行的注释内 `use` | 0 行 | 不含 |
| g2 | `#if 0` 内的块注释内的 `use` | 0 行 | 不含 |
| g6 | `/* one-line */ #pragma unisa use b` | `U b` | **含 `b.cx`**（与索引一致） |
| e4 | 行中 `int zz(void){return 1;} #pragma unisa use b` | 0 行 | 不含 |
| f3 | `#ifdef NOPE` 内 `use b` | `U b` | 不含（条件为假） |
| c4 | `use b\` + 空行 | `U b` | 含 |
| c5 | CRLF 行尾 | `U b` | 含 |
| c6 | `#pragma⇥unisa⇥use⇥b` | `U b` | 含 |

**结论**：多重集对账安全——索引保留同模块多次 `use`（含不同 `::sym`/`as`），不会把
合法重复压成一条，因此「表比 U 多」不会因去重被误判成 MISSING-U。`::` 之前取主名、
`as` 别名忽略，与 §7.2 规则 11 一致。`g6` 说明「`#` 前只有空白/注释残骸」算行首
（`<= 32` 字节不清行首），工具必须照此实现，否则会对 g6 类输入报 EXTRA-U。

唯一实测出的单向差异在**续行拆词**：

| 探针 | 源 | 索引 | 编译器 |
|---|---|---|---|
| c1 | `#pragma unisa \` 换行 `use b` | **0 U 行** | 含 `b.cx` |
| c2 | `#pragma \` 换行 `unisa use b` | **0 U 行** | 含 `b.cx` |
| c3 | `#pragma unisa use \` 换行 `b` | **0 U 行** | 含 `b.cx` |

即索引按物理行取词、不拼接续行；cxmod 先把 `\`+`\n` 变 `0x01`+`\n` 再解析，故能看见。
这类输入今天的层审计**完全无边**（`edges` 低报）。现网实测：tracked `.cx` 中
`^[[:space:]]*#[[:space:]]*pragma.*\\$` 命中 **0 个文件**，`#pragma unisa use ...\`
同样是 0——是实测值，不是恒等式。

### 9.2 对账绑定索引合同（三重判定，边数只增不减）

每源一次词法扫描得 `scan[mod] = {occurrences, min_cond_line}`；与 shard 的 `U` 行比：

| 判定 | 含义 | 边计数 | 状态 |
|---|---|---|---|
| `OK` | 次数相等 | 按 §7.4 判 cond | — |
| `MISSING-U` | 词法多于索引（续行拆词类） | **保持按 U 行计边，不新增** | `UNDECIDED MISSING-U path:line use M` → rc3 |
| `MISSING-U-CONDITIONAL` | 同上且该次出现 `cond>0` | 计边不变；本应 FAIL 的漏掉了 | `UNDECIDED` → rc3 |
| `EXTRA-U` | 索引多于词法 | 按 U 行计边不变 | `UNDECIDED EXTRA-U` → rc3（未观测到，但不假定为零） |

要点：MISSING-U **不**悄悄补边（补边会改变 `edges` 收据让人误以为覆盖提升），也**不**删
索引事实；它只把「覆盖不完整」变成非零退出并逐条命名。补边属于另案（扩展事实源），
需单独批准。

### 9.3 域等式动态推导（不钉数字）

输入三份，同一身份：`manifest.tsv`、shard 目录、`git ls-files -s -z -- '*.cx'`
（先例：`tools/affected.cx:441`）。运行期求集合：

- `T` = git 列出的 tracked `.cx`；`S` = `T` 中 mode `120000` 的子集；`R = T ∖ S`；
- `M` = manifest 行路径集合；`G = M ∖ R`（生成物，须全在 `out/` 下）；
- `D = M ∖ archive/ ∖ out/`（审计域）。

检查并打印（任一不成立 → `UNDECIDED <KIND>` → rc3，绝不静默）：

1. `|T| = |R| + |S|`，且 `R ∩ S = ∅`（集合相等，不是数量相等）；
2. `|M| = |R| + |G|`，`R ∩ G = ∅`，且 `G ⊆ out/`；`R ∖ M = ∅`（tracked regular 全在
   manifest）。若有 `R ∖ M` 非空或 `G ⊄ out/` → `UNDECIDED DOMAIN-MISMATCH` 并列出前 8 条。
3. `D` 中每个路径都有对应 shard 文件；shard 目录里每个 `*.tsv`（除 `manifest.tsv`）
   都被某个 manifest 行引用，否则 `UNDECIDED ORPHAN-SHARD`。
4. `git` 不可用 / 非仓 → `UNDECIDED NO-GIT`（不降级为「跳过」）。

`CX_LAYERS_SCOPE` 打**实算值**：

```
CX_LAYERS_SCOPE tracked=<|T|> regular=<|R|> symlink=<|S|> manifest=<|M|> generated=<|G|>
                 archive=<|M∩archive|> out=<|M∩out|> domain=<|D|> partition=OK|MISMATCH
```

`1f4129d306` 上的实算值是 `1943/1901/42/1902/1/288/1/1613`，仅作该快照的收据；
共享树变化后应随之变化，等式仍须成立。

### 9.4 输入身份与漂移

- **源**：`cl_scan` 读取源文件时顺带 `cx_sha256_hex`，与 manifest 该行 sha 比较；不等
  → `UNDECIDED SOURCE-CHANGED path manifest=<sha8> read=<sha8>`，rc3，且该源不参与边计数
  之外的任何推断（已扫结果保留但标记）。
- **manifest**：读取前 `sy_stat` 记 size/mtime/ino，审计结束后再 `sy_stat` 比对；变化 →
  `UNDECIDED MANIFEST-CHANGED`，rc3。
- **索引与源同一输入**：工具在审计前自己跑 `symindex update`（`main` 现状），故 manifest
  与源来自同一时刻；上面的 sha 比对覆盖「update 之后、逐源读取之前」的漂移窗口。
- 身份字段全部打印在 `CX_LAYERS_SCOPE` 与 `UNDECIDED` 行，不进入 PASS 判据之外的隐含状态。

### 9.5 盲区表述（不得写成 reconcile=OK 即全覆盖）

- 若索引与词法**同时**漏同一 `use`，对账仍 `OK`。词法镜像 `cxm_load`/`cxm_scan`，因此
  该盲区等于「编译器自己也看不见」；但工具只能声称：**在该输入上未观测到分歧**，
  且 `partition=OK ∧ identity 稳定 ∧ undecided=0`。不得表述为「全仓 `.cx` 层向覆盖完整」。
- 软链 `S`（当前 42 个）不在 manifest 也不在审计域；它们指向的目标文件本身在域内。
  scope 行必须显式给出 `symlink=`，不得省略。
- 被忽略的工作树 `.cx`（当前 2 个）不在 `T`；若选择扫描须单列 `UNDECIDED IGNORED-SOURCE`。
- 续行拆词（c1–c3）属 MISSING-U：既证明索引会漏，也证明「光看 U 行」不可靠；但补边
  另案，本次仍以 U 行为边事实源，只是不再宣称覆盖完整。

### 9.6 夹具（在 §8.6 的 5 条之外，总 +12 → +24）

| id | 形状 | 断言 |
|---|---|---|
| `k-multi-same` | 三行相同 `use b` | `undecided==0`、`edges` 计 1（realpath 去重）、rc0 |
| `k-multi-alias` | `b::b_x as ax` + `b::b_y as ay` | 索引 2 U、词法 2 次、对账 OK、`edges` 1、rc0 |
| `k-index-gap` | `#pragma unisa \` 换行 `use b` | `UNDECIDED MISSING-U`、`edges` 与无该行时相同、rc3 |
| `k-index-gap-cond` | 同上但位于 `#if 0` 内 | `MISSING-U-CONDITIONAL`、rc3、不产生 FAIL |
| `k-comment-only` | 只有注释里的 `use b` | 0 U、0 次、对账 OK、`edges==0`、rc0 |
| `k-trailing-comment` | `/* x */ #pragma unisa use b` | 对账 OK（`#` 前只有空白算行首）、`edges==1`、rc0 |
| `k-source-changed` | 写入→dump→改写→直接调 `cl_shard_file` | `UNDECIDED SOURCE-CHANGED`、rc3 |

测试点：`k-multi-*` 证明多重集不会把合法重复误判；`k-index-gap*` 证明漏 U 非零退出；
`k-trailing-comment` 钉住 `<= 32` 不清行首这一条（g6 实测）。

### 9.7 状态与退出码（沿用 §8.1，补充 KIND 清单）

`rc0`：`failed==0 ∧ undecided==0 ∧ partition==OK ∧ identity 稳定`。
`rc1`：`failed>0`。`rc3`：`failed==0` 且（`undecided>0 ∨ partition!=OK ∨ identity 不稳`）。

`UNDECIDED` 的 KIND 全集：`MISSING-U`、`MISSING-U-CONDITIONAL`、`EXTRA-U`、
`TOO-MANY-MODULES`、`LONG-MODULE-NAME`、`UNREADABLE`、`SOURCE-CHANGED`、
`MANIFEST-CHANGED`、`DOMAIN-MISMATCH`、`ORPHAN-SHARD`、`NO-GIT`、`IGNORED-SOURCE`。
每条形 `UNDECIDED\t<KIND>\t<path>:<line>\t<detail>`，同时计入 `undecided=`。

### 9.8 验收（替代 §8.7）

1. `--fixtures`：`checks` = 原值 + 24，`failed=0`；`k-*` 各自断言命中（含 rc3 类）。
2. 全量审计：`CX_LAYERS_SCOPE` 的实算值与本快照收据的**等式**成立（数量可变，等式不可
   破）；`undecided` 打实测值，>0 即本次交付不是 PASS；`edges/same/down/up/amb/out/unres`
   与 `1f4129d306` 收据逐项比对，差异须有来源说明。
3. `tools/affected.cx --files tools/check-cx-layers.cx --run`、`tools/ratchets.cx`。
4. `tools/commit-own.cx --whole tools/check-cx-layers.cx` + 批次/覆盖两行。
5. spec 与三源不动；G-3 不裁；`OPEN-LEX-LINE` 保持未决；补边（MISSING-U 转正）另案。

## 10. 方案 B 第三次修订：非原子快照的可观测边界、git index 身份、shard 缓存合同（未施工）

回应三点：不得把「update 后逐源比 sha」说成同一时刻；`git ls-files -s` 读的是共享
index + 工作树，不是 commit 树；内容寻址缓存允许旧 shard 残留，ORPHAN-SHARD 不能当错误。
**§9.3 的「集合等式 + ORPHAN-SHARD 为错」在此被取代。**

本节所有数字都是**当前共享工作树**的观测值（HEAD `32744f7421`，工作区有他人改动），
只作当次收据，不进断言。

### 10.1 快照不可原子：只做可观测的漂移检测

`symindex update` 与逐源审计不是一个事务，中间任何时刻其他源都可能变。而且还叠了
`index_one` 的 stat 预过滤：

```c
if (m >= 0 && !verify && man[m].size == st.size && man[m].mtime == st.mtime
    && man[m].ino == st.ino && man[m].dev == st.dev)   /* 直接沿用旧 sha，不读文件 */
```

`mtime` 只有秒粒度（`sy_stat_t` 自带注释），所以「同秒内同尺寸改写」会被判为未变，
manifest 里的 sha 与实际内容不符。因此工具**必须自己算 sha**，不能信 manifest 的 sha。

记录与判定（全部可观测，不涉及任何假想冻结）：

| 观测 | 时机 | 不成立时 |
|---|---|---|
| manifest 的 `size/mtime/ino/dev` + 自身 sha256 | 读取前、审计结束后各一次 | `UNDECIDED MANIFEST-CHANGED` → rc3 |
| 每源 `lstat` 元组（size/mtime/ino/dev） | 读前、读后各一次 | 两次不等 → `UNDECIDED SOURCE-CHANGED`（读期间被改） |
| 每源内容 sha256 | 读取后 | ≠ manifest 该行 sha，且元组与 manifest 行相同 → `UNDECIDED PREFILTER-STALE`（预过滤说谎）；元组也不同 → `UNDECIDED SOURCE-CHANGED` |
| 全审计域 `lstat` 元组复检 | 审计结束后 | 与读时不同 → `UNDECIDED SOURCE-CHANGED-AFTER-READ` → rc3 |
| 交付前的最终复核 | 打印 `CX_LAYERS` 之前 | 任一源变更 → 整体降级为 rc3，`CX_LAYERS` 行加 `window=MOVED` |

措辞纪律：工具只能声明

> 在该次运行的可观测窗口 `[首个源读取, 末次复检]` 内，未观测到源/manifest 漂移

**不得**写「索引与源同一时刻」「快照冻结」「原子视图」。窗口之外（工具退出之后、
或某源在其自身复检之后又变）无法观测，写入 `plans/` 的收据必须带窗口与 HEAD。

另外两条已读码确认的陈旧来源：

- `save_manifest` 在 `dirty` 为假时**完全不重写** manifest，因此 manifest 的
  `mtime/ino` 与树无关，可能是上一次有脏变更时的产物；
- `update` 因 45 s 上限 `DEFERRED` 时，`save_manifest` 会**保留旧行**给这次没走到的
  条目（`n_deferred` 分支），即 manifest 可含上一代的行；`symindex` 对未完成索引的查询
  以 rc3 退出，`check-cx-layers` 的 `rc` 分支已经把它变成 `FAIL SYMINDEX`——保持该行为，
  不降级。

### 10.2 `git ls-files -s` 的身份：index + 工作树，不是 commit 树

`symindex` 的文件集来自 `git ls-files -s -z -- '*.cx'`（`set_tracked`），工具端的域等式也
只能取同一来源。它是**共享检出**的 index 视图，会随他人 `git add` 变化，且不反映工作树
是否被改。因此：

- 记录 `git rev-parse --git-path index` 得到的 index 路径，读前/读后各取一次
  `size/mtime/ino`；变化 → `UNDECIDED GIT-INDEX-CHANGED` → rc3。
- 记录 `HEAD`（`git rev-parse HEAD`）仅供收据，**不作为域的定义**；打印
  `tree_source=index+worktree`，禁止出现 `commit tree`/`HEAD tree` 字样。
- index 条目 ≠ 工作树文件：`git ls-files` 会列出被 `rm` 但未 `git add -u` 的路径。
  今天实测：`R∖M` 恰 1 条 `tests/runner-faults.cx`（**文件实际存在、6680 字节**，
  只是 manifest 来自更早一次运行）——这正是「manifest 不是当前树」的实证。
  分类：
  - `R∖M` 且文件存在 → `UNDECIDED NOT-INDEXED path`（真实覆盖缺口）→ rc3；
  - `R∖M` 且文件不存在 → `UNDECIDED INDEX-ONLY path` → rc3（不得计为已审计）；
  - 若文件在 `archive/` 或 `out/`（域外）则记入 scope 的 `excluded=` 而非 undecided。
- `git` 失败 / 非仓 → `UNDECIDED NO-GIT`，不降级为「跳过」。
- `assume-unchanged` / `skip-worktree` 条目今天的检测不可靠：记 UNKNOWN，不声称已覆盖。

### 10.3 shard 缓存合同：内容寻址、写一次、不回收

读码与实测确认（`tools/symindex.cx` 头注释与 `index_one`/`write_atomic`）：

- shard 名为 `SHA256.tsv`，**按内容寻址**；只有在该 sha 的 shard 不存在时才解析并写
  （`if (r == 0) { parse(...); write_atomic(...) }`），写用临时文件 + `rename`；
- **没有任何 GC / 删除 / 清扫路径**（全文无 `unlink` 针对 shard 的调用）；
- 因此历史内容版本的 shard 会永久留存。

当前实测：manifest 1902 行、唯一 sha **1896**（6 行共享内容，例如
`tools/cxlib/pytempfile.cx` 与 `archive/.../pytempfile.cx` 同内容）、shard 文件 **1916**
⇒ **20 个未被引用的 shard**，最旧的可追溯到更早的源版本。这是**合同允许的正常状态**。

据此定判定（替代 §9.3 第 3 条）：

| 观测 | 判定 |
|---|---|
| 被引用 sha 的 shard 缺失 | `UNDECIDED MISSING-SHARD path` → rc3（现行 `cl_shard_file` 已 `FAIL SHARD`，保留） |
| 目录里存在未被引用 shard | `cache_orphans=N`，仅信息，**不影响 rc**，不得报错 |
| shard 行数与 manifest 引用数不一致 | 正常（多对一 + 残留），不参与等式断言 |

即：不对「每个 shard 必须被引用」做任何断言；只对「每个被引用 sha 必须有 shard」断言。

同源纠正：`manifest` 会出现**生成物行**。实测当前有 1 行
`out/cx-layers-fixtures-2985674/h-conditional/cx/sys/q.cx`，这是 `symindex dump FILE`
把命令行点名的文件加入索引所致（头注释明写「files named on the command line are added」），
`check-cx-layers` 的 `fx_dump` 正是这么调的。因此 `M∖R` 允许含 `out/` 行；只有出现在
`out/` 之外的 `M∖R` 才 `UNDECIDED DOMAIN-MISMATCH`。

### 10.4 更新的 scope 行

```
CX_LAYERS_SCOPE tree_source=index+worktree head=<HEAD> tracked=<|T|> regular=<|R|> symlink=<|S|>
                 symlink_broken=<n> manifest=<|M|> generated_out=<n> stale_rows=<n>
                 archive=<n> out=<n> domain=<|D|> cache_orphans=<n> window=<STABLE|MOVED>
```

各字段均为实算值，不做等式断言；等式只用于**跳变检测**（上一节的三条规则改成"发现不一致就
UNDECIDED 并列出前 8 条"，不再要求固定数量）。

### 10.5 夹具（在 §9.6 的 +24 之外，总 +27）

| id | 形状 | 断言 |
|---|---|---|
| `k-not-indexed` | 写文件但不入索引（模拟 manifest 早于树） | `UNDECIDED NOT-INDEXED`、rc3、该文件不计边 |
| `k-orphan-shard` | 索引目录里多放一个未被引用的 `SHA.tsv` | `cache_orphans>=1`、`undecided==0`、**rc 不变** |
| `k-missing-shard` | 删除某被引用 sha 的 shard | `FAIL SHARD`（或 `UNDECIDED MISSING-SHARD`）→ 非零 |

`k-orphan-shard` 是本次唯一"必须证明不误报"的用例：旧 shard 存在时结果必须与不存在时
完全一致（`edges`/`failed`/rc 全同）。

### 10.6 验收（替代 §9.8）

1. `--fixtures`：`checks` = 原值 + 27，`failed=0`；`k-orphan-shard` 与去掉该 shard 的
   对照用例结果逐字段相同。
2. 全量审计：`CX_LAYERS_SCOPE` 打实算值 + `window=STABLE`；`undecided` 实测值 >0 即非
   PASS；`edges` 与上一次同输入收据比对，差异须给来源。
3. 收据文字必须带：HEAD、可观测窗口、`tree_source=index+worktree`、`cache_orphans`，
   并注明「漂移检测，非原子冻结」。
4. `tools/affected.cx --files tools/check-cx-layers.cx --run`、`tools/ratchets.cx`。
5. spec 与三源不动；G-3 不裁；`OPEN-LEX-LINE` 未决；补边另案；shard GC 另案（不在本工具）。

## 11. 方案 B 第四次修订：内容摘要绑定实际扫描字节、索引内容身份、ABA 限制与代价（未施工）

回应三点：manifest 与共享 git index 不能只靠元数据；sha 必须算在被词法扫描的同一份
缓冲上且结束重读；两端哈希一致也排除不了中间 ABA。本节给出可执行方案、实测代价、
权限与 deadline，不用缩范围或静态计数掩盖。

### 11.1 三个身份，都要内容摘要

| 身份 | 取法 | 前后各一次 | 不一致 |
|---|---|---|---|
| manifest 内容 | `cl_read` 到 `cl_man` 后**立刻**对 `cl_man[0..n)` 做 sha256（解析前） | 结束后重读整文件再哈希 | `UNDECIDED MANIFEST-CHANGED` → rc3 |
| git 索引视图 | 我们消费的 `git ls-files -s -z -- '*.cx'` **原始字节**做 sha256（当前 151646 B） | 结束后重新 spawn 一次同一命令再哈希 | `UNDECIDED GIT-INDEX-VIEW-CHANGED` → rc3 |
| git 索引文件 | `$(git rev-parse --git-path index)` 整文件哈希（当前 967484 B ≤ `CL_CAP`） | 结束后重读重哈希 | `UNDECIDED GIT-INDEX-CHANGED` → rc3 |

关于「索引视图」作为主身份的理由（写进代码注释，避免后来者误读）：工具的域不是
index 二进制，而是它经 `git` 解析后交给我们的那份列表；列表字节就是工具真正消费的内容。
索引文件哈希是补充的跳变探针，`assume-unchanged`/`skip-worktree`/合并标记都会改变列表
输出，因此列表哈希覆盖得更直接。两者都记录，任一变化即 rc3。

索引侧还须显式判定的解析事实（现在的 `symindex` 只取 mode、不读 stage，所以工具必须自己判）：

| 观测 | 判定 |
|---|---|
| `git ls-files -s -z` 退出码非 0 | `UNDECIDED NO-GIT`（附 rc 与 stderr 前 200 B） |
| 任一记录 stage ≠ 0（未合并条目） | `UNDECIDED GIT-UNMERGED path`，最多列 8 条 |
| 记录字段数 ≠ 3、mode 非 6 位、sha 非 64 位十六进制、路径为空 | `UNDECIDED GIT-PARSE path`，最多列 8 条 |
| 索引文件 `size > CL_CAP` | `index_file_sha=TOO-LARGE`，以视图哈希为主身份，**在 scope 行显式写出**，不静默省略 |
| 索引文件 EACCES/其他读错误 | `UNDECIDED GIT-INDEX-UNREADABLE errno`（不得当作「跳过」） |
| `.git/index` 被替换（ino/dev 变但内容同） | 不算漂移：内容哈希相同即视为同一输入；但把 `index_ino_changed=1` 打进 scope 供审计 |

### 11.2 sha 绑定实际被扫描的字节

顺序固定为：`cl_read` → **哈希原始 `cl_src[0..n)`** → `cl_clean(n)` 原地清洗 → 词法扫描。

- 清洗只把字节替换成空格/`0x01`，长度与换行位置不变，所以哈希取清洗前即可代表
  「这份被扫描的字节」；不得对清洗后的缓冲再哈希（那哈希的是派生数据，不是输入）。
- `n == CL_CAP - 1` 表示被截断 → `UNDECIDED UNREADABLE path source-over-1MiB`，不得
  截断后继续扫描。
- 同一次循环内记录 `lstat` 元组（size/mtime/ino/dev）读前值；哈希后不再用它判断
  「内容未变」——元数据只能发现一部分漂移（同尺寸同秒改写、`mtime` 还原都会绕过）。

**结束重读**（不能只有 lstat）：审计主循环把每个域源的 64 位十六进制摘要存进
`static char cl_dig[4096][65]`（4096 × 65 = 260 KiB 静态；现域 1613，manifest 1902），
超过 4096 → `UNDECIDED TOO-MANY-SOURCES` → rc3。主循环结束后**重新 open 并读每个域源**，
哈希后与 `cl_dig` 比：

- 不等 → `UNDECIDED SOURCE-CHANGED-AFTER-READ path`（同时给出两个 sha 前 12 位）；
- 重读失败 → `UNDECIDED UNREADABLE`；
- 相等 → 记为该次运行「在该源上未观测到内容漂移」。

第二次读必须走 `sy_open_read` 重新打开，不得复用第一次的缓冲内容。

### 11.3 只声称可观测漂移，不给原子证明

即便首尾两端哈希一致，也排除不了窗口内的 A→B→A：工具在这两点之间没有任何观测点。
因此措辞上限固定为：

> 在本次运行的可观测点 `{首读哈希, 末次重读哈希}` 上，未观测到该源的内容漂移。

不得写「冻结」「原子」「同一时刻」「内容未变」。写入 `plans/`/收据时必须同时给出
HEAD、域规模、两次哈希所用算法路径（`digest=afalg|soft`）与窗口标记 `window=`。
`AF_ALG` 路径本身也可能失败（非 Linux、内核未编 `CONFIG_CRYPTO_USER_API_HASH`）→
退回 `cx_sha256_hex` 并在 scope 行写 `digest=soft`，不得因此跳过校验。

### 11.4 代价、权限、deadline（实测，绑定 `/tmp/pinH` = `1f4129d306` 快照与本机）

实测（本机，`unisacc.com` 解释执行）：

| 路径 | 块 | 次数 | 总字节 | 耗时 | 吞吐 |
|---|---|---|---|---|---|
| `sy_sha256_hex`（AF_ALG） | 587351 | 20 | 11.75 MB | 65 ms | ≈181 MB/s |
| `sy_sha256_hex`（AF_ALG） | 8328 | 3226 | 26.9 MB | 188 ms | ≈143 MB/s（≈58 µs/次） |
| `cx_sha256_hex`（软件） | 587351 | 20 | 11.75 MB | 1365 ms | ≈8.6 MB/s |
| `cx_sha256_hex`（软件） | 8328 | 3226 | 26.9 MB | 3072 ms | ≈8.7 MB/s |

域实测（当前共享树 manifest，仅当次收据）：1613 文件 / 13,432,673 B，均值 8328 B，最大
587,351 B。两趟（首读 + 末次重读）合计哈希 **26.9 MB**：

- AF_ALG：≈0.19 s；
- 软件回退：≈3.1 s（最坏情形，非 Linux）。

另加：manifest 两趟 ≈0.4 MB（可忽略）、索引文件 967 KB × 2（AF_ALG ≈11 ms / 软件 ≈0.23 s）、
`git ls-files` 视图 151 KB × 2 + 两次 spawn（各 ~10–20 ms）。
合计新增最坏 ≈3.5–4 s，批次 `budget_seconds=55` 足够。

deadline 规则（不许用缩范围换取时间）：

- 内部常量 `CL_DEADLINE_MS = 40000`（批次 55 s 的余量）；
- 在四个相位边界检查：`symindex update` 后、主循环每 256 个源、末次重读每 256 个源、
  打印前；
- 超限 → `UNDECIDED DEADLINE phase=<update|scan|redigest|report>` → rc3；
- **不允许**为了不超时而跳过末次重读、抽样哈希、或用文件计数代替摘要。

权限：

- 全部读取用 `sy_open_read`（无 `O_NOFOLLOW` 变化，保持现状），只读共享树；不写除
  `out/cx-layers-index/`（既有）之外的任何路径；
- `.git/index` 只需读权限；CI 上常见 0644；EACCES 必须报 errno，不得降级为跳过；
- `git` 只读子命令（`rev-parse`、`ls-files`），不写 index、不取锁；若 `git` 报
  `index.lock` 存在也不影响只读命令。

### 11.5 夹具（在 §10.5 的 +27 之外，总 +31）

| id | 形状 | 断言 |
|---|---|---|
| `k-digest-buffer` | 源里同时含 `/*`、`"#'`、`#if 0` 等清洗敏感形状 | 摘要取清洗前字节：与直接 `sha256sum` 该文件一致（用编译期期望值写死） |
| `k-unmerged` | 构造 3 个 stage 的索引条目（`git update-index --index-info`） | `UNDECIDED GIT-UNMERGED`、rc3 |
| `k-index-view-change` | 两次 `git ls-files` 之间 `git add` 一个新文件 | `GIT-INDEX-VIEW-CHANGED`、rc3 |
| `k-aba` | 记录首读摘要后改文件再改回（同内容） | **rc0**，且收据只能写「未观测到漂移」，不得写「未变」 |

`k-aba` 是本次唯一「必须证明不能声称更强结论」的用例：断言是「不报警」与
「收据措辞受限」，不是断言内容未变。

### 11.6 验收（替代 §10.6）

1. `--fixtures`：`checks` = 原值 + 31，`failed=0`；`k-aba` 断言 rc0 且 scope 行含
   `window=`；`k-digest-buffer` 的期望值来自外部 `sha256sum`（在 `plans/` 记一次）。
2. 全量审计：scope 行须含 `man_sha=<12>`、`index_view_sha=<12>`、`index_file_sha=<12>`、
   `digest=afalg|soft`、`window=STABLE|MOVED`、`cache_orphans=`；`undecided` 实测值 >0
   即非 PASS。
3. 收据文字受 §11.3 约束：只写「可观测点上未观测到漂移」。
4. `tools/affected.cx --files tools/check-cx-layers.cx --run`、`tools/ratchets.cx`；
   `srcshape` 记录该工具新增静态内存（+260 KiB `cl_dig`）与新增 `#pragma unisa use sys.sha`
   （第 8 条，远低于 48 上限）。
5. spec 与三源不动；G-3 不裁；`OPEN-LEX-LINE` 未决；补边另案；shard GC 另案。

## 12. 方案 B 第五次修订：`-s` 不承载跳过标志、索引视图改为 `-s -v`、夹具必须仓外独占（未施工）

**§11.1 关于「assume-unchanged / skip-worktree 已体现在视图里」的断言是错的，本节推翻它。**
以下全部为实测，`git version 2.34.1`。

### 12.1 `git ls-files -s` 不携带任何跳过标志

在 `/tmp/gitflags`（仓外独占临时仓）实测：

```
git update-index --assume-unchanged one.cx
git update-index --skip-worktree  two.cx
git ls-files -s -z   →  100644 <sha> 0<TAB>one.cx   （与普通文件逐字节同形，无标志）
git ls-files -v      →  h one.cx / H three.cx / S two.cx
```

- 小写 = assume-unchanged（`h`），大写 = 未设（`H`）；`S` = skip-worktree；
  `M` = unmerged（配 stage 1/2/3）；`R` = deleted、`C` = modified，但**只有显式给
  `--deleted` / `--modified` 才出现**——只给 `-v` 时，工作树里已删掉的
  `sub/four.cx` 仍输出 `H`。
- 因此：**视图能判 assume-unchanged / skip-worktree / unmerged，不能判工作树删除或修改**。
  删除/修改仍由工具自己的 `lstat` 与读取发现；不得从视图推断。

### 12.2 改为单次 `git ls-files -s -v -z`：标志、stage、路径一次取齐

实测 `-s` 与 `-v` 可同时给，且格式稳定：

```
<tag> <mode> <sha> <stage>\t<path>\0
H 120000 6ee58760…7094 0<TAB>link.cx\0
M 100644 b6802534…d746 1<TAB>three.cx\0   （冲突时三行，stage 1/2/3）
```

- `-z` 输出的是**原始路径字节**，不做 C 风格转义：实测路径含 `TAB`、换行
  （`we ird\tna\nme.cx`）时字节原样穿过；元数据段不含 `TAB`，所以**只在第一个 `TAB` 切分**。
- 解析规则（任一不满足 → `UNDECIDED GIT-PARSE path`，最多列 8 条 → rc3）：
  tag 必须是 `{H,S,M,R,C,K,?}` 的大小写之一；mode 6 位十六进制；sha 64 位十六进制；
  stage ∈ {0,1,2,3}；path 非空；记录内除首个 `TAB` 外不得再出现 `TAB`（否则元数据段被污染）。
  **未知 tag 必须报错，不得当作 `H`**。
- 任一 stage ≠ 0 → `UNDECIDED GIT-UNMERGED path`；tag 为 `M` 而 stage 为 0 属矛盾 →
  `GIT-PARSE`。
- 小写 tag 或 `S/s` → 记 `au=<n> sw=<n>` 进 scope。**这不是错误**：我们的内容 sha 来自
  自己的读取，不依赖 index 的 stat 缓存；但它意味着「index 元数据不可用于变更检测」，
  所以更不能用元数据代替内容哈希（与 §11.2 一致）。
- 当前共享树实测：两趟连续执行字节完全相同，`sha8=9fd8f60d286c8298`、**155534 B**
  （`-s` 单独为 151646 B）；flag 直方图 **1944 H**，无 au/sw/unmerged。
- **pathspec 相对 cwd**：实测在 `out/cx-layers-fixtures-2536959/` 里执行
  `git ls-files -s -z -- '*.cx'` 返回 **0 条**，而 `git rev-parse --show-toplevel` 仍是共享仓根。
  故 git 必须以 `cwd = cl_root`（或 `git -C cl_root`）调用；返回 0 条必须先判
  `UNDECIDED EMPTY-VIEW` 再继续，**不得**当成「0 条边」的正常结果。

索引视图身份随之改为：**`git ls-files -s -v -z -- '*.cx'` 原始字节的 sha256**，首尾各一次；
`.git/index` 文件哈希仍作辅助跳变探针。两者任一变化 → rc3。

### 12.3 会写 index 的夹具必须仓外独占

实测：夹具目录在共享检出内，`git rev-parse --show-toplevel` 指向共享仓根 ⇒ 在那里跑
`git update-index`/`git merge` 会**直接改共享 index**，是事故。

夹具协议（`k-unmerged`、`k-index-view-change` 及任何会写 index 的用例）：

1. 目录用 `$TMPDIR/cxlayers-fx-<pid>-<seq>/`（`sy_pid()`，`sy_mkdir`；`sy_mkdtemp` 当前
   `cx/sys` 无此入口，不新增）。`$TMPDIR` 未设时退到 `/tmp`；创建失败 → `FAIL FIXTURE-SETUP`。
2. `git init -q`，用 `-c user.email=… -c user.name=…` 显式提交，不读全局配置；
   同时设 `GIT_CEILING_DIRECTORIES=<父目录>`，实测可阻止向上发现共享仓
   （`/tmp/gitfx/sub` 解析仍得 `/tmp/gitfx/r`）。
3. **开跑前自检**：该目录里 `git rev-parse --show-toplevel` 必须等于夹具根，否则
   `FAIL FIXTURE-SCOPE` 并停止，绝不继续写 index。
4. 结束 `rm -rf` 该目录；清理失败只记 `fixture_cleanup=FAIL path`，不改用例判定
   （`out/` 之外的残留也不留在共享树）。
5. 夹具内**禁止**设置 `GIT_INDEX_FILE` 指向共享 `.git/index`，也禁止在 `cl_root` 下执行
   任何写型 git 子命令。

### 12.4 代价与 deadline 的更正

- 之前把 AF_ALG/软件吞吐外推说成「预算足够」是**成本估计，不是整批上限保证**。真实超时
  一律 `UNDECIDED DEADLINE phase=…` → rc3，不缩范围、不抽样。
- 实测 git 调用成本：`ls-files -s -v -z -- '*.cx'` ≈**6 ms/次**（5 次 32 ms）、
  `rev-parse --git-path index` ≈**2 ms/次**（5 次 9 ms）；四次调用合计 ≈16 ms。
- **新发现的 deadline 隐患**：现有 `cl_spawn` 用 `np_run(..., 50, ...)`，单次 spawn 上限
  50 s；四次 git spawn 最坏可达 200 s，远超批次 55 s。必须给 git 调用单独的 5 s 上限，
  超时 → `UNDECIDED DEADLINE phase=git`，不得沿用 50 s。
- 哈希成本沿用 §11.4 实测（两趟 26.9 MB：AF_ALG ≈0.19 s / 软件 ≈3.1 s）；软件路径 3.1 s
  已占 `CL_DEADLINE_MS=40000` 的 8%，属可接受，但不得据此宣称「一定不超时」。

### 12.5 夹具（在 §11.5 的 +31 之外，总 +36）

| id | 形状 | 断言 |
|---|---|---|
| `k-au-flag` | 仓外临时仓，`update-index --assume-unchanged` 一个 `.cx` | 视图解析出 `au=1`，该文件**仍被哈希并审计**，rc 不变 |
| `k-sw-flag` | 同上，`--skip-worktree` | `sw=1`，仍审计，rc 不变 |
| `k-view-cwd` | 从子目录调用，pathspec 相对 cwd | `EMPTY-VIEW` 或按 `cl_root` 调用后非空；**不得**静默 0 条 |
| `k-parse-tag` | 伪造未知 tag（如 `Z`）喂给解析器 | `UNDECIDED GIT-PARSE`、rc3 |
| `k-path-tab` | 仓外临时仓里建含 `TAB`/换行的 `.cx` | 路径原样解析、不误判为 `GIT-PARSE` |

（`k-unmerged`、`k-index-view-change` 保留，但按 §12.3 改为仓外独占仓执行。）

### 12.6 验收（替代 §11.6 第 1 条）

1. `--fixtures`：`checks` = 原值 + 36，`failed=0`；跑完 `git status --porcelain` 证明
   共享 index 未被夹具改动（运行前后各取 `.git/index` 的 size/mtime/ino + sha，必须相同）。
2. `k-au-flag`/`k-sw-flag` 断言「标志被识别」且「文件照常审计」，防止把 au/sw 当成跳过理由。
3. 全量审计 scope 行新增 `au=`/`sw=`/`view_sha=`/`view_bytes=`；`view_bytes==0` 直接 rc3。
4. 其余同 §11.6：收据只写「可观测点上未观测到漂移」；超时 rc3；spec 与三源不动；
   G-3 不裁；`OPEN-LEX-LINE` 未决；补边、shard GC 另案。

## 13. 方案 B 第六次修订：对象 ID 长度绑定 `--show-object-format`、路径只在首个 TAB 切分（未施工）

两处 cdx 指出属实，本节改正 §12.2 的解析规则。实测：`git version 2.34.1`。

### 13.1 对象 ID 长度不是常量

实测对比（两个仓外独占临时仓）：

| 仓 | `git rev-parse --show-object-format` | `git ls-files -s` 第二字段长度 |
|---|---|---|
| `/tmp/objfmt/sha1repo`（`git init`） | `sha1` | **40** |
| `/tmp/objfmt/s256repo`（`git init --object-format=sha256`） | `sha256` | **64** |
| **本检出** | `sha1`（`--show-object-format=storage`/`=input` 同为 `sha1`） | **40** |

**本检出是 SHA-1 仓 ⇒ §12.2 固定 64 位会把 1944 条合法行全部判成 `GIT-PARSE`。** 改正：

1. 先跑 `git rev-parse --show-object-format`（实测本仓 rc0、输出 `sha1`，≈2 ms）。
2. 映射：`sha1` → 40；`sha256` → 64。其它值、空输出、rc≠0（旧 git 无此选项）→
   `UNDECIDED GIT-OBJECT-FORMAT value=<raw>` → rc3。**不得**猜测或回退到尾行长度推断。
3. 每条记录的对象 ID 必须全为小写十六进制且长度等于上表值；不符 → `GIT-PARSE`。
   （40/64 二选一，不做「40 或 64」的宽松匹配——宽松匹配会把截断/拼接的记录放过。）
4. **两个哈希不得混用**：`git ls-files` 给出的是 **Git 对象名（blob id）**，算法由仓格式
   决定；我们自己算的是 **源内容 SHA-256**（§11.2）。scope 行分别用
   `objfmt=sha1|sha256`、`blob_id_len=40|64`、`digest=afalg|soft` 标注，绝不比较两者，
   也不把 blob id 当作「内容一致」的证据。manifest 里的 sha256 同理不能代替 blob id。

### 13.2 `-z` 路径：只在首个 TAB 切分，其余字节原样

实测（`/tmp/gitflags`，仓外独占仓）：

```
b'H 100644 975f...4ac 0\tbs\\path.cx'       ← 反斜杠原样，不转义
b'H 100644 bca7...9ff 0\tt\tb.cx'           ← 路径内含 TAB，原样穿过
b'H 100644 587b...6cb 0\tone.cx'
```

- `-z` 下路径是**原始字节**：TAB、换行、反斜杠都不做 C 风格转义。因此：
  **只在第一个 TAB 处切分元数据段与路径**；元数据段（tag/mode/id/stage）内不含 TAB，
  故切分唯一；**路径里后续的 TAB 是合法路径字节，不能描述为「元数据错误」**。
  §12.2 里「除首个 TAB 外不得再出现 TAB」一句**作废**。
- 唯一仍算 `GIT-PARSE` 的情形：**首个 TAB 之前**的元数据段字段数≠4、tag 非法、mode 非
  6 位十六进制、ID 长度不符（§13.1）、stage 不在 0..3、路径为空（首个 TAB 后立刻 `\0`）。
- 路径字节里不可能有 `\0`（`-z` 以 `\0` 分隔），故工具内的 C 字符串处理对 TAB/换行安全。
- 仍存的两处**表示**问题，必须显式处理而非默默丢行：

| 位置 | 现状（实测/读码） | 方案 |
|---|---|---|
| manifest 读入 | `symindex.putesc` 把路径中 `\t`/`\n`/`\\` 写成两字节转义（实测仓库里 `t<TAB>b.cx` 写成 `t\tb.cx`），而 `check-cx-layers.cx:347` **原样拷贝、无逆变换** ⇒ 去 stat 一个不存在的路径，误判 | 在 `cl_audit` 读 manifest 时实现**逆变换**（仅这 3 个转义，可完全还原）；遇非法转义（如 `\q`、末尾孤立 `\`）→ `UNDECIDED MANIFEST-PARSE path=<escaped>`；`MANIFEST-PARSE` 不丢行、不猜 |
| 工具输出 | `FAIL ... %s` 直接拼接路径（`check-cx-layers.cx:186`），路径含 TAB/换行会破坏下游单行 TSV | 输出路径用**同一套可逆转义**（`\t`/`\n`/`\\`），保证单行且可还原；这不是 UNSUPPORTED-PATH，因为无损 |
| 路径长度 | `path[CL_PATH]`（4400），超长会被静默截断 | 长度 ≥ `CL_PATH-1` → `UNDECIDED PATH-TOO-LONG len=<n>`，**不截断继续**；这是唯一需要 `UNSUPPORTED-PATH` 语义的残余情形 |

- 当前共享仓实测：1944 条 tracked `.cx` 中**非可打印路径 0 条、含反斜杠 0 条**，故这些
  分支目前是**潜伏**而非现症；但不得因此省掉，否则第一个带 TAB 的文件名就会静默漏审。

### 13.3 代价

新增一次 `git rev-parse --show-object-format` ≈2 ms（已并入 §12.4 的 git 5 s 上限）。
manifest 逆变换为单趟字节扫描（≤ `CL_CAP`），输出转义仅作用于命中行，均不计入热点。

### 13.4 夹具（在 §12.5 的 +36 之外，总 +40）

| id | 形状 | 断言 |
|---|---|---|
| `k-objfmt-sha256` | 仓外 `git init --object-format=sha256`，放入一个 `.cx` | `objfmt=sha256`、`blob_id_len=64`，行被接受、边正常 |
| `k-objfmt-bad` | 解析器喂入 40 位 ID 而声明 `sha256`（或反之） | `UNDECIDED GIT-PARSE` 或 `GIT-OBJECT-FORMAT`，rc3 |
| `k-path-tab-real` | 仓外仓里建**真实文件名**含 `TAB`、换行、反斜杠三个 `.cx` | 三条都被审计（边计入）；输出单行且逆变换还原为原名；`au/sw` 不影响 |
| `k-path-too-long` | 路径长度 ≥ `CL_PATH-1` | `UNDECIDED PATH-TOO-LONG`，不截断、不丢行 |

`k-path-tab-real` 必须用真实文件名（`fx_write` 传含 `\t` 的路径），不得用伪造的字节流喂解析器
代替——否则测不到 `git` 与文件系统这一段。

### 13.5 验收（追加到 §12.6）

5. `--fixtures` 总数 **+40**；`k-path-tab-real` 的三条路径在收据里各出现一次（不重不漏）。
6. 全量审计 scope 行含 `objfmt=`、`blob_id_len=`；本仓应为 `objfmt=sha1 blob_id_len=40`。
7. 其余不变：仓外独占夹具、收据只写「可观测点上未观测到漂移」、超时 rc3、
   spec/三源不动、G-3 不裁、`OPEN-LEX-LINE` 未决、补边与 shard GC 另案。
