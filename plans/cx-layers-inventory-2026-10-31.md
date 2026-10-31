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

## 14. 拆成 A/B 两项独立交付：边界、真实调用、可验证状态、维护成本（未施工）

结论：**可以安全拆分**。原因不是妥协，而是两条事实：

1. `cl_audit` 的域**本来就是 manifest 行**（`cl_audit` 只 `cl_read` manifest，按前缀跳过
   `archive/`、`out/`），今天从不读 `git ls-files`；tracked 对账是 B 新增的能力。
2. A 需要的「内容未漂移」只需**自算 sha 与 manifest 行的 sha 比**：二者相等时 shard 的
   `U` 行（内容寻址）必然属于该内容。**这条推理不需要 git**。§12/§13 的全部复杂度
   （object-format、`-s -v`、`-z` 路径字节、仓外仓、ceiling dirs）都只服务「域是哪些文件」，
   而那正是 B 的问题。

### 14.1 边界

| | A（`tools/check-cx-layers.cx`，改现有） | B（`tools/cx-layers-domain.cx`，新文件） |
|---|---|---|
| 问题 | 已审计的每个 `use` 是否被正确词法定位，manifest 的 `U` 行与源码是否双向对账 | 域到底是什么：tracked 入口、软链、manifest 是否覆盖 tracked |
| 输入 | manifest.tsv + `<sha>.tsv` + 源文件本身 | `git ls-files -s -v -z`、`git rev-parse`、manifest 路径集合 |
| 是否读 git | **否** | 是（全部 git 逻辑只在这里） |
| 是否读源 | 是（词法 + sha） | 否（只 `lstat` 判断存在性） |
| 输出 | `CX_LAYERS` / `CX_LAYERS_SCOPE` / `FAIL` / `UNDECIDED` | `CX_LAYERS_DOMAIN` / `FAIL` / `UNDECIDED` |
| rc | 0 / 1 / 3 | 0 / 1 / 3（同一三档语义，见 14.4） |
| 不得声称 | 全仓覆盖、软链覆盖 | 任何层方向结论（它不看 `use`） |
| 现有断言 | **全部保留**：编号向上 FAIL、白名单双向、`CLASS-OVERLAP`/`CLASS-SUM`、`PARSE-CONDITIONAL`、`SHARD`、`SYMINDEX` | 无既有断言，全新 |

A 修的是**判定正确性**（§7 的 12 条词法 + §8/§9 的双向对账 + §11 的摘要绑定）；
B 新增的是**域完整性**（§10 的 tracked/manifest 等式、§12 的 flag 与 cwd、§13 的 object-format）。

### 14.2 A 的真实调用与可验证状态

调用不变：

```
sh ./unisacc.com tools/check-cx-layers.cx -- [PRODUCT]
sh ./unisacc.com tools/check-cx-layers.cx -- --fixtures
```

内部仍是：spawn `symindex update` → `cl_audit`。不新增 git 调用，故 §12.4 的 50 s×4 隐患
在 A 上**不存在**；A 的 spawn 仍只有 symindex 一次。

可验证状态（每个都能被一条断言或一条收据字段钉住）：

| 状态 | 证据 |
|---|---|
| 词法定位与编译器同源 | §7.5 的 12 条夹具，逐条断言 `cl_fail`/`cl_edges` 与诊断行号 |
| `U` 行双向对账 | §9.6 的 7 条（缺 U / 多 U / 注释内 / 同行多模块 / 别名 / 续行） |
| 摘要绑定被扫描字节 | `k-digest-buffer`：期望值来自外部 `sha256sum` |
| 漂移可观测性 | `k-aba`：rc0 且收据只写「未观测到漂移」；`k-source-changed`：rc3 |
| shard 缺失 | `k-missing-shard` → 非零；`k-orphan-shard` → rc 不变 |
| 路径表示无损 | `k-manifest-escape`（真实文件名含 TAB）+ `k-path-too-long` |
| 未决即非成功 | 任一 `UNDECIDED` → rc3；`run-batch` 只有 rc0 才 PASS |

A 的夹具 = §7.5(12) + §8.6(5) + §9.6(7) + `k-orphan-shard` + `k-missing-shard` +
`k-digest-buffer` + `k-aba` + `k-manifest-escape` + `k-path-too-long` = **30 条**。

`k-manifest-escape` 用真实文件名（fixture 树里 `fx_write("t\tb.cx", …)` 后 `fx_dump` 点名），
这样测到的是 `symindex` 写转义 + A 读转义的正反两段，不是伪造字节流。

scope 行改为：

```
CX_LAYERS_SCOPE coverage=manifest-only symlinks=UNKNOWN tracked_reconciliation=NOT-DONE(B)
                 domain=<n> bytes=<n> digest=afalg|soft window=STABLE|MOVED
                 au_sw_relevance=none
```

**`coverage=manifest-only` 与 `symlinks=UNKNOWN` 是常量，不是计数**：A 没有 git 就不可能
知道软链数量，写数字就是编造。B 交付后由 B 的收据提供真值，A 的这两项才可改写。

### 14.3 B 的真实调用与可验证状态

调用：

```
sh ./unisacc.com tools/cx-layers-domain.cx -- [ROOT]
sh ./unisacc.com tools/cx-layers-domain.cx -- --fixtures
```

内部：`git rev-parse --show-object-format` → `git ls-files -s -v -z -- '*.cx'`（cwd=ROOT，
5 s 上限）→ 解析 → 与 manifest 路径集合做集合运算 → 打收据。**不读源、不读 shard、不看 `use`。**

可验证状态：

| 状态 | 证据 |
|---|---|
| 三分区 `T = R ⊎ S` | mode 120000 与其余分离，逐个列出 |
| 软链解析 | 每个软链给 `-> target` 与 `resolved|broken`；broken 逐个列出（今天 11 个，全在 `archive/`） |
| manifest 覆盖 tracked | `R∖M` 逐个列 `NOT-INDEXED`；`M∖R` 仅允许 `out/` 行，否则 `DOMAIN-MISMATCH` |
| 域外 | `archive/`、`out/` 计入 `excluded=`，不参与等式 |
| 解析健壮性 | `k-parse-tag`、`k-path-tab`、`k-objfmt-sha256`、`k-objfmt-bad`、`k-view-cwd` |
| 不误报 | `k-au-flag`、`k-sw-flag`（标志识别但仍计入域）、`k-orphan-shard` 类不适用 |
| 不写共享检出 | 夹具前后共享 `.git/index` 的 size/mtime/ino+sha 必须相同 |
| 身份 | scope 打 `objfmt=`、`blob_id_len=`、`view_sha=`、`view_bytes=`、`index_sha=` |

B 的夹具 = `k-not-indexed`、`k-unmerged`、`k-index-view-change`、`k-au-flag`、`k-sw-flag`、
`k-view-cwd`、`k-parse-tag`、`k-path-tab`、`k-objfmt-sha256`、`k-objfmt-bad` = **10 条**。
30 + 10 = 40，与 §13.4 的总数一致，没有一条被丢掉。

### 14.4 共享合同（不可拆）

- rc 三档：0 全清 / 1 `failed>0` / 3 `undecided>0` 或分区破裂。`tools/cxlib/allsession.cx:292`
  对其它值判 UNKNOWN，故两工具必须同语义，否则下游把 rc3 读成 UNKNOWN。
- 收据措辞：只许「在可观测点 `{首读哈希, 末次重读哈希}` 上未观测到漂移」。B 不读源，
  不得借用这句；B 只声明「在视图 `[首次 ls-files, 末次 ls-files]` 上未观测到域变化」。
- 既有 `UNKNOWN amb/out not-a-pass` 行**保留且不改语义**：AMB/OUT 是无 spec 编号的既定
  状态，不是新未决。新的未决（`UNDECIDED *`）一律 rc3。两者不得混用同一个词。

### 14.5 不能安全拆分的反例：无

逐条查过，没有「B 未交付则 A 无法判定」的情形，也没有「A 改坏 B」的共享状态：

- A 判 `use` 方向，不依赖域是否完整；域不全只会让 A 少审计几个文件，而这**正是今天的
  现状**，不是 A 引入的退化。
- A 的 `U` 行对账不依赖 tracked：manifest 有行就有 sha，有 sha 就有 shard。
- B 不读 shard、不读源，A 的改写不会改变 B 的输入；B 只依赖 manifest 的**路径列**。
- 唯一耦合是 manifest 的**转义规则**：A 需要逆变换，B 也需要（比较路径集合）。因此把
  转义/逆变换的函数与夹具放在 A（先交付），B 复用它——这是单向依赖，可接受；若 B 先生，
  则 A 必须等，故顺序固定为 **A → B**。

### 14.6 风险与缓解（必须写进交付说明）

1. **A 的 rc0 不等于全仓覆盖。** 今天就有实证：`tests/runner-faults.cx` 存在且 6680 B，
   但当时的 manifest 没有它（`R∖M` 恰 1 条），A 对此**完全无感**。缓解：把批次行的
   marker 从 `CX_LAYERS` 改成 `CX_LAYERS_SCOPE`，使「没有 scope 行」本身不能通过门禁；
   `coverage=manifest-only` 与 `symlinks=UNKNOWN` 为常量，A 不能靠沉默过关。
2. **A 与 `symindex update` 代际错位会假失败而非假通过**：manifest 若是上一代（`DEFERRED`
   保留旧行），A 自算 sha 与旧 sha 不等 → 该行 `UNDECIDED SOURCE-CHANGED` → rc3。
   这是安全的失败方向，但会让重活窗口后的第一批变红，需在批次说明里写明「先让
   `symindex update` 完整跑完」。
3. **软链覆盖在 B 交付前恒为 UNKNOWN**：manifest 不含 mode 120000 条目，A 无从发现，
   故 A 只能打印常量 UNKNOWN；`prd.md` 能力速查同时记这一行，避免后来者把 A 当成全仓。

### 14.7 维护成本

