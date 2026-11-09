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

## 实现备注（工具线交付，未实测）

- `tools/aw.cx` 提供 `ledger/items/check/migrate`，子命令后统一支持 `--root ROOT`（默认当前目录）；只读视图直接stdout，不写out、不回读旧表伪装迁移完成。
- 六列规范账本按日期月份及第六列安全owner拆分，不猜测作者；5/7列和其他不能安全推断的历史行进入严格的 `ledger/legacy.tsv`。容器严格双列表头 `aw-legacy-v1-hex<TAB>newline`，每行严格两列：原行小写十六进制及行尾换行存在位，读侧可逆还原。重复事件保留，正常分片仍拒绝错误列数/日期/ID/键。
- `check` 比较旧表与分片逐行多重集（含行尾位），旧文件缺失只结构检查并明确 `equivalence=unavailable`；不伪造迁移验收。Git冲突仅按物理行起点标记识别，字段内标记文字不误拒；诊断只含相对文件及双方安全ID/state/owner，不回显上下文/检查字段或URL查询，diff3基段不冒成一方，仍非零退出。pure新增真实CLI的合法标记及独立stderr隐私断言，未由本工具线运行。
- 迁移先校验，私有 `.aw-migrate-stage/` 构造/fsync/等价验收，收据记录schema、行数及发布边界；然后两个目录分别rename。不是跨目录原子事务，部分完成保留现场并拒绝重入。所有既有目标（含空目录）及暂存目录拒绝覆盖，不删旧文件，不改25处引用，无写者共享manifest。
- `check-aw.cx -- pure|parallel|conflict` 三模式分别55秒总预算，真实编译原生aw到唯一仓外夹具；parallel使用两个独立main克隆和两个进程，各20事件及不同项的真实提交合并；conflict只允许同ID文件冲突并核对双方状态。具体命令见tools/README.md。父统一登记与实测，本工具线未运行编译、检查、真实迁移、提交或推送。真实迁移/删旧/引用切换仍仅cc静默窗口负责。
- 所有夹具迁移均显式传唯一合成 `--root`；pure另测未知迁移参数严格拒绝。各模式启动前仅在夹具内保存源码仓旧两表快照，并记录源码仓 `agentwork/ledger`、`agentwork/items` 存在性和身份；成功结束及失败报告均核对旧表原字节、原有目录身份或原先不存在的目录仍不存在，避免只比较旧表而漏掉误建新分片。默认ledger/items成功输出仅TSV。
