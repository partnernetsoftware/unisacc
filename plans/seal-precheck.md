# 封版准入机械核对

`sh ./unisacc.com tools/seal-precheck.cx -- EVIDENCE_DIR` 逐条输出七行 `SEAL_GATE`，末行汇总 `SEAL_PRECHECK`。任何拒绝返回 2。工具只读，不执行发布或授予豁免。

在证据目录放置候选的 `strict.tsv`、发布用 `expected-fail.tsv`、`expected-unmeasured.tsv`、状态表 `state-unverified.tsv` 和 P0 账本 `p0-ledger.tsv`。严格表逐批重计并核汇总，PASS 必须绑定该 APE。预期失败和已登记 no-producer 的 UNKNOWN 单列计数，门仍要求 FAIL、UNKNOWN、INVALID 均为 0。空的预期表应保留表头。

P0 账本支持现有五列与在其后追加 `state`、`closure_evidence` 的七列格式。五列的 P0 一律按未关闭计数。七列 CLOSED 须给出目录内已落盘的证据文件。未验证项按 kind 报数，声称类按 R37 计，另计 R38 的 U21；上限 3。

清单定稿后由集成者写入 `checklist.tsv`，表头为 `item category batch`（制表符分隔）。项目编号 1 至 30，分类 2.1 至 2.5 分别占 8、8、6、4、4，批次键各自唯一并须出现在严格结果里。实得 PASS 至少 25 项。

`seal-evidence.tsv` 表头为 `kind key value ape_sha256 ref`（制表符分隔）。引用为证据目录内相对文件路径。集成者负责从原始归档核定连续发布序、CI 资格结果、前端完成状态与两个公开时钟。工具核表结构、目标全集、声明身份、引用存在及数值判据。CI 日志和公开事件记录的语义由集成者核定。

(1) `release` 依发布序提供三行，键为版本，前两行 PUBLISHED，末行可为 CANDIDATE 或 PUBLISHED；引用 `fixedpoint-6targets.tsv`。逐文件检查六目标、相对路径、g2/g3 摘要相等与 SAME，并核末行 APE 绑定严格结果。

(2) `ci` 每个目标一行，键为 `lnx/x86_64`、`lnx/arm64`、`osx/x86_64`、`osx/arm64`、`win/x86_64`、`win/arm64`，值为 QUALIFIED，摘要为当前 APE，引用对应 CI 归档证据。

(3) `frontend` 分别给 A、B1、B2，值 PASS，摘要栏填 `-`，引用对应完成证据。

(4) `timing` 按相同版本序填三行，值为 `START_EPOCH/END_EPOCH`（秒），摘要栏填 `-`，引用起链与公开时钟记录。每趟上限 7200 秒，待公开时此门拒绝。
