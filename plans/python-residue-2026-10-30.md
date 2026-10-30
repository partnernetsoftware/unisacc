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

## 自己工具里剩下的两个 .mjs（同日）

主人的口径：自己的工具里不要 `.mjs`/node/Python（`ujs/`、`apps/csih/`、`tests/ujs2wasm/`、`docs/uxe/` 是别的车道，不算）。在范围内的只剩这两个，都在 `.github/workflows/admission-capsule.yml`（`node release/capsule-export.mjs export`）里用：

| 原文件 | 做什么 | 现状 |
|---|---|---|
| `release/capsule-zip.mjs` | 严格 ZIP32 解析 `parseZip` | **已迁成 CX**：`release/ci/capsulezip.cx`（模块 `cz_parse`）。原文件已归档到 `archive/reference-js/release/capsule-zip.mjs` |
| `release/capsule-export.mjs` | 主人批准的私有 Draft 证据胶囊转运：读 GitHub API、严格校验、写 `$RUNNER_TEMP/unisacc-evidence-capsule-output/evidence.zip`、打印 JSON 回执 | **已迁成 CX**：`release/capsule-export.cx`，子命令 `export`（CI）、`parse-zip FILE...`、`replay DIR...`（后两个离线，不联网）。原文件已归档到 `archive/reference-js/release/capsule-export.mjs`；workflow 改为 `./unisacc.com release/capsule-export.cx export` |

校验、拒绝文案和顺序、回执字段和顺序（`JSON.stringify` 紧凑形式）、退出码（拒绝时 stderr `capsule export refused: MSG`，退出 1）都照 mjs。CX 不复用 `validate-manifest.cx` 的 ZIP 读取：那份照的是 CPython `zipfile`，语义不同；复用的是 `cx/runtime/codec.cx`（inflate、CRC）、`release/ci/json.cx`、`cx/sys`、`cx/text`、`cx/mem`。

和 mjs 的语义差异（都写进了 `.cx` 文件头）：
- **HTTP 走 curl 子进程**：CX 这里没有 TLS 客户端。`-q`、只许 https、不跟随重定向、`--max-time` 等于本次预算（总 55 秒期限、每次最多 50 秒）。token 不进 curl 命令行：Authorization 等头写进 `$RUNNER_TEMP`（没有就 `$TMPDIR`、`/tmp`）下独占创建、权限 0600 的 curl 配置文件，curl 退出后删掉，任何拒绝路径也会删。这个目录放不下配置时，CX 在读网络之前就拒绝（mjs 会先读，再在 realpath/mkdir 失败）。
- **资产下载**：只手动跟随 302，目标必须是 https、无用户名密码、默认端口、主机是 `release-assets.githubusercontent.com` 或 `objects.githubusercontent.com`，否则 `untrusted Draft asset redirect`；第二跳不带 Authorization，再遇 3xx 报 `fetch failed`。目标 URL 取 curl 的 `%{redirect_url}`（Location 相对请求解析后的结果）。比 WHATWG URL 严：authority 里出现 `@`、host 里有 `%` 或非 ASCII 都拒绝；相对 Location 在 mjs 是 `Invalid URL`，这里解析成 api.github.com 后被主机名单拒绝。下载内容照旧核大小和 sha256。
- curl 退出码 28 对应 `The operation was aborted due to timeout`，其它失败（DNS、TLS、断连、没有 curl）对应 `fetch failed`。先读 body 再看状态码，所以非 2xx 且 body 超界时报 `bounded Draft response`，mjs 会报 `...read denied...`。
- JSON 用 `release/ci/json.cx` 读，另外拒绝 NaN/Infinity；读失败一律报 `invalid JSON in Draft API response`，不是 V8 的 SyntaxError 文案。比 `JSON.parse` 严：非 UTF-8 字节、超过 4300 位的整数、超过 Python 递归深度的嵌套都拒绝。
- 属性读取照 JavaScript：`String(x) === id`、字符串与 `true` 用 `===`、`Number.isSafeInteger`、digest 用 JS 真值、release 或 asset 是 null 时报 `Cannot read properties of null (reading 'K')`；两次绑定按 `JSON.stringify` 的相等比较（数组下标键排前面，数字按值比）。
- inflate：`cxcodec_inflate`（puff）代替 zlib。最后一块之后的多余字节照 zlib 忽略；声明长度之外继续解到 64 MiB，所以超长流照样报 `ZIP size/CRC`、`Cannot create a Buffer larger than 67108864 bytes` 或之后遇到的 zlib 错误。输入提前结束报 `unexpected end of file`；其它错误给 zlib 对同一故障的文案。已知文案差异只出在本来就会被拒的输入上：puff 对坏的长度码和坏的距离码只有一个错误码（zlib 分 `invalid literal/length code` 和 `invalid distance code`）；有些畸形 dynamic 块头 zlib 可能换个名字报；puff 不报告停在哪里，"输入用完"和"真故障"靠在输入后面分别补 64 KiB 0x00 和 0xff 重跑来区分，两种补法都碰巧复现同一错误码的情况是残余风险。分配失败报 `ZIP arena`，mjs 没有对应文案。
- RUNNER_TEMP 照 `fs.realpathSync` 处理：未设置时当成相对路径 `undefined`，错误文案是 node 的 `CODE: 描述, lstat '路径'`；路径上的软链用 realpath(3) 解析（node 自己逐级走，目标内部出错时报的路径可能不同）。

