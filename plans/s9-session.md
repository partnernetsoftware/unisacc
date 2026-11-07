# s9 全批会话（c47-1 APE 一出即起）

不依赖已删路径（无 `/tmp/grk-s5-c46-2`）。streams 用 `STREAMS_ROOT` 或 cwd/`unisacc.com`（`tools/streams-material.cx`）。

## 一条命令

```
# P=c47-1 私有根（含 APE 作 unisacc.com）
# MAP=s9.map 见下
sh $P/unisacc.com tools/s8c-bind.cx -- $P $P/out/s9 $P/unisacc.com $P/out/s9.map
# 然后 drive：light 14、heavy 6；跳 cx-resolver/wt-gate 至后置；full-* = KNOWN-GAP
```

## s9.map（M7 转绿 45 的构造，按 m7-material-spec）

| 名 | 构造（在 $P 上，APE=当前 unisacc.com） | 批 |
|---|---|---|
| objects | `object-material.cx prepare $P $P/out/mat/objects` | objects 5 PASS |
| kernel-route | `kernel-route.cx -- $P $P/out/mat/kernel-route --isa x86_64 --isa arm64` | kr 7 PASS |
| ri-flow-runtime | `ri-material.cx -- $P MAT tests/cases/ri-flow.cx` 然后 compile\|seal | ri-flow 9 |
| ri-chain | `ri-chain-material.cx -- $P MAT` | ri-chain 7 |
| pp-gold | `pp-gold-material.cx -- prepare $P MAT` | pp-gold 2 |
| streams | `streams-material.cx -- $P MAT`（**不烘 ROOT**） | s8f 15 PASS |

p3 挂起（需 ctx tool-pack）。ref / `*-sha:sha256` 不造伪。

init 后必须 `s8c-bind` 对 MAP 每名 `all-waves material`，否则 `materials_registered=no`。

## 对照模板

`plans/s9-vs-s8b.tsv`：`batch<TAB>c46-3<TAB>c47-1`。s9 出结果再填第三列。
