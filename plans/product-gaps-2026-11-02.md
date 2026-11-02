# 产品缺口清单（0.0.43 → 0.0.44 视图，omp-ds 2026-11-02）

范围：**产品本身**的缺口——编译器正确性、诊断、运行库、驱动/链接、目标平台/APE/加载。
只读盘点，未改任何产品源码。工具/构造/测试结构类缺口不混入正文，见文末第五节。
来源：`plans/release-0.0.43-known-gaps.md`、`plans/v0.0.44.md`、`plans/defects-c43.md`、
`plans/d-class-diagnosis-2026-11-02.md`、`plans/cx-dir-structure-2026-11-01.md`。

排序口径：**离初心（C99 + NN 权重 + 单文件 APE + 多架构）的距离 × 用户可感知度**。
列说明：`0.0.44` = 属本版（加速/修 bug/结构）还是功能新增；`量` = S(<0.5d) M(0.5–2d) L(1–2w) XL(周计)；`快车道` = 能否只改 .cx + 探针 + `sim --cov` 验证，不触 SID/PC/权重。

## 一、编译器正确性：拒绝合法 C（用户直接撞上，最刺眼）

| # | 现象 | 影响面 | 阻塞原因 | 车道 | 证据 | 0.0.44 | 量 | 快车道 |
|---|---|---|---|---|---|---|---|---|
| 1 | `*(*q)++=0;` 报 `expected ';'` rc1，系统 cc 接受 | C99 6.5.2.4/6.5.3.1 后缀 ++ 于括号解引用 | 未定位到具体分支；疑似 parse 层后缀处理 | `[已核实]` 解析域 | v0.0.44: 已确认缺陷 C44-PARSE-POSTINC-01；`out/c44-pointer-postinc.cx` | 修 bug ✓ | S | **否（慢车道）**：omp-ds 2026-11-02 实测仍报 `expected ';'`，该字符串在现树只存在于 `archive/reference-*`，产品解析由权重/事实表驱动 |
| 2 | `sizeof ES[N].name` 进实参报 `not covered: argument list` rc1 | C99 6.5.3.4 sizeof 于带下标+成员后缀 | 关系/逻辑表达式与实参中的 sizeof 后缀未建模 | `[已核实]` 解析域 | v0.0.44: C44-CX-SIZEOF-POSTFIX-01；`/tmp/unisacc-c43-union-pic-probe/sizeof-arg.cx` | 修 bug ✓ | S–M | **否（慢车道）**：实测仍报 `unknown identifier`（`sizeof ES[N].name` → 宏展开后 `ES[ 4 ].name`），与 #1 同域 |
| 3 | 已知原型实参个数不查：`f(1)` 对 `f(int,int)` 静默接受 rc0 | C99 6.5.2.2p2 实参数量 | 无固定参数个数诊断；需覆盖可变参数/函数指针/无原型兼容 | 慢（parse2 事实表+函数类型表） | v0.0.44: C44-CALL-ARITY-01 | 修 bug ✓ | M | 否 |
| 4 | 23 处 C99 约束违反静默接受并产出对象（assign-to-const、discards-const、incompatible-pointer-init、void-value-used、duplicate-case、zero-array-size 等） | C99 6.5.16p2、6.5.6、6.3.2.3、6.1 声明层，共 23 条 | 事实表缺约束行，不是代码分支写错 | 慢（`exec/parse2/*.tsv`→`facts-gen`→`p2ini`） | defects-c43.md `c.negative`；d-class 分析 | 修 bug ✓ | L | 否 |
| 5 | 2 处预处理约束违反未拒（宏名非标识符、头名未闭合） | C99 6.10.3/6.10.2 | pp 模型域约束行缺失 | 慢 | defects-c43.md `pp.negative`；d-class 第三节 | 修 bug ✓ | M | 否 |
| 6 | 有效程序被拒：`int vla_fs[(int)1.0 + 0];` | C99 6.7.5.2 VLA 尺寸常量表达式 | 未定位 | `[已核实]` 解析域 | defects-c43.md:362（观察，低） | 修 bug ✓ | S | **否（慢车道）**：实测仍报 `not covered: constant expression`，reject 行在 `exec/parse2/constexpr-manifest.tsv` |

