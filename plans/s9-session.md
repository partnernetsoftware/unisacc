# s9 全批会话（c47-1 APE 一出即起）

不依赖已删路径（无 `/tmp/grk-s5-c46-2`）。streams 用 `STREAMS_ROOT` 或 cwd/`unisacc.com`（`tools/streams-material.cx`）。

## 口径（同 s8b illegal-action：结论披露 overlay）

- **product_source**：`7b98ec2c06`（构造 APE；不复用 f748/s8 PASS）。
- **harness_commit**：`>=4cdbb3ac7d`，只覆盖 `tests/`。现测 `4cdbb3ac7d:tests` = `0d515c9ad48dfbbd62b8e0fc11de243b36037115`（主树 git rev-parse，非私有 FP）。
- **覆盖文件清单**（`7b98ec2c06..4cdbb3ac7d -- tests/`）：`tests/README.md`、`tests/batches.tsv`、`tests/coverage.tsv`、`tests/tools-compile-boundaries.py`、`tests/tools-compile-sources.tsv`、`tests/tools-compile.cx`、`tests/update-tools-compile.py`。s9 组装根上再 `git rev-parse HEAD:tests` 写入 session 披露。


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
| nets | `net-material.cx -- $P MAT` 然后 `MAT/nets.cx -- $P` | 2026-11-08 NETS PASS models=18 |
| hostcc | MAP `hostcc	/usr/bin/x86_64-linux-gnu-gcc-11`（s8c-bind 已 mapped=1，无仓内构造） | c-negative 实跑 C_NEGATIVE pass=21 fail=38 rc=1，**转绿 0**（既有 c.negative 红行，非缺绑定） |
| hostar | MAP `hostar	/usr/bin/x86_64-linux-gnu-ar` | formats-archive-ar |
| hostld | MAP `hostld	/usr/bin/x86_64-linux-gnu-ld.bfd` | formats-object-ld |
| assembler | `asm-material.cx -- $P MAT` | 未实跑 |
| identity-helper | `identity-material.cx -- $P MAT BOOTSTRAP` | 未实跑 |
| libneed-native | `libneed-material.cx -- $P MAT BOOTSTRAP` | 未实跑 |
| ri-cli | `ri-material.cx -- $P MAT tests/cases/ri-cli.cx` | 未实跑 |
| prune-cases | NOT_CONSTRUCTIBLE tapeprune.c | — |
| model-identity | NOT_CONSTRUCTIBLE canonical P3 | — |
| e2-input-binding | NOT_CONSTRUCTIBLE fixture-footer | — |

64 自造分组：`plans/s9-self64-groups.tsv`。47 冻结参考：`plans/s9-ref47.tsv`。

p3 挂起（需 ctx tool-pack）。ref / `*-sha:sha256` 不造伪。

init 后必须 `s8c-bind` 对 MAP 每名 `all-waves material`，否则 `materials_registered=no`。SESSION 必须在 `$P/out/` 下。

## 2026-11-08 0.0.46 APE 实跑（s8c 快照+s8f streams，<1G）

- prune-cases：`REFUSE classic sources errno=2`（`/tmp/grk-s8c/src/tapeprune.c` 缺）。转绿 0。
- model-identity：`REFUSE not canonical P3`。转绿 0。
- e2-input-binding：`FAIL candidate-selected-e2-model`… `fixture-footer errno=11`。转绿 0。
- s9 init 空 MAP：`/tmp/grk-s8c/out/s9-dry` BIND_DONE mapped=0，不依赖已删目录。
- make ref 私有克隆 137M：`sourceflat: cannot read unisacc.c` elapsed=3.333。自指钉设计 `plans/s9-self-pin.md`。tapeprune/adopted-package 仍 KNOWN-GAP。

## 对照模板

`plans/s9-vs-s8b.tsv`：`batch<TAB>c46-3<TAB>c47-1`。s9 出结果再填第三列。
