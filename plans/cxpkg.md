# cxpkg 库包管理工具计划(快车道工具线,不占版本槽)

依据: `~/.claude/jobs/2798e440/tmp/pkgmgr/design.txt` (fable 2026-11-21 只读提案) 与 `prd_cx.md` 0.9 节。
状态: 规划已定稿(2026-11-21);未实现,未实跑,不得记 PASS。
车道归属: 快车道工具线(只改 `tools/` 与 `tests/`,不进 `unisacc.com`,不触产品输入,A 类文件数 0)。
对应台账: `build/owner-reqs.tsv` `R13a` (MVP 工具快车道)、`R13b` (核心配合,排 `0.0.97` 原 `0.0.96` cxmod 类或其后一号评审)、`R13c` (生态 G1 公开门,DEFER,待 30 天日历条件)。

## 一、定位与真源分工

1. **工具与核心解耦**:
   - 工具 MVP(切片甲至丁及己的兼容校验)放 `tools/cxpkg.cx`、`tools/cxpkg/{man,resolve,lock,store}.cx` 与 `tools/check-cxpkg-*.cx`,脚本旁解析,不进 `tools/cxlib`,避免 `tools.cxlib` 升位。
   - 核心配合(K-a 已装产品零配置解析根改 `cx/cx0/plan.cx` 与 `tools/cxlib/cxroot.cx`、K-c 驱动规划前验锁、K-d 模块语法 `alias`/`export use`)属 A 类产品输入,独立排入 `0.0.97`(原 `0.0.96` cxmod 类,因 U4 独立版 `0.0.90` 顺延一号)或其后一号评审。
2. **与 `build/tlib-packages.tsv` 的分工**:
   - 不将 `build/tlib-packages.tsv` 升格为注册表索引(它是带库构建与冻结真源,含不可对外的 K 环内部包)。
   - 走「事实表 -> 生成器 -> 表」:`tools/cxpkg.cx index --write/--check` 从 `build/tlib-packages.tsv` 加 `cxroot.tsv` 派生 `build/cxpkg-index.tsv` 与各 `ext/<包>/cxpkg.tsv`,K 环包不出索引。
   - 前缀真源迁移期先并存加逐行对拍,不删手写 `cxroot.tsv`。

## 二、MVP 切片表与工作量

| 切片 | 内容 | 写域 | 核心接口/命令 | A 类数 | 估计(加倍口径) | 排期 |
|---|---|---|---|---|---|---|
| 甲 (B1) | 清单 `cxpkg.tsv` (`CXPKG1`: N/V/K/S/D/X/E/C) 解析与校验 | `tools/cxpkg/man.cx`、`tools/cxpkg.cx`、`tools/check-cxpkg-man.cx`、`tests/manifests/cxpkg-man/` | `pm_load(file, m)`; `cxpkg init`; `cxpkg check [DIR]` | 0 | 1 天 | 首批派发 |
| 乙 (B2) | `resolve` 建 `cxpkg/<名>` 软链、生成 `cxroot.tsv` 与 `why` 诊断 | `tools/cxpkg/resolve.cx`、`tools/check-cxpkg-resolve.cx`、`tests/manifests/cxpkg-resolve/` | `cxpkg resolve [--check] [--core DIR]`; `cxpkg why NAME` | 0 | 2 天 | 首批并行(目标 `0.0.91` Z5 冻结前可用) |
| 丙 (B3) | 锁 `cxlock.tsv` (`CXLOCK1`)、包树 sha256 完整性校验与最小版本选择 | `tools/cxpkg/lock.cx`、`tools/check-cxpkg-lock.cx`、`tests/manifests/cxpkg-lock/` | `cxpkg lock`; `cxpkg verify` | 0 | 2 天 | 首批并行 |
| 丁 | 离线/钉提交号安装器(`path:` 与 `git:URL#40位提交号`,无安装脚本) | `tools/cxpkg/store.cx`、`tools/check-cxpkg-install.cx` | `cxpkg install [--frozen]`; `cxpkg list` | 0 | 3 天 | B1-B3 收口后二批 |
| 戊 | 索引与发布(`cxpkg-index.tsv`、search/publish;服务化门槛 50 包 / 10 第三方作者) | 规划 | `cxpkg index --write/--check`; `search`/`publish` | 0 | 4 天 | 后期规划 |
| 己 | 兼容声明校验(S 行对 `tlib-packages.tsv`)与签名验证(`ext.crypto` p256) | 规划 | 兼容先行(1 天),签名后做(4 天) | 0 | 1+4 天 | 兼容随丁后,签名后期 |

## 三、首批三切片验收标准(B1/B2/B3)

共同前置:先跑 `git log --all --oneline -- tools/cxpkg.cx tools/cxpkg` 确认无历史;不碰 `cx/`、`src/`、`ext/`、`cxroot.tsv`、`tests/batches.tsv`、`tests/coverage.tsv` 及 `.claude/worktrees/v0.0.89`;夹具临时文件只写 `$CLAUDE_JOB_DIR/tmp` 或 `/tmp`。

1. **B1 清单读取与校验**:
   - 固定接口(B2/B3 依赖):`int pm_load(const char *file, pm_t *m);` 返回 0 或 1(失败打印 `cxpkg: FILE:LINE: WHY`)。
   - 验收:`sh ./unisacc.com tools/check-cxpkg-man.cx` -> rc 0,末行 `CXPKG-MAN-CHECK cases=11 fail=0`(好清单 3 份、坏清单 8 份逐条断言拒绝文案),加先红证据与 3 个变异均 rc 非 0。
2. **B2 resolve 与 why**:
   - 验收:`sh ./unisacc.com tools/check-cxpkg-resolve.cx` -> rc 0,末行 `CXPKG-RESOLVE-CHECK cases=6 fail=0`(含夹具 resolve 前 `hello` 编不过、resolve 后已装产品实编 rc 0、`cxr_path` 逐前缀可解析、删 X 行 `--check` 报 `drift=1` rc 1、重复 resolve 字节幂等、`why` 未知名 rc 1)。
   - 目标验收对拍:与 `~/repos/moltbaby` 三份根表逐行对拍,在 `0.0.91` Z5 目录迁移前可用于自动生成吸收目录变动。
3. **B3 锁与完整性**:
   - 验收:`sh ./unisacc.com tools/check-cxpkg-lock.cx` -> rc 0,末行 `CXPKG-LOCK-CHECK cases=6 fail=0`(含同树两次哈希相同、改一字节 `verify` rc 1 且含包名与 `want=` `got=`、改回 rc 0、锁多一包/少一包/版本低于 D 最低版本各 rc 1、已知向量与独立 `sha256sum` 一致)。
4. **收口登记(协调者统一执行)**:
   - 三个 check 经 `tools/register.cx` 登记批次、`prd.md` 能力速查加一行、`tools/affected.cx --files ... --run` 验证。

## 四、回滚

- 工具线独立文件,回滚按提交回退 `plans/cxpkg.md`、`tools/cxpkg*`、`tests/manifests/cxpkg-*` 与对应批次行,不影响任何产品输入与已发布版本。
