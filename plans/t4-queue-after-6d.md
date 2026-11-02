# 6d 窗口后待办队列的落点（只读调查，2026-11-02）

cc 的 5 项队列，窗口内只做定位，不改 `tools/ tests/ cx/ src/`。以下每项都写明改哪、怎么验。

## 1. `check-package-decode-refusals.cx:27` 输入改 `P 5`

- 现：`{"version","P 4 1 1\n","unknown package version"}`。P4 已被 `1bf6f7e8c3` 受理，这条永远不可能同时满足库层与参考 C。
- 改：`{"version","P 5 1 1\n","unknown package version"}`。库层 `package_decode.cx:96` `ver>4` 拒；参考 C `run.c:665` 只认 1..3 也拒。两侧都报同一串。
- 验：`tools/bound 55 ./unisacc.com tools/check-package-decode-refusals.cx -- ROOT` 期望 `CXPACKAGE_REFUSALS failed=0`；再 `tools/affected.cx --files tools/check-package-decode-refusals.cx --run`。

## 2. `check-packagestruct.cx:54` 标签/输入更正

- 现：`audit("P 4 1 1\nD route stage in out 0\nM 1\nZ", 2, 0, "version-rejected")`。
- 事实：现在仍返回 2，但原因不是版本，而是 `packagestruct.cx:90` 对 ver=4 要求 5 个头部字段、这条只给 4 个。断言靠错理由通过。
- 改（择一）：
  - 保留「版本拒绝」语义 → 输入改 `P 5 1 1\n…`，标签保留 `version-rejected`；
  - 或保留 P4 输入、标签改为 `short-header`，另加一条 `P 4 1 1 1\n…` 的 P4 受理用例。
- 验：同上 `CXPS_*`/`failed=` 计数 + `affected --run`。

## 3. P4 受理正向覆盖（新增用例 + 登记批次）

现状（只读核对）：全仓 `grep 'P 4 '` 只命中两条拒绝断言与 `tools/release-build.cx`，**没有任何 P4 受理正向用例**；`1bf6f7e8c3` 改了 12 个实现文件、未动测试。

落点：

- 读侧：`tools/check-package-decode.cx` 已有 `P 1 1 1` 的合法样本与逐前缀截断循环。可加一份 **P4 五词 F 行**样本（含 `codec=0` 不压缩行与 `codec=1` deflate 行），断言 `cxpk_decode` 成功、`res[].rawlen/codec/crc` 正确、`codec=0` 时 `rawlen==len`。
- 结构侧：`cx/runtime/packagestruct.cx` 的 `walk` 对 ver=4 要求 `nf==5`。给 `check-packagestruct.cx` 加一条合法 P4、`visit` 收到正确 `stored`。
- 写侧：`tools/release-build.cx:269 rb_p3`，`--compress-resources` 置 `compress=1` → `out_ver=4`；`--no-compress-resources` 保留输入版本。定值用例应用**小样本包**跑两次（压缩/不压缩），比对输出包头 `P 4`/`P 3`，并把输出再喂回 `cxpk_decode` 与参考 C 的 `package_decode_reference`，确认资源逐字节等于源。
- 登记：`sh ./unisacc.com tools/register.cx -- ID CHECK.cx MARKER "COVERAGE TEXT" "FILES" LAYER`（`tools/register.cx` 一次做完两处 append + `--list` + `run-batch`，失败自动回滚两行）。

已知干扰：`tools/check-codec.cx:18` 硬编码 `memcmp(p,"P 3 ",4)` 读**已装产品**的包。一旦产品以 P4 安装（`--compress-resources`），该检查的 `real_member` 会直接 `goto done` 判 0。这条要么改成接受 `P 3`/`P 4` 并按 M 行版本分支，要么在 P4 默认化前明确它只覆盖 P3。**不要**为了让 `P 3` 字面量成立而固定产品版本。

## 4. `census.cx` 排除 `full-*`

- 选行块在 `tools/census.cx:88-95`：`col(s,l,0,id,…)` 取 id，之后依次过滤 `IMPLEMENTED` / `lnx/x86_64` / `mat=="-"`。
- 改：在 `mat` 过滤后加一条前缀判定 `tx_prefix(id, len, "full-", 5)` → 计入新的 `skipped_domain` 计数（不并进 `skipped_materials`，报告里要能分开看）。
- 理由：`full-*` 是域聚合批，要求同候选全格；单宿主上 5 条 OPEN 固定为外平台 `NEEDS_FOREIGN_HOST` + 本机 `MISSING`，必 FAIL 且不代表回归。上一次普查 19/30 红项来自这一组。
- 验：`tools/check-census.cx` 全部五例（首跑/resume/redo/report/bad-option）。
  **解析方已核对**：全仓只有 `tools/census.cx:123` 自己打印、`tools/check-census.cx` 用子串匹配消费（`has(out,"ran=1\tpass=1")`、`has(out,"ran=0")`、`has(out,"fail=0")`、`has(out,"total_recorded=1")`），没有别的程序解析 `CENSUS\t`。因此新增 `skipped_domain=` 字段不会破坏任何现有解析；只需同步 `census.cx:14` 的文件头注释与 `check-census.cx` 里若有逐字段断言的用例（现在没有）。
- 另发现（同属 harness，窗口内不改）：`tools/check-census.cx:29/32/34/36` 自己也是 `{"sh", com, tool, …}` 固定 `sh` 启动产品，与 `census.cx` 原先同一个毛病。薄头形态下没问题；产品一旦是独立 ELF，`check-census` 会整体失败。第 4 项落地时顺手把 `check-census` 改成同样的文件头判形态，或让它复用 `census.cx` 的判定。

## 5. 稳定产品上重跑全量 census

- 前提：bdy 宣布 6d 窗口结束；根产品 `./unisacc.com --version` 与 `sh ./unisacc.com --version` 至少一条 rc=0；记 `sha256sum unisacc.com` 与三个身份（薄头形态含旁目录两文件）。
- 新建目录：`out/census-<sha8>/identity.txt` 写入 sha + 旁目录 sha + `--version` 输出。旧 `out/census-83343ec1/` 保留不删（那 160 条仍是有时效的观测）。
- 红项：`optimization-run`、`negative-diagnostics` 用 `--redo`（只有 redo 分支会删 `out/<dir>/run/<id>`）；`cx-l0-*` 四条单独跑、记相位；`cx-core-bytes` 需确认窗口内产品未被替换（比 before/after 即可，或干脆在窗口内不跑它）。
- 不缓存 PASS：重跑是新的观测，不继承旧结果。
