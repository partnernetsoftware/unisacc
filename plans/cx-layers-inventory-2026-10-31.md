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
