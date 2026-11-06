# 0.0.45(预发布)

候选:APE sha256 `ae0b12d8c822efa44d6724782f32a275b4d3fb2cdb3387178cbd76541972c6f0`,6,128,709B,源码 `fa0a747c7f22ba20c0a875183108106afff21f4a`,`unisacc --version` 为 `unisacc 0.0.45`。未对本候选运行自举固定点(`tests/chain-fixed-point.cx`)。完整发布说明见 `plans/release-notes-0.0.45.md`。

全批会话没有对本候选重跑;最近一次(c45-1,lnx/x86_64)418 批已实现,292 PASS / 58 FAIL / 1 UNKNOWN / 67 MISSING,另 68 批材料未登记;失败项均登记于 `tests/waivers-0.0.44.tsv`。本版因此标预发布。

## 支持矩阵(GitHub 原生 runner,candidate-native,运行 42191390455,pass=36 fail=0 not_run=6)

| 目标 | 状态 |
|---|---|
| lnx/x86_64 | identity、version、hello、run、smoke、stdio-crossunit、stream-pipe 全 PASS |
| lnx/arm64 | 同上,全 PASS |
| osx/x86_64 | 除 stdio-crossunit 外 PASS;stdio-crossunit NOT_RUN(runner 无 `/dev/full`) |
| osx/arm64 | 同上 |
| win/x86_64 | identity、version、hello、run、smoke PASS;stdio-crossunit、stream-pipe NOT_RUN(需 POSIX `fork`) |
| win/arm64 | 同 win/x86_64 的 5 项 PASS,执行体是独立资产 `slice-win-arm64`(sha256 `380e28700a1a2595b7fda3cff563452efeb40b361c22c4829fc3f4a2eda85b25`)配合 `pkg.bin`,不是 `unisacc.com` 自身;APE 内尚无该片分发 |

## 修复(rc3 → 0.0.45)

- **kernel `long`→`I`**(LLP64):win 数据模型 `long=4`;lnx/osx 四片对象 sha 对拍不变。
- **`sy_tmpdir`**:win 临时路径 `'\'`→`'/'`。
- **WIN-RUN CreateProcess**:win 默认 `-run` 写临时 exe 再 CreateProcessA。
- **strdup**:win 头自带实现;`open`/`read`/`close`/`lseek`/`fdopen` 的 UCRT 下划线绑定名由事实表 `exec/facts/libc99posix.tsv` 驱动,fwdstub 按目标查绑定。
- **INC2 盘符绝对路径**:`#include "C:/…"` 在 win 目标按绝对路径解析(`INC2.drv` 补 `SBCLR`);尖括号形式不处理。
- **PE `DYNAMIC_BASE` 与启动崩点**:win 启动崩溃(0xC0000005)的根因修复。
- **`_Static_assert` 之后字符串字面量取错**(rc3 已知限制):`tests/cases/sa-pool.cx` 在本候选上五条全 OK。
- **win/arm64**:第一次有同候选原生证据;candidate-native 新增该格,FFI strdup/pread 在 win 与 lnx 逐字节相同。
- **版本串** `unisacc 0.0.45`。
- **`tools/install-ape.cx`**:原子安装 APE 到仓根,旧脚本留作 `unisacc-dev`,可 `--rollback`。

## 已知限制

- 全批会话未对本候选重跑(见上)。
- win 的 stdio-crossunit、stream-pipe,osx 的 stdio-crossunit 未跑(原因见矩阵)。
- APE 内尚无 win/arm64 分发。
- `#include <C:/x.h>`(尖括号)不按盘符绝对路径处理。
- 其余沿用下方 rc3「已知限制」中仍成立项:部分非法 C 被静默接受、`-c` 分开编译运行库状态不共享、手工拼 `stream` 管道的截断风险、预处理器接受两种非法输入、`-c -b osx/*|win/*` 文档与实际不符、运行库每对象一份。

# 0.0.44-rc3(候选)

候选:rc3 APE sha256 `54bb97a2672f5010c8815fb0f6ecfb3ee7bb7ca91af18723007430375cc3bbb8`,5,608,646B,源码 R4=`b1a219cf7d`;lnx/x86_64 候选三代逐字节相同(`45bdf636df1e`,`tests/chain-fixed-point.cx` failed=0)。