| | A | B |
|---|---|---|
| 代码量 | 改现有 700 行中的约 250 行（`cl_locate`/`cl_process_use`/新增 `cl_lex_uses`/`cl_dig`/scope） | 新文件约 200 行（git spawn/解析/集合运算/收据），无词法 |
| 新静态内存 | `cl_dig` 261 KiB + 现有三缓冲（`cl_src`/`cl_shard`/`cl_man`） | 仅视图缓冲（当前 155 KiB）+ 结果映射，**可复用 `cl_man` 大小上限** |
| 新增 `#pragma unisa use` | 1 条 `sys.sha`（第 8 条，上限 48） | 2–3 条（`sys.fs`/`sys.io`/`sys.proc`），需 `cxlib.netpack` 跑 git |
| 夹具数 | 30 | 10 |
| 新增批次 | 无（沿用第 332 行，仅改 marker） | 1 行 `cx-layers-domain`，用 `tools/register.cx` 登记 |
| 失败面 | 只扩不缩：既有 FAIL 类全留 | 全新，不影响 A |
| 回归风险 | 中（改动集中在定位与对账，夹具可覆盖） | 低（独立文件，A 的收据不变） |
| 依赖顺序 | 先 | 后（复用 A 的转义函数） |

### 14.8 交付顺序与门禁

1. A：改 `tools/check-cx-layers.cx` → `--fixtures` 30 条全绿 → 全量审计与上一次同输入
   收据逐项比对 → `tools/affected.cx --files tools/check-cx-layers.cx --run` →
   `tools/ratchets.cx` → `tools/commit-own.cx`（批次行只改自己的那一行）。
2. B：新文件 + `--fixtures` 10 条 → `tools/register.cx` 登记 `cx-layers-domain` 批次 →
   `tools/affected.cx` → `ratchets` → 提交。
3. B 落地前，A 的 scope 行与 `prd.md` 能力速查同时标 `symlinks=UNKNOWN`；
   B 落地后把该常量替换为 B 收据的真值，并复查 A 的 `coverage=` 是否可升级。

## 15. A 刀移交 omp-go：所有权、marker 改名消费者、夹具边界、记账口径（未施工）

### 15.1 移交决定：是，且附条件

**是。** 主人裁定代码由 omp-go 实施，A 刀（`tools/check-cx-layers.cx` 的修改）的**写入所有权
临时移交 omp-go**；本刀退出写入，只保留只读审查。条件：

1. **未移交前 omp-go 不碰该文件。** 本封只是方案移交，不是移交本身；移交生效以 cdx 批准 +
   本刀打下基线（下条）为准。
2. **基线由本刀在移交时点记录**：`git rev-parse HEAD` + `tools/check-cx-layers.cx` 的 sha256
   （本封写作时 HEAD `32744f7421`，工具当前 rc0、`edges=2413`，见 15.5 实测）。移交后任何
   改动都相对这个基线审查。
3. **审查通道**：本刀只读 review，意见走 `1:omp-cdx` 信封或 `write agent://…`，不直接改文件、
   不 `git add` 该路径、不替 omp-go 提交。
4. **范围锁**：A 刀只改 `tools/check-cx-layers.cx`（+ 它自己的 `--fixtures` 用例）。不得顺手
   改 `tools/symindex.cx`、`cx/*`、`tests/run-batch.cx`、spec；索引漏续行 use 属另案。
5. **回退权**：若 A 刀改动引入既有 FAIL 类减少、覆盖缩减或断言放宽，本刀有权要求回退到基线，
   冲突由 cdx 裁。
6. **B 不随本次授权**：`tools/cx-layers-domain.cx` 未获实施授权，B 侧一行都不写。

### 15.2 marker 改名的全部直接消费者（实测 grep，非推测）

全仓 `grep -rn CX_LAYERS`（排除 `out/`）命中 **3 个文件**：本文档、`tests/batches.tsv`、
`tools/check-cx-layers.cx` 自身。逐个核：

| 消费者 | 现状 | 改名影响 | 处理 |
|---|---|---|---|
| `tests/batches.tsv:332` 第 12 列 | `rc0:CX_LAYERS` | 不改则 A 的 scope 行不被认，改后仍严格要求 scope 行存在 | 改 `rc0:CX_LAYERS_SCOPE` |
| `tests/run-batch.cx:23` `marker()` | 整行**前缀**匹配且其后必须为 `TAB` 或行尾 | `want=CX_LAYERS` 时 `CX_LAYERS_SCOPE` 行在 `p[at+10]=='_'` 处失败 ⇒ **不会双匹配**；反之亦然 | 不改代码，语义已严 |
| `tests/run-batch.cx:36` PASS 条件 | `!rc && candidate 未变 && marker(...)==1` | 要求 scope 行**恰好一行** | A 必须在正常返回路径打印且仅打印一次 |
| `tests/run-batch.cx:39` | `ok?PASS:timed_out?UNKNOWN:FAIL` | 不变 | — |
| `tools/cxlib/allsession.cx:292` `as_verdict` | 只看 rc（0/1/3），不看 marker | 不变；rc 三档已定 | — |
| `tests/coverage.tsv:312` | 描述文本，`IMPLEMENTED_SEPARATE_BATCH` | 不改也能过；但描述应与新语义一致 | 追加「收据含 `CX_LAYERS_SCOPE`，`coverage=manifest-only`」一句，字段不变 |
| `prd.md:32` 能力速查 | 描述工具用途 | 需补 scope/coverage 与软链 UNKNOWN | 追加「A 另印 `CX_LAYERS_SCOPE`，声明 `coverage=manifest-only symlinks=UNKNOWN`；软链与 tracked 对账属 B」 |
| `tests/run.cx` smoke 通道 | `smoke_row_valid` 只认 `id==smoke` / `product.*` | **不受影响**（cx-layers 不在 smoke 名单） | 无需改 |
| `tools/affected.cx` | 按 `source`/`entry` 字段做影响面 | 字段不改，仅 marker 变 | 无需改 |

**验收证据随之变化**：PASS 从「rc0 + `CX_LAYERS` 出现一次」变成「rc0 + `CX_LAYERS_SCOPE` 出现
一次」。因为没有 scope 行就必然 FAIL，**沉默不再能通过**。

### 15.3 keyed 行归属

`tests/batches.tsv`、`tests/coverage.tsv`、`prd.md` 是多人同改文件，一律
`tools/commit-own.cx --keyed`（实现见 `tools/commit-own.cx:3-11`：私有索引 + 只带含 key 的
changed 行）：

| 文件 | key | 只带的行 |
|---|---|---|
| `tests/batches.tsv` | `cx-layers` | 第 332 行 |
| `tests/coverage.tsv` | `contract.cx-layers` | 第 312 行 |
| `prd.md` | `能力速查`（该行含工具路径 `tools/check-cx-layers.cx`，用它做 key） | 第 32 行 |

`tools/check-cx-layers.cx` 用 `--whole`（整文件属 A 刀）。不得用 `git add` 整文件提交共享
tsv；不得 `stash/clean/reset/checkout` 他人路径。

### 15.4 既有夹具边界

- **现有 `--fixtures` 的 16 条 checker 全部保留**，期望值与诊断行号一字不改；A–H 用例一个不删。
  实测当前输出 `CX_LAYERS_FIXTURES checks=16 failed=0`、耗时 4.4 s。
- 新增 30 条（§14.2），总数变 **46**。按当前 0.27 s/check 估算约 12–13 s；`--fixtures` 不在
  批次 55 s 预算内，但仍须单次 ≤60 s 命令上限内跑完，否则拆成两组入口（`--fixtures lex`
  / `--fixtures recon`），**不得**靠减少用例省时间。
- 旧夹具的 `FAIL` 行（`UNRES-ENOTDIR`、`UNRES-EACCES`、`PARSE-CONDITIONAL`）是**预期输出**，
  不是失败；`failed=0` 才算过。改名后这些类名不变。
- A 的夹具树 `out/cx-layers-fixtures-<pid>/` 在共享检出内但**不在 `out/` 之外写盘**；A 的夹具
  **不得**执行任何写型 git 子命令（现在只 spawn `symindex`）。新增一条免费自检：`--fixtures`
  前后共享 `.git/index` 的 size/mtime/ino + sha256 必须相同（B 的仓外仓要求同源条款）。

### 15.5 记账口径：sha 相等不是原子证明，三类未决分列

- **「源 sha == manifest sha」只说明**：在可观测点上，本次读到的字节与写 manifest 时被哈希的
  字节相同。它**不**证明窗口内未发生 A→B→A，**不**证明 shard 未损坏，**不**证明 git 索引
  视图一致。措辞沿用 §11.3/§14.4：只许「在可观测点 `{首读哈希, 末次重读哈希}` 上未观测到漂移」。
- **shard 完整性不在 A 的范围**：只断言「被引用 sha 的 shard 存在」（`MISSING-SHARD`）。文件
  内容与文件名是否一致**未校验**；本版不承诺，scope 记 `shard_body_verified=0`。要加校验（重算
  shard 的 sha256 并与文件名比）须另行提案。
- **三类独立记账，不许合并成一个数**，scope 行分列：

```
undecided_lex=<n>     词法未决：PARSE-CONDITIONAL、未知指令形状、行号未决(OPEN-LEX-LINE)
undecided_recon=<n>   对账未决：MISSING-U（源有 use、索引无 U）、EXTRA-U（索引有 U、源无该 use）
undecided_meta=<n>    元数据未决：MANIFEST-PARSE、PATH-TOO-LONG、MANIFEST-CHANGED
undecided_io=<n>      I/O 未决：UNREADABLE、SOURCE-CHANGED、SOURCE-CHANGED-AFTER-READ、空读
```
  `MISSING-U` 与 `EXTRA-U` **各自计数、不互相抵消**：一方多、一方少不许写成「净差 0」。
  四个分项之和必须等于 `undecided` 总数（`UNDECIDED-SUM` 自检，不等即 rc3）。
- `MISSING-U` 仍**不补边**（补边会改 `edges` 冒充覆盖提升），只记 rc3。

### 15.6 A 刀实测基线（供 omp-go 与验收比对）

本机、当前共享树、`HEAD 32744f7421`（工作区含他人改动）：

| 指标 | 值 |
|---|---|
| 全量审计 rc | 0 |
| 全量审计耗时 | **10,442 ms** |
| `CX_LAYERS` 行 | `edges=2413 same=771 down=1510 up=0 amb=82 out=50 unres=5 io=0 failed=0` |
| `UNKNOWN` 行 | `amb=82 out=50 not-a-pass` |
| `--fixtures` | `checks=16 failed=0`，**4,400 ms** |
| 工具基线 sha256 | `93619a0a4109e946b9bd95707fc485be187ccbbf8030f53ca5b54ab78fe67180` |

