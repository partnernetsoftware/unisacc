# W6 工具瘦身：归档清单与行数棘轮（2026-11-02）

依据：`plans/tools-inventory-2026-11-02.tsv`（351 行；当前 `tools/*.cx` 已增至 361）。
只写计划，不移动/删除源码。判定口径：
- `src`：`.cx/.c/.h/.sh/.ts/.js/.py/Makefile` 中的引用；排除 `plans/`、`tests/`、`archive/`、`out/` 与自身。
- `batch`：`tests/batches.tsv` 是否以该文件为入口/实现。
- `check`：`tests/cxraw.tsv`、`tests/cxlibc.tsv`、`tests/cxplatform.tsv`、`tests/cxcc/units.tsv` 等 check/棘轮表是否登记。
- `docs`：`*.md` 引用；文档引用不等于执行引用，但归档时同步改文档。
- 符号链接不计入文件行数，另表。

## 结论

严格按 `src + batch + check` 全为 0：**3 个**（168 行），但都有 md 引用，**可无条件归档 0 个**；其中 `precommit.cx` 是 `.git/hooks/pre-commit` 运行时入口，**不归档**。有效 A 档为 2 个/100 行。盘点表的 25 个“完全无引用”候选实际有 22 个被 check 表登记（含 source-job-wave）、3 个仅文档出现（含 precommit 运行时入口）。

## A. 可归档（仅文档引用；2 个/100 行；2026-11-02 已移至 `archive/tools-w6/`）

| 文件 | 行 | src | batch | check | 处置 |
|---|---:|---:|---:|---:|---|
| `check-cx-sys-fs.cx` | 67 | 0 | 0 | 0 | 归档；README 注明由 `check-cx-sys-*.cx` 覆盖 |
| `deadloop-guard.cx` | 33 | 0 | 0 | 0 | 归档；仅盘点文档引用 |

## B. 清 check 表后可归档（21 文件；1,128 行；2026-11-02 已归档 19，撤回 2）

| 文件 | 行 | check |
|---|---:|---|
| `check-core-execute-real.cx` | 19 | cxraw,cxlibc |
| `check-core-transition-real.cx` | 19 | cxraw,cxlibc |
| `check-cx-cross-units.cx` | 29 | cxraw,cxlibc |
| `check-cx-driver-target-unit.cx` | 20 | cxraw,cxlibc |
| `check-cx-separate-use.cx` | 32 | cxraw,cxlibc |
| `check-cx-unit-graph.cx` | 33 | cxraw,cxlibc |
| `check-driver-target-long.cx` | 28 | cxraw,cxlibc |
| `cxproc_ask.cx` | 92 | cxraw,cxlibc,cxcc/units |
| `driverdiag-candidate-install.cx` | 34 | cxraw,cxlibc |
| `fifo-candidate-install.cx` | 34 | cxraw,cxlibc |
| `fsize-exec.cx` | 14 | cxraw,cxplatform,cxlibc |
| `output-file-cases.cx` | 51 | cxraw,cxplatform,cxlibc |
| `p3-member-full.cx` | 421 | cxraw,cxplatform,cxlibc |
| `pp-built-private.cx` | 34 | cxraw,cxplatform,cxlibc |
| `pp-fullname-gen.cx` | 23 | cxraw,cxlibc |
| `pp-fullname-pack.cx` | 28 | cxraw,cxlibc |
| `pp-private-install.cx` | 26 | cxraw,cxplatform,cxlibc |
| `practice-slice.cx` | 32 | cxlibc |
| `runtime-budget-cases.cx` | 43 | cxraw,cxlibc |
| `tapelink-candidate-install.cx` | 34 | cxraw,cxlibc |
| `ucn-identity-tables.cx` | 82 | cxraw,cxlibc |

注意：`ucn-identity-tables.cx` 同时是 `exec/parse2/ucnidentity-*.tsv` 的生成器；`runtime-budget-cases.cx` 被 `tests/resources-domain-matrix.tsv` 引用，二者均**不归档**（2026-11-02 已撤回）。

## C. 源码引用，先改调用方（1 文件；66 行）

- `source-job-wave.cx`(66)：被 `tools/check-source-job-count.cx:11,77` 读取；先改该 check 或随构造链迁移，不能直接归档。
- 盘点表其余行中 `source-job-wave` 同时有 check 表登记；本项只列为“源码阻塞”。

## D. 不归档

- `tools/cx-workspace.cx`(3)：无 src/batch/check，但 `cx/workspace/README.md` 明确它是 `plan/build/resume/test` 入口；保留。
- 所有进入 `tests/batches.tsv` 的 135 个工具：不动。
- 当前 `tools/` 下 23 个符号链接：见下表；不按普通文件归档，先核对目标。

## E. 符号链接（23，不计行数）

```text
tools/compiler -> ../cx/compiler
tools/core -> ../cx/core
tools/crypto -> ../ext/crypto
tools/cx0/{bundle,plan}.cx -> ../../cx/cx0/*
tools/cxlib/{codec,packagefooter,reaper}.cx -> cx/runtime,cx/sys
tools/cxlib/{sys,sysx,text} -> cx/sys,cx/ext 目标
tools/driver -> ../cx/driver
tools/install-fixes/dedupe/sub/text -> ../../../../cx/text
tools/install-fixes/{dedupe,dupmod,iproto}/text -> ../../../cx/text
tools/install-fixes/dupmod/up -> .        # 自指目录，优先审计
tools/{mem,runtime,sys,sysx,text,workspace} -> cx/ 或 ext/
```

处置：链接目标存在且被使用者引用则保留；`dupmod/up -> .`、指向 `ext/crypto`/`ext/sysx` 的链接随 ext 迁移单独核对；本计划不删。

## F. tools 行数棘轮（只降不升）

- 基线：当前 `git ls-files -s tools` 中 mode `100644/100755` 的 `.cx` 文件总行数 = **60,724**（排除 `tools/install-fixes/` 为 **60,471**；符号链接不计）。
- 新增 `tools/check-tools-lines.cx` 或 CI 步骤：按上述命令重算，`current > baseline` 即失败；`current < baseline` 才允许把基线写回 `tools/tools-lines.baseline`。
- 规则：**只降不升**；新增文件必须同批有等量删除；更新基线只能随已审核的归档提交，不单独刷新。`install-fixes` 作为独立排除项单列，避免复制树污染总账。
- 归档提交顺序：①清 `tests/cxraw/cxlibc/cxplatform/cxcc` 中对应行并跑七棘轮 → ②移动文件到 `archive/tools-diet-2026-11-02/` 并写 SHA256 清单 → ③更新 `plans/tools-inventory`/`prd.md` 引用 → ④更新基线（净减 1,362 行）→ ⑤`bun`/产品检查与 `doctor` 无关，tools 行数检查单独跑。

## G. 首批建议

先做 A 档 2 文件（100 行，风险最低；precommit 保留）；B 档按 `p3-member-full`(421)、`cxproc_ask`(92)、`pp-private-install`(26) 分批，每批清 check 表并保留归档证据。C/D 档不动。所有移动只走 `archive/`，不直接删除。
