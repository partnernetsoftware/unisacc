# osx / win 的 -c：只查，不改（2026-10-30）

这一页只记录查到的事实。没有改编译器，没有写对象，没有选路。路交给 cc 和主人定。

查的是候选 `out/grk-host-sys/candidate`，sha256 `9fb93c137363b210295870853fcf01b6693414270bc57f5c1438779c197535bf`。两边都去掉了 `UNISA_CONTAINER` 和 `UNISACC_CONTAINER`。样例是 `examples/hello.c`。

## 拒绝在哪

`src/driver/route.cx` 和 `cx/driver/route.cx` 逐字节相同。

不带 `-funit` 的 `-c -b osx/x86_64` 和 `-c -b win/x86_64` 都退出 1，没有写出 `.o`。句子是：

```
unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
```

发出这句话的是 `cxt_object`，`src/driver/route.cx` 第 336 行。条件是：要写对象、目标不是 `lnx/` 开头、并且没有 `-funit`。第 215 行 `cxt_linux` 只认 `lnx/`。第 43 行是这句说明，不是执行点。已装产品那次四目标探针打出的就是这一句。

`tools/entry-build.cx` 编对象时带 `-funit`。当前这只候选上，`-c -funit -b osx/x86_64` 和 `-c -funit -b win/x86_64` 都不走第 336 行。它们退出 2，标准错误是：

```
route=osx/x86_64/object/O0
NOT_IMPLEMENTED object
```

win 那次把第一行换成 `route=win/x86_64/object/O0`。打印在 `src/drive.cx` 第 1938 行和第 1939 行（`pr == 3`，模型没有这一步）。`hello.c` 没有给 `-O`，所以路由是 `O0`。entry-build 会要 `O1` 或 `O2`。两种都没有写出 `.o`。

## 单文件 -o 能出镜像

不经过 `-c`。两次都退出 0，标准错误是空的。

| 命令 | 墙钟 | 字节 | 开头 |
|---|---|---|---|
| `-b osx/x86_64 -o hello-osx examples/hello.c` | 0.616 秒 | 66274 | `cf fa ed fe`，Mach-O 64 位，cpu `0x01000007`（x86_64），filetype 2，也就是 MH_EXECUTE |
| `-b win/x86_64 -o hello-win examples/hello.c` | 0.575 秒 | 37376 | `4d 5a`，偏移 64 处是 `PE\0\0` |

这是可执行镜像，不是可重定位对象。filetype 2 不是 MH_OBJECT（那是 1）。

33 个入口源一次 `-o -b osx/x86_64` 没有写出文件。7.543 秒时停在 `src/hostnative.cx` 第 8 行：`#error hostnative_requires_linux_x86_64_or_arm64`。退出码 1。所以没有「整只成功 -o」的耗时，不能把 7.5 秒当成低于 55 秒的成功。

同一天 lnx/arm64 的 33 个 `-c` 对象墙钟加起来是 46.433 秒，链接再 7.736 秒，合计 54.169 秒。那是多次进程，而且是 Linux 对象，不是 osx/win 的一次 `-o`。一次进程如果把同样的活做完，会贴着 55 秒。这次没有跑完，不能再往下说。

## 对象写出器在哪，缺多少

可执行镜像（上面的 Mach-O MH_EXECUTE 和 PE）已经由模型路由写出。缺的是对象。

参考编译器的对象写出在已归档的 `archive/reference-c/src/back_image.c`，产品不编译这个文件。

- `bk_machoobj` 第 417–493 行，77 行。MH_OBJECT，一节未命名段，`__text` / `__data` / `__bss`。旁边还有 `bk_oname_w`、`bk_mrel`、`bk_msym`。
- `bk_coffobj` 第 509–602 行，94 行。三个节，导入符号是 `__imp_Name`。旁边还有 `bk_cname`、`bk_csym`。
- `bk_object` 第 603–615 行按目标挑这三个。目标以 `o` 开头走 Mach-O，以 `w` 开头走 COFF，否则 ELF。
- 这三个函数前面还要用同一文件里的 `bk_symplan`、`bk_lower`、`bk_assemble`。只把 77 行和 94 行搬过来不够。

产品现在的 Linux 对象不走这个 C 文件，走模型路由 `lnx/.../object/O*`。osx 和 win 的对应路由在 `src/drive.cx` 第 1939 行被报成 `NOT_IMPLEMENTED object`。模型侧没有这两条对象路由。归档的 C 写出器也没有接回活路径。

两条可能的路，这一页不定：

1. 非 Linux 不用 `-c`。整只 `-o` 已经能出 Mach-O 和 PE，但 33 个源现在会被 `hostnative.cx` 第 8 行拒绝，成功时的墙钟也还没量到。Linux 那 33 只对象加链接是 54.2 秒，贴着 55 秒。
2. 补 `-c`。要有 `osx/*/object` 和 `win/*/object` 的模型步，或者把归档的 `bk_machoobj` / `bk_coffobj` 接回活路径。驱动在带 `-funit` 时已经把路由名算出来了，缺的是后面那一步。
