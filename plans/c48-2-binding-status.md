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

## osx材料消费与实际映像路径（实际跑过）

loader新增规范v2收据：候选必须是ROOT/unisacc.com，实际APE目标slot解压经CRC与完整字节比较等于所用slice，候选与MODELS的完整package+footer span相同；八个成员的规范路径、完整SHA和字节数重建后与原收据全字节相等。外传manifest SHA必需，未知尾、错成员、重复slot、越界、坏CRC均拒绝。该收据是八成员consistency与实际slice/package关系证明，未声称完整sourceclosure、源码与二进制可复现编译或敌手并发文件系统安全；输入在seal/verify期间需冻结。

新接口为`units-prepare --seal-loader-bound ROOT DIR CANDIDATE SLICE MODELS TARGET VERSION`及`--verify-loader ROOT DIR CANDIDATE MANIFEST_SHA`；仅ISA消费者先接`--material DIR ROOT CANDIDATE MANIFEST_SHA`，注册同时声明loader6路径与loader6-sha:sha256。其余五类loader检查仍保留原ENV开发合同，不冒称全六类已迁移。独立17/0及正式95423真实exit0/PASS/rc0/transport0；新私有材料证明将所有TEST_LOADER_*设错也不能替换显式ISA所用材料。

osx原兼容红的实际原因是test-loader和special硬读/proc/self/exe，现L0 sy_self_path在Darwin也走此路径。工具模块toolselfpath先保持L0成功路径，否则复用现sys.ffi发现宿主实际映像API，验证容量、返回、NUL及canonical regular文件；不用argv0。Linux分支未在本机冒称实际通过，产品L0与源SID未修改，L0缺口另记pkg-gaps。

独立检查2777实际exit0/15项，正式osx平台行22231实际exit0/15项；包括伪argv0、symlink、NULL与0/-1/1/短容量及真实映像SHA。修复后正式兼容批：common87131，pp10/0；special71227为21/0；net-replacement82720为46/0；ISA46723在错ENV下claimed1/failed0，均实际PASS/rc0/transport0。test-loader-com12879仍FAIL/24中1红，仅C-adapter-UNIPP2-byte-equality-foreign-cwd：旧f069/旧外包载荷与新UNIPP2预期不合，原断言保持，未将它改成PASS。原五个identity相关失败已修，不能据局部绿声称完整当前T1流资格。

新开发材料DIR为`out/c48-c48-2-material-audit/selfpath/development-v3`，收据SHA`d52f3de458266780bf3eb64f14711ce092784d14298620895b44fe911c2de06a`；loader实际SHA`7cb96624447efa7155c9e34a69534a4b39d766d4ba048cb75adb6dcd61fb3d88`，627682B。seal56512、verify11740均exit0，只记PREPARED/VERIFIED。原四红与新逐批全文日志分留在out/c48-loader-explicit-formal和out/osx-loader-final-formal，不覆盖失败现场。

## 登记失败回滚与工具pin（实际跑过）

`register`在`--list`因既有坏元数据拒绝时仍持append/list锁，却重新取得rollback锁；修复仅在失败分支先释放本调用锁，再按原锁预算及精确两行删除规则回滚。成功路径和外部竞争的DIRTY合同保留。新独立`--metadata-rollback`在私有表植入坏行，旧源配对38439实际checker exit1、child exit3/DIRTY/两行残留；最终3986实际exit0、8/0，child exit1/ROLLED_BACK且两行删除，原坏表和coverage字节完整保留。正式登记41926实际exit0、PASS/rc0/transport0。没有改原35项断言或预算。

原`--args-check`的行尾预期漏了已存在的第14列tier；按现行schema保留完整encoded args断言，补`product TAB commit LF`，最终91026实际exit0/5项。正式register-args在1627的受影响队列中独立PASS/rc0/transport0；该队列另一个pins当时FAIL，整体exit1没有记绿。默认35项89213在本机原55秒预算内未完成、exit142，仍未记全套通过，也未擅判语义红或新增serial。旧反例、超时、原args失败均保存在`out/register-metadata-rollback/`。

T1合入后`tools/source-package-pack.cx`实际SHA为`e67953bd9c1481b21ed462e1eba7ec68df579d8ab5a494baadbdd79d44eb8a21`，`sourcepackage.cx:117`字面相同；表仍旧a0f391…，另digest_module/selfcheck已移至98/100而声明仍96/98。只同步这些实际调用位置和已核工具字节，保留两组原重复行、所有known-broken和产品pin；新loader负控的全零SHA按literal登记。最终check-pins90845实际exit0、failed0/stale30/hostdep6，六个本机缺失Linux zlib宿主依赖继续报告，不能称这些依赖已满足或来源包构造已通过。原六红完整保留，未改工具判据。表单独修正不授予新候选资格。

## RI 20秒编译窄核（实际跑过）

同一本机f069，工具`tools/ri-material.cx -- --build ROOT NEW_OUT tests/cases/ri-cli.cx`一次新私有构造49392/exit0、stderr空；原内部bound20未改，输出严格ELF64/x64、前后身份复核及EXCL安装封印均实际通过。`runtime-native`为270272B，SHA`4394b7d8ab25fba8b3dcad81121f340acdeae58b907fdde9fdd62171a3eeddd1`，材料位于`out/c48-ri-native-phase/native-attempt/`。这是Linux交叉ELF的PREPARED，不是消费者PASS；没有在Mac执行ELF或冒充Linux原生资格。各相位精确耗时UNMEASURED。

原T2A-NATIVE-PHASE-20S失败绑定另一根/候选，仍保留KNOWN-GAP和原预算；这次未复现不能证明根因或关闭它。未因一次成功重复构造、造PASS缓存或提高时限。后续必须取得当前Linux候选的ROOT/SESSION/MAP与原始失败输入，按相同身份验证后决定是否需要拆分。

## 下一必要证据

cc已裁定Linux格归远程，标PENDING-REMOTE；本机无lnx入口是预期，不做不等。所需ROOT/SESSION/MAP清单已写items C48-2-LINUX与plans/s9-session.md。本机继续osx原生和材料接口。远程T1合入前，冻结表candidate_sid仅有声明；合入后实算会话65994真实exit0，当前源ident为e586721c7767275e1b5af84fe5a3cd199600e2550fc665c26cec642c6f4c9325，已与原声明全串相等。APE仍f069开发载荷；这不证明已取得该源对应的新候选产品，两者分账。取得当前Linux执行入口后，以新完整会话逐项验证23个材料条目和10个INFRA；注册表拆成多mode的批次须全覆盖，未知保持未知，不把上述开发证据迁成新候选资格。
