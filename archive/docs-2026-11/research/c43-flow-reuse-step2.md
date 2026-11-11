# 0.0.43 第 6 步第三刀：批量分类、按真实读集重算、未声明作业的补声明草案（只读）

标签：**[实际跑过]** = `strace -f -e openat` 对单个作业运行（≤55 秒，`/tmp/c43-tbprobe/trace_job.py`，结果见 §2）与 `tests/affected.py` 的回放；**[读源码]** = 静态分类规则（`tests/affected.py: classify_jobs`）。
不改队列与门禁；`research/c43-undeclared-draft.tsv` 只是附表，不进 gatedeps。

## 1. 批量分类（静态，全部 749 个闸作业）
`tests/gate.sh --plan --com` 给出每个作业的命令（当前计数 749；上一刀是 748，之后他人新增了 1 个）。分类规则：命令或脚本正文点名 `MODEL_COM` / `PRODUCT` / `unisacc.com` 的记为“读候选”；其余（只用 `$UA`、`python3`、`cc`、`sh` 且脚本正文无产品引用）记为“只读表与参考”。

| 类别 | 静态计数（分类器） | 样本核实（strace） |
|---|---:|---|
| 确实读候选 | 252 | 1/1：`com-fb12-multi` 打开 `unisacc.com` 20 次 **[实际跑过]** |
| 只读表与参考 | 497 | 7/7 无候选字节读取：`exec-lexpos`、`exec-e4-2`、`lib-sig3-equality`、`exec-elfobject`、`exec-warnprep-1`、`exec-lex-sourcefacts`、`exec-f1-sidecar` **[实际跑过]** |
| 不确定 | 0（分类器归入上两类） | 2 个“构造缓存”型：`exec-multi-ua-n-reverse`、`exec-driver-language-2` 在 55 秒内超时（rc124），读的是构造出的 `models.pkg`（来自表与构造器，不是候选）；结论未完整 **[实际跑过]** |

结论：样本支持分类（静态对、样本无反例）；但**样本少**（10 个），且“只读表”类的真实读集远大于声明（见 §3），分类本身不足以放宽依赖。

## 2. 按真实读集重算的回放 [实际跑过]
`tests/affected.py --declared-reads`（只读表类作业保留 files/guard 理由，去掉 trees 与候选理由）：

| 改动 | 现行规则 | 假设：files-only（**不安全**） | 假设：去掉候选理由，保留 trees（较保守） |
|---|---:|---:|---:|
| 0571eb62 Tape 验域 | 738 | 371 | 730 |
| 03936e9f..ecdbd98d 晚绑定 | 736 | 369 | 607 |
| df1ce7ed 转发 include | 736 | 370 | 607 |
| 6420cfb3 tapebin（只 tests） | 173 | 171 | 173 |

数字含义：
- **files-only 不可用**：strace 显示只读表类的真实读集远超声明。例：`exec-warnprep-1` 打开 1383 个仓库文件，其中 1382 个不在其 `files` 中，13 个不在其 `trees` 中（如 `seed/ape.c`）；`exec-e4-2` 的 113 个读取（如 `exec/build/gen.py`、`exec/c/*`）不在其声明的 files 或 trees 中。files-only 会**漏验**，不能作为节省依据。
- **去掉候选理由**的节省是真实的，但仍不完整（同样的声明缺口）。03936e9f..ecdbd98d 与 df1ce7ed 节省 129 个作业，0571eb62 只节省 8 个（它只动了 `unisa/tape.py`，而这些作业大多不声明 unisa）。
- 与 pp 族（只省 6 个）相比，这里的节省来自其余阶段套件，但**其前提是声明完整**，目前不成立。

## 3. 未声明作业（159 个）的补声明办法
按前缀分组（`research/c43-undeclared-draft.tsv`，第一列为组）：`other` 78、`unisa-` 31、`exec-` 14、`com-` 10、`rowcov-` 4、`difftest` 4、其余 ≤3 个。按类别：只读表 115、读候选 44。

- **最大组 `other`（78）**：多为门禁基础设施与队列检查（`checkrun`、`commitgate`、`gate-layers`、`gatesummary`、`freezecheck`、`ledgercheck`、`envpin`、`exittable` 等），以及 cdx 的 `.cx` 检查（`abi-manifest`、`middle-flow-*`、`lifecycle` 等，多数用 `./unisacc.com -run`）。
  - 最小办法：每个作业 `files = [脚本本身] + 脚本 import 的同目录模块`，`trees` 只给其脚本所在目录；门禁基础设施额外声明 `tests/gate.sh`、`tests/gateorder.json`、`tests/gatedeps.json`（它们确实被读取）。
  - 风险：基础设施脚本读 `gatedeps.json` 与 `gate.sh` 本身，声明必须包含它们，否则改门禁不会失效（漏验门禁）。
- **`unisa-`（31）与 `exec-`（14）**：多为只读表的 Python 检查，`files` 缺口最大（见 §2，warnprep、e4 的读集）。最小办法必须先做 strace 审计，再写声明；不能用脚本名推断。
- **`com-`（10）**：读候选；声明 `executable_inputs: true` 与 `MODEL_COM` 即可，风险低。

草案表只放脚本与命令，**不给出 trees**，因为真实读集未审计；这是有意的。

## 4. 风险与必须作废的条件
- 声明缺口：任何只靠声明的 affected 结果都可能漏验；合格的做法是“声明 + strace 审计的读集核对”，两者缺一不可。
- 同一作业的命令变了（例如 `--com` 与否、环境变量）必须重新分类（`classify_jobs` 依赖 `gate.sh --plan --com`）。
- 构造缓存型（`models.pkg`、`driver-*`）的作业：其依赖是构造器与表，不是候选，但构造缓存的键必须包含构造器哈希；这一点未审计。
- 本工具仍是日常挑选工具；发布整闸不变。

## 5. 建议（不执行）
1. 先对 `other` 组做补声明（风险低、数量大、收益是把门禁基础设施的改动定位清楚）。
2. 对 `exec-`/`unisa-` 组，先做批量 strace 审计（每个 ≤55 秒，串行），再写声明。
3. 在声明完整之前，`--declared-reads` 只能作探索，不能作为跳过依据。
