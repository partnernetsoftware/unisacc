# 164 有材料批次的材料盘点（只读）

日期：2026-11-02
作者：omp-go2
范围：`tests/batches.tsv` 中 `implementation=IMPLEMENTED && materials!=-` 的 164 行。
性质：只读调研；未运行 `all-waves`、未执行任何材料构造/重测。

## 0. 结论

- 164 行、59 种材料名（按逗号拆分、去掉 `:kind`）。
- 最大族：`streams` 60 行（含 18 行 `stream-*-sha`）、`ref` 23、`ri-flow` 15、`p3` 9、`objects` 9、`kernel-route` 9、`ri-cli` 8、`ri-chain` 8、`host` 9、`selfhost` 6、`identity` 8、`small` 13（族间重叠，总和大于 164）。
- **按目录存在性计，134/164 行可由现有已备材料路径 + 7 个 stream SHA 字面量直接覆盖**：注册后只做 content/fingerprint 重测（每材料 <1s），不需要构造。其中 `identity-helper` 的 `out/tb/mat-identity` 为空，实际 133 可直接复用；该 1 行归 P1 重建。
- **30 行不能直接覆盖**：p3-only 8、selfhost 6、source-origin/source-job/adopted 5、candidate-profile 3（其中 1 行也需 p3）、streams 变体 4、assembler 1、e2reader 1、libneed 1、baseline 1。
- 现有 11 个 `tools/*-material.cx` 只覆盖约 10 个材料名；`streams` 再生成受归档 C `exec/c/run.c` 阻塞，`assembler` 受 `src/asmtext.c` 归档阻塞，`kernel-route` 连锁依赖 `asm-material`；`candidate-profile` 当前候选没有 profile。
- 建议顺序：P0 先绑现有材料 + 7 个 stream SHA（覆盖 133）；P1 用现有生成器补 RI/identity/libneed（约 28）；P2 处理 p3/selfhost/source-origin/candidate-profile（约 25）；P3 个别确认 streams 变体/e2reader/baseline。

## 1. 口径与机制

- 材料声明在 `tests/batches.tsv` 第 8 列；`:` 后是 kind。
  - kind 1（默认）= canonical absolute path（文件或目录）。
  - kind 2（`:sha256`）= 严格 64 位小写 hex 字面量。
  - 解析见 `tools/cxlib/testplan.cx:18-47`。
- `all-waves init ... MATERIALS_MAP|-` 接受 `NAME<TAB>VALUE` 映射；每个材料必须再由 `all-waves material ROOT SESSION NAME` 重新测量 content/fingerprint 后才可被 wave 使用（`tools/all-waves.cx:1-27,94-116`）。
- 164 行目前 `UNPREPARED` 的直接原因是会话未提供/未绑定材料 map，不是材料名非法。
- 数字来源：`tests/batches.tsv`、`out/tb/sess-*/materials.tsv`、`out/c43-*/materials.tsv`、`tests/README.md`、`tools/*-material.cx` 头注释、`tools/streams.md`。

## 2. 材料族分组

