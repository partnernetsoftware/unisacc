# 编译提速：瓶颈地图（2026-10-30，cc）

只记实测。候选 = `out/grk-entry-build-01/candidate`（新 CX 入口，两只 `-O2 -funit` 对象 + hostnative + 已装 P3）；
产品 = 已装 `unisacc.com`。冷跑，`UNISACC_RR_TIME=1`，`-S`，Linux x86_64 本机，单次观测。

## 一、相位表（毫秒）

| 输入 | 谁 | 墙钟 | 包解析 | 模型解压合计 | e2 | e1 | e3 | tapecheck |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| `int main(){return 0;}` .c | 候选 | 122 | 36 | 约 66 | 3 | 0 | 4 | 0 |
| 同上 | 产品 | 127 | 23 | 约 56 | 2 | 0 | 2 | 0 |
| 带 `<stdio.h>` 的 hello .c | 候选 | 823 | — | 约 67 | 207 | 27 | 409 | 63 |
| 同上 | 产品 | 347 | 24 | 约 59 | 68 | 11 | 106 | 20 |
| 空 main 的 `.cx` | 候选 | 4831 | — | 约 65 | **4529** | 82 | 99 | 5 |
| 同上 | 产品 | 1585 | 27 | 约 58 | **1371** | 35 | 44 | 2 |

`-run h0.c`：候选 152 ms，其中包解析 36 ms + 七个模型解压 77 ms；候选没有产品的 run-cache。

## 二、瓶颈按收益排序

### B1　E2 每个 `#include` 都整缓冲重扫（模型规则，最大）

**前导部分已在结构层解决（4f4a2ae2b3，不改模型）：** `include/cx.h` 在 `__UNISA__` 下改读生成的单文件 `include/cx-flat.h`（`tools/cx-flatten.cx --write|--check`，源为 `include/cx-nested.h`；宿主编译器仍读嵌套源）。

- 实测：空 main `.cx` 的 E2，产品从 1.74 秒降到 0.09 秒；build-06 候选冷编 `tools/facts-gen.cx`（三单元）从 40.5 秒降到 16.8 秒（E2 26.4 → 3.1 秒）。
- 等价：`-E` 规范化空白后逐字节相同。
- 约束：展平文件必须放在 `include/` 里，因为 E2 只对系统 include 目录的头做 libc 体裁剪（`__UNISA_FTRIM_LIBC`）。用 `-I` 放的副本不裁剪，输出会从 5.8 KB 涨到 76–118 KB。
- 用户自己写的 `#include` 仍按下文的规则方案处理。

- 位置：`exec/pp/directive-body-byte.tsv` 的 `INCH`（头文件读完：`INPOP`、补换行、`SPAN2` 抄回外层剩余、`SWAP`），以及 `exec/pp/text-byte.tsv` 的 `P0`/`P1`、`directive-scan-result.tsv` 的 `P3END` 三处 `SWAP`。每次 include 后整缓冲回到 P0 重新规范化并重扫指令。
- 实测（宿主 GCC 构造的参考执行器回放产品真实 RI，tape 与产品逐字节相同）：空 main 的 `.cx` 一次 E2 共 **213 次 SWAP，累计换入 50,841,523 字节**；最终输出约 50 万字节，其中非空白约 3.2 万。每次 include 3 次 SWAP，约 71 个嵌套头，缓冲从 1.7 万涨到 15 万以上又继续涨。
- 对照实验：固定 32 KB 正文前放 K 个空头，K=0/10/20/40 时 E2 为 17/51/77/131 ms，严格线性；每个 include 每字节约 90 ns。
- 方向：已处理的前缀不再进入后续 SWAP。头文件单独做 P0/P1（C99 5.1.1.2：注释与续行不跨文件），前缀输出另存、末尾按序拼回，只对“头文本 + 外层剩余”重跑指令扫描。理论上换入量从约 5000 万字节降到约 200 万（3 遍 × 约 60 万）。
- 前提：保存前缀时要带每字节位置属性；现有 `OCUT` 只存字节，不存 `at[]`，位置映射会断。需要一条带属性的截存/回放动作（执行器机制，非语言语义），或改成不搬前缀的帧方案。属模型规则 + 构造，走慢车道；先在私有包上对拍 E2 正例 103、拒绝 187、位置/诊断批。
- 次级同源：被裁掉的 libc 函数体以空格保留，缓冲大且全靠 B1 反复搬运；E1 要再词法 50 万字节（候选 82 ms）。可在 B1 之后评估只留换行。

