# 回执：开 Draft v0.0.41（1418）

**墙钟**：2026-10-12 ≈14:19 SGT  
**授**：政委推到 42 公开；对齐 0.0.40 unsigned 先例。

## Draft

| 项 | 值 |
|---|---|
| tag | `v0.0.41`（**Draft**） |
| URL | https://github.com/partnernetsoftware/unisacc/releases/tag/untagged-65eaf3859a2967add786 |
| target | `440b5c2b` |
| version.h | 0.0.41 |
| Latest | **仍 v0.0.40**（未切） |
| cand sha | `bcd2b9b75129…` |
| 资产 | unisacc.com / .build.json / unisacc-unsigned.zip / unsigned-receipt.json / SHA256SUMS-unsigned.txt / final-bucket-vs-1146.md / receipt-envpin-closeout-1409.md |

## 结证材料

- env-pin 721/721 PASS627；对照 1146 +30；**未 void 1146**
- 分桶：`research/c41-envpin-closeout-1409/final-bucket-vs-1146.md`

## 缺口（不挡 Draft；挡正式 seal / 可挡 Latest 若政策要求）

1. `seal_candidate` / `release/candidate.json` 仍 0.0.40（cand 缺 model-audit/models.json）
2. `rc/v0.0.41` 未切
3. 签（Apple/Win）另记；unsigned 可发

## 下一步

CI release-check @ tip 绿 →（补 seal 若可）→ `gh release edit --draft=false --latest` → 开 0.0.42 计划入仓。

*1418*
