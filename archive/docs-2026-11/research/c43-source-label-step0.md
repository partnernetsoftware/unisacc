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

结论（绝对路径输入）：11/15 个用例有差异，全部差异都带有各自的物理目录名（检查里的 `carries` 列）。`#include "相对头"` 的解析本身不泄漏。rc 在两侧一致。

**相对路径输入（cc 的对照，本检查第二遍复现）[实际跑过]**：同一 f.c（`puts(__FILE__)`）放在两个临时目录，各自 `cd` 后以 `f.c` 调用。15 个用例**全部无差异**（stdout、stderr、输出文件逐字节相同；rc 相同）；`-S` 输出 sha 两侧均为 `cfa5303e…`，`-run` 都打印 `f.c`。唯一例外是 `-c -MF` 依赖文件，且那是检查自己把绝对 `-o` 目标写进依赖文件造成的，第二遍已改为相对 `-o`/`-MF`，依赖文件也变为无差异。

因此：**泄漏只发生在用户自己写绝对路径时**。物理目录是命令行上的显式输入，不是宿主状态的泄漏。

## 2. 泄漏点 [读源码]
- `exec/c/compiler.c:486`：`Buf srcres` 的声明，注释即为 `\0cli/source: the main source path`。
- `exec/c/compiler.c:704–708`：argv 原样路径放进 `\0cli/source` 资源；E2 用它命名 `__FILE__` 与诊断。这是 probe_file 与 probe_diag 差异的来源。
- `exec/c/run.c:759–761`：`-run` 路线的同名资源 `\0cli/source` 直接用 `src` 字符串。
- `exec/c/compiler.c:827–833`：多单元路线把 `sources[j]` 的路径字节原样写进 units 输入（4 字节长度 + 名字）。
- `exec/c/compiler.c:515、578`：源文件从 argv 原样收集，不规范化。
- `exec/c/fwdincrement.c:18–19`：转发路线按 `\0cli/source` 名字找槽位，改写时同样依赖原始路径。
- `exec/c/compiler.c:107–139`（`deps_name`、`deps_write`）：`-MF` 依赖文件写原始路径，这一层在宿主侧，不进核心。
- `exec/c/compiler.c:128–129、641、924`：输出名与 -MD 依赖名从 basename 派生（只看最后一段，不泄漏目录）。

## 3. 设计结论：保持 gcc 语义（推荐）
产品今天的语义就是“逻辑源名 = 命令行写法”，与 gcc 一致。因此不改产品，而是：
1. **把 `\0cli/source` = argv 原样写法登记为核心的显式输入**：它在身份表里占一行，并在 spec 中写一句（“源名以命令行写法进入核心，物理目录只在用户写了绝对路径时出现”）。
2. **封存与对拍一律用相对路径或固定逻辑名调用**。pp-literals 封存那次的随机 source-label 泄漏，是封存工具自己用了随机绝对临时路径调用造成的；修法在 `tests/` 侧，不在产品侧。
3. 宿主读字节的路径（`source_read`，compiler.c:775、816）与依赖文件（compiler.c:107–139）保持现状，它们不进核心。
4. 引号 include 的解析保持现状（probe_inc 无差异）。

备选（不推荐，列出以备裁定）：
- 核心只见 basename：会丢失用户写的子目录信息，诊断与 `__FILE__` 变化面最大。
- 相对化绝对路径（相对当前目录）：仍依赖调用时的工作目录，且同一绝对路径在不同 cwd 下得到不同的核心输入。

## 4. 四个裁定点（保留，推荐改为“保持现状”）
| 行为 | 现状（即推荐） | 备选 | 裁定点 |
|---|---|---|---|
| `__FILE__` | 打印命令行写法（绝对输入则绝对） | basename 或相对化 | 绝对路径输入的 `__FILE__` 是否保持？ |
| 诊断中的文件名 | 命令行写法 | basename | 诊断前缀保持命令行写法？ |
| `-MF`/`-MD` 依赖文件 | 宿主写原始路径 | 相对化 | 依赖文件是否需要相对化？ |
| 镜像与目标文件字节 | 相对输入时不含目录；绝对输入时含绝对路径 | 只含逻辑名 | 绝对输入的输出字节含路径是否接受？ |

转发路线（-run 宿主转发）按 `\0cli/source` 找槽位：无用户可见影响，保持现状。

## 5. 登记与提交
- `tests/sourcelabelcheck.cx` 已在 `7c70ccd8` 中登记进 `tests/ungated-checks.tsv`（record，不入门禁）。
- 检查包含两遍：绝对路径（上表）与相对路径（15 用例，0 差异，期望值）。退出 77 表示记录完成，1 表示检查自身出错。
