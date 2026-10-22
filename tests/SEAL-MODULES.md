# 封存范式模块（出货闸）

两种参数化检查器，对应 [prd.md](../prd.md) §3 的两种范式；**不是**第三种出货形态。

| 范式 | 检查器 | 叶配置 | 闸登记 |
|------|--------|--------|--------|
| **网格**（一网一封，金表全域） | `tests/netseal.cx` | `tests/seals/exec-net-<网>/net.tsv` | `net-<网>-seal`（17 网；**无 pp**） |
| **流格**（image-stages 一行，上游当夹具） | `tests/streamseal.cx` | `tests/seals/<叶>/cell.tsv` | `lex-e1-seal`、`e3-seal`、`opt-e4-seal`、`prune-seal`、`lower-seal`、`elf-seal` |

## streamseal 接口

```
streamseal.cx --check-core DATA ROOT
streamseal.cx --negative   DATA ROOT
streamseal.cx --gate       DATA ROOT
streamseal.cx --write-seal DATA ROOT    # reseal：写 identity.tsv（measurement.tsv 由叶目录提供）
streamseal.cx --capture    WORK ROOT DATA   # reseal：读 WORK/<capture_net>、cases/*
```

`cell.tsv` 关键字段：`stage`、`input`/`output`、`accepts`/`refusals`、`capture_net`、`omit_rule_prefix`、`negative_dep`/`negative_fixture`、`identity_core_row`；prune 另 `referee_note`，elf 另 `target`/`encode_note`。

Reseal：`tests/reseal-{lex-e1,opt-e4,prune,elf}.sh` 在 capture 前拷 `cell.tsv` + `measurement.tsv` 到 `OUTDIR/capture/`。

## 已挂 streamseal 的流叶

| stage | 叶目录 | gate job |
|-------|--------|----------|
| e1 | `exec-lex-e1` | lex-e1-seal |
| e3 | `exec-e3` | e3-seal |
| e4 | `exec-opt-e4` | opt-e4-seal |
| prune | `exec-prune` | prune-seal |
| lower | `exec-lower` | lower-seal |
| elf | `exec-elf` | elf-seal |

## 暂不合 streamseal 形状（故意不迁）

| stage | 现状 | 原因 |
|-------|------|------|
| **e2** | `exec-pp-literals` + `ppleafseal.cx` | 资源/双 model 叶；STALE 与 pp 头资源；非「单 net + exec/c/run」模板 |
| **e3** | （已迁 `exec-e3`） | 阶段合同 `stagecontract-e1-e3` 等仍并存；流格测量叶见 `exec-e3` |
| **lower** | （已迁 `exec-lower`） | 阶段合同 `stagecontract-tape-lower` 等仍并存 |
| **pp** | 无 `exec-net-pp` | **永远不是网格**；流侧也不走 streamseal 四叶模板 |

e3 八网、对照臂 isel/combo：**仍是网格**（`netseal`），不是流格第三范式。

## netseal 接口（简述）

```
netseal.cx --gate DATA ROOT     # DATA = tests/seals/exec-net-<网>/
netseal.cx --produce WORK ROOT NET
netseal.cx --capture ...
```

网名来自叶内 `net.tsv`；金表 `weights/gold/<网>.tsv` 在 check 时重派生全域 case。

## 独立测套（可单跑）

| 入口 | 作用 |
|------|------|
| `./tests/seal-modules.sh --check-only` | **出货闸 `seal-modules` job**：manifest + xy + edges 结构（默认快路径） |
| `./tests/seal-modules.sh` | 同上检查 + 可选 6×`streamseal` + 17×`netseal` 闸回放（开发用；全闸回放走单叶 `*-seal` job） |
| `tests/seal-modules.manifest.tsv` | 流叶/网叶登记（与上表闸 job 对齐） |
| `tests/sealparadigm.cx --audit ROOT` | 范式审计真源（`seal-modules --check-only` 与闸 `seal-paradigm-audit`） |
| `python3 tests/sealmodulescheck.py --check` | LEGACY shim -> `sealparadigm.cx --audit`（完整实现见 `archive/tests/legacy/sealmodulescheck.py`） |
| `tests/edgeablate.cx --gate LEAF ROOT` | 边级抽样消融见证（`ablate.tsv` + `asks.tsv` 交叉校验；e4/lower 走 edgeseal 参考路线）；**仅**闸 `edge-e2-pp-ablate`、`edge-e3-parse-ablate`（其余 `ablate.tsv` 为草稿，不入闸） |
| `tests/edgeseal.cx --gate LEAF ROOT` | 边级键答映射回放（15 主边 `edge-*-seal` 热路径）；`--produce` 仍走 `archive/tests/legacy/edgeseal.py` |
| `tests/seal-edges.tsv` | 边级登记（prd §3.0c(3) 主边全集 + 次边 + 对照臂）：`asked_evidence`=「问过」比对闸 job，`edge_frozen`=边级冻结原件路径（现全 none，0/17）；`--edges-only` 单查 |

