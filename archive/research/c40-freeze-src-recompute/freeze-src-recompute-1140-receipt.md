# 独立复算回执：compilercheck.reviewed_trees.src @ tip 6a5c3c8e（机房主任执行器 11:40）

## 锁 tree-ish
- **origin/main tip** `6a5c3c8e78d27fedd01ef8888c15fe9cddea5546`（= HEAD = ls-remote refs/heads/main）。
- 冻链：`e75d6e00`（version.h→0.0.40）+ `8b4e1e4f`（gatedeps refresh）；其后 research 冻回执+paper-a 至本 tip。
- `src/version.h` @ tip：`UNISACC_VERSION "0.0.40"`。
- 工作树 dirty 仅 `M exec/c/compiler.c`（.cx 旁路；**不**入本复算；`git archive HEAD src` 不受影响）。

## 算法
- 口径同 `tests/refresh_gatedeps.py`：
  1. `git -c tar.umask=022 archive --format=tar HEAD src` 解到临时目录（git 内容 + git 模式）。
  2. 成员：`src/**` 文件且 `suffix ∈ inventory_suffixes`（compilercheck：`.py .c .h .inc .tsv .json .sh .S`）；`src` ∉ `all_files_trees`（仅 `include`）；`excluded_dirs` 空。
  3. 每路径 `digest = [st_mode, sha256(content)]`。
  4. `stamp = sha256(json.dumps({relpath: digest}, sort_keys=True).encode())`。
- **不以**前缀 `35ab1353` 代全长；本回执给出 64 hex 全长。

## 读数
| 项 | 值 |
|---|---|
| tip | `6a5c3c8e78d27fedd01ef8888c15fe9cddea5546` |
| 键 | `families.compilercheck.reviewed_trees.src` |
| n_files | 17 |
| sha256 **全长** | **`35ab135379d27bc933628e70453c5c8502db547236bf7a550b70141393ad6f56`** |
| gatedeps 所记 | `35ab135379d27bc933628e70453c5c8502db547236bf7a550b70141393ad6f56` |
| 与目标 | **吻合** |
| unique modes | `[33188]`（= 100644） |

## 命令摘要
```
TIP=6a5c3c8e78d27fedd01ef8888c15fe9cddea5546
# Python：git archive HEAD src → digest map → stamp
# → 35ab135379d27bc933628e70453c5c8502db547236bf7a550b70141393ad6f56
# 与 tip tests/gatedeps.json families.compilercheck.reviewed_trees.src 全长相等
```

## 不宣称 / 未做
- **未改** `tests/gatedeps.json`（已是目标值）。
- 未 bump；未 Draft；未跑全量门；未把 queuecheck provenance 红计本轮新增；未宣称 .cx 产品绿。
- 本复算是**本机独立核验**；「未核 src 目录戳 → 不计通过」在本回执落盘后对本键 **解除**。