A 施工后的判据：`up==0` 与 `failed==0` 必须保持；`edges` 允许变化但**每次变化必须给出原因**
（改词法定位会修正误报/漏报，`same/down/up/amb/out` 五项之和必须仍等于 `edges`）；
审计耗时允许 +3–5 s（两趟哈希）；`undecided_*` 四项须为 0 或逐条解释。

### 15.7 交付清单（A 刀）

1. 改 `tools/check-cx-layers.cx`：`cl_locate`/`cl_process_use` 按 §7 的 12 条词法重写、
   新增 `cl_lex_uses`、`cl_dig`（+261 KiB）、双向对账、scope 行、四个 `undecided_*`、
   `UNDECIDED-SUM`、rc 三档、manifest 逆变换、输出转义、`PATH-TOO-LONG`。
2. `--fixtures` 46 条全绿（16 旧 + 30 新），并记录 `.git/index` 前后一致。
3. 全量审计与 §15.6 逐项比对，差异给原因。
4. `tools/affected.cx --files tools/check-cx-layers.cx --run`；`tools/ratchets.cx`。
5. 提交：`--whole tools/check-cx-layers.cx` + 三个 keyed 行（15.3）；批次 marker 一并改，
   **同一提交内**完成，避免出现「marker 已改、工具未改」的中间态把共享 main 变红。

## 16. A 刀移交生效与最终验收合同（cdx 批准，未施工）

cdx 已批准 §15 的临时移交并通知 omp-go。本节是**生效后的唯一验收合同**，与 §15 冲突处以本节为准。

### 16.1 生效与边界

- 生效：本刀对 `tools/check-cx-layers.cx` **只读复核 + 冲突升级**；不改该文件、不 `git add`、
  不代提交。基线见 §15.6（HEAD `32744f7421`，工具 sha256 `93619a0a…e67180`）。
- **B 未授权**：`tools/cx-layers-domain.cx` 一行不写，软链覆盖维持 `UNKNOWN`。
- 冲突升级路径：本刀只读意见 → `1:omp-cdx` 信封；不得以「修一行就行」绕过移交。

### 16.2 `checks` 数是收据，不是断言（已核实现行代码）

读码确认 `fx_checks` 在 `tools/check-cx-layers.cx` 中**只自增、只打印**（`:396` 声明、`:400`
自增、`:609` 打印），**没有任何 `==`/`<`/`>` 断言**；`--fixtures` 的 rc 只由 `fx_fail`
决定（`cl_fixtures` 返回 `fx_fail ? 1 : 0`，`main` 直传）。因此：

- 新增用例**必须**靠各自的具名断言（`fx_note(cond, "name")`）失败，**不得**加
  「`checks == 46`」之类计数断言——那会把「加用例」变成「改断言」。
- 收据里仍打 `checks=<实际>`，作为本次运行的观测值；数量变化不构成失败。
- A 刀**不得**反向删除任何既有具名断言；16 条 checker 的语义与期望值一字不改。

### 16.3 具名断言清单：原有 16 条（语义不变）

`--fixtures` 现有 16 条 checker 全部保留。按当前输出，被检的具名项至少含：

| 类别 | 具名项（现状） |
|---|---|
| 正例/向上 | 合法分层、编号层向上必拒（文件:行 + 目标 realpath） |
| 未解析 | `UNRES-ENOTDIR`、`UNRES-EACCES`、精确白名单行、多余未解析 |
| 词法 | 注释内 `use`、`#if 0` 内 `use`（`PARSE-CONDITIONAL`）、别名 `b::b_x as ax` |
| 编译器交叉 | `-cx-plan` 直接计数比对、`h-compiler-refuses-conditional` |

（具体 16 个 `fx_note` 名字以工具正文为准；本节只锁「数量不减、语义不改、名字不换」。）
新增 30 条见 §14.2，逐条具名，预期总数 46 —— **这是收据预期，不是断言**。

### 16.4 scope 行必含三项显式未知

```
CX_LAYERS_SCOPE coverage=manifest-only symlinks=UNKNOWN shard_body_verified=0
                 domain=<n> bytes=<n> digest=afalg|soft window=STABLE|MOVED
                 undecided_lex=<n> undecided_recon=<n> undecided_meta=<n> undecided_io=<n>
```

三项为**常量**，不得写成计数：A 无 git 不知软链真值（现 42 条 tracked 软链、11 条 broken，
全部在 `archive/` 之外不被 manifest 收录）；A 不校验 shard 内容与文件名一致。

### 16.5 rc 语义（最终）

| 情形 | rc |
|---|---|
| 无 `failed`、无 `undecided` | **0** |
| `failed > 0`（编号层向上、白名单失配、`CLASS-OVERLAP`/`CLASS-SUM`、`SHARD` 缺失、`SYMINDEX` 失败、`UNDECIDED-SUM` 自检不等） | **1** |
| `failed == 0` 且仅 `undecided > 0` | **3** |
| 两者同时 | **1**（失败优先） |

`tools/cxlib/allsession.cx:292` 对 0/1/3 之外的 rc 判 UNKNOWN，故不得引入第 4 个值。
`AMB/OUT` 的 `UNKNOWN … not-a-pass` 行**保持原状**，不参与上述计数。

### 16.6 提交纪律

- 工具本体 + `tests/batches.tsv` 的 marker 行 + `tests/coverage.tsv`/`prd.md` 的 keyed 行
  **必须在同一次精确提交内**；允许分批的只有 `plans/` 文档。
- 提交用 `tools/commit-own.cx`：`--whole tools/check-cx-layers.cx` +
  `--keyed tests/batches.tsv cx-layers` + `--keyed tests/coverage.tsv contract.cx-layers`
  + `--keyed prd.md tools/check-cx-layers.cx`。
- 提交前 `tools/own-diff.cx` 自查，确认工作区里他人路径不被裹入。

### 16.7 外来改动与重测试窗口

- **`cx/sys/fs.cx` 是他人在途改动**，会连带影响已装编译器（omp-go 报
  `tests/sys/fs.cx:44 not covered: member access`）。A 刀验收若该失败仍在，**不得**据此判定
  A 回归，也不得重跑它来「确认」；记录为外来阻塞，A 的证据只用 `--fixtures` + 全量审计 +
  `affected`/`ratchets` 中与该文件无关的部分。
- 全量审计是重活：单机同时只跑一组，且与 `symindex update` 的代际耦合见 §14.6-2
  （先让 `update` 完整跑完，否则第一批会因 `SOURCE-CHANGED` 变红——安全的假失败）。
- 命令上限 60 s：全量审计实测 10.4 s、`--fixtures` 4.4 s；新增 30 条后预计 `--fixtures`
  12–13 s，仍在限内；若逼近 60 s，拆 `--fixtures lex` / `--fixtures recon` 两个入口，
  **不得**删用例或放宽断言。

## 17. `8361c3cf6b` 只读审查（局部，不记 A 完成）

对象：`8361c3cf6b`（`check-cx-layers: A scope marker, digest binding, rc3 skeleton`，4 文件 +25/−7）。
方式：静态读码 + 与 §15.6 基线比对；**未重跑**（go 已报 rc0/16 通过，按「他人观测即事实」不重复确认）。
结论：**局部绿，A 未完成**（与 go 自述一致）。新逻辑**零夹具覆盖**，且 SHA 未绑定被扫描的字节。

### 17.1 逐项回答 cdx 四问

| 问 | 答 | 证据 |
|---|---|---|
| 16 条断言是否保留 | **保留**；`cl_fixtures` 区一字未改，`fx_note` 调用数 17→17，`checks=16` 不变 | `git diff 8361^ 8361 -- tools/check-cx-layers.cx` 26 行，全在 `cl_audit`/静态量/`main` 的 printf |
| 四类 undecided 分区 / rc 优先级 | 优先级**正确**（`:396` fail 优先 `return 1`，`:397` 纯 undecided `return 3`）；分区**骨架正确但两个分区空转**，`UNDECIDED-SUM` 是**恒真死检查** | 见 17.2 |
| SHA 是否绑定实际扫描字节 / window 措辞 | **否**：digest 绑定「读取 #1」，词法定位用「读取 #2」，同一文件被重读 N 次；无末次重读、无 `MANIFEST-CHANGED`；`window=` 是**常量字符串**不是观测 | 见 17.3 |
| marker / keyed 协议 | 四文件**同一次提交**，各自只改自己的行；`marker()` 前缀+TAB 匹配不会双匹配；`CX_LAYERS` 行整体消失，仓内无其它消费者 | 见 17.4 |

### 17.2 事实表（按严重度）

