# F1 scratch 验证凭据

实际跑过。基线 8b82f76d；七补丁 git am 后补 cx_read 八流容量边界和 enum bare 回归。本表记录 scratch 测量；F1 是否完成以 main 整体落地及进度台账为准。

旧产品 SHA256：4fbfdad2d919d20717a074bc792772be82608bb0b4de51da211e0e3ff5c43c79。
新产品 SHA256：4f0b493c7a83d73a4ae1b348c8400ea3b7eda0ea190051b54a84c712a6730eda。二者均 native ident check fresh。

原跟踪语料 652，新加 5，共 657。新旧产品各配同代头与相同测试源；编译 -S、rc、stderr 与 -run stdout/rc/stderr 六项逐文件比较。结果 628 SAME、29 DIFF；独立重算清单集合、去重、六项结论和输入 pin。普通每步 4/3 秒；两个大型工具补采每步 20 秒，原超时保留。遍历独立 fd3，子进程关闭 fd3，stdin /dev/null。

corpus-results.tsv SHA256 eaf64f842c270cd32958a6fa1c234f9b340e40e2299d08881078f5d48239d2a9。
输入 pin 1456 行 SHA256 6d75da9b5f61f52d1426781d44f06c4c0caecb7fe909d2d3c2a577768113af2d。

差异审查表是只读代码审查意见，非独立运行证据。其中 exeinfo/memmap/procview 的动态差异已追加固定输入实测：/bin/ls、三行 maps、四行进程表，三者新旧 rc0 且 stdout/stderr 相等。posix 的 Tape 相等，运行差异限单调时钟与 PID。29 项中其余变化对应已修 libc 或 enum；没有将旧非零程序统称功能通过，详见 nonzero-review.tsv。

补充证据边界：固定输入 stdout/stderr 文本相等已由只读审查复核；后补六次隔离执行均 rc0，逐次记录返回值及输出哈希，新旧三行收据完全相等（fixed-input-results.tsv）。这三例仅覆盖上述固定输入；实时映射、进程树、自映像差异仍属动态观测。difference-review.tsv 中相应行据此更新为有限范围的可解释差异，不是整套功能 PASS。

nonzero-review.tsv 是语料调用方式与既有结果的分类，不能代替 release/rulings.tsv 的套件裁定。尤其 fwrite/sim、librarywinimportscheck 的旧失败及 warn/format 的双边崩溃均未获新增豁免；若 F3/F4 出现对应套件红，须按套件名、失败类型和首错签名匹配已有裁定，否则保留 NEEDS_RULING，不得由本表推成已裁。P2 的独立 trim-smoke/runtime.tsv 记录 fopen/dup2 旧1新0；不把这份专项结果反推为29条语料里的独立函数变化。

补充实际验证：八流占满目录读取旧候选失败、新候选及经典参考/宿主 cc 通过；cx_k0 bad=0 files=18；system/popen/execvp 子环境探针模型/经典 bad=0；fopen/dup2 修剪宏旧1新0；enum bare/repeat 新接受，未完成对象/sizeof 仍具名拒绝。seedmemory 17 项自测通过，gen/parse2 键分别 1aba99c3d52243aa / 289b42a4ad9b85db；ABI manifest 523 项匹配。

E3 正式重封 16 步 rc0，目录摘要 335ff8dc8223a03ce1d1e106096a53c93103bfbf5d491f3b21d2771218566e16。
PP 正式重封四步 rc0，目录摘要 fc0a4656087ce39e1059ff7defa42bae6b57aecc876608c3c592b9a677fcbb6a，负例检查通过。采集器对齐空白修复见 ../c43-ppcapture-padding.md。

所有实测在 scratch + 隔离命名空间；先前混代头、继承 stdin 的两轮均作废。未 push，未做 Draft/Latest/签名/公开或 resume7000。F3/F4/F6 尚无本轮全量结论。