### B2　执行器代码质量（每个阶段乘 3–4 倍）

- 同一模型：候选 e2/e3 是产品的约 3.3/3.9 倍；prd 记过同一 CX 执行算法宿主 GCC 编约 40 ms、旧汇编 58 ms、自身 `-O2` 编 182 ms。
- grk 的插桩分解（plans/cx-modules.md 第 153 节）：步间衔接约一半，转移约两成半，输出约两成，稀疏内存不到 1%。
- 根因是自身 `-O2`（E4 有界窥孔 + 软件栈）对热循环生成大量压栈/出栈。只换汇编转移最多拿回那两成半。
- 方向（待主人定）：a）整个执行循环（转移 + 动作分派）用汇编；b）改进 E4/lower 的寄存器分配与栈槽消除——同时让所有自编程序变快。
- **步级计数（2026-10-30，cc）。** 测试架在 `out/b2`：用 `src/executor.cx` + 产品包里的真实模型，以 `-O2 -o` 编译；输入为 `tools/safedel.cx` 的 `-E` 文本，作为 48 万字节的无 include C 源。

  | 阶段 | 步数 | 墙钟 | 每步耗时 | 每步动作数 | 转移平均 count/cut |
  |---|---:|---:|---:|---:|---:|
  | e2 | 1016 万 | 1.33 s | 131 ns | 1.9 | 6.4 / 1.9 |
  | e3 | 477 万 | 0.67 s | 141 ns | 1.2 | 19.6 / 5.9 |

  - 动作以 ADV（0）、COPY（14）、COPYT（15）为主（e2），e3 另有 ALUI/CMPI/PUSH/POP。
  - 每步约 400 个时钟周期，没有算法热点。转移的二分查找加累加只有几次迭代，`NN 逐项求值` 合同也不允许改成前缀和查表。
- **已试、不采纳：** `action_run` 的 56 路 switch 被编成线性比较链（每过一个 case 约 10 条指令）。改成每个操作码一个处理函数、按操作码查函数指针表后，输出逐字节相同，但 e2 只快 7%，e1/e3 不变，因为常用操作码本来就排在链首。收益不抵改产品输入的代价，改动只留在 `out/b2/coremod`。
- 结论：B2 只能靠代码质量解决，也就是上面的 a 或 b（G1–G6 见第四节）；重排 CX 源码结构拿不回多少。
- **主人定走 a（2026-10-30），已接上：** 汇编核心早已存在（`exec/c/asm/`，x86-64 与 AArch64 各一份，含转移、56 个动作与分派、运行循环），由 `tools/kernel-build.cx` 用产品自带的汇编器构造成 UNIKERN1，作为 P3 包资源 `\0kernel/{x86_64,arm64}` 随包携带，已装产品用的就是它。
  - `cx/core/kernel.cx`（f03c5a940d）把 `exec/c/asm/binding.c` 移植成 CX 模块，`core_run` 与 `core.execute` 同签名同 ABI。
  - 测试架实测（image/O0，120 函数程序）：七段输出逐字节相同，产出的 ELF 运行结果与 gcc 一致。e2 1.36→0.44 s，e3 3.0 倍，全链 2.95→1.11 s。
  - 待办：入口切换（`src/executor.cx` 改 `use core.kernel`）在 grk 车道。

### B3　每次运行的固定开销（小程序的大头）