| # | 事实（含行号） | 影响 | 处置 |
|---|---|---|---|
| 1 | `cl_und_total` 只在 `cl_und_meta`/`cl_und_io` 自增的 3 处同步自增（`:369`、`:373`、`:375`），`cl_und_lex`/`cl_und_recon` **全文无自增** | `:392` 的等式恒成立 ⇒ `UNDECIDED-SUM` 永不触发，是**死断言**；且 `undecided_lex=0 undecided_recon=0` 是**假清白**（词法与 U 对账根本没实现） | 未实现的分区打 `NA`（或 scope 加 `phase=partial`），别打 0；`cl_und_total` 应=四个分项之和由**独立来源**校验，不能由同源自增 |
| 2 | digest 读 `cl_src`（`:372`）→ `cl_eq` 比较（`:374`）；随后 `cl_shard_file` → `cl_locate` **再 `cl_read(srcpath, cl_src, CL_CAP)`**（`:116`），词法扫描用的正是这次读到的字节 | digest 与**被扫描的字节**不是同一份；每个 `U` 行触发一次整文件重读，一个文件内可能读到 N 种内容 | 同 §11.2：一次读 → `cl_clean` → 取 digest → 在**同一缓冲**上扫描；或扫描前重算 digest 并要求相等 |
| 3 | 无末次重读、无 `cl_dig` 存储、无 `MANIFEST-CHANGED`；`window=OBSERVED-NOT-ATOMIC` 是字面常量（`:657`） | 无法区分「观测到稳定」与「根本没观测」；§11.3 的措辞因此没有证据支撑（但也没有虚假宣称原子） | 加末次重读与 manifest 重哈希，`window=STABLE\|MOVED` 由观测导出；常量只可作为未实现期占位，且须同时打 `phase=partial` |
| 4 | `sy_sha256_hex(cl_src,sn,hex) != 0 \|\| !cl_eq(hex,sha)`（`:374`）把**哈希失败**并入 `SOURCE-CHANGED` | 无 AF_ALG/无 `CONFIG_CRYPTO_USER_API_HASH` 的宿主上**全树**判 SOURCE-CHANGED ⇒ rc3；两个不同原因混记，与「独立记账」冲突 | 哈希失败单独计数（`undecided_meta`）+ 软回退 `cx_sha256_hex` + scope 打 `digest=afalg\|soft` |
| 5 | `sn <= 0` 只覆盖读失败/空文件（`:373`）；`sn == CL_CAP-1` 截断无分支 | 1 MiB−1 字节以上文件被哈希**截断字节** ⇒ 误判 SOURCE-CHANGED（类别错，不会假绿）；0 字节文件被判 SOURCE-UNREADABLE（当前 tracked `.cx` 中 0 字节文件 = **0**，未触发） | 截断 → `PATH-TOO-LONG`/`SOURCE-UNREADABLE` 明确记 undecided；空文件按合法输入处理 |
| 6 | `cl_locate` 内 `n <= 0` 直接 `return`（`:117`），`line=0`、`cond=0`，调用方照常 `cl_edge` | 窗口内被删除/不可读的源 ⇒ 边以 `line=0` 计入且 `rc0`；这是双读引入的**新静默洞** | 第二次读失败必须记 undecided 并抑制该行 |
| 7 | `k != 64` 直接 `continue`（`:369`） | 丢行（不调用 `cl_shard_file`）；有 undecided 兜底不会假绿，但与「不丢行不猜」不符 | 记 undecided 后仍走完行处理，或在 scope 记 `rows_dropped=` |
| 8 | scope 缺 `digest=`、`domain=`、`bytes=` 三个 §16.4 约定字段 | 收据无法表达哈希后端与域规模 | 补齐 |
| 9 | 新增逻辑 **零夹具**：无任何 `fx_note` 引用 `cl_und_*`，无任何夹具断言 `rc==3`，也无 `MANIFEST-PARSE`/`SOURCE-CHANGED` 用例 | digest 绑定、rc3、自检、未决分区全部只有「真实树上观察到 0」这一种证据 | 这些必须在 30 条新夹具中逐条具名，且 rc3 要有一条真实触发 |

### 17.3 基线可比性

`8361` 报表 `edges=2402 same=770 down=1500 up=0 amb=82 out=50 unres=5` 来自**隔离 archive + fresh index**，
与 §15.6 的共享树 `edges=2413 same=771 down=1510` 不是同一输入；`down` 差 10、`same` 差 1
**未解释**。要求：用同一输入（同一工作区、同一索引）跑 A 前后两次对比，差值逐项给原因；
`same+down+up+amb+out==edges` 与 `up==0`、`failed==0` 仍为硬判据。

### 17.4 marker / keyed 协议核查（通过）

- 同一次提交内改：`prd.md:32` 能力速查、`tests/batches.tsv:332` 第 12 列 → `rc0:CX_LAYERS_SCOPE`、
  `tests/coverage.tsv:312` 描述、工具本体。四文件各自只动自己的行（`git show` 逐行核）。
- `tests/run-batch.cx:23 marker()` 整行前缀 + 其后须 TAB/行尾 ⇒ `CX_LAYERS` 与 `CX_LAYERS_SCOPE`
  互不误匹配；`:36` 要求恰一行 ⇒ 工具确实只在 `main` 打印一次（`:657`），且 `cl_wl_load` 失败先 `return 1`
  （`FAIL WHITELIST` 路径不打印 scope ⇒ 批次 FAIL，方向正确）。
- `CX_LAYERS` 行被整体删除：全仓（排除 `out/`）无其它按该行解析的消费者；`tests/run.cx` smoke
  只认 `smoke`/`product.*`；`tools/affected.cx` 读 `source`/`entry` 字段。**无破坏**。
- 一个下游语义提醒：纯 undecided 时 `rc3`，而 `tests/run-batch.cx:39` 把非 0 rc 记 `FAIL`（不是 UNKNOWN）。
  这是合同要的「非成功」，但读日志的人不能把该 FAIL 当编译/断言失败。

### 17.5 对后续的妨碍（必须继续 A 前先修 #2 #1）

- #2 一旦 30 条词法夹具落地，`cl_locate` 会整段重写；**现在就把「读一次、哈希同一缓冲、在同缓冲扫描」定下来**，
  否则词法实现会把双读固化成结构。
- #1 的空转分区若带着 `0` 进入最终收据，后续实现者补计数时容易忘记独立的 SUM 来源，形成永久死检查。
- #4 会把「哈希后端不可用」伪装成「源码全被改过」，在跨平台/新内核上产生整树 rc3，污染所有未决统计。
- 建议顺序：#2（单读单缓冲）→ #1（NA + 独立 SUM）→ #4（回退 + `digest=`）→ #5/#6/#7（类别与静默洞）
  → #8（scope 补齐）→ #9（夹具）→ 词法/对账/转义/末次重读。

### 17.6 复查补证（哈希后端与截断风险，实测）

用本机空跑一个小程序（`/tmp/shaprobe/shaq.cx`，仓外，模块经软链指向 `cx/sys`、`cx/text`）：

```
sy_sha256_hex("abc") = ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad  rc=0
cx_sha256_hex("abc") = 同上                                                              equal=1
```

结论与对 §17.2 的修正：

- **软回退确实存在且无需新增依赖**：`cx_sha256_hex(const void *, size_t, char[65])` 是
  **隐式 prelude 的 static**（`include/cx-flat.h:7582` 声明、`:7802` 定义 → `unisa_sha256_hex`），
  任何 `.cx` 不写 `#pragma unisa use` 就能调；实测与 AF_ALG 路径逐字节相同（FIPS 向量相符）。
  因此 §17.2 #4 的处理是**一行**：
  `if (sy_sha256_hex(...) != 0) cx_sha256_hex(...);` 并可据此打 `digest=afalg|soft`。
- **`cx/sys/sha.cx:8` 的注释点名 `cx_sha256_hex` 是准确的**（早先怀疑其不存在是错误的）：
  软实现由 prelude 提供，`cx/crypto/sha256.cx` 的 `cs_sha256_hex` 是另一条等价实现，二选一即可。
- **`sy_sha256_hex` 在非 Linux x86_64 目标上恒返回 1**（`sy_sha256_af` 的 `#else` 分支直接
  `return 1`），所以缺回退时**整树**判 `SOURCE-CHANGED` ⇒ rc3；本批次声明 `lnx/x86_64`，CI 上
  不触发，但任何跨平台运行都会全红。回退是跨平台前的**前置条件**，不只是健壮性。
- **截断风险当前为 0**：13,432,673 B / 1613 个 tracked `.cx` 中最大为 `cx/runtime/lopm.cx` 与
  `cx/compiler/lopm.cx` 各 **587,351 B**，远低于 `CL_CAP-1 = 1,048,575`；0 字节 tracked `.cx` 数 **0**。
  故 §17.2 #5 属**将来**风险（文件长到 1 MiB 才触发），仍应显式分支而非依赖概率。

清理：`rm -rf /tmp/shaprobe /tmp/shaq.cx`。

### 17.7 CX-Y-4 与 `sy_sha256_hex`：静态冲突确认（A 不改 L0/spec）

cdx 指出的冲突**成立**，逐项核过：

| 项 | 事实 |
|---|---|
| spec 条款 | `cx/spec.md:184`（CX-Y-4）：「L0 失败必须返回 `-errno`（或已文档化的负码）；空指针／空路径／负长度应当 `-EINVAL`；失败不得留下半开 fd」 |
| 实现 | `cx/sys/sha.cx:51` `int sy_sha256_hex(...) { return sy_sha256_af(...); }`；`sy_sha256_af`（`:14`）在 socket/bind/accept/短写/短读/参数非法处一律 `return 1;`（共 7 处 `return 1;`，**0 处** `return -E*`） |
| 文档化？ | 模块头（`:5-8`）写的是「Any failure returns non-zero and leaves hex empty」。这是**非零**，既不是 `-errno`，也不是文档化的**负码** ⇒ 与 CX-Y-4 字面冲突 |
| 是否孤例 | **是**。`cx/sys/` 全部 21 个模块中，只有 `sha.cx` 是 `-E*`=0、`return 1;`=7；其余（`fs.cx` 38、`proc.cx` 15、`posix.cx` 14、`sig.cx` 11、`net.cx` 9、`mem.cx` 7、`time.cx` 7 …）都用负 errno |
| 半开 fd | **当前无**：bind 失败关 sfd、accept 失败关 sfd、短写关两个、短读在两个 `close` 之后才判 `got != 32` ⇒ 失败路径不泄漏 fd（CX-Y-4 这一条已满足） |

ABI 面（只做静态事实，判定归 go）：

| 项 | 事实 |
|---|---|
| 是否在裁剪 libc 符号表 | **否**：`kernel/unisa_headers.inc` 中 `sy_sha256_hex` 出现 0 次 |
| 是否在隐式 prelude | **否**：`include/cx-flat.h` 中 0 次（`cx_sha256_hex` 才是 prelude 里的） |
| 可达方式 | 仅 `#pragma unisa use sys.sha`，或手写前置声明：`cx/runtime/runexec.cx:242`、`src/runcache.cx:218`（签名一致） |
| 树内调用点 | 5 文件 6 处：`cx/runtime/runexec.cx:245`（真值）、`src/runcache.cx:229`（`== 0`）、`tools/symindex.cx:451`（`== 0`）、`tools/check-cx-sys.cx:378/381/388`（只断成功 `af_rc == 0`）、`tools/check-cx-layers.cx:395/718`（`!= 0`，go 在途） |

**结论（对 A 的影响：零）**：所有调用点只区分「零 / 非零」，负码仍是非零 ⇒ 若 go 把
`return 1;` 改成 `return -errno;`（短写无 errno，需文档化负码如 `-EIO`；参数非法 `-EINVAL`），
并保留「失败时 `hex[0]=0`」的既有行为，A 的 `!=0` + `cx_sha256_hex` 回退**一行都不用改**。

