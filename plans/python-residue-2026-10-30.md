# 活路径上剩下的 Python（2026-10-30 盘点）

目标：确认 py 已全部迁到 cx，或者有不能迁的明确理由。只盘仍在活路径上的；`archive/` 不算。

盘点方法分两步：
- `git ls-files '*.py'` 去掉 `archive/`，剩 2 个文件。
- 在 `.cx/.sh/.c/.yml/Makefile` 里找真正执行 Python 的写法：argv 里有 `"python3"`、`python3 ...` 命令行、`| python -`、heredoc。只在注释里提到 python 的不算。

| # | 位置 | 做什么 | 现状 | 处置 |
|---|---|---|---|---|
| 1 | `.github/workflows/defender-scan.yml` 原来调 `release/tools/draftfetch.py` | 从草稿 release 取回并核对 `unisacc.com` | **已迁（19fab7b70f）**：改用 `gh api`、`release/tools/draftcheck.jq`、`Get-FileHash`；`draftfetch.py` 已归档 | 首次 CI 实跑之前没有云端证据 |
| 2 | `.github/workflows/windows-signing.yml`：两段 `\| python -`（约 150 行），并 import `release/apeinspect.py` | 签名前核对 seal 与 release 身份，解析 zip，检查 APE/PE 证书 | **已迁成 CX（db8048c8b3）**。主人定的口径是迁成 cx，所以这里原来写的"改用 PowerShell 重设计"作废，改为原样移植：<br>- `release/ci/seal-bind.cx`、`release/ci/validate-manifest.cx`、`release/ci/json.cx`、`release/ci/apecore.cx`；<br>- workflow 改为调用 `./unisacc.com release/ci/*.cx`，并开启 `core.symlinks`；<br>- `apeinspect.py` 已归档。<br>本地以 Python 为对照：validate-manifest 24 例一致，seal-bind 38 例中 37 例一致（另 1 例只是 Python 版本措辞不同），apeinspect 8 个文件一致；cc 抽查 `unisacc.com` 也逐字节相同 | **CI 还没实跑，而且照现在的样子跑不起来**：`unisacc.com` 被 `.gitignore` 忽略，检出里没有这个文件（`ci.yml` 也有同样问题）。在 CI 能跑之前，要先定由哪一步提供 `unisacc.com`，例如按 sha 从固定的 release 资产下载，这一步需要主人定。只有在 Windows runner 上才能证明的：`.com` 作为 PE 运行 `.cx`、软链解析、PATH 上能找到 git 和 gh |
| 3 | `apps/csih/deepseek-proxy.py` | 明文 HTTP 转到 api.deepseek.com 的中转 | **已迁（b3bf45395b，csih 车道）**：改写为 `apps/csih/deepseek-proxy.cx`，`.py` 已归档到 `archive/reference-py/apps/csih/`；仓外 `moltbaby/bin/rawcsih` 已改为编译并拉起 cx 版（cc 看过它第 95–98 行） | — |
| 4 | `adapter/ujswasm/tables.cx` 的 `--write`/`--check` 跑 `python3 -m ujs build-weights` | 生成 ujswasm 的 10 个 `*.net.tsv` | **已迁（9eed33fe1b）**：网表改由 `iterate/construct/construct.c` 生成，它就是 `build_net` 的 C 移植，由 `tables.cx` 经子进程默认 -run 调用。10 个 `.net.tsv` 逐字节重建成功，`--write` 21/21 与提交文件相同，`--check` rc=0（cc 已复跑）；改动 1 个字节后 `--check` 报出差异 | `.net.tsv` 文件头里保留着 "python3 -m ujs build-weights" 的字样，因为文件字节是冻结的，这只是说明来源，不会执行 |
| 5 | `exec/c/buildcompiler.sh` 调 `exec/c/tbl.py`、`net.py`、`exec/build/gen.py`、`exec/c/provenance.py` | 旧 Makefile 构造入口 | **已归档（7556aa351a）**：本来就跑不起来，调的 .py 和旧 C 都已归档 | 现在位于 `archive/reference-py/exec/c/buildcompiler.sh`；Makefile 删掉了 `com`、`model-com`、`comboot-*` |
| 6 | `release/tools/precheck.sh` 调 `tests/bound.py`、`tests/ledgercheck.py`、`exec/facts/export.py`、`tests/subtractsafety.py` | 旧发布前检查 | **已经跑不起来**：调用的都已归档 | 归档，并更新 `release/RELEASE-PIPELINE.md` 里的引用 |
| 7 | `release/tools/seal_candidate.sh`：内联 `python3 -c`，外加 `python3 - <<PY` | 把旧链产出的 `unisacc-next.com` 推到 GHCR，写 `release/candidate.json` | **已归档（7556aa351a）**：输入是旧 `product-build` 的 generation 目录，现在位于 `archive/reference-py/release/tools/seal_candidate.sh` | 新链的发布形态定下来后，用 jq 和 sha256sum 重新写一份，不带 Python |
| 8 | `ujs/scripts/release-artifacts.sh`（`python3 -m ujs`）、`ujs/scripts/uxe-gate.sh`（`python3 -m http.server`） | ujs 子项目的发布产物；本地静态服务 | **不在本目标范围**：`ujs/` 是论文 B 的独立子项目，主人定不碰。cc 先前在 b230ade92f、d8f87c469c 等提交里做的改动（`.mjs`、`.cx` 脚本、package.json）已在 ab1c743ba0 全部撤回，恢复到 b230ade92f 之前逐字节相同 | — |
| — | `research/**/*.sh` 里的 python | 历史研究现场 | 不在活路径上，是证据 | 不动 |
| 9 | `kernel/` 41 个生成文件的溯源注释仍写着 "`python3 -m unisa emit-kernel` writes this file" | 文件头说明，不执行 Python | 这批文件当初是 Python 写的，之后没有用原生的 `tools/kernel-emit.cx` 重写过，所以 `kernel-emit --check . kernel` 41 个都报 DIFF。其中 40 个第一处差异在溯源注释；`unisa_headers.inc` 现在由 `tools/carried-header-export.cx` 写，和 `kernel-emit` 写出的内容不同，有两个写入者 | `kernel/` 是 `package-refresh` 的输入（源包刷新），一改包身份就变。放到下一次慢车道的"新 P3"一步：先定下 `unisa_headers.inc` 由哪个工具写，再用 `kernel-emit --write` 重写 `kernel/`，`--check` 41/41 SAME 之后再刷新包 |
| 10 | `ujs/package.json` 的 `build`、`demo` 脚本：`python3 -m ujs web-build`、`python3 -m http.server` | npm 脚本 | — | **不在本目标范围**（`ujs/` 独立子项目）。cc 对 `ujs/package.json` 的改动已在 ab1c743ba0 撤回 |

## 只有底层能 include（同日盘点）

`tests/cxinclude-boundary.tsv` 的白名单是 `include/`、`tools/cxcc-hostinc/`、`cx/sys/`，也就是底层。白名单外仍有 `#include` 的：

- `apps/csih/` 共 30 个文件（基线行 2 个，超基线 28 个）：需要的 socket、termios、signal、tty 等接口还没进 `cx/sys`。csih 车道。
- `src/host.cx` 原来有 3 个：`<sys/mman.h>`、`<dlfcn.h>`、`<fcntl.h>`。已经收进 `cx/sys/mem.cx` 和 `cx/sys/dl.cx`，`src/host.cx` 现在 0 个 include。open 的 flag 走前导。`tests/cxinclude.cx --check` 不再报这个文件。
- 两行旧基线：`exec/c/qprefixcheck.cx`（1 个）、`exec/parse2/librarycallsitecheck.cx`（2 个）。
