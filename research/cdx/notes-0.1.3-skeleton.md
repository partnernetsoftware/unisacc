# unisacc 0.1.3 发布说明骨架

状态 `<RELEASE_STATUS>`

## 分支提交

以下为工作树中 `git log --oneline refs/heads/v0.1.2..refs/heads/v0.1.3` 的原始输出。显式分支引用消除同名发布标签歧义。冻结后用同一范围重新提取。

```text
warning: refname 'v0.1.2' is ambiguous.
fc0b3555b v0.1.3 T3.6-1: add --nn-dir and --no-external-nn in cx/driver/cli.cx (keep CxcState==144)
80d0778b9 v0.1.3: keep historical text in research and release-note docs, add fixed-commit refs for 127 moved paths instead of current-path links
fb01a0714 v0.1.3: restore post-publish-verify-106 historical worktree paths and cite 2e8a19a342:release/candidate.tsv as the fixed reference
af0276d05 v0.1.3: restore cx-layers history text and point the migration sources at fixed commits (3aae1b769 kernel/process/lnx/winprocess, fab2451e9 deflate)
becfe3430 v0.1.3: stream-pack.cx is not archivable (executed by tools/check-streampack.cx:82,123 and listed in tests/coverage.tsv:358); classify 96 tools as kept-on-evidence and record the evidence column
ffdb8fd3c v0.1.3: merge origin/main (v0.1.2 post-publish: thaw 0.1.2-FREEZE, version-seq public)
b540dd766 v0.1.3: classify 181 check-less tools: 28 in batches, 1 archive candidate (stream-pack.cx), 0 byte-identical duplicates
36ed4bc63 v0.1.3: scrub 64 home-path lines in research and plans (before files=4 lines=64, after 0)
387184426 v0.1.3: repoint 290 moved doc links, record 346 unresolved in broken-links-residue.tsv
7eb0d06a4 v0.1.3 prep: land pending research and skills from main checkout
```

## 新增 CLI 选项

(1) `--nn-dir DIR` 和 `--nn-dir=DIR` 解析显式 NN 目录。缺少参数或等号形式的值为空时，解析返回 `missing nn dir`。来源 `fc0b3555b:cx/driver/cli.cx` 的 `cxc_step`。实际优先级、回退与拒绝规则待由 `<NN_SELECTION_COMMIT>` 和 `<NN_SELECTION_EVIDENCE>` 填入。

(2) `--no-external-nn` 记录禁用外部 NN 的标志。来源 `fc0b3555b:cx/driver/cli.cx` 的 `cxc_step` 与 `cxc_no_ext_nn_get`。实际效果待由 `<NN_SELECTION_COMMIT>` 和 `<NN_SELECTION_EVIDENCE>` 填入。

## 候选身份与验证

| 字段 | 机器提取占位 | 证据入口 |
|---|---|---|
| 冻结、X、Y1、Y2 | `<FREEZE_COMMIT>` / `<X_COMMIT>` / `<Y1_COMMIT>` / `<Y2_COMMIT>` | Git、发布证据 INDEX |
| APE 摘要和大小 | `<APE_SUMMARY>` | `ape-receipt.txt`、候选构建记录 |
| 源快照身份 | `<SOURCES_SID>` | 候选构建记录 |
| 发布资产哈希 | `<ASSET_SHA256_ROWS>` | `release/candidate.tsv`、公开读回 |
| 整链起止、用时 | `<CHAIN_TIMES>` | `owner.tsv`、`exit.tsv` |
| 不动点 | `<FIXEDPOINT_ROWS>` | `fixedpoint-6targets.tsv` |
| 打包与预热 | `<PACK_RESULT>` / `<WARM_RESULT>` | 打包、预热收据 |
| 严格预跑 | `<PRERUN_MERGE_ROW>` / `<STRICT_SUMMARY_ROW>` / `<STRICT_SHA256>` | 合并日志与 `strict.tsv` |
| CI 与同次矩阵 | `<CI_RUN_ID>` / `<CI_ATTEMPT>` / `<CI_MATRIX_ROWS>` | GitHub 运行与矩阵证据 |
| 草稿、公开读回 | `<DRAFT_READBACK_RESULT>` / `<PUBLIC_READBACK_RESULT>` | 草稿与公开读回文件 |
| tape M1 性能门 | `<TAPE_M1_GATE_ROW>` | 同输入原始基准与门结果 |

数字与哈希统一由 `notes-evidence` 从对应证据提取。缺少可用输入时保留占位，待对应证据提交后再填。

## 已知缺口和未测范围

(1) C3 本版拟收口 S1 至 S3 与 S5 最小子集。范围为 L0 所有权和错误路径、绝对期限、消费者收口及最小检查。实际合入范围待 `<C3_SCOPE_COMMITS>` 与 `<C3_CHECK_RESULTS>`。processcore 与 reaper 的 S4 顺延。

(2) macOS 与 Windows 的 `sy_proc_finished` 路径为 `UNCOVERED`。冻结后以 `<C3_PLATFORM_EVIDENCE>` 核实际支持范围。

(3) NN 端到端层为 `UNMEASURED`。解析器提交与端到端证据各自绑定。

(4) tape M1 15 倍门数据待补。实测前保留 `<TAPE_M1_GATE_ROW>`，实际倍率、输入、冷热口径和门结果由基准证据填充。
