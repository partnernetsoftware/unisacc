# unisacc 0.0.92 发布说明

整体裁决 NOT_QUALIFIED

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交 | e30496b653c35c6b47230329170f45ab2d895ad5 |
| 候选提交 | 96d0c6dd2eb77b51e4ecba52903df72aefb6a92f |
| 发布 APE SHA256(unisacc.com) | de8d48f5e60b03cd6394fe73f56edace5ff19318c04cac4b32a432b9881f5b3a |
| 源快照 SID | 5aa21548a2c80d58ef98e560727a5a13b8bf47eab467d22140ed17f0ecb3bf0e |
| pkg.bin | 47227efe9f518918e0a844b09e6a38d0d6c53aba20c243b732ffe3fee476aa92 |
| slice-win-arm64 | 3b52228f3a870d7b859a453aeefca2f5d8630b578964ecdbd3c3f336a92534c0 |
| SHA256SUMS | 3f5c9dc71a059599d8c851f25dccaeeda24834f7f49e3d912da97ecd7dd2e55f |

| 对象 | 身份与字节数 | 回执来源路径 |
|---|---|---|
| 冻结基点(F) | `e30496b653c35c6b47230329170f45ab2d895ad5` (`T..F` 仅 C 类提交 `R79` `fbe594b2a0` 与 `dup-census` 去重 `e30496b653`, 零 A 类变动, 与 `T` 同 `SID`) | `git rev-parse e30496b653` / `release/candidate.tsv` |
| 候选整链与预跑基点(T / REV) | `96d0c6dd2eb77b51e4ecba52903df72aefb6a92f` (由 `v0.0.92` `56e49da1d3` `--no-ff` 冻结合并入 `main`) | `git rev-parse 96d0c6dd2e` / `~/.cache/unisacc/chains/bg-b1/96d0c6dd2eb77b51e4ecba52903df72aefb6a92f/owner.tsv` |
| 链产出 APE (`chain_bytes.ape` / `unisacc-next.com`) | `de8d48f5e60b03cd6394fe73f56edace5ff19318c04cac4b32a432b9881f5b3a` (`8992973 bytes`, 相对 0.0.91 `8991222 bytes` `+1751 B`) | `~/.cache/unisacc/chains/bg-b1/96d0c6dd2eb77b51e4ecba52903df72aefb6a92f/ape-final/ape-receipt.txt` |
| 源快照 SID (`source-packages-v3`) | `5aa21548a2c80d58ef98e560727a5a13b8bf47eab467d22140ed17f0ecb3bf0e` | `~/.cache/unisacc/chains/bg-b1/96d0c6dd2eb77b51e4ecba52903df72aefb6a92f/seal.tsv` |
| 链产出权重包 (`chain_bytes.p3` / `compiler.p4.pkg`) | `47227efe9f518918e0a844b09e6a38d0d6c53aba20c243b732ffe3fee476aa92` (`2904381 bytes`, 相对 0.0.91 `2902646 bytes` `+1735 B`) | `~/.cache/unisacc/chains/bg-b1/96d0c6dd2eb77b51e4ecba52903df72aefb6a92f/ape-final/ape-receipt.txt` |
| 链产出包体 (`chain_bytes.p3_body` / `p3-package.body` / `pkg.bin`) | `47227efe9f518918e0a844b09e6a38d0d6c53aba20c243b732ffe3fee476aa92` (`2904365 bytes`, 相对 0.0.91 `2902630 bytes` `+1735 B`) | `~/.cache/unisacc/chains/bg-b1/96d0c6dd2eb77b51e4ecba52903df72aefb6a92f/ape-final/ape-receipt.txt` |
| 发布资产 `slice-win-arm64` (`asset_bytes.slice_win_arm64`) | `3b52228f3a870d7b859a453aeefca2f5d8630b578964ecdbd3c3f336a92534c0` (`4572672 bytes`, 版本注入后; 链产出原始切片 `54bb23dea3c64841da45020aff9eb0a2e2f87ff6e65e75ee6505c677195a816b`) | `release/candidate.tsv` / `release/evidence/0.0.92/pack-run.out` |
| 发布资产 `SHA256SUMS` | `3f5c9dc71a059599d8c851f25dccaeeda24834f7f49e3d912da97ecd7dd2e55f` | `release/candidate.tsv` |

## 严格证据

459 批:PASS 457、FAIL 0、UNKNOWN 2、INVALID 0