| 族 | 批次数（重叠） | 主要材料名 | 代表入口 | 现有来源/生成器 | 置信度 |
|---|---:|---|---|---|---|
| streams | 60 | `streams`、`streams-cc`、`streams-cc2`、`streams-o1`、`stream-{e1,e2,e3,e4,elf,lower,prune}-sha` | `tests/stream-e1..e4.cx`、`stream-lower*.cx`、`stream-prune.cx`、`stream-elf.cx`、`tools/opt-corpus-gen.cx`、`properties-cases-gen.cx` | `out/tb/mat-streams`（含 7 个 `unisacc-stream*.com`）；生成链 `tools/stream-pack.cx` + stream runtime（`tools/streams.md`），但再生成受归档 `exec/c/run.c` 阻塞 | 高 |
| ref | 23 | `ref` | `tools/opt-corpus-gen.cx`、`o2-cases-gen.cx`、`properties-cases-gen.cx` | `unisacc-ref-500d6cf3d8/out/ua_ref`（冻结导出 `make ref`） | 高 |
| ri-flow | 15 | `ri-flow-runtime`、`ri-flow-data` | `tests/ri-flow.cx`、`ri-flow-controls.cx`、`ri-flow-data-controls.cx` | `out/c43-ri-flow-material-02/03`、`out/c43-ri-flow-data-01`；`tools/ri-material.cx` + `tools/ri-flow-material.cx` | 高 |
| p3 | 9 | `p3-pack-ctx`、`p3-source-ctx`、`p3-gen-c`、`p3-gen-b`、`p3-tool-elf`、`p3-member-elf` | `tests/p3-library-members.cx`、`p3-member-consume.cx`、`p3-nn-asm-provenance.cx`、`tools/check-cx-library-cache.cx`、`check-p3-extract.cx` | `unisacc.com.dev/p3-package.bin`、`out/c43-source-package-context-*`、host gcc ELF；没有专用 `*-material.cx` | 中 |
| objects | 9 | `objects` | `tests/objects.cx` | `out/tb/mat-objects`；`tools/object-material.cx` | 高 |
| kernel-route | 9 | `kernel-route` | `tests/kernel-route.cx` | `out/tb/mat-kroute`；`tools/kernel-route.cx`；再生成依赖 asm-material（归档 C） | 高 |
| ri-cli | 8 | `ri-cli` | `tests/ri-standalone.cx`、`ri-material-controls.cx` | `out/c43-ri-cli-material-01..03`；`tools/ri-material.cx --build` | 高 |
| ri-chain | 8 | `ri-chain` | `tests/ri-standalone.cx`、`ri-chain-controls.cx` | `out/tb/mat-richain`；`tools/ri-chain-material.cx` | 高 |
| host | 9 | `hostcc`、`hostld`、`hostar` | `tools/opt-corpus-gen.cx`、`pp-differential.cx`、`c-negative.cx`、`formats-*` | 系统工具：`/usr/bin/x86_64-linux-gnu-{gcc-11,ld.bfd,ar}` | 高 |
| stream-*-sha | 18 | 7 个字面量 | 同 streams | 由 `out/tb/mat-streams/unisacc-stream{1..7}.com` 计算，见 §3 | 高 |
| selfhost | 6 | `prior-generation`、`prior-package`、`fresh-generation`、`fresh-package`、`prior/fresh-log`、`run-id` | `tests/selfhost.cx`、`tools/selfhost-mixed.cx` | `out/c43-selfhost-mixed-*`、`out/c43-native-package-12`；四个 ≤55s endpoint + compare，重型 | 中 |
| identity | 8 | `candidate-profile`、`candidate-profile-sha`、`current-source-id`、`recorded-source-id`、`source-origin*`、`source-job-context`、`adopted-package`、`logical-source-package` | `tests/candidate-identity/admission`、`origin-package.cx`、`adopted-package.cx`、`source-job-union.cx` | `out/c43-source-package-context-01`、`out/c43-source-package-forward-context-01`、`out/c43-adopted-package-01`；`tools/source-package-*`、`source-origin-verify.cx`；`candidate-profile` 当前缺失 | 中 |
| small | 13 | `pp-gold`、`nets`、`kernels`、`e2model`、`e2reader`、`identity-helper`、`libneed-native/read`、`assembler`、`baseline`、`hosts-domain-matrix` | 各自单批 | 见 §4 | 中 |

## 3. 7 个 stream SHA 字面量（可直接补）

`tools/streams.md` 明确七个产物顺序为 **e2, e1, e3, e4, prune, lower, elf**，对应 `out/tb/mat-streams/unisacc-stream1..7.com`。实测 SHA256：

| 材料名 | 文件 | SHA256 |
|---|---|---|
| `stream-e2-sha:sha256` | `unisacc-stream1.com` | `634abb8aaff0af8f61d93cc8959050aa311dd7eeac9a40291e154b19c45aab8f` |
| `stream-e1-sha:sha256` | `unisacc-stream2.com` | `19ea3b0654d7ec25a97134d5a424161ebed2e2315674d9f32cb77824cc5ec5ff` |
| `stream-e3-sha:sha256` | `unisacc-stream3.com` | `2ae556b1aac656ddd0b3d096b1a6c4213a7d923ea43ef4cb779e2870a3e08311` |
| `stream-e4-sha:sha256` | `unisacc-stream4.com` | `67de5d0478782c68f23278ecd289122f9ef134350c1d1f4ae7f2ed7353ea56b9` |
| `stream-prune-sha:sha256` | `unisacc-stream5.com` | `1110daef11fc53924acaedcbdcfa2cc33dd68b0782db9626d7174060916c4d21` |
| `stream-lower-sha:sha256` | `unisacc-stream6.com` | `4cde2b4e465251b0c1da2124a630c3062bc3c76a3545e597d5b2c1d8b875de42` |
| `stream-elf-sha:sha256` | `unisacc-stream7.com` | `8d9ae2443f4e5239791c3ff05e2fb1c235130fa21ed6f74cf764f95c5da675c6` |