- 候选每次：包解析 36 ms + 解压 e3 42、e2 11、e1 9、lower 9、elf 4、prune 1 ≈ 113 ms，占 `-run h0.c` 152 ms 的四分之三。解压由自编 CX inflate 完成。
- 方向：按包 SHA 与成员 CRC 绑定的已解码模型缓存（mmap 只读，命中仍核身份），或包内存未压缩副本；包目录解析同样可缓存。
- 候选还没有产品的 run-cache（产品热跑约 65 ms），属功能缺口，对应 0.0.44 启动提速需求。
- **复测（build-13，汇编核心，仓库根，空 main 的 `.cx`）：**
  - 候选每次约 325 ms：解压合计约 80 ms（e3 45、e2 11、lower 10、e1 9、elf 4、prune 2），推理约 187 ms（e2 98），其余约 60 ms。
  - 已装产品冷跑 330 ms，命中 run-cache 后 58 ms。run-cache 是最大的单项缺口，已交给 grk。
- **产品 run-cache 的实现（移植依据）：** `exec/c/runreuse.h`（612 行 C）。
  - 实测：同一个空 main 的 `.cx`，第一次 `lookup_hit=0`，334 ms；第二次 `lookup_hit=2`，58 ms。缓存目录是 `/tmp/unisacc-e3cache-<uid>/`。
  - **USE33：** 存 E3 产出的 tape、ATTRS 和 include 回执。
    - 键 = sha256(`USE33`、版本 `RR_VER`、产品文件 sha、包 sha（包在别处时）、路由、目标、优化级别、源身份)。
    - 源身份 = sha256(cwd、全部 CLI 参数（`--` 之前）、每个源的"路径\0内容")。软链按目标内容算。
  - **USE34（`<key>.host`）：** 存宿主编译之后的 tape，回执从匹配的 USE33 条目克隆。
  - **命中校验：** 头魔数和各段长度有上限；每个 present 回执要重新哈希对应文件并比对；每个 absent 回执要求那个路径仍然只是 ENOENT（ENOTDIR、EACCES 都使缓存失效）；最后核对 tape 的 sha。
  - **写入：** 先写临时文件再原子改名。
  - **不用缓存的情形：** `UNISACC_RUN_CACHE=0`、转发第二遍、带警告、tape 输入、带 deps 输出、strict/capture 模式、源多于 16 个，或者目标不是 lnx/x86_64。
  - 命中后仍要走 lower/elf 和执行；被缓存的只是前端产物，运行期资源每次重新绑定。
  - 候选移植成 CX 模块时，键里的"产品 sha"要换成候选自己的身份（候选文件加附带的 P3）。
- **包内嵌头：** cwd 不在仓库根时，E2 从包资源 `\0hdr/` 取的是旧版嵌套 cx.h，同一个空 main 的 e2 要 1.6 s。展平前导要随下一次包刷新进包，才会处处生效。
- **候选 run-cache 命中（2026-10-30，build-17）：**
  - 单源也存转发 ELF。命中后直接执行，不再跑 lower、elf 和模型装载。
  - 键里 64 KiB 及以上的文件用 `cx/sys/sha.cx` 的 `sy_sha256_hex`（Linux x86_64 的 AF_ALG）。失败或更小的块用 `cx_sha256_hex`。
  - `tests/runcache.cx` 对 `out/grk-entry-build-17/candidate`（sha256 `a051997fdf52304bfffd285cca02474b1f4743012abaf26e1a7b1f2439f3f2b9`）12/0。`TIMING cold_ms=546 hit_ms=40 phases=UNMEASURED`。产品同一测试的命中是 61 ms。
  - 只存了转发 ELF、哈希仍是软件实现的中间候选，热路径墙钟 0.196 s。合同命中从 build-15 的 506 ms 降到这只的 40 ms。
  - cx_read 从 4 KB 起倍增扩容，读 2.7 MB 约多花 20 ms（先前估算）。先 fstat 定长可省，但新增的 fstat 要进 exec/facts/pp-autoinc-gen.tsv，留在慢车道，这轮没动。

### B4　多单元的重复工作

