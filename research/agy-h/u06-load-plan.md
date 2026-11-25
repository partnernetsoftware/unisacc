# U06 并发写负载下 50 轮零红测试方案草稿 (按 cdx 复核修订, 待 0.0.106 预跑完成后执行)

日期: 2026-11-24
执行窗口: agy-h (执行实跑、旧写法对照与留证), agyneon (定因归属裁定)
依据: `archive/research-2026-11-25/2026-11-24/R38-ruling.md:75,97,99,111`、`build/state-unverified.tsv:4`、`git:813e1629ec`、`git:80155036e6`、`tools/tsv-lint.cx:38,302-309`、`tools/check-tsv-lint.cx:196-226`

## 1. 历史输入与真实读写关系核查 (回应 cdx 复核第 1 点)

### 1.1 `U06` 历史红现场与推测来源 (`813e1629ec`, 2026-11-14)

- `813e1629ec` (`prd_mempalace.md`) 原文记录 `U06`: 2026-11-14 (`0.0.61`/`0.0.65`) 同日三次实跑 (`A2 复验 PASS -> B5 分片跑报 rc=1 FAIL -> 随后单跑 PASS`), 当时怀疑与并发写表 (他人暂存的 `build/` 目录下 `.tsv` 表) 有关, 但未留 `git status` 与失败行。
- `build/state-unverified.tsv:4` (`U06`): 对象工具为 `tools/tsv-lint.cx`, 缺失证据为 "同日 PASS/FAIL/PASS 的定因"。

### 1.2 `tools/tsv-lint.cx` 真实读集 vs `80155036e6` 写集对照

核对源码读写路径, 两者没有直接读写文件交集:

| 对象 | 读取文件 / 目录 | 写入文件 / 目录 | 与对方的交集 |
|---|---|---|---|
| `tools/tsv-lint.cx` | `exec/pp/` (`-manifest.tsv`、`-byte.tsv`、`-result.tsv`、`-template.tsv`)、`exec/parse2/`、`exec/facts/` (`.tsv`)、`tools/tsv-lint-policy.tsv`、`build/tsv-lint-baseline.tsv` (`0.0.92` 起) | 无 (只读审计并输出 stdout) | 不读 `build/tool-index.tsv`、`build/table-index.tsv`、`$TMPDIR/unisacc-derive-last.tsv`、`.native_tool.path`、`submit-patch-bin` |
| `tools/check-tsv-lint.cx` | 同上 (第 223-224 行对 `--root .` 调 `tsv-lint`), 外加 `tools/tsv-lint.cx` 源文件与 `tools/tsv-lint-cases/` | 仅 `$TMPDIR/tsv-lint-check-XXXXXX` 私有夹具 | 不读写 `80155036e6` 修改的任何表与临时文件路径 |
| `80155036e6` (5 文件修复) | 仓库目录树、源码与 `build/layer-api/` 等 | `tools/tool-index.cx:119` (`build/tool-index.tsv`、`build/table-index.tsv` 及 devmeta 镜像改 `sy_write_atomic`); `tools/derive.cx:90` (`$TMPDIR/unisacc-derive-last.tsv` 改 `sy_write_atomic`); `tools/check-candidate-chain.cx:836` (`.native_tool.path` / `.native_gen.path` 改 `sy_write_atomic`); `tools/check-submit-patch.cx:55` (`%s.bin` 私有命名) | 所修复写路径中没有任何一条落入 `tsv-lint.cx` 读集; 历史上 `build/tsv-lint-baseline.tsv` 且晚至 `0.0.92` (`74c0c7c43c`) 才引入 |

关键推论 (待本方案实验证伪/证实):
(1) `tool-index --write` 与 `tsv-lint --check` 并行空跑 50 轮无读写交集, 不能用来证明 `80155036e6` 修复了 `U06`;
(2) 若历史上真有写表导致 `tsv-lint` 同日 PASS/FAIL/PASS, 写者必须落在 `tsv-lint` 的真实读集 (`exec/pp/`、`exec/parse2/`、`exec/facts/`、`tools/tsv-lint-policy.tsv` 或 `0.0.92` 后的 `build/tsv-lint-baseline.tsv`), 而目前仓内仍以非原子 `cx_write()` 写文件的路径 (`cx/runtime/runexec.cx:1005` 截断重写) 若与读者 `load()` (`tools/tsv-lint.cx:38` `lstat` + `cx_read`) 碰撞即会撕裂 (`rc=2` `input extent or NUL` / `empty baseline file` / `invalid` REFUSE)。

