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

转绿（同源 884adfef 重构造后实跑 PASS）：objects 5 + kernel-route 7 + ri-flow 9 + ri-chain 7 + pp-gold 2 = **30**。streams 28 实跑未完，不改此数。**p3: 需 ctx tool-pack（chain-step p3 REV S SRC_PKG OUT --nets CTX/jobs --routes R），待磁盘/候选链复用，本轮不做。**