- 早先产品实测 `tools/dev-run.cx` 冷 8.7 s 中 units+e3 因 fwd 重启跑两遍（约 1.8 s）。新入口 `53af80134a` 对已测用例不再重跑 e3，需转发桩重启的用例（dup.c）仍红。
- 每个 `.cx` 单元各自重跑同一份 cx.h 前导的 E2（B1 的成本乘单元数）。B1 之后可评估前导结果按（目标、预定义、前导字节）缓存。

### B5　构造自身

- 入口自举冷构造约 33–40 s（O0 单只），`-O2` 整只超 55 s，已拆成三只对象。构造时间同样受 B1（每单元 cx.h 前导）与 B2 支配。
- **build-13 分相（第二代，汇编执行器，35 只对象，墙钟合计 58.3 s，非独占窗口）：**

  | 相位 | 耗时 | 占比 |
  |---|---:|---:|
  | e4 推理 | 18.3 s | 31% |
  | elf | 9.3 s | 16% |
  | e2 | 7.6 s | 13% |
  | e3 | 6.8 s | 12% |
  | lower | 5.7 s | 10% |
  | 模型解压合计 | 3.3 s | 6% |

  - 模型解压缓存对构造最多省 6%；对空 main 的单次 `-run`（约 80/325 ms）更有意义，但 run-cache 命中后它本来就不出现。
  - 用户 include 重扫最多落在 e2 那 13% 里。
  - elf 每字节约 800 ns（15 KB 对象）到 1400 ns（1.6 MB 对象），温和超线性；lower 稳定在约 550 ns/B。big.c 上 elf 没有 SWAP，每字节约 7.8 步。elf 是常数项重，不是 B1 那种整缓冲重扫。
  - e4 与大小大致成线性（约 3 µs/B）。最大的单只是 driveface.o（1.6 MB，e4 5.4 s，elf 2.3 s）。
- **建议（待实测）：** 执行热循环已由汇编核心承担，驱动层 CX 代码的 `-O2` 运行收益变小了。不在热路径上的对象改用 `-O0`/`-O1` 构造，可以省下 e4 的大头。每次运行都会走的 inflate、模型装载等，保留 `-O2`。按对象分别实测运行时间后再定。
  - 测试架初测（out/b2/k，汇编核心，极小输入，取 3 次最好）：驱动、包解析和模型解码代码用 `-O0` 编时，e3 模型那次运行 126 ms，`-O2` 时 87 ms；lower 那次 85 ms 对 57 ms。解码密集的代码 `-O0` 慢约 45%，所以 codec、model_decode、package_decode、packmodel、executor 要保留 `-O2`。
  - e4 的大头在只走一次的驱动对象：driveface 5.4 s、driveroute 1.7 s、drive 1.4 s、host 1.3 s、cxmod 1.2 s、tapegate 1.1 s。这些对象改用 `-O0`/`-O1` 后，每次运行要多花多少，需要构造一只候选、做同候选的冷热 `-run` 分相对照后再定。
