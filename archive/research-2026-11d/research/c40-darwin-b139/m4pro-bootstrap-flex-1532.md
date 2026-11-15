# m4pro 本机 bootstrap B=C=U 取证 @ tip 7acb254a（1532 flex）

## 终表
| 项 | 值 |
|---|---|
| 宿主 | m4pro.local · Darwin 25.5.0 · arm64 (T6041) · macOS 26.5.1 (25F80) |
| tip | `7acb254a217f614b73c811d4b9575160cfa86dd0` |
| 含 | `ed50f00c`（bootstrap `-t HOST_TARGET`）、`c2b140bc`（acceptance 10-*） |
| worktree | `/tmp/unisacc-m4pro-flex-1532`（detached；主检出 dirty 旁路） |
| HOST_TARGET | `osx/arm64` |
| 墙钟 | 2026-10-11 15:33:59 → 15:34:24 CST（约 25s） |
| `./tests/bootstrap.sh` | **rc0** |
| 判定行 | `ok B=C=U 257c4ef6944103ef (421304 lines)` |
| fixed point | **reached** |
| B=C / B=U | yes / yes（三 tape 同 sha256） |
| tape sha256 | `9266b52f953646af278b9fb28862e873da51728336280e37dac10fcd175424a1` |
| brc / urc | 0 / 0 |
| B 镜像 | 2328418 B · text 1840540 · 396903 insns · magic cffaedfe |
| U 镜像 | 2328418 B · text 1832636 · 402463 insns · magic cffaedfe |

## 对照（历史红）
| 采样 | tip | bootstrap |
|---|---|---|
| 1446 FINDINGS | `82e1a9f0` | FAIL B=139 U=0 |
| **本刀 1532** | `7acb254a`（含 ed50f00c/c2b140bc） | **ok B=C=U** |

## 旁观 CI（不混判）
- run `38119555397` @ `ed50f00c`：按 1532 裁定仍旁观；本证是 m4pro 本机绿，不代写 CI 产品 PASS。

## 入仓附件
- `m4pro-flex-1532-SUMMARY.txt`
- `m4pro-flex-1532-bootstrap-full.log`
- `m4pro-flex-1532-meta.txt`
- `m4pro-flex-1532-B.log` / `m4pro-flex-1532-U.log`
- （未入仓：7MB tape／镜像；哈希已钉）

## 硬禁备注
本页=取证。不构成 Latest／公开／擅自开 Draft；开 Draft 前须再确认公开边界。