## 二、运行库：`-run` 下用户程序行为错

| # | 现象 | 影响面 | 阻塞原因 | 车道 | 证据 | 0.0.44 | 量 | 快车道 |
|---|---|---|---|---|---|---|---|---|
| 7 | **R15**：`-run` 一条磁带 → SIGSEGV rc139；同磁带 `-b lnx/x86_64 -o` 出的镜像退出正常 | 任何 `-S` 产物（连 `int main(void){return 0;}` 的磁带）；用户可见 | **已核实（omp-ds 2026-11-02，二次更正后的最终版）**：`-run` 的 argv 只在 cache 分支绑定（`src/drive.cx:543`），磁带分支被 `!as_tape && !cxp_tape_path(src)` 排除 ⇒ `dv_argc_b`/`dv_argv_b` 全 0 ⇒ 程序见 argc=0、argv=NULL。`.c` 磁带侥幸返错状态；`.cx` 磁带必有模块前奏 `_unisa_environ_init`，算 `environ = argv + argc + 1 = 0x8` 后解引用，实测 `SEGV_MAPERR si_addr=0x8`。磁带 body 在任何模式下都是正确产物，与 `.sys`、体积、memory 路线均无关 | 快车道（`src/drive.cx` 1 处：磁带分支绑定 argv，失败并入既有 `if (pr)` 出口；净零行） | 实测：`argc2` 磁带 `-run -- zz` 打印 `argc=0 a1=-`；`strace` 见 si_addr=0x8；`tools/check-run-tape.cx` 现装红 5 → 候选 18/18；提交 `8e7d8edb94` | 已修 ✓ | S | **是**（提交 `8e7d8edb94`，装机随下一批候选） |
| 8 | 分配失败不置 `errno`：`malloc/realloc/calloc` 返回 NULL 后 `errno` 仍是调用前的值（`perror` 打 `malloc: Success`，按 `errno==ENOMEM` 分支走错） | POSIX/C 惯例（与 glibc 对齐）：失败置 ENOMEM，成功不动 errno | **已修**：`_unisa_map` 失败分支 + `malloc`/`calloc`(两条)/`realloc` 的上界与溢出守卫都置 `errno = ENOMEM`；成功路径零改动（不写 0）。`include/cx-flat.h` 由 `tools/cx-flatten.cx --write` 重生成，`--check` SAME | 快车道（`include/stdlib.h` +9/-6，`include/cx-flat.h` 生成物） | HEAD 树：`tools/check-errno.cx` failed=1（`BAD huge p=0x0 errno=0`），fopen/strerror 两项控制 PASS；候选 `865b43f6…`：3/3 PASS，`errno 12` 与 glibc 一致 | 已修（待提交） | S | **是** | 早先 `-ENOMEM` 当地址那条已在树里修掉（`_unisa_map` 的 `<= 0`）；本条是同一族的 errno 漏设。落地仍要新包（旧 `unisacc.com.dev/p3-package.bin` 内的头） |
| 9 | stdout 重定向到 `/dev/full`：`puts=0 fflush=0 ferror=0`，rc0，写失败无人发现 | C99 7.19.8.1/7.19.8.3 错误状态传播 | `_u_st_raw` 只回短计数不置位；标准流不建 slot，`ferror(stdout)` 恒读到新 0 | `[已核实]`：`include/stdio.h` 的 `_u_st_serr[3]`（按 fd 0/1/2）+ `include/sys/_exit.h` 声明 | v0.0.44: C44-STDIO-01，**已修**于 `93aa6f0d59`（方案 B'：每标准流 err 位，未占 slot） | 已修 ✓ | M | 已核 |
| 10 | `-Werror` 被解析、`opts.werror` 无人读，警告仍是警告 rc0 | 编译驱动合同（用户构建脚本按它判红） | `stages.cx:199` 每次发布 `cli/werror` 长度 0；补非空改资源=改权重 | 慢 | v0.0.44: C44-WERROR-01 | 修 bug ✓ | S（代码）+ 慢（权重） | 否 |
| 11 | ~~FFI errno 不桥接~~ 已否证（2026-11-02 实测） | C99 7.5 errno；POSIX 导入边界 | 标准运行库路径**是桥接的**：`open("/nonexistent-…")` 得 -1 且 image 内 `errno==2`；随后 `open("/dev/null")` 得 3、errno 仍为 2（与 glibc "成功后不改 errno" 一致）。原始现象里的 `flock` 在 `.cx` 里根本没有声明（`unknown identifier`），所以那条观察是探针错误，不是缺陷 | `[已核实]` 标准路径正常；`--host-provider` 实测是**死参数**（2026-11-02）：`bx_diag_opt`（`src/entrybind.cx:221`）只查 mode/target 从不打开路径，`hb_provider_filter` 在产品路径无调用者；坏 magic/错 target/保留位/截断/不存在五例全部 rc0 且写镜像。见 `plans/diff-probe-2026-11-02.md` §八与 `tools/check-host-provider.cx`（6 断言，现 failed=5） | v0.0.44: C44-FFI-ERRNO-01 | 待核（宿主导入路径） | S | 否（重定义后） |
| 12 | `UNISA_MAXSTEPS` 原先在产品里被忽略：设 `1000` 编译仍 rc0；非法值 `abc` 也 rc0 | 运行器合同（预算门与具名拒绝） | **已修**：`src/executor.cx` 两处 `core_run` 的常量换成 `ex_budget_now()`（三元短路，失败置 `st=2` 走既有出口，净零行、不新增 `if`）；`cx/runtime/runexec.cx` 复用既有 `ex_budget` + 新增 `ex_dec`/`ex_budget_now`/`ex_budget_used`，非法值与零值按文档行拒绝并返回 2，缺省回落 4e11。**范围说明**：预算限的是编译用的模型解释器步数，不约束用户原生程序的死循环 | 快车道（`src/executor.cx` +140/-5，`cx/runtime/runexec.cx` +41） | 候选 `c3ddcef5…`：`tools/check-budget.cx` 3/3、check-stdio 10/10、check-run-tape 18/18；装根仍红 2（待装机） | 已修（待提交） | S | **是** |
| 13 | 静态表给 lnx/x86_64 切片约 214 MB BSS，RLIMIT_AS <225 MB 即起不来（现靠启动器预检具名拒绝兜底） | 产品在受限环境（容器/CI）可用性 | 静态表未动态化；并入体积/内存工作 | `[已核实]` 证否 | v0.0.44: C44-ASLIMIT-01 | 修 bug ✓ | M | 现状无需修（omp-ds 2026-11-02 实测证否：`prlimit --as=80000000` 下 `int main(void){return 3;}` 仍 rc=3） |

