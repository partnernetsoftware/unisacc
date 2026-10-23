# F1 后的边封印与阶段合同夹具

本次只更新与已落地根产品／头文件对应的测量数据，不改变执行器、gold 表、正负断言或豁免。原 F3 的红日志保留，最终完整 CONTRACT 队列另行结证。

## 十二条边

在同一 `8377aee0bc` 源码、探针 `examples/fib.c`、目标和 gold 表上，先以私有只读挂载替换为原 `8b82f76d` include 树，再使用当前头文件重放。原 include 树每个文件的 Git blob 均与该提交核对；`old-headers-tree.tsv` 保存清单。十二个旧头文件重放体全部与原 asks.tsv 主体逐字节相同，证明本次差异来自 F1 头文件。

`edges.tsv` 保存新旧主体 SHA256、键数及差异计数；压缩文件保留实际完整新旧重放。没有删除键，共有键的答案零变化；只有问次变化和 `added-keys.tsv` 列出的八个新增键。新增的 sizeof 查询对应新库代码；两个返回 illegal 的指针加法查询是原参考 `binary()` 在处理指针相等比较时额外查询 `+` 以决定运算符号性的路径，并非新增指针加法源码。其问次分别与相应 `==` 查询一致，旧 gold 答案原样保留。

经以上对照后，使用既有 producer 双次记录且要求字节一致；新产生的主体必须再次与独立审查的当前重放体比较。每叶仍需原 `.cx` 闸的回放、pin 和删行／改答案／改键三个负例通过。没有把首轮红项改写为历史绿项。

## 两个边界

旧根 SHA256 `4fbfdad2d919d20717a074bc792772be82608bb0b4de51da211e0e3ff5c43c79` 的实际 P3 解包（extent/CRC 校验）提取出 `ac5f7703…`，与旧 e3 夹具完全相同。新根 SHA256 `4f0b493c7a83d73a4ae1b348c8400ea3b7eda0ea190051b54a84c712a6730eda` 提取的 e3 模型为 `2bbfe160b7d843e3f8081a4e4abd9b43dc65c6b3c50fd71810e31ffc38411449`；候选目录的 errorparse.net 经过现有 `seed/pack.c` 的 `compact_q` 和 `binary_encode` 后与之逐字节相同。

使用匹配的 compiler.pkg 和既有 middleflowrunner 重新捕获。完整 accept/ 目录进行了双向逐文件比较，文件集合和所有字节均不变，故只替换 e3 网络和计算产生的 pin。lower 模型仍是 `3f4c80e2500315ddf9db4086e7bd7bffab0ac6c5944186a1e391581d71ea3ad5`，与提取结果完全相同，不更新其数据。原两个边界的正例、资源缺失／重复／帧错误／模型变异／reset 负例及 ABSENT/EMPTY 的 UNKNOWN 记录均须保留。