本机全批会话(lnx/x86_64,同一候选 `54bb97a2`,会话 s3):408 批,274 PASS / 63 FAIL / 68 MISSING / 3 UNKNOWN;`tests/check-known-fail.cx` known=134 unknown=0 stale=0(每个非 PASS 都有带条目号的 F 行)。会话外在同一候选的 split2 安装上重跑 15 批:12 个读已装 split2 部件的批次(会话克隆只装了单文件 APE)与 3 个超时批次全部 PASS;cross-targets 用时 53 秒,超出会话 40 秒上限,0.0.45 按 OS 拆批(CROSS-TARGETS-SPLIT)。

## 支持矩阵(GitHub 原生 runner,CI-RUN-6,运行 42092234905)

| 目标 | 状态 |
|---|---|
| lnx/x86_64 | 已在原生 runner 验证:identity、version、hello、run、stdio-crossunit、stream-pipe、smoke 7/7 PASS |
| lnx/arm64 | 已在原生 runner 验证:7/7 PASS |
| osx/x86_64 | 已在原生 runner 验证:6 项 PASS;stdio-crossunit 的 devfull 用例需要 Linux 的 `/dev/full`,记 NOT_RUN |
| osx/arm64 | 已在原生 runner 验证:同 osx/x86_64 |
| win/x86_64 | **已知不可用**:`--version` 退出 1 并输出乱串,编译与 smoke 访问违例(0xC0000005);根因未定位(NATIVE-WIN-PE,见下) |
| win/arm64 | APE 没有这个片;原生入口留 0.0.45 |

## 修复(rc1 → rc3)与证据

- **默认 `-run` 的宿主目标**(NATIVE-RUN-HOST-TARGET):宿主目标不再写死 `lnx/x86_64`,取本片自身目标(生成的 L0 `cx/sys/hostid.cx`,来源 `exec/facts/pp-targets.tsv`)。证据:CI-RUN-1 里 lnx/arm64 `-run` 报 errno 8,CI-RUN-2 起 `run` 项 PASS。
- **osx 的 `-run`**(tmpfile 启动,L0 `cx/sys/runlaunch.cx`):osx 片把镜像写进 TMPDIR 下 0700 私有目录再执行、结束即删;win/lnx 片不带这段进程代码。证据:CI-RUN-1 osx `Bad system call: 12`,CI-RUN-2 起 osx 两格 `run` PASS。
- **`-c` 链接告警误报**(WARN-PROBE-USED):判据由"对象带运行库流表"改为"对象调用了流表入口"(`exec/facts/rt-shared.tsv` 第 8 列)。证据:rc3 候选本机 objects-link 等五批由 FAIL 转 PASS;两个都调用 printf 的对象链接仍告警一次(`tests/cx-unit-graph.cx` 正例)。
- **APE 首次解包向 stderr 写 `tail: stdout: Broken pipe`**(LOADER-STDERR,macOS):解包改为读尽管道。证据:CI-RUN-3 osx `--version` 的 stderr 为 26 字节该句,CI-RUN-6 osx 两格 version PASS(判据要求 stderr 为空)。
- **`unisacc stream` 默认目标写死**(STREAM-HOST-TARGET):默认取本片目标。证据:CI-RUN-2 lnx/arm64 与 osx 的 stream-pipe `B plain-image-pipe-cmp` FAIL,CI-RUN-6 三格 PASS。
- `--emit-host-symbols` 帮助文字(b90f074527)。

## 已知限制(相对 rc1 新增或改写)

- win/x86_64 原生不可用。CI-RUN-3/4/5 的诊断:Linux 交叉编的 argv 探针在 Windows 上 `argc` 读出 `.data` 里的一个常量指针、`argv` 为空;把 PE 头改成固定基址后映像按 ImageBase 加载,行为不变,所以不是"随机基址未重定位"。0.0.45 先补调试手段(WIN-DEBUG-LOOP)。
- `objects-unsupported-*` 四批仍红:合同要求拒绝 osx/win 的 `-c`,而产品编出了合法的 Mach-O/COFF 对象;合同改为"编出且格式合法"留 0.0.45(OBJ-UNSUPPORTED-CONTRACT)。
- 其余同下方 rc1「已知限制」。

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
- rc1 候选:rev `7266862b08`,APE `a76e5fc1`(5,494,758B,lnx/x86_64 冒烟通过);lnx/x86_64 候选 `c3996bd4` 在一、二、三代逐字节相同。已装:entry `0a82f989`(1,789,094B)、包 `7080b53a`(1,659,574B)。