## 三、驱动、链接与包

| # | 现象 | 影响面 | 阻塞原因 | 车道 | 证据 | 0.0.44 | 量 | 快车道 |
|---|---|---|---|---|---|---|---|---|
| 14 | 未定义外部函数被链接成崩溃镜像（应具名拒绝） | 链接合同；产物运行期才炸 | 拟议补丁已起草未应用；模型驱动/经典参考/cxcc 三端要一致 | 慢（src/tapelink.c + 模型） | defects-c43.md:149 `link.undef-function-refusal` | 修 bug ✓ | M | 否 |
| 15 | 畸形输入：两处容器不校验；历史上有一处产品自身崩溃 | 链接器健壮性 | 容器修复已随 Root 4c2831c3 开发安装并验收 | 快（已大部分落地） | defects-c43.md:366/383/417 | 修 bug ✓ | S（余项） | 待核 |
| 16 | 驱动/ar 四处不具名或错因拒绝（诊断质量） | 用户排错成本 | 文案与分支分散在 `exec/c/compiler.c` | 慢（C 参考端）+ 模型端 | defects-c43.md:302 `driver.diagnostics` | 修 bug ✓ | M | 否 |
| 17 | `-nostdinc -E` 仍展开包内 `stddef.h` | C99 6.10.2 include 语义 | e2 资源形状问题（`\0hdr/` ABSENT 行被吞） | 慢（e2 模型） | v0.0.44: C44-NOSTDINC-01；批 `pp-cli-includes` 现 KNOWN_GAP | 修 bug ✓ | M | 否 |
| 18 | 宏展开内诊断列号/摘录按展开文本计（行号正确） | C99 6.10.3；排错体验 | 需 UNIPP1 展开偏移映射 + tokenlocations 读入 + 摘录读原源 | 慢（约 1 天 + 一次纪元换证） | known-gaps R3 = C44-DIAG-COLUMN-01 | 修 bug ✓ | M | 否 |
| 19 | 结构体自含成员拒绝措辞非 C99 `incomplete type`；目录当输入 rc1（旧版 2） | 6.7.2.1p2；退出码一致性 | 改 e3 拒绝表 → 动 SID | 慢 | known-gaps 措辞段；同 C44-DIAG-COLUMN-01 | 修 bug ✓ | M | 否 |
| 20 | ~~`--emit-host-symbols` 接受但不写旁文件~~ 已否证（2026-11-02 实测）：`-S --emit-host-symbols F -o x.tape` 确实写出 12 字节 `UNISYM1\n` + `01 00 00 00` | 侧车合同 LB-1/LB-2 | 写出路径存在；则缺口只剩"内容语义是否正确"（零宿主符号时应是空表？12 字节里的 1 待核） | `[已核实]` 合契约 | v0.0.44: C44-HOSTSYM-01 | 已定案 | S | **无缺陷**（omp-ds 2026-11-02 定案：`01 00 00 00` = target=1、reserved=0、count=0、indirect=0，合 `cx/compiler/hostbind.cx:57` 契约） |

