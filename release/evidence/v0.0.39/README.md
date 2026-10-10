# v0.0.39 unsigned custody (publish.sh step 3 deleted assets)

> Custody only. **Not** a re-publish. **Not** a change to `research/r39-release-acceptance.json` `published.*`.

## Why
`publish.sh` step 3 removed draft assets `unisacc-unsigned.zip` and `unsigned-receipt.json` before making `v0.0.39` public. Public release keeps only `unisacc.com`. Optional 机房主任 19:06: re-fetch those two for in-repo safekeeping.

## Obtained: YES

| file | sha256 | bytes |
|---|---|---|
| `unisacc-unsigned.zip` | `04cfd258f6e385555fec7df552417fb198f838e48f85d6cb0dac9a842c98d19d` | 1500388 |
| `unsigned-receipt.json` | `f3e0098bb50077cd5368bec6adaadffe0c6c48e7d4f4dd280eb7cfbc6051b1e8` | 737 |

Want values = `research/r39-release-acceptance.json` → `windows.unsigned_zip_sha256` / `windows.unsigned_receipt_sha256`.

## Provenance (GHA, read-only download)

| source | id |
|---|---|
| qualification run | [38043394306](https://github.com/partnernetsoftware/unisacc/actions/runs/38043394306) |
| company run | [38043453555](https://github.com/partnernetsoftware/unisacc/actions/runs/38043453555) |
| qual artifact | `unisacc-sealed-signing-input-1` id `11666555723` |
| company artifact (same input) | `unisacc-sealed-signing-input-1` id `11666214766` |

- `unsigned-receipt.json`: **byte-identical** member of both sealed-signing-input artifacts (and of `unisacc-company-signed-1`).
- `unisacc-unsigned.zip`: **not** stored as a named artifact member. Rebuilt from artifact `unisacc.com` (sha `2b20f4b2…` = `windows.before_sha256`) with the same recipe as `release/tools/unsigned_receipt.py` (`ZipInfo('unisacc.com',(1980,1,1,0,0,0))` + `ZIP_DEFLATED`). Rebuilt archive sha = want `04cfd258…`. Cross-check: `/tmp/cc39-b5b/windows/unisacc-unsigned.zip` same bytes.
- Deleted release asset ids `627609743` / `627609748` are **gone** from tag `v0.0.39` (public assets: `unisacc.com` only); not re-uploaded.

## Boundaries
- Did **not** re-run `publish.sh`, edit Draft/Latest, or re-attach deleted assets.
- Did **not** edit receipt `published.*` (four-eyes / publish wording untouched).
- Recorded CST: 2026-10-10T19:10+08:00
