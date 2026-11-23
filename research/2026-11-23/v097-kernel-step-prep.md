# 0.0.97 执行核每步成本(19.8 档)开版前诊断准备

基于 `main` (`d21f46dccf`) 在工作树 `.claude/worktrees/v0.0.97` (分支 `v0.0.97`) 完成只读核查与单次轻量基线采样(未改动任何产品源码)。

## 1. 五项前置核查结果

| # | 核查项 | 结果 | 源码/命令证据 |
|---|---|---|---|
| 1 | `CoreModel.packed` 预留指针与生成偏移 | **已到位** | `exec/c/core.h:36`、`cx/core/execute.cx:34`、`cx/sys/kernel.cx:23` 均含 `const void *packed`; `exec/c/asm/layout.h:24` 由 `exec/facts/core-abi.tsv` 生成 `#define CM_PACKED 152`; `exec/c/asm/layoutcheck.c:9` 已做静态断言 `CHECK(packed, CM_PACKED)` |
| 2 | 核体积增 31% 是否触及尺寸上限 | **不触上限(仍落单 16 KiB 页)** | `cx/sys/kernel.cx:93-98`: `UNIKERN1` 长度上限 2 GiB, 按 16384 字节对齐映射 (`(length + 16383) & -16384L`), 无 8 KiB/16 KiB 硬拒绝。当前单 ISA 核约 7.2 KB, +31% 约 9.5 KB, 仍为 1 页(16 KiB) |
| 3 | `runcheck.c` 失败注入序数约束 | **已核实, 须宿主建表** | `exec/c/asm/runcheck.c:26-41`: `core_host_panic` 断言 `live == (fail_at == 1 ? 0 : 4)`。正式版由宿主装载期建表挂 `CoreModel.packed`, 核入口不做 `calloc`; 当 `packed == NULL`(单测夹具)时回退原路径, 保证 `runcheck.c` 序数零变动 |
| 4 | `UNISA_KERNEL` 私有核装载路径 | **已到位** | `cx/sys/kernel.cx:70-87` 与 `exec/c/asm/binding.c:43-56` 支持 `UNISA_KERNEL` + 可选 `UNISA_KERNEL_SHA`, 可供 `arm64` qemu 先行对拍 |
| 5 | A 类文件清单与封顶核算 | **9-12 / 封顶 12** | 宿主建表 `cx/runtime/model_decode.cx` (+ `src/packmodel.cx` 释放); 两架构 8 个 `.S` (`run`/`transition`/`action`/`buffer` x `{x86_64,arm64}`); 可选 `exec/facts/core-abi.tsv` + `exec/c/asm/layout.h` |

## 2. 0.0.93 已装产品 7 流 `RR_TIME` 基线(单次轻量实测)

条件: `UNISACC_RUN_CACHE=0 UNISACC_RR_TIME=1`, 输入 `hello.c` (`#include <stdio.h>`), `load average: 5.80, 6.84, 6.52` (S3 预跑并发背景):

| 阶段 | `ns` | `steps` | `ns/step` |
|---|---:|---:|---:|
| `e2` | 66,385,800 | 1,575,232 | 42.14 |
| `e1` | 3,815,176 | 36,163 | 105.50 |
| `e3` | 140,921,041 | 5,390,588 | 26.14 |
| `e4` | 64,242,413 | 3,028,436 | 21.21 |
| `prune` | 13,474,698 | 506,394 | 26.61 |
| `lower` | 65,949,497 | 1,651,738 | 39.93 |
| `elf` | 36,590,537 | 1,311,096 | 27.91 |
| **合计** | **391,379,162** | **13,499,647** | **28.99** |

## 3. 下一阶段实施顺序(解冻/开版后)

1. 宿主建表与 `CoreModel.packed` 紧凑状态记录布局入表 (`exec/facts/core-abi.tsv` -> `layout.h`), 保证 `packed == NULL` 时原核行为不变、`runcheck.c` 分配序数不变;
2. `arm64` 私有核移植先行 (`run_arm64.S`、`transition_arm64.S`、`action_arm64.S`、`buffer_arm64.S`), 经 `UNISA_KERNEL` 在 `qemu` 下验字节等价;
3. `x86_64` 四文件移植 (`run_x86_64.S`、`transition_x86_64.S`、`action_x86_64.S`、`buffer_x86_64.S`), 跑交错成对差值(目标中位降幅 >= 6 ns/step、逐阶段步数完全一致、`-o` 与 `check-core-run-diff` 逐字节等价)。
