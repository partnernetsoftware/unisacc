# S 第0步回执：E1→e3 流接口合同（stagecontract-e1-e3）

## 结论范围（只是实测范围）

- 固定夹具：`tests/seals/stagecontract-e1-e3/`（accept/ 为 HEAD 捕获的整个目录；e3.net 由 `tests/modelstreamextract.cx` 原样抽取，sha256 `5e9ab6c4…ea0ed0ea6`）。
- 产品：HEAD 干净导出 `unisacc-next.com`，sha256 `83e6e7e4…dafb`；包 `compiler.pkg` sha256 `71832a39…d67`。
- 这是**一个输入、一个 e3 阶段、一条路由**的测量，不是整个阶段或整个候选的裁决。
- 检查：`tests/stagecontractcheck.cx`（运行于 STALE 守卫之后）；回放器：`tests/middleflowrunner.c`（replay 模式）。

## 结果（HEAD 产品 83e6e7e4）

| 项 | 结果 | 依据 |
|---|---|---|
| STALE 守卫 | 通过 | 从产品重抽 e3.net，与冻结件逐字节相同 |
| S-POS-01 | 通过 | 回放 accept 捕获，stdout 与 output.bytes 逐字节相同，stderr 空 |
| S-NEG-RESOURCE | 通过 | 删除一条已请求的 present 行：rc 2，stdout 空，`MISSING_EXPLICIT_RESOURCE <key>` |
| S-NEG-RESET | 通过 | accept、拒绝、再 accept：第二次 accept 与第一次逐字节相同 |
| S-NEG-DUP | 通过 | 同一 key 两行（present 在前、absent 在后）：rc 2，stdout 空，`INTERFACE_ERROR duplicate-resource-key` |
| S-NEG-FRAME | 通过 | 声明长度 5、blob 实为 4 字节：rc 2，stdout 空，有诊断 |
| S-NEG-MODEL | 通过 | 模型 sha 不符（`MF_MODEL_SHA256`）：rc 2，stdout 空，`MODEL_IDENTITY_MISMATCH` |
| S-NEG-ABSENT | **UNKNOWN** | 该 key 显式 absent：rc 0，stdout 与 output.bytes 逐字节相同，trace 为 `present 0`，未报 MISSING |
| S-NEG-EMPTY | **UNKNOWN** | 该 key present 且 0 字节：rc 0，stdout 与 ABSENT、POS 相同，trace 为 `present 1 len 0` |

- ABSENT 与 EMPTY 的答复不同（trace 可见），但输出字节相同。按约定记为“该输入上该阶段不区分”，依据是此二者输出相同；不声称语义差异。
- UNKNOWN 不是通过。这两项没有预先的结果预测，因此不预填拒绝。
- middle-flow-e3 作业（根产品 `./unisacc.com`，`--sealed-e3`）通过（rc 0），未设 `MF_MODEL_SHA256`，行为不变。

## 回放器的两处改动（提交 f2fc06a3）

1. 重复 key（不论 presence 是否一致）→ INTERFACE_ERROR，rc 2，主输出空。此前回放取第一条，DUP 因此被接受（这是测试适配器的漏洞，不是产品合同红）。
2. 可选的模型身份校验 `MF_MODEL_SHA256`：设了就校验模型 sha，不符则拒绝。此前回放对模型字节不敏感（见下）。

ABSENT/EMPTY 的答复逻辑未改：presence 位原本就逐行生效。新增的 `MF_TRACE` 只在检查里设置，用于记录每次答复。

## MODEL 负例的发现：e3.net 末字节对回放不敏感

- 翻转 e3.net 的最后一个字节（偏移 648168，即末字节），旧回放仍然 rc 0，stdout 与 output.bytes 逐字节相同。
- 也就是说，回放对这一字节不敏感。模型字节与身份之间原本没有绑定，这是 tests 侧回放器的缺口，改动见上面第 2 点。
- **解释（只读，已验证）**：偏移 648168（1 基，即末字节）属于最后一个状态的银行头 `H 0 0 256 0 10513 455`（状态 10513 = NS−1，0 行，bn=10513，bq=455）。它**不是填充，也不是被跳过的段**：
  - 装载器在二进制模式下逐字节消费并以 `if (LP != LN) die("trailing model data")` 收尾（exec/c/run.c），所以该字节一定被读取，不会被静默忽略。
  - 执行器对每次转移都读 `base_seq[q]`（exec/c/core.c 的 `core_transition`，对 `isnet` 分支 `s = m->base_seq[q]`），因此 `bq` 只在进入状态 q 时生效。
  - 实测（`exec/c/` 的临时插桩副本，仅记录访问过的状态，产品未改）：accept 路由下访问过的状态最大为 10512；原模型与翻转模型都**没有进入 10513**，两者输出逐字节相同。于是“翻转后输出不变”的原因是：**该输入从未进入这个状态**，而不是字节无意义。
  - 构造器侧：`exec/c/networkformat.py` 的 `encode` 只是把文本网络逐字段序列化，二进制是文本网络的纯函数；文本行 `H 0 0 256 0 10513 455` 来自构造器输出。产品级确定性由 STALE 守卫（整文件逐字节比对，包含该字节）覆盖；本步**未**核对 unisa 构造器（Python）与 C/P 对该行的写法是否一致。
  - **风险（列为风险，未闭合）**：模型身份校验（`MF_MODEL_SHA256`）覆盖整个文件，因此任何字节改动都会被拒，身份本身没有漏洞。但回放在**本路由**上对该状态的 `bq` 不敏感：若别的输入能进入状态 10513，本夹具就无法见证该字段的改动。“10513 是否对其他输入可达”未查证，需要另做可达性分析（不在本步范围内）。

## 未做 / 边界

- 根产品 `./unisacc.com` 上的检查未重跑（此前为 STALE，需先重抽 e3.net）。
- capture 级别的重复请求未单独做（ledger 级已覆盖）。
- 本步不改 exec/、src/、unisa/、run.c、W 文件，也不动 gate 登记。

## 复现

```
sh <PRODUCT> -run tests/stagecontractcheck.cx -- --fixture tests/seals/stagecontract-e1-e3 --product <PRODUCT>
```

HEAD 产品：`/home/box/c43/W/cand/unisacc-next.com`。
