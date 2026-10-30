# lnx/arm64 第二片：链接和 qemu 冒烟（2026-10-30）

这不是通过。没有 wrap，没有安装，没有改仓库根 `unisacc.com`。

## 链接

构造器是已装产品，用 `sh` 拉起。`tools/entry-build.cx --target lnx/arm64`，默认六只 `-O1`，其余 `-O2`。目录 `out/grk-arm64-link`。P3 仍是 `out/grk-entry-build-14/pkg.bin`，sha256 `b976719eb971fceae766398fe531c0cd16c25bbffab9ef2a7ab6d8184af70fd3`。33 个对象分三批，再 `--link-only`。每批外层 `tools/bound 55`。

| 文件 | 字节 | 权限 | sha256 |
|---|---|---|---|
| `entry.elf` | 1315376 | 0755 | `c0adf34abc953ee4846695994c91843530d2003888a253a1a308113ad5c72a75` |
| `candidate`（`entry.elf` 后面接同一只 P3） | 2951396 | 0755 | `16f5dc9938be68212479f40f7a95dce4e42dd992e5b8397d3bdde80e79e28ec5` |

`file` 看这两只都是 ELF 64-bit ARM aarch64，静态链接。对照的 x86_64 是 `out/grk-host-sys`：`entry.elf` sha256 `da3f42daed4faef32ac550b67856eade8eb344effaafca9a5ea0cb3c3c09facd`，候选 sha256 `9fb93c137363b210295870853fcf01b6693414270bc57f5c1438779c197535bf`。

## qemu

`/home/devali/.local/bin/qemu-aarch64-static -L /usr/aarch64-linux-gnu`。`/usr/bin/qemu-aarch64-static` 不存在。两边都去掉 `UNISA_CONTAINER`、`UNISACC_CONTAINER`、`UNISACC_RR_TIME`、`UNISACC_E3CACHE`。

`--version` 跑的是 `entry.elf`。两边退出码 0。标准输出都是 `unisacc 0.0.43\n`，15 字节，逐字节相同。标准错误都是空的，逐字节相同。

裸的 `entry.elf` 做 `-run` 时两边都退出 1，标准输出都是空的。标准错误都是「没有包」那一句，但句子里的路径分别是两只 `entry.elf` 自己的路径，所以标准错误不是逐字节相同。

`-run` 要包，所以程序冒烟跑的是接了 P3 的候选，不是裸的 `entry.elf`。

`examples/hello.c`（`printf`，退出时应打印 `from hello.c\n`）：x86_64 候选退出 0，标准输出 13 字节 `from hello.c\n`，标准错误空。arm64 候选在 qemu 下退出 1，标准输出空，标准错误是下面这一句。不是逐字节相同。

`out/grk-arm64-link/smoke/main.c` 只有 `int main(void) { return 7; }`。x86_64 候选退出 7，标准输出和标准错误都空。arm64 候选退出 1，标准输出空，标准错误是同一句。退出码不同，标准错误不同。

```
unisacc: model driver: this program calls the host C library, and its image could not be started from an anonymous memory file (errno 38); write the program with -o and run that
```

errno 38 是 ENOSYS。`src/host.cx` 的 `hs_forward_exec` 在 aarch64 上用系统调用号 279（memfd）加标志 `1|16`。这次 qemu 没有回 EINVAL，所以没有改成不带 16 再试。这一步没有改这段代码。

## memfd 没有回退（2026-10-30，只看，不改）

`hs_forward_exec`（`src/host.cx` 第 572 行）只做 memfd 再 `execveat`。第 549–552 行写明没有落盘回退，也不回到 `hs_enter`。第 586 行用标志 `1|16`。只有返回 -22（EINVAL）才在第 587 行改成标志 `1` 再试一次。其它失败，包括 ENOSYS（-38），直接走到第 608 行把 errno 打出来并返回 1。没有写临时文件，没有第二次 exec。记为待补。这一刀不改。

## 改测 -o 产物（2026-10-30）

裸的 `entry.elf` 做 `-o` 退出 1：没有包。所以进程仍是 `out/grk-arm64-link/entry.elf`，包用 `UNISA_CONTAINER` 指到接了 P3 的候选 `out/grk-arm64-link/candidate`。qemu 是 `/home/devali/.local/bin/qemu-aarch64-static -L /usr/aarch64-linux-gnu`。

`entry.elf -b lnx/arm64 -o out/grk-arm64-run/hello-arm examples/hello.c`：退出 0，3.659 秒，标准错误空。`hello-arm` 45096 字节，`file` 是 ELF 64-bit ARM aarch64，静态链接。

对照：`out/grk-host-sys/entry.elf`，包用它自己的候选，`-b lnx/x86_64 -o out/grk-arm64-run/hello-x86`。退出 0，0.549 秒。`hello-x86` 32808 字节，ELF x86-64，静态链接。

两边的程序都退出 0。标准输出都是 `from hello.c\n`，13 字节，逐字节相同。标准错误都是空的，逐字节相同。qemu 跑的是编出来的 hello，不是编译器的 `-run`。这不是五片通过，也不是 `-run` 通过。