## 四、目标平台、APE、加载器与体积

| # | 现象 | 影响面 | 阻塞原因 | 车道 | 证据 | 0.0.44 | 量 | 快车道 |
|---|---|---|---|---|---|---|---|---|
| 21 | `-S -b <linux> -o FILE.s` 报 `NOT_IMPLEMENTED object` rc2 不写文件；产品路线无汇编文本步（归档参考编译器六目标都能写 GNU 汇编） | 六目标；`-S -b` 用户预期 | 路由步集只有 e1–e4/lower/prune/units/elf | 慢（路线/步集） | known-gaps R4 行 = C44-ASM-TEXT-01 | 功能新增（用户裁定优先级） | 0.0.44 范围外-已裁定顺延 0.0.45 | 否 |
| 22 | Windows on ARM64 无原生入口，跑 x86_64 PE 靠模拟（`-b win/arm64` 写出的用户镜像已是原生） | win/arm64；约 22 行矩阵豁免 | 需 ARM64X 混合 PE 或另发 arm64 exe（改单文件/签名流程） | 慢 | known-gaps = C44-WIN-ARM64-01 | 功能新增 | 0.0.44 范围外-已裁定顺延 0.0.45 | 否 |
| 23 | libunisacc `us_*` 嵌入式 API 不存在（活实现随旧 C 归档） | 库矩阵 33 格豁免 | 先要定它是否属产品 | 慢 | known-gaps = C44-LIBUNISACC-01 | 需主人裁定 | 0.0.44 范围外-已裁定顺延 0.0.45 | 否 |
| 24 | 阶段位置格缺失：Darwin carry、Windows gate 保存/恢复、ARM narrowing 不在产品镜像；lnx/arm64 `-c` 是 `NOT_IMPLEMENTED object`；无独立整镜像编码器 | 6 目标；`-S` 对 Mach-O/COFF 按名拒绝 | 编码器/路线缺口 | 慢 | known-gaps = C44-STAGE-POS-01 | 功能新增 | 0.0.44 范围外-已裁定顺延 0.0.45 | 否 |
| 25 | 加载器不拒绝语义级坏包（改 UNINETB1 动作/越界字段仍能装） | 加载器安全合同 | 需私有重编码器（解压、改、重压、重算 CRC、改页脚） | 慢 | v0.0.44: C44-LOADER-SEMANTIC-01 | 修 bug ✓ | L | 否 |
| 26 | `-S --tapebin` 写出的是文本磁带（cx 编码器缺，解码器好的） | UTAPEBIN 格式合同；`formats-tapebin` 批次/`tapebin.roundtrip` 格 | 编码器只有 C 版；v0.0.44 已记第 1/2 步提交 | 快（cx 编码器 + 行为等价） | v0.0.44: C44-TAPEBIN-01；`tools/check-tapebin.cx` 现 `BYTE-IDENTICAL` | 修 bug ✓ | M | 部分 ✓ |
| 27 | 33 对象入口链接不能导入宿主符号（lnx 两片退到 whole-o） | lnx 产物结构；`release-ape` smoke | tapelink 需按 whole-o 导入方式解析宿主符号，或 `sys.dl` 走 loader 导入表 | 慢 | v0.0.44: C44-ENTRY-HOSTIMPORT-01 | 结构+修 bug ✓ | M–L | 否 |
| 28 | 独立单流容器非产品构建；流运行器依赖已归档 `exec/c/run.c` | 11 格矩阵豁免 | 要从当前树重建流运行器 | 慢 | known-gaps R4 = C44-STREAM-CONTAINER-01 | 功能新增 | 0.0.44 范围外-已裁定顺延 0.0.45 | 否 |
| 29 | 体积：头文件资源已压（387K→95K），主体已落（P3→P4 省 597 KB/29%） | 启动固定成本 | — | 已落地 | v0.0.44 C44-SIZE-01 各节 | 结构/加速 ✓ | — | — |

