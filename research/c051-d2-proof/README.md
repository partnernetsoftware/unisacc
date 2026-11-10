# c051 D2 实测:cs_target 顺序决定 routes/compiler.pkg 字节

- 无 D2(cs_target 现状,arm64 在前): routes/compiler.pkg sha=a1797e8cc9d6e091 bytes=2370305
  来源 /tmp/c051-nod2-run3.log 第 33 行(PREPARED chain-srcpkg /tmp/c051-nod2-out/routes/compiler.pkg)
- 有 D2(cs_target 改为 x86_64 在前,与图一致): routes/compiler.pkg sha=57a727cc281d1549 bytes=2370305
  来源 /tmp/c051-d2-run5.log 第 365 行(PREPARED chain-srcpkg /tmp/c051-d2-out/routes/compiler.pkg)
- 结论: 顺序改变 ctx 作业清单次序,进而改变 routes/compiler.pkg 字节,影响 P4 与发布产物。
- 复现: 同一 bootstrap,分别用有/无 D2 的 tools/chain-step.cx 跑 tools/candidate-chain.cx 到 routes 阶段,比较 routes/compiler.pkg 的 sha。
- 日志状态: 日志在撰写时仍存在(未 LOGS_GONE)。
