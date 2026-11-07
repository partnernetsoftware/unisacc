# 切片瘦身前置（SLICE-SHRINK-PRE）

**不改产品行为。不碰 loader。本页待 fable 评审后才动产品。**

## 1. 归属度量（可复跑）

| 列 | 含义 | 是否最终镜像 |
|---|---|---|
| `TAPE_PAYLOAD` | 对象内 `.unisa.tape` 载荷（`tl_tape`，不调 `tl_begin`） | 否 |
| `OBJECT_BYTES` | 容器文件长 | 否 |
| 单元 `IMAGE_BYTES` | 恒为 `UNATTRIBUTED` | — |
| `TOTAL.image_bytes` | 若给 `--image`，整文件长+sha | 整文件，不摊单元 |

不调 `tl_begin`：其 `sy_rlimit_as` 在 `-run` 工具宿主未导出。合并 emit 仍需 sidecar，不得用载荷冒称镜像。
依据 `plans/link-map-sidecar.md`：prune/lower/elf 丢来源边；无 sidecar 不得把 tape 区间冒称镜像贡献。工具把该事实打成 `image_attr=UNATTRIBUTED`。

校验：`sh ./unisacc.com tools/check-image-contrib.cx` → `IMAGE_CONTRIB_CHECK failed=0`。

## 2. 可迁性分类（代码事实，非直觉）

标签由源码扫描（`--src`），不是对象体积：

| CLASS | 条件（`tools/image-contrib.cx` `class_of`） | 对应主人四类 |
|---|---|---|
| TRANSFORM | 无 IO/getenv/sy_、无 abs、static 少、dispatch&lt;4、loop≤4 | 纯变换 |
| FINITE | dispatch≥4 且无 IO | 有限状态 |
| UNBOUNDED | static/abs/过多循环，或缺源 | 需无界状态 |
| SYS-IO | `puts(`/`getenv(`/`sy_`/`open` 等 | 系统 IO |

无 `--src` 的单元记 UNBOUNDED（缺证据，不猜 PURE）。

夹具事实：`pure.cx`→TRANSFORM；`io.cx`→SYS-IO。

产品侧已有机械标签（`slice-inventory` PURE/TABLE/MIXED/SYSCALL/EXECUTOR）是另一套规则；本表四类是瘦身合同。两套不得混报为同一度量。

## 3. 试点：`cx/compiler/asmtext.cx`（fable 首选；评审前不动）

netname 不作首试点。对照 enc 网、gold 往返见 [asmtext-nn-pilot.md](asmtext-nn-pilot.md)。

RT-DEDUP/libc 副本与迁 NN 无关，同候选实测见 [rt-dedup-recover.md](rt-dedup-recover.md)。


## 4. 明确不做

- 不实施 `link-map-sidecar` 产品接口。
- 不改 `cx/driver` loader（TEST-LOADER-COM / cdx）。
- 不把 `TAPE_EMIT` 写成可回收镜像 KB。
- BOUNDED-WAIT 暂缓。
