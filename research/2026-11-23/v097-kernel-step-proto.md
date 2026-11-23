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

## 4. A 类文件清单与上限余量(`a_files_cap=12`)

- **`cx/sys/kernel.cx`**: **不改 (0 文件)** — `CoreModel.packed` (`CM_PACKED=152`) 已存在, `UNIKERN1` 映射按 16 KiB 对齐无尺寸硬拒绝。
- **`exec/c/asm/runcheck.c`**: **不改 (0 文件)** — 宿主建表加 `packed==NULL` 回退保证 `calloc` 序数不变。
- **正式版 A 类清单**:
  - 宿主建表与透传 (2): `cx/runtime/model_decode.cx`、`src/executor.cx`
  - `x86_64` 汇编核 (4): `exec/c/asm/run_x86_64.S`、`exec/c/asm/transition_x86_64.S`、`exec/c/asm/action_x86_64.S`、`exec/c/asm/buffer_x86_64.S`
  - `arm64` 汇编核 (4): `exec/c/asm/run_arm64.S`、`exec/c/asm/transition_arm64.S`、`exec/c/asm/action_arm64.S`、`exec/c/asm/buffer_arm64.S`
  - 可选 ABI 偏移生成 (0 或 2): `exec/facts/core-abi.tsv`、`exec/c/asm/layout.h`
- **合计**: **10 / 12 (余量 2)**; 若含 ABI 偏移入表则为 **12 / 12 (余量 0)**, 严守 `a_files_cap=12`。
