# 材料重建路径评估（只读）

日期：2026-11-02
作者：omp-go2
范围：`out/aw-m4-mat/attribution.tsv` 的 **class A = 100 行材料不同源批次**；只读，未运行材料构造/重测。
目标：用当前候选的 `.cx` 工具判断哪些材料能重建、哪些卡归档 C/C 引导依赖，给出分层估算与最小第一步。

## 0. 结论

- **Layer 0（当前树，不补 C 源）最多救回 28/100**：
  - `objects` 9、`ri-chain` 7、`pp-gold` 2、`nets` 1、`kernel-route` 9。
  - 这 28 行的材料生成器都是 `.cx`，依赖文件在当前树里齐备；但 `kernel-route` 9 行尚未实跑确认，若失败则 Layer 0 = 19。
- **Layer 1（补 `exec/c/run.c` + `exec/c/core.c`）可到 81/100**：
  - 再救回 `streams` 17 + `streams,stream-*-sha` 16 + `ri-flow-*` 13 + `ri-cli` 7 = 53 行。
  - 这两个 C 文件当前缺失，但 `archive/reference-c/exec/c/` 有原件；`ri-material.cx` 的其他固定输入（`core.h`/`codec.h`/`packagefooter.h`/`resourceledger.h`/`include/unisa_sha256.h`）都在。
- **Layer 2（补外部 `ref`）可到 100/100**：
  - 再救回 `ref` 10 + `streams,ref` 9 = 19 行；`ref` 不是 `.cx`，是经典参考二进制（`/tmp/ua_ref` 或冻结导出 `make ref`）。
- **最小第一步**：先重建 `objects`（`tools/object-material.cx`，约 55s），用当前候选跑一个 class A 的 objects 批次（如 `objects-link`）。不碰冻结源码，就能验证 Layer 0 的第一组 9 行。
- 最大杠杆不是继续写生成器，而是**恢复/迁移 `exec/c/run.c` + `core.c`**：一个动作解锁 53 行。但这是 `exec/` 源码变更，需 cc 批准，且在材料 P0 冻结解除后单独做。

## 1. 口径与当前树事实

- `out/aw-m4-mat/attribution.tsv`：117 数据行；class A = 100。class A 是“材料不同源”归因，不等于批次本身失败原因唯一。
- 当前树检查（2026-11-02）：

| 文件 | 状态 | 用途 |
|---|---|---|
| `exec/c/run.c` | **MISSING** | `tools/stream-runtime.c` 的 `#include "../exec/c/run.c"`；`ri-material.cx` 的固定输入 |
| `exec/c/core.c` | **MISSING** | `run.c` 内 `#include "core.c"`；`ri-material.cx` 的固定输入 |
| `src/asmtext.c` | **MISSING** | `asm-material.cx` 的输入（class A 不含 assembler，class F 有） |
| `exec/c/core.h` | PRESENT | RI 固定输入 |
| `exec/c/codec.h` | PRESENT | RI 固定输入 |
| `exec/c/packagefooter.h` | PRESENT | RI 固定输入 |
| `exec/c/resourceledger.h` | PRESENT | RI/ledger 固定输入 |
| `include/unisa_sha256.h` | PRESENT | RI 固定输入 |
| `seed/ident.c` | PRESENT | object/pp-gold/identity/kernel 的 identity |
| `seed/unisa_libneed.c` | PRESENT | libneed-material |
| `tools/stream-runtime.c` | PRESENT | streams runtime 源 |
| `exec/c/asm/*.S`、`layout.h`、`actions.h` | PRESENT | kernel-route |
| `archive/reference-c/exec/c/run.c` | PRESENT | Layer 1 可恢复 |
| `archive/reference-c/exec/c/core.c` | PRESENT | Layer 1 可恢复 |
| `archive/reference-c/src/asmtext.c` | PRESENT | assembler（非 class A） |

- `tools/ri-material.cx` 固定输入：`exec/c/run.c`、`exec/c/core.c`、`exec/c/core.h`、`exec/c/codec.h`、`exec/c/packagefooter.h`、`exec/c/resourceledger.h`、`include/unisa_sha256.h`、`tools/ri-material.cx` + fixture。除前两个外都在当前树。
- `tools/stream-runtime.c` 直接 `#include "../exec/c/run.c"`；`run.c` 又 `#include "core.c"`。所以 streams 与 RI 共用一个缺口。

