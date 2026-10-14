# c43 声明缺口补齐，批次 B（2026-10-14）

性质：实测读集（strace，干净导出 e021ab0 上的 gate 作业命令）+ 声明核对。exittable 声明写进 `tests/refresh_gatedeps.py`（生成条目）；precommithook 与 h1source 是 JSON 条目，由 `refresh_gatedeps.py` 保留。

| 套件 | 实测读集 | 补入声明 | 覆盖 | 说明 |
|---|---:|---|---|---|
| exittable | 11 | `release/preauth.tsv`、`release/rulings.tsv`、`release/tools/exittable.py`、`release/tools/h1source.py`、`src/version.h`、`tests/gatelayers.py`、`tests/gateorder.json`、`tests/gatequeue.py` | 11/11 | 打印的账目与门禁顺序都在这些文件里 |
| precommithook | 10 | `release/tools/checkrun.sh`、`release/tools/derivedcheck.py`、`tests/gatedeps.json` | 10/10 | 两个点文件 `.gitattributes`、`.gitignore` 已在批次 05/`bf5e27ba` 中声明 |
| h1source | 4 | 新条目：命令 `python3 ./tests/h1sourcecheck.py`；`release/tools/build_candidate.sh`、`release/tools/h1source.py`、`tests/bound`、`tests/h1sourcecheck.py` | 4/4 | 此前未声明（全局输入）；声明后仍是全局输入加声明文件，**不会因此变窄** |

## 关键文件计算证明（均在输入集内）

输入集 = `files` ∪ 树内 `git ls-files`。

- exittable：`release/tools/exittable.py`、`src/version.h`、`tests/gatequeue.py` 在输入内。
- precommithook：`release/tools/checkrun.sh`、`release/tools/derivedcheck.py`、`tests/gatedeps.json`、`.gitignore` 在输入内。
- h1source：`release/tools/build_candidate.sh`、`release/tools/h1source.py`、`tests/bound` 在输入内。

## 声明变更核对

- 相对 HEAD（cdbac0db）：套件数 683→684，只新增 `h1source`；变化的套件只有 `exittable`、`precommithook`；顶层键与 `families` 未变；无删除。

## 未补（与评估结论一致）

- **docs**：`docs.sh` 在 HEAD 上 rc=1，`python3 -m unisa docs --check` 抛 `IndexError`（`unisa/docgen.py` 的 `_acc` → `unisa/intnet.py` `predict`）。二分定位：`ea46504a` 通过，**首个失败提交是 `9645c6db`**（把 `.libraryfn` 加入 tape.SHAPE 的那笔）。docgen 的阶段网络按 SHAPE 建表，新增操作后索引越界。这是 HEAD 的真红，需要 owner 决定修法（重生成 docgen 的网络表，或让 docgen 容纳新操作）。在它修好之前，docs 的读集不完整，**不据此声明**。
- **ledgercheck**：已覆盖（5 个读，3 个在 `plans/` 下，已由该套件的 `plans` 树声明）；无需补。
