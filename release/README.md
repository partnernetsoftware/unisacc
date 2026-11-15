# 0.0.68 发布入口与收据合同

本文件说明 0.0.68 的发布顺序与证据归属，不是发布完成声明。当前 `release/candidate.tsv`、`decision.txt`、`evidence.tsv` 仍是上一版记录；冻结与整链确定后由本轮真实产物重写，不改名沿用旧收据。`rollback.txt` 是本版待填草稿，所有待定值统一使用约定的待填占位符；发布前逐项核对，公开后的回执与 Y2 身份只能在实际产生后补记。

主调度 cc 负责集成、冻结、构造、对外发布与装机；cdx 按派工准备本目录文档与证据。所有读写、命令与提交经仓开发服务 `tools/devsvc.cx`，共享检出不切分支，只提交自己的路径。版本文档在 `.claude/worktrees/v0.0.68` 修改，公开前由 cc 集成；不改正在执行的冻结根或其 RUN。

## 六个文件各自记录什么

| 文件 | 本版内容与生成时点 |
|---|---|
| `candidate.tsv` | 候选与资产 pins 的唯一来源。打包后计算摘要，X 只提交本表；同候选 CI 实际完成后在 Y1 回填精确 run id。 |
| `evidence.tsv` | 八步的本版 verdict 与证据入口。Y1 保留尚无公开读回的 UNKNOWN，公开后 Y2 才更新第 8 步。 |
| `decision.txt` | 对当时候选与证据实跑 `tools/release-decide.cx` 的输出，Y1/Y2 分别生成。任何缺证据步不得猜 PASS。 |
| `rollback.txt` | 公开前声明撤回到 0.0.67 的动作、保留范围、完整回退摘要及真实 X/CI 身份；公开后补 Y2 与读回回执。 |
| `freeze.tsv` | 历史三列冻结窗口登记，保留旧行；0.0.68 的真实冻结 REV 定下后再追加。旧消费者已在 `archive/testing-c43-legacy/`，本表不冒充现行冻结门通过证明。 |
| `README.md` | 本流程、工具接口和证据边界。实际状态以候选表、原始执行回执及裁决为准。 |

仓内 `release/freeze.tsv` 与准备器状态目录的 `freeze.tsv` 是不同合同。后者由 `tools/cxlib/releaseprepare.cx` 生成，绑定 `BUILD_REV`、`source_sid`、构造子路径和 `constructor_sha256`；不能用其键值格式覆盖仓内的历史三列表。

## 候选身份与六资产

`candidate.tsv` 严格为一行表头加一行候选、21 列 TAB 分隔，列序保持:

```text
repo tag commit version workflow workflow_ref ci_run_id ape ape_sha256 package package_sha256 win_arm64 win_arm64_sha256 record record_sha256 build_alias build_alias_sha256 zip zip_sha256 checksums checksums_sha256
```

`repo` 为 `partnernetsoftware/unisacc`，本版 `tag/version` 为 `v0.0.68/0.0.68`。`commit` 绑定发布标签实际指向的完整提交，`workflow/workflow_ref` 绑定实际调度的 `candidate-native.yml` 与分支；`ci_run_id` 来自本轮实际 CI，不复制 0.0.67 的值。源码冻结 REV、整链 BUILD_REV 与发布 F 提交的对应关系须在证据中写清，并实测源身份一致，不能只凭版本号认定等价。CI 的输入 commit X 与候选源码提交分别核对，不混为一个字段。

本版发布六件，不发布 ZIP:

(1) `unisacc.com`。
(2) `pkg.bin`。
(3) `slice-win-arm64`。
(4) `unisacc.com.build.json`。
(5) `build.json`，与构建记录别名字节一致。
(6) `SHA256SUMS`，覆盖前五件；其自身摘要钉在候选表。

`zip` 与 `zip_sha256` 两列为 `-`。所有摘要由实际文件计算；源 SID 不增加为第 22 列，而由整链快照、`sources.tsv`、构建记录中的 `sources_sha256` 和冻结收据交叉核对。`RcCandidate` 加载器继续拒绝多候选、错误表头或列数、非法摘要、不安全或重复资产名。0.0.62 及更早版本的七资产与 ZIP 合同留在兼容工具和检查中，不套用到本版，也不删除旧版覆盖。

## 从冻结到 Y2

(1) cc 集成全部修复并固定 REV，先跑冻结门。历史配方见 `archive/prd_mempalace-2026-11-14.md` 的“起整链的做法”，当前发布门与差距见 `plans/release-gates-0.0.68.md`。核 bootstrap 0.0.59 及配套构建记录、快照与 `cxroot.tsv`、身份 helper、真实在途作业和截止。历史配方要求起链只设 `UNISACC_IDENT_HELPER`，不设 `UNISACC_RUN_CACHE` 或 `UNISACC_CACHE_DIR`，不能把已经实测否定的环境写法重新带入本轮。
(2) 正规整链产生 `chain-run.identity`、`chain-run.tsv`、`checkpoint.tsv`、固定点收据与最终打包输入。公开硬门要求 compare DONE、最后一次 compare attempt 的 rc=0、residue=0、memo=off，objects/kernels/slices/product 均 same，g2/g3 六目标切片 SHA256 逐项相等。仅有 pack 产物或 PREPARED 不代表门已过。
(3) 从实际被构造 REV 取得发布源码 F，核 F 与构造输入的源身份，再绑定发布标签和源码标签。`tools/pack-srcdir.cx` 从整链打包收据装配全新输入目录；`tools/release-run.cx -- --pack ROOT TABLE SRC_DIR NEW_OUT --chain-out CHAIN_DIR [--snap SNAP]` 核整链、打包、注入版本、生成布局并回填候选资产摘要。ROOT 必须有匹配的候选表与产品，输出目录必须全新。
(4) cc 提交 X，只含已核实的 `release/candidate.tsv`，创建草稿并上传六资产。用同一张表核远端草稿身份与资产，调度同候选 CI；记录 X、run id、attempt、输入参数、候选 SHA 与每个作业的实际结论。旧 run、普通源码 CI 或计划输出不能替代本轮同候选 CI。
(5) cdx 按真实证据准备 Y1:候选表回填 CI id，重写八步 evidence 与 decision，填发布说明和 rollback 中此时已产生的身份。公开读回尚未发生时，第 8 步保持 UNKNOWN。cc 核对公开硬门、同候选 CI、草稿读回、回滚声明和缺口披露后提交并推送 Y1。
(6) cc 公开并置 Latest，再用全新目录下载六件，核公开状态、标签提交、资产身份、全部摘要、版本、smoke 与 COMM 等原有合同；实际通过才在 Y2 更新第 8 步与裁决，并记录公开时间、实际 Y2 提交和装机回执。发布页正文与说明文件须读回比对，不把上传命令成功当作公开读回成功。
(7) 公开读回后才装机并做装后检查；保留 0.0.67 回退件及其完整 SHA。清理在装机和基线取数之后，按真实进程与保留条件串行做。本文不授权清理任何正在用的根、RUN 或他人产物。