## 2. class A 的 100 行按材料分组

| 材料组 | 行数 | 重建路径 | 当前状态 |
|---|---:|---|---|
| `streams` | 17 | `tools/streams.md` 的 runtime 编译 + `tools/stream-pack.cx` | 卡 `exec/c/run.c` |
| `ref` | 10 | 外部经典参考（`/tmp/ua_ref` 或冻结导出 `make ref`） | 非 `.cx`；依赖外部导出 |
| `streams,ref` | 9 | 两者都要 | 卡 `run.c` + ref |
| `route-input-changed`（kernel-route） | 9 | `tools/kernel-route.cx` | 当前树依赖齐；待实跑确认 |
| `ri-flow-runtime` | 9 | `tools/ri-material.cx` + `tools/ri-flow-material.cx` | 卡 `run.c`/`core.c` |
| `objects` | 9 | `tools/object-material.cx` | 可重建 |
| `ri-cli` | 7 | `tools/ri-material.cx --build` | 卡 `run.c`/`core.c` |
| `ri-chain` | 7 | `tools/ri-chain-material.cx` | 可重建 |
| `streams,stream-lower-sha` | 5 | streams + 7 个 SHA 之一 | 卡 `run.c` |
| `streams,stream-e1-sha` | 4 | 同上 | 卡 `run.c` |
| `ri-flow-runtime,ri-flow-data` | 4 | `ri-material` + `ri-flow-material` + `flow-native prepare` | 卡 `run.c`/`core.c` |
| `streams,stream-e2-sha` | 3 | 同上 | 卡 `run.c` |
| `pp-gold` | 2 | `tools/pp-gold-material.cx` | 可重建 |
| `streams,stream-prune-sha` | 1 | 同上 | 卡 `run.c` |
| `streams,stream-elf-sha` | 1 | 同上 | 卡 `run.c` |
| `streams,stream-e4-sha` | 1 | 同上 | 卡 `run.c` |
| `streams,stream-e3-sha` | 1 | 同上 | 卡 `run.c` |
| `nets` | 1 | `tools/net-material.cx` | 可重建 |

## 3. 分层重建矩阵

### Layer 0：当前树 + 现有 `.cx` 工具（28/100）

| 材料 | 行数 | 生成器 | 依赖 | 成本 | 样例批次 |
|---|---:|---|---|---|---|
| `objects` | 9 | `tools/object-material.cx` | `unisacc.com`、`seed/ident.c`、`tools/cxlib/netpack.cx`、`tools/bound` | 3 个对象编译，子预算合计 40s，外层 55s | `objects-link`、`objects-mixed-target`、`objects-unsupported-*` |
| `ri-chain` | 7 | `tools/ri-chain-material.cx` | 静态 NN，无编译 | 12s | `ri-chain-valid`、`ri-chain-nnflip`、`ri-chain-tableflip` |
| `pp-gold` | 2 | `tools/pp-gold-material.cx` | `unisacc.com`、`weights/gold/pp.tsv`、`exec/kernel/body.c`、`seed/ident.c`、`netpack` | prepare 55s | `pp-source-constructed`、`pp-gold-controls` |
| `nets` | 1 | `tools/net-material.cx` | `weights/gold/*.tsv`、`iterate/kernel/*.tsv`、`tests/nets.cx`、`unisacc.com` | 单命令 <55s | `net-controls` |
| `kernel-route` | 9 | `tools/kernel-route.cx` | `exec/c/asm/*.S`、`layout.h`、`actions.h`、`core.h`、`tools/asm-source.cx`、`tools/kernel-link.cx`、`unisacc.com` | 11 单元 × (-E/as/link)；单命令 ≤55s | `kernel-route-encoding`、`kernel-route-relocation`、`kernel-route-object` |

> `kernel-route` 9 行是 Layer 0 里唯一未实跑确认的一组：文件都在，但 `asm-source.cx`/`kernel-link.cx` 需要产品自身能处理 `.S`；建议先跑 `kernel-route-object` 一行验证。若失败，Layer 0 降为 19。

### Layer 1：补 `exec/c/run.c` + `exec/c/core.c`（81/100）

