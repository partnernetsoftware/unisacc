# FFI 接桥 + 浮点通道:设计页(2026-11-02 修订版,只写 plans)

> 本节回应退回的六项。实测命令与结果随条目给出;未核实的明确标注。
> 现状:现装 `entry.elf` **1,643,406 字节**;FFI 开关那一步叫 **FFI 1a(b)**(已提交,非 step 1b)。

## 1. 汇编怎么进镜像(第二版重写:走 kernel-build / UNIKERN1)

前两版的路线都错,这里更正:

- `kernel/*.inc` **不是机器码**:`kernel/dense.abi.inc` 头注是 `Network-evaluated dense literal`,
  是数据,放进数据段也不能执行,且它的生成器 `python3 -m unisa emit-kernel` 不允许新增依赖。
- 真正的路线在 `cx/sys/kernel.cx` 头注与 `tools/kernel-build.cx`:
  `exec/c/asm/<unit>_<arch>.S` 由**产品自带的汇编器**组装(头注 no external assembler or linker),
  步骤是 `prepare-assembler` → `preprocess-ISA` → `assemble-ISA` → `link-ISA`,
  链成 **UNIKERN1** blob,由 P3 包携带为 `\0kernel/x86_64`、`\0kernel/arm64`;
  运行期 `core_host_fetch` 取出,先映射可写、把 `kn_service` 地址写进 service slot,
  再 `mprotect` 成可执行(现做法是 mmap + mprotect,**未**使用 MAP_JIT)。

### 先回答:桥要进谁的进程

实测 f)(命令见下):`use ffi` 的使用者只有 tools 两个检查与 `apps/csih` 两个文件,产品侧为 0;
而且**用户程序自带 FFI 运行时**——`cx/sys/ffi.cx` 头注写明 `uffi_*` 来自 carried 头,编进用户程序。

```text
$ sh ./unisacc.com out/probe/ffimini.cx          # 默认 -run
ffi r=0 pid=2995280
$ sh ./unisacc.com out/probe/ffimini.cx -o /tmp/ffimini   # 62072 字节
$ cp /tmp/ffimini /tmp/empty2/ && cd /tmp/empty2 && ./ffimini
ffi r=0 pid=2995669
```

**`-o` 产物在空目录里照样能解析 `dlopen`/`__hostcall` 并调到 `getpid`**,不需要 P3 包。
所以把桥放进 P3 blob(C1/C2)对真实消费者是错的:C1/C2 都假设有包可取。

### 三案对比

| | C1 并入 kernel blob | C2 独立 P3 blob | **C3 carried 字节表(推荐)** |
|---|---|---|---|
| 桥的载体 | UNIKERN1,P3 携带 | 独立 UNIKERN1,P3 携带 | **用户程序自己的 .text/数据**,由 carried 头带入 |
| -o 产物可用 | 否(无包) | 否(无包) | **是**(实测同路径) |
| 用户程序增量 | 0 | 0 | 桥字节(155 B x86_64 / 276 B arm64 量级) |
| 身份影响 | kernel blob 变 | 新增一份 blob 身份 | **产品身份变一次(entry + P3 都含 carried 头),kernel 不变** |
| 首次调用成本 | 无 | 取包+映射 | mmap+写入+mprotect(一次) |
| 车道 | 构造链 | 构造链 | include 生成链(需与 cc 对齐归属) |

身份更正(实测):`grep -a -c uffi_dlopen` 在 `p3-package.bin`=7、`entry.elf`=5,
且 `cx/compiler/fwdstub.cx` 也引用 `unisacc_ffi`,所以 carried 头**在产品里**。
改 `include/unisacc_ffi.h` 会同时改 entry 与 P3 的内容 → **产品身份变一次,kernel blob 不变**。

### j) 现装产品跑 check-ffi-bridge 读的是哪份头(实测)

```text
$ printf '\n#error "TREE-HEADER-IS-USED"\n' >> include/unisacc_ffi.h
$ sh ./unisacc.com tools/check-ffi-bridge.cx
reject: not covered: a preprocessing diagnostic inside an included header
$ (还原后)  FFI_BRIDGE assertions=8 failed=0
```

结论:**`-run` 编译时读的是树里的 `include/unisacc_ffi.h`**,不是产品内嵌的那份。
所以存在一个必须写明的分叉:`-run` 用树、**打包后的产品用包内副本**;
两者不同步时,同一批检查在两种运行方式下可能给出不同结果(FFI 1a(b) 的 8/0 就是"树已改、包内旧"的状态)。

**推荐 C3**:桥的字节由自带汇编器在构造期产出,经生成器写成 carried 头里的 per-arch 字节表,
只在 `use ffi` 的程序里编入(沿用 `__UNISA_FTRIM_LIBC` 的裁剪),首次调用时 mmap+写入+`mprotect` RX。

### 必须回答的五点