0.0.67 的分段流水可核对 X `9e54f512a8`、Y1 `f3849c6b94`、Y2 `2b6a9804b5`；它们是结构参考，所有身份、收据和结论在本版重测。

## 八步证据与资格

`evidence.tsv` 每行是 `step<TAB>verdict<TAB>path`，verdict 只用 PASS、FAIL、UNKNOWN:

| 步 | 名称 | 必须来自 |
|---|---|---|
| 1 | freeze-sid | 本轮冻结 REV、实算 SID、构造子摘要及冻结门回执。 |
| 2 | construct | 同输入正规整链、checkpoint、compare 及 g2/g3 收据。 |
| 3 | ape-pack | 本轮 pack 状态、输入来源、六资产摘要与构建记录。 |
| 4 | s9 | 同候选、同登记与源身份绑定的会话聚合及 verdict；不迁移旧产品或不同输入的结论。 |
| 5 | layout-check | 同表同资产的 `release-layout --check` 与校验和检查。 |
| 6 | draft-assets | 本轮远端草稿及实际下载资产的身份和完整性检查。 |
| 7 | ci-qualified | 本轮精确 run/attempt、输入与作业原始结果；仅 pending 不算通过。 |
| 8 | readback | 公开后实际下载与语义检查；Y1 尚未产生该证据。 |

用 `tools/release-decide.cx -- --candidate TABLE --evidence EVIDENCE --assets DIR` 生成 `decision.txt`，保留 stdout、stderr 与退出码。rc=0 本身不表示 QUALIFIED，必须核输出裁决。D1 允许如实披露缺口后公开，不放宽冻结门、整链、公开硬门或同候选 CI，也不把 UNKNOWN/FAIL 改成 PASS；未取得完整资格仍写 NOT_QUALIFIED。

daily 聚合与 release 的 `RELEASE_ONLY` 是独立结论。F4c 只在显式封存的 daily/no-candidate 会话按定义判不适用，发布说明披露 `S9_NA reason=no-candidate-material n=N` 及批名；有候选的会话缺同一材料不得借此改标。无候选摸底会话不能当作本版候选全批资格。

## 可复用工具与读回边界

`tools/release-oneshot.cx`、`tools/cxlib/releaseprepare.cx` 可准备并封存输入；已有整链只能按其严格 identity/checkpoint/产物合同接入。`release-run --pack` 是本地打包，`release-dispatch --dry-run` 只验计划，不创建草稿、不上传、不公开，也不证明 CI 成功。`release-layout.cx` 与 `release/tools/c47-1-readback.cx` 共用候选表，不另建 pins 表。

读回接口为 `ROOT TABLE NEW_OUT OLD_APE true|false`，NEW_OUT 必须绝对且不存在；`true` 要求草稿，`false` 要求非草稿并核标签提交。保留资产 SHA、布局、版本、smoke、COMM 和原有逐命令 stdout/stderr/status(含 rc 与 residue)断言。任何失败具名拒绝，不上传或修改 Release。

当前 `tools/cxlib/releaseexec.cx` 的第 8 步证据取 draft-readback 状态，不能直接将其当作公开读回 PASS；本版按 X/Y1/Y2 分段更新公开证据。准备器或 dry-run 的 PREPARED、已有缓存与计划行都不能缓存为测试 PASS。

## 旧签名流水与凭据边界

旧 C 链的签名流水已归档在 `archive/plans/2026-11-01/RELEASE-PIPELINE.md`，历史资格说明在 `archive/release/`。旧“必须签名、公证才可公开”的要求已被后续决定撤销，不能作为本版新增公开门。保留既有签名工具与配置；若本版实际执行签名或公证，须保留最终资产及其真实检查回执，不因配置存在或历史成功便声称本版已签。

仍适用的安全合同:不复制其它仓库的 Entra app/SP；不猜 OIDC subject，核实际不可变仓库/环境身份与最小 profile 授权；不导出或写入令牌、私钥、密码、公司坐标和私有 keychain 内容。调度所用精确源码身份、封存资产 id/大小/摘要与 CI attempt 须一致，下载前后核资产身份，不以普通源码 CI 替代封存候选证据。

如需私有 keychain，只操作选定的专用链；操作前保存搜索表，成功、失败或超时都恢复并读回核对，不改 login keychain ACL，不以关闭 Gatekeeper 或去 quarantine 代替验证。检查配置只列名称，不输出秘密值。