> 这 18 行是 P0 最低成本项：只要把 7 个字面量写进 materials map，`all-waves material` 只做字面量校验，不构造任何东西。前提是 `out/tb/mat-streams` 仍是测试期望的 pin；若 pin 已换，以测试源里的期望为准，不能用旧哈希冒充。

## 4. 现有生成器覆盖与成本

| 生成器 | 覆盖材料 | 构造成本 | 状态/证据 |
|---|---|---|---|
| `tools/object-material.cx` | `objects` | 3 个产品对象编译，显式子预算合计 40s，外层 55s | 可用；`out/tb/mat-objects` 已备 |
| `tools/net-material.cx` | `nets` | 读取当前 kernel/gold，生成私有 CX 模块；单命令 <55s | 可用；`out/tb/mat-nets` 已备 |
| `tools/pp-gold-material.cx` | `pp-gold` | prepare 55s / verify 20s；不跑 Python | 可用；`out/tb/mat-ppgold` 已备 |
| `tools/ri-chain-material.cx` | `ri-chain` | prepare/verify 12s；静态 NN，无编译 | 可用；`out/tb/mat-richain` 已备 |
| `tools/ri-material.cx --build` | `ri-cli`、`ri-capture`、`ri-consumers` | 每条 ≤55s（原生编译内层 20s） | 可用；`out/c43-ri-{cli,capture,consumers}-material-*` 已备 |
| `tools/ri-flow-material.cx` | `ri-flow-runtime`、`ri-flow-data` | compile 55s + seal 55s + `flow-native prepare` 20s（3 条独立命令） | 可用；`out/c43-ri-flow-material-02/03`、`out/c43-ri-flow-data-01` 已备 |
| `tools/identity-material.cx` | `identity-helper` | 单命令 ≤55s；依赖 `seed/ident.c` 与 saved bootstrap | 可用；`out/tb/mat-identity` 为空，需重建 |
| `tools/libneed-material.cx` | `libneed-native`、`libneed-reader` | 单命令 ≤55s；依赖当前 include 树 + saved bootstrap | 可用；未在 164 的主流 map 中出现 |
| `tools/asm-material.cx` | `assembler` | 单命令，但读取归档 `src/asmtext.c` | **阻塞**：归档 C，当前候选不能直接生成 |
| `tools/ledger-material.cx` | `ledger`/snapshot | 单命令，但读取归档 `exec/c/resourceledger.h` | **阻塞**：归档 C；不在 164 主列表 |
| `tools/gold-partition-material.cx` | 经典 net 表（`nets`/`kernels` 候选） | 构造 1–3 字段经典 net，单命令 | 需确认落到哪个材料名；不作为已覆盖 |
| `tools/kernel-route.cx` | `kernel-route` | 11 单元 × (-E/as/link)，依赖 asm-material | 已有 `out/tb/mat-kroute`；**再生成阻塞** |
| `tools/stream-pack.cx` + stream runtime | `streams` | 10 条编译/链接 + pack + make，每条 ≤55s | 已有 `out/tb/mat-streams`；**再生成阻塞**（`exec/c/run.c` 只在 archive） |
| 冻结导出 `make ref` | `ref` | 外部参考构建，不在当前仓 | 已有 `unisacc-ref-500d6cf3d8/out/ua_ref` |
| 系统工具 | `hostcc`/`hostld`/`hostar` | 无构造，直接路径 | `/usr/bin/x86_64-linux-gnu-*` |

## 5. 30 个不能直接覆盖的批次

按主阻塞分组；边界行只计一次。

### 5.1 p3 材料相关（9 行；其中 1 行同时需要 candidate-profile，p3-only 8 行）

`p3-nn-asm-provenance`、`p3-member-consume`、`p3-library-members`、`p3-library-members-cxcodec`、`p3-library-members-cxruntime`、`p3-library-api`、`candidate-admission-lnx-x86_64`、`cx-library-cache`、`cx-p3-extract`。

- 需要 `p3-pack-ctx`（含 `unisacc.com.dev/p3-package.bin` 的 PC 目录）、`p3-source-ctx`（`out/c43-source-package-context-01` 或 `forward-context-01`）、`p3-gen-c`/`p3-gen-b`（生成 C/B 产物）、`p3-tool-elf`/`p3-member-elf`（host gcc O2 ELF）。
- 没有专用 `tools/*-material.cx`；目前靠 `unisacc.com.dev` + `tools/source-package-*` + host gcc 手工链。
- `candidate-admission` 还叠加 `candidate-profile` 缺失。