A 侧只加一条收据要求：`digest=afalg|soft` 之外再记 `digest_reason`（`ok` / `no-afalg` /
`hash-fail`）。理由：`sy_sha256_hex` 无法区分「本平台不支持」与「本次写/读失败」，没有这个字段时
一次瞬时失败会静默降级成软哈希，事后无法分辨。此项只改 A 的收据，不动 L0、不动 spec。

### 17.8 rc 分档实测（仓外夹具，`8361c3cf6b` 工具本体）

构造：`/tmp/cxrc3` 一个假根，软链 `sys`/`text`/`cxlib` 到真实模块，`unisacc.com` 换成只把预置
`manifest.tsv` + 一个 shard 拷进 `out/cx-layers-index/` 的 stub，绕开 `symindex update`。
源码 `src/a.cx` 内容 `one line\n`；四个场景各跑一次。

| 场景 | manifest 内容 | 输出 | rc |
|---|---|---|---|
| A 正常 | sha = 文件真实 sha，shard 存在，`U` 行仅 `#symindex v1` | `edges=0 failed=0`，四项 undecided 全 0 | **0** |
| B 纯未决 | sha 全 0（≠ 文件），shard 存在 | `UNDECIDED SOURCE-CHANGED`，`undecided_io=1`，`failed=0` | **3** |
| C 畸形行 | sha 长 63 | `UNDECIDED MANIFEST-PARSE`，`undecided_meta=1`，`failed=0` | **3** |
| D 失败+未决 | sha 全 0 且无 shard | `UNDECIDED SOURCE-CHANGED` + `FAIL SHARD`，`failed=1` | **1** |

由此确认（**实跑收据**，非推断）：rc 三档与「失败优先」在单根场景下按设计工作。同时复现了 §17.2
的两条：#4（哈希不匹配与不可读同归 `SOURCE-CHANGED`+#B）、#7（C 场景 `continue` 后**没有** `FAIL SHARD`
输出 ⇒ 畸形行连 shard 缺失都一起吞掉）。

清理：`rm -rf /tmp/cxrc3`。

### 17.9 修正：`digest_reason` 三分类撤回（cdx 裁定）

我上一条提的 `digest_reason=ok|no-afalg|hash-fail` **不成立**，撤回。理由（cdx 判、与静态事实一致）：
`sy_sha256_hex` 的全部失败路径都只 `return 1;`（`cx/sys/sha.cx` 7 处），调用方拿到的只是一个正 1，
**没有错误原因**。于是「本平台不支持 AF_ALG」与「本次 write/read 失败」在 A 的视角下不可区分；
用平台宏（`__linux__`/`__x86_64__`）去推断运行时原因，是用编译期条件冒充运行时观测，属超范围。
A 的收据只允许记录**可观察事实**，不得输出貌似确定的三分类。

改为三个**可数、可复核**的观察量（都是计数器，不是原因判定）：

| 字段 | 含义 | 取值 |
|---|---|---|
| `digest_afalg_ok` | 调了 `sy_sha256_hex` 且返回 0，直接采用其 hex | 次数 |
| `digest_afalg_nonzero` | 调了 `sy_sha256_hex` 且返回非零 ⇒ 已软回退到 `cx_sha256_hex` | 次数 |
| `digest_soft_direct` | 未经尝试就直接用软实现（例如按预定的尺寸阈值走软路径） | 次数 |

约束：
- **小块直接走 soft 不计入失败**，只计 `digest_soft_direct`；不得把「没用 AF_ALG」写成失败或未决。
- 三者之和必须等于本次实际取 digest 的次数（`DIGEST-SUM` 自检，不等即 rc1，与既有 `UNDECIDED-SUM`
  同属自检失败，不进 rc3）。
- 若将来 L0 提供明确负码，再单独定义原因字段；届时改版本号，不复用本组计数。
- `scanned_and_hashed_same_buffer=1`（§17.2 #2 修好后为 1）；`enforcement`/`window` 仍各按 §16.4。
- 不改 spec、不改 L0、不改 `cx/sys/sha.cx`。这属于 A 的收据格式，不是 L0 缺陷修复。

## 18. CX 上下文 preflight：根域落地障碍（只读梳理，无实施）

对象：`plans/cx-context-preflight-2026-10-31.md`（sha256 `7a5f8cdeee24be9842a870c11938921919e2deb2a1cf448ad9bba9d78e90cfdc`）。
本节只列**测量前必须先解决/先声明**的事实与缺口，不建 `roots.tsv`、不写预检工具、不开测、
不改 cxmod/entry-build/spec、不给目录定层号。

### 18.1 身份基线（本节所有数字的绑定）

| 项 | 值 |
|---|---|
| commit | `9cdfaae7442151d300265611adc134795e4666a6` |
| 产品 | `/home/devali/repos/unisacc/unisacc.com`，`unisacc 0.0.43`，sha256 `fc1d4513fb1363c8506df8afa08444882337e615bf38d82dd42f022e2c23dc6f` |
| 索引快照 | `out/cx-layers-index/manifest.tsv`，sha256 `8e452be0372008ac150608485987504795ff194e9ece2e5739d0f9580b979e1f`，1908 行，1935 shard |
| 盘点工具 | `tools/check-cx-layers.cx`（`lint sha256 93619a0a…`，go 在途，本基线取 `8361c3cf6b` 版） |

注意：manifest 由**工作区**生成（含他人未提交改动），本节 `git grep` 对的是 **commit 树**。两者
在当前时刻对 `main` 判定完全一致（见 18.2），但这不是结构保证；正式测量必须以 §2 的快照+快照索引
重算，不能引用本节数字。

### 18.2 可复用清单与索引：`D main` 与文本 `main` 的一致性（实测）

| 口径 | 数量 |
|---|---|
| 索引中带 `D main` 的 manifest path | **847** |
| commit 树中文本 `main` 定义（`git grep '^ *(int\|void) main *('`） | **847** |
| 索引有、文本无 | **0** |
| 文本有、索引无 | **0** |

847 中 `archive/` 占 216，`archive/` 外 **631**。按前缀：

| 前缀 | 数量 | 归属 |
|---|---|---|
| `tools/` | 330 | 本车道 |
| `tests/` | 176 | 本车道 |
| `apps/csih/` | 95 | **csih 车道**（AGENTS.md 规定，改动须先发信封） |
| `ujs/` | 15 | **论文 B 车道**（unisacc 侧不碰） |
| `release/` | 4 | 本车道 |
| `examples/` | 3 | 本车道 |
| `cx/` | 2 | `cx/check_memory.cx`、`cx/check_owned.cx`（仓内手工检查程序，非入口层） |
| `exec/` | 2 | `exec/c/qprefixcheck.cx`、`exec/parse2/librarycallsitecheck.cx` |
| `adapter/` | 2 | 本车道 |
| `src/` | 1 | 产品入口 |
| `research/` | 1 | `research/c43-resourceflow-probe-draft.cx`（草案探针） |

⇒ **可以**用 `D main` 作候选入口的发现手段（当前零差）；**不可以**用它宣称「全集」：
它只覆盖 manifest 里存在的 path，而 manifest 明确缺 31 个 path（18.3），且它的判定来自索引实现，
一旦索引降级（DEFERRED/不完整）就会静默变少。

### 18.3 索引**不**覆盖的部分（不能从 symindex main 宣称全集的三条理由）

1. **31 个 mode 120000 的 `.cx` 软链完全不在 manifest 里。** 实测：非 archive 的 tracked `.cx`
   1651 个，非 archive 的 manifest 行 1620 个，差 **31**，逐一对上"全部是 mode 120000"。
   例：`tests/cxlib/allsession.cx → ../../tools/cxlib/allsession.cx`、
   `cx/runtime/compiler/bindstage.cx → ../../compiler/bindstage.cx`、
   `ujs/native/compiler_min.cx → ../seed/stage0/compiler_min.cx`。
   缓解：这 31 个**没有任何一个**声明 `main`（逐个解析 target 后 grep），且真正的目标文件本身在
   manifest 里 ⇒ **当前不丢根**。但这是巧合级的缓解，不是合同：若将来新增一个「指向含 main 文件」
   的 `.cx` 软链，`D main` 枚举会静默漏掉它。
   ⇒ 候选集必须**并上** §4 规则 3 的 `tests/batches.tsv` SOURCE，并在 roots.tsv 里对
   「软链形式的根」显式声明；不得只跑 `D main`。
2. **manifest 含 288 行 `archive/`**，`tools/check-cx-layers.cx` 主动跳过 `archive/`、`out/`。
   preflight 若直接吃同一索引，会把 archive 计入候选而工具不计，两个数不可比。
3. **索引可 DEFERRED**（45 s 上限）。一旦出现，`D main` 会少行且**不报错**；preflight §3 已要求
   该情形整次 UNKNOWN，本节确认该要求是必须的，不是保守。

### 18.4 正负批次：`batch-negative` 无法从 `batches.tsv` 生成

实测 `tests/batches.tsv`：345 行 = 1 头 + **344 行数据，12 列**，`expect` 列 **344 行全是 `rc0:*`，
零条负期望**。⇒ §4 的 `batch-negative`（「SOURCE 属故意拒绝/缺模块/运行时生成的夹具」）在
`batches.tsv` 里**没有任何行可依据**：该表的语义是「这条批次整体应该 rc0」，负例是**程序内部**
的行为（例如 `tools/check-cx-layers.cx --fixtures` 里自己写 `cx/sys/q.cx` 再造 `FAIL
PARSE-CONDITIONAL`）。

后果与要求：
- **不得**把 `batches.tsv` 的 SOURCE 里"看起来像负例"的行归进 `batch-negative` 并因此缩短其展开；
  它们都是正常 rc0 批次，全部属 `batch-positive`。
- 负例的真实来源只有两类：① 运行时在 `out/` 下生成的夹具（`generated`，expect=unknown-ok）；
  ② 仓内已提交、但设计上就不该被当作产品入口的文件（`cx/check_memory.cx`、`exec/*check.cx`、
  `research/*-draft.cx`），归 `unclassified` 而不是 `negative-known`，除非逐行写明理由。
