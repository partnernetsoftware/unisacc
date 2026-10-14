# c43 名单外读取审计 批次 06（rowcov、k2b，及第 5 批的三项修补，2026-10-14）

性质：实测（干净导出 1d9cb115，strace `openat`，每个作业自己的命令，串行，每个 ≤55 s）。

## 一、第 5 批的修补

1. **tools 的 corpus 缓存（tests/tools.sh）。** 原来 `[ -d "$CACHE/$name" ] && continue` 对已存在的缓存不核对提交号。现在先 `git -C 缓存 rev-parse HEAD`，等于固定提交号才复用；不等则**明确失败**并提示删除缓存后重取（不擅自删除用户目录）。
   - 测试：`TOOLS=<假缓存，提交号不同>` 时 rc=1，输出点名了实际与固定提交号；主检出的 corpus 三个缓存均与固定提交号一致，`SHARD=1/3 ./tests/tools.sh` rc=0，4/4 通过。
   - 指纹：tools-1/2/3 未声明，走全局输入；`tests/tools.sh` 在全局名单中（`git ls-tree` 的 `tests` 前缀，已核），因此改固定提交号会改变这三个作业的指纹。缓存目录不进声明（gitignored，内容由固定提交号决定）。
2. **precommithook 声明（tests/gatedeps.json）。** 新增条目：命令 `./tests/precommithookcheck.sh`，文件：脚本本身、`tests/freezecheck.py`、`release/tools/commitgate.sh`、`release/tools/hooks/pre-commit`、`release/tools/install_hooks.sh`、`.gitattributes`、`.gitignore`。
   - 计算证明（同 gatequeue 的 per-file 摘要：内容 sha256）：在副本里对 `.gitignore` 追加一行，它的摘要变了；其余六个声明文件摘要不变。
   - `./tests/precommithookcheck.sh` 在主检出上 rc=0。
3. **k2b 补测。** 用 `tests/build_ref.sh` 在干净导出上构建 UA（11.8 s），再以 `K2B_UA`、`UA` 指向它运行 `tests/k2bcheck.py`：**rc=0**。注意：`k2bcheck.py` 调用 `git ls-files`，干净导出没有 `.git`，因此在导出里 `git init` 并 `git add -A`（只为建索引，不提交）。这是运行方式，不是产品问题。

## 二、批次 06 名单外读取

| 作业 | rc | 用时 | 读集 | 名单外 | 备注 |
|---|---:|---:|---:|---:|---|
| rowcov-pp-locations | 124 | 55.2 s | 341 | 0 | 超时，读集截断 |
| rowcov-pp-shared | 124 | 55.3 s | 466 | 0 | 超时，读集截断 |
| rowcov-lex | 0 | 39.5 s | 553 | 0 | 完整 |
| rowcov-parse2-build | 124 | 55.2 s | 1741 | 0 | 超时，读集截断 |
| rowcov-lower-build | 0 | 20.2 s | 1741 | 0 | 完整 |
| rowcov-enc-build | 0 | 15.0 s | 1741 | 0 | 完整 |
| rowcov-lower-union | 124 | 55.1 s | 1609 | 0 | 超时，读集截断 |
| rowcov-enc-union | 124 | 55.1 s | 1603 | 0 | 超时，读集截断 |
| rowcov-parse2-union | 124 | 55.1 s | 1741 | 0 | 超时，读集截断 |

结论：**在观测窗口内，批次 06 没有名单外读取。** 但 6 条是超时（rc=124）的，读集被截断；“0”只说明截断前没有名单外读取，不说明完整作业没有。此前表里 `rowcov-*` 标的 `research/` 在本次观测窗口内**没有**出现（与此前 static 误报的结论一致）。若要完整结论，需要单独的超时预算（不在本笔范围内，超时是 gate 的限制，不放宽）。

## 三、未完成

- 批次 07（`rowcov-parse2-1..24`）与批次 08（`rowcov-lower-1..8`、`rowcov-enc-1..8`）尚未测；它们每个 ≤55 s，且多数 shard 同样会在窗口内超时，留作下一笔。
- 批次 05 的 tools 名单外读取（`corpus/`）已由第一节的修补处理。
