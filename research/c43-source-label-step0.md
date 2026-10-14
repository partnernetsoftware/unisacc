# 0.0.43 逻辑源名：第 0 步现状记录与设计（只读，不动产品）

标签：**[实际跑过]** = `tests/sourcelabelcheck.cx`（PRODUCT = 0556ea9f 候选，`/tmp/c43-forward-paths/final/unisacc-next.com`，freshness 已核）；**[读源码]** = file:line。
检查未入闸，登记前不提交（见 §5）。

## 1. 现状实测 [实际跑过]
同一源（两个 basename 相同的文件，分别在 `dir-a/src/` 与 `dir-b/elsewhere/`，绝对路径输入），15 个用例：

| 探针 | 模式 | 因物理目录不同而不同的字节 |
|---|---|---|
| probe_file（`puts(__FILE__)`） | -S | stdout（.str 中的路径） |
| probe_file | -run | stdout（打印的 __FILE__） |
| probe_file | -c -o / -o / -c -MF | 镜像/目标文件中的路径字符串 `.str S0 "…/probe_file.c"`；-MF 依赖文件中的源路径 |
| probe_diag（语法错误） | -S/-c/-o/-run/-MF | stderr（诊断里的文件名） |
| probe_inc（`#include "rel.h"`） | 全部 | **无差异**：相对头在两处都被找到，返回值与输出字节一致 |

结论：11/15 个用例有差异，全部差异都带有各自的物理目录名（检查里的 `carries` 列）。`#include "相对头"` 的解析本身不泄漏。rc 在两侧一致。

## 2. 泄漏点 [读源码]
- `exec/c/compiler.c:486`：`Buf srcres` 的声明，注释即为 `\0cli/source: the main source path`。
- `exec/c/compiler.c:704–708`：argv 原样路径放进 `\0cli/source` 资源；E2 用它命名 `__FILE__` 与诊断。这是 probe_file 与 probe_diag 差异的来源。
- `exec/c/run.c:759–761`：`-run` 路线的同名资源 `\0cli/source` 直接用 `src` 字符串。
- `exec/c/compiler.c:827–833`：多单元路线把 `sources[j]` 的路径字节原样写进 units 输入（4 字节长度 + 名字）。
- `exec/c/compiler.c:515、578`：源文件从 argv 原样收集，不规范化。
- `exec/c/fwdincrement.c:18–19`：转发路线按 `\0cli/source` 名字找槽位，改写时同样依赖原始路径。
- `exec/c/compiler.c:107–139`（`deps_name`、`deps_write`）：`-MF` 依赖文件写原始路径，这一层在宿主侧，不进核心。
- `exec/c/compiler.c:128–129、641、924`：输出名与 -MD 依赖名从 basename 派生（只看最后一段，不泄漏目录）。

## 3. 逻辑源名的编码建议
1. **核心只看到逻辑名，逻辑名是显式字节输入**：`\0cli/source` 的值由适配器给出，而不是 argv 的原样路径。
2. **规范化规则（建议）**：
   - 相对路径：去掉前导 `./`，合并重复 `/`，保留用户写的相对段（`src/a.c` 仍为 `src/a.c`）。
   - 绝对路径：不进核心，逻辑名取 basename，或取“相对于当前目录的最短写法”（需裁定，见 §4）。
   - 逻辑名不得含宿主前缀（`/tmp/…`、盘符、符号链接的解析结果）。
3. **物理路径只用于宿主读字节**：`source_read(sources[j])`（compiler.c:775、816）保留，但读完即丢弃路径。
4. **引号 include 的搜索目录**由适配器输出为显式资源：逻辑名的目录部分 + 物理目录只在宿主侧映射，不让核心看到物理目录。probe_inc 说明当前的相对头解析是正确的，这一点要保留。
5. **多单元框架**（compiler.c:827–833）：框架里的名字改用逻辑名。

## 4. 哪些用户可见行为会变（需主人裁定）
| 行为 | 现状 | 建议 | 裁定点 |
|---|---|---|---|
| `__FILE__` | 绝对路径输入时打印绝对路径 | 打印逻辑名（相对写法或 basename） | 绝对路径输入的 `__FILE__` 是否改变？ |
| 诊断中的文件名 | 原样路径 | 逻辑名 | 诊断前缀是否改为用户写的相对名或 basename？ |
| `-MF`/`-MD` 依赖文件 | 原始路径（宿主写） | 保持原始路径（宿主侧，不进核心） | 依赖文件是否需要相对化（便于可移植构建）？ |
| 镜像与目标文件字节 | 含源路径字符串（probe_file） | 只含逻辑名 | 同一源在不同目录的输出字节将相同，这是否可接受？ |
| 转发（-run 宿主转发）路线 | 按原始 cli/source 找槽位 | 按逻辑名 | 无用户可见影响，仅内部 |

cc 的 gcc 行为是“`__FILE__` 为命令行写法”；逻辑名方案与之不同。这是主人要决定的第一件事。

## 5. 登记与提交（需窗口）
- `tests/sourcelabelcheck.cx` 未跟踪。script-inventory 会判 orphan，必须先登记进 `tests/ungated-checks.tsv`。
- 该文件工作区有他人未提交的登记行（hostenvcheck、librarycontract），本人不能只提交自己的一行而不带上他人的改动。**请求：给一个窗口，或由登记方加入下面这一行**：
  `tests/sourcelabelcheck.cx	record	Source-label record: same source in two physical dirs; 11/15 differ, all carry their dir name	Not a gate; 77 = record complete, 1 = harness error; step 0 for the logical source name protocol`
