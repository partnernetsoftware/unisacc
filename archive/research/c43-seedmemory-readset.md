# c43 seedmemory 参考键收窄：被测路线实际读取的文件（2026-10-14）

性质：**实测**（strace 跑过），不是设计。RSS 未重测（见第 5 节）。

## 1. 方法

- 基线：`git archive 498fe26a`（HEAD，干净导出）到 scratchpad，不用共享工作树。
- 30 条 Python 路线：`python3 -I exec/build/gen.py STAGE OUT ARGS`，STAGE/ARGS 取自 `tests/seedgencheck.sh` 的 spec 表；30 条 C 路线：`seed/gen.c` 用 `cc -std=c99 -O2 -Iseed` 编出，同参数运行。
- 每条 `timeout 55 strace -f -qq -e trace=openat`，串行。60 条全部 rc=0。
- 读取集 = openat 成功、位于导出根下、是普通文件的路径；去掉 `__pycache__`/`.pyc`；相对路径按 cwd 归一。
- 工具：`readset.py`（scratchpad，不入库）。

**为什么不用共享工作树的 trace。** 第一轮 trace 在共享检出上跑，`e3`、`warnparse`、`errorparse` 在 `exec/parse2/librarycontractimports-result.tsv`（工作树未提交的修改，非本刀所有）上 rc=1，读取集是残缺的，弃用。

## 2. 家族划分

- `gen`：`seedgen`、`seedgen-2/3/4` 的全部 30 条路线的并集。
- `parse2`：`requirement()` 里 parse2 家族的两条路线，`x`→`e3`、`errors`→`errorparse`，两者并集。
- `e3` 同时属于两个家族；`warnparse` 只属于 gen（它在 `seedgen-4` 里）。

| 路线 | 家族 | Python 读取 | C 读取 |
|---|---|---:|---:|
| e2 | gen | 69 | 62 |
| e1 | gen | 21 | 20 |
| e3 | parse2 + gen | 333 | 326 |
| e4 | gen | 34 | 31 |
| o1 | gen | 22 | 17 |
| prune | gen | 7 | 3 |
| nativeabi | gen | 27 | 22 |
| enc-elf | gen | 90 | 87 |
| enc-macho | gen | 100 | 97 |
| enc-pe | gen | 96 | 93 |
| arm-elf | gen | 87 | 83 |
| arm-macho | gen | 97 | 93 |
| arm-pe | gen | 93 | 89 |
| lower-lnx-x | gen | 92 | 89 |
| lower-lnx-a | gen | 94 | 91 |
| lower-osx-x | gen | 92 | 89 |
| lower-osx-a | gen | 94 | 91 |
| lower-win-x | gen | 89 | 86 |
| lower-win-a | gen | 91 | 88 |
| obj-lower-x | gen | 99 | 96 |
| obj-lower-a | gen | 101 | 98 |
| obj-enc-x | gen | 107 | 104 |
| obj-enc-a | gen | 108 | 105 |
| tokenpp | gen | 51 | 50 |
| tokenlex | gen | 14 | 13 |
| warnlex | gen | 23 | 22 |
| warnparse | gen | 372 | 368 |
| warnunits | gen | 50 | 47 |
| errorparse | parse2 + gen | 352 | 348 |
| warnpp | gen | 69 | 68 |

并集：gen **770** 个文件，parse2 **355** 个文件（全部在 `exec/` 下，无 `weights/`，无 `exec/librarypartition/`）。

C 读取是 Python 读取的子集（C 独有 0 个）。Python 独有 11 个，是参考实现自己的源码与部分表（`exec/assemble.py`、`exec/finite_rules.py`、`exec/build/*.py`、`exec/facts/load.py` 等），仍须纳入键，因为它们决定 Python 参考的输出。

## 3. 旧键漏掉的真实输入（本刀的主要发现）

旧键 `26af43a34bb53b95`（gen）与 `833d3f6504128e12`（parse2）用的是 shell glob。与实测读取集对拍：

