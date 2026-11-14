# Draft 材料进度（1532；本机绿后；未开 Draft）

## 本机终表（已入）
- m4pro @ tip `7acb254a`：`ok B=C=U 257c4ef6944103ef`／421304 lines／rc0
- 含修：`ed50f00c`／`c2b140bc`

## 链步进度（对照 1140）
| 步 | 状态 |
|---|---|
| B.1 version + freezecheck | 已落（历史 1124） |
| B.4 冻后 gatedeps | 已落（历史） |
| Darwin bootstrap 本机绿 | **本刀闭合**（1532） |
| B.2 同源 UA → cand → ident | 材料可推进；须净 WT @ 当前 tip；**未在本刀实跑** |
| B.3 comboot stage1→2→3→fixedpoint | **待 B.2 绿后**（本刀未开） |
| seal / rc / 签 | **未授实跑** |
| `gh release create --draft` | **未开**；开前须机房主任再确认公开边界 |
| Latest | **禁**；须另授 |

## 卡点
1. **公开边界再确认**（开 Draft 前硬卡；本刀停于此请示点）
2. B.2/B.3 须净 worktree、重活一机一件；勿与旁观 CI 混判
3. tip 已前进至 `7acb254a`（相对 1140 的旧 tip）；执行前重核 HEAD=origin/main=ls-remote
4. Latest／公开发布未授

## 明确未做
未 `gh release create`；未 seal_candidate 实跑；未 rc_tag；未 Latest；未 bump；未改验收数值。
