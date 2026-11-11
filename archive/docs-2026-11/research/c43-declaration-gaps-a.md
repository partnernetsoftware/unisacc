# c43 声明缺口补齐，批次 A（2026-10-14）

性质：实测读集（strace，干净导出 HEAD 与 578ec1cc / e021ab0 上的 gate 作业命令）+ 声明核对。经 `tests/refresh_gatedeps.py` 写入（声明写在脚本里，脚本会重新生成这些条目）。

| 套件 | 实测读集 | 补入声明 | 声明覆盖 | 说明 |
|---|---:|---|---|---|
| decision-ledger | 260 | `tests/decisionledger.allow`；树 `exec` | 260/260 覆盖 | 读集来自它的遍历：`exec/**/*.py`（rglob）与 exec 下的 manifest（glob）；`tests/decisionledger.allow` 被读 2638 次 |
| rowcov-lex | 553 | `tests/sourceflat.py` | 553/553 覆盖 | 仅缺一个文件 |
| k2b | 5138 | `tests/k2bcheck.py` | **不覆盖（设计如此）** | k2b 对全部被跟踪文件做身份摘要（`git ls-files -co`），读集就是整棵树，无法用声明收窄；见 `research/c43-k2b-enable-assessment.md` |
| com-tapedomain | 2 | `tests/bound`、`tests/bound.c` | 2/2 覆盖 | 门禁用 `tests/bound` 包住产品运行 |
| com-latebind | 2 | `tests/bound`、`tests/bound.c` | 2/2 覆盖 | 同上 |

## 关键文件计算证明（均在输入集内）

输入集 = `files` ∪ 树内 `git ls-files`。

- decision-ledger：`tests/decisionledger.allow`、`exec/c/buildtarget.py`、`exec/assemble.py` 都在输入内。
- rowcov-lex：`tests/sourceflat.py`、`tests/rowcov.py` 在输入内。
- k2b：`tests/k2bcheck.py` 在输入内（整树未收窄）。
- com-tapedomain、com-latebind：`tests/bound`、`tests/bound.c`、各自的 `.cx` 在输入内。

## 声明变更核对

- `tests/gatedeps.json` 相对 HEAD：683 个套件数不变，无删除、无新增；变化的套件只有上表 5 个（com-latebind、com-tapedomain、decision-ledger、k2b、rowcov-lex）；`families` 与顶层键未变。
- `tests/refresh_gatedeps.py`：只改上表 4 个套件的生成代码（rowcov-lex 由 JSON 维护，不在脚本里）。

## 方法注记

- 直接手改 JSON 会被 `refresh_gatedeps.py` 覆盖（它重写这些条目）；因此声明写进脚本，再经脚本生成。
- 读集以每个作业自己的命令实测，限时用 `tests/bound.py`；全部在超时前结束（decision-ledger 1 s，rowcov-lex 40 s 内，k2b 33 s，com-* 5 s 内）。
