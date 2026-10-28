# cx 模式

本篇是 cx 的现行阅读。仓库 [prd.md](../../prd.md) 只做索引。`gcc_cx.com` 与 `clang_cx.com` 的要求是 APE 六格，不是本机 shell。驱动运行时不调用 `unisacc.com -cx-plan`。

草案仍是 [docs/cx-spec-draft.md](../cx-spec-draft.md)。阶段表在 [plans/cx-modules.md](../../plans/cx-modules.md)。**论文：** 模块化＋内存安全归 [Paper D 意向书](../../research/paper-d-intent.md)；工程进度不冒称论文完成。

## `.com` 的要求：APE 六格

`gcc_cx.com` 和 `clang_cx.com` 必须是同一份六格 APE。六格是 `win`／`lnx`／`osx` × `x86_64`／`arm64`。打包进文件的是五份切片：`win/x86_64`、`lnx/x86_64`、`lnx/arm64`、`osx/x86_64`、`osx/arm64`。`win/arm64` 跑 `win/x86_64` 那份 PE。Unix 上 shell 按 `uname` 解开对应切片；Windows 上文件本身就是 PE。构建入口是 `sh ./unisacc.com -run tools/build-cxcc-ape.cx`。它调用 `ujs/product/ape-pack.cx`，布局与别的 `.com` 相同：MZ 头，DOS stub 里是 polyglot shell，切片 gzip 存在文件尾。

格子文件名是 `gcc_cx_{lnx,osx,win}{64,32}-{x86,arm}.{bin,exe}`，clang 用同一形状、前缀 `clang_cx`。Unix 扩展名是 `.bin`，Windows 扩展名是 `.exe`。打进 APE 的五份是 `gcc_cx_win64-x86.exe`、`gcc_cx_lnx64-x86.bin`、`gcc_cx_lnx64-arm.bin`、`gcc_cx_osx64-x86.bin`、`gcc_cx_osx64-arm.bin`（clang 同形）。`win/arm64` 不另存 `gcc_cx_win64-arm.exe`。`{32}`、`win32-x86`、`win32-arm` 与 `win64-arm` 只在本机构建器按 `uname` 或编译器宏取名时出现，不是额外切片。

每一格里面仍是 `-o` 宿主驱动：进程内按 `src/cxmod.c` 展开 `#pragma unisa use`，再交给该机 PATH 上的 gcc 或 clang。运行时不调用 `unisacc.com`。系统 gcc 与 clang 会丢掉不认识的 `#pragma unisa`，所以展开在驱动里。产物是普通 C，不授予已检查身份。

## 当前磁盘

仓库根的 `gcc_cx.com` 是 1606288 字节，`clang_cx.com` 是 1606304 字节。两份都是六格 APE，`file` 为 PE32+ console x86-64，文件头是 `MZ`，polyglot 文本含 `Linuxx86_64`、`Linuxaarch64`、`Darwinarm64`。两个文件被 git 忽略。每一格由 `tools/gcc_cx.cx` 或 `tools/clang_cx.cx` 加上 `src/cxmod.c` 编出，驱动用 `#pragma unisa use cxproc` 带上 `tools/cxproc.cx`。切片名是上一节那五份。这次 `sh ./unisacc.com -run tools/build-cxcc-ape.cx` 退出 0。构建自己的检查跑过 Linux x86-64：无参数退出 2，gcc 编过 lex，模块夹具退出 42，clang 在本机编过 lex。`win/x86_64`、`lnx/arm64`、`osx/x86_64`、`osx/arm64` 只写出，没有在对应系统上执行。`win/arm64` 用 `win/x86_64` 那份 PE。

没有 `use` 时，一次执行系统编译器：`-std=c99 -x c -include include/cx.h`。

## 进程 API

`tools/cxproc.cx` 是按需模块。不写 `#pragma unisa use cxproc` 的程序不依赖它。导出的是 `cxproc_run`。`posix_spawnp` 的原型在 static 函数体内，不在没有函数体的文件作用域声明上。Linux 上，调用者传入的环境交给子进程；空指针换成只以 NULL 结尾的空表，不是继承。驱动自己在 Linux 上传 `environ`。Windows 上把空指针原样传入，CreateProcessA 把它当作继承；驱动在 Windows 上传空指针。这份 Windows 行为编进 `win/x86_64` 切片，没有执行。这个模块的 `.cx` 源没有 `#include`。前奏头文件没有删，也没有改 `include/cx.h`。

模块 `#include` 是 lint。不在对接前缀里的 `.cx` 新文件必须为 0；`tests/cxinclude.tsv` 旧债只降不升。`include/` 与 `tools/cxcc-hostinc/` 是 libc／POSIX 对接单元，可以用 `#include`。短记 [plans/cx-include-lint.md](../../plans/cx-include-lint.md)。检查是 `sh ./unisacc.com -run tests/cxinclude.cx -- --check`。树上若有别人暂存的新 `#include`，该检查退出 1，这不表示棘轮被抬高。

包清单把导入身份钉到路径。一个目录一份 `cxpack.tsv`：模块名对应旁边的文件，公开列是该文件非 static 定义的有序子集。短记 [plans/cx-pack.md](../../plans/cx-pack.md)。检查是 `sh ./unisacc.com -run tests/cxpack.cx -- --fixtures`。`src/cxmod.c` 在模块文件同目录有这份清单、且某一行的路径对上该文件时，CXIF 只写出公开列里的函数；没有对上的行仍导出全部非 static。本机用一份 Linux x86-64 探针编过这个源码：公开调用退出 5，调用未公开的 `pack_extra` 被 gcc 拒绝，没有清单的模块夹具退出 42。仓库根的 `.com` 和已安装的 `unisacc.com` 仍是上一份，这次没有重打，所以它们还导出全部非 static。

CXIF v0 文本是 banner、守卫、类型、原型和 `#endif`。短记 [plans/cxif-v0.md](../../plans/cxif-v0.md)。检查是 `sh ./unisacc.com -run tests/cxif.cx -- --fixtures`。

## 还没做

已安装的 `unisacc.com` 与仓库根 `.com` 仍导出每一个非 static 函数。源码里的裁剪要等这两份重新构造才进入它们。

`#include` 不是编译器拒绝。`src/cxmod.c` 仍编译带 `#include` 的模块。

spec §5 的所有权没有实现。

宿主 `-run` 没有实现。把 `-run` 传给 `gcc_cx` 或 `clang_cx` 会进系统编译器。

四份非本机切片已写进 APE，没有在 Windows 或 OSX 上执行。

## 下一动作

公开集裁剪已经写进 `src/cxmod.c`，并用一份 Linux 探针跑过。仓库根的六格 APE 这次没有重打。下一步是让已安装的 `unisacc.com` 带上这份源码；所有权仍未实现。

## 索引

| 页 | 内容 |
|---|---|
| [plans/cx-modules.md](../../plans/cx-modules.md) | 阶段表 CX0–CX4 |
| [plans/cx-include-lint.md](../../plans/cx-include-lint.md) | 模块 `#include` 只降不升 |
| [plans/cx-pack.md](../../plans/cx-pack.md) | 包清单与导入身份 |
| [plans/cxif-v0.md](../../plans/cxif-v0.md) | CXIF v0 文法 |
| [docs/cx-spec-draft.md](../cx-spec-draft.md) | 草案，含尚未实现的所有权 |