验证（本机，不访问 GitHub 网络）：
- 新 batch `capsule-export`（`tools/check-capsule-zip.cx`，已登记到 `tests/batches.tsv` 和 `tests/coverage.tsv`）。`tests/run.cx -- --list` 仍是 metadata_valid=1，`tests/run-batch.cx ... capsule-export` 结果 PASS（121 项，`CHECK_CAPSULE_ZIP failed=0`）。共三组：57 个构造的 ZIP 走 `parse-zip`；45 个录制的 release/env 用例走 `replay`，其中 5 个写出的 evidence.zip 和原字节相同，目录权限 0700、文件 0600；12 次 `export` 跑在 PATH 上的假 curl 上（就是这个检查程序的 `--fake-curl` 模式，前面套一行 sh exec 薄入口），覆盖 302 到允许主机、直接 200、不可信主机/端口/userinfo/http 重定向、二次重定向、404、301、curl 退出 6 和 28、4 MiB 上限。假 curl 同时核对：带了 `-q`、`--proto =https`、`--max-redirs 0`；token 只出现在 0600 配置里，不在 argv 里；对象主机那一跳不带 Authorization；配置文件跑完都删了。
- 和原 node 实现做差分（归档前用 `node -e` 直接调 mjs 的 `parseZip` 和 `exportApproved`，读同一批录制文件，main 里 RUNNER_TEMP 这几步也照做）：**102 例中 101 例 SAME，1 例 DIFF**。唯一的 DIFF 是 `json-nan`：两边都拒绝，只是文案不同（V8 SyntaxError 和 CX 的固定文案），理由见上。node 写出的 5 个 evidence.zip 和 CX 写出的 sha256 相同。夹具由 `tools/check-capsule-zip.cx -- DIR --prepare-only` 生成（这个 CX 生成器里带 ZIP 写入器，没有用 python 或 zip 命令）。
- 没验证的：真网络 export（真 curl、TLS、GitHub 真实返回、真实 302 目标）没有跑；CI 也没跑。
- **CI 前置**：和 `windows-signing.yml` 一样，检出里没有 `unisacc.com`（被 `.gitignore` 忽略），这个 step 在定下由哪一步提供 `unisacc.com` 之前跑不起来（见上面第 2 行，需要主人定）。另外要求 runner 上有 curl（GitHub Ubuntu runner 自带）。

差分表（case，mjs 结果，cx 结果，结论）。ZIP 用例（`parseZip`；OK 行比较了每一项的名字、方法、长度、flags、mode 和 extra）：

