# L1b sys6 验收（第一片：POSIX sys6 私有 80B 帧）

cc 2026-10-09 按 cdx 指派准备。**实际跑过；这里只有旧参考与原型的结果，原型证据不是候选证据。**

## 探针与命令

- `sys6probe.c`：12 例 write 系统调用，具名（常量号）6 例、动态（volatile 号）6 例。每例的六个源（号加五参数）放在六个局部变量里，按不同声明顺序送进 gate，这样 lower 时源寄存器就被置换；buf 是帧内地址（原 r7 帧），多余参数带哨兵。期望输出 `expect.txt`，rc 0。
- `accept.c`：`unisacc.com -run research/l1b-sys6-acceptance/accept.c -- CC OUTDIR [RUN]`。对 CC：四个 POSIX 目标出镜像，两个 Linux 目标另出 `-S` lower 文本（Mach-O 不支持 `-S`，osx 以镜像字节判定）；两个 Windows 目标编译 `winfix.list` 的源，镜像必须与旧参考逐字节相同。RUN 在本机实跑 osx/arm64 与 osx/x86_64（Rosetta）镜像并对 expect.txt。每次编译、运行都经 tests/bound 限时。

## 预期

| 文件 | 编译器 | 结论 |
|---|---|---|
| `expect-ref.tsv` | 旧参考（与已发布 unisacc.com 镜像相同） | 基线；Windows 行即候选必须保持的字节 |
| `proto-private.tsv` | `/tmp/cdx-l1b-proto/ref-private`（sys6 私有帧原型） | 四个 POSIX 目标镜像和 lower 文本全变，Windows 四行与基线相同；osx 两目标实跑 12/12 |
| `proto-private3.tsv` | `/tmp/cdx-l1b-proto/ref-private3` | osx/x86_64 与 lnx/x86_64 同 private，arm64 不同；**win/arm64 两个镜像变了**，违反“Windows 原字节不变” |

正式候选的判定：Windows 四行必须等于 `expect-ref.tsv`；POSIX 六行应与基线不同（帧确实改了），且 RUN 两行 ok；与 `proto-private.tsv` 逐行相同可作为参考，但不同不算失败（生产 δ 与原型的编码可以不同），不同时报首差。

## 未覆盖

- Linux 实跑（cc 10-09 补，Lima default 原生 arm64）：lnx/arm64 镜像 ref、private、private3 三组都 rc0，输出与 expect.txt 逐字相同（md5 a0f3252e）。lnx/x86_64 在该客机无 x86 模拟，exec 失败 rc2，未跑；x86_64 证据看 release-check 真机 runner。
- 信号嵌套、sys/write 共享格、ARM 逐指令 SP：不在第一片。
