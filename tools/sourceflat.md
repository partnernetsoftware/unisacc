# 完整 C 源导出

从默认产品运行，不需要 Python、AWK、Clang 或额外编译的辅助程序：

```
sh ./unisacc.com tools/sourceflat.cx -- out/unisacc-flat.c
sh ./unisacc.com tools/sourceflat.cx -- --verify out/unisacc-flat.c
```

输出包含完整独立 C 源与相邻的 `.source.json`。源文件展开规则保留旧 `export_ref.sh` 合同：递归替换整行的 `#include "relative-path"`（允许前后空格或制表符），同一文本路径只输出一次，其他行逐字节保留并补齐末尾换行。角括号 include、带注释的 include 和条件指令原样保留；这是一份有序源装配，不是通用 C 条件预处理器。

`unisacc-sourceflat-v2` 身份绑定实际递归输入、工具自身及随产品携带的 `include/` 文件。`source` 是域标记与有序 `路径 NUL SHA256十六进制 LF` 的 SHA256；`flat` 是最终 C 字节的 SHA256；`inputs` 列出相对路径及摘要。它与旧 Python 身份算法不同，不能混用旧侧车。`--verify` 重新导出到内存并严格比较当前源、输出字节和侧车，不写文件。

导出拒绝覆盖任何输入（包括规范源及其符号链接、硬链接）、读取失败、非普通输出、源漂移。输出的父目录须已经存在；只有 ENOENT 可以表示目标不存在。C 和 JSON 各自在同目录独占临时文件写完、同步、检查源摘要后原子替换。两个文件不是一次联合原子事务：若进程在两次替换之间中断，验证会拒绝不匹配的文件对。一般错误会清理本次临时文件；强制终止可能留下 `.tmp.*`，不把这些文件当有效输出。

旧新字节比较（旧入口归档后相应替换其路径）：

```
timeout 55 sh tests/export_ref.sh /tmp/sourceflat-old.c
timeout 55 sh ./unisacc.com tools/sourceflat.cx -- /tmp/sourceflat-new.c
cmp /tmp/sourceflat-old.c /tmp/sourceflat-new.c
timeout 55 sh ./unisacc.com tools/sourceflat.cx -- --verify /tmp/sourceflat-new.c
```

范围：本工具只导出经典参考编译器的独立 C 源，不构建编译器，不替代产品、七流十八网或跨平台功能验收。
