# cx 包清单与导入身份

一个目录一份 `cxpack.tsv`。每行是 `模块名<TAB>路径<TAB>公开函数`。

- 路径必须等于模块名把 `.` 换成 `/` 再加 `.cx`。这与编译器 `#pragma unisa use` 在使用者旁边找文件相同。`std` 与 `std.cx` 是前奏，不进表。
- 公开函数按名字排序，每个都是该文件的非 static 定义。static 函数是文件内的，写进公开列就拒绝。
- `use` 只能点到表里的另一行，或前奏。
- 跨模块调用时，该函数必须在提供方的公开列里，并且调用方写了对应的 `use`。

`sh ./unisacc.com -run tests/cxpack.cx -- --fixtures` 跑正例和四个反例，并在 /tmp 拒绝一份带 include 的模块。正例 `tests/cxpack/ok/main.cx` 的 `-run` 退出 5。

这不是 spec §5 的编译期所有权。CXIF v0 仍导出全部非 static 定义。`tests/cxpack/ok/mark.cx` 的 `-cx-interface` 写出 `pack_extra` 与 `pack_mark`，没有 `pack_hide`。清单只公开 `pack_mark`。`tests/cxpack/bad-call/main.cx` 调用 `pack_extra`，lint 退出 1，同一文件 `-run` 仍退出 2。密封与晋升不读这张表。
