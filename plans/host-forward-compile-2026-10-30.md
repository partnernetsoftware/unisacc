# 宿主转发和单独编译（2026-10-30）

只查，不改源。候选是 `out/grk-modid/candidate`，sha256 `d903d2258641dc17141cef736f36b3fc8501b169ce704f6d81923eaa1dc7bb88`，2976044 字节。同一组小例子在已装产品上也对过。

## 结论

这是现在的设计，不是缺了声明。

宿主调用的桩只在做整程序镜像时生成。`-run` 走 mode 3。`-o` 走 mode 0，目标是 `lnx/`、`osx/` 或 `win/` 才建桩。`src/drive.cx` 第 1535 行写明：对象、tape 和 `-E` 不进这条路。

`-S` 和 `-c` 在 `cx/driver/cli.cx` 第 405 行先把 mode 设成 2（tape）。只有带了 `-b`，并且 `-c` 要写对象、或 `-S` 的输出文件名以 `.s` 结尾，`cx/driver/route.cx` 的 `cxt_object`（第 329 行、第 339 行）才改成 mode 5。不带 `-b` 时一直是 tape。tape 不建桩，于是已有原型的 `setpgid`、`clock_gettime` 被报成 `undefined function`。

## 实跑

小例子在 `out/grk-fwd-probe/`（不提交）。`setpgid.cx` 只有原型和一次调用。`clock.cx` 同样，没有包含 `time.h` 里那个带函数体的 static 包装。`runv.cx` 包含 `cx.h` 并调用 `cx_runv_timeout`。

| 命令 | 候选 | 结果 |
|---|---|---|
| `-S FILE -o /dev/null` | `setpgid.cx`、`clock.cx`、`runv.cx`、`tools/refactor.cx` | 退出 1。`refactor.cx` 的第一句是 `undefined function 'setpgid'`。`runv.cx` 指到 `cx-flat.h` 里 `cx_runv_timeout` 的 `setpgid(0, 0)` |
| `-c FILE -o /dev/null`（没有 `-b`） | `setpgid` 这一类、`runv.cx`、`tools/refactor.cx` | 退出 1，同一句 `undefined function 'setpgid'` |
| `-c -b lnx/x86_64 -o FILE.o` | `setpgid.cx`（带和不带 `-funit`）、`clock.cx`、`runv.cx`（带和不带 `-funit`）、`tools/refactor.cx`（带 `-funit`） | 退出 0，写出对象。`nm` 里 `setpgid`、`clock_gettime` 是 `U`。`refactor.o` 1248096 字节 |
| `-o FILE.elf` 从源码 | `setpgid.cx`、`clock.cx` | 退出 0，写出镜像 |
| `-o` 去链接 `setpgid.o` | 候选 | 退出 1，`link: undefined reference to setpgid` |
| `-S -b lnx/x86_64 -o FILE.s` | `setpgid.cx`、`clock.cx` | 退出 2，`route=lnx/x86_64/object/O0`，`NOT_IMPLEMENTED object`。这是汇编文本那条，和上面的 undefined function 不是同一句 |

已装产品对 `setpgid.cx` 的 `-S -o /dev/null` 同样退出 1；`-c -b lnx/x86_64 -o FILE.o` 同样退出 0。

## 单独编译检查

要检查「这份源码能不能编」，用：

```text
sh ./unisacc.com -c -b lnx/x86_64 -o OUT.o FILE.cx
```

`tools/refactor.cx` 在候选上用了 `-funit`，退出 0。一个文件自己把 `cx_runv_timeout` 叫进来时，不带 `-funit` 也退出 0。

这只对象不能再拿去链接成能跑的程序。桩不在对象里。能跑的程序仍然从源码 `-o`，或者 `-run`。

`-S` 不能当这个检查。`-o /dev/null` 留在 tape，报 undefined function。`-o FILE.s` 停在 `NOT_IMPLEMENTED object`。
