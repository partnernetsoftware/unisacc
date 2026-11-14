## unisacc 0.0.40

**Status: Public / Latest. Unsigned product bytes (signing deferred).**

### Authority
- 政委：「如果m4pro已经过了就先发布吧」→ 授权切 Latest / 公开发布 v0.0.40（m4pro bootstrap 绿为准）。
- 缺签则尽最大可发布：unsigned 可发则发，签另记。
- Flex 裁定：`research/c40-darwin-b139/ruling-darwin-host-m4pro-flex-1532.md`。
- Draft 开窗：`research/c40-draft-open-1626/ruling-open-draft-v0.0.40-1626.md`。
- Seal：`research/c40-seal-1645/receipt-seal-draft-assets-1645.md`。

### Local-green evidence (m4pro)
- Host: m4pro.local Darwin arm64
- Tip: `7acb254a` — `ok B=C=U 257c4ef6944103ef` / 421304 lines / rc0
- Pack: `m4pro-flex-1532-evidence.tar.gz` (+ HASHES)

### Product seal
- `release/candidate.json` → **0.0.40**
- GHCR: `ghcr.io/partnernetsoftware/unisacc-candidate:0.0.40` @sha256:8798d9c880c9fddb5e8ae09c9e8fc0e6d22ec5247814811e8ba2e70900215386
- `unisacc_com_sha256` (unsigned): `50ab4c213feedb2f1a4f1f39c8bfca7d8d63c74a1d6b50915db8ceb233f5cf58`
- `sealed_from_commit`: `b12901c7…`；`rc/v0.0.40` → `1db30835`
- Attached: `unisacc.com` (unsigned), `unisacc.com.build.json`, `unisacc-unsigned.zip`, `candidate.json`, `unsigned-receipt.json`, `SHA256SUMS-unsigned.txt`

### Signing deferred (另记)
- Apple dmg / Developer ID sign + notarize — not done this window
- Windows company signing — not done
- Formal `publish.sh` path (signed acceptance / release_eligible) — not used; unsigned public by explicit authority

### Note
- Tip CI observational only; m4pro bootstrap green is the publish gate for this cut.