- **候选整链(`T=96d0c6dd2eb77b51e4ecba52903df72aefb6a92f`, `F=e30496b653c35c6b47230329170f45ab2d895ad5`, `T..F` 零 A 类变动、同 `SID=5aa21548a2c80d58ef98e560727a5a13b8bf47eab467d22140ed17f0ecb3bf0e`)**: 一次性 `499/499 DONE` (`iterations=9`, `rc=0`, 0 失败、0 repin; 回执路径: `~/.cache/unisacc/chains/bg-b1/96d0c6dd2eb77b51e4ecba52903df72aefb6a92f/exit.tsv` `EXIT 1795337014 pid=2398178 rc=0 why=exited` 与 `bg.log` `SUPERVISE STOP iterations=9 rc=0`):
  - `chain_bytes` 与 `0.0.91` (`~/.cache/unisacc/chains/bg-b1/7be3cc5d4b03e1b3b281a372ac26b9da19c18b44/ape-final/ape-receipt.txt`) 逐项对拍:
    - **真实有差(事实表进权重包)**:
      - `p3` (`compiler.p4.pkg`): `b717fff8...` (`2902646 B`) -> `47227efe9f518918e0a844b09e6a38d0d6c53aba20c243b732ffe3fee476aa92` (`2904381 B`, `+1735 B`)
      - `p3-package.body`: `21a77765...` (`2902630 B`) -> `c99331271feb29d39a97575d119f038c31c18dc174478fb8fe417d0958d6d260` (`2904365 B`, `+1735 B`)
      - `unisacc-next.com`: `eb18eb43...` (`8991222 B`) -> `de8d48f5e60b03cd6394fe73f56edace5ff19318c04cac4b32a432b9881f5b3a` (`8992973 B`, `+1751 B`)
    - **逐字节恒等(本版机械 A 类为 0, `src/`/`cx/`/`seed/`/`exec/c/` 未改)**:
      - `lnx/x86_64` (`13d94c1b`, `2375382 B`)、`lnx/arm64` (`d37c2d81`, `2571934 B`)、`osx/x86_64` (`ca5523db`, `3104482 B`)、`osx/arm64` (`b78c52b9`, `4194274 B`)、`win/x86_64` (`7fbf1373`, `3414016 B`)、`win/arm64` (`54bb23de`, `4572672 B`)、两份 `cache-manager` (`osx/x86_64 847748bb 247906 B` / `osx/arm64 f41542f3 264418 B`)。
- **`ci-early` 六目标原生 CI**: `run_id=45004230169` 一次性 `6/6 PASS` (`pass_cells=6, fail_cells=0`, `home_paths_redacted=194`, 草稿标签 `v0.0.92-early` 已删):
  ```text
  CI_EARLY_LOG_SCRUBBED	home_paths_redacted=194
  CI_EARLY_CLEANUP	tag=v0.0.92-early	status=DELETED
  CI_EARLY_CELL	target=lnx/x86_64	conclusion=success	check=ALL	origin=-	diag=-
  CI_EARLY_CELL	target=lnx/arm64	conclusion=success	check=ALL	origin=-	diag=-
  CI_EARLY_CELL	target=osx/x86_64	conclusion=success	check=ALL	origin=-	diag=-
  CI_EARLY_CELL	target=osx/arm64	conclusion=success	check=ALL	origin=-	diag=-
  CI_EARLY_CELL	target=win/x86_64	conclusion=success	check=ALL	origin=-	diag=-
  CI_EARLY_CELL	target=win/arm64	conclusion=success	check=ALL	origin=-	diag=-
  CI_EARLY_SUMMARY	rev=96d0c6dd2eb77b51e4ecba52903df72aefb6a92f	ape_sha=de8d48f5e60b03cd6394fe73f56edace5ff19318c04cac4b32a432b9881f5b3a	run_id=45004230169	verdict=PASS	pass_cells=6	fail_cells=0	failed_targets=-	first_origin=-	first_diag=-
  ```
