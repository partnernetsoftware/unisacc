# S1 RF 收据（SC1 身份落地）：RF-ABSENT、RF-EMPTY、RF-LOWER、RF-STAGE

状态：**仓外开发闭环全绿；未经 CI/门闸、未落主。** 等 cc 开窗后做 .com 冷构建与门闸验收。

## 一、产品修复（cc 裁 A）

问题：严格模式下异源账本被静默复用，账本未与阶段/输入身份绑定（违背 SC1 的 stage_id + 输入身份字段）。

修法（文件域 `exec/c/resourceledger.h`、`exec/c/run.c`、`exec/c/compiler.c`，回放器 `tests/middleflowrunner.c`）：

- **账本头**：`resources.tsv` 第一行为 `#SC1<TAB>stage=ID<TAB>input_sha256=HEX`（ID 不含 TAB；HEX 为输入字节的 SHA-256，复用 `include/unisa_sha256.h`）。头只允许在首行出现一次，格式错即 `INTERFACE_ERROR resource-identity-malformed`。
- **严格模式**（`--strict-resource-flow`）：在读入输入之后、任何阶段运行之前，`rf_identity_gate` 检查：无头 → `INTERFACE_ERROR resource-identity-absent`；阶段不符 → `…resource-identity-mismatch stage …`；输入哈希不符 → `…resource-identity-mismatch input …`。均 rc 2、无主输出。
- **捕获**（`UNISACC_RF_CAPTURE`）：先写头（阶段标签与本次输入的 SHA-256），再逐行追加。
- **默认路径**：不带开关时 `rf_identity_gate` 不执行，行为不变（`RF_STRICT || rf_capture_dir()` 才进入）。
- **回放器**：`middleflowrunner` 的 capture 写头（`e3` 与 `lower` 两个阶段标签）；replay 读 `input.bytes` 比对头中的输入哈希。
- **阶段标签**：产品路径固定为 `e3:lnx/x86_64/tape/O0`（`RF_STAGE_ID`）。

## 二、夹具重采（仓内配方）

格式变化：五份账本前置头行。配方：头 = `#SC1 / stage=<阶段> / input_sha256=sha256(input.bytes)`，其余行逐字节不动。

| 夹具 | 阶段标签 | input.bytes sha256 前缀 |
|---|---|---|
| middleflow-step0/e3/accept | e3 | 503bf2961eaf5277 |
| middleflow-step0/e3/refuse | e3 | cdad9050cc7a42b7 |
| middleflow-step0/lower/accept | lower | 4230637b59dc3251 |
| stagecontract-e1-e3/accept | e3 | 503bf2961eaf5277 |
| stagecontract-tape-lower/accept | lower | 4230637b59dc3251 |

`exec-pp-literals` 的 resources.tsv 是另一种格式（pp-leaf 封存），不在本次范围，未改。

## 三、检查器修正（旧 RF 的假 PASS 根因）

根因：RF-STAGE 与 RF-LENGTH 的命令在 `cd src` 下用**相对路径** `-o stage.tape`，产物落在 `src/`，而检查器读 `tmp/stage.tape`（不存在）。RF-STAGE 的断言 `sg != 0 || !same(...)` 因此空过，RF-LENGTH 的“无输出”也因此空过。旧 RF-EMPTY 只重放了默认账本，并未真写 0 字节行。新版全部改为绝对路径，EMPTY 行真实改写。

## 四、断言（新版 `tests/resourceflowcheck.cx`）

- **RF-ABSENT**（cli/defines，标注 this key does not distinguish）：ABSENT 行 → rc 0、无 MISSING、输出与默认逐字节相同。
- **RF-EMPTY**（cli/defines，同上）：0 字节 PRESENT 行 → rc 0、无 MISSING/INTERFACE_ERROR、输出与默认相同。
- **RF-LOWER**（library/module，lower 路线，夹具 stagecontract-tape-lower 与冻结 lower.net）：ABSENT → rc 0，输出等于夹具 `accept/output.bytes`；EMPTY → rc 1，stderr 具名 `not covered`，stdout 空；二者结果不同。
- **RF-STAGE**：异源账本（return 7）→ rc 2、stderr 具名 `INTERFACE_ERROR`、无主输出。字节相等不算通过。

## 五、开发闭环实测（仓外，原生驱动，同一包）

驱动 = `exec/c/compiler.c` 以 `cc -O2 -w` 直接编译（`drv-after` 为本树，`drv-before` 为 fbcefbda），统一加 `--models S8/cand-s2/compiler.pkg`。包为 a3b44feb+S2 的网络，与主线网络一致性待 .com 验收确认。

| 检查 | 结果 |
|---|---|
| `resourceflowcheck`（新驱动，基线为旧驱动） | rc 0，12 行 PASS（LEGACY、CAPTURE、POS、ENV、MISSING、DUP、LENGTH、RESET、ABSENT、EMPTY、LOWER、STAGE） |
| `stagecontractcheck`（e1-e3 与 tape-lower，夹具已重采） | rc 0，两个边界 S-POS-01 与负例通过；ABSENT/EMPTY 仍为 UNKNOWN（记录，不算通过） |
| `middleflowcheck --ledger`（实时捕获与封存账本逐字节） | rc 0，`live resources.tsv equals the sealed ledger`（头格式与捕获一致） |
| `latebindp6check` | rc 0，26 源 130 件逐字节同快照 |
| 五形默认路径（新驱动对旧驱动） | 五形 rc、字节、sha256 全同（唯一差异是 harness 回显的输出目录名） |

## 六、窗内验收（候选 .com 冷构建，2026-10-16）

- 候选：`build-cand`（本树 fbcefbda + 本笔），`unisacc-next.com` 2467738 B；基线：`build-base`（fbcefbda 干净树），2460298 B；两者 `build_candidate.sh` 七步+pack 全 rc 0。
- 五形默认路径（`run5.sh`，绝对路径）：候选与基线 rc、字节、sha256 全同（quote 738、angle 676、inc 676、forced 694；missing 两者 rc 1）。
- 门闸（候选 MODEL_COM）：`resourceflow` rc0（RF 全行 PASS）；`stage-contract-e1e3` rc0；`middle-flow-ledger` rc0；`latebind-p6` rc0（26 源 130 件）。
- `reviewed-trees` 戳按工作树字节重算（S5/stamp.py，exec 根）；门闸 rc0。
- `pp-leaf-seal`：封存重算（`tests/reseal-pp-literals.sh`，四步全 rc0）；只 `dependencies.tsv` 两行变化（resourceledger.h、run.c 的字节与长度），其余 case/model/resource/identity/closure 逐字节同；门闸 rc0。
- `derivedcheck`（暂存提交）：ok。abi-manifest、inventory-derived、pp-leaf-seal、pp-leaf-seal-negative、reviewed-trees、seedmemory 全 rc0。

## 七、局限

- 回放器的 replay 现比对阶段标签与输入哈希（`MF_STAGE_ID` 必填，缺省即拒，rc2，具名）；`stagecontractcheck` 的 `boundary.tsv` 带 `stage` 行，`middleflowcheck --sealed-*` 与 RF-LOWER 显式传入。
- `RF_STAGE_ID` 仍是本路线（C→Tape，-S -o）的固定标签，不是逐阶段模型标识符；阶段内部 cache 结构未测。
- 候选树是 fbcefbda 加本笔；主线随后已前进到 22e46738（纯文档 #338，与本笔路径无交）。
- 无 CI 证据；本收据不是发布证据。
