# acceptance.sh 10-* 裸 `-c` 同源风险（1453）

## 同源断言（相对 ed50f00c / ruling-b139-pc-fix-1446）
- 根因：裸 `unisacc -c` 默认 `t=lnx/x86_64` → tape 烤 Linux `MAP_ANON(0x20)` → Darwin from-tape 镜像 mmap EBADF(−9) → store-indirect SIGSEGV（B=139）。
- `tests/bootstrap.sh` 三处已钉 `-t "$HOST_TARGET" -c`（tip `ed50f00c`）。
- `tests/acceptance.sh` sec10 在 `ACCEPT_CASE=all` 时委托 `bootstrap.sh`（已修）。
- **残留**：`10-Btape` / `10-Ctape` / `10-compare` 三处仍裸 `-c`；`10-Bimage`/`10-Uimage` 曾硬编码 `--target osx/arm64`，与 tape 目标可不一致。

## 证据够否
够。与 bootstrap 同调用形（UA/B/U + SELF + `-c`），同 Darwin/arm64 门控（sec10 早退非 Darwin）。m4pro 已证裸 `-c`→imm 34、加 `-t osx/arm64`→imm 4098 且 B 不再 139。

## 最小修（本刀落地）
| 位点 | 改前 | 改后 |
|---|---|---|
| 10-Btape | `"$UA" "$SELF" -c` | `-t "$HOST_TARGET" -c` |
| 10-Ctape | `"$S/B" "$SELF" -c` | `-t "$HOST_TARGET" -c` |
| 10-compare | `"$S/U" "$SELF" -c` | `-t "$HOST_TARGET" -c` |
| 10-Bimage/Uimage | `--target osx/arm64` | `--target "$HOST_TARGET"`（与 -t 对齐） |
| 前置 | 仅 Btape `. lib.sh` | 分片路径统一 `. lib.sh` + `HOST_TARGET` 且断言 `osx/arm64` |

不改：验收谓词、`ACCEPT_CASE=all` 委托、sec6 等其它 osx/arm64、`src/main.c` 默认 target。禁 bump/Draft。
