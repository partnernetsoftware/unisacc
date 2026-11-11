# c43 k2b 窄指纹启用评估（只读设计，2026-10-14）

结论：**不启用**。先修声明，再只试点 `c99-ledger` 与 `ccinterop` 两个。收益很小（见第三节），风险在声明不完整（见第二节）。`tests/k2b-whitelist.tsv` 未改（仍为空）。

## 一、现状（代码）

- `tests/gatequeue.py` 的窄路径要同时满足：套件在白名单中（`k2b-whitelist.tsv`，当前 0 行），且守卫摘要审计通过（`guards`）。不满足即回到全局输入（`global_inputs`，即全部 names 的摘要）。
- 因此今天**任何名单内改动都会重跑所有已声明但未白名单的套件**。`tests/affected.py` 回放：0571eb62 影响 736/753，346ba62e 730/753，c4d45200 733/753。
- 声明里的 `files` 与 `trees` 是窄路径的输入集；它们若不完整，窄指纹会**漏验**。

## 二、候选逐个核对（干净导出上实测读集，对比 gatedeps 声明）

实测来源：`scratchpad` 批次 05–10 的 strace（导出 1d9cb115 与 578ec1cc）。只把 rc=0 且读集完整的作业当作候选；超时（rc124）读集截断，不能判断。

| 套件 | rc | 实测读 | 声明未覆盖的读 | 名单外 | 结论 |
|---|---:|---:|---|---|---|
| c99-ledger | 0 | 3 | 0 | 0 | **合格** |
| ccinterop | 0 | 96 | 0（声明 323 个） | 0 | **合格** |
| rowcov-lower-1..8 | 0 | 1629–1637 | 0（声明 2238 个，≈全 exec/src） | 0 | 合格但声明过宽，省得少 |
| subtract-safety-selftest | 0 | 1 | 0（声明 555 个） | 0 | 合格但声明过宽 |
| rowcov-lex | 0 | 553 | `tests/sourceflat.py` | 0 | **声明缺口，先补** |
| k2b | 0 | 1 | `tests/k2bcheck.py` | — | 声明缺口，先补 |
| com-tapedomain、com-latebind | 0 | 2 | `tests/bound`、`tests/bound.c` | 0 | 声明缺口，先补 |
| exittable | 0 | 11 | 8（`release/tools/*.py`、`release/*.tsv` 等） | 2（已声明的 plans/archive 之外） | 不合格 |
| decision-ledger | 0 | 260 | **258**（`exec/*.py`、`exec/c/*` 等，声明只有 2 个文件） | 1 | **不合格，声明严重不完整** |
| precommithook | 0 | 10 | 3（`release/tools/checkrun.sh`、`derivedcheck.py`、`tests/gatedeps.json`） | 2（点文件已补声明） | 不合格 |
| realprog | 0 | 178 | 73（`Makefile`、`README.md`、corpus 等） | 73（外部 corpus 与点文件） | 不合格 |
| h1source | 0 | 4 | 未声明 | 0 | 不合格（未声明） |
| tools-1..3 | 0 | 25–28 | 未声明（corpus 外部固定语料） | 9–12 | 不合格 |
| com-tools | 0 | 37 | 32（corpus 外部） | 31 | 不合格 |
| rowcov-pp*（pp、pp-locations、pp-shared） | 124 | 不完整 | — | 0（窗口内） | **无法判断**（超时） |
| docs、ledgercheck | — | 未测 | — | — | 未测，需先 strace |

要点：

- 合格的只有 `c99-ledger`、`ccinterop`，以及声明过宽的 `rowcov-lower-1..8`、`subtract-safety-selftest`。
- 声明缺口有两类：**真实读取不在声明里**（decision-ledger、exittable、precommithook、realprog、k2b、com-*），这会漏验；以及**外部固定语料**（corpus，tools/realprog/com-tools），它不在仓内，窄指纹无法覆盖。
- pp 族的读集从来没有完整测出（超时），所以不能用于窄指纹；docs、ledgercheck 也还没测。

## 三、收益（affected.py 回放 + 候选重算）

| 提交 | 候选 9 个中窄路径会跳过 | 全部作业今天受影响 |
|---|---:|---:|
| 0571eb62 | 1（c99-ledger） | 736/753 |
| 346ba62e | 2（c99-ledger、ccinterop） | 730/753 |
| c4d45200 | 1（c99-ledger） | 733/753 |

收益按作业数计约 0.1–0.3%。候选中的 `rowcov-lower-*` 声明覆盖几乎全部 exec/src，所以几乎每次改动都会重跑。窄指纹的主要价值在于**声明完整之后**的大套件，而目前它们的声明都不完整。

## 四、风险与启用前提

1. **漏验是主要风险**：decision-ledger 只声明 2 个文件、却读 258 个，若启用窄路径，改 `exec/` 的 Python 不会重跑它。这类声明必须先修。
2. **外部语料不进窄指纹**：corpus（tools、realprog、com-tools）按固定提交号检出，不是仓内输入。它们不能启用窄路径，保持全局。
3. 启用前提（每个套件都要满足）：
   - 声明覆盖：干净导出上 strace 的读集 ⊆ `files` ∪ 声明树内的 git 跟踪文件；零缺口。
   - 名单外读取为零，或已在声明中覆盖。
   - 守卫（`guards`）审计通过，代码树的 .py 也在守卫中。
   - 每次声明变更后，重做 strace 并核对（可写成派生检查：改读集文件 → 对应套件必须重核）。
4. **发布整闸仍一律全局**：release 队列不得使用窄指纹；窄路径只用于日常开发队列。
5. 修补顺序：先补 rowcov-lex（`tests/sourceflat.py`）、k2b（`tests/k2bcheck.py`）、com-*（`tests/bound`、`tests/bound.c`）等小缺口；再处理 decision-ledger 等大缺口；外部语料类不处理。

## 五、推荐

- **不启用**：目前任何套件都不满足第四节的启用前提，且收益只有 0.1–0.3%。
- **先试点（完成声明核对之后）**：`c99-ledger`、`ccinterop`。两者读集小而完整，声明已覆盖。试点的价值主要是验证窄路径机制本身，而不是省时间。
- **不试点**：`rowcov-*`（声明过宽、收益几乎为零）、`tools-*`、`realprog`、`com-tools`（外部语料）、`decision-ledger`、`exittable`、`precommithook`（声明不完整）。
- docs、ledgercheck 需先 strace 实测，本笔未测，不据此推荐。

## 六、方法与边界

- 读集：批次 05–10 的 strace（干净导出 1d9cb115、578ec1cc），`openat` 成功且为仓内普通文件的路径。
- 声明：`tests/gatedeps.json` 当前版本，`trees` 用 `git ls-files` 展开。
- 回放：`tests/affected.py --rev`（只读）；候选跳过数用同一套声明计算。
- 本笔不改 `tests/k2b-whitelist.tsv`、不改队列或门禁行为。
