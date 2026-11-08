# s9 全批会话（历史c47-1与当前c48-2分账）

## c48-2远程Linux格

cc裁定：状态 **PENDING-REMOTE**，归远程lnx车道。本机无lnx入口是预期，不在本机执行Linux全批，不等待它；本机继续osx原生格与可跨平台的材料/接口施工。

当前候选源SID：`e586721c7767275e1b5af84fe5a3cd199600e2550fc665c26cec642c6f4c9325`。T1合入后本机实算65994/exit0与此全串相等；本机根产品仍f069开发载荷，不作为该SID候选的Linux资格证据。

远程格必须披露以下材料（尚未在本机取得，不影响本机施工）：

- `ROOT`：绝对规范的冻结执行根，实际`unisacc.com`完整SHA与构造/source身份，实算源SID必须为上述e586串；源SID不冒充产物SHA。
- `SESSION`：该ROOT/out下全新会话绝对路径，当前完整注册表、coverage及harness身份；每个作业真实开始/退出、stdout/stderr/status与可重推的receipt。旧会话/旧PASS不迁移。
- `MAP`：绝对材料映射文件及实际内容/身份，包含所有声明的path与sha256材料；loader显式入口须同时绑定loader6目录与loader6-sha，host-cell须候选实际摘要期待值。未制备/未登记/错身份保持未知或NOT-AVAIL。
- 逐项对照`plans/m7-notavail-s9.tsv`原23个逻辑条目及原10 INFRA，拆成多mode的现行批次逐格执行；计数不能通过换分母或只跑单mode降低。NOT-AVAIL目标≤8；INFRA实际转绿或证明真实冲突并登记串行域，不以serial掩盖语义红。

本机材料PREPARED、接口负控、osx原生PASS各自绑定实际f069/材料身份，不折算为该远程格PASS；以下c47-1内容只保留历史口径。见items的C48-2-LINUX。

## 历史c47-1会话

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

P0 OBJ-C-PRUNES-EXTERNAL源修复已经02a7a0d2d推送，旧e586/3d0e不是修复后的冻结身份。新源SID9023ad22…、本机ARM848e0404…和固定f06940模型仅作本机开发分账；六目标符号保留已验，新42包unit闭合/链接尚未验。下一候选须按[源修复集成清单](c48-2-binding-status.md#obj-c-prunes-external源修复集成通道)提供APE/模型完整SHA、ROOT/全新SESSION/typed MAP/原始回执；Linux同候选全批仍PENDING-REMOTE，本机不做、不等。