- **实测（2026-10-30，同一份 `tools/entry-build.cx`，已装产品，P3 用 build-14 的 pkg.bin）：** 默认全是 `-O2`。`--o1-drivers` 才把 `drive.o`、`host.o`、`driveface.o`、`driveroute.o`、`cxmod.o`、`tapegate.o` 编成 `-O1`，其余保持 `-O2`。两只候选都由这份源分三批跑完（6 只驱动、其余 27 只、链接）。每批重新 `-run` 同一份源，开工前 `sha256sum -c` 通过。`entry-build.cx` 的 sha256 是 `f7231608fb6d2f0dc5fef01e00dc9aa84ab246be915e5ad259fe56ecae0e98a3`。33 个源的 sha256 在对应的代码提交说明里。这不是一次 `entry-build` 进程的墙钟。对象墙钟和链接墙钟是回执里的 `ns`。e4/elf 是该对象 stderr 里 `RR_TIME inference` 的较大值。早先直接调用编译器、并且在分批期间改过 `entry-build.cx` 的那一轮不拿来对比。
  - 33 只对象的墙钟 55558 ms → 45509 ms。链接 5890 ms → 7259 ms。对象加链接 61448 ms → 52768 ms。外层三批相加（含每批 `-run` 这份入口）73650 ms → 65045 ms。
  - 这 6 只的墙钟 32557 ms → 22634 ms。33 只的 e4 合计 20205 ms → 7327 ms，elf 合计 8518 ms → 10766 ms。`-O1` 少做优化，对象变大，elf 和链接变长，净构造仍少 8.7 s。另外 27 只对象两边逐字节相同。
  - 6 只各自的墙钟 / e4 / elf（毫秒，默认 `-O2` → `--o1-drivers`）：driveface 11536/5400/2236 → 7311/393/2857；driveroute 4713/1928/822 → 3448/197/1111；drive 7797/3148/1299 → 5607/273/1869；host 3091/1203/474 → 2323/124/750；cxmod 2664/1226/475 → 1867/110/713；tapegate 2757/1104/504 → 2078/111/721。
  - 全 `-O2` 候选 `out/grk-opt2-o2/candidate`，2717972 字节，sha256 `2d59f550b6cb142147fd6f0a5f1b62354e1e2759736ad158494bf26322018e36`。`tests/runcache.cx` 12/0，`TIMING cold_ms=496 hit_ms=41`。`tools/dev-run.cx` 不带参数的冷 `-run` 编译加上一次用法拒绝，6126 ms，退出 2。这不是 dev-run 自己的运行时间。
  - 混合候选 `out/grk-opt2-o1/candidate`，2959636 字节，sha256 `e477a4c70be28c11b86f9fce129fae2acf4e6c6813aa7d8436a8881d75160970`。`tests/runcache.cx` 12/0，`TIMING cold_ms=515 hit_ms=41`。`tools/dev-run.cx` 不带参数的冷 `-run` 编译加上一次用法拒绝，6112 ms，退出 2。这不是 dev-run 自己的运行时间。runcache 的冷跑和命中没有变快。
  - 同一份 `out/grk-opt-cmp/main.c`，两边 `-run -- z` 的 stdout 和 stderr 逐字节相同，都退出 0，stdout 是 `opt-cmp z`。
  - `-O0` 没有再试。这 6 只的 e4 已经降到 1.2 s，再降优化级别主要会把 elf 和链接再拉长。
- **默认（148 格全 SAME 之后）：** `tools/entry-build.cx` 不带旗标也编那六只 `-O1`，`--o1-drivers` 仍接受，选的是同一六只。上面的计时仍属于改默认之前的那份源（sha256 `f7231608…`）。依据是 `out/grk-opt2-o1/candidate`（sha256 `e477a4c70be28c11b86f9fce129fae2acf4e6c6813aa7d8436a8881d75160970`）上 148 格全 SAME，表在 `out/grk-opt2-mx`。没有按新默认重编整只候选。无旗标只编了 tapegate.o 和 path.o，目录 `out/grk-opt2-default`，构造器是已装产品经 `sh`。tapegate.o sha256 `1310b3c403ad29d9bc3c8ababf50774f561d8f4acd9848b272957aaa16133d3d`，与混合候选逐字节相同。path.o sha256 `8322dfd20e44f434000bb37af5cf43310cc87920f1959c4a204cb6bc2952574a`，与全 `-O2` 的 path.o 逐字节相同。回执是 `mode=o1-drivers`，`other=-O2`。

## 三、建议顺序

1. B1（单项最大，`.cx` 冷编译可望从数秒降到数百毫秒级）：先写规则方案与私有包对拍，再走慢车道构造。
2. B3（与 B1 正交、改动在运行时一层）：模型解码缓存，带身份核对。
3. B2：等主人定执行器路线；在此之前不动热循环。
4. B5 已测：六个驱动对象 `-O1`，对象加链接 61.4 s → 52.8 s。148 格全 SAME 之后，这已是 `tools/entry-build.cx` 的默认。B4 仍随前三项再复测。

