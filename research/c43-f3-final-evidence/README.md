# 0.0.43 F3 最终根产品 CONTRACT 全扫

实际完成，构建上下文 main 1a26da092bc56c5b88c38cac16d59bf9c20799d6，根与候选均为 4f0b493c7a83d73a4ae1b348c8400ea3b7eda0ea190051b54a84c712a6730eda。同源参考编译器在该树由 tests/build_ref.sh 重建。验收在隔离 scratch 完成，STRICT=1、jobs4、window50；env-pin 和完整输入摘要均保留。

最终现行 CONTRACT 全部187项执行完毕：186 PASS，0失败，1 UNVERIFIED，0待办。队列最终退出4；没有改成0。唯一未验证为 bound 的 Darwin Terminal 归属子项，原日志明确 required=Darwin；同套件的本机退出/信号/派生进程预算与 Linux 清理断言已通过。按终点线 F3 的宿主例外单独列明，不作为 PASS、没有加入全局豁免，也不覆盖其 rc1/142。

十四个窗口原始日志、最终 results.json、env-pin.json、既有 exittable 输出及全套日志压缩包保留；log-sha256.tsv 可逐文件核验。尾部预算不足的尝试由原队列按原机制重新安排完整预算，不拼接旧735队列结果。首轮162绿25红的诊断记录仍独立保留。

十二条重新封存的边、`stage-contract-e1e3`、八个 `fresh-order` 模式及 units、K2b、`pipeline-cache`、`deletecheck`、`gate-infra-38` 均在这一完整队列通过。每窗口前后根产品、候选、完整收据、seed、compiler.pkg、参考编译器和导出源码的哈希一致。

本记录只结证 F3；F4全量队列与F5发布说明尚未完成，不作为外部CI、Darwin或发布验收通过的证据。