| case | mjs | cx | 结论 |
|---|---|---|---|
| bad-crc | `ZIP size/CRC` | `ZIP size/CRC` | SAME |
| central-extent | `ZIP central extent` | `ZIP central extent` | SAME |
| central-signature | `ZIP central header` | `ZIP central header` | SAME |
| central-start-moved | `ZIP central header` | `ZIP central header` | SAME |
| deflate-distance-too-far | `invalid distance too far back` | `invalid distance too far back` | SAME |
| deflate-empty-stream | `unexpected end of file` | `unexpected end of file` | SAME |
| deflate-invalid-block-type | `invalid block type` | `invalid block type` | SAME |
| deflate-invalid-stored-lengths | `invalid stored block lengths` | `invalid stored block lengths` | SAME |
| deflate-longer-than-declared | `ZIP size/CRC` | `ZIP size/CRC` | SAME |
| deflate-past-64mib | `Cannot create a Buffer larger than 67108864 bytes` | `Cannot create a Buffer larger than 67108864 bytes` | SAME |
| deflate-shorter-than-declared | `ZIP size/CRC` | `ZIP size/CRC` | SAME |
| deflate-truncated | `unexpected end of file` | `unexpected end of file` | SAME |
| descriptor-identity | `ZIP descriptor identity` | `ZIP descriptor identity` | SAME |
| descriptor-length-zero | `ZIP size/CRC` | `ZIP size/CRC` | SAME |
| dir-with-data | `ZIP size/CRC` | `ZIP size/CRC` | SAME |
| dir-with-file-mode | `ZIP link/special file` | `ZIP link/special file` | SAME |
| disk-number | `ZIP disk/inventory` | `ZIP disk/inventory` | SAME |
| duplicate-name | `unsafe/duplicate ZIP path` | `unsafe/duplicate ZIP path` | SAME |
| end-comment | `ZIP end/comment` | `ZIP end/comment` | SAME |
| extra-duplicate | `ZIP extra extent/duplicate` | `ZIP extra extent/duplicate` | SAME |
| extra-extent | `ZIP extra extent/duplicate` | `ZIP extra extent/duplicate` | SAME |
| extra-header-short | `ZIP extra header` | `ZIP extra header` | SAME |
| extra-local-length | `unsupported ZIP extra` | `unsupported ZIP extra` | SAME |
| extra-unsupported-id | `unsupported ZIP extra` | `unsupported ZIP extra` | SAME |
| fifo-mode | `ZIP link/special file` | `ZIP link/special file` | SAME |
| file-comment | `unsupported ZIP features` | `unsupported ZIP features` | SAME |
| file-dir-collision-dir-entry | `ZIP file/directory collision` | `ZIP file/directory collision` | SAME |
| file-dir-collision | `ZIP file/directory collision` | `ZIP file/directory collision` | SAME |
| flag-encrypted | `unsupported ZIP features` | `unsupported ZIP features` | SAME |
| gap | `ZIP gap/overlap` | `ZIP gap/overlap` | SAME |
| inflated-budget | `ZIP inflated budget` | `ZIP inflated budget` | SAME |
| inventory-count-mismatch | `ZIP disk/inventory` | `ZIP disk/inventory` | SAME |
| local-crc-differs | `ZIP local size/CRC` | `ZIP local size/CRC` | SAME |
| local-method-differs | `ZIP local identity` | `ZIP local identity` | SAME |
| local-name-differs | `ZIP local extent` | `ZIP local extent` | SAME |
| local-signature | `ZIP local identity` | `ZIP local identity` | SAME |
| method-12 | `unsupported ZIP features` | `unsupported ZIP features` | SAME |
| ok-deflate-trailing-bytes | OK，1 项，清单同 | OK，1 项，清单同 | SAME |
| ok-descriptor-signed | OK，1 项，清单同 | OK，1 项，清单同 | SAME |
| ok-descriptor | OK，2 项，清单同 | OK，2 项，清单同 | SAME |
| ok-extra-utf8flag | OK，1 项，清单同 | OK，1 项，清单同 | SAME |
| ok-mode0-charset | OK，1 项，清单同 | OK，1 项，清单同 | SAME |
| ok-stored-deflated-dir | OK，3 项，清单同 | OK，3 项，清单同 | SAME |
| overlap | `ZIP gap/overlap` | `ZIP gap/overlap` | SAME |
| oversize-declared-descriptor | `ZIP local extent` | `ZIP local extent` | SAME |
| oversize-declared | `ZIP local extent` | `ZIP local extent` | SAME |
| payload-extent | `ZIP payload extent` | `ZIP payload extent` | SAME |
| size-too-small | `ZIP size` | `ZIP size` | SAME |
| stored-size-differs | `ZIP size/CRC` | `ZIP size/CRC` | SAME |
| symlink-mode | `ZIP link/special file` | `ZIP link/special file` | SAME |
| unsafe-absolute | `unsafe/duplicate ZIP path` | `unsafe/duplicate ZIP path` | SAME |
| unsafe-backslash | `unsafe/duplicate ZIP path` | `unsafe/duplicate ZIP path` | SAME |
| unsafe-dotdot | `unsafe/duplicate ZIP path` | `unsafe/duplicate ZIP path` | SAME |
| unsafe-dot | `unsafe/duplicate ZIP path` | `unsafe/duplicate ZIP path` | SAME |
| unsafe-double-trailing-slash | `unsafe/duplicate ZIP path` | `unsafe/duplicate ZIP path` | SAME |
| unsafe-empty-segment | `unsafe/duplicate ZIP path` | `unsafe/duplicate ZIP path` | SAME |
| unsafe-non-ascii | `unsafe/duplicate ZIP path` | `unsafe/duplicate ZIP path` | SAME |

录制 release 用例（`exportApproved` 加上 main 的 RUNNER_TEMP 几步；OK 行比较整条回执）：

