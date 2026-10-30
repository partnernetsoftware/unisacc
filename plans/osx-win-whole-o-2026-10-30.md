# osx / win 整程序 -o（2026-10-30，已装产品）

只测能不能在 55 秒内出片。没有改 `tools/entry-build.cx`，没有 wrap，没有安装。

构造器是已装产品，`sh ./unisacc.com`。3035300 字节，sha256 `79fb84e65a10b55ce40be5f68746748acbd38f441944046c0a5dfa5718bc0c32`。33 个入口源一次交给它，外层 `tools/bound 55`。`UNISACC_RR_TIME=1`，为的是超时时能看到停在哪一段。`UNISA_CONTAINER` 和 `UNISACC_CONTAINER` 都去掉了。

两次都没有超时，也都没有写出文件。

| 目标 | 墙钟 | 退出码 | 停在哪 |
|---|---|---|---|
| `-b osx/x86_64 -o out/grk-slice-o/slice-osx-x86_64` | 2.857 秒 | 1 | e2。构造 22.4 ms，e2 inflate 9.3 ms，e2 推理 127.0 ms 后退出 |
| `-b win/x86_64 -o out/grk-slice-o/slice-win-x86_64` | 2.835 秒 | 1 | e2。构造 27.7 ms，e2 inflate 9.5 ms，e2 推理 140.6 ms 后退出 |

标准错误都是这一句：

```
src/hostnative.cx:8:1: error: #error hostnative_requires_linux_x86_64_or_arm64
```

`src/hostnative.cx` 第 7 行要求 `__linux__` 并且是 x86_64 或 aarch64。osx 和 win 不满足，预处理就停了。没有 Mach-O，没有 PE，也就没有可核对的 P3。

结论：以现在这 33 个源，整程序 `-o` 不能在 55 秒内交出 osx 片或 win 片。不是慢，是 e2 拒绝。没有进入后面的阶段，所以没有超时，也没有按源分批的内存和时间可量。单文件 `examples/hello.c` 的 `-o` 仍能出 Mach-O 和 PE，那是上一页的事，不是这 33 个源。

## 去掉 hostnative.cx 再试（2026-10-30，不改源）

`src/hostnative.cx` 还在盘上，没有改，也没有改 `tools/entry-build.cx`。这次命令行不带它，用入口表里另外 32 个源，顺序不变，`tapelink.cx` 仍在最后。构造器仍是上面那只已装产品。外层 `tools/bound 55`。`UNISACC_RR_TIME=1`。`UNISA_CONTAINER` 和 `UNISACC_CONTAINER` 都去掉了。目录 `out/grk-slice-o32`。

| 目标 | 墙钟 | 退出码 | 结果 |
|---|---|---|---|
| `-b osx/x86_64 -o out/grk-slice-o32/slice-osx-x86_64` | 36.025 秒 | 0 | 写出文件 |
| `-b win/x86_64 -o out/grk-slice-o32/slice-win-x86_64` | 11.218 秒 | 1 | 没有文件 |

osx 这只是 Mach-O 64 位 x86_64 可执行文件，0755，1486306 字节，sha256 `382995ac179949a0570fe34cc7ea0b92beaa15420947a5f5ccbb6075d5c547a4`。开头 `cf fa ed fe`，cpu `0x01000007`，filetype 2（MH_EXECUTE）。`file` 的标志是 `NOUNDEFS|DYLDLINK|TWOLEVEL|PIE`。计时里有一段名叫 `elf`，那是 `src/stages.cx` 第 42 行给镜像阶段起的名字，不是文件格式。

这只文件里没有 `UNIPKG1`。尾部 16 字节不是包尾。整文件比已装产品里那段 1636020 字节的包还小，所以没有带上 P3。`-o` 只写了镜像，没有像入口构造那样在后面接包。本机是 Linux，没有跑这只 Mach-O。

win 停在 e3。units 那一段退出码 0，e3 推理退出码 1。标准错误一共 5 条，第一条是：

```
src/host.cx:265:50: error: unknown identifier
          fd = open(tmp,  1  |  0x100  |  0x400  | O_CLOEXEC | O_NOFOLLOW, 0755);
                                                   ^
```

源文件同一行写的是 `O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW`。前三个已经换成数字，`O_CLOEXEC` 没有。后面四条是 `src/host.cx:622` 的 `O_NONBLOCK`、`src/drive.cx:908` 的 `environ`、`src/sys/mem.cx:63` 和 `:69` 的 `PROT_READ`。没有 PE，没有 P3，没有执行。这一刀不改这些行。

## 收进 cx/sys 后再跑 win（2026-10-30）

五条都收进 `cx/sys`，上层不再写这些名字。`90fbd3f37b` 是 `cx/sys/fs.cx`、`env.cx`、`mem.cx`：`sy_open_new`、`sy_open_read`、`sy_env_vec`，保护函数改传种类。Linux 上原来的标志值留在 static 帮手里。`src/host.cx` 改调前两个，`src/drive.cx` 改调 `sy_env_vec`。调用处只声明，没有 `use sys.fs`：`drive.o` 经 `runcache` 已经有这些函数体。

