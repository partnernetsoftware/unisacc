# V2 (0.1.2) B2 2.4: 17 项硬上限全仓消费者只读预审清单

维护窗口: `agy-h` | 日期: 2026-11-25 | 检查性质: 预跑静默窗纯只读全仓扫描(未跑编译) | 对应补丁: `b2-k2-caps-rebased.patch` (`exec/facts/k2-caps.tsv`)

---

## 0. 关键发现摘要 (2.4 验收须同步补漏)

对仓内 `exec/facts/k2-caps.tsv` 的 17 条上限、5 个旧别名(`MAXISPL`/`ENUM_LIMIT`/`SBB`/`POSSPAN`/`STRUCT_LIMIT`)与全部字面量消费者执行只读逐行核对，发现 17 条基线出处全部精准命中，但另有三项隐藏多点消费与步长耦合须在 `2.4`（及 `2.1` 后续补丁）中同步纳管，否则「改表一处」会出现同值消费者分裂:

(1) `INCLUDE_SPLICE_MAX (4000)`: `k2-caps.tsv` 仅记 `exec/pp/directive-body-byte.tsv:43` (`IN34`), 同文件第 `46` 行存在第 2 个完全同义消费者 `IN62` (`CMPI NINCL 4000`)；
(2) `RELOC_MAX (262144)`: `k2-caps.tsv` 仅记 `exec/enc/x86object-result.tsv:30`, 跨文件存在 arm64 等价消费者 `exec/enc/armobject-result.tsv:35` (`s6 AO.record`: `LDI ao_limit 262144`, `C64U obj_nrel ao_limit`)；
(3) 槽位乘数耦合与非容量常量需明示区分:
- `LAYOUT_SID_MAX (128)`: `exec/parse2/layoutfacts-result.tsv:10` 为 `CMPI lf_sid 128`, 同文件第 `16` 行为成员数上限 `CMPI lf_n 128`, 第 `19` 行为稀疏步长 `ALUI mul lf_key lf_sid 128`——若仅放宽第 10 行而不区分第 16/19 行的二维槽步长，直接改值会引发键空间重叠；
- `STRUCT_MEMBER_MAX (512)`: 除 `exec/parse2/control-result.tsv:575` (`CMPI t 512`) 外，成员下标稀疏寻址同步硬编码 `512` 于 `control-result.tsv:570, 578` 及 `exec/parse2/initializers-result.tsv:15` 等多处（而库模式 `exec/parse2/librarytypes-result.tsv:69` 仍为独立上限 `64`）；
- `INT_CONST_BITS (63)` 与 `SETJMP_TEMP_MIN (0)`: 分别为 `64` 位符号边界不变量与深度下界不变量，而非可动态扩容的池大小。

---

## 1. 17 条硬上限逐项消费者对账表

