# T2：逐清单条目测试试点

## 模板

1. 从生产清单选安装行及必要的子清单，在私有临时目录装配；不重写安装规则，不改共享 exec 树。
2. 使用生产 assemble.run 与图类，传明确 flags，读取生产领域 facts。
3. 独立断言一个状态的观察键、目标和有序动作。期望不能从被测 action TSV 或整图哈希复制。
4. 同时覆盖正常路径、领域参数绑定、拒绝路径。子图可以包含外部目标，本试点不宣称独立运行整阶段。
5. 在私有清单副本改坏一个语义操作数，装配必须仍成功，同一断言必须失败；解析错误不算抓住语义突变。
6. 每阶段单独 job，10 秒内完成；gatedeps 声明清单、facts 和共用装配器依赖。

## 首批覆盖

- pp：body-manifest 的 directive-scan 行，6 条契约。TAKEB 深度表偏移、已决深度、非活动 #error 跳过、活动 #error 长度检查、超长错误拒绝、包含文件诊断边界。私有突变 TAKEB=SEENB：图仍生成，偏移断言失败。
- lex：gen-manifest 的 output call 与 dispatch template，5 条契约。逐个领域字符检查空白、标识符、数字、非法字节，另查 EOF 文本。私有突变 output-manifest 的 scan.start 从 MARK S 改为 LDI S 0：图仍生成，标识符动作断言失败。

这不是 C99 语义证明，也不取代整图等价与系统编译器对拍。后续每张清单增加至少一个独立语义契约及有效突变；不能只把整图 hash 分拆成局部 hash。

## T2b：四阶段推广

每条契约单独装配一个有效突变副本，要求装配成功而该条断言失败；不在组装后的图上篡改值。两编码器和 lower 的突变改其清单引用的 result/byte 声明行，parse2 改清单自身的输出模板行。所有副本在临时目录，不写 exec。

| 阶段 | 生产清单片段 | 独立契约及对应突变 |
|---|---|---|
| parse2 | truth-manifest 五条 output 行 | FT.double、FT.float、FN.double、FN.float、FN.integer 的规范输出及结果类型；每一行的零立即数分别变成一，五次独立断言捕获 |
| enc | gen-manifest 的 x86-procs/procs 行 | REX 位移3→4；MODRM 位移6→5；ALU 宽位1→0；LB.o 字节移位8→7；ME.66 续点 ME.rex→RET。检查有序动作/目标及合法 ALU 续点 |
| enc/arm | arm-manifest 的 armcontract/scan 行 | header ADV→COPY；EOF FINISH→FAIL；非法数字 FAIL→ENC；REG 银行→OP 银行；64位无符号十进制限值→0。检查头/EOF路径、拒绝、领域 REG 偏移、溢出守卫动作 |
| lower | data-manifest 的非 win data 行 | 字符串/BSS标志各取反；COPYLINE目标→SKIP；数据容量→extra；DEFINED 银行→RAW；FAIL目标→RET。检查种类、保序复制、领域容量/偏移和拒绝路径 |

推广命令：`python3 tests/manifestentries.py --stage parse2`（或 enc、enc/arm、lower）。每条命令由 `tests/bound.py 10` 限时，测试内部也断言阶段时长低于10秒。新增 job/依赖声明由 cc 合并；本片不改 gate/gatedeps。

预期按领域 facts 绑定，不取被测输出表的动作作为答案。架构位域常量是明确的机器编码契约。这里检查的是安装规则产生的有序动作及局部分支；不宣称这些子图能独立跑完编译，也不代替指令执行差分或C99语义对拍。
