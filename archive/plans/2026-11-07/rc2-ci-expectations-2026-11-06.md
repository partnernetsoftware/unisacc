# rc2 CI 预期表(先验,不写 PASS)

R2 = f37d7b8414。本文件是**预期**,不是结果:任何一格在 CI 出结果前都不得写成 PASS。
用途:CI 一跑完,按下表逐格比对,立刻能分类,不用现想原因。

- 口径:6 个 runner × 7 项(identity / version / hello / stdio-crossunit / stream-pipe / smoke / run),`run` 列是 rc2 新增(默认 `-run`)。
- 「应该是什么」写的是**若某修复有效**的预期;「失败说明什么假设」写该格红时被证伪的那条假设。
- 每格给三个最可能根因 + CI 诊断该看哪一行。
- 与 rc1 的差别只有三处产品改动:NATIVE-RUN-HOST-TARGET(3f550c1313)、WARN-PROBE-USED(f37d7b8414)、`--emit-host-symbols` 帮助文字(b90f074527)。

## 表 1:预期矩阵

| runner | 目标 | identity | version | hello | stdio-crossunit | stream-pipe | smoke | run | 依据 |
|---|---|---|---|---|---|---|---|---|---|
| ubuntu-24.04 | lnx/x86_64 | 应 PASS | 应 PASS | 应 PASS | 应 PASS | 应 PASS | 应 PASS | 应 PASS | rc1 六项全 PASS;rc2 该项行为未变 |
| ubuntu-24.04-arm | lnx/arm64 | 应 PASS | 应 PASS | 应 PASS | 应 PASS | 应 PASS | 应 PASS | **应 PASS(本格是 rc2 的主要目的)** | NATIVE-RUN-HOST-TARGET:`tn_self()` 在 arm64 片返回 `lnx/arm64` |
| macos-15-intel | osx/x86_64 | 应 PASS | 应 PASS | 应 PASS | 应 PASS | 应 PASS | 应 PASS | **应 PASS(次要目的)** | `tn_self_launch()=="tmpfile"` → `hs_tmpfile_exec` |
| macos-15 | osx/arm64 | 应 PASS | 应 PASS | 应 PASS | 应 PASS | 应 PASS | 应 PASS | **应 PASS(次要目的)** | 同上,arm64 切片 |
| windows-2025 | win/x86_64 | 应 PASS | **应仍 FAIL** | **应仍 FAIL** | 应仍 NOT_RUN | 应仍 NOT_RUN | **应仍 FAIL** | 应仍 NOT_RUN | NATIVE-WIN-PE 未在本版修;`run_launch=none` 表示 win 不走 tmpfile/memfd |
| windows-11-arm | win/arm64 | 应 PASS | 应仍 NOT_RUN | 应仍 NOT_RUN | 应仍 NOT_RUN | 应仍 NOT_RUN | 应仍 NOT_RUN | 应仍 NOT_RUN | 无该片(设计);rc2 不改变 |

**判读要点**:
- win/x86_64 的 version/hello/smoke 三格**仍红是预期的**,不是退步。若它们变绿,反而是意外,要先查是不是跳过了断言。
- win 两格的 `run` 若报「未实现」而不是「不存在」,与 CHANGELOG 的 `WIN-RUN-CREATEPROCESS` 一致。

## 表 2:每格失败时的三个最可能根因 + 该看哪一行

### lnx/arm64 的 run(本版主目标)

| # | 根因 | 该看的 CI 输出行 |
|---|---|---|
| 1 | `-run` 仍编出 x86_64 镜像(hostid 没生效) | `UNISACC_RR_TIME=1` 的推理行里 `image=` / `target=` 字段;或 stderr 的 `errno 8`(ENOEXEC) |
| 2 | hostid 生效但 arm64 片没有可执行装载路径(仍走 memfd+execveat) | stderr 完整行;`hs_forward_exec` 里 `HS_MEMFD` 的返回值 |
| 3 | 交叉编目标对但运行库尾部按 x86_64 发(sysno/ABI 不一致) | `SIGILL`/`SIGSYS` 的 signal 号;`rc=132`(SIGILL)或 `rc=159`(SIGSYS) |

### lnx/arm64 的其余六项

