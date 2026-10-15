# S 第 2 步设计：固定逻辑源名协议（只是设计，未实测，未改产品）

依据：`research/c43-plan-proposal.md` 第 2 步（固定逻辑源名协议）、`plans/v0.0.43.md` 决定节“逻辑源名（cc 自裁）”、S1 资源流（`59150d12`）。本文只说明要改什么、不改什么、如何验收；不含产品改动，不声称已跑。

## 0. 先校正前提

提案写“逻辑源名作为显式字节输入”，而裁定已做了一半：`exec/c/compiler.c:728–729` 把 argv 原样写法作为 `\0cli/source` 资源交给核心，`exec/c/run.c:808` 的 `sres[2]` 同样；`__FILE__` 与诊断用的就是这一份字节。裁定把它定为“核心看到的源名 = 命令行写法”，不做 basename 与相对化。所以第 2 步**真正要改的不是源名**，而是两处仍由核心侧触碰宿主路径的地方：

1. `exec/c/run.c:653–678`（`rf_default_fetch`）：非 `\0hdr/` 键被当作**宿主路径**直接 `readfile`（`run.c:670–672`）；`\0hdr/` 键由核心自己拼 `INCDIR/…`（`run.c:671–672`），即物理路径在核心内部构造。
2. `exec/c/compiler.c:700–701`：`\0cli/includes`、`\0cli/include-dir` 以 argv 形式进入核心，核心据此解析 include 查找（`fwdpaths.h` 与 `libunisacc.c:725` 同样有 include-dir 路径）。

第 2 步要做的是：把“键 → 字节”的查找从核心挪到适配器，核心只见逻辑键与字节；适配器（宿主侧）负责把逻辑键映射到物理文件并读字节，同时输出资源映射供 S1 账本与严格模式使用。

## 1. 协议

- **逻辑源名 L**：argv 原样写法，作为显式输入字节（已是 `\0cli/source`，不变）。
- **include 逻辑键**：由核心发出，形如 `\0inc/q/<写法>`（双引号）或 `\0inc/a/<写法>`（尖括号），以**包含者的逻辑目录**为基准做相对解析：`逻辑目录(L)` 与写法相连。核心不做任何宿主路径拼接。
- **适配器**（宿主，不属核心）：收到逻辑键后按 `-I`/`quote-dir`/`include-dir` 的物理根映射到文件，读字节，返回 `(bytes, present)`。
- **资源映射输出**：适配器把 `逻辑键 → 物理路径 → 字节 sha256` 写入账本（`resourceledger.h` 的 `rl_load` 格式，`key present file len sha`）。物理路径只出现在账本的元数据列或旁路日志，**不进入核心输入字节**。
- **严格模式**（S1）：核心只查账本；缺键 → `MISSING_EXPLICIT_RESOURCE`，不回退磁盘。

## 2. 要改

| 位置 | 改动 |
|---|---|
| `exec/c/run.c:653–678` | `rf_default_fetch` 在严格/账本模式下只查 `RF_LEDGER`；默认模式行为不变（见 §4）。核心侧不再 `snprintf(path, key)`。 |
| `exec/c/compiler.c:700–701` | include 查找的物理根改由适配器持有；核心收到的是逻辑键。`\0cli/include-dir` 仍作为显式输入登记，但其含义是“适配器的物理根”，不是核心要拼的路径。 |
| `exec/c/fwdpaths.h`、`libunisacc.c:725` | 同上，仅调整 include-dir 的读取位置，不改语义。 |
| 新增适配器（仅 tests 侧与 CLI 宿主侧） | 产出账本；默认 CLI 路径仍由它读字节。 |

## 3. 不改（裁定边界）

- `\0cli/source` 的值仍是 argv 原样写法；`__FILE__`、诊断位置、诊断文件名不变。
- basename、相对化、绝对路径改写：仍不采纳（裁定）。
- 用户写的绝对路径仍原样进入输出（那是显式输入）。
- E2 的 include 诊断（“no such file for #include”）语义不变；只是缺失来源从“磁盘 absent”变为“账本 absent”，两者都是 ABSENT，不是 EMPTY。
- 未覆盖的资源仍是拒绝，不软回退（同 S1）。

## 4. 验收（动态 C 合同，拟新增，未跑）

