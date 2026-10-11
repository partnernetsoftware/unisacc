# 1446 PC→窄修旁核（cdx）

采样：与 ruling-b139-pc-fix-1446 同期。已读 FINDINGS/IPS 关键/step rc/崩溃反汇编对照/U mmap 0x1002/bootstrap 缺 -t / predef 与 mman.h。

## 核验
- PC `0x10000b2d0` `str x0,[x1]` + FAR/−9 + Darwin mmap `neg`→−errno(9) 链：**成立**。
- B tape `imm r0, 34`×N 且无 4098；U 镜像 `__mmap` 用 `0x1002`：**成立**。
- bootstrap UA/B/U 三处裸 `-c` 与 `main.c` 默认 `lnx/x86_64`：**成立** → 修 `tests/bootstrap.sh` 三行加 `-t "$HOST_TARGET"` **不越界**。
- 非 emit store-indirect 根因；不扩到 `emit_arm.py` / `back_encode.c`：**同意**。
- 硬禁：未见 bump/Draft/放宽。

## 风险
- 只改 bootstrap 后，`acceptance.sh` Darwin 10-Btape 仍裸 `-c`（硬编码 from-tape osx）→ 分片路径可能仍红；与 bootstrap all 路径分开记账。
- tip diag `38118763767` 不含本修；勿用其 Darwin 结果证明本刀。

## 结论
旁核：**通过** — 证据够窄修；修域文件仅 `tests/bootstrap.sh`。
