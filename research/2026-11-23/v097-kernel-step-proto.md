# 0.0.97 执行核每步成本(19.8 档)宿主建表布局草稿与最小原型实测

分支: `v0.0.97` (工作树 `.claude/worktrees/v0.0.97`, 未触共享检出与产品冻结域)。

## 1. 宿主建表布局草稿与生命周期

### 1.1 `CorePackedState` 紧凑布局(64 字节 = 1 条缓存行)

```c
typedef struct {
    int mode;                    /* +0  */
    int count;                   /* +4  */
    int lo;                      /* +8  */
    int hi;                      /* +12 */
    int base_next;               /* +16 */
    int base_seq;                /* +20 */
    int ret_seq;                 /* +24 */
    int pad;                     /* +28 (zero) */
    const unsigned char *ret_ok; /* +32 (NULL when m->ret_ok == NULL) */
    const int *keys;             /* +40 */
    const int *next;             /* +48 */
    const int *seq;              /* +56 */
} CorePackedState;               /* sizeof == 64 */
```

- 定位方式: `packed + ((uint64_t)q << 6)`, 每步仅 1 次位移加法命中单条 64B 缓存行, 取代原 `CM_MODE`/`CM_RET_OK`/`CM_RET_SEQ`/`CM_LO`/`CM_HI`/`CM_BASE_NEXT`/`CM_BASE_SEQ`/`CM_COUNT`/`CM_KEYS`/`CM_NEXT`/`CM_SEQ` 11 个分离数组的 11 次指针追寻。

### 1.2 谁建、何时建、何时释放与失败注入序数

- **谁建与何时建**: 宿主 `cx/runtime/model_decode.cx` 在 `cxmd_decode` 解码成功末尾分配 `calloc(ns, 64)` 并一次性填充 `CxModel.packed`; `src/executor.cx:ex_model` 仅做指针透传 `m->packed = src->packed`。模型复用路径 (`ex_open`/`ex_step`) 跨多次 `core_run` 零重建成本。
- **何时释放**: `cx/runtime/model_decode.cx` 的 `cxml_release` 随模型其他数组一并释放 `packed`。
- **失败注入序数 (`exec/c/asm/runcheck.c:26-41`)**:
  - `runcheck.c` 注入失败针对的是 `core_run` 内部的 `calloc` 序数 (`fail_at=1 -> live==0`, `fail_at=5 -> live==4`)。
  - 正式版 `core_run` 入口**不做任何 `calloc`**: 直接读 `CM_PACKED(%rbx)`; 当 `m->packed == NULL` (独立夹具 `runcheck.c`/`transitioncheck.c` 未走宿主解码器) 时回退原未打包路径。
  - 结论: `runcheck.c` 失败注入序数 **0 变化**, **不需修改 `runcheck.c`**。

## 2. 可跑最小原型实测 (`hello.c`, 5 对交错, 同口径)

- 原型文件: `research/2026-11-23/v097-proto-run-x86_64.S` (仅含「64B `CorePackedState` + 转移内联进 `run_x86_64.S` 主循环」, 尚不含动作循环并入 `core_action`、输出追加快路径与尾部线程化分派)。
- 构造方式: 用 `tools/asm-source.cx` + `gcc -c` + `tools/kernel-link.cx` 编出 `base.kernel` (`7032` B) 与 `proto.kernel` (`7664` B, `+632 B` / `+9.0%`, 仍落单个 16 KiB 页), 通过 `UNISA_KERNEL` 注入同一 `0.0.93` 产品二进制 (`UNISACC_RUN_CACHE=0 UNISACC_RR_TIME=1`)。
- 注: 因 `0.0.93` 已装宿主尚无 `m->packed`, 原型核在 `m->packed == NULL` 时于入口临时 `calloc(ns, 64)` 建表并在退出前释放(建表开销已计入下方 `proto` 耗时, 正式版宿主建表将进一步省去该开销)。
- 实测环境: `load average: 8.13, 6.30, 6.23` (S3 预跑并发背景), `hello.c` (`#include <stdio.h>`), 5 对交错原行:

```text
PAIR	1	steps=13499647	base_ns_step=28.89	proto_ns_step=22.08	diff=6.81
PAIR	2	steps=13499647	base_ns_step=31.03	proto_ns_step=23.30	diff=7.73
PAIR	3	steps=13499647	base_ns_step=28.96	proto_ns_step=24.28	diff=4.68
PAIR	4	steps=13499647	base_ns_step=29.82	proto_ns_step=24.68	diff=5.14
PAIR	5	steps=13499647	base_ns_step=35.64	proto_ns_step=26.03	diff=9.61
BENCH_SUMMARY	pairs=5	steps=13499647	same_steps=1	same_sha=1	base_med=29.82	proto_med=24.28	paired_diff_med=6.81
```

- 正确性核对: 全部 10 次运行 7 流合计 `steps=13499647` 逐流完全相等 (`same_steps=1`); 全部 10 次产物二进制 `sha256=d5b28bcbb1ec92e36091c0d96b4cebf3c6bde32c28c0c6bb7707f4201ab8fb61` 逐字节完全相等 (`same_sha=1`)。

## 3. 收益换算与不可证伪/撤出条件

### 3.1 收益换算(诚实口径)

- **单步实测**: 同批 5 对交错中位差 **`-6.81 ns/step`** (`29.82 -> 24.28`, `-22.8%`; 相对单次基线 `28.99` 降 `23.5%`; 叠满 19.8 档动作线程化后预计再降 1.5-2.5 ns/step)。
- **单次冷跑 (`hello.c`, 13.50M steps)**: 理论推理省 `6.81 ns * 13.50M = 91.9 ms` (总墙钟 ~450 ms 降约 17%-20%, 诚实对折约 **46-92 ms**)。
- **U7 冷 `-o` 编译 (`43.4 s`)**: 按推理占单核冷编 70%-77% 推断(未实测), 每步降 22.8% 预计降单核冷编约 **16%-18%** (省约 **6.9-7.8 s**, `43.4 s -> ~35.6-36.5 s`), 与 U7 单元并行/缓存正交叠加。
- **整链 (`27.8 min`) 与 `s9` / 预跑**: 按推理占比 45%-65% 推断(未实测), 理论降 **10%-15%**, 诚实对折口径约 **6%-10%** (27.8 min 整链省约 **1.7-2.8 min**, 预跑同比例)。

### 3.2 不可证伪与撤出条件(触发任一即撤出 `0.0.97`)

1. **收益不达标**: 在低载 (`load1 < 4`) 下 9 对交错成对实测 `x86_64` 中位降幅 **`< 6.0 ns/step`** (或降幅 `< 18%`), 立即撤出 `0.0.97`;
2. **语义/等价回退**: 任一阶段 `steps` 变动, 或 `check-core-run-diff`、`-o` 17 份整程序、两代内核 `g2 == g3` 出现任何 1 字节差异;
3. **宿主建表开销反噬**: `modelphase layout` 因构建 `CorePackedState` 增加耗时超过推理节省的 `10%` (`> 9 ms` on `hello.c`);
4. **越界破封顶**: 正式实现所需 A 类文件超过 `a_files_cap=12`。

## 4. 决定与 A 类文件清单(`a_files_cap=12`)

### 4.1 架构与范围决定

1. **`CorePackedState` 偏移入 `exec/facts/core-abi.tsv`**: **采纳入表** (`PS_MODE=0`..`PS_SEQ=56`, `PS_SIZE=64`), 经 `tools/core-abi-gen.cx -- --write` 生成 `exec/c/asm/layout.h`, 实测 `CORE_ABI_GEN OK failed=0`, 严守「事实表 -> 生成器 -> 表」。
2. **动作循环并入与尾部线程化移出 `0.0.97`**: 「动作循环并入 `core_action`、输出追加快路径、尾部线程化分派」明确列为 **`0.0.97` 之外的后续候选**, 不在本版扩范围, 故 `action_*.S`、`buffer_*.S`、`transition_*.S` 全部不改。
3. **不改文件**: `cx/sys/kernel.cx` (0)、`exec/c/asm/runcheck.c` (0)。

