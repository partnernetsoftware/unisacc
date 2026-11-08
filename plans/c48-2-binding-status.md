# c48-2消费绑定与资格边界

当前结论：接口修复实际验证通过；尚无本轮Linux全批会话资格。`plans/l-tests-materials-prepared.tsv`的16 PREPARED/7 NOT_PREPARED与十INFRA历史PASS仍各绑原根，不回填旧s9，也不折算为当前候选的NOT-AVAIL减少数。

## host-cell（实际跑过）

原检查允许缺SHA时读取sidecar，批次未声明候选摘要。现在必须显式传`ROOT CANDIDATE_SHA`，摘要为64位小写hex，核实际产品digest；Linux/x64匹配才PASS、错误摘要仍MISMATCH/exit1。其他宿主先核摘要再NOT_RUN/exit3，摘要错则拒绝。陈旧或缺失sidecar不影响显式绑定，版本仍由limits.tsv提供。

注册正批`host-cell`声明`candidate-sha:sha256`，参数`{root};{candidate-sha}`。现行元数据禁止commit批带材料，所以正批置daily；无材料的独立`host-cell-binding`保留commit。没有把正批删掉或把外宿主NOT_RUN改成PASS。

| 验证 | 真实句柄/终态 | 结果与边界 |
|---|---|---|
| 最终直接默认-run配套检查 | 37895 / exit0 | 19/0，osx/arm64，linux_positive=NOT_RUN |
| 正式host-cell-binding | 55993 / exit0 | PASS/rc0/transport0，原19断言全部通过 |
| Linux/x64分支交叉编译 | 79121 / exit0 | 实际ELF64 x86-64；未在Linux执行 |
| 直接host-cell CLI | 41807 / exit3 | NOT_RUN，实际产品SHA f069…，不是Linux原生证据 |

产品完整SHA为`f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82`。`tests/host-cell.cx`完整SHA为`1798e819be6869f3b95d06430719180fe6edebc0e29f3c94742721f2801d4a76`，配套check为`7bc7af1d82286f62b6229c617c662d14aca734e90cb79c68a3e83ff1dfa624bb`。负控覆盖短/长/非hex/大写摘要、缺参、多参、无效根/产品类型、错摘要、缺/旧sidecar、含空格分号的字面根路径、实际digest输出及产品未变；短摘要先校验长度避免越过NUL。

首次foreign分支把未定义平台宏当表达式导致实编错误（84236/exit2）已保留，改用既有sys.hostid后新作业通过。首次登记时host-cell仍在commit且带材料，被schema明确拒绝；register作业35233外bound60退出142，两条新登记行尚存。随后修正正批tier，明确核对两行未重复，再由21482/exit0验证全表metadata_valid=1，正式55993验证新批。没有将失败登记算REGISTERED或复用其结果。新表共679批、667 IMPLEMENTED、12 PLANNED，executed_batches=0/NOT_EXECUTED是元数据清单输出，不是全批测试结果。

原日志及独立报告在`out/c48-c48-2-infra-audit/host-cell-*`；正式批仅验证接口合同，不授予c48-2全批资格。

## RI 20秒编译窄核（实际跑过）

同一本机f069，工具`tools/ri-material.cx -- --build ROOT NEW_OUT tests/cases/ri-cli.cx`一次新私有构造49392/exit0、stderr空；原内部bound20未改，输出严格ELF64/x64、前后身份复核及EXCL安装封印均实际通过。`runtime-native`为270272B，SHA`4394b7d8ab25fba8b3dcad81121f340acdeae58b907fdde9fdd62171a3eeddd1`，材料位于`out/c48-ri-native-phase/native-attempt/`。这是Linux交叉ELF的PREPARED，不是消费者PASS；没有在Mac执行ELF或冒充Linux原生资格。各相位精确耗时UNMEASURED。

原T2A-NATIVE-PHASE-20S失败绑定另一根/候选，仍保留KNOWN-GAP和原预算；这次未复现不能证明根因或关闭它。未因一次成功重复构造、造PASS缓存或提高时限。后续必须取得当前Linux候选的ROOT/SESSION/MAP与原始失败输入，按相同身份验证后决定是否需要拆分。

## 下一必要证据

本机`ssh lnx`实际无法解析，私有当前ROOT/SESSION/MAP未定位。冻结表的candidate_sid声明只记原串，未证明其哈希类型或与实物的对应；本机实算源ident与APE摘要分账。取得当前Linux执行入口后，以新完整会话逐项验证23个材料条目和10个INFRA；注册表拆成多mode的批次须全覆盖，未知保持未知，不把上述开发证据迁成新候选资格。
