# tools 写死身份与路径普查（2026-11-02）

只读调查，未改任何代码。范围：`tools/*.cx`（120 个文件）与 `tools/cxlib/*.cx`（约 40 个）。
判据统一：**目标存在性 + 当前 sha 与 pin 比对 + `git log --diff-filter=D` 找断裂提交**。构造链按 cc 顺序排最前。

## 0. 摘要

| 指标 | 实测 |
|---|---|
| 含 64 位十六进制常量的行 | 338（`grep -rE '"[0-9a-f]{64}"'` 的行数），共 420 个 pin 值 |
| 写死路径字面量（含 `/` 且首段为已知目录） | 2509 处；其中不存在 849 处，去重后带扩展名 137 个 |
| 自动按邻行配对判 BROKEN | 64 条；人工复核后**真断裂 9 组**，其余为「多 pin 同窗口错配」或产物/自引用 |
| 已确认的三种腐烂 | ① `exec/c/*.c` + `src/*.c` 旧 C 源（`e16028ffa1`）② 34 个 `cx/compiler` 门模块（`bb21168edf`）③ 计数 34/107 → 42/131（`5760e9e8a9`）未同步到 verify/adopt/mixed |

## 1. 方法

```sh
# pin 与邻行路径配对 + sha 比对（脚本在 /tmp，未入仓）
grep -rnE '"[0-9a-f]{64}"' tools/*.cx tools/cxlib/*.cx
# 路径存在性
grep -rnoE '"[A-Za-z0-9_./%-]{3,160}"' tools/*.cx tools/cxlib/*.cx   # 首段限定 cx|src|include|exec|seed|kernel|weights|tools|tests|out
# 断裂提交
git log --diff-filter=D --format='%h %ad %s' --date=short -2 -- <path>
```

配对局限：同一窗口里多个 pin 与多个路径时按顺序对齐会错位（典型是 `paths[]`+`pins[]` 数组）。
下表只列**人工复核过上下文**的条目；标 `待核` 的表示目标判断来自自动配对，未逐句确认。

## 2. 现行构造链（cc 排序，优先处理）

| 工具 | 行 | 类型 | 目标 | 状态 | 断裂提交 | 修法 |
|---|---|---|---|---|---|---|
| `tools/source-job-wave.cx` | 4,5,17,37,39,47 | 计数 | 131 jobs / 42 gen·tbl·net | **一致**（`nids!=131||counts!=42`） | — | 保留；它是权威源，其他工具应从这里派生 |
| `tools/source-package.cx` | 90,104 | 路径 | `tools/bound`、`tools/cxlib/netpack.cx`、`seed/ident.c`、`seed/%s.c` | OK（模板 `%s` 除外） | — | 保留 |
| `tools/release-build.cx` | 389 | 路径 | `include/cx-flat.h` | OK | — | 保留 |
| `tools/release-build.cx` | 391 | 错误标签 | `"cx-flat.h"`（`rb_die`） | 误报 | — | 不改 |
| `tools/release-build.cx` | 535-567,640,729,825 | 路径 | `src/*.cx` 53 条 + `tools/bound` + `tools/entry-build.cx` | 全部 OK | — | 保留 |
| `tools/entry-build.cx` | 336,753,756,853 | 中间产物名/错误标签 | `cxmod.o`、`executor.o`、`cx-plan` | 误报（生成物与标签） | — | 不改 |
| `tools/entry-build.cx` | 732-776 | 路径 | `src/*.cx` 45 条 + `tools/bound` | 全部 OK | — | 保留 |
| `tools/release-ape.cx` | 95,115,117 | 输出路径/标签 | `out/release-ape-smoke` 等 | OK | — | 保留 |
| `tools/kernel-build.cx` | 22,29,35-57 | 路径 | `seed/ident.c`、`tools/asm-*`、`tests/assembler.cx` 等 | 全部 OK | — | 保留 |
| `tools/kernel-build.cx` | 51 | 模板 | `exec/c/asm/%s_%s.S` | 生成物模板，目录 `exec/c/asm` 存在 | — | 保留 |
| `tools/kernel-route.cx` | 37-76 | 路径 | `exec/c/asm`、`tools/*.cx` | OK | — | 保留 |
| `tools/kernel-emit.cx` | 56,134,346-711 | 路径 | `weights/built.json`、`exec/facts/kernel-emit.tsv`、`exec/kernel/body.c` | OK | — | 保留 |
| `tools/kernel-emit.cx` | 167,696 | 模板 | `weights/gold/%s.tsv` | 生成物模板（`gold/` 目录存在） | — | 保留 |
| `tools/all-waves.cx` | 51-264 | 断言标签 | `outer-bound-required`、`executor-*` 等 | 误报（错误串） | — | 不改 |
| `tools/zero-wave.cx` / `selfhost-mixed.cx` | — | — | 无 pin；路径全 OK | OK | — | 保留 |
| **`tools/product-build.cx`** | **74,76,78** | **路径** | `include`、**`exec/c`**、**`exec/c/asmcompiler.c`**、`-I/usr/include/node -lz` | **BROKEN**：`exec/c/asmcompiler.c` 已归档，`exec/c` 现只剩文档与头 | **`e16028ffa1` 2026-10-29 archive old c ref** | 见 §5：确认是否退出链；在链上就改走 `src/compiler/*.cx`，系统 zlib 改登记 host-dep |