## 2. 修订后的三臂对照压测方案 (真读写依赖 + 旧写法对照 + 稳定性)

触发前提与纪律不变: 须等 `0.0.106` 预跑彻底结束并退出压载纪律后再跑; 总并发 `<= 4` 路, 单命令硬顶 `<= 55` 秒, 编排全部用 `$TMPDIR` 下 `.cx` 临时脚本经 `sh ./unisacc.com` 驱动, 不改共享仓追踪表, 总预算 `25` 分钟内。

### 2.1 臂 A: `tsv-lint` 真实读集并发读写撕裂对照 (核心必测)

在隔离沙盒 `$TMPDIR/u06-sandbox-A` (`git archive HEAD` 解压 `exec/`、`tools/`、`build/`、`cxroot.tsv` + 符号链接 `unisacc.com`) 中构建真读写配对:
- 读者 (`Reader-A`): 连续执行 `sh ./unisacc.com tools/tsv-lint.cx -- --root $TMPDIR/u06-sandbox-A --check` (预编原生二进制直跑以拉高每秒碰撞次数)。
- 写者 (`Writer-A`, 与 `Reader-A` 真实碰撞同一批被读文件): 对以下两类 `tsv-lint` 真实读入文件循环以合法内容原地覆写 (使文件内容语义完全合规, 任何红只可能来自并发截断读撕裂):
  - 对象 1: `$TMPDIR/u06-sandbox-A/build/tsv-lint-baseline.tsv` (覆写为相同合法行 `unresolved\t10764\n`, 由 `tools/tsv-lint.cx:304` `load_baseline` 读取);
  - 对象 2: `$TMPDIR/u06-sandbox-A/exec/pp/start-byte.tsv` 或 `$TMPDIR/u06-sandbox-A/tools/tsv-lint-policy.tsv` (按原字节合法原文覆写, 由 `tools/tsv-lint.cx:303`/`56` 读取)。

对照两档切换:
- A1 (旧写法对照臂 — 复刻 `80155036e6` 前截断写): `Writer-A` 调用 `cx_write(path, orig_bytes, orig_len)` (等价 `open(O_WRONLY|O_CREAT|O_TRUNC)`) 持续覆写; `Reader-A` 连跑 `50` 轮 (若快速完成则补至 `200` 轮或命中首次撕裂为止, 记录首次命中轮次、退出码与 stderr `input extent or NUL` / `empty baseline file` / `invalid` 原文), 验证非原子写下 `tsv-lint` 是否真实可撕裂出红。
- A2 (新写法实验臂 — 采用 `80155036e6` 的 `sy_write_atomic`): 同一沙盒与写频, 仅把 `Writer-A` 替换为 `sy_write_atomic(path, orig_bytes, orig_len, 0644)` (同目录临时文件 + `rename`); `Reader-A` 连跑 `50` 轮以上 (加补压至总碰撞等价次数 `>= A1`), 核验是否 `0` 撕裂 (`rc=0`, `errors=0`)。

### 2.2 臂 B: `80155036e6` 原文对象真读写依赖对照 (检视 `tool-index` 与 `derive` 自身机制)

在隔离沙盒 `$TMPDIR/u06-sandbox-B` 中验证 `80155036e6` 提交说明直接声称的对象 (写者与读者同文件):
- 读者 (`Reader-B`): 循环读并校验同一份生成表: `sh ./unisacc.com tools/derived-query.cx -- table-index --check` (或等价直接解析 `$TMPDIR/u06-sandbox-B/build/table-index.tsv` 与 `tools/status.cx` 读取 `$TMPDIR/unisacc-derive-last.tsv`)。
- 写者 (`Writer-B`): 对 `build/table-index.tsv` (以及 `unisacc-derive-last.tsv`) 同步循环写:
  - B1 (旧写法对照): 从 `git show 80155036e6^:tools/tool-index.cx` 抽出旧 `cx_write` 模式或对同文件调 `cx_write`, `Reader-B` 连跑至少 `50` 轮 (建议 `200` 轮以测试统计撕裂), 记录是否复现撕裂;
  - B2 (现行 `HEAD` 写法): 直接调当前 `tools/tool-index.cx -- --tables` (`sy_write_atomic`) 与 `Reader-B` 对跑至少 `50` 轮 (`200` 轮), 验证 `0` 撕裂。

### 2.3 臂 C: 真实主仓 `50` 轮稳定性臂 (满足 R38 字面连跑 50 轮要求)

