# c43 名单外读取审计 批次 05（gate 作业，干净导出，2026-10-14）

性质：**实测**（strace `openat`，每个作业自己的命令，串行，每个 ≤55 s）。干净导出 = `git archive 1d9cb115`。名单 = `gatequeue.py` 的全局名单（`git ls-tree` 同样前缀）。只写实测读集，不改登记文件。

## 结果

| 作业 | rc | 用时 | 读集 | 名单外 | 是否已声明（gatedeps） |
|---|---:|---:|---:|---|---|
| tools-1 | 0 | 12.5 s | 28 | 12：`corpus/crypto-algorithms/*` | 未声明（全局输入） |
| tools-2 | 0 | 46.8 s | 28 | 12：`corpus/crypto-algorithms/*` | 未声明（全局输入） |
| tools-3 | 0 | 8.5 s | 25 | 9：`corpus/crypto-algorithms/*` | 未声明（全局输入） |
| decision-ledger | 0 | 1.7 s | 260 | 1：`research/decision-ledger.json` | 已声明 files |
| exittable | 0 | 0.2 s | 11 | 2：`archive/plans/v0.0.38.md`、`archive/plans/v0.0.42.md` | 已声明（files、trees: plans） |
| h1source | 0 | 1.4 s | 4 | 无 | — |
| precommithook | 0 | 1.5 s | 10 | 2：`.gitattributes`、`.gitignore` | **未声明** |
| seedpy | 0 | 0.1 s | 2 | 无 | — |
| subtract-safety-selftest | 0 | 0.1 s | 1 | 无 | 已声明 |
| k2b | 1 | 0.1 s | 1 | 无 | 已声明（trees） |

## 发现

1. **tools-1/2/3 读的是 git 忽略的克隆缓存 `corpus/`，不在名单里，也不在任何声明里。** `tests/tools.sh` 用 `CACHE=${TOOLS:-$R/corpus}`，三个仓库（crypto-algorithms、tiny-AES-c、tiny-regex-c）按 `REPOS` 表里**固定的 commit** 检出。`.gitignore` 第 13 行忽略 `/corpus/`。
   - 干净导出里没有 `corpus/`，本次运行时从 GitHub 克隆（46.8 s 中有网络成分）。
   - **缓存存在即跳过**（`[ -d "$CACHE/$name" ] && continue`），不核对它是否在固定 commit 上。于是陈旧的缓存会被静默使用，而 gate 不会因此变红。这是可复现性漏洞，需要 owner 判断是否改成检查 HEAD 与 pin。
   - 结论：tools 作业的结果依赖一个未哈希的外部缓存；名单外读取是真的。
2. **precommithook 读 `.gitattributes`、`.gitignore`，且该套件没有 gatedeps 声明。** 改这两个文件不会使它失效。范围很小（scratch 仓库的测试），但按规则应补声明或记录为全局输入。
3. **k2b 在 0.1 s 内 rc=1，只读了 1 个文件。** 命令用 `K2B_UA=${UA:-/tmp/ua_ref}`，我未设 `UA`，所以这是**调用方式的失败，不是读集的测量**。要测它需要带 UA 重跑；本批不下结论。
4. decision-ledger、exittable、subtract-safety-selftest 的名单外读取都在已声明的 files 或 trees 里，**已覆盖**，不算漏洞。

## 未测（本批跳过）

- `com-tapedomain`、`com-latebind`、`com-tools`：命令依赖 `$PRODUCT`（产品二进制），需要先建好产品，留到产品就绪后测。
- `rowcov-*`：下一批（batch06–08），表里原先标 `research/`，实测待查（已知是 static 误报的先例）。

## 对登记的影响

本笔不改登记文件。若要把 tools 的 corpus 与 precommithook 补声明，需要登记窗（gatedeps.json），等 cc 给窗。
