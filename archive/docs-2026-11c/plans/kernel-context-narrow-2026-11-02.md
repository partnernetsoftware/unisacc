# kernel-context-narrow：把 kernel 构造的输入身份收窄到真实输入

日期：2026-11-02。只写计划，不改代码。根因：`tools/kernel-build.cx` 的 `context()` 把 kernel 构造的身份绑定到「全树 SID + 10 个工具/源文件 + `unisacc.com` 薄头」，其中只有一部分是 kernel 的真实输入；其余是活文件（每次自举、每次慢车道改动都会变），导致旧 kernel blob 永远无法合法复用、`kernel-build` 也无法从现树重建。本计划把身份收窄到真实输入，并修复 `asm-material.cx` 的已归档路径。

## 1. 现状与过度绑定（已实测）

`tools/kernel-build.cx` 的 `context()`（第 27-34 行）与 `tools/cxlib/kernelcontext.cx` 的 `kc_check`（第 9-30 行）算的是：

```
context = sha256(
  "unisacc-native-kernel-context-v2\n" +
  source_id(root) + "\n"                    # 全树 SID
  tools/kernel-build.cx        <sha>
  tools/asm-source.cx          <sha>
  tools/asm-material.cx        <sha>
  tools/kernel-link.cx         <sha>
  tools/cxlib/netpack.cx       <sha>
  tools/cxlib/elfkernel.cx     <sha>
  tests/assembler.cx           <sha>
  tests/arm-assembler.cx       <sha>
  tests/asm-layout.cx          <sha>
  unisacc.com                  <sha>         # 薄头，每次自举变
  identity-helper              <sha>
)
```

其中 `source_id(root)`（native 路径）用 kgen 里的旧 `identity-helper` 跑当前 root，得到**全树 SID**（读 `exec/src/kernel/include/weights/seed/cx`，旧版还读 `unisa` 目录）。

**三个失败点（实测）**：

1. 旧 kgen 的 `identity-helper`（10-29 生成）是 `6656ea26` 之前的旧版，其 `ident.c` 的 `dirs[]` 仍列 `unisa`，跑当前 HEAD 树报 `ident: cannot open directory: unisa`（HEAD 已把 `unisa` 归档，快照无此目录）。
2. 全树 SID 每次慢车道改动必变（10-29 `afef2ba9` → 当前 `dc33c3ac`），旧 context.sha256 永远不匹配。
3. `asm-material.cx` 输入表第 9 行硬编码 `src/asmtext.c`（该文件 2026-10-29 已归档到 `archive/reference-c/src/asmtext.c`），从现树跑 `prepare` 报 `bounded regular input required`。

## 2. kernel 的真实输入（梳理）

| 输入 | 路径 | 是否 kernel 字节的构成 |
|---|---|---|
| 汇编器源码（22 文件） | `exec/c/asm/{transition,arith,buffer,memory,intern,bytes,format,stack,action,run,bridge}_{x86_64,arm64}.S` | **是**（kernel 是汇编产物） |
| 汇编器 C 源 | `src/asmtext.c`（已归档 → `archive/reference-c/src/asmtext.c`）+ `src/internal_types.h` | **是**（组装出 assembler 源码） |
| 汇编器材料组装器 | `tools/asm-material.cx` | 是（决定 assembler 源码字节） |
| 预处理 | `tools/asm-source.cx`（产品 `-E`） | 是 |
| 链接器 | `tools/kernel-link.cx`、`tools/cxlib/elfkernel.cx`、`tools/cxlib/netpack.cx` | 是 |
| 汇编器测试 | `tests/assembler.cx`、`tests/arm-assembler.cx`、`tests/asm-layout.cx` | **否**（是校验，不是 kernel 字节构成） |
| 薄头 | `unisacc.com` | **否**（kernel 是 UNIKERN1 blob，与薄头无关） |
| 全树 SID | `exec/src/kernel/include/weights/seed/cx` 全树 | **否**（kernel 只依赖 exec/c/asm + 汇编器，不依赖 parse/lex/opt 等） |

**结论**：kernel 的字节由「22 个 `.S` + asmtext + internal_types + asm-material/asm-source/kernel-link/elfkernel/netpack 5 个工具」决定；`tests/assembler*.cx`、`unisacc.com` 薄头、全树 SID 都不该进 context。

## 3. 收窄方案（改哪些工具、身份怎么算）

### 3a. `tools/kernel-build.cx` 的 `context()` 与 `source_id()`