| 材料组 | 行数 | 生成器 | 解锁条件 | 成本 |
|---|---:|---|---|---|
| `streams` 17 + `streams,stream-*-sha` 16 | 33 | `tools/streams.md` 链 + `tools/stream-pack.cx` | `exec/c/run.c`（+它 include 的 `core.c`） | 10 条编译/链接 + pack + make；每条 ≤55s |
| `ri-flow-runtime` 9 + `ri-flow-runtime,ri-flow-data` 4 | 13 | `tools/ri-material.cx` + `tools/ri-flow-material.cx` | `exec/c/run.c` + `core.c` | 55 + 55 + 20s（3 条独立命令） |
| `ri-cli` 7 | 7 | `tools/ri-material.cx --build` | `exec/c/run.c` + `core.c` | 每条 ≤55s |

- 恢复来源：`archive/reference-c/exec/c/run.c`、`archive/reference-c/exec/c/core.c`。
- 注意：`ri-material.cx` 会把 `run.c`/`core.c` 的 hash 写进材料收据；恢复的是归档旧源，材料身份会变成“旧 C + 当前候选”，测试可能仍 FAIL，但不再是“材料不存在/不同源”。
- 这一步会改 `exec/`，属冻结路径；必须 cc 批准并在材料 P0 会话外单独做。

### Layer 2：补外部 `ref`（100/100）

| 材料组 | 行数 | 来源 | 解锁条件 |
|---|---:|---|---|
| `ref` 10 + `streams,ref` 9 | 19 | `/tmp/ua_ref` 或冻结导出 `make ref` | 经典参考二进制可用/可构建 |

- `ref` 不是 `.cx` 生成器产物；若冻结导出还在，直接绑定；否则需要经典参考构建环境，成本不在本仓 `.cx` 工具内。

## 4. 最多能救回多少（分层估算）

| 层 | 动作 | 新增可救 | 累计 | 备注 |
|---|---|---:|---:|---|
| L0 | 当前树，现有 `.cx` 工具 | 28 | 28 | 19 if kernel-route fails |
| L1 | 恢复 `exec/c/run.c` + `core.c` | +53 | 81 | 解锁 streams/RI；改 `exec/` 需批准 |
| L2 | 恢复/构建 `ref` | +19 | 100 | 外部参考，不是 `.cx` |

> 这 100 行是 class A（材料不同源）；即使材料重建成功，测试仍可能因语义/候选差异 FAIL。本文只估“材料可重建”，不是“批次必 PASS”。

## 5. 最小第一步

1. **先跑 objects 一组（不碰冻结源）**：
   ```sh
   tools/bound 55 sh ./unisacc.com tools/object-material.cx -- prepare \
       /home/devali/repos/unisacc /home/devali/repos/unisacc/out/rebuild-objects-01
   # 然后在新会话里 all-waves material objects=<out/rebuild-objects-01>，跑 objects-link
   ```
   成本约 55s + 一个批次；成功则 Layer 0 的 9 行 objects 可复用。
2. **再跑 ri-chain（12s）与 pp-gold（55s）**：分别验证 7 行与 2 行。
3. **kernel-route 先跑 `kernel-route-object` 一行**：确认 Layer 0 的 9 行是否成立。
4. **若目标是把 100 行尽量拉回**：向 cc 提“恢复 `exec/c/run.c` + `core.c`（来自 archive）到私有构建副本”的申请；这是解锁 53 行的唯一单点，但属 `exec/` 变更，且会改变材料身份，必须单独评估。

## 6. 未核实 / 风险

1. `kernel-route` 9 行未实跑；`asm-source.cx`/`kernel-link.cx` 对 `.S` 的处理可能还有缺口。
2. 恢复归档 `run.c`/`core.c` 后，`ri-material`/`stream-runtime` 的编译是否与当前候选 ABI/头文件兼容，未验证；可能还要补 `asm/binding.c` 等（当前在 `exec/c/asm/binding.c`）。
3. `ref` 的 `/tmp/ua_ref` 是临时路径；冻结导出 `unisacc-ref-500d6cf3d8/out/ua_ref` 是否仍在，需确认。
4. class A 是归因分类，不等于“材料重建后必 PASS”；测试语义失败仍可能。
5. 本文未运行任何材料构造；所有成本是工具头注释/文档估计，不是实测。