复现：参考执行器回放工具在仓外 `/tmp/claude-1001/w/`（`archive/reference-c/exec/c/run.c` 加 SWAP 计数，宿主 GCC 仅作测量，不作产品证据）。

## 四、B2 细化：自身 `-O2` 生成码的具体缺口（2026-10-30 实测）

微基准 `mb.c`（二分查找 + 求和，40 万次调用）：unisacc `-O2` 206 ms，`-O0` 333 ms；宿主 gcc `-O0` 166 ms、`-O2` 43 ms。
unisacc `-O2` 比 gcc `-O0` 还慢，比 gcc `-O2` 慢 4.8 倍。二分循环体 39 条 x86 指令；gcc `-O1` 整个函数 26 条。

`-O2 -c -b lnx/x86_64` 对象反汇编看到的缺口，按每次出现省下的指令数排：

| 缺口 | 现在 | 应该 | 所在层 |
|---|---|---|---|
| G1 常数移位 | `imm r0,1; shr64` → mov/mov/mov/push rcx/mov/sar %cl/pop/mov，8 条 | `sar $1`，1 条 | E4 立即数折叠 + enc 立即数形式 |
| G2 比较加分支 | `slt64; jumpz` → cmp/setl/movzbq/test/je，5 条 | cmp/jge，2 条 | E4/lower 比较—分支融合 |
| G3 局部全在栈槽 | 每次读写 `movslq -0x28(%r9)`、`mov %eax,-0x28(%r9)` | 寄存器 | tape 只有 r0–r7（weights/gold/regmap.tsv：rax rdi rsi rdx rcx r8 r9 rsp），rbx、r10–r15 未用；需扩 tape 寄存器并在 O2 提升不取址的标量局部 |
| G4 三地址落两地址 | `add64 r0,r1,r0` → mov r11 / mov / add，3 条 | `add %rdi,%rax`，1 条 | lower/enc：目的与一个源相同或可交换时直接两地址 |
| G5 下标乘法 | `imm 4; mul64; add64; .ld` → mov/imul/mov/mov/add/movslq | `movslq (%rdi,%rax,4)` | E4 pow2 改移位（peep 已有 pow2 关系）+ enc 带比例寻址 |
| G6 32 位局部不转发 | peep 只覆盖 `store64/load64`（weights/gold/peep.tsv 1632 键），C 里最常见的 int 局部走 `.st/.ld`，存后立刻重读不被消除 | 存—读转发、重复读消除 | peep 事实表加 `.st/.ld` 宽度变体 |

G1、G2、G4、G6 只改有限表与构造，不改 tape 协议，可先做；G5 需 enc 新寻址形式；G3 改 tape ISA，影响 E3/E4/lower/enc 与执行器自身，最后做。
G1+G2+G4+G6 估计二分循环体从 39 条降到约 20 条；再加 G3 接近 gcc `-O1`。执行器、E2/E3 等自编热循环同样受益。
注意：peep 等 gold 表的生成器仍是 Python（`unisa/gold.py` → `gold-export`），改表要先走该生成器或先把它移植为 `.cx`。

### 四之补：动手前核对出的约束（2026-10-30）

- gold 表改动不再需要 Python：`tools/gold-table.cx`（`56d9fc2a6f`）以 `weights/gold/*.tsv` 为权威，20 张全检；11 张有 `seed/gold_*_label.c` 规则形，`--write` 可由规则重写行。
- G6 不能照 64 位规则直接做：`.st [x],r,4` 只存低 32 位，`.ld` 会按宽度符号/零扩展读回；把“存后重读”改成 `mov` 或删掉重读，会丢掉 u32 回绕与 int 截断语义（E3 正靠这次重读截断）。只有寄存器已知为规范扩展值（例如它本身刚由同宽 `.ld` 读出）时才安全，需要先在 E4 跟踪“规范宽度”状态，或在 tape 增加显式扩展操作。降级为 G3 之后再做。
- G1 没有现成落点：tape 没有移位立即数操作（opinfo 操作表里无 shli/shri），编码器也不跟踪寄存器常量。要么给 tape 加立即数移位操作并让 E4 折叠 `imm`+移位，要么在 lower 做“上一条是 imm 且该寄存器随后死亡”的融合。
- G2 的落点是 lower：x86 的 `slt64` 走 isel `setl`，`jumpz` 单独编码；需要在 lower 里对“比较 + 紧随的 jumpz 且比较结果随后死亡”做融合，输出带条件的跳转。lower 已有 ARM 的有界 dead-after 扫描与立即数融合，可复用其扫描方式。
- 以上每项都要重建 E4 或 lower/elf 模型（六目标），并在全部目标上重新对拍，属慢车道工作量；按项单独私有包验证后再合并。
## 六、冷启/热启时间构成（2026-10-31 复测，bun 为基线）

