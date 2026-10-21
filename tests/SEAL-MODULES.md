# 封存范式模块（出货闸）

两种参数化检查器，对应 [prd.md](../prd.md) §3 的两种范式；**不是**第三种出货形态。

| 范式 | 检查器 | 叶配置 | 闸登记 |
|------|--------|--------|--------|
| **网格**（一网一封，金表全域） | `tests/netseal.cx` | `tests/seals/exec-net-<网>/net.tsv` | `net-<网>-seal`（17 网；**无 pp**） |
| **流格**（image-stages 一行，上游当夹具） | `tests/streamseal.cx` | `tests/seals/<叶>/cell.tsv` | `lex-e1-seal`、`opt-e4-seal`、`prune-seal`、`elf-seal` |

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
| e4 | `exec-opt-e4` | opt-e4-seal |
| prune | `exec-prune` | prune-seal |
| elf | `exec-elf` | elf-seal |

## 暂不合 streamseal 形状（故意不迁）

| stage | 现状 | 原因 |
|-------|------|------|
| **e2** | `exec-pp-literals` + `ppleafseal.cx` | 资源/双 model 叶；STALE 与 pp 头资源；非「单 net + exec/c/run」模板 |
| **e3** | `stagecontract-e1-e3` 等 | 阶段合同/库夹具，**不是**测量叶；待 tokens→tape 同名叶封再挂 cell |
| **lower** | `stagecontract-tape-lower` 等 | 同上；待 tape→target.text 测量叶 |
| **pp** | 无 `exec-net-pp` | **永远不是网格**；流侧也不走 streamseal 四叶模板 |

e3 八网、对照臂 isel/combo：**仍是网格**（`netseal`），不是流格第三范式。

## netseal 接口（简述）

```
netseal.cx --gate DATA ROOT     # DATA = tests/seals/exec-net-<网>/
netseal.cx --produce WORK ROOT NET
netseal.cx --capture ...
```

网名来自叶内 `net.tsv`；金表 `weights/gold/<网>.tsv` 在 check 时重派生全域 case。

## 历史

2026-10-21：四份近副本 `lexseal/optseal/pruneseal/elfseal.cx` 已删除；行为由 `streamseal.cx` + 各 `cell.tsv` 覆盖。过程笔记：`/home/box/c43/csr-paradigm-mod-1801/`。