a) **`librarycall_*.S` 能否被自带汇编器接受** —— **实测:需要先预处理,预处理后 x86_64 可以**。

```text
$ sh ./unisacc.com tools/asm-source.cx -- ROOT x86_64 exec/c/librarycall_x86_64.S out/lc2/pre.s
$ out/c43-native-kernel-06/assembler-native x86_64 out/lc2/pre.s out/lc2/pre.o
 both rc=0;size -> .text = 155 字节(与 gcc -O2 的 155 完全相同);目标文件整份 cmp 有 250 处差异(含符号/头)
$ printf 'fmov d0, x0\n' > fp.s; assembler-native arm64 fp.s fp.o   -> rc=0(反方向也被接受)
$ objcopy -O binary --only-section=.text 两份,各 155 字节;cmp -l -> **9 处不同**
$ 差异偏移 = 71..79(连续 9 字节);自产目标在这段是 `nop` 填充,即末尾对齐填充写法不同
   (`objdump` 尾部:ret 之后 47: nop / 48: nop ...)
```

我先前"两处硬阻塞"的结论**作废**:那是跳过 `preprocess-ISA` 直接把 .S 喂给汇编器造成的。
arm64 侧仍是硬阻塞:`librarycall_arm64.S:12` 的 `stp d8, d9, [sp, #96]` 不在子集内。

a') 旧结论作废:先前的 "reject: unexpected character" 是因为跳过了 `preprocess-ISA`。
b) **并入后对 kernel blob 身份 / kernel-source / core_run 差分 / K2 表的影响** ——
   `tools/kernel-build.cx` 的 `units[]` 是 11 个,`files_receipt` 与 `verify_material` 都按 11 循环,
   所以 C1 需要同步改这三处,且 `unisa-kernel-files-v1` 清单会变 → 身份变 → 相关批次必然重跑。
   具体哪些批次重跑**未核实**(需要跑一次 `tools/affected.cx`)。
c) **cx 侧怎么拿到桥入口** —— `cx/sys/kernel.cx` 目前只有 `entry` 与 `service slot`,**没有符号表**。
   所以 C1/C2 都要新增一个"入口偏移"约定:由 kernel-build 在链接后把桥入口的偏移写进 blob 头
   (或一个新的 slot),cx 侧按该偏移取地址并保存;这是接口变更,必须先定。
d) **`#ifdef __APPLE__` 的 CSYM** —— 现有 asm 有平台符号处理;`librarycall_arm64.S` 里是
   `#define BRIDGE_SYMBOL _us_library_bridge_raw`(Apple 下加前导下划线),说明它**已经**按同一约定写过,
   但这是 C 预处理器做的事:自带汇编器的 `preprocess-ISA` 是否处理 `#if/#define` **未核实**。
e) **osx/arm64 上 mmap + mprotect 成 RX 的实跑证据** —— **没有实证**,标未核实。
   kernel.cx 的注释描述的是 x86_64 与 aarch64 都走"先可写再 RX",但 macOS 的 W^X 策略与
   Hardened Runtime 下的签名要求另说,不得写成已支持。

## 2. 接口补全(原原型缺项)

```c
/* 返回 0 成功;负数是拒绝码。RESULT 由 RET_KIND 决定写哪块存储。
   ARGS[6]   : 整数/指针参数,按 ABI 顺序(未用的槽忽略)
   FARGS[8]  : 浮点参数(xmm0-7 / d0-7)
   STACK     : 超出寄存器预算的参数,按 ABI 顺序排列的 8 字节槽
   NFIXED    : 命名参数个数;TOTAL 为含变参的总数。NFIXED < 0 表示非变参调用;
               0 <= NFIXED < TOTAL 表示变参调用;NFIXED == TOTAL 等价于非变参。
   变参且 TOTAL > NFIXED 时,超出部分按各 ABI 的变参规则放置(见 §3)。 */
int us_library_bridge_call(const void *entry,
                           const uint64_t args[6], const uint64_t fargs[8],
                           const uint64_t *stack, int nstack,
                           int ret_kind, int nfixed, int total,
                           void *result);
```

- `ret_kind`:与 `SY_FFI_*` 同值;浮点返回写 `result` 的 8 字节(`double`)或 4 字节(`float`),
  **不能**用 `uint64_t` 返回值承载 `xmm0/d0`。
- 整数返回仍可由返回值带出,但为统一,一律写 `result`。
- 结构体按值 / 128 位:原型里**没有**通道,继续保持具名拒绝。

## 3. ABI 表更正(原表把 osx/arm64 与 lnx/arm64 合并是错的)