## 五、属产品但不属「用户可见缺陷」的近期条目（一句话，不展开）

- C44-SPEED-01 编译加速（第一优先级，已实测分相：e3 两遍、lower、elf）；C44-RUN-REUSE-01 `-run` 冷启动复用残留多源不命中；C44-RUN-STARTUP-01/START-01 固定 3.5–3.7 s 启动；C44-QUEUE-START-01、C44-COMBOOT-01/02 构造/队列启动；C44-TEST-01/ORDER-01/ORDER-02/RECOVERY-01 测试结构与失败传播；C44-ARCHIVE-01 归档 536 悬空引用；C44-NATIVE-01 剩 94 个活路径 `.py`；C44-CX-STRUCT-01 `cx/` 目录归位；C44-TOOLS-01 工具链事件化。**这些是 0.0.44 主线（加速+结构），不是产品行为 bug。**

## 六、判定

1. **快车道最优先且最稳的三条**：R15（`-run` 磁带误喂 ELF → 改走 `execf` 执行支，一处判定 + argv 绑定）、C44-STDIO-01（错误状态传播，探针已存在）、C44-PARSE-POSTINC-01 / SIZEOF-POSTFIX（同一地区：后缀表达式解析，两处可一起修）。
2. **量大但价值明确、必须走慢车道**（cc 2026-11-02 已定排期）：先由 omp-bdy 用 **c-negative 23 处 / pp-negative 2 处**打穿「事实表→facts-gen→p2ini」链路；链路通了再排 C44-WERROR-01、C44-NOSTDINC-01、C44-DIAG-COLUMN-01。
3. **五条已裁定顺延 0.0.45**（cc 2026-11-02）：C44-LIBUNISACC-01、C44-ASM-TEXT-01、C44-WIN-ARM64-01、C44-STREAM-CONTAINER-01、C44-STAGE-POS-01（#23 #21 #22 #28 #24）。都是功能新增/跨平台资格，不是修 bug；原文保留作记录，不进 0.0.44。
4. **未核实标记**（omp-ds 2026-11-02 全部实测收敛）：#1 #2 #6 仍红但落在模型/事实表域 → 慢车道，不是快修项；#9 #12 已修（#12 待装机）；#11 的宿主导入路径是死参数（见该行）；#13 实测证否（80 MB 地址空间仍可跑）。
5. **#7（R15）已由实测改写**：原 `.sys` 归因不成立（去掉 `.sys` 仍 rc139，41 行磁带却能跑），真因是磁带路线 run 时误把成品 ELF 喂给 memory 路线；截断磁带的 rc1 拒绝是该误喂的副产品，不是既有拒绝路径。方案见 `out/` 实测证据。
