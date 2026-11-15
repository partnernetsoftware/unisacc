# 代裁：开 GitHub Draft v0.0.40（机房主任 1626）

墙钟：2026-10-11 16:26 CST。授源：政委「自主一点，如果我有想法自然会提」+ 跳过 Draft 边界问；机房主任代裁执行。

## 裁定
1. **按 m4pro flex**：本机绿证据齐 → **开** `gh release create v0.0.40 --draft`。
2. **禁切 Latest / 禁公开发布 / 禁** `publish.sh` / 禁 `--draft=false`。
3. **seal/资产**：本刀挂 **本机绿证据包**（1532 flex B=C=U）与裁定/paste 材料；**产品** `seal_candidate` / 签前签后 `unisacc.com` / dmg **未齐**（另列缺口）。
4. **tip 采证 CI 旁观不挡 Draft**：主 CI `38119555397` @ ed50f00c、`38124099823` @ 8ef1dbd6 仍旁观，不因未结回滚本机绿、不挡本 Draft。

## 本机绿（已齐）
| 项 | 值 |
|---|---|
| 宿主 | m4pro.local · Darwin arm64 |
| 取证 tip | `7acb254a217f614b73c811d4b9575160cfa86dd0` |
| 判定 | ok B=C=U `257c4ef6944103ef` / 421304 lines / rc0 |
| 含修 | ed50f00c / c2b140bc |
| 证据 | `research/c40-darwin-b139/m4pro-bootstrap-flex-1532.md` + HASHES-1532 |

## Draft 身份
| 项 | 值 |
|---|---|
| tag | `v0.0.40`（Draft） |
| target tip（开 Draft 时） | `d70a80388f4acf08ad231c62a0b91175215206b1` |
| version.h | 0.0.40（冻后已落） |
| Latest | **仍须保持 v0.0.39**（本刀不碰） |
| candidate.json | 仍 0.0.39（本刀未 reseal） |

## 硬禁
禁 Latest；禁公开发布；禁把旁观 CI 未结写成产品 FAIL/PASS；禁以 U 代 B；禁放宽验收数值；禁把 0.0.39 候选字节冒充 0.0.40 seal。

## 缺口（不挡本 Draft 开窗；挡产品资产挂齐 / 公开）
1. 无 0.0.40 `unisacc-next.com` 候选（B.2/B.3 未实跑；m4pro 本窗 offline）
2. 未 `seal_candidate.sh` → `release/candidate.json` 仍钉 0.0.39
3. 无 `rc/v0.0.40`
4. 无 unsigned/signed 产品资产、无 dmg
5. tip 相对绿证 tip 已前进（政策+guard：AGENTS.md / archive/plans/v0.0.40.md / RELEASE-PIPELINE.md / gatedeps.json；无 src/exec/tests）；未在当前 tip 重跑 m4pro bootstrap