- `source_id(native=1)` 改为**不跑全树 SID**，改为跑一个**收窄的 kernel 源身份**：hash 22 个 `exec/c/asm/*.S` + `src/asmtext.c`（改为归档路径，见 3b）+ `src/internal_types.h`。
- `context()` 的 `extra[]` 从 10 个文件收窄到真实输入：
  - 保留：`tools/asm-material.cx`、`tools/asm-source.cx`、`tools/kernel-link.cx`、`tools/cxlib/elfkernel.cx`、`tools/cxlib/netpack.cx`、`tools/kernel-build.cx`（自身，防工具改动）。
  - 去掉：`tests/assembler.cx`、`tests/arm-assembler.cx`、`tests/asm-layout.cx`（校验，非输入）、`unisacc.com`（薄头）。
  - 新增：`exec/c/asm/*.S` 的逐文件 hash（或一个汇总 hash）、`src/asmtext.c`（归档路径）、`src/internal_types.h`。
- 这样 kernel context = 「真实输入的字节」，不含全树 SID、不含薄头、不含测试。

### 3b. `tools/asm-material.cx` 的输入表

- 第 9 行 `src/asmtext.c` 改为 `archive/reference-c/src/asmtext.c`（归档后纯 C 源仍在此，`internal_types.h` 的 `#include` 也在归档目录内需一起带上）。
- 若归档路径的 `internal_types.h` 与 `src/internal_types.h` 不同，需确认 asmtext 依赖的 `internal_types.h` 在归档目录内是否完整；不完整则把归档的 `asmtext.c` + 其 `internal_types.h` 一并作为 kernel 输入声明。

### 3c. `tools/cxlib/kernelcontext.cx` 的 `kc_check`

- 与 3a 同步：`kc_check` 里的 10 文件名 + 全树 SID，改为同一套收窄输入。`identity-helper` 仍用于「跑一个收窄的 kernel 源身份」，但不再读 `unisa` 目录（用当前 HEAD 的 `seed/ident.c` 编译，或用专门的收窄 ident，见 3d）。

### 3d. `identity-helper` 的 unisa 问题

- 旧 helper 读 `unisa` 是 `6656ea26` 之前的 ident.c。当前 HEAD 的 `seed/ident.c` 已不列 `unisa`（第 103 行 dirs 无 unisa），所以**重新编译 helper 即可**，无需改 ident.c。
- 收窄后 kernel 源身份不依赖 `cx`/`src` 全树，只依赖 `exec/c/asm` + asmtext 相关，`unisa` 目录根本不在输入里，问题自然消失。

## 4. 旧 blob 能否据此合法复用

**能，但要重算 context 并核对**：

- 旧 blob（`out/cx-source-xtarget-kernel-01/{x86_64,arm64}.kernel`，sha `32cecc8a…`/`4d6e45a1…`）与现装 P3 里 `\0kernel/*` 逐字节一致（已实测）。
- 收窄后的 context 只绑「22 个 `.S` + asmtext + internal_types + 5 工具」。这些文件自 10-29 以来**是否变过**，决定旧 blob 能否合法复用：
  - `exec/c/asm/*.S`：cc 上封说「exec/c/asm 在那之后只删过脚本」，`.S` 文件本身大概率未变——**待收窄后实测**。
  - `asmtext.c`（归档）：归档后是静态文件，未变。
  - 5 个工具（asm-material/asm-source/kernel-link/elfkernel/netpack/kernel-build）：`kernel-build.cx` 与 `asm-material.cx` 若在本轮收窄中改动，context 会变；若收窄改动**只动 context 计算逻辑、不动物料字节**，则 kernel 字节不变，可复用旧 blob。
- 复用判定：收窄后，用当前树的真实输入重算 context，与旧 kgen 重算的 context 比对。若**真实输入字节未变**（`.S`、asmtext、internal_types、工具物料部分），则旧 blob 合法，写入新的 `context.sha256` 即可复用。

## 5. kernel-build 能否从现树重建

**能，前置是 3b 的 asm-material 路径修复**：

- 修 `asm-material.cx` 的 `src/asmtext.c` → 归档路径后，`kernel-build prepare`（跑 asm-material）不再报 `bounded regular input required`。
- 后续 `preprocess-ISA`（asm-source 预处理 `.S`）→ `assemble-ISA`（assembler 汇编）→ `link-ISA` 各步不依赖已归档的 C 编译器源，应能完整重建。
- 唯一宿主依赖是 `assembler-native` 用宿主 CC（`-O2 -fno-trim-libc`，声明非 hermetic），与现有契约一致。

## 6. 执行顺序（待审后）

