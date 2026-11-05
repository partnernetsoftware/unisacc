# 0.0.44

## 编译器
- 多源上限：单元数 64→128（parse2 UCN_MAXU、tokenlocations 帧、driver DV_MAXU/CXM_MAXU 统一 128）；65/72 源可编可跑，129 源干净拒绝 `too many sources (limit 128)`。
- 99+ 源栈越界 SIGSEGV 改为拷贝前判界、干净拒绝（SEGV-127）。
- 拒绝定位：tbl 把带原因的 REJECT 按状态区分（UC./DL. 前缀）；`UNISACC_REJECT_AT=1` 额外打印 reject-at 行；转发桩失败分两种报（超单元上限带数值 / 桩文件创建失败），不再统一报 `forwarding stub`。
- 帧拼接统一到 cx/compiler/frames.cx（driveunits 496→449 行）。
- 产品版本号 8 处纳入 check-limits 一致性校验。

## 构造链 / 发布
- `release-build p3 --nets CTX/jobs --routes`：用冻结链真实网重打包，进包前每张网跑 T1（网==表）；不再沿用 SRC_PKG 旧模型字节。
- release-build 快照补齐根产品三件套（build.json + unisacc.com.dev/）。
- whole-o（osx/win）改为工具内墙钟看门狗 rb_wait_child（120 s，可注入 UNISACC_RB_WHOLE_MS），超时报 `timeout after Nms`、删残余产物、杀进程组；tools/bound 超时明确报 `bound: timeout after N seconds`。
- 单元上限事实表 exec/facts/limits.tsv + check-limits：6 处上限与两份对象表跨表一致校验。

## 提速（SPEED-02）
- 作业身份 = 声明输入闭包（ptrace 观测读取 `read<TAB>sha<TAB>path`，含 ENOENT 探测），四类变异各自 MISS。
- reuse-batch 单进程判定：131 作业全命中 226 s → 12.7 s；复用默认仍关（UNISACC_WAVE_REUSE=1 启用）。

## 测试
- tests/module/（cxmod/driveplan/driveunits 直连单测）、tests/unit-boundary（1/2/63/64/65/127/128/129）、reject-locate、check-limits、check-net-dump、trace-tools-only。

## 已知未完成（发布时删除本段）
- 五片 APE：ape-pack 复制到 tools/ape-pack.cx 并加 P4 支持（APE-OWN，进行中）；本版安装走 dev-lnx。
- LOADER-GRAD（分段管道转正）进行中；DOCK2*、ARITY、AUDIT-DUP、CHAIN-ARCHIVE、SRC-SPLIT 为 P2。
