# cx 模式

本篇是 cx 的现行阅读。仓库 [prd.md](../../prd.md) 只做索引。`gcc_cx.com` 与 `clang_cx.com` 的要求是 APE 六格，不是本机 shell。驱动运行时不调用 `unisacc.com -cx-plan`。

草案仍是 [docs/cx-spec-draft.md](../cx-spec-draft.md)。阶段表在 [plans/cx-modules.md](../../plans/cx-modules.md)。**论文：** 模块化＋内存安全归 [Paper D 意向书](../../research/paper-d-intent.md)；工程进度不冒称论文完成。

## `.com` 的要求：APE 六格

`gcc_cx.com` 和 `clang_cx.com` 必须是同一份六格 APE。六格是 `win`／`lnx`／`osx` × `x86_64`／`arm64`。打包进文件的是五份切片：`win/x86_64`、`lnx/x86_64`、`lnx/arm64`、`osx/x86_64`、`osx/arm64`。`win/arm64` 跑 `win/x86_64` 那份 PE。Unix 上 shell 按 `uname` 解开对应切片；Windows 上文件本身就是 PE。构建入口是 `sh ./unisacc.com -run tools/build-cxcc-ape.cx`。它调用 `ujs/product/ape-pack.cx`，布局与别的 `.com` 相同：MZ 头，DOS stub 里是 polyglot shell，切片 gzip 存在文件尾。

格子文件名是 `gcc_cx_{lnx,osx,win}{64,32}-{x86,arm}.{bin,exe}`，clang 用同一形状、前缀 `clang_cx`。Unix 扩展名是 `.bin`，Windows 扩展名是 `.exe`。打进 APE 的五份是 `gcc_cx_win64-x86.exe`、`gcc_cx_lnx64-x86.bin`、`gcc_cx_lnx64-arm.bin`、`gcc_cx_osx64-x86.bin`、`gcc_cx_osx64-arm.bin`（clang 同形）。`win/arm64` 不另存 `gcc_cx_win64-arm.exe`。`{32}`、`win32-x86`、`win32-arm` 与 `win64-arm` 只在本机构建器按 `uname` 或编译器宏取名时出现，不是额外切片。

每一格里面仍是 `-o` 宿主驱动：进程内按 `src/cxmod.c` 展开 `#pragma unisa use`，再交给该机 PATH 上的 gcc 或 clang。运行时不调用 `unisacc.com`。系统 gcc 与 clang 会丢掉不认识的 `#pragma unisa`，所以展开在驱动里。产物是普通 C，不授予已检查身份。

## 当前磁盘

仓库根的 `gcc_cx.com` 与 `clang_cx.com` 现在是 shell，分别 `exec` `out/cxcc-host/gcc_cx_lnx64-x86.bin` 与 `out/cxcc-host/clang_cx_lnx64-x86.bin`。这两个 `.com` 被 git 忽略。它们不满足上面的六格要求。本机只跑过这个 Linux x86-64 入口。Windows／OSX 的对接头在 `tools/cxcc-hostinc/`，没有在对应格子上执行。

没有 `use` 时，一次执行系统编译器：`-std=c99 -x c -include include/cx.h`。

模块 `#include` 是 lint。不在对接前缀里的 `.cx` 新文件必须为 0；`tests/cxinclude.tsv` 旧债只降不升。`include/` 与 `tools/cxcc-hostinc/` 是 libc／POSIX 对接单元，可以用 `#include`。短记 [plans/cx-include-lint.md](../../plans/cx-include-lint.md)。检查是 `sh ./unisacc.com -run tests/cxinclude.cx -- --check`。树上若有别人暂存的新 `#include`，该检查退出 1，这不表示棘轮被抬高。

包清单把导入身份钉到路径。一个目录一份 `cxpack.tsv`：模块名对应旁边的文件，公开列是该文件非 static 定义的有序子集。短记 [plans/cx-pack.md](../../plans/cx-pack.md)。检查是 `sh ./unisacc.com -run tests/cxpack.cx -- --fixtures`。

CXIF v0 文本是 banner、守卫、类型、原型和 `#endif`。短记 [plans/cxif-v0.md](../../plans/cxif-v0.md)。检查是 `sh ./unisacc.com -run tests/cxif.cx -- --fixtures`。

## 还没做

编译器仍导出每一个非 static 函数。包清单里的公开集不会裁剪 CXIF。

`#include` 不是编译器拒绝。`src/cxmod.c` 仍编译带 `#include` 的模块。

spec §5 的所有权没有实现。

宿主 `-run` 没有实现。把 `-run` 传给 `gcc_cx` 或 `clang_cx` 会进系统编译器。

六格 APE 还没按现行驱动重打。磁盘上的 `.com` 仍是 Linux shell，指向 `gcc_cx_lnx64-x86.bin` 与 `clang_cx_lnx64-x86.bin`。

## 下一动作

用 `tools/build-cxcc-ape.cx` 按现行 `tools/gcc_cx.cx`、`tools/clang_cx.cx` 和 `tools/cxcc.cx` 重打两份六格 APE，替换仓库根的 shell。切片名用上面的五份。本机只执行 Linux x86-64 那一格；其余四份切片写出即可。这一步还没做。

公开集裁剪仍未做：未列入 `cxpack.tsv` 的非 static 函数还在 CXIF 里。那是下一件编译器的事，本篇不改 `src/cxmod.c`。

## 索引

| 页 | 内容 |
|---|---|
| [plans/cx-modules.md](../../plans/cx-modules.md) | 阶段表 CX0–CX4 |
| [plans/cx-include-lint.md](../../plans/cx-include-lint.md) | 模块 `#include` 只降不升 |
| [plans/cx-pack.md](../../plans/cx-pack.md) | 包清单与导入身份 |
| [plans/cxif-v0.md](../../plans/cxif-v0.md) | CXIF v0 文法 |
| [docs/cx-spec-draft.md](../cx-spec-draft.md) | 草案，含尚未实现的所有权 |
