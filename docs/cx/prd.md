# cx 模式

本篇是 cx 的现行阅读。仓库 [prd.md](../../prd.md) 只做索引。2026-10-26 前后写在产品叙事里的六目标 APE、以及「驱动调用 `unisacc.com -cx-plan`」，不是当前启动方式。

草案仍是 [docs/cx-spec-draft.md](../cx-spec-draft.md)。阶段表在 [plans/cx-modules.md](../../plans/cx-modules.md)。

## 当前模式

Linux 上 `gcc_cx` / `clang_cx` 是 `-o` 宿主驱动。仓库根的 `gcc_cx.com` 与 `clang_cx.com` 是 shell，启动 `out/cxcc-host/` 里的本机 ELF。有 `#pragma unisa use` 时，驱动在进程内按 `src/cxmod.c` 展开，再交给 PATH 上的系统 gcc 或 clang。驱动不调用 `unisacc.com`。系统 gcc 与 clang 会丢掉不认识的 `#pragma unisa`，展开发生在驱动里。产物是普通 C，不授予已检查身份。

没有 `use` 时，一次执行系统编译器：`-std=c99 -x c -include include/cx.h`。

模块 `#include` 是 lint。不在对接前缀里的 `.cx` 新文件必须为 0；`tests/cxinclude.tsv` 旧债只降不升。`include/` 与 `tools/cxcc-hostinc/` 是 libc／POSIX 对接单元，可以用 `#include`。短记 [plans/cx-include-lint.md](../../plans/cx-include-lint.md)。检查是 `sh ./unisacc.com -run tests/cxinclude.cx -- --check`。树上若有别人暂存的新 `#include`，该检查退出 1，这不表示棘轮被抬高。

包清单把导入身份钉到路径。一个目录一份 `cxpack.tsv`：模块名对应旁边的文件，公开列是该文件非 static 定义的有序子集。短记 [plans/cx-pack.md](../../plans/cx-pack.md)。检查是 `sh ./unisacc.com -run tests/cxpack.cx -- --fixtures`。

CXIF v0 文本是 banner、守卫、类型、原型和 `#endif`。短记 [plans/cxif-v0.md](../../plans/cxif-v0.md)。检查是 `sh ./unisacc.com -run tests/cxif.cx -- --fixtures`。

## 还没做

编译器仍导出每一个非 static 函数。包清单里的公开集不会裁剪 CXIF。

`#include` 不是编译器拒绝。`src/cxmod.c` 仍编译带 `#include` 的模块。

spec §5 的所有权没有实现。

宿主 `-run` 没有实现。把 `-run` 传给 `gcc_cx` 或 `clang_cx` 会进系统编译器。

当前启动器不是六目标 APE。本机只跑过 Linux。Windows／OSX 的对接头在 `tools/cxcc-hostinc/`，这里没有执行。

## 下一动作

让编译器按包清单的公开集裁剪 CXIF：未列入公开集的非 static 函数不再出现在接口里。这一步还没做，本篇不改 `src/cxmod.c`。

## 索引

| 页 | 内容 |
|---|---|
| [plans/cx-modules.md](../../plans/cx-modules.md) | 阶段表 CX0–CX4 |
| [plans/cx-include-lint.md](../../plans/cx-include-lint.md) | 模块 `#include` 只降不升 |
| [plans/cx-pack.md](../../plans/cx-pack.md) | 包清单与导入身份 |
| [plans/cxif-v0.md](../../plans/cxif-v0.md) | CXIF v0 文法 |
| [docs/cx-spec-draft.md](../cx-spec-draft.md) | 草案，含尚未实现的所有权 |