## 3. 链外但仍在 tools 里的工具

### 3.1 真断裂（人工复核）

| 工具 | 行 | 类型 | 目标 | 状态 | 断裂提交 | 修法 |
|---|---|---|---|---|---|---|
| `tools/source-package-pack.cx` | 77 | 64hex pin | `tools/source-package.cx`：pin `ddeb0021…`，现 `9a638515…` | **BROKEN** | `fe8028e28e` 2026-10-29 Add cross-target unit model recipes and package routes | 改为派生：打包时读 `source-package.cx` 现 sha 写进记录，或把 pin 换成该文件的 build.json 记录 |
| `tools/source-package-adopt.cx` | 73,116 | 计数 | 107 jobs / 34 models | **STALE**（权威值 131/42） | `5760e9e8a9` 2026-11-02 source-job-wave 34/107→42/131 | 删字面量，改从 inventory 派生 |
| `tools/source-package-mixed.cx` | 89,182 | 计数 | 同上 | **STALE** | 同上 | 同上 |
| `tools/source-package-mixed.cx` | 38 | 64hex pin | `basis_pin=13a58d0e…`（待核目标） | **待核** | 待核 | 同 pack 的 basis 处理 |
| `tools/source-origin-verify.cx` | 173 | 计数 | `jobs!=107||models!=34` | **STALE** | `5760e9e8a9` | 改读 inventory 的 131/42，并在值不符时报 `inventory-count-mismatch` |
| `tools/asm-material.cx` | 9 | 路径表 `inputs[]` | `src/asmtext.c` 不存在（其余 6 条 OK） | **BROKEN** | `e16028ffa1` | 删该条并把生成注释里的来源改为 `cx/compiler/asmtext.cx`（现存的对应实现） |
| `tools/cxpack-source-delta.cx` | 35,36,40,41 | 内容 pin | `src/main.c`、`exec/c/compiler.c`、`src/cxmod.c` | **BROKEN**（文件不存在，pin 无法校验） | `e16028ffa1` | 该工具钉旧 C 源，退出链；要保留就改为 `--verify` 现源 |
| `tools/link-source-delta.cx` | 24 | 内容 pin | `src/tapelink.c` | **BROKEN** | `e16028ffa1` | 同上 |
| `tools/prune-cases-gen.cx` | 34 | 路径 | `src/tapeprune.c` | **BROKEN** | `e16028ffa1` | 同上 |
| `tools/cxlib/rebuildcontext.cx` | 50；`tools/cxlib/sourcepackage.cx` | 195 | 路径 | `src/asmtext.c` | **BROKEN** | `e16028ffa1` | 改 `cx/compiler/asmtext.cx` |
| `tools/cxlib/e2stream.cx`；`tools/cxlib/riflowmaterial.cx`；`tools/check-model-decode-real.cx` | 258；5；42 | 路径 | `exec/c/run.c` / `exec/c/core.c` | **BROKEN** | `e16028ffa1` | 这两个文件已归档，检查项应改为对 `src/runtime/*.cx` |
| `tools/cx-driver-build.cx`；`tools/cx-library-build.cx`；`tools/hostseed-step.cx`；`tools/hostseed-step-nativefirst.cx` | 48；252,255；100；63,118 | 路径表 | 34 个 `cx/compiler/*.cx` 旧门模块 + `exec/c/*.c` | **BROKEN** | `bb21168edf` 2026-10-30 / `e16028ffa1` | 这些是旧驱动构造器，退出链；保留则整表改 `src/compiler/bindstage.cx` 等新模块 |
| `tools/identity-material.cx` | 61,65,67 | 路径 | `cxlib/kernelsha.cx`、`kernelsha.cx`（缺 `tools/` 前缀） | **BROKEN**（实际存在 `tools/cxlib/kernelsha.cx`） | 未定位（可能一直是错字） | 补 `tools/` 前缀 |
| `tools/cxlib/mixedprofiles.cx` | 22,29 | 产物路径 | `out/epoch-build-01/Root41-bootstrap.com`、`Rootf370-bootstrap.com` | **MISSING**（`out/` 可再生） | 未定位（纪元产物） | 改为按纪元参数传入；禁止默认路径 |
| `tools/check-cx-entry.cx` | 29,701 | 路径 | `cxstd_main.cx`、`tests/cx-entry-matrix-2026-10-30.tsv` | **MISSING** | 待核 | 改名或删检查 |
| `tools/cx-flatten.cx` | 262 | 路径 | `cx-nested.h` | **MISSING** | 待核 | 改为参数 |
| `tools/check-matrix-stale.cx` | 37 | 路径 | `tests/x-domain-matrix.tsv` | **MISSING** | 待核 | 删或改指向现行矩阵 |
| `tools/cxpack-read-failures.cx` | 9 | 路径 | `cxpack.tsv` | **MISSING** | 待核 | 改为参数 |

