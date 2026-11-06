# 对 gcc 的差分探测（2026-11-02，C44-DIFF-PROBE-01）

对象：现装产品 `14b906fd0541cee6…`（`sh ./unisacc.com FILE.c`）对 `cc -std=c99 -Wall -Wextra -fsanitize=undefined -lm`。
目的：找用户真会撞的**运行库 / 头文件**缺陷（快车道可修范围），不修，只交清单。

## 方法与约束

- 探针目录 `out/diff-probe/`（`out/` 忽略区）。31 个探针 + 7 个最小复现，全部合法 C99、无未定义行为；`-fsanitize=undefined` 全部干净（曾出现过的一条 qsort 空比较器已删，其覆盖由 `p09` 提供）。
- 驱动 `out/diff-probe/run.sh`：串行（并发 1 ≤ 2），每次运行 `prlimit --cpu=10`，不 `pkill`，每个探针比较 stdout 与退出码。
- 说明：标准 C 里 `strcmp` 返回值、`rand` 序列、`signal` 处理器是否重置、`(char)-1 < 0` 均属实现定义；这类差异不计缺陷，单列在第五节。

## 一、覆盖与一致面（31 个探针中 19 个完全一致，12 个差异落在 D1–D7 与两处实现定义上）

一致（SAME）的探针：`p01` printf 整数各格式与宽度/精度/长度修饰、`p06` math、`p07` string.h（含 strtok/strncat/strncpy/strpbrk）、`p08` strtol/strtoul/strtoll + atoi/atol、`p09` qsort/bsearch（int、结构体、字符串指针）、`p11` div/ldiv/lldiv/abs 族、`p12` ctype 全 0..127 表与 EOF、`p13` setjmp/longjmp 嵌套、`p15` time/clock/difftime、`p16` va_arg/vsnprintf、`p17` 结构体布局/offsetof/位域/union、`p18` 整数提升与转换、`p19` C99 语法（`//`、`long long`、`_Bool`、复合字面量、指定初始化、`restrict`、`inline`、for 内声明）、`p20` 浮点常量与比较、`p21` inttypes 宏、`p23` 按值传/返结构体与多参数 ABI、`p24` 数组/指针/递归、`p31` 二进制 IO、`p32` tmpfile/setvbuf/vfprintf/vsprintf/memchr/fflush。

## 二、差异清单

### D1 `putc` 未声明 —— 合法程序编译失败（头文件，快车道）

```c
#include <stdio.h>
int main(void) { FILE *f = fopen("/tmp/m01","w"); putc('A', f); fclose(f); remove("/tmp/m01"); return 0; }
```
gcc: 编译通过、rc 0。产品: `m01_putc.c:2:52: error: undefined function 'putc'`，rc 1。
`include/stdio.h` 有 `fputc`/`getc`/`getchar`/`putchar`/`ungetc`，唯独没有 `putc`（C99 7.19.7.8 要求）。
用户撞到可能性：**高**（`putc` 是最常用的写字符函数之一）。
归类：头文件（快车道）。修法：`static int putc(int __u_c, FILE *__u_f) { return fputc(__u_c, __u_f); }`。

### D2 printf 族不支持 `%n`，且把参数当数字打出来（运行库 printf，快车道）

```c
#include <stdio.h>
int main(void) { int n = -1; char b[16]; snprintf(b, sizeof b, "ab%n", &n); printf("n=%d [%s]\n", n, b); return 0; }
```
gcc：`n=2 [ab]`。产品：`n=-1 [ab1407347670415]`。
产品把 `%n` 当成未知转换，**把指针的十进制值打进输出**：`%n` 未生效（`n` 不动）、返回值多算、输出里泄露地址；`b` 只有 16 字节时 `p03` 里同一条还把 `snprintf` 的返回算成 18/21，超出 `b` 的长度，固定缓冲会溢出。
用户撞到可能性：中低（`%n` 少见，但库代码、日志、进度输出会用）。
归类：运行库（`include/stdio.h` 的格式化器，快车道）。

### D3 scanf 族不支持 `%[...]` 扫描集（运行库 scanf，快车道）

```c
#include <stdio.h>
int main(void) { char s[8]; int n = sscanf("abc123", "%[a-z]", s); printf("n=%d [%s]\n", n, n>0 ? s : "-"); return 0; }
```
gcc：`n=1 [abc]`。产品：`n=0 [-]`（一个字符都不吃，`s` 不变）。
`p27` 四种形态全红：`%[a-z]`、`%[^=]`、`%[a-z0-9]`、`%[A-Z]` 都返回 0；`sscanf("abc123","%[a-z]%d",…)` 产品 `n=0`，gcc `n=2`。
用户撞到可能性：**中高**（解析配置、CSV、逐字段读取的常见写法）。
归类：运行库（`include/stdio.h` 的 `vsscanf`，快车道）。