1. 改 `tools/asm-material.cx`：`src/asmtext.c` → `archive/reference-c/src/asmtext.c`（含 internal_types.h 依赖确认）。
2. 改 `tools/kernel-build.cx`：`context()` 收窄（去全树 SID、去 tests 三文件、去 unisacc.com；加 `.S`/asmtext/internal_types hash）。
3. 改 `tools/cxlib/kernelcontext.cx`：`kc_check` 同步收窄。
4. 改 **第 8 节列的其余 4 个校验方**：`tools/cxlib/rebuildcontext.cx`、`tools/cxlib/sourcepackage.cx`、`tools/source-package-adopt.cx`、`tools/source-package-mixed.cx`、`tools/source-package-pack.cx`（连同它们的 assembler 输入表 `src/asmtext.c` → 归档路径）。
5. 重新编译 `identity-helper`（当前 HEAD 的 `seed/ident.c`，已无 unisa）。
6. 新增 `tools/check-kernel-context.cx` 负例 check（第 9 节），登记批次。
7. 快照验证：`kernel-build` 从现树重建，产出的 kernel blob 与旧 blob 逐字节比对；若一致，说明收窄没改变 kernel 字节，旧 blob 合法。
8. 旧 blob 复用判定：收窄后重算 context，若真实输入未变，则旧 blob 复用合法，写新 context.sha256。

## 7. 第 6 点核实结果（omp-cc 条件 1，已实测）

用 `git log --oneline --since='2026-10-29 00:00'` 逐个核查真实输入是否自 10-29 变过：

| 输入 | 自 10-29 改动 | 结论 |
|---|---|---|
| `exec/c/asm/*.S`（22 文件） | 无任何 commit 触及 | 未变 |
| `archive/reference-c/src/asmtext.c` | 仅 `e16028ffa1`（10-29 21:50 归档移动，`src→archive` 重命名 0 字节改动），归档后再无改动 | 未变 |
| `src/internal_types.h` | 最后改动 `b0a0cbc6a7`（10-17），10-29 后无改动 | 未变 |
| `tools/cxlib/elfkernel.cx` | 无 commit | 未变 |
| 其余 5 工具（kernel-build/asm-source/asm-material/kernel-link/netpack） | 有 `tx_eqz` 机械改写（c2b3d5bf26/90a09608f9/669980b461）+ netpack 超时重写（35be30f114 等），但**都是语义等价的机械/对象层改写，不改 kernel 字节** | 待字节比对最终确认 |

**结论：kernel 真实输入的字节未变，旧 blob（`32cecc8a…`/`4d6e45a1…`，与现装 P3 `\0kernel` 一致）可合法复用**。最终判定以收窄后从现树重跑 `kernel-build` 产出的 kernel blob 与旧 blob 逐字节比对为准（见第 5/6 节）。

## 8. 关键发现：context 计算逻辑硬编码在 7 个文件，必须全量同步

`unisacc-native-kernel-context-v2` 的「10 文件 + 全树 SID」口径不是只在一处，而是硬编码在 **7 个文件**里，收窄必须全部同步改，否则构造方与校验方算出的 context 会 mismatch（`equal(p,&expected)` 失败）：

| 文件 | 角色 | context 计算位置 |
|---|---|---|
| `tools/kernel-build.cx` | 构造方（bootstrap 写 context.sha256） | `context()` 第 28 行 + `source_id()` 第 18 行 |
| `tools/cxlib/kernelcontext.cx` | 校验方（kc_check） | 第 11 行 |
| `tools/cxlib/rebuildcontext.cx` | 校验方（rb_kernel_context + rb_assembler） | 第 49-50 行 |
| `tools/cxlib/sourcepackage.cx` | 校验方（sp 验证） | 第 195（assembler）+ 200（context）行 |
| `tools/source-package-adopt.cx` | 校验方 | 第 85（assembler）+ 90（context）行 |
| `tools/source-package-mixed.cx` | 校验方 | 第 101（assembler）+ 106（context）行 |
| `tools/source-package-pack.cx` | 校验方（慢车道关键路径） | 第 167（assembler）+ 172（context）行 |

同样的 `src/asmtext.c`（已归档）硬编码也散布在 asm-material + rebuildcontext + sourcepackage + source-package-adopt + source-package-mixed + source-package-pack 共 6 个文件的 assembler 输入表里。收窄要一并修。

## 9. 负例 check（omp-cc 条件 2）

收窄后新增 `tools/check-kernel-context.cx`，负例断言：

1. **改一个 `.S` 字节 → context 变**：复制私有根，改 `exec/c/asm/transition_x86_64.S` 末尾一个字节，重算 context，断言与新 context 不同（正例：真实输入进 context）。
2. **改 `tests/assembler.cx` → context 不变**：改一个测试文件字节，断言 context 相同（负例：测试非输入）。
3. **改 `unisacc.com` 薄头 → context 不变**：替换薄头，断言 context 相同（负例：薄头非输入）。

check 输出 PASS/FAIL，登记到 `tests/batches.tsv`（用 `tools/register.cx`），作为下一次纪元慢车道的批次。

## 10. 未核实项（收窄后实测）

1. 收窄后的 context 是否覆盖 kernel 字节的全部决定因素（是否有漏掉的输入，如 `kernel-link.cx` 读的额外表）。
2. 5 个工具自 10-29 的机械改写是否真未改变 kernel 字节（以重跑 kernel-build 字节比对为准）。
