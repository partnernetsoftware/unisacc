# agentwork 分片设计(cc 2026-11-09,取代 EFF-049-5 的「merge=union」评估)

## 症状(本轮实测)
- 2026-11-01 起 `agentwork/ledger.tsv` 被 51 个提交改、`items.tsv` 47 个;每次集成必冲突,我已手工三方合并 ≥3 次。
- 根因:单文件 = 单一冲突域。窗口越多冲突越多,与行内容无关。

## 原则
**每个写者只写自己的文件。** 冲突域 = (写者, 键);不同窗口永不同写一个文件。汇总是读侧生成的视图,不入库。

## 布局
| 数据 | 旧 | 新 |
|---|---|---|
| 账本 | `ledger.tsv`(505 行) | `agentwork/ledger/<YYYY-MM>/<owner>.tsv`(每窗口每月一个,只追加) |
| 看板 | `items.tsv`(230 行,11 列) | `agentwork/items/<ID>.tsv`(一项一文件,单行 + 表头同旧;owner 改自己的项,状态流转由 owner 写) |
| 其余小表 | pkg-gaps/phase-times/units | 先不动(各 12/34/33 行,近期冲突少) |

## 读侧
- `tools/aw.cx ledger [--owner X] [--since D]`、`aw.cx items [--state S] [--owner X]`:合并分片、按键/时间排序输出旧格式 TSV。
- 25 处旧路径引用(tools 6 个、README/prd/plans 等)改为调用 `aw.cx` 或读 `aw.cx` 生成的 `out/aw/` 视图(不入库)。
- `aw.cx check`:每分片列数、ID 与文件名一致、无重复 ID、旧文件与分片并集逐行等价(迁移验收)。

## 迁移(一次性,集成者做,窗口静默 ≤10 分钟)
1. 宣布静默窗口;2. `aw.cx migrate` 把旧文件无损拆分(ledger 按 owner 字段/作者推断,拆不出的进 `legacy.tsv`);3. `aw.cx check` 证并集 == 旧文件;4. 旧文件删除、引用改写同一提交;5. 全窗口收到新写法信封。
回滚:`git revert` 该提交,旧文件原样复原。

## 验收
- 两个窗口并发各追加 20 条账本、各更新不同项,集成后零冲突(夹具实测)。
- 同一项两窗口同时改 → 冲突只落在该一个文件,且 `aw.cx` 给出可读的双方状态。
- precommit 对分片目录不受冻结闸影响(`agentwork/` 永不冻结)。

## 排序与负责
最高冲突源先行:先账本,后看板。归属 omp-cdx2(工具线)。验收不含全批资格声明。