### 5.2 selfhost（6）

`selfhost-pair`、`selfhost-mixed-prior`、`selfhost-mixed-fresh`、`selfhost-mixed-after-prior`、`selfhost-mixed-after-fresh`、`selfhost-mixed-compare`。

- 需要 `prior-generation`、`prior-package`、`fresh-generation`、`fresh-package`、`run-id`、4 个 log。
- 生产链：`tools/selfhost-mixed.cx` 四个 endpoint（每个 ≤55s）+ `compare`；重型、多命令、跨代。
- 现有 `out/c43-selfhost-mixed-*`、`out/c43-native-package-12` 可作候选，但必须重新核当前候选身份。

### 5.3 source-origin / source-job / adopted（5）

`origin-package`、`nn-inputs`、`adopted-package`、`source-job-union`、`source-recorded-jobs`。

- `origin-package` 需要 `source-origin`、`source-origin-verifier`、`source-origin-profile`、`logical-source-package`；37 项 origin gate。
- `nn-inputs` 需要 `source-origin`、`libneed-reader`。
- `adopted-package` 需要 sealed PC `out/c43-adopted-package-01` + `current-source-id:sha256` 字面量（示例 `0d5923...`）。
- `source-job-union` 需要 `source-job-context`（`out/c43-source-package-forward-context-01`）+ `current-source-id:sha256`（示例 `03a5e1...`）。
- `source-recorded-jobs` 需要 `source-job-context` + `recorded-source-id:sha256`。
- 这些是构造链/身份材料，不是简单 hash。

### 5.4 candidate-profile（3）

`hosts-same-candidate-lnx-x86_64`、`candidate-identity-lnx-x86_64`、`candidate-admission-lnx-x86_64`。

- 需要 `candidate-profile`（目录/文件）+ `candidate-profile-sha:sha256`。
- `plans/release-0.0.43.md:213` 明确：当前候选 `21782dd5` 没有 profile，SHA 模式 13/0 不进 all-waves；不造 profile。
- 这是当前最硬的身份材料缺口之一。

### 5.5 streams 变体（4）

`stream-e4-io-cc`、`stream-prune-io-cc`（`streams-cc`）、`stream-e4-o1`（`streams-o1`）、`stream-prune-library`（`streams-cc2`）。

- 候选目录：`out/c43-streams-cc-02`、`out/c43-streams-cc-o1-01`、`out/c43-streams-cc-01`/`out/c43-streams-ccd29-01`。
- `streams-cc2` 的精确目录未核实；先按 `out/c43-streams-cc-01` 或 `ccd29` 试，记录指纹。

### 5.6 单个（4；其中 candidate-admission 已在 5.1/5.4 计）

- `assembler`：`tools/asm-material.cx` 依赖归档 `src/asmtext.c`；当前候选不能直接生成。
- `cx0-e2`：需要 `e2reader`（候选 `out/c43-cx-pipeline-source-default-01/e2-reader`）和 `e2model`（`unisacc.com`）。
- `package-controls`：需要 `baseline`（候选 `out/c43-native-package-12` 或 `out/e3-current-f/baseline-root`，未核实）和 `kernels`（`out/c43-native-kernel-13`，已备）。
- `libneed-attributes`：需要 `libneed-native`；`tools/libneed-material.cx` 可生成。

## 6. 优先级建议

### P0：现有材料 + 7 个 SHA（目标覆盖 133 行；identity-helper 1 行归 P1）

1. 从已有会话合并材料路径（`out/tb/sess-f2/materials.tsv`、`sess-qual`、`out/c43-cc-allsession-01/materials.tsv`）：
   - `streams` → `out/tb/mat-streams`
   - `ref` → `unisacc-ref-500d6cf3d8/out/ua_ref`
   - `objects` → `out/tb/mat-objects`
   - `nets` → `out/tb/mat-nets`
   - `pp-gold` → `out/tb/mat-ppgold`
   - `ri-chain` → `out/tb/mat-richain`
   - `kernel-route` → `out/tb/mat-kroute`
   - `kernels` → `out/c43-native-kernel-13`
   - `e2model` → `/home/devali/repos/unisacc/unisacc.com`
   - `hostcc` → `/usr/bin/x86_64-linux-gnu-gcc-11`
   - `hostld` → `/usr/bin/x86_64-linux-gnu-ld.bfd`
   - `hostar` → `/usr/bin/x86_64-linux-gnu-ar`
