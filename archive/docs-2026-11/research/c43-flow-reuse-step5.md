# 0.0.43 第 6 步第六刀：补实测读取声明（ledgercheck、docs、subtract-safety）

标签：**[实际跑过]** = strace `-f -e openat`（同一批次的 `st-*.txt`）；**[计算]** = 按队列同样的展开（`tests/gatequeue.py:385–410`：files ∪ trees 经 `git ls-files` 展开；全局名单 `:186–189`）对探针文件做的成员判定。

## 1. 声明内容（只写 strace 实测读到的）
| 套件 | files | trees | 依据 |
|---|---|---|---|
| ledgercheck | `tests/ledgercheck.py`、`src/version.h`、`archive/plans/roadmap-0.0.x.md`、`archive/plans/v0.0.41.md`、`archive/plans/v0.0.42.md` | 无 | 实际打开的 3 个 plans 文件（版本由 version.h 决定） |
| docs | `tests/docs.sh`、`prd.md`、`spec.md`、`research/model-bytes.json`、`research/unisacc-paper.md` | 无 | 实际打开的文件 |
| subtract-safety | `tests/subtractsafety.py`、`prd.md`、`spec.md`、`.gitignore`、`.gitattributes` | `apps`、`bench`、`docs`、`plans`、`research`、`release`、`scripts`、`.github` | 它检查“全部已跟踪文件”（脚本注释）；实测打开的顶层目录即上述集合（`archive/`、`ujs/` 未打开，故不声明） |

声明写入 `tests/refresh_gatedeps.py`（末尾新增一段，注释注明依据），由 `refresh_gatedeps.py` 生成 `tests/gatedeps.json`。refresh 同时按 HEAD 重算了其它套件的守护与复核树摘要（compilercheck 的 exec/src/unisa、tape-reader 的 run.c、script-inventory 等的 prd/spec 守护），这些是 HEAD 变化带来的正常重算，不是本刀的内容。

## 2. 指纹成员计算 [计算]
探针文件与声明后的结果（“global”表示在全局名单内，所有套件都会变）：

| 探针 | 全局名单 | 通过声明影响的套件 |
|---|---|---|
| `archive/plans/v0.0.42.md` | 否 | ledgercheck、subtract-safety，及既有的 script-inventory、publish-order、gate-infra |
| `plans/v0.0.43.md` | 否 | subtract-safety，及既有的三个（**ledgercheck 不受影响**：实测读取只到 v0.0.42，见 §3） |
| `prd.md` | 否 | docs、subtract-safety，及既有的 script-inventory 等 |
| `spec.md` | 否 | docs、subtract-safety，及既有套件 |
| `research/model-bytes.json` | 否 | docs、subtract-safety，及既有套件 |
| `docs/dogfood-targets.md` | 否 | subtract-safety |
| `archive/plans/v0.0.38.md` | 否 | 无（没有套件声明，实测也不读） |
| `apps/csih/agent.c` | 否 | subtract-safety |
| `src/version.h` | 是 | 全部（全局名单） |
| `tests/ledgercheck.py` | 是 | 全部（全局名单） |

结论：未进入全局名单的文件改动，只影响声明它的套件；因此本刀把 ledgercheck、docs、subtract-safety 的依赖面变得可见。注意 **`plans/` 与 `docs/` 的改动会同时影响 subtract-safety 与既有三个套件**，这是 subtract-safety 的全树性质决定的，不是错误。

## 3. 已知限制
1. **ledgercheck 的读集随版本变化**：它读哪些 plans 文件由 `src/version.h` 决定。实测版本为 0.0.42，读的是 v0.0.41、v0.0.42。版本升到 0.0.43 后，它会读 `plans/v0.0.43.md`，而该文件**未被声明**（它不在实测集合中）。因此 version.h 变化会让 ledgercheck 重跑，但**新版本的 plans 文件**要在版本升级时重新 strace 并更新声明。
2. subtract-safety 的声明是“全部已跟踪文件”的近似：实测打开的顶层目录即声明的 trees，但 `archive/` 的排除依据是实测未读，若其检查逻辑改变需重测。
3. docs 只声明了实测打开的 5 个文件；若 docs.sh 改为读其它文件，需重测。
4. 本刀未改其余 67 个静态命中作业，它们仍待逐个 strace。
