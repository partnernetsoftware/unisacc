# X3 Darwin预检窄修：真实三probe成功，完整检查尚红

结论：去Python PATH下，真实Mach-O arm64 G1的 `probe include`、`probe entry`、`probe use` 全部实际退出0，已越过旧ELF头拒绝并真正执行构造器。完整配套检查最新仍 `74 checks / 9 failed`，不能记全部通过；九红由此前未在Darwin走通的job目录快照崩溃串联引起。按父的有限定位要求，已暂停继续猜改，未提交或登记。

## 两个正式路径的当前修改

- `tools/chain-precheck.cx`：构造器及job工具共用严格编译期宿主OS/CPU判断，不依赖PATH uname。Darwin要求32字节Mach-O64头、magic、准确CPU及MH_EXECUTE；Linux要求ELF64、小端、版本、准确machine、ET_EXEC/ET_DYN及64字节头。Linux cross-probe目标与finish ELF输出合同保持。内部run上限58→55，以符合本轮每child≤55。
- `tools/check-chain-precheck.cx`：保留原全部断言；实际host CC构造false-native代替/bin/false，原job工具亦是真实宿主native；新增script/wrongCPU/foreignOS/truncated/non-executable-type五拒绝。mkdtemp结果physical realpath解决Darwin /var与/private/var别名。默认先实编一个私有controller，后续复用本次exe执行断言；不是缓存PASS。
- 父后续明确授权的试修：job `mapped`64×4400改一个同容量heap区域及64个索引指针，OOM明确拒绝、正常结束free，所有快照/容量/映射断言保留。**该试修未消除9红，不能当作根因修复。** 当前仍保留在正式工具，供父审查决定去留。

源码SHA与诊断最小件SHA见 `darwin-source-identity.tsv`。两正式.cx均被当前已装产品实编；检查本身三次默认-run均真实执行，未改产品。

## 三个真实无Python probe

本轮编译 `out/c48-native-selfhost/chain-precheck-darwin`，session10911实际exit0。PATH保持只含 `/usr/bin/git` 软链；python/python3均明确ENOENT。沿前一轮清楚标为兼容性夹具的私有会话，增加完整f069 footer容器pkg.bin及当前源码模块路径（tools/cx/ext软链和cxroot.tsv），没有增加Python、CC、uname等PATH命令。

构造器始终是实际G1字节，SHA `e5f80facc67e6b419e8824e2fc81efaa24b0f4936250e9ad7a6518c09e3de5b1`。三个独立调用都是 `env -i PATH=ABS_NO_PYTHON_BIN PRIVATE_PRECHECK ROOT CHAIN OUT probe NAME`，外bound50；内部command.status均精确 `rc0 / transport0 / residue0`。

| probe | 实际终态 | 证据 |
|---|---|---|
| include | exec直接exit0 | probe-include.tsv；真实G1执行相对/绝对头及宏值预处理 |
| entry | session61691 exit0 | probe-entry.tsv；真实G1编译entry-build交叉ELF |
| use | exec直接exit0 | probe-use.tsv；真实G1模块导入编译交叉ELF |

entry/use输出目标仍为lnx/x86_64，不能称这些ELF产物在Darwin原生运行。这里证明的是Darwin原生构造器实际执行Linux交叉probe；本次未执行所产ELF。兼容夹具不是完整131作业正规init，未冒充完整预检或N22。G2/G3原不动点证据没有重复构造。

## 完整pairedcheck真实失败及有限诊断

- 首轮session32896 exit1，74/11红；其中两红为TMPDIR别名导致根内绝对路径被判根外。physical realpath修后两红消失。
- 第二轮session36154 exit1，74/9红；最新heap试修第三轮session11366 exit1，74/9红。最新完整stdout/stderr复制到 `darwin-check.tsv` / `darwin-check.stderr`。新格式五负控、真实native构造器执行、权限及ENOTDIR检查均通过；未删九个红断言。
- 单独当前私有controller的job gen-case在bound8中实际exit139；新作业目录已创建、command.status未产出，故后继失败状态/复制链gen/tbl/net的断言连锁失败，不是可忽略的预期拒绝。
- 私有插桩副本 `precheck-instrument-tree.cx` 输出依序 `JOB:start/native/mapping/protected/snapshot/tree1`，在 `tree_snapshot(recorded)` 内深度0、1、2、3的真实目录排序均完成；深3首叶 `gen-case` 排序后崩溃，未到tree2。实际exit139，证据 `out/c48-native-selfhost/instrument-tree-job.{log,stderr}`。只对小夹具打边界点，没有逐产品事件插桩或全片重试。
- 大stack最小件 `large-stack.cx` 默认-run exit0；含同64×4400数组与np_buf append的 `large-stack-buffer.cx` session68561 exit0（bytes1900）；含真实字符串指针数组和qsort callback的 `qsort-probe.cx` 修正模块/名字后的session22131 exit0（a b）。这些结果均未复现产品缺陷，**大frame寻址只能列疑点，且heap试修也未修复，不得记确定根因**。
- lldb实际拒绝非交互调试许可，未请求扩权；未据此判语言根因。未刷新产品、放宽预算、删快照断言。

下一动作：父按真实首叶后边界对 `tree_snapshot` 的path/stat/mtime/cx_fmt/np_puts及递归上下文做更小抽象诊断，再分派修复；在完整74检查绿前，不将本轮工具改动标为已完成门禁。既有host-C引导、watchdog构造、外部冻结模型包与Python来源债仍按前报告逐项保留，三个无Python probe不外推为依赖清零。