### D4 scanf 族不支持 `%n`（运行库 scanf，快车道）

```c
#include <stdio.h>
int main(void) { int a=0,k=-1; int n = sscanf("77","%d%n",&a,&k); printf("n=%d a=%d k=%d\n", n, a, k); return 0; }
```
gcc：`n=1 a=77 k=2`。产品：`n=1 a=77 k=-1`（静默跳过，不报错）。
用户撞到可能性：中低。
归类：运行库（同上，快车道）。可与 D3 同批修。

### D5 `strtod` 不认 `inf`/`infinity`/`nan`（运行库 strtod，快车道）

```c
#include <stdio.h>
#include <stdlib.h>
int main(void) { char *e; double d = strtod("inf", &e); printf("%.1f end=[%s]\n", d, e); return 0; }
```
gcc：`inf end=[]`。产品：`0.0 end=[inf]`（一点不吃，`end` 指向原串）。
C99 7.20.1.3 要求接受 `INF`/`INFINITY`/`NAN`（大小写不敏感，可带 `n-char-seq`）。`strtod("-inf")`、`"nan"`、`"infinity"` 同；`"0x1p3"` 十六进制浮点、`"1e-400"` 下溢设 `ERANGE` 都与 gcc 一致，说明只缺这三个特例。
用户撞到可能性：中（读配置/数据里的 `inf`、往返写读浮点）。
归类：运行库（`include/stdlib.h` 的 `strtod`，快车道）。

### D6 `NAN` 是负 NaN，`printf` 打出 `-nan`（头文件 math.h，快车道）

```c
#include <stdio.h>
#include <math.h>
int main(void) { printf("%f\n", NAN); return 0; }
```
gcc：`nan`。产品：`-nan`。同理 `-NAN` 在产品上打成 `nan`（符号反了）。
`include/math.h:20` 写的是 `#define NAN (0.0f / 0.0f)`，x86 上 `0.0/0.0` 生成符号位为 1 的默认 qNaN；glibc 的 `NAN` 是 `__builtin_nanf("")`（符号位 0）。影响：打印结果、`signbit(NAN)`、逐位比较/序列化。
用户撞到可能性：中低（打印 NaN 的数值程序）。
归类：头文件（快车道）。修法：让宏产出一个符号位为 0 的 qNaN（如 `#define NAN (-(0.0f / 0.0f))`，x86/ARM 上取反只翻符号位），并在 `p29` 上钉住。

### D7 `SIGUSR1`/`SIGUSR2` 未定义（头文件 signal.h，快车道）

```c
#include <stdio.h>
#include <signal.h>
int main(void) { printf("%d\n", (int)SIGUSR1); return 0; }
```
gcc：`10`。产品：`signal.h:3(...) error: unknown identifier`，rc 1。
`include/signal.h` 只有 SIGHUP/INT/QUIT/ILL/ABRT/FPE/KILL/SEGV/TERM/WINCH；`SIGUSR1`/`SIGUSR2` 缺。产品头注明自己是 POSIX.1-2008 核心头（`include/cx.h` CX1 版），这两个常量属 POSIX 基础部分。
用户撞到可能性：中（自唤醒、测试用信号、`SIGUSR1` 做日志轮转触发）。
归类：头文件（快车道）。注意与既有注释的一致性：该头明说"外部信号永不投递"，`raise` 路径仍要能走通，加常量后补一条 `raise(SIGUSR1)` 的探针。

## 三、最小复现位置

`out/diff-probe/m01_putc.c` … `m07_scanfn.c`，每个都可用
`cc -std=c99 -Wall -Wextra -fsanitize=undefined -o X m0N_*.c && ./X` 与
`cd /home/devali/repos/unisacc && sh ./unisacc.com out/diff-probe/m0N_*.c` 两条命令复现。
批量重跑：`sh out/diff-probe/run.sh`（串行、`prlimit --cpu=10`）。

## 四、按"可感知度 × 修复成本"的初排（供排序用）

