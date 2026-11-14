# X3 Darwin预检最终窄修：74/74实际通过

结论：当前 `tools/chain-precheck.cx` 与完整 `tools/check-chain-precheck.cx` 默认-run session76230实际exit0，`CHAIN_PRECHECK_CHECK checks=74 failed=0`，外bound55；最终完整输出见 `darwin-check-final.tsv`，stderr空。此前报告的9红保留为历史失败，不再代表当前终态。

正式修改在前一轮strict宿主格式验收上，只将 `tree_snapshot` 名字所有权改为显式 `mem.heap` 的 `mm_strdup/mm_free` 配对，并完整回退无效job mapped heap试修，恢复原64×4400局部容量。snapshot原4096阵列、64MiB清单上限、前后冻结比较与全部74断言不变；没有通过删free或删断言制造绿。

先私有同递归夹具验证配对改动：`precheck-mm-private` 编译session91930 exit0，单job实际controller exit1、正确输出 `run-gen-case FAIL rc9`，并保存 `rc9/transport0/residue0`，相较原同夹具139完成了快照和失败状态传播。此后才修改正式工具并跑完整check，未刷新产品或include。

## 独立C库接口疑点记录

父指出并已实读：`include/string.h` 在Linux/macOS声明host `strdup`，而 `include/stdlib.h` 的carried malloc/free使用自有16字节header及类索引；`cx/mem/heap.cx` 的mm_strdup显式malloc+copy并与mm_free同模块配对。这是源码可核的分配族差异，不能单凭阅读判某次具体地址必然无效。

本轮单次默认-run实证：

| 最小源码（out/c48-native-selfhost） | 实际结果 |
|---|---|
| strdup-free-probe.cx：单次strdup("command.argv.tsv")后free | session94751 exit0，FREE returned |
| strdup-free-module-probe.cx：同单次行为+netpack模块 | session73760 exit0，FREE returned |
| mm-strdup-free-probe.cx：显式mem.heap同字符串复制/释放 | 直接exec exit0，FREE returned |

因此**单次最小件未复现崩溃**，不能写成“strdup/free必崩”或已证明通用ABI根因。真正可复现的差别是先前已保存的递归snapshot夹具：host-strdup/本地free原路径139，同字段/同容量/同断言的mm配对路径正确传播rc9；完整check再74/0。源码与真实SHA见 `allocator-pair-identity.tsv`，所有最小源/stdout/stderr在out保留，供父另立产品缺陷调查，产品头本轮未改。

## 去Python与来源边界

前一轮真实G1的include/entry/use三个去Python PATH probe均已exit0，且内部状态为rc0/transport0/residue0。当前最终变化仅在job目录快照分配配对与无效试修回退，不影响三probe分支，按父授权不重复同输入。旧probe材料、构造器SHA及PATH仅git清单仍适用；它们仍是明确Linux交叉probe，不能称所产ELF在Darwin原生执行。

本轮完整pairedcheck的native_fixture使用host CC构造小夹具，私有controller由已装产品实编一次复用；这不是host-C依赖清零。固定外部模型包、引导产品与系统库依赖仍按原报告保留。完整131作业/N22未运行。

最终仅两正式工具路径变更由父集成，未修改注册表/PRD/计划、产品src/cx/seed/exec/include，未提交。所有句柄均终态。
