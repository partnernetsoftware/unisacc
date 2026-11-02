# cx 模块 `#include`：旧债只降，新债硬拒

`tests/cxinclude.cx --check` 与 `--write` 都读 `tests/cxinclude.tsv`。

- 模块 `.cx`（不在 `tests/cxinclude-boundary.tsv` 前缀下）不许新增 `#include`。没有基线行就按 0。计数高于本行则退出 1。
- `--write` 只在全部模块计数不高于基线时重写该表，用来记下下降和消失的行。有新债时表的字节不动。
- `include/` 与 `tools/cxcc-hostinc/` 是对接 libc／POSIX 的 C 头，`#include` 允许，不进这张表。

这不是编译器里的严格模式。`src/cxmod.c` 仍不因 `#include` 拒绝编译。产品候选的密封与晋升也不读这张表。
