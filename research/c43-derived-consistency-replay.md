# 派生一致性：历史回放（只读，工具 release/tools/derivedcheck.py）

方法：对每笔历史提交 C，取父提交 P 的干净导出（`git archive P`），覆盖 C 中改动路径的内容（`git show C:path`），
按 `release/derived-map.tsv` 映射出的套件在导出里运行其 gatedeps 命令。根产品只通过符号链接只读引用，不复制。
工具不写检出，临时目录在 /tmp 并在结束时删除。

| 提交 | 改动（派生相关） | 映射出的检查 | 结果 |
|---|---|---|---|
| 0571eb62 `unisa/tape.py` | 派生 | facts-export、tapebin-shape、abi-manifest | **facts-export rc1：2 张表不一致**（tapebin-shape、abi-manifest rc0） |
| 9ed67c03 `src/front_pp.c` | 派生 | facts-export、abi-manifest | **facts-export rc1：1 张表不一致**（abi-manifest rc0） |
| 926a8925 `src/front_pp.c` | 派生 | facts-export、abi-manifest、script-inventory | **facts-export rc1：2 张表不一致**（新增 tests/iquotecheck.cx 的 script-inventory rc0） |
| 665dd7e9 `exec/c/libunisacc.c|.h` 等 | ABI 源 | abi-manifest、script-inventory | **abi-manifest rc1：提示“commit the manifest with the change”** |
| df1ce7ed `exec/c/compiler.c`、`fwdincrement.c`、`fwdpaths.h` | ABI 源 | abi-manifest、script-inventory | **abi-manifest rc1** |
| 9fee770a（纯 research） | 仅 research 文件 | 无 | **不追加任何检查**（rc0） |

结论：六笔中，要求的五笔都被报出（facts-export 三笔、abi-manifest 两笔），纯研究提交不追加检查。

注意：
- 新增的 `script-inventory` 只在测试脚本新增或删除时追加（926a8925、665dd7e9、df1ce7ed 都新增了 tests/*.cx）。
- abi-manifest 的 `--check` 读取导出里的源文件，根产品只是执行器；因此符号链接到根产品并不让结果依赖根产品是否新鲜（经 `tests/abimanifestcheck.cx` 的实现确认，只读源码）。
- 该回放只证明工具能报出这些不一致；它不证明修复后会通过（修复后的通过需要重新生成派生文件，属于各自的提交）。
- 工具与映射表未接入 `release/tools/commitgate.sh`（第 4 步等前三步验证完再定）。