| case | mjs | cx | 结论 |
|---|---|---|---|
| after-changed | `Draft asset changed during download` | `Draft asset changed during download` | SAME |
| after-key-order | `Draft asset changed during download` | `Draft asset changed during download` | SAME |
| after-not-draft | `fixed existing private Draft release required` | `fixed existing private Draft release required` | SAME |
| asset-digest-empty-object | `asset API digest mismatch` | `asset API digest mismatch` | SAME |
| asset-digest-other | `asset API digest mismatch` | `asset API digest mismatch` | SAME |
| asset-id-other | `fixed asset ID/name/size/state/URL` | `fixed asset ID/name/size/state/URL` | SAME |
| asset-id-unsafe | `fixed owner release/asset/SHA/candidate identity required` | `fixed owner release/asset/SHA/candidate identity required` | SAME |
| asset-null-element | `Cannot read properties of null (reading 'name')` | `Cannot read properties of null (reading 'name')` | SAME |
| asset-size-over | `fixed asset ID/name/size/state/URL` | `fixed asset ID/name/size/state/URL` | SAME |
| asset-size-string | `fixed asset ID/name/size/state/URL` | `fixed asset ID/name/size/state/URL` | SAME |
| asset-size-zero | `fixed asset ID/name/size/state/URL` | `fixed asset ID/name/size/state/URL` | SAME |
| assets-none | `unique evidence.zip asset required` | `unique evidence.zip asset required` | SAME |
| assets-not-array | `bounded release inventory` | `bounded release inventory` | SAME |
| asset-state-other | `fixed asset ID/name/size/state/URL` | `fixed asset ID/name/size/state/URL` | SAME |
| assets-two-evidence | `unique evidence.zip asset required` | `unique evidence.zip asset required` | SAME |
| asset-url-other | `fixed asset ID/name/size/state/URL` | `fixed asset ID/name/size/state/URL` | SAME |
| evidence-sha-other | `owner evidence raw SHA/size mismatch` | `owner evidence raw SHA/size mismatch` | SAME |
| evidence-size-other | `owner evidence raw SHA/size mismatch` | `owner evidence raw SHA/size mismatch` | SAME |
| inventory-outside-evidence | `proof-only evidence/ inventory required` | `proof-only evidence/ inventory required` | SAME |
| inventory-work | `proof-only evidence/ inventory required` | `proof-only evidence/ inventory required` | SAME |
| json-nan | `Unexpected token 'N', ..."Z","size":NaN}]}" is not valid JSON` | `invalid JSON in Draft API response` | DIFF |
| ok | OK，回执同 | OK，回执同 | SAME |
| ok-after-same-by-stringify | OK，回执同 | OK，回执同 | SAME |
| ok-digest-falsy | OK，回执同 | OK，回执同 | SAME |
| ok-digest-match | OK，回执同 | OK，回执同 | SAME |
| ok-id-forms | OK，回执同 | OK，回执同 | SAME |
| output-exists | `EEXIST: file already exists, mkdir '…/replay/output-exists/rt/unisacc-evidence-capsule-output'` | `EEXIST: file already exists, mkdir '…/replay/output-exists/rt/unisacc-evidence-capsule-output'` | SAME |
| producer-sha-short | `owned source-repository main producer required` | `owned source-repository main producer required` | SAME |
| ref-other | `owned source-repository main producer required` | `owned source-repository main producer required` | SAME |
| release-array | `fixed existing private Draft release required` | `fixed existing private Draft release required` | SAME |
| release-draft-string | `fixed existing private Draft release required` | `fixed existing private Draft release required` | SAME |
| release-id-fraction | `fixed existing private Draft release required` | `fixed existing private Draft release required` | SAME |
| release-id-leading-zero | `fixed owner release/asset/SHA/candidate identity required` | `fixed owner release/asset/SHA/candidate identity required` | SAME |
| release-id-other | `fixed existing private Draft release required` | `fixed existing private Draft release required` | SAME |
| release-not-draft | `fixed existing private Draft release required` | `fixed existing private Draft release required` | SAME |
| release-null | `Cannot read properties of null (reading 'id')` | `Cannot read properties of null (reading 'id')` | SAME |
| release-url-other | `fixed existing private Draft release required` | `fixed existing private Draft release required` | SAME |
| repo-other | `owned source-repository main producer required` | `owned source-repository main producer required` | SAME |
| runner-temp-missing | `ENOENT: no such file or directory, lstat '…/replay/runner-temp-missing/rt'` | `ENOENT: no such file or directory, lstat '…/replay/runner-temp-missing/rt'` | SAME |
| sha-uppercase | `fixed owner release/asset/SHA/candidate identity required` | `fixed owner release/asset/SHA/candidate identity required` | SAME |
| source-sha-other | `fixed owner release/asset/SHA/candidate identity required` | `fixed owner release/asset/SHA/candidate identity required` | SAME |
| source-sha-unset | `fixed owner release/asset/SHA/candidate identity required` | `fixed owner release/asset/SHA/candidate identity required` | SAME |
| token-absent | `source Actions token absent` | `source Actions token absent` | SAME |
| token-empty | `source Actions token absent` | `source Actions token absent` | SAME |
| zip-refused | `ZIP size/CRC` | `ZIP size/CRC` | SAME |
