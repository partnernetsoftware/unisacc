# c43 缺陷分诊（cc，2026-10-28 18:1x；fable 裁定 D；来源 plans/defects-c43.md，复现入口均为红批）

级别：S1 静默接受错误程序并落盘对象/镜像 > S2 崩溃镜像 > S3 拒绝无名或错因 > S4 诊断文案。
复现：在冻结导出（产品 f3705dfa）`sh ./unisacc.com tools/<批>.cx -- ROOT ABS_OUT [HOST]`；行名即用例。宿主工具：`/usr/bin/x86_64-linux-gnu-gcc-11`。

| 级 | # | 现象（最小输入） | 条款 | 域/位置 | 批.行 | 现况 → 预期 |
|---|---|---|---|---|---|---|
| S1 | 1 | `int *p = 5;` | 6.7.8p11 | E3/parse2 | c-negative.int-to-pointer-init | rc0+.o → 具名拒绝 |
| S1 | 2 | `const int k; k=2;` / `c[0]='y'` 经 const 指针 | 6.5.16p2 | E3 | c-negative.assign-to-const / write-through-const-pointer | rc0 → 拒绝 |
| S1 | 3 | `int *p; char *c = p;` / `p == q` 不兼容指针 | 6.5.16.1p1 / 6.5.9p2 | E3 | c-negative.incompatible-pointer-init / distinct-pointer-compare | rc0 → 拒绝 |
| S1 | 4 | `return a;`（int[] 当 int） | 6.8.6.4p3 | E3 | c-negative.return-pointer-as-int | rc0 → 拒绝 |
| S1 | 5 | `case 1:` 在 switch 外 / `case 1: case 1:` | 6.8.1p2 / 6.8.4.2p3 | E3 | c-negative.case-outside-switch / duplicate-case | rc0 → 拒绝 |
| S1 | 6 | `struct S` 同作用域重定义；`typedef int T; typedef char T;`；`int n(int); char n(int);`；`enum{A}; enum{A};` | 6.7.2.3p1 / 6.7p3 / 6.7p4 | E3 | c-negative.struct-redefinition / conflicting-typedef / conflicting-function-types / enumerator-redeclared | rc0 → 拒绝 |
| S1 | 7 | `int x=1; int x=2;` 同块；`void f(int a,int a)` | 6.7p3 / 6.9.1p9 | E3 | c-negative.local-redefinition / duplicate-parameter | rc0 → 拒绝 |
| S1 | 8 | `k2(1,2)` 原型一参 | 6.5.2.2p2 | E3 | c-negative.too-many-args-prototype | rc0 → 拒绝 |
| S1 | 9 | `x = (void)0;` / `return a18();`（void） | 6.5.16.1p1 / 6.8.6.4p1 | E3 | c-negative.void-value-used / void-call-as-value | rc0 → 拒绝 |
| S1 | 10 | `i += "str"` / `a[1.5]` / `int *p=&c`（const） | 6.5.16.2p1 / 6.5.2.1p1 / 6.7.8p11 | E3 | c-negative.int-plus-assign-string / float-subscript / discards-const | rc0 → 拒绝 |
| S1 | 11 | `int t[0];` / `void f(void x)` / `static extern int u;` | 6.7.5.2p1 / 6.7.5.3p10 / 6.7.1p2 | E3 | c-negative.zero-array-size / void-parameter-named / multiple-storage-classes | rc0 → 拒绝 |
| S1 | 12 | `#include <stdio.h`（未闭合，静默包含 stdio.h） | 6.4.7 / 6.10.2 | E2 | pp-negative.header-name-unclosed | rc0 → 拒绝 |
| S1 | 13 | `#define 3x 1` | 6.10.3 | E2 | pp-negative.macro-name-not-identifier | rc0 → 拒绝 |
| S2 | 14a | 未定义外部函数：直接 C 路线 `-o img main.c`（对象/归档/跨归档三例已在 Root 4c2831c3 修复，896888f424） | prd.md:771 R20-3 | exec/c 直接 C 路线（不经 tapelink 检查） | link-undefined-function.direct-c-route（4c2831c3 上 4/1） | rc0+SIGSEGV 镜像 → 具名拒绝 |
| S2 | 14b | `-run noret.tape`（_start 无 ret 落出末尾）产品进程 SIGSEGV | 有界终止合同 | 运行器 | link-malformed-input.run.tape-falls-off-end | rc -11 无 stderr → 具名拒绝/有界终止 |
| S1 | 14c | ELF 魔数覆盖 / 截断到 2000 字节的单元对象被链接成镜像 | prd.md:771 | compiler.c tl_tape 定位 | link-malformed-input.object.bad-elf-magic / truncated-2000 | rc0+镜像 → 具名拒绝 |
| S3 | 15 | `-run u.o`（单元对象） | CLAUDE.md 具名拒绝 | exec/c/compiler.c 驱动 | driver-diagnostics.run.unit-object.named | rc1 **stderr 空** → 具名 |
| S3 | 16 | `-run ok.a` / `-o p ok.a`（仅归档） | 同上 | 驱动 | driver-diagnostics.run.archive-alone / link.archive-alone | 错因文本 → 具名（无 main/无单元） |
| S3 | 17 | `ar zz empty.a` | 同上 | compiler.c product_ar:445 | driver-diagnostics.ar.bad-op-empty | rc0 静默 → `ar: operation must be r, q, t or x` rc2 |
| S3 | 18 | `-o p a.o`（无 main） | 同上 | 驱动/tapelink | link-edge-cases.no-main（INFO） | `not covered: address or target declaration` → 具名 "undefined reference to main" |
| S3 | 19 | `int s2[-1]` | 6.7.5.2p1 | E3 | c-negative.negative-array-size（已拒） | 文本 `tape data directive` 错因 → 具名 |
| S4 | 20 | 四类 pp 拒绝（unterminated #if、参数个数、#if 表达式、未闭合宏调用）及 `int;`/`1 2` 等无 `file:line` | — | E2/E3 | pp-negative.*.diagnostic-has-file-line=0；c-negative 同名 INFO | 加 file:line |
| 观察 | 21 | `int a[(int)1.0+0];` 有效程序被拒 `not covered: constant expression` | 6.6p6 | E3 | defects-c43.md 观察 | 应接受 |
| 非缺陷 | 22 | 字面量合并指针相等（未指定）；`#foo` 非指令；`#define __STDC__` | — | — | — | 不改 |

未覆盖面（本轮未实跑，待 B 限量）：tapebin 损坏节 / 畸形 ELF 对象或单元 tape 输入链接器；`-S` 畸形 tape 文本；资源/包拒绝。

建议修复顺序：#14（补丁已拟，一处改动关四行）→ #12/#13（E2 两处词法约束）→ #1–#11（E3 类型/作用域约束，按表格顺序：先指针/const/void 语义类，再重定义类，再声明类）→ #15–#19 文案 → #20。