### 4.2 正式版 A 类清单(7-8 / 封顶 12, 余量 4-5; 严禁扩范围)

- 事实表与 ABI 布局 (3-4): `exec/facts/core-abi.tsv`、`exec/c/asm/layout.h`、`exec/c/core.h` (+ `exec/c/asm/layoutcheck.c` 静态断言)
- 宿主建表与透传 (2): `cx/runtime/model_decode.cx`、`src/executor.cx`
- 两架构汇编核 (2): `exec/c/asm/run_x86_64.S`、`exec/c/asm/run_arm64.S`
- **合计**: **7-8 / 12 (余量 4-5)**, 即使 `core-abi.tsv` 入表也远低于 12 封顶。

## 5. `arm64` 原型编过与 `qemu-aarch64-static` 7 流逐流对拍原行

- 原型文件: `research/2026-11-23/v097-proto-run-arm64.S` (引用 `core-abi.tsv` 生成的 `PS_*` 宏, 零触 `x18`/`x28`)。
- 构造产物: `base_arm64.kernel` (`UNIKERN1 arch=1 bytes=7024 entry=6644 slot=7016`) vs `proto_arm64.kernel` (`UNIKERN1 arch=1 bytes=7680 entry=7296 slot=7672`, `+656 B` / `+9.3%`, 仍落单个 16 KiB 页)。
- 实跑方式: 提取 `0.0.93` `unisacc.com` 内置 `Linuxaarch64` 原生切片, 经 `UNISA_KERNEL` 分别注入 `base_arm64.kernel` 与 `proto_arm64.kernel`, 在 `qemu-aarch64-static` 下编 `hello.c` (`UNISACC_RUN_CACHE=0 UNISACC_RR_TIME=1`):

```text
=== ARM64 BASE ===
RR_TIME	inference	stage=e2	rc=0	ns=596059078	steps=1575220
RR_TIME	inference	stage=e1	rc=0	ns=30753010	steps=36163
RR_TIME	inference	stage=e3	rc=0	ns=1292938000	steps=5390590
RR_TIME	inference	stage=e4	rc=0	ns=698272996	steps=3028436
RR_TIME	inference	stage=prune	rc=0	ns=97613502	steps=506394
RR_TIME	inference	stage=lower	rc=0	ns=586666712	steps=1801995
RR_TIME	inference	stage=elf	rc=0	ns=433966088	steps=1697355
=== ARM64 PROTO ===
RR_TIME	inference	stage=e2	rc=0	ns=516826324	steps=1575220
RR_TIME	inference	stage=e1	rc=0	ns=26814919	steps=36163
RR_TIME	inference	stage=e3	rc=0	ns=1026472995	steps=5390590
RR_TIME	inference	stage=e4	rc=0	ns=535225193	steps=3028436
RR_TIME	inference	stage=prune	rc=0	ns=79662012	steps=506394
RR_TIME	inference	stage=lower	rc=0	ns=433868868	steps=1801995
RR_TIME	inference	stage=elf	rc=0	ns=341597346	steps=1697355
=== SHA256 ===
dc2b6459afa0cf3de29e25ff45cfdcbe76beb0ca6ff65fdb0922726961147f3e  h_base_arm64
dc2b6459afa0cf3de29e25ff45cfdcbe76beb0ca6ff65fdb0922726961147f3e  h_proto_arm64
```

- 核对结论: 7 流合计 `14,036,153` 步逐流完全一致, `arm64` 产物二进制 `sha256` 逐字节完全一致, `qemu-aarch64-static` 实跑输出 `hi`(qemu 仿真耗时 `3736.27 ms -> 2960.47 ms` 仅作趋势参考, 原生达标计时待低载实机/CI)。
