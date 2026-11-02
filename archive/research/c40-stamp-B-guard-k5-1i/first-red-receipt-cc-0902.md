# 首红后零启动夹具实跑回执（cc，0902；(A) 续）

- 裁定：机房主任 0902 代裁（实写夹具 + scratch 双 case）；见 `/tmp/unisacc-cdx2/observe-10m/ruling-k5-1i-first-red-fixture-run-0902.md`。
- 固定 tip：`7b07b72343907d06f797340ae26585f14174a377`；`exec/c/chain.sh` blob `82956486…`（未改）。
- 夹具：`tests/chainfirstredcheck.sh`（本次新增，未挂入 gate，未改 gatedeps）。注入：shim 拦截 E3 argv 的 `sh exec/parse2gen/gen-delta.sh`（chain.sh:55），返回 rc=3，不运行 helper，不写 e3.json。
- 正式实跑：r3，`FIRSTRED_EVIDENCE=/tmp/cc40-prep/first-red/r3`，exit 0，全部断言 ok。副本与校验：`first-red-run-0902/`（`sha256sum -c SHA256SUMS` 通过）。

## 结果

| 项 | POS | NEG |
|---|---|---|
| chain rc | 0 | 1 |
| stdout | `chain net files 1 equal 1` | `chain: E3 gen failed` |
| 首个非零子 rc | 无 | `helper-parse2-rc=3`（E3，第 17 行） |
| e3.json | 产出 | 不存在 |
| tbl / net / check-net / probe-run / ua-ref / gen | 3 / 3 / 3 / 3 / 1 / — | 0 / 0 / 0 / 0 / 0 / 0 |
| 首红后全部 shim 事件 | — | 0 |

## 夹具历史（透明记录）

- r1：旧夹具 NETWORK=0，POS 通过（`/tmp/cc40-prep/first-red/r1/`）。
- r2：NEG 全过；POS 判 FAIL，原因是正则写成 `chain tbl`，实际输出 `chain net`，属夹具误判（`/tmp/cc40-prep/first-red/r2/`）。
- r3：修正正则 + NETWORK=1 后全绿，即本回执的正式记录。

## 边界（不因本回执扩大）

- 预期负例 ≠ 验收 PASS：NEG 只证明命令层面的后续启动为零。
- 未证明：PGID/SID 清空、TERM、UA 本体行为（仅包装计数）、exec-chain 门预算、净提速、真实 exec-chain 门绿。
- 未做：bump、version.h、Draft、freeze、改验收措辞、删测；未跑 gate；未改 gatedeps、prd。
- 原件仓外：`/tmp/cc40-prep/first-red/r3/`；仓内为副本。