- 因此 `negative-known` 的上限是 **0 行**（当前证据下），不是任意行。要引入必须新增具名清单并说明
  「该文件为什么是负例」；不能用泛化分类代替。

### 18.5 生产入口 / 工具入口的可辩护边界

- `production`（可进入产品构建图）：`src/` **1** 个 + `release/` **4** 个 + `examples/` **3** 个
  合计 **8** 个候选（本车道）。`apps/csih/` 95 个与 `ujs/` 15 个**不能**由本车道直接列为
  production 根：前者须 csih 车道确认其入口集合，后者按 AGENTS.md 不碰。可先列为
  `unclassified`+UNKNOWN，或由 owner 提供清单后并入，二选一须显式。
- `tool`：`tools/` 330 个含 main 中，只有 214 个是 `batches.tsv` 的 SOURCE（全局唯一批次数 214）。
  ⇒ `tools/` 里**约 116 个含 main 但无批次**。这批既不能算 production，也不能默认算 tool：
  必须逐项落 `tool`（有明确人工用途）或 `unclassified`。这是 preflight 里**最大的一块未知**，
  且不能靠"在 tools/ 下"推断类别。
- 交叉核对：`main` 含 631 个、batch source 214 个，二者完全包含（214 ⊂ 631，无 batch source 缺 main）。
  非批次但含 main 的 **417** 个：`tools` 215、`apps`(csih) 95、`tests` 78、`ujs` 15、`release` 4、
  `examples` 3、`exec` 2、`adapter` 2、`cx` 1、`src` 1。

### 18.6 显式 joint root 与分账

- 现状：`src/` 只有 1 个含 main 文件，`tools/`/`tests/` 各自单源码批次占绝对多数 ⇒ **今天没有
  真实的多源码联合根**。`joint_id`/`double_count` 因此是**未来位**，不是当下缺失的测量。
- 但仍要现在就定死两条，否则将来加第一条联合根时会静默重复计量：
  - 同一 `joint_id` 的多行，`contexts_total`/`edges_total` **只在联合根内求和一次**；成员单根若也
    出现在表中，必须 `double_count=1` 且聚合时**只取其一**（建议取联合根，成员行仅作报告）。
  - 跨根 `unique_contexts` 只能报「跨根按字面路径去重的规模」，不得作为任一根的峰门，也不得触发
    身份比较（§14 已写，本节只确认它必须落到字段而不是注释）。
- `pair_peak`：各根独立，跨根求和不是峰值。

### 18.7 最小可审验收清单（只读，不建正式文件）

用于评审「这份 preflight 结果是否可信」，不用于授权测量。任一不满足即整次 UNKNOWN：

1. **输入身份**：commit 完整 sha；`git ls-tree -rz` 原始输出及其 sha256；`git archive` tar 与
   解出清单 sha256；三者互相可复核。
2. **产品身份**：显式绝对路径 + `unisacc 0.0.43` + sha256（本节基线
   `fc1d4513…2dc6f`）；不得从 PATH 猜、不得不记录。
3. **实现身份**：`implementation_source_sha` + `implementation_binary_sha` + `policy_doc_sha`
   三者齐；缺一不得出数字。
4. **索引完备**：无 `DEFERRED`；`--in` 清单 sha 记录；`--verify` 通过。
5. **根清单**：候选全集由「树 `.cx` ∩ `D main`」∪「batches.tsv SOURCE」生成；roots.tsv 缺行 →
   `ROOTS incomplete` 非零；多行 → `ROOTS stale` 单列。**软链根须显式声明**（18.3）。
6. **类别可核**：每行 `category` 只能取 18.4/18.5 定义的值；`negative-known` 当前应为 0 行，
   出现即须附逐行理由。
7. **只读强制**：`ENFORCEMENT` 只能是 `bwrap-ro-bind`/`unshare-ro-bind`；`permission-only`
   一律 UNKNOWN（§11/§13 既有结论，本节确认不可放宽）。
8. **未决不冒充**：`pairs_proxy=1` 不得进峰门；`measurement_complete` 不得简写为 `complete`；
   `unclassified` 的 UNKNOWN 不能计入生产覆盖。
9. **跨入口对拍**：夹具覆盖 `./`、空段、`a/../b`、目录软链+`..`，preflight 键与编译器
   `cxm_norm` 逐项一致，否则设计不通过（§5 既有）。
10. **残差披露**：31 个软链 `.cx` 的处置（排除/UNKNOWN）必须写进回执；不得只写「全集已覆盖」。

### 18.8 结论

- **可复用**：commit 树 + `git archive` 快照、`symindex --in … update --verify` 索引、
  `tools/check-cx-layers.cx` 的 manifest/shard 读取与 `cl_locate` 词法（作为负例与交叉核对源）、
  `tests/batches.tsv` 的 214 个批次源码集合、以及 §15.6/§18.1 的身份基线。
- **不可复用/不可宣称**：不得用 `D main` 单独宣称入口全集（18.3）；不得从 `batches.tsv` 生成
  负例（18.4）；不得把 `apps/csih/`、`ujs/` 直接列为本车道 production 根（18.5）；不得把
  `tools/` 里 116 个无批次的含 main 文件默认归 `tool`（18.5）。
- **最大未知**：`tools/` 116 个无批次含 main 文件的类别归属；`apps/csih` 95 + `ujs` 15 的入口集合；
  只读强制（当前 `bwrap` 缺失、`unshare` 只证明了 remount,ro 单点）。
- 本节不产生任何 PASS；未建 `roots.tsv`、未写预检工具、未开测、未改 cxmod/entry-build/spec。

### 18.9 修正：§18.4/§18.5/§18.6 的三处推断不成立（cdx 裁定后实算）

三处都是我拿集合减法**推**出来的，不是实算；逐条改正。

#### C1 `tools/` 未登记数：**215**，不是 116

我用 `330 − 214 = 116` 是错的：214 是**全表** SOURCE 唯一值，其中 99 个不在 `tools/` 下，
相减等于把两个不同论域的集合硬减。实算：

| 集合 | 数量 |
|---|---|
| `tools/` 含 main | 330 |
| `tools/` 含 main ∩ batches SOURCE | **115** |
| `tools/` 含 main **但不登记批次** | **215** |
| batches SOURCE 唯一值 | 214 |
| SOURCE − `tools/`-含-main | **99**（`tests/` 98 + `cx/` 1） |

⇒ SOURCE 214 = `tools` 115 + `tests` 98 + `cx` 1。未登记规模比原先说的大近一倍，未知块更重。

#### C2 `expect=rc0` 不证明源码图是正例：真正的信号在 `layer` 列

`tests/batches.tsv` 344 行：`implementation` **344 行全 IMPLEMENTED**，`expect` **344 行全 `rc0:*`**。
但 `run-batch` 判 PASS 的条件仅四条（`tests/run-batch.cx` 末段）：

```
ok = !transport && !result.rc && tx_eqz(before, after) && expect 前缀 "rc0:" && marker(...)==1
```

即「子进程没超时 + rc0 + 候选二进制前后 sha 不变 + 标记行恰好出现 1 次」。**没有任何一条**断言
被测源码图、被生成材料或被测配置是正例。把 rc0 读成"该批次内容为正例"是超读。

实际的正负信号是 `layer` 列（第 3 列）：

| layer | 行数 |
|---|---|
| contract | 133 |
| interfaces | 108 |
| **negative** | **63** |
| properties | 15 |
| platform | 10 |
| library | 8 |
| product | 7 |

`layer=negative` 63 行、24 个唯一 SOURCE（`tests/ri-flow-controls.cx` 9、`tests/objects.cx` 8、
`tests/hostseed-controls.cx` 8、`tests/ri-material-controls.cx` 7、`tests/ri-chain-controls.cx` 7 …）。
它们**仍是 rc0 批次**：程序内部跑负例，程序本身必须成功退出。

第二维是 `mode`：342 `SOURCE` + **2 `PREPARED`**。两条 PREPARED（`nets`、`assembler`）的 `entry`
是材料占位而非仓内路径：`{nets}/nets.cx`、`{assembler}/contracts.cx`，`source` 是 `tests/nets.cx`、
`tests/assembler.cx`。**未提供对应材料时，其根身份只能是 UNKNOWN**，不能算已解析根。

⇒ 分类至少要三维，缺一即错：① 行级 `layer=negative` ？② 行级 `mode`（SOURCE/PREPARED）？
③ 运行时是否生成材料？`batch-positive`/`batch-negative` 不能由 `expect` 单列得出，更不能由批次 id
命名（`full-negative` 的 source 是 `tests/full-domain.cx`，layer 是 product，不是 negative）推断。

#### C3 联合根**今天就存在**：产品本身就是 33 TU / 1 main

我原写「`src/` 只有 1 个含 main 文件 ⇒ 今天没有真实联合根」——错。查 recipe（`tools/entry-build.cx`）：

- `src_rel[0..34]` 共 35 个键、**33 个唯一 TU**；其中**只有 `src/unisacc.cx` 声明 main**。
- 33 个 TU 里 **22 个**经目录软链到达：`src/runtime → ../cx/runtime`、`src/compiler → ../cx/compiler`、
  `src/core → ../cx/core`、`src/driver → ../cx/driver`、`src/sys → ../cx/sys`、`src/text → ../cx/text`。
- 全树 tracked **目录软链 59 个**（另有 §18.3 的 31 个文件软链）。
- `git ls-files src` 只看到 **29 个条目**、其中 **15 个 `.cx`**——目录软链被 git 折叠成一个条目，
  所以按 tracked 路径枚举产品输入会**漏掉 22 个 TU**（它们物理上是 `cx/**` 的已跟踪文件）。

⇒ 这就是 cdx 说的形态：链接/构建同时提供多个 `.cx` 根、只有一处 `main`。两条后果：

1. `joint_id`/`double_count` **不是未来位**，现在就要对产品成立；而且联合根成员**不能**从 `D main`
   或 `git ls-files` 枚举，只能从 recipe（或真实 argv）取。
2. 若 preflight 只按「树 `.cx` ∩ `D main`」定根，产品根会被记成 1 个文件、其 32 个 TU 的依赖边
   会全部丢失或被错误地挂到 `cx/**` 物理路径上（两套路径并存 ⇒ 同一边可能算两次）。
   ⇒ roots 表必须显式区分 **recipe 成员（lexical 路径）** 与 **物理文件**，并对二者建立 `double_count`。

