# 回执：v0.0.40 公开发布 + 切 Latest（unsigned 最大可发）

墙钟：2026-10-11 18:04:02 +0800（API publishedAt 2026-10-11T10:04:02Z）。

## 授源
政委：「如果m4pro已经过了就先发布吧」= 授权切 Latest / 公开发布 v0.0.40（m4pro bootstrap 绿为准）。
缺签则尽最大可发布（unsigned 可发则发，签另记）。

## 核对（发布前）
| 项 | 结果 |
|---|---|
| m4pro bootstrap 绿 | **齐** — tip `7acb254a` ok B=C=U `257c4ef6944103ef` / rc0（`release/evidence/v0.0.40/m4pro-bootstrap-flex-1532.md`） |
| Draft 产品资产 | **齐** — `unisacc.com` / `unisacc-unsigned.zip` / `candidate.json` / `unsigned-receipt.json` / `SHA256SUMS-unsigned.txt` + 1532 证据包 |
| seal / rc | `candidate.json` 0.0.40；GHCR `:0.0.40` @sha256:8798d9c8…；`rc/v0.0.40` → `1db3083506e7af178ed604052532d4226511ad01` |
| Apple dmg / Win 企业签 | **缺** → 不走 `publish.sh`（需签后 acceptance）；改走 unsigned 公开 |

## 动作
1. `gh release edit v0.0.40 --draft=false --latest` + 更新公开 NOTES
2. **未**跑 `release/tools/publish.sh`（无签后 `release_eligible` 回执；本刀显式走 unsigned）
3. **保留** unsigned 资产（未按签后路径删 zip/receipt）

## 发布后核
| 项 | 值 |
|---|---|
| Latest | **v0.0.40**（原 Latest v0.0.39 已让位） |
| draft | **false** |
| URL | https://github.com/partnernetsoftware/unisacc/releases/tag/v0.0.40 |
| target | `1db3083506e7af178ed604052532d4226511ad01` |
| git tag `v0.0.40` | → `1db3083506e7af178ed604052532d4226511ad01` |
| 公开 `unisacc.com` sha256 | `50ab4c213feedb2f1a4f1f39c8bfca7d8d63c74a1d6b50915db8ceb233f5cf58` = candidate / Draft 字节（**HASH_OK**） |
| tip at receipt commit prep | `162261918d9e315c2048207f78bebd8efedf318e`（research-only since seal；产品闭包未变） |

## 公开资产（保留全集）
candidate.json, HASHES-1532.sha256, HASHES.sha256, m4pro-flex-1532-evidence.tar.gz, ruling-open-draft-v0.0.40-1626.md, SHA256SUMS-unsigned.txt, unisacc-unsigned.zip, unisacc.com, unisacc.com.build.json, unsigned-receipt.json

## 签另记（卡点 / 未做）
1. Apple dmg / Developer ID + notarize（m4pro offline 本窗）
2. Windows company signing / qualification
3. 全量 `tests/release.sh --com` 对封存字节
4. 签后 `research/r40-*` `release_eligible` + `publish.sh` 路径（待签齐后可另刀）

## 禁项自检
- [x] m4pro 绿为准已核；[x] Draft 资产已核；[x] 已 `--draft=false --latest`；[x] 公开哈希 = unsigned candidate；[x] 未冒充已签；[x] 签缺口另记。

## 附：unsigned-receipt.json 修复
Draft 上传件 `sealed_from_commit` 含意外换行（`b1290\nc749…`）；公开发布后已替换为合法 JSON（同 sha256 产品字节不变），并标 `published_unsigned=true` / `latest=true`。
