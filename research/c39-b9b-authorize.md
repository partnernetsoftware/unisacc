# 机房主任裁定 B.9b 公开（2026-10-10 18:15 SGT / CST）

> 仓内落盘副本（可核）。源件仓外：`/tmp/unisacc-cdx2/workflow-efficiency/b9b-authorize-1815.md`
> 源件 sha256：**34166188aaf4ae29da46bcda413edf1a03a9402a1df6856293d2db3db8c49c5a**
> 公开执行与回执指针见 `research/r39-release-acceptance.json`（`courts.owner-promotion.auth_doc` → 本文件）。

## 授权链
| 时刻 (CST/SGT) | 角色 | 事实 |
|---|---|---|
| 18:05 | **政委** | 「不是授权过你自己安排了吗」→ **自决公开**：公开由机房主任排，不再问政委 |
| 18:15 | **机房主任** | 授 **B.9b：YES** — 授权跑 `publish.sh` + owner-promotion + Draft v0.0.39 → 公开（变 Latest） |
| 18:15–18:16 | **执行人** | 机房主任 grk（cc Stop-hook 队列未即时开干 → 主任直接执行） |
| 18:16:17 | gh | Draft→Latest；`published_at=2026-10-10T10:16:17Z`（CST 18:16:17） |

## 依据（授权当时）
- B.7 签名齐：签后 sha **6b2b96803b23fc19c578bb511cddd9434b0c64d7743adc99fa2f6e174eb6c57a** / asset **627617308**
- B.8 六格/Defender 齐（smoke 38043652719 / Defender 38043654697）
- B.9a 验收回执已落、`release_eligible=true`（tip≈4cc2e800；精度补全祖先 d87222ac）
- Draft **408747179**

## 裁定原文要点
**授 B.9b：YES** — 授权跑：
```
./release/tools/publish.sh v0.0.39 6b2b96803b23fc19c578bb511cddd9434b0c64d7743adc99fa2f6e174eb6c57a research/r39-release-acceptance.json
```
公开成功后勿立刻开干 0.0.40 实现；先确认 v0.0.40-prep 门闩。

## 执行结果（gh facts 摘要）
| 项 | 值 |
|---|---|
| `publish.sh` rc | **0** |
| Latest / tag | **v0.0.39**（isDraft=false） |
| URL | https://github.com/partnernetsoftware/unisacc/releases/tag/v0.0.39 |
| publishedAt | **2026-10-10T10:16:17Z**（CST 18:16:17） |
| public `unisacc.com` sha256 | **6b2b96803b23fc19c578bb511cddd9434b0c64d7743adc99fa2f6e174eb6c57a**（=签后） |
| asset id | **627617308** |
| 公开回执提交 | 4c3f25db（其后 40e6e25a 误覆盖 → 436ec713 Revert；`published.*` 保持） |

## 引语落仓（可核）
- 政委 18:05：「不是授权过你自己安排了吗」→ 公开由机房主任排，不再问政委
- 回执归因短句：「政委 18:05 自决公开」（见 `publish_authority` / `owner-promotion.authority`；本刀不改措辞）

## 窗分工（裁定当时）
- **cc**：执行 owner-promotion 回执补齐 + publish.sh + 核 Latest/公开 sha
- **cdx / cdx2**：只读盯 Draft→public、Latest、公开 sha；勿代跑 publish

## 边界
- 本文件只入库授权文档与执行摘要；**不**改 tag、**不**重跑 publish、**不**改回执 `published.*` 内容字段
- 0.0.40 实现另等 prep 门闩；A7 examples 九删脏树勿碰
