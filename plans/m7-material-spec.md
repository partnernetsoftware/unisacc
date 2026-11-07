# M7 材料规范（从批次 REFUSE 代码抄，非试错）

`materials-prepare` 先按此表检查再跑批。PREPARED≠PASS。

| 类 | 判定源 | 目录/叶 | 计数与 receipt | 构造命令 |
|---|---|---|---|---|
| kernel-route | `tests/kernel-route.cx:81-88` | `route.tsv`；`{x86_64,arm64}/{11unit}.{s,o}`；`{isa}.kernel` | `canonical=YES`；`stale=0`；**input=33 source=22 object=22 kernel=2**（双 ISA×11） | `tools/kernel-route.cx -- ROOT OUT --isa x86_64 --isa arm64` |
| objects | `tools/object-material.cx:12,20,29` `tests/objects.cx:verify` | `main.cx` `value.cx` `main-x86_64.o` `value-x86_64.o` `value-arm64.o` `material.tsv` | banner `objects-material-v1`；`--verify` 打印 `VERIFIED objects-material`；product/tool/netpack sha 对应当前 ROOT | `object-material.cx prepare ROOT MAT`（须在该 ROOT/APE 上构造） |
| ri-flow | `riflowmaterial.cx:5,37-43` | `runtime.c` `material.tsv` `flow-native` `flow-native.tsv` | `ri-material-v1`；bootstrap=产品 sha；**9 个 closure 字段** `exec/c/{run,core}.c core.h codec.h packagefooter.h resourceledger.h` `include/unisa_sha256.h` `tools/ri-material.cx` **`tests/cases/ri-flow.cx`**。误用 `ri-cli.cx` 会 `material hash field missing or duplicate` | `ri-material.cx ROOT MAT tests/cases/ri-flow.cx` 然后 compile\|seal |
| ri-chain | `tools/cxlib/richain.cx:41` `tests/ri-chain-controls.cx:33` | `material.tsv` `model0.tbl` `model1.tbl` `model0.net` `model1.net` | receipt 字节 ≡ 现算；错则 `material-receipt-mismatch` | 现有 ri-chain 材料工具，须对应当前 APE |
| p3-* | `tests/p3-library-members.cx:27` | p3-pack-ctx 下规则文件 | `regular-input`：存在、常规文件、≤8MiB | 图有路径仍缺叶文件则补叶，不造伪 |
| pp-gold | `tools/pp-gold-material.cx:62-64` | `pp.weights` `pp-observe.cx` `material.tsv` | `--verify` 与 `record()` 字节相等；`material-content-mismatch` | `pp-gold-material` 对当前产品重构造 |
| ref / streams,ref | — | 冻结参考 | **不可自造** | 标来源 |

kernel-route 单 ISA 会得到 inputs=22/11/11/1，被 :88 拒绝。objects 必须在同一快照 ROOT 上 prepare，否则 `material receipt mismatch`。

## 进度 2026-11-07

转绿 **45**（与 30 无重叠）：objects5+kr7+ri-flow9+ri-chain7+pp-gold2 + s8f streams **15** = peep-0..7, opinfo-0/1, properties-bound/tu, e4-semantics, prune-io, prune-semantics。p3 挂起。prune-cases=材料 classic sources errno=2；model-identity=材料 not canonical P3；det/meta=PROPERTY not_run=1（既有 STREAM-DET-NOTRUN / STREAM-WIN-TAPE-RT）。

## streams / PEEP / OPINFO（2026-11-07 定性）

- **unrealizable=179**：`tools/peep-cases-gen.cx:178-180`。`realise()` 返回 0 且 `y==keep` → `UNREALIZABLE keep-by-definition`，**不跑 e4**。200 键里 179 条 keep 不构成测试。
- **手工一例** `0000.store64.store64.none`（y=keep 却被 realise 成可跑？results 记 FAIL rc=2）：want 四行 `_start:/store64/store64/ret`；got **空文件**。diff= want 全文被删。不是 peep 改写错，是 **STREAM4_COM 对 tape rc=2 无 stdout**。
- **三档**：①材料/运行器——21 FAIL 均 `rc=2` 空 `.out`（与 classic sources / product-identity 同类，不是网语义）；②用例表——179 unrealizable 使 keep 网几乎无正例；③网真缺陷——**本例未证明**。不立 P1 网缺陷。OPINFO fail=54 同批运行器，未逐字节。