2. 加 7 个 stream SHA 字面量（§3）。
3. 加现有 RI 路径：
   - `ri-cli` → `out/c43-ri-cli-material-01`（或 02/03，先核 status）
   - `ri-flow-runtime` → `out/c43-ri-flow-material-02`
   - `ri-flow-data` → `out/c43-ri-flow-data-01`
   - `ri-capture` → `out/c43-ri-fetch-material-05`（含 `runtime-native`；01 缺 native）
   - `ri-consumers` → `out/c43-ri-consumers-material-05`（含 `runtime-native`；01 缺 native）
4. 对每个材料跑 `all-waves material` 重新测 content/fingerprint；只读 hash，不构造。
5. `all-waves pending` 复核剩余 UNPREPARED；预期从 164 降到约 30。

> 风险：预建材料可能是旧候选；`material` 只证当前内容一致，不证与当前候选的语义一致。测试仍可能 FAIL，但至少不再是“材料清单不存在”。

### P1：现有生成器补 RI/identity/libneed（约 28 行）

- `ri-cli`（8）、`ri-capture`/`ri-consumers`（2）：`tools/ri-material.cx --build ROOT NEW_ABS_MATERIAL tests/cases/<fixture>.cx`，每条 ≤55s。
- `ri-flow-runtime`/`ri-flow-data`（15）：`tools/ri-material.cx` → `tools/ri-flow-material.cx compile` → `seal` → `flow-native prepare`，3 条独立命令。
- `identity-helper`（1）：`tools/identity-material.cx`，≤55s；重建 `out/tb/mat-identity`。
- `libneed-native`/`libneed-reader`（2）：`tools/libneed-material.cx`，≤55s。
- 这些生成器都已存在，不需要新代码；按材料逐个跑，失败就保留最小复现，不吞。

### P2：重型/缺失链（约 25 行）

- p3（9）：先确认 `p3-pack-ctx`/`p3-source-ctx`/`p3-gen-*`/`p3-*-elf` 的权威生成命令；没有专用 `*-material.cx`，不要临时造不可审计的链。可先做 `p3-library-members`（只需 `p3-pack-ctx`）单点，验证 `unisacc.com.dev/p3-package.bin` 能否直接作为 PC。
- selfhost（6）：按 `tools/selfhost-mixed.cx` 四个 endpoint + compare；每条独立 55s，跨代身份必须重核。
- source-origin/source-job/adopted（5）：先做 `adopted-package`（sealed PC 已在 `out/c43-adopted-package-01`，只需 `current-source-id` 字面量），再做 `source-job-union`/`source-recorded-jobs`，最后 `origin-package`/`nn-inputs`。
- candidate-profile（3）：当前候选没有 profile，需先决定是否生成 profile；在决定前，这 3 行保持 UNPREPARED，不要用旧 profile 冒充。
- kernels/baseline（1）：`kernels` 已备；`baseline` 需确认候选目录。

### P3：个别确认（约 5 行）

- `streams-cc`/`streams-o1`/`streams-cc2`：先确认目录与指纹；若 `out/c43-streams-cc-02`/`cc-o1-01` 可直接绑定，归入 P0。
- `assembler`：归档 C 阻塞；要么接受预建 `out/.../assembler`（若有），要么记录为本版不可生成。
- `cx0-e2`：确认 `e2reader` ELF 目录；`e2model` 已是 `unisacc.com`。
- `package-controls`：确认 `baseline` 目录。

## 7. 未核实与风险

1. **预建材料与当前候选的语义一致性**：`material` 只重测内容/fingerprint，不证明材料与当前候选同源；P0 后仍可能 FAIL。
2. **p3 生成命令**：仓库没有 `p3-*-material.cx`；`p3-pack-ctx`/`p3-gen-*` 的权威 CLI 未在本文确认。
3. **streams-cc2 / baseline / e2reader** 的精确目录未核实，先按候选目录试，记录指纹。
4. **stream SHA pin**：7 个哈希来自 `out/tb/mat-streams`；若测试源期望不同 pin，以测试源为准。
5. **133/134 的口径**：134 是按目录存在性计；`identity-helper` 目录为空，实际可复用 133。包含 `ri-cli`/`ri-flow` 等预建目录；若其中某个目录缺 `runtime-native` 或 status 不是 PREPARED，应降级到 P1 再生成。
6. **不要**为了让 `pending` 变绿而绑定未核实的路径；all-waves 的 fingerprint 会拒绝，且会把错误材料带进证据链。
