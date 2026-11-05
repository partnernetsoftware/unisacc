# lab-loader 归档清单（督导 2026-11-05；cc：先列清单再动，stream 验收通过后一次归档）

依据：cc 改路线——分段管道由产品子命令 `unisacc.com stream STAGE`（7c84ebc669，src/drivestream.cx）承担，lab-loader 转正不再是目标形态；p3-swap 这类开发工具保留。

## A. 被 stream 替代，验收通过后归档（含其批次退役）

| 文件 | 现批次 | 替代 | 依赖它的其他文件 |
|---|---|---|---|
| tools/lab-loader/slice.cx | slice-check（IMPLEMENTED） | stream 直接用产品包，不再切单流容器 | check-boot-head.cx、tools/phase-pipe.cx、experiments.tsv、README |
| tools/lab-loader/check-slice.cx | slice-check | 同上 | — |
| tools/phase-pipe.cx | phase-pipe-single（PLANNED） | stream 七段管道 | lab-loader/README L7 |
| tools/lab-loader/check-phase-pipe.cx | phase-pipe-single | tests/check-stream-pipe.cx | — |
| tools/stream-runtime.c | （被 phase-pipe 调） | stream 在产品进程内跑，无外置 runner | tests/waivers-0.0.43.tsv 提及 |
| tools/lab-loader/cx0-slice.cx、cx0-main.cx、check-cx0-slice.cx | cx0-slice-check（IMPLEMENTED） | cx0 单流容器实验，stream 不需要 | coverage |

## B. 保留（开发工具或不在 stream 路线上）

| 文件 | 理由 |
|---|---|
| tools/lab-loader/p3-swap.cx、check-p3-swap.cx | cc 指定保留的开发工具；tools/cxlib/netwire.cx 与 prd 引用 |
| tools/lab-loader/net-check.cx、check-net-check.cx | tools/release-build.cx 的 p3 --nets 每网 T1 用它（P3 发布资格），不是管道 |
| tools/lab-loader/utf8-gen.cx、check-utf8.cx、utf8-*.tsv | utf8-net 批次（独立网实验，不依赖切片） |
| tools/lab-loader/boot-head.cx、boot-native.cx、check-boot-head.cx、template.cx | booter 头（cx-booter-ape 与 B4 gate 依赖）；check-boot-head 现用 slice.cx 打单流容器——A 归档前须改为不依赖 slice |

## C. 待核

| 文件 | 问题 |
|---|---|
| tools/lab-loader/decode-once.cx | 全仓无引用，疑死代码；归档前确认 |
| tools/lab-loader/core/、cxlib/、runtime/、sys/、mem/、extx/、cx0/ | 子目录逐个看是否只服务 A 类 |
| tools/lab-loader/CONTRACT.md、plan-normalize.md、experiments.tsv | 随 A 类更新或归档 |

## 执行顺序

1. stream 验收（LOADER-GRAD 新 gate）通过，check-stream-pipe 批次 IMPLEMENTED。
2. check-boot-head 改为不经 slice.cx（否则归档 slice.cx 会打断 B 类）。
3. 一次 `git mv` A 类到 archive/lab-loader-legacy/，批次进 retired-batches.tsv 并逐断言对照（同 CHAIN-ARCHIVE 口径），check-archive-refs 通过。
