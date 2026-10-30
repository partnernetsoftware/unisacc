# 参考 C

这些文件从活路径挪过来，只供对照。编译器入口在 `src/unisacc.cx`。`cx/` 是架构。不要再往这里加逻辑。

## cx/compiler 的 gate 模块（2026-10-30 归档）

`cx/compiler/` 下 34 个从参考 C 抽出的判断模块（bind*、*gate、front*、back*、*more 等）与只测它们的
`tools/check-compiler-units.cx`。它们不在新入口 `src/unisacc.cx`、`src/entryexec.cx`、`src/hostnative.cx`
的 use 闭包里，入口需要的部分已在 `src/entrybind.cx` 自有。仍被活代码使用的 codecgate、leword、libgate、
libsig、libtype、boundgate、tapegate、tapelink、hostbind、bindstage 等留在原处。旧构造清单
（exec/c/buildcompiler.sh、tools/cx-driver-build.cx 等）本来就因参考 C 归档而不可用，未改。