在仓库真实根目录 `.` 上 (不破坏共享追踪表, 不触发他窗口路径写冲突):
- 主测 (`Reader-C`, 连跑 `50` 轮):
  - 子项 1: `sh ./unisacc.com tools/check-tsv-lint.cx --` (预期 `rc=0`, `CHECK_TSV_LINT\tchecks=62\tfailed=0`);
  - 子项 2: `sh ./unisacc.com tools/tsv-lint.cx -- --root . --check` (预期 `rc=0`, `tsv-lint: ok:`, `errors=0`)。
- 并发扰动 (`Worker-C`, 全程后台伴随):
  - `DEVSVC_OWNER=agy-h sh ./unisacc.com tools/devsvc.cx -- f write research/agy-h/.u06-load-probe.tsv --from <tmp_payload>` + `tools/check-submit-patch.cx` + `tools/status.cx` + `tools/derive.cx -- --check`。

## 3. 判定口径与交给 `agyneon` 的闭环证据规格

### 3.1 结果表 (`$TMPDIR/u06-results.tsv` 摘要落 `research/agy-h/u06-50rounds.md`)

分臂记录:
- 臂 A (`A1_cx_write` vs `A2_sy_write_atomic`): 总轮次、写操作次数、`Reader-A` 失败/REFUSE 次数、首次失败原行 (含 `rc` 与 stderr 文案);
- 臂 B (`B1_old_tool_index` vs `B2_new_tool_index`): 总轮次、撕裂次数、失败原文;
- 臂 C (真仓 `check-tsv-lint` + `tsv-lint --root . --check` 50 轮): `round | ts | check_rc | checks | failed | root_rc | errors | unresolved | wall_ms`, 以及结束后 `ps`/`pgrep` 进程残留核实 (`0` 残留)。

### 3.2 `U06` 定因归属双支判据 (交 `agyneon` 最终一句裁定)

(1) 若 `A1` 复现撕裂红且 `A2` 零红, 同时 `C` 满足 `50/50` 零红:
- 证明了机制 ("非原子 `cx_write` 并发写 `tsv-lint` 读集确实会造成同日 PASS/FAIL/PASS, 而 `sy_write_atomic` 可消除该类撕裂");
- 但由于 `80155036e6` 并未修改任何写入 `exec/` (`exec/pp`、`exec/parse2`、`exec/facts`) 或 `build/tsv-lint-baseline.tsv` 的写者 (`0.0.65` 时期尚无 `tsv-lint-baseline.tsv`), `agyneon` 需据此在两条裁定路线中二选一:
  - 路线 1 (可关): 若核定当前全仓已无对 `exec/pp|parse2|facts` 与 `build/tsv-lint-baseline.tsv` 的在途非原子并发写路径 (仅人工提交/静态检查), 且当年疑为诱因的 `build/` 工具侧并发写表竞态已在 `80155036e6` 消零, 则凭 `C` 臂 `50/50` 零红加一句归属收口;
  - 路线 2 (保留或补改): 若查出仍有写 `exec/` 或 `tsv-lint-baseline.tsv` 的工具/补丁未改用 `sy_write_atomic` 且 `A1` 已打穿, 则按 `R38:111` 保留 `U06` 并开具体修复指向该写者 (比草率关行更有价值)。
(2) 若 `C` 在 50 轮内自身报任何红: 当场停止并保全 `git status --short`、stdout/stderr 原文交 `agyneon`, 直接实锤未稳, 不关 `U06`。

## 4. 时长预算 (总上限 25 分钟)

| 阶段 | 内容 | 预计耗时 |
|---|---|---|
| 阶段 0 | 搭建 `$TMPDIR/u06-sandbox-A` 与 `u06-sandbox-B` (含预编 `tsv-lint` 与旧/新对比小工具) | `2` 分钟 |
| 阶段 1 | 执行臂 A (`A1` 旧写法 + `A2` 新写法各 `>=50` 轮, 直跑预编 `tsv-lint` 每轮 `<0.3s`) 与臂 B (`B1` vs `B2` 各 `>=50` 轮) | `3-5` 分钟 |
| 阶段 2 | 执行臂 C (真仓 `check-tsv-lint` + `tsv-lint --root . --check` 共 `50` 轮 + 并发 `Worker-C`, 热编后约 `7-11s/轮`) | `6-11` 分钟 |
| 阶段 3 | 清理后台进程组 (`TERM -> KILL -> ps 核实`) 与临时沙盒、落盘 `research/agy-h/u06-50rounds.md` 并 `envelope` 发 `agyneon` 与 `cc` | `2-3` 分钟 |
| 合计 | 全部三臂执行加清理预计 `13-21` 分钟, 严控在 `25` 分钟预算以内 | `<= 25` 分钟 |