- **严格预跑(459 批 151 窗 / 含恢复子窗 154 窗, DONE)**:
  - 归档与双份备份: `release/evidence/v0.0.92-strict-2026-11-22.tsv` (`sha256=8c204098f58098b37b291a3c50b12b8502e5f012ab3dbe86597b9ea2dc6aa0bd`), `/tmp/unisacc-release-evidence-backups/0.0.92/` 与 `~/.unisacc-release-evidence-backups/0.0.92/` (`BACKUP_VERIFY PASS version=0.0.92 files=7 repo=.`)
  - 实测原行(`PASS=457, FAIL=0, UNKNOWN=2`, 原 `netregen-*` 与 `env-semantics` 全部实跑 `PASS`; 2 项 `UNKNOWN` 为 `cx-booter-ape` 与 `tools-default-regression`, `reason=no-producer`, 证明文件 `release/evidence/0.0.92/cx-booter-ape-deferred.tsv` 与 `release/evidence/0.0.92/tools-default-regression-deferred.tsv`):
    ```text
    PRERUN_RUN	status=DONE	ran=1	skipped=150	out=/tmp/unisacc-session-v0.0.92-prerun-96d0c6dd2e	window_wall_ms=4529	sum_batch_ms=2610	outer_overhead_ms=1919
    PRERUN_MERGE	windows=154	batches=459	PASS=457	FAIL=0	UNKNOWN=2	SUPERSEDED=0	out=/tmp/unisacc-session-v0.0.92-prerun-96d0c6dd2e/strict-final.tsv
    SUMMARY	batches=459	PASS=457	FAIL=0	UNKNOWN=2	INVALID=0	total_ms=6581	layout_ms=110	shadow=off	ape_sha256=de8d48f5e60b03cd6394fe73f56edace5ff19318c04cac4b32a432b9881f5b3a	version=unisacc 0.0.92
    PRERUN_ARCHIVE	version=0.0.92	windows_bytes=12977	log_bytes=20824	home_paths_redacted=0
    BACKUP_VERIFY	PASS	version=0.0.92	files=7	repo=.
    ```
- **候选基点 `T` (`96d0c6dd2e`) 与冻结基点 `F` (`e30496b653`) 双基点 18 项棘轮并列实测**(证据文件 `release/evidence/0.0.92/ratchets-T-96d0c6dd2e.txt`、`release/evidence/0.0.92/ratchets-F-e30496b653.txt` 与 `release/evidence/0.0.92/ratchets.txt`):
  ```text
  [T 候选点] SOURCE	frozen-commit	commit=96d0c6dd2eb77b51e4ecba52903df72aefb6a92f	tree=git-archive -> RATCHETS	ok=18	fail=0	elapsed_ms=24084
  [F 冻结点] SOURCE	frozen-commit	commit=e30496b653c35c6b47230329170f45ab2d895ad5	tree=git-archive -> RATCHETS	ok=18	fail=0
  ```
- **冻结前分支复核实测**(基点: 合入 `origin/main` `e2301bad8c` 且切 0.0.92 槽后的 `v0.0.92` `HEAD=56e49da1d3`):
  - `11-version-slot`: `VERSION_SLOT OK version=0.0.92 target=0.0.92 source=arg expect_self_report=unisacc 0.0.92`
  - `tools/derive.cx -- --check`: `14/14 OK` (`drift=0 refused=0`)
  - `15-gen-all`: `44/44 pass=44 diff=0 error=0`
  - `tools/ratchets.cx`: `RATCHETS ok=18 fail=0` (`tsv-lint: ok: unresolved=10764 baseline=10764 ceiling=13260 errors=0`)
  - `tools/check-tsv-lint.cx`: `CHECK_TSV_LINT checks=60 failed=0` (改前先红 `checks=54 failed=1`)
  - `tools/check-ratchets.cx`: `CHECK_RATCHETS failed=0`
  - `tools/check-asm-attr-reject.cx`: `CHECK_ASM_ATTR_REJECT ok=32 failed=0` (撤位置转交变异 `ok=28 failed=4`)
  - 字节等价复核(30 格 + 17 工具): `tests/check-netregen-bytes.cx` 30/30 (`5 案例 x 6 目标`) 全 `SAME`; 17 份仓内 `.cx` 工具 `-o` 逐字节对拍 `O_EQUIV_17 same=17 diff=0 err=0`
  - `tests/run.cx -- --smoke`: `SMOKE assertions=16 failed=0`

## 产品改动

- 变更口径披露(对照 `0.0.91` 机械 `19/20`、手工 `32`, 基点 `e2301bad8c..96d0c6dd2e`):
  - **机械口径**(`19-chain-preflight`, 排除 `exec/`、`kernel/`): **`0 / 10`**(`src/` 0、`cx/` 0、`seed/` 0、`include/` 0);
  - **手工口径**(含 `exec/` 事实表): **`12` 个 `exec/` 文件**(F1 8 个 + F2 4 个)。
- **F1: 行内汇编(`asm volatile` / `__asm__ __volatile__` / 裸 `asm`)拒绝由 `E1` 迁 `E3` 并报告 `file:line:col` (`842eebbd75`)**:
  - 改 `exec/lex/{f1-template,output-manifest,count-result}.tsv`、`exec/facts/{parse2-extra,k2-gen2-tokens,k2-gen2,k2-libraryenv}.tsv`、`exec/parse2/errors-result.tsv` 共 8 个 `exec/` 文件;
  - 4 种行内汇编形状 `-c` 样本均报 `:2:5: error: not covered: inline asm` (`rc=1`, 无 `.o` 残留), 补齐 `0.0.91` 遗留的缺行列号缺口; U4 函数标签与 `int asm=1` 零回归; 非 `asm` 代码生成 30/30 格与 17 工具 `-o` 逐字节 `SAME`。
