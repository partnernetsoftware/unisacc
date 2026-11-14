# 配套检查最终安全修正

结论：正式 chain-precheck 批次 session80563 已实际 poll 退出0，完整74项断言全部通过，BATCH_RUN 为 PASS/rc0/transport0，绑定当前产品f069d4c9…。正式完整stdout见 safety-final-chain-precheck.log，stderr保留于 out/c48-final-integrated/chain-precheck.stderr（no child-subreaper）。

仅 tools/check-chain-precheck.cx 改动：physical路径用 cx_fmt 并拒截断；变量长度拷贝使用 mm_copy，dcap 为实际 malloc(n) 容量，先核 n 转 long 可表示。新增 mem.bytes use。源码23774B，SHA256为03b850b65fc9c1cfdd186421346a15a4c070af66a75e95af007ad6d94aa2ae6d。正式默认-run已实际编译执行本检查。

tools/chain-precheck.cx 控制器未由本代理改动，SHA256为49236f542b4158fdc4d020cea2efa8c06102e39e9837bbd2757653d446f20572。三路径内容身份见 safety-final-source-sha256.tsv。采样前后文件cmp0；保留原全部74断言、输入容量、预算和基线，不扩额。

静态cxapis实际exit0；cxtrunc实际exit1，本路径无红，其他4路径超基线原样保留，详见同轮post-freeze报告。25个受影响precommit/报告正式批次也全部逐句柄实际exit0，证据见 research/c48-toolsloc-report-only/post-freeze-status.tsv。

本证据仅绑定本次配套检查与当前控制器/产品。旧gen2/gen3自举证据不迁身份，不声称全构造链或全仓静态门通过。未修改控制器、产品源、注册表、断言、基线或旧研究证据；未提交，无在途句柄。