### 3.2 必然过期（纪元材料 / 钉产物）

| 工具 | 行 | 类型 | 目标 | 结论 |
|---|---|---|---|---|
| `tools/{cxpack,link,tapelink,fifo,driverdiag}-candidate-check.cx` | 18-27 等 | 64hex pin | `unisacc.com`、`tools/cxpack-source-delta.cx`、`context.tsv`、`identity-helper` | **纪元绑定**：钉的是某次候选快照，每次重建必过期；不修 pin，标注「一次性证据工具，不入链」 |
| `tools/cxpack-candidate-install.cx`、`tools/link-candidate-install.cx` | 23 | 64hex pin | `unisacc.com` | 同上 |
| `tools/classic18-product-nets.cx` | 31 | 64hex pin | `unisacc.com` | 同上 |
| `tools/pp-gold-material.cx` | 14 | 64hex pin | `unisacc.com` | 同上 |
| `tools/dev-run.cx` | 77 | 64hex pin | `tools/bound`（二进制，每次重建变） | 改为运行时读 build.json |
| `tools/cxlib/richain.cx` | 15 | 自引用 pin | 文件自身 sha | 设计缺陷：自钉必然断。改 `digest(__FILE__)` 运行时比较或删 |
| `tools/libneed-native-profile.cx` | 23 | 自引用 pin | 自身/材料 | 同上，待核 |
| `tools/hostseed-step.cx`；`hostseed-step-nativefirst.cx` | 25；37 | 系统绝对路径 + pin | `/usr/include/node/zlib.h`、`zconf.h`、`libz.so.1.2.11` | **当前 SAME**（71e0dcf4… / 3f928c8… / 64c206f0…），但宿主机升级即断；登记为 host-dep，不硬失败 |

### 3.3 误报（多 pin 同窗口错配，人工复核否定）

`tools/cxlib/adoptedsnapshot.cx:50,51`、`tools/cxlib/mixedsnapshot.cx:40`、`tools/cxlib/nninputs.cx:177`：
pin 是格式常量或材料哈希，邻行 `support[]`/`nninputs.cx` 不是它的目标。不改。

## 4. 写死计数清单

| 位置 | 值 | 权威值 | 状态 |
|---|---|---|---|
| `source-job-wave.cx:4,5,17,39,47` | 131 / 42 | 131 / 42 | 一致 |
| `source-package-adopt.cx:73,116` | 107 / 34 | 131 / 42 | 过期 |
| `source-package-mixed.cx:89,182` | 107 / 34 | 131 / 42 | 过期 |
| `source-origin-verify.cx:173` | 107 / 34 | 131 / 42 | 过期 |
| `check-cx-fmt.cx:160`、`ucn-identity-tables.cx:66-68` | `"42"`、`"34"` | 无关（数字解析用例/Unicode 表） | 误报 |

## 5. 修法建议

**应当改为派生（删除字面量）**

1. 所有 34/107 计数：唯一权威是 `source-job-wave.cx` 的 inventory。verify / adopt / mixed 改读 inventory 输出的 `jobs`/`models` 行，不符时报具名错误。否则每次食谱新增都要改三处（本次 `5760e9e8a9` 只改了一处，另三处已腐烂）。
2. `source-package-pack.cx:77` basis_pin：改为在打包时把 `tools/source-package.cx` 的现 sha 写进记录，校验侧比较「记录 vs 现文件」，不写死。
3. 钉产物（`unisacc.com`、`tools/bound`、`out/**`）的 pin：全部改为从 `<product>.build.json` / 纪元记录派生；`--verify` 语义保留。
4. 自引用 pin（`richain.cx:15`、`libneed-native-profile.cx:23`）：删掉，改运行时 `digest(__FILE__)` 或纯结构校验。