- gen 实际读取、旧键**没覆盖**的 25 个文件：`exec/facts/load.py`、`exec/modelinput-*`、`exec/modelbindings-*`、`exec/modelcandidates-*`、`exec/modelgraphequality-*`、`exec/modelsignature-*`、`exec/layoutprovenance-*`（顶层 `exec/` 下，不在子目录里，`exec/*/*.tsv` 匹不到）。
- parse2 实际读取、旧键**没覆盖**的：上述顶层表，以及 `exec/parse/*`（parse2 键只含 `exec/parse2/*`）。
- 旧键覆盖但**未被读取**的：gen 208 个、parse2 308 个（如 `exec/c/compiler-routes.tsv`、`exec/enc/hostbridge.tsv`、`exec/parse2/…` 之外的表）。这些改动会让旧键变化，属于过度敏感，不是漏检。

结论：旧键**既漏检又过敏**。它保护不了真实输入，也会因无关文件变动而失效。RSS 证据的键应当只由实际读取的文件决定。

## 4. 新键

- `tests/seedmemory-reads.tsv`：`family<TAB>path`，每族一行一个文件，共 1125 行（gen 770、parse2 355）。
- `reference_key(family)`：按列表顺序，对每个文件累积 `path + NUL + sha256(内容)`，取 sha256 前 16 位。文件缺失返回 `None`（不再静默）。
- 键值（干净 HEAD 导出上计算）：
  - gen：`8103012af730a59f`（旧 `26af43a34bb53b95`）
  - parse2：`15718a9bbe4d5fba`（旧 `833d3f6504128e12`）
- `REFERENCE_KEYS` 已更新为上述值（`tests/seedmemory.py`）。

### 4.1 证明（在干净导出的副本上计算）

| 操作 | gen 键 | parse2 键 |
|---|---|---|
| 基线 | `8103012af730a59f` | `15718a9bbe4d5fba` |
| 修改 `exec/librarypartition/librarypartition-byte.tsv`（未被读取） | 不变 | 不变 |
| 修改 `exec/lower/armfuse-byte.tsv`（被 gen 读取） | `2585960fa41dba8e`（变） | 不变（parse2 不读 lower） |

`tests/seedmemorycheck.py` 新增 `test_reference_key_reads_only_listed_files`，复现上表。在干净 HEAD 导出上 15 项全部通过。在共享工作树上 `test_gate_low_memory_exits_before_generator` 两项失败，原因是工作树的 `exec/parse2` 未提交修改使键不等于固定值，这是 `requirement()` 的 `generator inputs differ` 分支在正确工作，不是回归。

## 5. RSS 是否需要重测

规则：只有路线输入在 346ba62e 之后变化时才重测。

- 对两个家族的读取集求 `git diff --name-only 346ba62e HEAD`：交集为空，gen 0 个、parse2 0 个。
- 因此 **不需要重测任何路线的 RSS**，只更新键值。
- 注意：旧键漏掉了若干输入，所以"键未变"此前不能证明输入未变。这次用实测读取集对 346ba62e..HEAD 做了核对，结论成立。

## 6. 局限与待办

- 读取集是**静态列表**。若将来某路线开始读取列表外的文件，键不会察觉。补救：每次改 `gen.py`/manifest 时重跑本 trace，并让派生映射覆盖读取集（第 7 节）。
- `tests/seedmemory.py` 的 parse2 家族 `DEFAULTS` 包含 `locations`、`warnings`，但 `C_RSS` 没有对应路线，自检报 `unmeasured C flags: locations`。这是既有行为，本刀未改。
- 派生映射（`release/derived-map.tsv`）：需为读取集中的每个文件加一行，映射到 `seedmemory` 检查；登记窗口由 cdx 占用，**本刀未动**。`seedmemory` 目前是 `tests/gate.sh:198` 的 `job seedmemory python3 ./tests/seedmemorycheck.py`，`gatedeps.json` 无该名的显式条目。
- 共享工作树上 `exec/parse2/*` 的未提交修改，会让工作树的 trace 与键失真；本刀的所有结论都基于 HEAD 导出。

## 7. 待办（等登记窗口）

1. 派生映射：把 `tests/seedmemory-reads.tsv` 的每个路径加入 `release/derived-map.tsv`，映射到 `seedmemory`（或按目录合并为 glob，代价是多跑）。
2. 提交 `tests/seedmemory.py`、`tests/seedmemorycheck.py`、`tests/seedmemory-reads.tsv`、本笔记，用 `release/tools/commitgate.sh -- 路径`。
