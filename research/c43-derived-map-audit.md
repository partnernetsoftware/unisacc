# 派生映射全量体检（只读，2026-10-15，cc2）

范围：`release/derived-map.tsv`（139 行，14 个套件，全部已在 `tests/gatedeps.json` 声明）对 `tests/gatedeps.json` 中的指纹/封存/派生类套件的覆盖。本文不改映射；漏项列出后另一笔补（每条附历史回放）。

## 方法

- 读取集 = 套件在 `tests/gatedeps.json` 中声明的 `files` 与 `trees`（闸的 stamp 即按此计算；这是可核实的代理，不是逐行读源码）。
- 覆盖 = 存在一条 derived-map 行，其 glob 匹配该路径，且行的套件**就是该套件**（owner 覆盖）。只看“任一套件”会掩盖漏项，故以 owner 为准。
- 产品依赖 = 命令含 `{MODEL_COM}` 或以 `unisacc.com` 运行。按 cc 裁，只列为“只在整闸”。

## 结果（owner 覆盖）

| 套件 | 类别 | 声明 | owner 未覆盖 | 结论 |
|---|---|---:|---:|---|
| pp-leaf-seal | 运行器为本地产品 | 193 | 192 | 漏：封存数据 `tests/seals/exec-pp-literals/*`（case-*、streams 等）未映射。依赖闭包 93 项已覆盖（981bc90e） |
| pp-leaf-seal-negative | 同上 | 193 | 193 | 漏：`tests/ppleafseal.cx`（981bc90e 只映射给 pp-leaf-seal）及封存数据 |
| latebind-p6 | 产品依赖 | 313 | 313 | 只在整闸（`{MODEL_COM}`）；派生映射无行，属设计上的整闸项 |
| lib-latebind-W | 产品依赖 | 235 | 235 | 只在整闸 |
| seedmemory | 本地 | 1587 | 3 | 漏：`seed/gen.c`、`tests/seedmemory.py`、`tests/seedmemorycheck.py` |
| tapebin-shape | 本地 | 9 | 7 | 漏：`docs/tapebin-v1.records.tsv`、`docs/tapebin-v1.shape.tsv`、`exec/c/tapebin.h`、`src/tapebin_shape.inc` |
| reviewed-trees | 本地 | 1660 | 2 | 漏：`tests/gatedeps.json`、`tests/reviewedtreescheck.py`（检查器与其输入自身变更应触发） |
| inventory-derived | 本地 | 3086 | 0 | 覆盖 |
| abi-manifest | 本地（经 unisacc.com） | 1672 | 983 | 声明树 `exec` 比派生范围宽（exec/*.md、exec/*.py 等）；真实读取集须读 `abimanifestcheck.cx` 才能定，本轮未完成，列为待核 |
| exec-librarytypedreader-model、exec-librarytypedlower-model-*、exec-librarytypedpartition-model-* | 本地生成 | 647–1233 | 全部 | 这些套件由 gatequeue 按 stamp 失效（生成器与 seed 变即重算），不是派生文件的一致性检查。设计上不需 derived-map；记为“stamp 覆盖”，不计漏项。 |
| script-inventory | 整树 | 4606 | 1221 | 设计上整树扫描（清点所有脚本），不计漏项 |
| kernel、tapebin-roundtrip、lib-stack-x86-hostabi、exec-native-chain、com-tapebin-* | 本地 | 553–2221 | 68–131 | 声明含 examples/apps 等树，stamp 覆盖；派生一致性不适用，不计漏项 |

## 确认的漏项（需补映射，每条须附历史回放）

| 漏项 | 建议映射 | 历史回放候选（最近触及提交） |
|---|---|---|
| `seed/gen.c` | seedmemory | c9f4d792（fix: add opt-in fixed typed USLCON reader …） |
| `tests/seedmemory.py`、`tests/seedmemorycheck.py` | seedmemory | d51f2b10（fix: add reusable gated fixed-typed lower model witness） |
| `docs/tapebin-v1.records.tsv`、`docs/tapebin-v1.shape.tsv`、`src/tapebin_shape.inc` | tapebin-shape | c08a1c00（fix: reject underscore Tape integers …） |
| `exec/c/tapebin.h` | tapebin-shape | 981430f5（exec/c/tapebin.h: callm by name …） |
| `tests/seals/exec-pp-literals/*` | pp-leaf-seal、pp-leaf-seal-negative | fb504549（tests: reseal pp-leaf with relative source names …） |
| `tests/ppleafseal.cx` | pp-leaf-seal-negative | 981bc90e（当前提交）；回放须用其前一提交 |
| `tests/gatedeps.json`、`tests/reviewedtreescheck.py` | reviewed-trees | 981bc90e（gatedeps 改动） |

每条的回放形式：`python3 release/tools/derivedcheck.py --base <C>^ --over <C>`，期望输出包含对应套件且 rc 非 0（若该旧树本身红）或 rc 0（若是纯映射验证）。补映射后重跑同一命令，期望**新增**命中该套件。

## 未完成与边界

- `abi-manifest` 的真实读取集未核实（声明树过宽）：下一步读 `tests/abimanifestcheck.cx` 的输入表后再定映射。
- 整闸项（latebind-p6、lib-latebind-W）按 cc 裁只列出，不进派生映射。
- 本体检只用声明输入作代理；若某套件读取的文件不在其声明内，本表无法发现。这是方法边界。
- 漏项未在本笔修改；修复待 cc 登记窗后逐条补映射并回放。