**边级口径**：比对闸（stages-1*–3*、stages-back）证「Python 问过则 C 必问过」，**不是**按探针冻结的边级封存原件；原件落成后把该边 `edge_frozen` 从 `none` 改为叶目录，检查器计数即前进。主边全集硬编码在 `sealparadigm.cx`（与 `sealmodulescheck.py` 同表），表与 prd 任何一边改动而另一边没改都会红。

**边级原件形态**（键答映射闸 `tests/edgeseal.cx`；设计依据：脱敏设计简报的推荐一「规范化键答映射叶」，实测收窄：每边 distinct 键仅 3–84，全序列双跑确定故 refcount 入判等）：叶 `tests/seals/edge-<流>-<网>/` = `manifest.tsv`（探针/金表/叶体三重 sha 钉扎）+ `asks.tsv`（排序去重的 key→answer→refcount，参考路线分相位归边）。`--produce` 双录判等才写（拒空边、拒不确定）；`--gate` 回放逐字节判等 + 空集即败 + 三类负例（删行/改答案/改键）在闸内必红；金表演进时 `table_sha256` 钉扎主动红，不静默过期。证明力是组合推理：边原件（边消费哪些键）× stages/stages-back（问过）× oracle-cache（DENSE≡weights）× net-*-seal（weights≡gold）——原件冻的是参考端边使用面，不直接观测生产端逐问。首叶 `edge-e4-peep`（闸 `edge-e4-peep-seal`，探针 fib.c，8 键）。

**产品侧见证件**（`tests/edgewitness.sh`；两条参考侧录不到的边）：e3→pfconv 的 ask 点在 `src/front_parse.c` 的内建 printf 降级（`do_printf`），自 991d3373（2026-09-26）起任何 `printf(` 都 autoinc stdio.h、走库调用，**唯 `-nostdinc` 可达**（探针全 9 键，asked 9 实测）；参考侧 `PFCONVS` 是死 import、永不问，stages 存在性比对对它空真（边表 asked_evidence 已改记 `-`）。elf→enc：参考路线 enc 问全在 lower 相位（◐ 边已由键答映射件冻），elf 流 enc ask 产品侧独有（`unisa/image/` 不问 oracle）。见证件冻（探针 sha + 该网 asked 计数 + 程序 stdout 字节 + 退出码，产品双观测判等才写）；答案面由 net-*-seal 金表全域兜底，行为字节钉「答案被用」。**强度弱于键答映射件**（不枚举键；pfconv 件经 9 键探针例外地全枚举），分类计数不混同。未改产品代码——设计报告点名的 T3 冲突（产品加 trace）以冻结既有可见面绕开。

出货闸仍可逐 job 跑（`lex-e1-seal`、`net-*-seal`）；本测套是**聚合入口**，不替代单 job，也不读 middle-flow ledger。

## 历史

2026-10-21：四份近副本 `lexseal/optseal/pruneseal/elfseal.cx` 已删除；行为由 `streamseal.cx` + 各 `cell.tsv` 覆盖。过程笔记：`/home/box/c43/csr-paradigm-mod-1801/`。

2026-10-21：独立测套 `seal-modules.sh` + manifest；x流×y网适用性见 `sealmodulescheck.py`（4 流挂 streamseal、17 网挂 netseal、pp/e2/e3/lower 记 shape-gap）。过程笔记：`/home/box/c43/csr-suite-xy-1936/`。

2026-10-21：边级登记表 `seal-edges.tsv` 入套（17 出货边、冻结原件 0/17 起步计数；负例实测三类破坏均红）。

2026-10-21：首个边级冻结原件 `edge-e4-peep` 落闸（`edgeseal.py`，1/17）；pfconv 零见证探针记为已知空洞。

2026-10-21：批量铺满参考路线可达的全部边原件（+14，合计 **15/17**；探针 fib.c，各叶双录判等）；余 e3→pfconv、elf→enc 两条见「边级已知空洞」。

2026-10-21：余两边以产品侧见证件收口（`edgewitness.sh`，闸 `edge-e3-pfconv-witness`、`edge-elf-enc-witness`），合计 **17/17 = 15 键答映射 + 2 产品侧见证**。pfconv 可达性考古：`-nostdinc` + 字面量 printf。
