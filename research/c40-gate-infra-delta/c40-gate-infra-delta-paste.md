# gate-infra exec 三文件真审 + 处置 (a) 裁决落仓（机房主任 02:32 消息入口）

**本刀只把仓外真审/裁决证据 paste 进 `research/`，使评审机可核。不改归因/验收、不删测、不改 gatedeps、不 bump。**

## 不把这次刷新当通过

- tip **fbb0d350** 已将 `tests/gatedeps.json` → `families.compilercheck.reviewed_trees.exec` 从 `9ae3c358…` 刷为 `2d52b9bc…`（机房主任 02:24 处置 a）。
- 该提交说明里引用的真审/裁决路径当时仅在 `/tmp/cc40-prep/next/`，**评审机上不存在**。
- **证据可被独立核验之前，不把这次刷戳当作“已通过 / gate-infra 已绿 / 精确闭包已恢复”。** 本 paste 只消除“材料不在仓”的缺口；不代核、不代裁、不宣称门绿。
- inventory 口径（含 `research/c40-k5-inventory-cut.md` 中 gate-infra「另列待核」等）**本刀未改**。

## 仓内证据（字节对拷自 `/tmp/cc40-prep/next/`；cmp 一致）

| 角色 | 仓内路径 | sha256 |
|---|---|---|
| **真审**（cdx，§5 第1–6项 PASS可进处置） | `research/c40-gate-infra-delta/gate-infra-delta-true-review-cdx.md` | `6c0be030f3059705536991ac0216380276c81df5c0ee04720881543520414934` |
| **裁决**（机房主任 02:24 处置 a） | `research/c40-gate-infra-delta/ruling-gate-infra-disposition-a-0224.md` | `858319bdf3550dec8c178e937569ab314e3116a7fb70e4033853a45ffd1b71d3` |
| 授权真审（机房主任 02:08） | `research/c40-gate-infra-delta/ruling-gate-infra-true-review-0208.md` | `dfce5204f7d067cd5990b2ad80de0a12982c96566cda5d5bde1f4aee7a195ecf` |
| delta 审材料 | `research/c40-gate-infra-delta/gate-infra-delta-review-materials.md` | `f4bde515d7393b40705c00fac3d545ab12db3833581ca2512355cc83360d1b3d` |
| 三文件 diff（相对 b8ea2faa） | `research/c40-gate-infra-delta/gate-infra-delta.diff` | `06761f9b1300ca5ba3416f8977efdd863db9ac210d5a4f5faed980dbf36a894f` |
| 刷戳回执（含写前证据更正） | `research/c40-gate-infra-delta/gate-infra-stamp-update-receipt.md` | `8ae57082ad4217223fce34930a746b0ece58675ca3ccd28eb2f03f85e55fd5f7` |
| 提交绑定头 | `research/c40-gate-infra-delta/stamp-header.txt` | `a8a1eb2e1e804bede5a7b7a4d6fda232e987e6e2e5af34de724f96903c02fad7` |
| 授权 delta 材料（02:02） | `research/c40-gate-infra-delta/ruling-gate-infra-delta-materials-0202.md` | `d22d93189c1cfebf4472bc816a610192e328b7c046dd7bb5491094941266131c` |
| 第1步只读回执 | `research/c40-gate-infra-delta/gate-infra-step1-readonly-receipt.md` | `b86a8a05b6688164503d0a6ec38307ff27b948786b10722f71e586fa7ad8b9df` |
| 授权第1步（01:52） | `research/c40-gate-infra-delta/ruling-gate-infra-step1-readonly-0152.md` | `0c1c0fba0c218ddc6b3f3724d79c5cd9151a9008cda1a077b771f1e6844ae411` |

## 路径对照（原仓外 → 仓内）

| 原路径（fbb0d350 说明所引） | 现仓内路径 |
|---|---|
| `/tmp/cc40-prep/next/gate-infra-delta-true-review-cdx.md` | `research/c40-gate-infra-delta/gate-infra-delta-true-review-cdx.md` |
| `/tmp/cc40-prep/next/ruling-gate-infra-disposition-a-0224.md` | `research/c40-gate-infra-delta/ruling-gate-infra-disposition-a-0224.md` |

原件正文里仍可能写着 `/tmp/cc40-prep/next/…`（历史落笔）；**核验以本表仓内路径与 sha256 为准**，不以仓外路径冒充验收依据。

## 刷戳绑定（已落 main，供核）

- 提交：`fbb0d350f6848b398159da3813eb4e22222faf95`（父 `1157e9af`；单文件 `tests/gatedeps.json` +1/−1）
- 旧：`9ae3c35897c7185c7b2ef2c11f6985d5281aafb424fe57b9668c2eabd3d34036`
- 新：`2d52b9bc856caf12e939c40e29a860fb5a236e71415abfb173257b02971aea27`
- 覆盖三文件（真审范围）：`exec/opt/gen-delta.sh`、`exec/pp/gen-delta.sh` 新增；`exec/pipeline/prepare.sh` 变更
- 见 `stamp-header.txt`：pre/commit diff-sha 均为 `0dfe5557535e705e…`

## 明确不宣称 / 本刀未做

- 不宣称 gate-infra 已绿、七 exec-driver 精确闭包已恢复、正式 K2b 白名单非空。
- 不改 inventory 归因/验收措辞；不删测；不改 queuecheck 期望；不 bump；不开 Draft。
- 不把仓外私有证绿目录（`/tmp/cc40-prep/gi-green/`、`gi2/`）当作本 paste 的“通过”依据一并入仓。