## 提速（SPEED-02）
- 作业身份 = 声明输入闭包（ptrace 观测读取 `read<TAB>sha<TAB>path`，含 ENOENT 探测），四类变异各自 MISS。
- reuse-batch 单进程判定：131 作业全命中 226 s → 12.7 s；复用默认仍关（UNISACC_WAVE_REUSE=1 启用）。

## 测试
- tests/module/（cxmod/driveplan/driveunits 直连单测）、tests/unit-boundary（1/2/63/64/65/127/128/129）、reject-locate、check-limits、check-net-dump、trace-tools-only、stream-pipe(七段管道对产品逐字节)、stages(tests/stages/<段>/ 分段字节用例,正例/拒绝/链/锚点,无拒绝用例的段记 GAP)、kernel-base、facts-current。

## 已知限制

本版是 0.0.44-rc1,只发 lnx/x86_64 候选,不打 0.0.44 正式标:主人 11-04 定的完成线要求两代不动点加六目标同候选证据,目前达不到。打 tag 时 CI 没跑;之后在 GitHub 原生 runner 上跑了一次同候选资格(CI-RUN-1,APE a76e5fc1),只有 lnx/x86_64 通过。

**默认的 `-run`(直接运行)在 lnx/arm64、osx、win 主机上不可用(CI 实测),多架构 APE 目前只有 lnx/x86_64 能用默认模式。** 原因是 `-run` 的宿主目标写死为 lnx/x86_64(`src/drive.cx:504`、`cx/driver/route.cx:392`),在别的主机上也编出 lnx/x86_64 镜像;修复进 rc2(NATIVE-RUN-HOST-TARGET)。

| 目标 | 状态(CI-RUN-1) |
|---|---|
| lnx/x86_64 | 原生 6/6 PASS(identity、version、hello、stdio-crossunit、stream-pipe、smoke) |
| lnx/arm64 | 原生 FAIL:`-run` 报 `image could not be started from an anonymous memory file (errno 8)`,整格无证据 |
| osx/x86_64 | 原生 FAIL:同一步 `Bad system call: 12`(exit 140) |
| osx/arm64 | 结果未出 |
| win/x86_64 | 原生 FAIL:身份核对通过,但 `--version` 退出 1 且输出乱串,编译与 smoke 访问违例(0xC0000005);NATIVE-WIN-PE。CI-RUN-3/4/5:Linux 交叉编的 argv 探针在 Windows 上 `argc` 读出 `.data` 里的一个常量指针、`argv` 为空;把 PE 头改成固定基址后映像按 ImageBase 加载,行为不变,所以不是"随机基址未重定位"。根因未定位,0.0.45 先补调试手段(WIN-DEBUG-LOOP) |
| win/arm64 | 没有这个片,只核身份;原生入口留 0.0.45 |