`check-cx-sys` 是 100/0。`check-cx-sys-net`、`spawn`、`ffi`、`guard` 都是 failed=0。

Linux 候选不是逐字节相同。`out/grk-winfix/candidate` 2980140 字节，sha256 `96369ef9fb483cb74498f65158933971a7e0f372f100428f20252755a89eb165`。`tests/runcache.cx` 对这只是 12/0，冷 563 ms，命中 44 ms。对拍切片写出 18 格，全是 SAME：要求的前 10 格（`do.c` 的 `-E`/`-S`/`-run`，六个目标的 `-o`，再加 `fact.c -E`）都在里面，后面 `fact.c` 又有 8 格 SAME。LEFT 130。切片自己退出 2，是 18 秒到了，不是有一格不同。

同一只已装产品，外层 `tools/bound 55`，`UNISACC_RR_TIME=1`，32 个源、不带 `hostnative.cx`，`-b win/x86_64 -o out/grk-win-o/slice-win-x86_64`。墙钟 13.761 秒，退出 1。units 推理退出 0，e3 推理退出 1。标准错误三条，没有行号：

```
unisacc: error: undefined function 'readlink'
unisacc: error: undefined function 'stat'
unisacc: error: undefined function 'getuid'
```

这三处调用是 `src/host.cx:80` 的 `readlink("/proc/self/exe")`、`cx/sys/fs.cx:154` 的 `stat`（`sy_stat`）、`cx/sys/env.cx:49` 的 `getuid`（`sy_uid`）。没有文件，没有 PE。这一次没有再改。

## readlink 和 getuid 收进帮手（2026-10-30）

`78bc1b53c4` 是 `cx/sys/env.cx`、`fs.cx`。`sy_self_path` 在 Linux 上读 `/proc/self/exe`，在 Windows 上经 `_ux_sym` / `_ux_call` 调 `GetModuleFileNameA`。`sy_stat` 的 Windows 分支用 `GetFileAttributesExA` 填 size、mtime、kind，ino 和 dev 填 0，注释写明 Windows 上不能用 `(dev, ino)` 判断是不是同一个文件。`sy_uid` 在 Windows 上返回 0，缓存目录起名用这个数，固定值够了。导出函数没有 `#if`。`6c3ede8ea7` 里 `src/host.cx` 的 `hs_self` 改调 `sy_self_path`，函数里声明，没有 `use sys.fs`。

`check-cx-sys` 是 103/0。`check-cx-sys-net`、`spawn`、`ffi`、`guard` 都是 failed=0。

对上这刀源码的 Linux 候选是 `8660d14d8dbddd223003647b232099efd334a1783ef2059cd4b254e6cda01487`，2980148 字节。runcache 12/0，冷 576 ms，命中 41 ms。对拍前 10 格 SAME。切片写出 21 格全 SAME，LEFT 127，退出 2。这只二进制后来被下一刀覆盖，sha 留在这里。

同一只已装产品，外层 `tools/bound 55`，`UNISACC_RR_TIME=1`，32 源，`-b win/x86_64 -o out/grk-win3-o/slice-win-x86_64`。墙钟 13.745 秒，退出 1。units 推理退出 0，e3 推理退出 1。标准错误只剩一条，没有行号：

```
unisacc: error: undefined function 'stat'
```

`readlink` 和 `getuid` 没有了。没有文件，没有 PE。

## 再把 lstat 收进去（2026-10-30）

前导里 Windows 的 `lstat` 就是去调 `stat`。`sy_lstat`、`sy_exists` 的 Windows 分支改走 `sy_stat` 那条属性读取，不报 kind `'l'`。`hs_write_out` 改调 `sy_is_reg`，不再写 `lstat`。Linux 上仍是 `lstat`。

Linux 候选 `out/grk-win3/candidate` 2980148 字节，sha256 `ea57700cfe910f73ab32344eaf922ad008964e4bc12b7f581079fd07127feb3f`。`host.o` 里 `sy_self_path` 和 `sy_is_reg` 是 U，`drive.o` 里各有一个 T，`sy_fstat` 仍只有一份 T。runcache 12/0，冷 578 ms，命中 41 ms。对拍前 10 格 SAME。切片写出 23 格全 SAME，LEFT 125，退出 2。

再跑一次 32 源，`-o out/grk-win3-o2/slice-win-x86_64`。墙钟 13.840 秒，退出 1。units 推理退出 0，e3 推理退出 1。标准错误仍是一条：

```
unisacc: error: undefined function 'stat'
```

这时 `src` 里已经没有活的 `stat(` 或 `lstat(`。`fs.cx` 里剩下的三处都在非 Windows 的 `#else`。没有文件，没有 PE。没有再追下一处调用。
