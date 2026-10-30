# 活路径上剩下的 Python（2026-10-30 盘点）

目标：确认 py 已全部迁到 cx，或者有不能迁的明确理由。只盘仍在活路径上的；`archive/` 不算。

盘点方法分两步：
- `git ls-files '*.py'` 去掉 `archive/`，剩 2 个文件。
- 在 `.cx/.sh/.c/.yml/Makefile` 里找真正执行 Python 的写法：argv 里有 `"python3"`、`python3 ...` 命令行、`| python -`、heredoc。只在注释里提到 python 的不算。

| # | 位置 | 做什么 | 现状 | 处置 |
|---|---|---|---|---|
| 1 | `.github/workflows/defender-scan.yml` 原来调 `release/tools/draftfetch.py` | 从草稿 release 取回并核对 `unisacc.com` | **已迁（19fab7b70f）**：改用 `gh api`、`release/tools/draftcheck.jq`、`Get-FileHash`；`draftfetch.py` 已归档 | 首次 CI 实跑之前没有云端证据 |
| 2 | `.github/workflows/windows-signing.yml`：两段 `\| python -`（约 150 行），并 import `release/apeinspect.py` | 签名前核对 seal 与 release 身份，解析 zip，检查 APE/PE 证书 | 活。`release/apeinspect.cx` 已与 Python 输出一致，但 runner 上要检查的正是待签名的 `unisacc.com`，拿它跑 `.cx` 来检查自己是循环依赖。整条 workflow 都绑着旧 C 产品的发布流程：源闭包要遍历 `unisa/` 和 `exec/**/*.py`，上游是 `release-check.yml`，还要读 `seal_candidate.sh` 写的 `release/candidate.json` | **不按原样翻译。** 现在照搬就是把一套即将作废的规则移植一遍。跟 `plans/candidate-to-product-2026-10-30.md` 一起重新设计新产品的发布与签名收据，到时直接用 PowerShell 写（`apeinspect` 的 PE、证书、UNIPKG1 解析约 80 行，可用 `[IO.File]::ReadAllBytes` 加 `BitConverter` 实现），并在 CI 上实跑一次。届时归档 `apeinspect.py` |
| 3 | `apps/csih/deepseek-proxy.py` | 明文 HTTP 转到 api.deepseek.com 的中转 | 唯一调用方是仓外的 `moltbaby/bin/rawcsih` | rawcsih 改成直连 https 之后归档。csih 车道 |
| 4 | `adapter/ujswasm/tables.cx` 的 `--write`/`--check` 跑 `python3 -m ujs build-weights` | 生成 ujswasm 的 10 个 `*.net.tsv` | **已迁（9eed33fe1b）**：网表改由 `iterate/construct/construct.c` 生成，它就是 `build_net` 的 C 移植，由 `tables.cx` 经子进程默认 -run 调用。10 个 `.net.tsv` 逐字节重建成功，`--write` 21/21 与提交文件相同，`--check` rc=0（cc 已复跑）；改动 1 个字节后 `--check` 报出差异 | `.net.tsv` 文件头里保留着 "python3 -m ujs build-weights" 的字样，因为文件字节是冻结的，这只是说明来源，不会执行 |
| 5 | `exec/c/buildcompiler.sh` 调 `exec/c/tbl.py`、`net.py`、`exec/build/gen.py`、`exec/c/provenance.py` | 旧 Makefile 构造入口 | **已归档（7556aa351a）**：本来就跑不起来，调的 .py 和旧 C 都已归档 | 现在位于 `archive/reference-py/exec/c/buildcompiler.sh`；Makefile 删掉了 `com`、`model-com`、`comboot-*` |
| 6 | `release/tools/precheck.sh` 调 `tests/bound.py`、`tests/ledgercheck.py`、`exec/facts/export.py`、`tests/subtractsafety.py` | 旧发布前检查 | **已经跑不起来**：调用的都已归档 | 归档，并更新 `release/RELEASE-PIPELINE.md` 里的引用 |
| 7 | `release/tools/seal_candidate.sh`：内联 `python3 -c`，外加 `python3 - <<PY` | 把旧链产出的 `unisacc-next.com` 推到 GHCR，写 `release/candidate.json` | **已归档（7556aa351a）**：输入是旧 `product-build` 的 generation 目录，现在位于 `archive/reference-py/release/tools/seal_candidate.sh` | 新链的发布形态定下来后，用 jq 和 sha256sum 重新写一份，不带 Python |
| 8 | `ujs/scripts/release-artifacts.sh`（`python3 -m ujs`）、`ujs/scripts/uxe-gate.sh`（`python3 -m http.server`） | ujs 子项目的发布产物；本地静态服务 | **已迁（b230ade92f）**：版本读取、指纹核对、RELEASE.txt 三段改由 `ujs/scripts/release-check.mjs` 完成（对照旧 Python 跑过：RELEASE.txt 逐字节相同，指纹拒绝结果相同）；静态服务改用 `ujs/scripts/static-serve.mjs` | `FORCE_WEB_BUILD` 原来调已归档的 `python3 -m ujs web-build`，现在直接报出这个原因后退出。`ujs/scripts` 里已经没有 Python |
| — | `research/**/*.sh` 里的 python | 历史研究现场 | 不在活路径上，是证据 | 不动 |
| 9 | `kernel/` 41 个生成文件的溯源注释仍写着 "`python3 -m unisa emit-kernel` writes this file" | 文件头说明，不执行 Python | 这批文件当初是 Python 写的，之后没有用原生的 `tools/kernel-emit.cx` 重写过，所以 `kernel-emit --check . kernel` 41 个都报 DIFF。其中 40 个第一处差异在溯源注释；`unisa_headers.inc` 现在由 `tools/carried-header-export.cx` 写，和 `kernel-emit` 写出的内容不同，有两个写入者 | `kernel/` 是 `package-refresh` 的输入（源包刷新），一改包身份就变。放到下一次慢车道的"新 P3"一步：先定下 `unisa_headers.inc` 由哪个工具写，再用 `kernel-emit --write` 重写 `kernel/`，`--check` 41/41 SAME 之后再刷新包 |
| 10 | `ujs/package.json` 的 `build`、`demo` 脚本：`python3 -m ujs web-build`、`python3 -m http.server` | npm 脚本 | 还有 `test:game`、`test:uxe:packet`、`test:uxe:input` 三个，要跑的 `_*_selftest_run.py` 早已归档（`archive/ujs-legacy-py-c/`），原本只是给 node 测试套的计时外壳，所以 uxe-gate 之前根本跑不起来 | **已迁**：三个测试改为直接 node 跑 `.mjs`（packet、input 均 rc 0）；`demo` 改用 `static-serve.mjs`（200）；`build` 明确报出"Python 构建器已归档"后退出 1。`test:game` 失败与 Python 无关：`practice/web/core/wasm_run.js` 不存在，是 ujs 原有的布局问题。`ujs/package.json` 里已经没有 python |

## 只有底层能 include（同日盘点）

`tests/cxinclude-boundary.tsv` 的白名单是 `include/`、`tools/cxcc-hostinc/`、`cx/sys/`，也就是底层。白名单外仍有 `#include` 的：

- `apps/csih/` 共 30 个文件（基线行 2 个，超基线 28 个）：需要的 socket、termios、signal、tty 等接口还没进 `cx/sys`。csih 车道。
- `src/host.cx` 原来有 3 个：`<sys/mman.h>`、`<dlfcn.h>`、`<fcntl.h>`。已经收进 `cx/sys/mem.cx` 和 `cx/sys/dl.cx`，`src/host.cx` 现在 0 个 include。open 的 flag 走前导。`tests/cxinclude.cx --check` 不再报这个文件。
- 两行旧基线：`exec/c/qprefixcheck.cx`（1 个）、`exec/parse2/librarycallsitecheck.cx`（2 个）。
