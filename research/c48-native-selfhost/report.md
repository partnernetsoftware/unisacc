# X3：osx/arm64原生不动点与去Python预检

本轮已完成固定外部包的44根原生编译器裸片自举：G1→G2→G3均由Mach-O arm64直接执行；G2/G3完整memcmp0，均3,071,458B，SHA256 e5f80facc67e6b419e8824e2fc81efaa24b0f4936250e9ad7a6518c09e3de5b1。两个真实whole-o子阶段分别30.269952s、30.419658s，外bound55均退出0。原始身份、两代收据与包span全字节核对见[初始实测](initial-report.md)及fixedpoint.tsv。

去Python PATH只含git的三个真实G1 probe include/entry/use均退出0。这些probe按既有合同生成Linux交叉输出，不能称其ELF在Darwin上运行。chain-precheck现在严格检查真实宿主OS/CPU的构造器及任务工具，保留脚本、外宿主、错CPU、截短、权限与ENOTDIR拒绝；完整正式批74/74通过，transport0、candidate f069d4c9…，见[最终窄修实测](darwin-precheck-final.md)及darwin-check-final.tsv。

曾经的ELF硬编码拒绝和递归快照139没有覆盖成PASS：[旧报告](darwin-precheck-report.md)保留74中9红。产品编译的同源控制器在首叶free处139，宿主C控制能完成任务并正确传播子rc9；换用已有mem.heap的mm_strdup/mm_free同族配对后，原完整递归夹具与全部74断言通过。大帧、qsort、名字槽迁heap等猜测均无效；单次strdup/free最小件也未复现，因此不宣称已独立证明通用allocator ABI根因。未删断言、缩容量、跳快照或加时。

依赖边界保留：固定外部NN/头资源包、引导产品、宿主系统库、原生watchdog/工具构造的host-C来源及测试中host-C小夹具；NN和旧编排的Python构造来源没有因去PATH实验而消失。完整131任务与六片APE/N22没有本轮重建。这是固定包裸片不动点和已执行预检probe的证据，不是完整源头引导或依赖清零。

本目录只归档小文本证据，产物及完整诊断日志留out/c48-native-selfhost。各旧报告中的未提交/旧红仅指当时阶段；最新结论以本页、最终窄修报告和正式批日志为准。
