# 0.0.43 第 6 步第五刀：名单外读取扫描与 ledgercheck 单套件声明（只读；不改 gatedeps）

标签：**[读源码]** = file:line；**[计算]** = 只读计算（未改文件，未跑队列）；**[实际跑过]** = strace `-f -e openat` 单作业（≤55 秒）。
前提：`seed/` 已按 780c5a95 进入全局名单，因此本刀只看名单外的 `plans/`、`prd.md`、`spec.md`、`research/`、`archive/`、`docs/`，以及 `/tmp` 与系统路径。

## 1. 已声明套件的指纹确实包含声明的 trees [读源码 + 计算]
- `tests/gatequeue.py:385–398`：`inputs` 由 `files` 与每个声明的 `trees` 展开而来；对名单外的 tree（如 `plans`、`seed`）单独做 `git ls-files`（`:391–396`）。
- `tests/gatequeue.py:410`（非窄路径）：`identity = [common, command, settings, executables, global_inputs, {n: digest(n) for n in inputs}]`。所以声明的 trees 的内容摘要进入指纹。
- **计算证明（ledgercheck，拟声明 files=`tests/ledgercheck.py`、`src/version.h`，trees=`plans`）**：展开得 11 个输入，其中 `plans/v0.0.43.md` 在内（`plans` 下 8 个已跟踪文件，含 `plans/roadmap-0.0.x.md`、`plans/v0.0.37..42.md`）。`plans/v0.0.43.md` **不在全局名单**（计算：`False`），因此它只通过声明进入指纹。
- **副作用必须说明**：`plans/` 已经被另外三个套件声明（`script-inventory`、`publish-order`、`gate-infra`，见 `tests/gatedeps.json`，计算核对），改 `plans/v0.0.43.md` 会让它们**也**重跑；不是“只改 ledgercheck”。这三个是既有行为，本刀不改它们。

## 2. 名单外读取：静态扫描与抽样 strace [计算 + 实际跑过]
静态扫描（全部 749 个作业的命令与脚本正文）：71 个作业命令或脚本正文提到 `plans/`、`prd.md`、`spec.md`、`research/`、`archive/`，清单在 `research/c43-flow-reuse-step4-outside.tsv`。静态的 “read-ish” 判定过粗，只能作上界。

抽样 strace（实际跑过）：

| 作业 | 实际读取（名单外） | 结论 |
|---|---|---|
| ledgercheck（前一刀已测） | `plans/roadmap-0.0.x.md`、`plans/v0.0.41.md`、`plans/v0.0.42.md` | **真漏洞**，需单套件声明 |
| subtract-safety | `plans/gate-overlaps.md`、`plans/ideas.md`、`plans/roadmap-0.0.x.md`、`plans/v0.0.37/39/40/41/42.md`，以及 `docs/*`（多个） | **真漏洞**；`docs/` 也在名单外 |
| docs | `prd.md`、`spec.md`、`research/model-bytes.json`、`research/unisacc-paper.md` | **真漏洞**：它改 prd/spec 的内容时，指纹不变 |
| rowcov-pp | 无（静态提到 `research/`，运行时未读） | 静态误报 |

在 5 个抽样里，3 个确有名单外读取，1 个是静态误报。静态清单应按“需要 strace 才算数”处理。

## 3. 一个必须澄清的事实：`docs/` 也在名单外
`docs/` 不在 `tests/gatequeue.py:186–188` 的 pathspec 中（计算核对）。subtract-safety 与 `docs/*.md` 的读取说明：名单外不只有 `plans/`，还有整个 `docs/`、`archive/`、`research/`（除 `research/referee.tsv`）。这一点在上一刀的名单论证里没有覆盖。

## 4. 声明草案（不写进 gatedeps；提交前需问登记窗）
- `ledgercheck`：`files: [tests/ledgercheck.py, src/version.h]`，`trees: [plans]`。（已验证机制，见 §1。）
- `subtract-safety`（含 selftest）：`trees: [plans, docs, archive]`，files 取其脚本。读集由 strace 确认，但 `archive/` 本次未在抽样中读到，需要再确认。
- `docs`：`files: [prd.md, spec.md, research/model-bytes.json, research/unisacc-paper.md]`，由脚本的 `docs.sh` 读取列表确定。**注意**：prd/spec 只作为本套件的声明输入，不进全局名单（与 780c5a95 的决定一致）。
- 其余 67 个静态命中作业：需要 strace 或读脚本确认后才能声明，不能按静态清单一次性声明。

## 5. 风险与建议
1. 声明缺口仍在：静态清单有误报（rowcov-pp）也可能有漏报（脚本间接读取）。只靠静态不足，需要每个套件一次 strace 审计。
2. 声明 `plans/` 会让三个既有套件也重跑（§1），这是正确但昂贵的副作用；应在登记窗统一提交。
3. 把 `docs/`、`archive/` 加入名单而非声明，会让每次改文档都全量重跑（与 cc 的 780c5a95 决定相反），不建议。
4. `gatedeps.json` 的改动须经 `tests/refresh_gatedeps.py` 生成并在登记窗提交；本刀**没有**修改它。
