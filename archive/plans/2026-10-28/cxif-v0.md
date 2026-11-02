# CXIF v0 文本

`-cx-interface FILE.cx` 打印的就是这一份文本。文法以 `src/cxmod.c` 的生成器为准。

1. 首行 `/* CXIF v0 module STEM src-sha256 HEX16 -- generated, do not edit */`。`STEM` 是文件名去掉目录和 `.cx`。`HEX16` 是源码 sha256 的前 16 位。
2. 下一行 `#ifndef CXIF_STEM_HEX16`，再一行 `#define` 同一个守卫。
3. 中间先是类型（`struct` / `union` / `enum` / `typedef`，可以带花括号），再是函数原型，每个原型以 `;` 结束。
4. 末行 `#endif`。
5. 正文里没有 `static`，也没有函数体。

`tests/cxif/box.cx` 的实测正文是：

```
struct box_point { int x; int y; };
typedef int box_id;
int box_make(box_id n);
```

`box_hide` 是 static，不出现。`tests/cxpack/ok/mark.cx` 的正文是 `int pack_extra(int n);` 和 `int pack_mark(int n);`，没有 `pack_hide`。

`sh ./unisacc.com -run tests/cxif.cx -- --fixtures` 用现场的 `-cx-interface` 核这两份，并拒绝缺 `#endif`、带函数体、带 `static`、版本不是 v0 的文本。

这不是带效果、布局或所有权字段的下一版 schema。传递依赖用的「仅类型」文本（banner 里有 `types`）不在这份全接口检查里。密封与晋升不读这份文本。公开集仍由 `cxpack.tsv` 另记：v0 继续导出全部非 static 函数。