当前状态：recipe 已核（33 TU / 1 main / 22 经目录软链），**真实 argv 未核**（未抓取构建实际传入的
命令行）。在抓到 argv 前，产品根成员的完备性记为 **JOINT-UNKNOWN**，不写成已定。

#### 修正后的最小约定

- 本节所有数字绑定 commit `9cdfaae744…`；manifest 由工作区生成，与 commit 树在 `D main` 上当前零差，
  但**不作为** preflight 输入，preflight 必须用快照+快照索引重算。
- `D main` 只能做**发现**；根全集 = recipe/argv 成员 ∪ 「树 `.cx` ∩ `D main`」∪ batches SOURCE，
  三者的差集分别报告（缺 → incomplete，多 → stale）。
- 未核项显式标 UNKNOWN：产品真实 argv、`apps/csih` 入口集合、`ujs` 入口集合、215 个 tools 无批次
  文件的类别、只读强制。

## 19. 给 omp-go 的 preflight 文档最小修订建议

对象：`plans/cx-context-preflight-2026-10-31.md`（下称 P）。本节只给**差分**，不重写 P，不改
cxmod/entry-build/spec，不建 roots/工具，不开测。每条都附锚点与证据，go 可逐条采纳或驳回。

### 19.1 根身份必须四件分离（P §3 §4 §14）

P 现在把「根」当成一个 path 字符串，加 `joint_id`/`double_count` 两个修饰列。实际有四个**不同**
可观测对象，混在一个字段里会导致（a）产品 33 TU 只记 1 个根、（b）同一边因词法/物理两条路径算两次：

| 编号 | 对象 | 来源 | 现在的 P 里 |
|---|---|---|---|
| R1 | recipe 声明成员 | `tools/entry-build.cx` 的 `src_rel[0..34]`（35 键 / 33 唯一） | **无字段** |
| R2 | 实际捕获 argv | 构建真实命令行（未捕获） | **无字段** |
| R3 | 词法入口 | 传给编译器的 path 文本（如 `src/runtime/routestep.cx`） | `root` 列 |
| R4 | realpath | R3 在快照里解析后的物理文件（如 `cx/runtime/routestep.cx`） | 只在 §5 讲 realpath 组 |

建议列改（P §4 的 TSV）：

```
root        # R3 词法入口，原样
joint_id    # 空=独立根；非空=联合根
double_count
category
provenance  # 只能取下方枚举，不由 batch id 推断
origin      # NEW: recipe|argv|tree-main|batch-source（可多值逗号分隔）
physical    # NEW: R4 realpath（快照内相对路径）；无法解析留空 + UNKNOWN
expect
sha256
```

- `origin=recipe` 的行必须附 `joint_id`，且 `physical` 必须填；`physical` 落在别的目录域（如
  `cx/runtime/**`）时**不得**因为"跨域"就拒绝——那是合法的 recipe 成员，只影响层号报告。
- 只有 R1 与 R2 同时存在且成员一致，才能把联合根记 `measurement_complete`。R2 未捕获 ⇒
  `joint_root=JOINT-UNKNOWN`，此时仍可展开 R1 报规模，但**不得**作为产品覆盖证据。

### 19.2 `argv` 未捕获：显式状态，不用推断补

P §3 的回执字段里有产品身份，但没有构建命令身份。建议加两字段：

| 字段 | 取值 | 说明 |
|---|---|---|
| `build_recipe` | `recipe:<path>@<sha256>` | 至少要能指回 `tools/entry-build.cx` 的具体版本 |
| `build_argv` | `captured:<sha256(规范化argv)>` 或 `NOT-CAPTURED` | 未捕获就是 `NOT-CAPTURED` |

约束：
- `build_argv=NOT-CAPTURED` ⇒ 联合根状态只能是 `JOINT-UNKNOWN`，且聚合行里
  `joint_members_verified=0`（计数，不是布尔），不得写"联合根已核"。
- **不得**从 `git ls-files src`、`D main`、目录软链形态或 `entry-build.cx` 里**别的**函数推断
  argv；那是 recipe，不是 argv。
- 抓 argv 属于开测前置，本轮不做（cdx 已定：不再构造/开测）。

### 19.3 分类不能用 `expect` 或 batch id 推断（P §4 类别表）

实测证据（§18.4/§18.9 C2）：344 行 `implementation` 全 IMPLEMENTED、`expect` 全 `rc0:*`；
判 PASS 只查「无超时 + rc0 + 候选二进制前后 sha 相同 + 标记行恰 1 次」，**不涉及**被测内容正负。
`layer` 列才是正负信号（contract 133 / interfaces 108 / **negative 63** / properties 15 /
platform 10 / library 8 / product 7），`mode` 列区分 `SOURCE` 342 / `PREPARED` 2。
反例：`full-negative` 这个批次 id 的 source 是 `tests/full-domain.cx`、layer=`product`——
按 id 命名推断必然错。

建议把 P §4 的类别语义改成**三维可核**，并删掉 `batch-negative` 的"由 expect 推定"路径：

| 类别 | 判定条件（全部可核，不含推断） |
|---|---|
| `production` | `origin` 含 `recipe`/`argv`（且非 UNKNOWN）；或 `src/**` 直接含 main |
| `tool` | `tools/**` 含 main 且**登记在 `tests/batches.tsv` SOURCE**（115 个） |
| `batch` | 登记在 `tests/batches.tsv`，`mode=SOURCE`；正负另用 `batch_layer` 记录 `layer` 列原值 |
| `batch-prepared` | `mode=PREPARED`（2 行：`nets`、`assembler`）；未供材料 ⇒ UNKNOWN |
| `generated` | 运行时生成、不在 commit 树中；`expect=unknown-ok` |
| `unclassified` | 其余候选（含 `tools/**` 含 main 但**无批次**的 215 个） |

- `provenance` 只允许：`recipe`、`argv`、`tree-main`、`batch:<id>`、`generated:<test>`。**禁止**用
  批次 id、文件名里的 `negative`/`refuse`/`fault` 之类词充当原因。
- 若要保留 `batch-negative` 这个名字，只能作为**报告分组**（`layer=negative` 的投影），
  不能参与覆盖计量，也不能缩短展开。
- `negative-known` 的上限维持 0 行（§18.4），引入须新增具名清单 + 逐行理由。

### 19.4 候选全集三向对齐（P §4 核对门）

P 现在只有两源（树∩`D main`、batches SOURCE）。建议改为三源**分别**报告：

```
S1 = recipe/argv 成员（R1，若 R2 缺失则只报 R1 并标 UNKNOWN）
S2 = 树 .cx ∩ D main（P 现有规则 1–2）
S3 = tests/batches.tsv SOURCE
候选 = S1 ∪ S2 ∪ S3
```

差集必须分开写，不允许合并成"缺失数"：

| 差集 | 含义 | 处置 |
|---|---|---|
| `S2 \ S3` = 417 | 有 main 但无批次 | 逐项落 `tool`/`unclassified` |
| `S3 \ S2` = 0 | 有批次但无 main | 当前为空，出现即 `ROOTS STALE-SOURCE` |
| `S1 \ S2` | recipe 成员但索引里无 main | **正常**（TU 无 main），必须按联合根成员处理，不算缺失 |
| `S2 \ S1` | 索引有 main 但 recipe 不用 | 报出，不算错 |

实测（§18.2）：`S2` 847 = `D main` 847，双向零差；`S3` 214；`S2∩S3` 214。
另外必须声明：**`D main` 只覆盖 manifest 里的 path**，31 个 mode 120000 的 `.cx` 软链不在内
（当前不含 main，属巧合级缓解），所以 S2 是"发现集"而非"全集"（§18.3）。

### 19.5 跨根只做统计，不混峰值（P §6 §14 已写，补一句可执行定义）

P §6 写「不把求和冒充全局同时峰值」，§14 写「跨根求和只作统计量」。建议加一句把边界钉死：

> 联合根内部的 `pairs_visited`/`pair_peak` **只在该 `joint_id` 内求和一次**；成员单根若同时出现在
> 表中必须 `double_count=1` 且聚合时**只取联合根那一份**（成员行仅作报告）。不同 `joint_id`
> 之间、以及独立根之间的求和只能进 `AGGREGATE`，字段名必须带 `sum_` 前缀（如
> `sum_edges`），不得写入任何 `peak`/`max_*` 字段。

理由：未来一旦新增联合根，`double_count` 语义若只写在散文里，聚合代码会静默双计；提前钉成字段
前缀是最小成本的可核约束。

### 19.6 采纳顺序建议（给 go 的优先级）

| 优先级 | 项 | 理由 |
|---|---|---|
| P0 | 19.2 `build_argv=NOT-CAPTURED` + `JOINT-UNKNOWN` | 现在是错的默认：P 未声明 argv 缺失，读者会当成"根已确定" |
| P0 | 19.1 `physical` 列 + recipe 成员不算缺失 | 不做则产品 32 个 TU 会成为隐形缺口或有重复边 |
| P1 | 19.3 类别三维化、删 expect 推定 | 直接影响 `negative-known=0` 与覆盖计量 |
| P1 | 19.4 三源差集分报 | 现在 `S1` 完全缺席 |
| P2 | 19.5 `sum_` 前缀约定 | 未来联合根才会触发，但改字段名要趁早 |

本节不含任何 PASS；未改 P、未建 roots/工具、未动 cxmod/entry-build/spec。身份与 epoch 沿用 §18.1，
不外推到其它 commit。

### 19.7 重要更正：33 TU 是**构造/链接组**，不是一次 33 根 cxm 调用

cdx 指出的区别成立，且我 §18.9 C3 的措辞会误导。核 `tools/entry-build.cx`：

| 事实 | 位置 |
|---|---|
| `eb_plan` 的 argv 逐字为 `[bound, "50", (sh), ctor, "-cx-plan", src]`，**只追加一个 src** | `:151-166` |
| `eb_compile` 的 argv 为 `[bound, "55", (sh), ctor, opt, "-c", "-b", triple, "-funit", "-o", obj, src]`，**同样只一个 src** | `:227-245` |
| 33 个 TU 由 `while (i < 33) eb_compile(...)` 逐个调用 | `:508-523` |
| 计划阶段也是 `while (i < 33) eb_plan(...)` 逐个调用，每 src 一次 | `:527-533` |

