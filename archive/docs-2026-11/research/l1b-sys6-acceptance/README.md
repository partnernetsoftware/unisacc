# L1b sys6 验收（第一片：POSIX sys6 私有 80B 帧）

cc 2026-10-09 按 cdx 指派准备。**实际跑过；这里只有旧参考与原型的结果，原型证据不是候选证据。**

## 探针与命令

- `sys6probe.c`（C 参数求值证据）：12 例 write，常量号与 volatile 号各 6 例，参数按不同声明顺序求值，buf 为帧内地址。front_parse.c 把每个 `__syscall6` 都发成 `.sys6 syscall, r0..r5`，所以它**不**覆盖具名 op、tape 源置换或 r7 直接作源（cdx 10-09 审阅指出）；保留为参数求值证据。期望 `expect.txt`。
- `sys6tape.tmpl`（低级 tape 证据）：手写 tape，accept.c 按目标填 `@NR_WRITE@`、`@MAP_ANON@`。A 动态 syscall 且六源置换（号在 r3）；B r7 直接作 buf 源；C 号在 r5、六源倒序；D 具名 `mmap` 六源置换，写读回映射页，再用 r7 源 write。每例校验返回 7，期望 `expect-tape.txt`，失败退出码 11–15 指明哪一例。
- `accept.c`：`unisacc.com -run research/l1b-sys6-acceptance/accept.c -- CC OUTDIR [RUN]`。四个 POSIX 目标各出两个探针的镜像；lnx 两目标另出两份 `-S` 文本（Mach-O 无 `-S`，osx 以镜像字节判定）；Windows 两目标编译 `winfix.list`，镜像必须等于基线。RUN 在本机实跑 osx/arm64 与 osx/x86_64（Rosetta）的两个探针。
- 防假绿（cdx 审阅）：每个产物先删除，编译到 `tmp-NAME`，成功才改名；编译失败打印 `FAIL compile`、不哈希、记 BAD，退出码 1。故障注入实测：在旧产物齐全的目录上用 `/usr/bin/false` 当 CC，20 行全 FAIL，`accept BAD`，rc1，没有一行读到旧镜像。tmp 名保留扩展名，因为 `-S` 只在 `*.s` 时写文本（前一版用 `.tmp` 后缀，lower 行实际是镜像，已修）。

## 预期（原型证据，不是候选证据）

| 文件 | 编译器 | 结论 |
|---|---|---|
| `expect-ref.tsv` | 旧参考（POSIX 镜像与已发布 unisacc.com 相同） | 基线；Windows 四行即候选必须保持的字节；RUN 四行 ok |
| `proto-private.tsv` | `/tmp/cdx-l1b-proto/ref-private`（sys6 私有帧原型） | POSIX 十二行全变，Windows 四行同基线，RUN 四行 ok |
| `proto-private3.tsv` | `/tmp/cdx-l1b-proto/ref-private3` | **win/arm64 两镜像变了**，违反 Windows 字节不变；首片不搬 private3 ARM 编码 |

正式候选判定：`accept ok`；Windows 四行等于 `expect-ref.tsv`；POSIX 行与基线不同（帧确实改了）；RUN 全 ok。与 `proto-private.tsv` 相同只作参考，不同不算失败，报首差。

## 未覆盖

- Linux 实跑（cc 10-09 补，Lima default 原生 arm64）：lnx/arm64 镜像 ref、private、private3 三组都 rc0，输出与 expect.txt 逐字相同（md5 a0f3252e）。tape 探针 lnx/arm64：ref 与 private 都 rc0，输出同 expect-tape.txt（md5 e1112b1e）。lnx/x86_64 在该客机无 x86 模拟，exec 失败 rc2，未跑；x86_64 证据看 release-check 真机 runner。
- 信号嵌套、sys/write 共享格、ARM 逐指令 SP：不在第一片。

## 产品候选验收（cc 2026-10-09，私有同源候选 a4943d78，树 8bb59293）

`accept ok`，rc0：Windows 四行与 `expect-ref.tsv` 相同；POSIX 十二行与基线全不同（帧确实变了），其中 6 行与 `proto-private.tsv` 相同；RUN 四行 ok（osx/arm64 与 osx/x86_64 Rosetta，C 与 raw tape）。Lima default 原生 lnx/arm64：C 探针与 tape 探针都 rc0，输出同期望（a0f3252e、e1112b1e）。lnx/x86_64 本机不能运行，看 release-check 真机。这是私有候选，正式发布候选另建后重跑同一命令。
