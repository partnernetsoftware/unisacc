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