| # | 根因 | 该看哪一行 |
|---|---|---|
| 1 | 与 run 同因:cell 工具自身经 `-run` 启动失败 → 六项全部无证据 | 每项开头是否有 `errno 8` |
| 2 | 交叉编产物能编不能执行(仅 version 过) | version 绿而 hello 红 → 看 hello 的 signal 号 |
| 3 | 材料/产品身份不符(R2 候选与 CI 下载物不一致) | identity 格若红,先看 `product_digest` 与 `build.json` 两行是否相等 |

### osx/x86_64 与 osx/arm64 的 version / hello(经 cell-tools 交叉编)

| # | 根因 | 该看哪一行 |
|---|---|---|
| 1 | `run` 仍走 memfd 路径(SIGSYS) | `Bad system call: 12` → exit 140;若 version 也红说明连 `-S` 都不行 |
| 2 | tmpfile 路径建在不可写 TMPDIR | stderr `cannot make a directory in TMPDIR` / `errno 13` |
| 3 | tmpfile 写完没有执行权限或被执行器拒绝 | stderr `Permission denied` / `errno 13`,或 execve 返回 `ENOEXEC` |

### osx 两格的 run

| # | 根因 | 该看哪一行 |
|---|---|---|
| 1 | `tn_self_launch()` 返回 `memfd` 而非 `tmpfile`(hostid 分支没命中) | `UNISACC_RR_TIME=1` 的推理行;或直接在 CI 里 `sh ./unisacc.com -S -o x x.c` 后用 `file x` 看格式 |
| 2 | tmpfile 路径把 Linux sysno 带进 Mach-O 执行 | signal 号(SIGSYS=12)与 exit 140 |
| 3 | osx 上 `flock`/`fork` 语义差异导致子进程没收尾 | stderr 是否出现 `residue` / 超时行 |

### lnx/x86_64 的任一格(基线格)

| # | 根因 | 该看哪一行 |
|---|---|---|
| 1 | 本机应绿而红 → 装配/下载的候选不对 | identity 格;`product_digest` |
| 2 | WARN-PROBE-USED 新判据误伤 stdio-crossunit | stdio-crossunit 的 `assertions=` 行与 stderr 是否出现 `private stdio table` |
| 3 | 环境(容器变量/UNISA_* )污染 | 该格 stderr 首行 |

### win/x86_64 的 version / hello / smoke

| # | 根因 | 该看哪一行 |
|---|---|---|
| 1 | NATIVE-WIN-PE 未修(预期) | `--version` 输出的乱串字节;hello 的 `0xC0000005` |
| 2 | 若 hello 之外全绿而 hello 仍 0xC0000005 | 与 rc1 一致,条目不变 |
| 3 | 若 version 变绿 | 先查是不是断言被放宽(比对 `expected` 列) |

## 表 3:与「必检条目」的对应(失效即整格不算证据)

| 条目 | 失效表现 | 影响 |
|---|---|---|
| NATIVE-RUN-HOST-TARGET | lnx/arm64 或 osx 两格的 run 仍红 | 门槛 6 的 native 证据仍 0,rc2 的主要卖点不成立 |
| WARN-PROBE-USED | objects-* 九批在 rc2 候选上仍红 | rc2 会话的 check-known-fail 仍报这九批 FAIL |
| WIN-RUN-CREATEPROCESS | win 格 run 从 NOT_RUN 变 FAIL | 说明有人把 win run 当已实现,需回退说明 |
| NATIVE-WIN-PE | win 的 version/hello/smoke 变绿 | **反常**,先查断言 |

## 判读顺序(CI 出结果后照此走)

1. 先看 identity 六格:任何一格红 → 候选/装配问题,后面全部不看。
2. 再看 lnx/x86_64 七项:基线不绿 → 全局问题。
3. 然后 lnx/arm64 的 run → 决定 NATIVE-RUN-HOST-TARGET 成不成立。
4. 然后 osx 两格的 version/hello/run → 决定 tmpfile 路径成不成立。
5. win 两格按表 1 预期核对,变绿要解释。
6. 每格红时按表 2 查对应那一行,把根因条目写进 CI-RUN-2 表。

## 未取证声明

本文件全部是预期。R2 的任何一格在 CI 原生 runner 上出结果前,状态都是「待取证」。
emulation、容器、缺席 guest 一律不得记 PASS。
