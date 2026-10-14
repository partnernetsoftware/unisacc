# 0.0.43 第 6 步：分层复验的第一刀（调查与设计，只读，不改队列与门禁）

标签：**[读源码]** = file:line；**[实际跑过]** = `tests/affected.py --rev …` 在本仓库四段真实改动上的回放（`AFFECTED_ROOT` 指向本检出）。
工具：`tests/affected.py`（只读，登记 ungated，不入闸）。发布仍是整闸；本工具只用于日常挑套件。

## 1. 现状 [读源码]
- **逐套件声明**：`tests/gatedeps.json` 665 个套件（`suites`），1 个家族 `compilercheck`。其中 632 个 `executable_inputs: true`，即依赖候选字节或源树（`tests/gatequeue.py` 的 `executable_inputs()`，约 119 行起）；151 个命令里点名产品，481 个不点名（多数经 `$UA`，即由源树 `export_ref` 构建的参考，仍属源树依赖）。
- **结果复用规则**：每个作业的指纹是 `[common, command, settings, executables, inputs]`（`tests/gatequeue.py:400–408`）。声明过的套件只用它的 files/trees/guards 与可执行身份；**未声明的套件用全局输入**（`global_inputs = {n: digest(n) for n in names}`，:404），而 `names` 是 `src exec tests include kernel weights unisa examples iterate release scripts unisacc.c …` 下所有受版本控制或未跟踪的文件（:188–189）。结论：**任何 tests/、exec/、unisa/ 的改动都会让未声明的套件失效**。与 CLAUDE.md“测试结果不跨源码变化复用”一致，是保守而非错误。
- **分层**：`tests/gatelayers.py` 把套件归入 contract / stage / pipeline / platform，只用于诊断，不用于选择或跳过（文件头注释）。它不知道改动路径。
- **封存**：`tests/seals/exec-pp-literals`（44 次模型执行的 pp 叶子封存）与 `tests/seals/middleflow-step0`（中段第 0 步）是数据，由各自的检查读取哈希校验；它们不是 gatedeps 的复用凭证，门禁仍按指纹重跑对应套件。

## 2. 回放：四段真实改动 [实际跑过]
“闸作业”以 `tests/gate.sh --list --com` 的 748 行计；cc 的“722”是队列实际完成数（口径待对齐，见 §5）。

| 改动 | 路径数 | 声明受影响 | 未声明（全局输入命中） | 闸作业受影响 / 748 |
|---|---:|---:|---:|---:|
| 0571eb62 Tape 验域 | 14 | 578 | 159 | **737** |
| 03936e9f..ecdbd98d 晚绑定第一刀 | 5 | 576 | 159 | **735** |
| df1ce7ed 转发 include | 7 | 576 | 159 | **735** |
| 6420cfb3 tapebin 拆分（只 tests/） | 7 | 14 | 159 | **173** |

读法：产品闭包（exec/unisa/src/…）一动，几乎全部 748 个作业都失效（632 个 executable 声明 + 159 个未声明）。只改 tests/ 的 6420cfb3 仍命中 159 个未声明作业，因为 `tests/` 本身在全局输入里。“没有任何声明认领的路径”为 0，即这四段的改动都能被至少一个声明或全局规则覆盖。

## 3. 第一刀设计（已实现为 `tests/affected.py`）
- 输入：`--rev C` / `--rev A..B` / 路径列表；输出：受影响的闸作业数与分因（files / trees / guard / product / family / undeclared），以及“没有任何声明认领”的路径（漏验风险）。
- 只读：不写队列、不写门禁、不写 gatedeps。运行 ≤ 1 秒。
- **早段封存（pp/lex/units）何时可复用**：只有同时满足以下条件才可把封存当作当前证据：① 封存清单里记录的每个输入哈希与当前一致（`package.tsv` / `closure.expected.tsv`）；② 对应阶段网络的哈希与当前产品的 `model-audit/models.json` 一致；③ 检查代码本身未变；④ 目标、标志、源标签的规范形式未变（**源标签见 step0 设计：绝对路径输入时含目录，封存必须用相对名**）。任一不满足即作废，回到整验证。
- 默认策略：`affected` 只用来**挑选**套件跑；`发布` 永远整闸（748）。

## 4. 声明缺口：最大风险（漏验）
1. **未声明的套件**（159 个）用全局输入：过保守，但不漏验。风险是它们的成本（每次几乎都跑）。
2. **声明不全**（最大风险）：`affected` 只能看到声明的 files/trees；若某套件运行时读了未声明的路径（子进程、`include`、数据文件），改动它时不会失效，而工具会报“未认领”——只有该路径落在全局输入里才不漏。应做的：对每个声明做一次读取审计（运行时记录 `openat` 路径，比对声明），这是第 6 步的后续工作，不在第一刀。
3. **可执行身份的粒度**：632 个 `executable_inputs` 把候选字节当依赖；481 个命令不点名产品，多数经 `$UA`（源树构建）。这些是否真需要候选字节，需要逐个确认；确认前不得据此减少依赖。
4. **分层不是选择依据**：gatelayers 只诊断；不要用它决定跳过。

## 5. 与 env-pin / 正式定点闸的关系
- env-pin 与正式定点闸（队列、comboot、fixedpoint、release.sh --com）保持不变；本工具不读也不写它们的状态，也不产生任何发布证据。
- 日常验收可用 `affected` 挑选套件（例如只跑 exec-net 与对应的 com-*），结果只作“本次改动的验收记录”，不计入发布证据。
- 发布前整闸仍须对封存字节（fresh）跑完全部作业；“722 vs 748”的口径差异须在发布前对齐，否则不能说“整闸全绿”。

## 6. 下一步（建议，不执行）
1. 把 159 个未声明作业逐个补声明（先做 com-* 与 exec-*），减少全局输入的过保守。
2. 做读取审计（只读），找出声明缺口。
3. 对 481 个不点名产品的 executable 声明，确认其真实依赖（UA 还是候选字节），据此收窄。
