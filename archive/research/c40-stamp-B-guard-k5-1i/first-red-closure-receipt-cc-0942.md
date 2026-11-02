# 首红残差命令层限定闭合回执（cc，0942；research-only）

## 闭合的内容（仅此）

- **命令层限定闭合**：`tests/chainfirstredcheck.sh` 在 tip `7b07b723` 的 scratch 中实跑（r3，exit 0）。POS：chain rc 0，E3 helper 运行并返回 0，后续 tbl/net/check-net/probe-run/ua-ref 均被日志看到。NEG：E3 注入 rc 3 后 chain rc 1，stdout `chain: E3 gen failed`，首个非零子 rc 为 E3 的 3，首红之后六类（tbl/net/check-net/probe-run/ua-ref/gen）与所有 shim 事件计数均为 0。
- 证据：原件 `/tmp/cc40-prep/first-red/r3/`，副本 `first-red-run-0902/`，两边 SHA256SUMS 总哈希 `71b3f97d…56cbf`。
- 身份：`first-red-identity-0910.md`（**事后采样**，非跑前瞬时身份）。
- 独立核验：cdx 0931 与 cdx2 0931 只读复核，结论见 `first-red-residual-review-cdx-0931.md` 与 `cdx2-firstred-0931.md`（本目录副本）。
- 材料 §6 与 §7 已对齐为实际状态。

## 限定（不升格）

- **不**证明 PGID/SID 清空、TERM 行为、UA 本体行为（仅包装计数）、exec-chain 门预算或门绿、净提速。
- **不**补造历史缺测：运行前瞬时身份与整轮 monotonic 未采，保留为缺口，不事后补写。
- r1、r2 不作为证据：r1 为旧夹具（NETWORK=0）的早期通过记录；r2 为夹具正则误判，仅存于 `/tmp/cc40-prep/first-red/r2/`。
- run1 旧原件（`/tmp/cc40-prep/k5-1i/`）已不存在，不可引用。

## 不做（本刀）

- 未 bump、未改 `version.h`、未开 Draft、未 freeze 实跑、未改验收措辞、未删测、未改 gatedeps / prd / chain.sh、未重跑整门。
- Draft 下一卡点仍是冻结授权 + 单独 bump + 净树一致 + 冻后 refresh；本回执不授权。