- **F2(R75): 静态 `@str` 绑定削减 `tsv-lint` 未解析引用 (`aa70fc41dc`)**:
  - 改 `exec/modelcandidates-manifest.tsv`、`exec/modelsignature-manifest.tsv`、`exec/modelsignature-result.tsv`、`exec/parse2/unarycontrol-manifest.tsv` 共 4 个 `exec/` 文件;
  - 将全仓 `tsv-lint` `unresolved` 由 `0.0.91` 的 `11351` 降至 `10763`(叠 F1 后实测 `10764`), 消除 `0.0.91` 遗留的 `TSV_LINT errors=1` 红灯。
- **F3: `tsv-lint` 硬上限调整至 `13260`、固化基线 `10764` 只降不升并接入第 18 项棘轮 (`74c0c7c43c`)**:
  - 按所有者 2026-11-22 决定, 将 `tsv-lint` 硬上限放宽 20% (`11050 -> 13260`), 同时新增 `build/tsv-lint-baseline.tsv` 固化当前实测基线 `10764` 只降不升;
  - 将 `tsv-lint` 接入 `tools/ratchets.cx` 作为第 18 项执行门(`ok=18 fail=0`), 遍历同步全部消费者(`tools/check-tsv-lint.cx`、`tools/check-ratchets.cx`、`tools/chain-ready.cx`、`tools/README.md`、`tests/coverage.tsv`), 清零旧上限字面量残留。
- **版本序列、台账顺延与脱敏基线 (`1b05a90c86`, `da3e64dbc9`, `5d191311ea`, `fbe594b2a0`, `e30496b653`)**:
  - `build/version-seq.tsv` 与 `build/owner-reqs.tsv` 同步顺延一位(原 0.0.92 执行器内核产品化顺延 `0.0.93`, 后续各版顺延至 `0.0.107`, 补齐 `R21 DEFER 0.0.97->0.0.98`, 无需求被砍);
  - 同步合入 `5d191311ea`(`no-home-path` 0.0.91 CI 日志基线登记、`release-exec`/`ci-early` 归档脱敏与 `pkg-gaps` `SY-SPAWN-EXTRA-FD`)、`fbe594b2a0`(`R79` C 类工具线提交) 与 `e30496b653`(`uaf-census` `dup-census` 去重)。

## 已知缺口

### 未跑范围(`UNMEASURED`)
- (1) 全套 `s9` `daily`/`release` 长跑未运行, 整体裁决保持 `NOT_QUALIFIED`;
- (2) 严格预跑 459 批中 2 项批(`cx-booter-ape` 需 `{booter-raw-elf}`、`tools-default-regression` 需 `{old-checker}`)在 `tools/affected.cx` 中无自动生产者(`prepare-failed=no-producer`), 记 `UNKNOWN rc=3` 并附 `release/evidence/0.0.92/cx-booter-ape-deferred.tsv` 与 `release/evidence/0.0.92/tools-default-regression-deferred.tsv` 登记入 `release/evidence/0.0.92/expected-unmeasured.tsv`;
- (3) `arm64`/`osx`/`win` 原生运行除 `ci-early` (`45004230169`) 与发布 CI (`candidate-native.yml`) 6 格验证外, 本地 `x86_64` 宿主未作跨平台原生全量跑批。

### 已知产品与工具缺口
- (1) `R76`(`ext/net/httpd.cx` B 类明文 HTTP 服务端)仍在 `main` 推进, 不进本版产品候选;
- (2) `tsv-lint` 增长源治理(`control-manifest` section 静态展开与按表只降不升基线)留待后续版本推进; 当前已由第 18 项棘轮锁死总基线 `10764` 只降不升;
- (3) `R69.6` 零断言守卫第二阶段十批试点排 `0.0.93`, 尚未对新批强制拒绝;
- (4) 真实程序前端已知缺口: 文件域不完整 `extern` 数组(`A`)与括号内左值后缀自增/自减(`D`)排 `0.0.94`; `stdlib.h` 分配函数 `size_t` 签名(`M`)排 `0.0.95`; 全局对象重声明类型/尺寸冲突漏检(`G-redecl`, `exec/parse2/control-result.tsv:1802-1817`)与函数指针初始化器签名漏检(`M-fnptr-init`)已登记排 `0.0.95+`。
