# 0.0.44-rc1

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

## 已知限制

本版是 0.0.44-rc1,只发 lnx/x86_64 候选,不打 0.0.44 正式标:主人 11-04 定的完成线要求两代不动点加六目标同候选证据,目前达不到。没有 push,CI 没跑,六目标原生证据 0/6。

| 目标 | 状态 |
|---|---|
| lnx/x86_64 | 本候选在本机原生实跑(证据见 plans/rc1-gates-2026-11-06.md) |
| lnx/arm64 | 只在 qemu 模拟下实跑通过,未原生验证 |
| osx/x86_64、osx/arm64 | NOT_RUN(未验证);这两片走 whole-o 路线,与 lnx 的 objects 路线不同 |
| win/x86_64 | NOT_RUN(未验证);走 whole-o 路线 |
| win/arm64 | 没有这个片;原生入口留 0.0.45 |

- 跨单元共享 FILE 已在 0.0.44 修复：运行库状态(流表、atexit 表、`errno`、`setenv` 的 environ 堆)改为**全程序一份**(`RT_UNIT`/`RT_EXTERN` 三态；默认无宏仍是 `static`，单源产物字节不变)。多单元一次编译的产物字节因此变化：`entry.elf` 3438193B → 3448676B(**+10483B**，本版只共享状态、不去重)。
- `-c` 分开编再链接的路径本版**只告警不修**：链接输入里有两个及以上自带私有流表的对象时，stderr 输出 `unisacc: warning: 2 objects each carry a private stdio table (cross-unit FILE state is not shared; compile the program in one invocation or link the RT_UNIT entity)`；rc 不变(报错会让今天能链接的程序失败)。真正合并需要 common/weak 符号(RT-COMMON，0.0.45)。
- tape 入口校验(e4/prune/lower)本版**已撤回**：逐字节校验首行 `_start:` 的网改动会误拒 `-c -funit` 单元对象(首行是 `.unit N`),且把 ASCII 字节值写进 result 表让 check-literal 超基线;止损期内没能在不改 seed/gen.c 的前提下做完白名单与常量引用,整段撤掉,`stages` 批次如实红(9 条 reject 用例)。tape 没有结束标记,截在任一行尾、或缺 `__main_ret` 段的 tape 都能进下游。实测:一个 stdio 程序的 tape 共 10670 个行尾截断点,逐个喂 prune|lower|elf,10667 个被 elf 以 `a branch to an undefined label` 拒绝;另 3 个只剩开头 1-3 行,能产出可执行文件但运行 rc 127 或 SIGSEGV,没有运行成功而行为错的情况。这类输入只出现在手工拼的 `stream` 管道里,`-o` 不会产生。加结束标记与入口白名单一起留 0.0.45(TAPE-END)。手工拼管道时要用 `set -o pipefail` 判断上游失败。
- 运行库每对象各带一份(约 264KB,ELF 的 15%),去重留 0.0.45(RT-DEDUP)。
- 产品接受 22 种 C99 约束违例而不报错(c-negative 批次 fail=22,0.0.43 起既有,本版未修,批次保持 FAIL)。要做约束检查,请另用 gcc/clang `-std=c99 -pedantic-errors`。
  - A 类 12 项,**可能静默产出错误代码,这类程序的产物不可信**:duplicate-case、case-outside-switch、struct-redefinition、conflicting-typedef、conflicting-function-types、enumerator-redeclared、local-redefinition、duplicate-parameter、void-value-used、void-call-as-value、float-subscript、return-pointer-as-int。修复列为 0.0.45 P1。
  - B 类 10 项,只是缺少诊断:assign-to-const、write-through-const-pointer、discards-const、multiple-storage-classes、void-parameter-named、incompatible-pointer-init、int-to-pointer-init、int-plus-assign-string、distinct-pointer-compare、zero-array-size。
- DOCK2*、ARITY、SRC-SPLIT 为 P2。