| # | `k2-caps.tsv` 常量 | 值 | 表内登记 `source` (已核) | 仓内实际消费与耦合/隐藏点明细 | 一致性归类 |
|---:|---|---:|---|---|---|
| 1 | `ERR_TEXT_MAX` | `283` | `exec/pp/directive-scan-result.tsv:12` (`CMPI t 283`) | 仅此 1 处字面量消费者(第 13 行拒绝文案同写 `283`) | 单点(已齐) |
| 2 | `INCLUDE_SPLICE_MAX` | `4000` | `exec/pp/directive-body-byte.tsv:43` (`IN34 CMPI NINCL 4000`) | 遗漏第 2 处同义点: `exec/pp/directive-body-byte.tsv:46` (`IN62 CMPI NINCL 4000`) + `directive-body-result.tsv:16` 拒绝文案 | 漏点(+1, 需补记) |
| 3 | `CONTINUATION_MAX` | `4096` | `exec/pp/directive-body-byte.tsv:79` (`CMPI NISP ["constant","MAXISPL"]`) | 经旧别名 `MAXISPL` 引用 `exec/facts/pp-layout.tsv:20` (`4096`, 由 `tools/facts-gen.cx` 产出); `directive-body-result.tsv:5` 拒绝文案 | 别名同步(`pp-layout.tsv`) |
| 4 | `INT_CONST_BITS` | `63` | `exec/parse2/unarycontrol-result.tsv:177` (`$f_part6_1206_U_b_1 -> DEAD.big`) | 行 176 为 `CMPI nx 1`, 实际位宽常量 `WORD_SIGN_SHIFT=63` 定义在 `exec/facts/k2-control.tsv:2`, 拒绝文案在第 185 行 | 位宽不变量(源表行非直接 `63` 字面量) |
| 5 | `ENUM_POOL` | `128` | `exec/parse2/enumtypes-result.tsv:8` (`CMPI en_base ["constant","ENUM_LIMIT"]`) | 经旧别名 `ENUM_LIMIT` 引用 `exec/facts/enumtypes.tsv:4` (`=ENUM_LIMIT 128`) | 别名同步(`enumtypes.tsv`) |
| 6 | `SIGNATURE_POOL` | `8192` | `exec/parse2/functiontypes-result.tsv:28` (`CMPI fs_code ["constant","SBB"]`) | 经旧别名 `SBB` 引用 `exec/facts/k2-control.tsv:2` 与 `k2-gen2.tsv`, 被 `functiontypes` 与 `initializers` 共用作签名池边界与槽基准 | 别名同步(`k2-control.tsv`) |
| 7 | `SETJMP_EXPR_DEPTH_MAX` | `40` | `exec/parse2/setjmpcontrol-result.tsv:86` (`CMPI sj_depth 40`) | 仅此 1 处上限消费者, 第 88 行输出拒绝文案 | 单点(已齐) |
| 8 | `STRUCT_ID_MAX` | `2049` | `exec/parse2/control-result.tsv:510` (`CMPI nsid ["constant","STRUCT_LIMIT"]`) | 经旧别名 `STRUCT_LIMIT` 引用 `exec/facts/k2-control.tsv:2` (`STRUCT_LIMIT: 2049`), 注意受 `MEMBER_STRIDE=4096` 槽域约束 | 别名同步(`k2-control.tsv`) |
| 9 | `STRUCT_MEMBER_MAX` | `512` | `exec/parse2/control-result.tsv:575` (`CMPI t 512`) | `SMN` 比较仅在 `control-result.tsv:575`; 但成员数组乘法步长 `512` 硬编码在 `control-result.tsv:570,578` 与 `exec/parse2/initializers-result.tsv:15` (同步耦合, 非 `librarytypes-result.tsv:69` 之 64) | 单点比较 + 步长耦合提示 |
| 10 | `FORWARD_CALL_MAX` | `8192` | `exec/parse2/forward-result.tsv:17` (`CMPI fw_count 8192`) | 仅此 1 处表级阈值(产品实界为车 2 修复的 `FWD_STUB_BYTES=131072`, 与 `8192` 并存) | 单点(已齐) |
| 11 | `RELOC_MAX` | `262144` | `exec/enc/x86object-result.tsv:30` (`CMPI obj_nrel 262144`) | 遗漏第 2 处架构等价点: `exec/enc/armobject-result.tsv:35` (`LDI ao_limit 262144`, `C64U obj_nrel ao_limit`) | 漏点(+1, arm64 需补记) |
| 12 | `LAYOUT_SID_MAX` | `128` | `exec/parse2/layoutfacts-result.tsv:10` (`CMPI lf_sid 128`) | 同文件第 16 行限制每结构成员条数 `CMPI lf_n 128`, 第 19 行计算二维槽键 `ALUI mul lf_key lf_sid 128` | 单点比较 + 二维槽耦合提示 |
| 13 | `SIG_EQ_DEPTH_MAX` | `31` | `exec/parse2/functiontypes-result.tsv:388` (`CMPI fs_eqdepth 31`) | 仅此 1 处递归深度消费者 | 单点(已齐) |
| 14 | `SETJMP_TEMP_MIN` | `0` | `exec/parse2/setjmpcontrol-result.tsv:89` (`CMPI sj_depth 0`) | 仅此 1 处非负平衡检查(第 91 行拒绝文案 `setjmp temporary depth`) | 单点下界不变量(已齐) |
| 15 | `SHAPE_SPAN` | `67108864` | `exec/parse2/shape-result.tsv:120` (`CMPI shape_next ["constant","POSSPAN"]`) | 经旧别名 `POSSPAN` 引用 `exec/facts/k2-gen2.tsv:36,55` (`67108864`), 共用位置/形状空间域 | 别名同步(`k2-gen2.tsv`) |
| 16 | `MODELGRAPH_NODE_MAX` | `17409` | `exec/modelgraphequality-result.tsv:12` (`CMPI mg_next 17409`) | 仅此 1 处节点池上限消费者 | 单点(已齐) |
| 17 | `TOKEN_POS_SPAN` | `67108864` | `exec/parse2/control-result.tsv:2231` (`CMPI toklen ["constant","POSSPAN"]`) | 与 #15 共享 `exec/facts/k2-gen2.tsv` 中的 `POSSPAN` 别名 | 别名同步(`k2-gen2.tsv`) |

---

## 2. 对 `V2 (0.1.2)` `B2` 实施与 `2.4` 验收的处置建议

(1) 不破坏当前已验证 `b2-k2-caps-rebased.patch` (`2.1..2.3`):
- 当分出 `v0.1.2` 树时，先原样合入 `b2-k2-caps-rebased.patch` 完成 `C1a`（保证首件 8/0 与 18/0 立即绿）。
(2) 在 `2.4` 消费者遍历收口 (`C1b`) 时明确两项增补动作:
- 把 `exec/pp/directive-body-byte.tsv:46` (`IN62`) 与 `exec/enc/armobject-result.tsv:35` (`AO.record`) 作为显式次级消费点登记进 `k2-caps` 校验清单或事实注释中，消除「留一处字面量未被覆盖」的真缺口；
- 对 `STRUCT_MEMBER_MAX (512)` 与 `LAYOUT_SID_MAX (128)` 在 `2.4` 报告中如实区分「上限比较 (`CMPI`)」与「稀疏槽位乘法步长 (`ALUI mul ... 512 / 128`)」，防止后人误改单边常量。
