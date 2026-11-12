## unisacc 0.0.40 (Draft)

**Status: GitHub Draft only. Not Latest. Not published.**

### Authority
- 政委：自主推进；跳过 Draft 边界问。
- 机房主任 1626 代裁：按 m4pro flex，本机绿证据齐 → `gh release create v0.0.40 --draft`。
- Flex 裁定：`research/c40-darwin-b139/ruling-darwin-host-m4pro-flex-1532.md`。
- 本刀裁定：`research/c40-draft-open-1626/ruling-open-draft-v0.0.40-1626.md`。

### Local-green evidence (attached)
- Host: m4pro.local Darwin arm64
- Tip: `7acb254a` — `ok B=C=U 257c4ef6944103ef` / 421304 lines / rc0
- Includes: `ed50f00c` (bootstrap `-t HOST_TARGET`), `c2b140bc` (acceptance 10-*)
- Pack: `m4pro-flex-1532-evidence.zip` (+ HASHES)

### Explicitly NOT done (does not block this Draft window)
- Product `seal_candidate` for 0.0.40 / `rc/v0.0.40`
- Unsigned/signed `unisacc.com`, dmg, Windows signing
- B.2 / B.3 comboot fixed-point on current tip
- Tip CI runs remain observational (do not gate this Draft)

### Hard bans
- Do not set Latest to v0.0.40
- Do not `gh release edit --draft=false` / `publish.sh` without separate authority