⇒ 源码层面**支持**「产品构造组含多单元、每次计划/编译单根」的模型。**但这是源码事实，不是运行证据**：
未捕获实际 argv，不能排除 via_sh 分支、`only_keys` 子集、或其它调用者走不同形状。故：

- 33 路**不得**自动构成一个 cxm 多根图；单次调用的 context/canonical/pair 峰值默认**按调用隔离**。
- 产品构造组要作为独立范围（下称 **build-instance**）另立，不得与"同一次 cxm 调用的多根"混用同一套
  计数字段（参见 19.5）。

### 19.8 构造级跨计划合并：确有其事，且比单根风险更隐蔽

`tools/entry-build.cx:521-582`：33 次 `eb_plan` 的输出**全部进入同一个 `ncl`/`eb_creal` 去重域**：

| 行 | 行为 |
|---|---|
| `:521` | `ncl = 0`（整个计划阶段共享一个计数） |
| `:566-570` | 新候选的 realpath `rp` 与 `eb_creal[0..ncl)` 逐个比较，命中即 `dup=1` **跳过编译** |
| `:571` | `ncl >= EB_MAXC` → `eb_die("closure-cap")`（上限拒绝，非静默） |
| `:573-582` | 未重复者追加 `eb_csrc`/`eb_creal`，编号 `m%ld.o` |
| `:587-594` | 对 `ncl` 个条目**逐个** `eb_compile`（同样是单 src 调用） |
| `:604-606` | `ncl` 个目标文件一起 `eb_link` |

即：**跨 33 个 plan 调用做 realpath 去重，结果并进一次链接**。这不是一次 cxm 多根调用，却是独立的
**构造级跨 root 合并**风险：

- 单根 preflight 合格（每根展开正确）**不能**推出构造级身份一致：两个根若解析到同一物理文件，
  构造期只编译一次、只算一个目标；而 preflight 若按词法根分别展开并**分别求和**，会**多计**；
  若按 realpath 合并后**只报一份**，又无法区分"同一根内重复"与"跨根合并"。
- 反过来，`dup` 判定用的是 `eb_real`（见 19.9），失败时回退词法路径 ⇒ 同一物理文件在两次计划里
  若一次 `sy_realpath` 成功、一次失败，会被当成**两个**不同单元（漏合并、链接期重复符号或双份对象）。
- 因此 build-instance 必须有**自己的**去重域与计数：`build_units`（去重后编译单元数，对应 `ncl`）、
  `build_plan_calls`（计划调用次数，理想 33）、`build_dedup_hits`（被 `dup` 跳过的候选数）。
  三者关系 `build_plan_calls = build_units + build_dedup_hits` 可作为构造级自检（不等则构造图不自洽）。
- 该自检只能证明**构造脚本**自洽，不能证明产品身份一致，也不能替代单次 cxm 的 pair 统计。

### 19.9 `eb_real` 的回退：错误类别未区分（源码事实，单列风险）

```c
static void eb_real(const char *p, char *dst, int cap) {
    if (sy_realpath(p, dst, cap) >= 0) return;
    eb_copy_tok(dst, cap, p);      /* :346-349 */
}
```

- `sy_realpath` 的任何失败（`ENOENT` 悬空、`EACCES`、`ENOTDIR`、路径过长、`ELOOP`）都**同样**退回
  词法路径，调用方拿到一个"看起来成功"的字符串，**没有错误类别**。
- 后果：① 19.8 的 `dup` 判定在失败回退下可能把两条词法路径当两个单元、或把二次失败当不同；
  ② 后续 `eb_compile` 若对这条件失败，报错点会落在编译而非路径解析，诊断指向错误的位置。
- 与 CX-Y-4 无关（`sy_realpath` 在 `cx/sys/posix.cx`，确实返回负 errno）；问题在**调用方吞掉**了返回值。
- 本节**只记录**：不改 `tools/entry-build.cx`，不授 W-1，不把该行为当作 CX-Y 类违规。

### 19.10 对 P 的追加修订（并入 19.1–19.6）

| 编号 | 追加内容 |
|---|---|
| 19.1+ | 根的 `origin` 增加 `build-instance:<id>`；`physical` 必须来自**捕获到的** realpath，回退词法路径时置空 + UNKNOWN |
| 19.2+ | 回执增 `build_instance_id`、`build_plan_calls`、`build_units`、`build_dedup_hits`；`build_argv=NOT-CAPTURED` 时这四个只能记 UNMEASURED/UNKNOWN |
| 19.5+ | 明确三层，互不替代：**单调用**（一次 cxm 的根，context/pair 峰值按调用隔离）／**联合根**（同一次调用多根，`joint_id`）／**build-instance**（跨调用、跨计划去重的构造组）。三层各自的求和域与峰值域分开命名，禁止跨层比较 |
| 新 | 单根 preflight 通过 **不蕴含** build-instance 身份一致；反之亦然。两者结论不得互相引用作为证据 |

本节不含 PASS；未改 P、未改 `entry-build.cx`、未建 roots/工具、未开测。身份沿用 §18.1，不外推。

### 19.11 更正：§19.8 的计数等式单位错误；`sy_realpath` 文件位置也写错

cdx 指出的两处，逐项改正。

#### A. `build_plan_calls = build_units + build_dedup_hits` **不成立**

`ncl` 的单位是**接纳的闭包对象**，`eb_plan` 的单位是**调用**；一次调用输出**多行**，每行各自做去重
判定。把三者的加法写成等式是量纲错。核 `tools/entry-build.cx:534-583` 的实际分支：

| 行 | 条件 | 结局 |
|---|---|---|
| `:548` | `f2 >= e`（无 TAB ⇒ 畸形行） | `continue`，**不计入任何接纳** |
| `:552` | `len <= 0 \|\| len >= 576` | `continue`，同上 |
| `:556` | `cx_fmt` 溢出 | `eb_die("plan-path")`，**整次构造中止** |
| `:557` | `eb_real(full, rp, …)` | 可能回退词法路径（见 §19.9） |
| `:560-563` | `rp` 命中 33 个根的 realpath | `dup=1` ⇒ **根命中** |
| `:564-568` | `rp` 命中既有 `eb_creal[0..ncl)` | `dup=1` ⇒ **闭包重复** |
| `:571` | `ncl >= EB_MAXC` | `eb_die("closure-cap")`，中止 |
| `:573-582` | 以上皆否 | **接纳**，`ncl++` |

正确做法是**七个分立的计数**（单位各自声明），不是一个等式：

| 计数 | 单位 | 含义 |
|---|---|---|
| `build_plan_calls` | 调用 | 实际成功返回的 `eb_plan` 次数（理想 33，`only_n` 时为 0） |
| `plan_output_rows` | 行 | 所有 plan 输出的非空行总数 |
| `plan_malformed_rows` | 行 | `:548` 无 TAB |
| `plan_invalid_len_rows` | 行 | `:552` 长度越界 |
| `plan_root_hits` | 行 | `:560-563` 命中 33 根 realpath |
| `plan_closure_hits` | 行 | `:564-568` 命中既有闭包 |
| `build_units`（= `ncl`） | 对象 | `:573-582` 接纳的**新增闭包对象**；**不含** 33 个根 |
| `plan_fatal` | 次 | `plan-path` / `closure-cap` 中止 |

**唯一可断言的等式**（且仅在无 `plan_fatal`、且每行分类互斥完备时）：

```
plan_output_rows == plan_malformed_rows + plan_invalid_len_rows
                  + plan_root_hits + plan_closure_hits + build_units
```

任一不满足（有 fatal、或出现未归类行）⇒ 该式**不成立**，只能报各计数原值 + `closure_selfcheck=UNKNOWN`。
**不得**为了凑等式把 `malformed`/`invalid_len` 塞进"重复"。

另两条 cdx 点出的陷阱，写进约束：

- **budget 命中（`closure-cap`）不是接纳**：它是中止，`build_units` 不是一个完整数，此后 `ncl` 既不
  增长也不代表闭包大小；该次构造只能记 `closure_complete=0` + UNKNOWN。
- **realpath 回退不是接纳理由**：`eb_real` 失败退回词法路径时，`root_hits`/`closure_hits` 的判定
  基于**不可靠的键**——同物理文件可能被算作两个不同单元（漏合并），也可能因两次都回退而"看起来
  未重复"。此类行必须单列 `plan_realpath_fallback_rows`，其归类为 **UNKNOWN**，不得计入
  `build_units` 的"已核"部分（只能算"已接纳但身份未核"）。

⇒ 因此 `build_units` 要拆成两个可审计的数：`build_units_identity_verified` 与
`build_units_identity_unverified`（后者 ≥ `plan_realpath_fallback_rows` 对应行）。

#### B. `sy_realpath` 不在 `cx/sys/posix.cx`，在 `cx/sys/fs.cx:95`

§19.9 的括号注释写错文件。实测：

```c
/* cx/sys/fs.cx:92-95 */
/* The canonical absolute path of PATH ... Its length, or -errno (-ENAMETOOLONG when it does not fit). */
int sy_realpath(const char *path, char *dst, int cap) {
```

- 成功返回**长度**（`>= 0`），失败返回 `-errno`（`-EINVAL`/`-ENAMETOOLONG`/`sy_fail()`）；**符合
  CX-Y-4**，没有任何正 `1`。
- 故 §19.9 的结论不变、定性更明确：**完全正确**的 L0 函数，被调用方 `eb_real`（`entry-build.cx:346-349`）
  用 `>= 0` 判成功、失败即静默回退词法路径，**丢弃了 -errno 的全部区分**。这是上层吞错，与 L0 无关。
- `cx/sys/fs.cx` 当前有他人未提交改动，本节只读，不触碰。

#### C. 未变项

`33 TU = build-instance ≠ 一次多根 cxm 调用`（§19.7）不变；单根 preflight 不蕴含构造级身份一致
（§19.8）不变。**真实 argv 与运行数据仍未捕获** ⇒ 上述所有计数在当前只能是**源码推导的字段设计**，
不是实测值，记 UNMEASURED；不得写成运行验证。