| 序 | 缺陷 | 可感知度 | 修复成本（估计） | 车道 |
|---|---|---|---|---|
| 1 | D1 `putc` 缺声明 | 高 | 1 行 | 头文件/快 |
| 2 | D3 scanf `%[...]` | 中高 | 中等（`vsscanf` 加一个转换分支 + 边界） | 运行库/快 |
| 3 | D5 strtod 的 `inf`/`nan` | 中 | 小（前置串比较 + 十六进制已就绪） | 运行库/快 |
| 4 | D7 `SIGUSR1/2` | 中 | 小（两行宏 + 补 `raise` 探针） | 头文件/快 |
| 5 | D2 printf `%n` | 中低（但会溢出/泄露） | 中等（在格式化器里实现写入计数） | 运行库/快 |
| 6 | D4 scanf `%n` | 中低 | 小（与 D3 同处） | 运行库/快 |
| 7 | D6 `NAN` 符号 | 中低 | 1 行宏 | 头文件/快 |

## 五、实现定义差异（不计缺陷，建议只在文档里写清）

1. `rand()`：产品 `RAND_MAX = 32767`、自带 LCG（`srand(1)` → 16838…）；glibc `RAND_MAX = 2147483647`。C99 只要求 ≥32767，产品合规。若库用户按 32 位范围取模，结果会与 glibc 不同——这是 `known-gaps` 级别的说明，不是 bug。
2. `signal()` 处理器投递后是否保留：产品保留（BSD/glibc `-std=gnu99`），glibc `-std=c99` 走 SysV 语义会重置。标准两可；`p30` 的 `restore 0/1` 差异即此。
3. `printf` 的 `%p` 指针格式、`strcmp` 返回的具体数值：标准只约束"非零/符号"，不与之比对。
4. `(char)-1 < 0`、`(int)(signed char)200`：产品与 gcc 都按有符号 char，一致；写在文档里以免误判。

## 六、探测中被排除的"假阳性"（方法学记录）

- 字符串字面量地址相减（`strchr("abc",'c') - "abc"`）：字面量是否合并由实现决定，产品不合并 → 差异不是 strchr 的缺陷；改用数组变量后 `p07` 完全一致。
- 同一 `printf` 实参里同时调 `strtol`/`fopen` 和读其结果/`errno`：未指定求值顺序，gcc 与产品顺序不同会造出假差异；`p08` 改为先存临时变量后即完全一致（`p05` 唯一差异就是 D1 的 `putc`）。
- `%s` 传 `NULL`、`qsort` 空比较器、越界读：属未定义行为，直接删掉，不进清单。

## 七、修复落地与验证（2026-11-02，omp-ds 车道）

回归入口：`sh ./unisacc.com tools/check-libc-probes.cx -- ROOT`，七个用例各以 `.c` 与 `.cx` 两种输入各跑一次，期望值是 glibc 输出的字面量（断言写死在探针程序里，运行时不依赖宿主 gcc）。改前红、改后绿的证据是 `out/diff-probe/res/` 里的 `m0N_*.uni.out` 与本节表格。

| 缺陷 | 改动位置 | 修复 | 验证 |
|---|---|---|---|
| D1 `putc` | `include/stdio.h`（`fputc` 之后） | 受 `__UN_putc` 门控的 `static int putc(int, FILE *)`，转发 `fputc` | 源码改完仍红，且**不是**头文件写法的问题：名字表在 `unisacc.com.dev/p3-package.bin`（`grep -c __UN_fputc` 命中 2，重建前的包），`tools/entry-build.cx` 只重编对象、把现成 P3 包原样附在候选尾部，实测候选（`b39f3460…`）上 `putc` 仍然 undeclared —— 转绿要下一批合并候选生成的新 P3 包。`exec/facts/pp-autoinc-gen.tsv` 已重生成：`keys` 里有 `putc`、`bodies` 里有 `__UN_putc`，即新包一旦构造即命中。`check-libc-probes` 的 `putc/{c,cx}` 两行就是这道闸门 |
| D7 `SIGUSR1/2` | `include/signal.h` | `#define SIGUSR1 10` / `SIGUSR2 12` | `sigusr/{c,cx}` 绿（信号号 + `signal`/`raise` 走通） |
| D6 `NAN` 符号 | `include/math.h` | `#define NAN (-(0.0f / 0.0f))`，取反只翻符号位，与 glibc `__builtin_nanf("")` 一致 | `nan/{c,cx}` 绿：`%f`/`%g` 为 `nan`、`%F` 为 `NAN`、`signbit` 为 0 |
| D3 scanf `%[...]` | `include/stdio.h` `vsscanf` | 新增 scanset 分支：`^` 取反、首字符 `]` 是成员、`a-z` 区间、`-` 首尾是字面量、宽度上限、零长度视为匹配失败、不跳前导空白 | `scanset/{c,cx}` 绿：区间/取反/宽度/前导 `]`/16000 字符无宽度字段/`%12[a-z]` 边界 |
| D2 printf `%n` | `include/stdio.h` `_u_vfmt` | `%n` 分支按 `hh`/`h`/`l` 写计数并 `continue`，不输出、不计入填充 | `pctn/{c,cx}` 绿：返回值计数、`%ln`/`%hhn`/`%hn`、8 字节缓冲截断后 `%n` 不越界 |
| D5 strtod inf/nan | `include/stdlib.h` `strtod` | 符号之后、十六进制之前识别 `inf`/`infinity`/`nan[(n-char-seq)]`，大小写无关，`*end` 指向词尾 | `strtod/{c,cx}` 绿；`0x1p3`、`1e-400`/ERANGE 未动 |
| D4 scanf `%n` | `include/stdio.h` `vsscanf` | 把 `%n` 分支从末尾移到 `%c` 之前（原位置在空白跳过与输入耗尽检查之后，`"%d%n"` 在 `"77"` 上永不写入），并按 `hh`/`h`/`l` 定宽 | `scanfn/{c,cx}` 绿：`k=2`、`"%n%s"` 空白不跳、`ln`/`hn`/`hhn` |

