# 机房主任代裁 ③ — src 轻审盖戳 18ce7e3b（2026-10-10 ≈16:21 SGT）

> 仓内落盘副本。源件仓外：`/tmp/unisacc-cdx2/observe-10m/version-stamp-ruling-3-20261010T082147Z.md`（latest-version-stamp-ruling-3.md）。
> 盖戳台账短行见 `research/c38-open-rulings.md` / `plans/v0.0.39.md`（0586f01a）；本文件保留裁定③全文。

## 裁
**盖戳=YES**：`18ce7e3b` 记为 **已审 0.0.39 src 轻审**（版本行-only）。

## 依据
1. 同源重建定点：`/tmp/cc39-freeze2` @82290c17；cand `/tmp/cc39-b5b` comboot rc=0 于 16:09:33；pair 2b20f4b2；seed c244c2e4。
2. 净开 B.5：queue.start=16:10:29；593 作废未复活；freshness verified 2b20f4b2。
3. 裁②时序：先重建/queue，后轻审；不设版本行自动已审。
4. 材料在 main：7b6af8db/ec855d03；17 文件唯一差 version.h 0.0.38→0.0.39；APE UNKNOWN 保留。
5. cdx2 独审通过；交机房主任盖。

## 归因（仓内路径）
| 角色 | 谁 | 仓内材料 |
|---|---|---|
| 内容轻审（具名事实材料） | **cdx**（作者 cdx-unisacc；7b6af8db / ec855d03） | `research/c39-src-version-only-review.md` |
| 独立二审 / 独核 | **cdx2**（仓外续审回执入库） | `research/c39-src-version-cdx2-independent.md` |
| 盖戳裁定 | **机房主任**（代裁③） | 本文件；短行 `research/c38-open-rulings.md` |

> 勿把 `c39-src-version-only-review.md` 误标为 cdx2 独审；该文件自题与 git author 均为 cdx。cdx2 独核见上表另附。

## 边界
- 本戳 ≠ B.5 出口 / Draft 收口 / 公开。
- Draft v0.0.39 勿公开；F4″→0.0.40；A7 九删不变。
- tip 当时 ec855d03；台账短行后进 0586f01a。
- 不撤回本戳；不重刷 gatedeps；不改验收措辞；不打断 B.5 queue。

## 同源重建 / freeze 摘要（不入库整树）
- 冻结树：`/tmp/cc39-freeze2` @82290c17
- 候选树：`/tmp/cc39-b5b`；日志 `/tmp/cc39-b5b/rebuild.log`
- build_ref rc=0 @16:04:34；六平台 pack 步均 rc=0；build_candidate rc=0 @16:06:15
- pair / cand `unisacc-next.com` sha256 **2b20f4b2fd9a496d2e36d47577741420f9e4aba47be0d377e3866f5f91aa5ed9**
- seed `unisacc-seed.com` sha256 **c244c2e488d4ec00f3cb81a18f96647fdaffeb89004a666342bdd749fb5d7ae5**
- comboot 定点 rc=0 @**16:09:33+08**（fixed point：freeze2 == stage3）
- B.5 净开 queue.start=**16:10:29**；QUEUE_BACKUP=/tmp/cc39-b5b/queue-backup-fresh
