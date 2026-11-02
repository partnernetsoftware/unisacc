# 独立复算回执：compilercheck.reviewed_trees.src @ tip 10c2e9b7（机房主任执行器 11:53）

## 锁 tree-ish
- **origin/main tip** `10c2e9b768f422f8646e930aace687321f528d4f`（= HEAD = ls-remote refs/heads/main）。
- 相对 1140 认键 tip `6a5c3c8e`：其后有 research 1140 ACK 入仓、paper-a merge、以及 `10c2e9b7` fix(run) .cx 续源列表；**`src/` 与 `tests/gatedeps.json` 相对 6a5c3c8e 无 diff**。
- `src/version.h` @ tip：`UNISACC_VERSION "0.0.40"`。
- 工作树：**干净**（无 dirty；git archive HEAD src 不受工作树影响）。

## 算法
- 口径同 `tests/refresh_gatedeps.py`（本机独立复算，未调用 refresh 写回）：
  1. `git -c tar.umask=022 archive --format=tar HEAD src` 解到临时目录（git 内容 + git 模式）。
  2. 成员：`src/**` 文件且 `suffix ∈ inventory_suffixes`（compilercheck：`.py .c .h .inc .tsv .json .sh .S`）；`src` ∉ `all_files_trees`（仅 `include`）；`excluded_dirs` 空。
  3. 每路径 `digest = [st_mode, sha256(content)]`。
  4. `stamp = sha256(json.dumps({relpath: digest}, sort_keys=True).encode())`。
- **不以**前缀 `35ab1353` 代全长；本回执给出 64 hex 全长。

## 读数
| 项 | 值 |
|---|---|
| tip | `10c2e9b768f422f8646e930aace687321f528d4f` |
| 键 | `families.compilercheck.reviewed_trees.src` |
| n_files | 17 |
| sha256 **全长** | **`35ab135379d27bc933628e70453c5c8502db547236bf7a550b70141393ad6f56`** |
| gatedeps 所记 | `35ab135379d27bc933628e70453c5c8502db547236bf7a550b70141393ad6f56` |
| 与目标 | **吻合** |
| unique modes | `[33188]`（= 100644） |

## 命令摘要
```
TIP=10c2e9b768f422f8646e930aace687321f528d4f
# Python：git archive HEAD src → digest map → stamp
# → 35ab135379d27bc933628e70453c5c8502db547236bf7a550b70141393ad6f56
# 与 tip tests/gatedeps.json families.compilercheck.reviewed_trees.src 全长相等
```

## 不宣称 / 未做
- **未改** `tests/gatedeps.json`（已是目标值）。
- 未 bump；未 Draft；未跑全量门；未打断首观察补证（1152）；未宣称 .cx 产品绿。
- 本复算是**本机独立核验**；「未核 src 目录戳 → 不计通过」在本回执落盘后对本键 **维持解除**（与 1140 ACK 同值，tip 前进后重核仍吻合）。