实现定义两项只写注释，不改行为：`include/stdlib.h` 的 `rand` 段写明"合规、序列与 glibc 不同、依赖具体序列的程序不可移植"；`signal()` 处理器投递后不重置，与本节第五节第 2 条一致。

生成物同步：`exec/facts/pp-autoinc-gen.tsv`（`facts-gen --write`）与 `include/cx-flat.h`（`cx-flatten --write`），`--check` 为 SAME 41 headers；`tools/ratchets.cx` 7/7。

## 八、D8 `--host-provider` 是死参数（驱动/链接，2026-11-02 omp-ds 追加）

`C44-HOSTPROV-01`。上一轮把 #11 记为"宿主导入路径未测"，实测后定性更重：**该参数根本不被读取**。

最小复现（不依赖宿主 gcc）：

```sh
printf 'int main(void){return 7;}\n' > prog.cx
printf 'UHPROV2\n\1\0\0\0' > bad.prov        # 坏 magic
printf 'UHPROV1\n\4\1\0\0' > wrongtgt.prov   # target=4，与 lnx/x86_64 不符
printf 'UHPROV1\n\1\020\0\0' > bits.prov     # 保留位非零
printf 'UHPROV1\n' > trunc.prov                 # 8 字节截断
for p in bad wrongtgt bits trunc; do
  sh ./unisacc.com --host-provider $p.prov -o $p.img prog.cx
  echo "$p rc=$? img=$(test -f $p.img && echo yes)"
done
```

现装产品（sha256 `d93c1b24…`）实测：四种坏输入全部 `rc=0` 且写出镜像；`--host-provider /nonexistent`
同样 rc0；合法 `UHPROV1\n\1\1\0\0` 也 rc0。没有任何一条被具名拒绝。

定位：`src/drive.cx:512` 把两个路径交给 `bx_diag_opt`，后者（`src/entrybind.cx:221`）只判断"有 provider
且 mode/target 组合合法"就返回 0，**从不打开文件**；`cx/compiler/hostbind.cx` 的 `hb_provider_filter` /
`hb_requirements` 在产品路径没有调用者（只有 `archive/reference-c` 的参考测试调用）。与 #27（33 对象入口
链接不能导入宿主符号）同源：那条路线自己的导入表也是空的。

判据来源：`hb_provider_filter` 要求 12 字节、magic `UHPROV1\n`、target 字节等于构建目标、`b[9]` 保留位为 0、
`b[10..11]` 为 0；坏文件必须具名拒绝，合法文件必须被接受。

探针：`tools/check-host-provider.cx`（`-- ROOT`，6 条断言，约 8 秒）。现装跑出
`CHECK_HOST_PROVIDER failed=5`：`provider-absent`、`provider-badmagic`、`provider-wrongtgt`、
`provider-bits`、`provider-trunc` 全 FAIL（各带 `NOTE … image-written`），`provider-valid` PASS。
**故意不登记 `tests/batches.tsv`**：该批当前必红，红批次不进正式清单；等链接/绑定层真的读 sidecar 后再登记并转绿。

修复方向（不在本轮范围，量 M）：`cxt_text` 之后、写镜像之前打开 provider，用 `hb_provider_filter` 校验
（具名 `invalid-provider framing` / `provider-target differs` / 保留位非零），把 mask 传给链接/绑定层；
不存在文件报 `cannot open host provider`。若只想先止住"静默接受"，最小改动是读 12 字节校验后仍忽略 mask，
但那会造出"校验通过却不生效"的假合同，不如等症状入口（#27）一起做。