**应当删除或退出链**

5. 旧 C 链工具（`cxpack-source-delta`、`link-source-delta`、`prune-cases-gen`、`cx-driver-build`、`cx-library-build`、`hostseed-step*`、`product-build` 的 `exec/c` 分支）：文件已归档（`e16028ffa1`），保留只会继续撞。若仍有证据价值，移到 `archive/reference-c/tools/`，tools/ 里只留一条 README 指向。
6. 34 个 `cx/compiler` 门模块引用（`product-build.cx:75`、`cx-driver-build.cx:48`、`hostseed-step*`）：同上，新链用 `cx/compiler/bindstage.cx`。

**应当保留**

7. 现行链里的路径清单（`release-build.cx:535-567`、`entry-build.cx:732-776`、`product-build.cx:75` 中仍存在的模块）：这些是「显式源清单」，属于可审计的合同，不要抽象成通配目录。
8. 系统 host-dep（zlib 三路径）：保留 pin，但登记为 `host-dep` 并在不匹配时给 `HOST-DEP-CHANGED`（提示换版本），而不是 `die` 掉整个构造。
9. 纪元材料（candidate-check 族）：保留，标注「一次性证据」，并在文件头写明它绑定哪一次候选 sha。

## 6. 防腐方案

### 6.1 新表 `tools/pins.tsv`

列：`path`（哪个工具的哪一行）/`kind`（pin|count|path|host-dep|epoch）/`target`（目标文件或「derived:source」）/`value`/`owner`。
现状普查结果直接落成这张表（§2、§3 的行），一次录入，之后由检查器驱动。

### 6.2 新检查 `tools/check-pins.cx`

- 逐行实测：`path` 存在（仅 ENOENT 判缺失；ENOTDIR/EACCES 报错）、`kind=pin` 时 sha 相等、`kind=count` 时与权威 inventory 相等、`kind=host-dep` 时不匹配只报 `HOST-DEP-CHANGED` 不 rc=1。
- 退出码：`pin`/`path`/`count` 过期 → rc=1；`host-dep` → rc=0 但打印告警。
- 输出：`CHECK_PINS pins=N failed=M stale=S hostdep=H`，逐条 `FAIL<TAB>kind<TAB>path<TAB>target<TAB>detail`。
- 登记批次 `pins`（批次表新增一行，source 为 `tools/pins.tsv`）。

### 6.3 让过期在改动提交时立刻红

- `tools/affected.cx` 的依赖表（`plans/affected-deps-plan-2026-11-02.md` 的 P1/P2）增加 `role=pin` 行：被 pin 的文件（`tools/source-package.cx`、`src/*.cx`、`tools/bound`）一旦改动，映射到 `pins` 批次；`tools/own-diff.cx` 报告里同时打印「本次改动的文件被哪些 pin 钉住」。
- 规则硬约束（检查器实现）：工具里出现 64 位常量 pin 而 `pins.tsv` 无对应行 → `UNDECLARED_PIN` rc=1；出现 `"107"`/`"34"` 这类计数字面量而权威值不同 → `COUNT_LITERAL` rc=1。
- 本地开发：`check-pins` 必须 ≤2s（表驱动，不做全树扫描），可放进 `tools/ratchets.cx` 同一次调用。

### 6.4 不建议

- 不做「自动重写 pin」：那会把「谁批准了新身份」抹掉。
- 不在驱动/模型里加特判来容忍过期 pin（`drive.cx`/`host.cx` 不写平台/身份特判的既有约束照旧）。

## 7. 未核实

- `source-package-mixed.cx:38` `basis_pin` 的目标文件未逐句确认（标待核）。
- `identity-material.cx` 的 `cxlib/kernelsha.cx` 是否一直是错字（未找到引入提交）。
- `mixedprofiles.cx` 的 `out/epoch-build-01/*.com` 属于哪个纪元、当前是否仍需（out 已不存在）。
- `check-cx-entry.cx`、`cx-flatten.cx`、`check-matrix-stale.cx`、`cxpack-read-failures.cx` 的缺失目标是否已改名（待核）。
- 自动配对出的另外 55 条 BROKEN 未逐条复核，绝大多数为多 pin 同窗口错配。
