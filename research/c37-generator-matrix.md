# group-tail 修片与完整构造矩阵（cdx，2026-10-09，实际跑过）

交接树 b8d96260 的未提交修片补 `value_json` 独占树深释放与 joined 借用容器浅释放；父验证时 errors 的 ASan 两次在 50 秒退出142，无结果，保留失败。只缩短诊断栈未解决。现同一 group-tail 模板行按原序列缓存不变结果，保持边遍历、first-seen label 编号、seq 文本intern副作用；树只解析/释放一次，spec在最后使用后释放。cc已独立只读核实所有权及缓存语义。

完整覆盖 shared七个、六 target各lower/enc十二个、modeljob十一个，共三十调用位置；sites.txt与covered.txt差集空。新源码生产O2三十项、clang19 O2 ASan三十项全部rc0、逐字节同Python、source前后稳定。回执逐项绑定源、flags、构造器二进制、参考/输出哈希与耗时；详见JSON。参考来自cc本轮新鲜A2，Python/TSV始终未改。

每个生成器受bound50约束，单次外围bound55，串行且使用同一重活锁；ASan detect_leaks=0（原配置）、quarantine64/context5。生产 errors 6.75s（旧40.58s），warnings+errors5.79s（旧33.69s）；ASan最重两项约37s/3.03GiB。此前context1仅诊断实验，真实UAF负例被检出；完整矩阵恢复context5。

矩阵通过之后才提交冻结。未把旧候选52173312或旧队列10/642记为当前验收；候选、seed、自举、全量与封存仍待本次冻结之后执行。未改测试或验收、未提高预算、未增加jobs。
