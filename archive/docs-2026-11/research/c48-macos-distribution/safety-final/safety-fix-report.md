# X4新增缓冲区安全债收尾

仅修改tools/cache-retain.cx与tools/check-cache-retain.cx。管理器动态格式统一检查cx_fmt结果；动态复制检查mm_copy，实际容量分别为protected65537、active_names单槽256、Item.name256、Item.key17。固定16字节key切片保持原有边界与显式NUL。检查器11处格式调用均检查cx_fmt，原缓冲容量及全部16断言保持。无基线修改、断言删除、容量收缩或提时限。

冻结源码SHA256：
- cache-retain：c3d1eaf2340bddf2f1a205968234bc6acba29af46b89d2b8960a353313a1c424
- check-cache-retain：9906a6b8082241811f81570953ed1a52bc33fefdf32679efd895bdd82861d904

实际默认检查36756 exit0/16/0；检查本体与内部管理器均由已装产品实编。正式run-batch48059 exit0，PASS/rc0/transport0/CACHE_RETAIN_CHECK，候选f069d4c9…，见safety-fix-registered.log。父已收到冻结窗口执行最终全局安全门；本报告不抢记尚未收到的全局门结果。

使用父此前相同P3和六个真实slice材料，默认release-ape重新实际构造两管理器并打包，65827 exit0。收据full-six-slot-safety-fix/ape-receipt.txt记录全部输入、构造产品及两个helper完整SHA。新私有载体6274578B，SHA a9c138ec1fc44daea10648ed6c685857d3abe02126084900068901741790de3f；包body仍0d569653…，六目标slice仍原材料。

两架构在各自全新私有XDG实际运行：ARM版本79240 exit0，hello直接终态1226bf exit7；Rosetta版本77270 exit0，hello38291 exit7，两个版本都打印unisacc0.0.47，stderr均空。实际管理标记与lease分别使用ARM raw-key e5f80facc67e6b41、x64 raw-key42ce67ed2f5864d6，证明管理器成功调用而非失败回退。stdout/stderr见safety-fix-{arm,x64}-{version,hello}.*。

本轮未重演真实缓存136→4删除，不修改根产品、公开资产或真实缓存策略；先前Gatekeeper拒绝、macOS13未实跑等边界不迁移或放宽。未提交。