只记实测。产品 = 仓库根 unisacc.com（5025194 字节）。候选 = 用 tools/entry-build.cx 在 out/hotfix1 重建的 candidate：33 个键对象逐个 --only 编译，再手动 -b lnx/x86_64 链接并拼接 p3 包尾（entry-build 自带 link 有 45 秒上限，本机不够）。干净环境（剔除所有 UNISA* 变量），多跑取 p50。输入 int main(){return 0;}。

### 6.1 热启（run-cache 命中）

| 谁 | p50 | min | 证据 |
|---|---:|---:|---|
| 已装产品（经 sh） | 57.5 ms | 55.8 ms | 冷启 285 ms 与它差 5 倍，确为命中 |
| 重建候选 | 3.1 ms | 2.8 ms | strace 只见一次 execveat 转发到已存镜像 |
| bun 1.4.4（bun -e 0） | 11 ms | 10 ms | |

热启目标（p50 优于 bun 10 ms）两条路径都已达成，候选是 bun 的 3.2 倍快。

### 6.2 冷启（run-cache 目录全新）

缓存目录是 TMPDIR/unisacc-e3cache-<uid>，不在 HOME。只换 HOME 测出 3 ms，是旧目录命中；必须同时换 TMPDIR 且目录事先不存在。

| 谁 | p50 | min |
|---|---:|---:|
| 已装产品（经 sh） | 285 ms | 271 ms |
| 重建候选 | 144 ms | 140 ms |
| bun | 11 ms | 10 ms |

候选冷启比产品快一半，但仍为 bun 的 13 倍。

### 6.3 候选冷启 144 ms 的构成（逐段实测）

| 段 | 墙钟 | 测法 |
|---|---:|---|
| 壳启动 | 1.4 ms | --version |
| 编译器模型装载 + 预处理就绪 | 60 ms | -E 空文件 61 ms，-cx-plan 1 ms |
| 生成到汇编 | 70 ms | -S 131 ms 减 -E 61 ms |
| 汇编 + 链接 + 运行 | 13 ms | 全流程 144 ms 减 -S 131 ms |

装载为纯用户态解码：strace 全程 syscall 合计 0.9 ms（14 次 read、20 次 mmap、0 次 brk）。
瓶颈是 2 039 850 字节的 p3 包体是文本格式（包头 P 3 32 1479 77），逐字符走 num()，
二进制快路（首 8 字节 UNINETB1，src/runtime/model_decode.cx:348）从未被产品包触发。

### 6.4 冷启追平 bun 的动作（按收益排序）

C1 二进制包（60 ms → 约 5 ms）。生成器同时产出 UNINETB1 包，装载直接按元素宽度搬字节。已有写者：tools/source-package-pack.cx、tools/cxlib/sourcepackage.cx；缺的是把现状 p3 包换过去并让生成的 .com 装载它。收益最大、风险最低。

C2 生成 70 ms。属 E4/lower 代码质量，与 B2 同源（自身 -O2 的压栈/出栈），慢车道。

C3 装载结果按包 SHA 预展开缓存。与 B3 的已解码模型缓存是同一件事，命中仍须核包身份。

C1 之后理论冷启约 90 ms，仍是 bun 的 8 倍；要进 20 ms 档必须叠加 C2 与 C3。
