# compilercheck reviewed_trees：exec 快照为何在 1605288c 就不一致（只读）

快照由 `1605288c`（2026-09-28，“share reviewed driver-family inputs”）写入 `tests/gatedeps.json` 的 `families.compilercheck.reviewed_trees`。

## 1. 复算与口径（实际跑过）

在 `1605288c` 的干净导出上，按 `tests/gatequeue.py` 的规则重算：树内 `git ls-files --cached --others --exclude-standard` 的文件，后缀属于 `inventory_suffixes`，内容为 `[模式, sha256]`，再对 JSON（sort_keys）取 sha256。

| 树 | 文件数 | 与快照 |
|---|---|---|
| src | 9 | 一致 |
| unisa | 44 | 一致 |
| exec | 916 | **不一致**（快照 dac04d45…） |

同一规则对 src、unisa 完全吻合，所以**口径本身没有问题**，是 exec 的内容或文件集对不上。

对 exec 试过的其它口径，全部不匹配：

- 含非后缀文件（985 个 tracked 或 987 个磁盘文件）；
- 只取 tracked 文件；
- 加上当前工作区里 `exec/build` 的生成文件（921 个）。

## 2. 穷举已提交状态（实际跑过）

- 对 1605288c 及其之前所有改动过 exec 的提交（550 个），逐个按同一规则重算 exec：**无一匹配**。
- 对全部分支头、stash（4 个）与悬空提交（共 331 个 ref）：**无一匹配**（7 个无 exec 树，记为 ERR）。

结论：dac04d45 不对应 main、任何分支或 stash 中的 exec 已提交状态。

## 3. 判断（读源码 + 上面的实测，未能完全确证）

- 不是“录入口径不同”：同一规则对 src、unisa 吻合。
- 最可能是**脏树录入**：录入时 exec 工作区含未提交或未跟踪文件，其内容此后未以同样字节进入任何提交。仅凭 git 无法还原那份文件集，因此无法确证是哪些文件。
- 对照：src 在 1605288c 一致，之后被 314cc04d（我的提交，改 src/front_parse.c）打破；unisa 在 1605288c 一致，之后在 314cc04d^ 之前已变。

## 4. 1605288c 之后的改动（供审阅）

命令：`git log --format='%h %ad %s' --date=short --stat 1605288c..HEAD -- exec src unisa`

- exec：751 个提交，1008 个文件（新增与修改合计）；
- src：182 个提交，16 个文件（8 新增、8 修改）；
- unisa：61 个提交，31 个文件（9 新增、22 修改）；
- 三者合并为 912 个提交。全文过长，不在本页列出。

## 5. 设计建议（未实施，需你定）

1. 刷新快照前，必须先确定 dac04d45 的来源。在 git 之外无法恢复，只能由录入者确认。
2. 可行的做法：刷新为 HEAD 清单，并在 `reviewed_trees` 旁加 `reviewed_commit`（录入时的提交号），使快照可由 git 复算；否则下一次录入同样可能脏。
3. 断言（`README-real-reviewed-snapshot`）不改。
