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
- APE 只带尾部一份模型包:ape-pack 拒绝片尾带 UNIPKG1 的片(`slice carries a package; pass <slice>/entry.elf`),lnx 片传 entry.elf;实测 APE 8,512,083→5,477,539B(APE-ONEPKG,2f5512d04b)。
- 产品子命令 `unisacc stream STAGE`:单阶段过滤器,e2|e1|e3|sidecar|prune|lower|elf 与 `-o` 逐字节相同;改一行表到 e3 结论双向实测 21.9 s/21.8 s。

## 提速（SPEED-02）
- 作业身份 = 声明输入闭包（ptrace 观测读取 `read<TAB>sha<TAB>path`，含 ENOENT 探测），四类变异各自 MISS。
- reuse-batch 单进程判定：131 作业全命中 226 s → 12.7 s；复用默认仍关（UNISACC_WAVE_REUSE=1 启用）。

## 测试
- tests/module/（cxmod/driveplan/driveunits 直连单测）、tests/unit-boundary（1/2/63/64/65/127/128/129）、reject-locate、check-limits、check-net-dump、trace-tools-only、stream-pipe(七段管道对产品逐字节)、stages(tests/stages/<段>/ 分段字节用例,正例/拒绝/链/锚点,无拒绝用例的段记 GAP)、kernel-base、facts-current。

## 已知未完成（发布时删除本段）
- 跨单元共享 FILE:多源一次编译时,单元 A 写入的缓冲在单元 B fclose 时丢失(RT-SHARED 修复中,将并入本版);修复后多单元产物字节会变。`-c` 分开编再链接的路径本版只给警告,不修(RT-COMMON,0.0.45)。
- tape 入口校验(e4/prune/lower)只检查首行 `_start:` 和末字节换行(STAGE-REJECT 修复中)。tape 没有结束标记,所以截在任一行尾、或缺 `__main_ret` 段的 tape 能通过入口检查,只能靠下游拒绝。实测:一个 stdio 程序的 tape 共 10670 个行尾截断点,逐个喂 prune|lower|elf,10667 个被 elf 以 `a branch to an undefined label` 拒绝;另 3 个只剩开头 1-3 行,能产出可执行文件,但运行 rc 为 127 或 SIGSEGV,没有运行成功而行为错的情况。这种输入只会出现在手工拼的 `stream` 管道里,`-o` 不会产生。加结束标记留 0.0.45(TAPE-END)。手工拼管道时要用 `set -o pipefail` 判断上游失败。
- 运行库每对象各带一份(约 264KB,ELF 的 15%),去重留 0.0.45(RT-DEDUP)。
- DOCK2*、ARITY、SRC-SPLIT 为 P2。