- 跨单元共享 FILE 已在 0.0.44 修复：运行库状态(流表、atexit 表、`errno`、`setenv` 的 environ 堆)改为**全程序一份**(`RT_UNIT`/`RT_EXTERN` 三态；默认无宏仍是 `static`，单源产物字节不变)。多单元一次编译的产物字节因此变化：`entry.elf` 3438193B → 3448676B(**+10483B**，本版只共享状态、不去重)。
- `-c` 分开编再链接的路径本版**只告警不修**：链接输入里有两个及以上自带私有流表的对象时，stderr 输出 `unisacc: warning: 2 objects each carry a private stdio table (cross-unit FILE state is not shared; compile the program in one invocation or link the RT_UNIT entity)`；rc 不变(报错会让今天能链接的程序失败)。真正合并需要 common/weak 符号(RT-COMMON，0.0.45)。
- tape 入口校验(e4/prune/lower)本版**已撤回**：逐字节校验首行 `_start:` 的网改动会误拒 `-c -funit` 单元对象(首行是 `.unit N`),且把 ASCII 字节值写进 result 表让 check-literal 超基线;止损期内没能在不改 seed/gen.c 的前提下做完白名单与常量引用,整段撤掉,`stages` 批次如实红(9 条 reject 用例)。tape 没有结束标记,截在任一行尾、或缺 `__main_ret` 段的 tape 都能进下游。实测:一个 stdio 程序的 tape 共 10670 个行尾截断点,逐个喂 prune|lower|elf,10667 个被 elf 以 `a branch to an undefined label` 拒绝;另 3 个只剩开头 1-3 行,能产出可执行文件但运行 rc 127 或 SIGSEGV,没有运行成功而行为错的情况。这类输入只出现在手工拼的 `stream` 管道里,`-o` 不会产生。加结束标记与入口白名单一起留 0.0.45(TAPE-END)。手工拼管道时要用 `set -o pipefail` 判断上游失败。
- 运行库每对象各带一份(约 264KB,ELF 的 15%),去重留 0.0.45(RT-DEDUP)。
- 产品接受 22 种 C99 约束违例而不报错(c-negative 批次 fail=22,0.0.43 起既有,本版未修,批次保持 FAIL)。要做约束检查,请另用 gcc/clang `-std=c99 -pedantic-errors`。
  - A 类 12 项,**可能静默产出错误代码,这类程序的产物不可信**:duplicate-case、case-outside-switch、struct-redefinition、conflicting-typedef、conflicting-function-types、enumerator-redeclared、local-redefinition、duplicate-parameter、void-value-used、void-call-as-value、float-subscript、return-pointer-as-int。修复列为 0.0.45 P1。
  - B 类 10 项,只是缺少诊断:assign-to-const、write-through-const-pointer、discards-const、multiple-storage-classes、void-parameter-named、incompatible-pointer-init、int-to-pointer-init、int-plus-assign-string、distinct-pointer-compare、zero-array-size。
- 坏包会被明确拒绝(新批次 bad-package,23/0):包缺失、截掉 1 字节或截掉一半、footer 魔数错、包里 kernel 记录被改名、kernel 头魔数错、kernel 头声明的长度超过实际,都是 rc 非 0、stderr 给出原因、不出产物。但**声明长度正确、kernel 代码内容被破坏**时(例如代码尾部 128 字节被抹成 0),产品会段错误(rc 139),不会给出具名拒绝:kernel 内容没有校验和,补校验留 0.0.45(KERNEL-CONTENT-CHECK)。
- 该告警对 `.cx` 对象**误报**:每个 `.cx` 对象的 `.unit` 记录都带运行库预置的 `.bss g_<probe>` 行,判据(`src/drivelink.cx` 的 `dv_note_shared`)只数这一行,所以两个及以上 `.cx` 对象链接时恒打印 1 次,源码是否用 stdio 都一样;纯 C 对象不受影响。可忽略,rc 与产物不变。复现:`main.cx` 只写 `extern int object_value; int main(void){return object_value;}`,另一单元定义 `object_value`,各自 `-O2 -c -funit -b lnx/x86_64` 后一起链接,stderr 出现该告警。依赖"链接时 stderr 为空"的脚本要相应调整;objects-* 九批因此如实红。判据修复进下一候选(WARN-PROBE-USED)。
- `unisacc stream sidecar` 不接受需要 forwarding-stub 的输入(有未定义外部符号的源,例如只 `extern int g;` 再在 `main` 里用它):报 `stream sidecar: 1 host slot(s) need the forwarding-stub pass; not a pipe stage`,rc=1。这是有意的设计,forwarding-stub 一步不是管道阶段(`src/drivestream.cx`);这类源请用整链(`-o`/`-run`)。后续是否提供管道形态见 STREAM-SIDECAR-FWDSTUB(0.0.45)。
- 预处理器接受两种非法输入而不报错(pp-negative 批次 pass=8 fail=2,0.0.43 起既有):`#define` 后的宏名不是标识符、`#include` 的头文件名没有闭合。修复留 0.0.45(PP-NEGATIVE-2)。
- DOCK2*、ARITY、SRC-SPLIT 为 P2。