- **默认路径逐字节不变**：`latebind-p6`、`stage-contract-e1e3`、`exec-driver-core-modes`（冷准备按既有三步）。
- **源名不变**：`tests/sourcelabelcheck.cx`（拟新增）：同一逻辑名、两个物理目录 → 输出与诊断逐字节相同；逻辑名改变 → `__FILE__` 随之变化。
- **include 映射负例**：相对 include 的逻辑键缺失 → 明确拒绝；存在 → 接受；物理目录换位不改变输出。
- **RF 全套**：`resourceflow`（S1 已登记）。
- **候选**：改 exec 闭包 → 候选失效，需新建候选；验收用新 HEAD 构建的候选。

## 5. 五问

1. **复用**：键→字节由 S1 的 `resourceledger.h`（`rl_load`/`rl_find`）承担；不新增读写机制。
2. **新机制？**：没有。适配器只是把现有 `rf_default_fetch` 的宿主读取移到宿主侧，输出的仍是 S1 账本格式。
3. **默认路径逐字节？**：要求不变，由 §4 的三个套件核验。
4. **MISSING / ABSENT / EMPTY**：保持 S1 区分；include 缺失为 ABSENT，账本缺键为 MISSING，空文件为 EMPTY。
5. **与 2c2 / SC1 的关系**：未触碰 2c2 文件。SC1 指 `research/c43-stream-interface-S-step0.md:11` 的最小阶段合同（字段定义提案），落位见 §8。

## 8. SC1 resources 字段落位（新逻辑键、S1 账本、三态语义）

SC1 的 `resources` 字段定义（step0 `:11` 表）：完整显式 `key → PRESENT(bytes, length, hash)` 或 `ABSENT`；每个请求必须匹配一个条目；无条目为 `MISSING_RESOURCE` 合同失败，**绝不能等同 ABSENT**；同 key 重复请求必须返回相同 presence 与 bytes。本步在此之上的落位：

- **键的表示**：`\0inc/q|a/<写法>` 含 NUL 前缀，SC1 不以 C 字符串标识 key，统一写长度加 hex（step0 同条款）。账本行 `key` 列即该 hex，与 S1 `rl_load` 格式一致。
- **条目来源**：适配器对核心发出的每个 include 逻辑键必须产出一条账本行：找到 → `present=1`，附 bytes 与 sha256；所有根都找不到 → `present=0`（ABSENT）。适配器**不得**对找不到的键静默不写行，否则严格回放会把它判成 MISSING_RESOURCE，把“真的不存在”误报为合同失败。
- **搜索顺序**：引号形式先查包含者的逻辑目录，再查 quote-dir / `-I`；尖括号形式只查 `-I`。搜索顺序是适配器的合同部分，记入账本元数据（列出用到的根序列与其摘要），使重放可核对同一搜索。物理路径只进元数据，不进 key。
- **请求轨迹**：step0 的“stage 请求序列/响应摘要”由核心侧记录（已请求的逻辑键序列），严格回放时逐条核对 key 命中；请求了账本未收录的键即 MISSING。
- **三态语义保持**：
  - `MISSING`：账本无此键 → 合同失败（S1 已有 `MISSING_EXPLICIT_RESOURCE`，rc 2）。
  - `ABSENT`：账本有行且 `present=0` → E2 既有 “no such file for #include” 拒绝，属正常拒绝而非合同失败。
  - `EMPTY`：`present=1` 且 `len=0` → 合法的空文件，PRESENT_EMPTY，不是 ABSENT。
- **同 key 一致**：适配器对同一逻辑键只解析一次并缓存；账本内出现重复 key 直接拒绝（S1 的 `duplicate-resource-key` 已覆盖）。
- **RI/包资源优先级**：step0 要求在适配器解析成唯一响应；严格回放只读账本，不再重查宿主。

这一节不新增资源机制：键、条目、三态都是 S1 账本与 SC1 字段的既有形式，适配器只是把原先在核心 `rf_default_fetch` 中的宿主读取，换成产出同一账本格式的步骤。

## 6. 待主人/cc 裁定

- 逻辑键前缀（`\0inc/q/`、`\0inc/a/`）与现有 `\0hdr/` 是否统一；
- 适配器的物理根是否允许在核心外配置（默认 CLI 行为不变前提下）；
- 诊断中 include 的文件名是否仍显示写法原样（裁定倾向：是）。

## 7. 债

- **RF-LEGACY 未覆盖**（S1 `59150d12`）：`tests/resourceflowcheck.cx` 的 LEGACY 行需要 base 产品参数，本次未给。下次用 base 产品补跑，验证默认路径逐字节同。