| 目标 | 整数寄存器 | 浮点寄存器 | 变参规则 | 备注 |
|---|---|---|---|---|
| lnx/x86_64 | rdi rsi rdx rcx r8 r9 | xmm0–7 | `al` = 使用的向量寄存器个数 | SysV |
| osx/x86_64 | 同上 | 同上 | 同上 | SysV |
| lnx/arm64 | x0–x7 | d0–d7 | 与命名参数同规则 | AAPCS64 |
| **osx/arm64** | x0–x7 | d0–d7 | **所有变参一律走栈**(Apple 规则),命名参数仍用寄存器 | 与 lnx/arm64 **不可合并** |
| win/x86_64 | rcx rdx r8 r9 | xmm0–3 | **不用 `al`**;变参浮点要**同时**复制进对应整数槽 | 不接(0.0.21 4a),但表里写对 |
| win/arm64 | x0–x7 | d0–d7 | 同 AAPCS64 | 不接 |

因此 `snprintf("%f", …)` 在 osx/arm64 走的是"浮点进栈"路径,必须单独一个向量覆盖。

## 4. 头文件:单一来源,不复制两份

- 否决"复制成 include/librarycall.h"。
- 优先**符号链接** `include/librarycall.h -> ../exec/c/librarycall.h`;**单一来源**,与
  `tools/sys -> ../cx/sys` 同一约定。
- 未核实点(下一步实测):`include/` 的扫描器是否接受符号链接 —— 具体是
  `tests/cxinclude-boundary.tsv` 的列举方式、以及打包时 `release-build p3` 的 include 根枚举
  (它会按"每个文件一个资源"处理,链接可能导致同一内容两份或用错路径)。
- 若链接不可接受,退到**生成**:由生成器从 `exec/c/librarycall.h` 生成 `include/librarycall.h`,
  带 `--check` 防漂移。两条都是"单一来源",区别只在构建期还是符号链接。

## 5. 验证与顺序(删掉"每步单独候选")

- **只在私有根验构建链**:`git archive HEAD` → 现建 P3 → entry-build → dev-lnx 打包,
  产物放 `out/…`,**不单独合入候选、不安装**。
- **接桥 + 浮点通道 = 一次候选身份扰动**:两件事在同一次构建窗口里完成;
  中途不产生中间候选。
- 差分口径:同向量在 `UNISA_FFI_BRIDGE=off`(libffi)与 `=force`(桥)下逐返回值比较,
  浮点按 8 字节 `memcmp`;拒绝面(结构体/128 位/变参超限)仍须具名拒绝。
- 字节增量:构建前后 `stat -c %s entry.elf`,并记录生成物 `.inc` 的体积。

## 6. 事实更正

- 现装 `entry.elf` = **1,643,406 字节**(不是"3 MB 级")。
- 已提交的那步是 **FFI 1a(b)**(开关 + 具名拒绝 + last_error),不是 "step 1b"。
- 本页只写 plans;提交由 cc。

## 7. 运行期证据(2026-11-02,仓外 harness:mmap RW → 拷 .text → mprotect RX)

| 向量 | 结果 |
|---|---|
| `getpid()`(零参、整数返回) | **通过**:`getpid=3106783`,`b.res == getpid()` 成立 |
| `snprintf("%d %f %d", 1, 2.5, 3)` | **通过**:`[1 2.500000 3]`,浮点入参数通路成立 |
| `>6` 整数参数走栈 | **通过**(修复后):`[1234567]` |
| `sqrt(3.0)`(浮点返回) | **通过**:`sqrt3=1.732050807569`。先前崩溃不是桥的问题——harness 用 `libc.so.6` 的句柄 `dlsym("sqrt")` 拿到 **NULL**(sqrt/sin 在 libm),调用 0 地址导致 SIGSEGV |

关键修复:**栈 16 字节对齐**。SysV 要求 `call` 前 `rsp%16==0`;桥入口 `rsp%16==8`,
六个 `push` 之后仍是 8,所以必须在 `call` 前 `subq $8,%rsp`、之后 `addq $8,%rsp`。
加上这一对之后零参调用通过;此前所有崩溃都是这一个原因(加上一版 harness 自己把 `rbp` 改成常量)。

现状:桥 `.text` 221 字节,可组装、零参调用可运行;浮点与栈参数四条**尚未分别证明**。

**第二个根因(栈参数)**:16 字节补位必须放在**栈参数区之上**。原写法先 `subq nstack*8` 再 `subq $8`,
于是 `(%rsp)` 是补位、参数整体错一槽(实测输出 `123877240488456`);改成先 `subq $8` 再开参数区后,
`(%rsp)` 就是第 7 个参数,输出恢复 `1234567`。

**第三个结论(harness 侧)**:`dlsym(libc, "sqrt")` 返回 **NULL**(实测 `handle=... sqrt=(nil) sin=(nil)`),
数学函数在 `libm.so.6`。这不是桥的缺陷,但任何"用 FFI 调数学函数"的检查都必须先确认地址非 0,
否则得到的是"调用 0 地址"的假崩溃。

至此四条向量全部通过:getpid(零参)、sqrt(浮点入/返回)、snprintf 混合、>6 整数栈参数。
