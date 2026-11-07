# 发布签名：契约与配置

> 现状（2026-10-02）：Windows 企业 Authenticode 签名与 Apple 签名、公证都已在发布流程中常规执行（自 0.0.10 起；最近一次 v0.0.20，回执 research/r20-release-acceptance.json）。`signing-policy.json` 为 `required`。操作步骤见已归档的 [RELEASE-PIPELINE.md](../archive/plans/2026-11-01/RELEASE-PIPELINE.md)（只对旧 C 链有效）；本文只保留收据格式、身份配置、检查脚本与 keychain 边界。0.0.9–0.0.10 的“延期 / 资格”叙述已移到 [archive/release/README-0009-0010.md](../archive/release/README-0009-0010.md)。

## Windows 签名

### 本地封存输入契约

以下是 qualification 与 company 签名共用的输入契约（示例取自历史 0.0.9 回执，字段不变）；每个版本须用当时冻结的版本与源码重新生成，不能继承上一版回执。

先完成候选字节与独立构建证据；**Draft/发布验收以 GitHub CI（含 macos runner / release-check）为准**，不以私有 Darwin 宿主上线为硬依赖。不要为了测试 push。
源码和工作树必须干净；签名调度只能使用当前 `main`，调度 workflow 的
`GITHUB_SHA`、checkout、远端 `main`、回执与 CI SHA 必须相同。

主人在已有草稿 Release 上传两个文件（此工作流不创建草稿）：

1. `unisacc-unsigned.zip`：仅含根目录 `unisacc.com`，没有目录、重复成员、
   符号链接、加密成员或额外文件。
2. `unsigned-receipt.json`：如下 schema；主人计算并审核它的完整 SHA256，
   用作 `receipt_sha256` 调度参数。文件最大 64 KiB，ZIP 最大约 16 MiB。

```json
{
  "schema": 1,
  "kind": "unisacc-owner-local-build",
  "source_sha": "<40 lowercase hex>",
  "source_dirty": false,
  "built_by": "owner-local",
  "local_gate_complete": true,
  "product_version": "0.0.9",
  "local_build": {
    "host": "<local build host label>",
    "created_at": "<UTC ISO8601>",
    "toolchain_sha256": "<64 lowercase hex>"
  },
  "upstream": {"run_id": 123, "run_attempt": 1},
  "archive_sha256": "<SHA256 of exact ZIP>",
  "assets": {
    "unisacc.com": {"sha256": "<SHA256 of exact local file>", "bytes": 123456}
  }
}
```

`local_gate_complete` 是主人的本地构建/门禁声明字段，必须对应保留的证据；
工作流验证封存声明和字节一致性，不会重新执行本机门禁。`upstream` 必须是
本仓库 `.github/workflows/release-check.yml` 在 `main` 上已经成功、与RC源码匹配的精确 attempt，
以 `windows-signing.yml` 的真实校验为准。另行调度的 `ci.yml` 是源码安全网，不能替代封存候选的六原生pull／执行证据。
**正式 Draft/发布验收以该 CI 成功 attempt 为准**（含 macos runner）；CI 不是发行二进制的生产者，但**替代**「须等私有 Darwin 宿主上线才开 Draft」的旧口径。
`product_version` 必须精确等于源码 `src/version.h` 中唯一的 `UNISACC_VERSION`；
未知版本或仅形状合法但不匹配的版本拒绝。ZIP原始名称必须没有NUL截断，成员只接受
普通文件和 STORED/DEFLATED 压缩；上游API响应的 run id/attempt、下载前后资产 id、
实际下载大小也逐项匹配，避免将替换后的资产记成原来的封存id。
调度参数为 `mode/source_sha/seal_tag/receipt_sha256/upstream_run_id/upstream_attempt`。
回执绑定草稿 Release id、两个资产 id 和主人审核的回执 SHA；下载后重新验哈希。

### 配置：一个仓库一个 Entra 身份

复用公司的 Artifact Signing account/Public Trust profile；**不能复制 minicon
的 Entra application/service principal 或 AZURE_CLIENT_ID**。unisacc专属无密码
Entra app/SP及本仓库 `release-signing` Environment 已配置并读回核验，
绑定GitHub实际返回的immutable repo/environment OIDC `sub`，
issuer 为 `https://token.actions.githubusercontent.com`，audience 为
`api://AzureADTokenExchange`，只授予 profile 范围的
`Artifact Signing Certificate Profile Signer`。

已读回核验profile级Signer授权、联邦配置、主人审核与main-only分支约束，
并设置身份验证标志；依据为 [PRD微软身份接入记录](../prd.md)。
正式 OIDC 登录与企业服务签名自 0.0.10 起在每次发布中实际执行，policy 为 required（最近一次 v0.0.20，company run 36933254966）。
以下配置名称保留供资格核对；检查时只列名称，不输出值：

| 类型 | 名称 |
| --- | --- |
| Environment secrets | `AZURE_CLIENT_ID`, `AZURE_TENANT_ID`, `AZURE_SUBSCRIPTION_ID` |
| Environment variables | `ARTIFACT_SIGNING_ENDPOINT`, `ARTIFACT_SIGNING_ACCOUNT`, `ARTIFACT_SIGNING_PROFILE` |
| Environment variable | `UNISACC_REPO_IDENTITY_VERIFIED=true`，仅在主人核对独立 Entra 身份、精确 OIDC subject 与 profile RBAC 后设置 |

`UNISACC_REPO_IDENTITY_VERIFIED` 是部署检查的显式声明，不能证明 Azure 对象已
存在；正式 Azure 登录与实际签名仍须成功。GitHub 找不到 Environment 时可能自动
创建空 Environment；空 secrets/variables 会在登录和服务调用前硬失败。
没有实际配置成功回执时，不声称部署完成。

不要猜 legacy subject；核对本仓库实际 OIDC 格式。使用 immutable claims 的仓库
subject 含 owner/repository 数字 ID；旧仓库可能仍是 legacy 格式。
这些标识、provider coordinates、令牌与私钥不进入源码或公开签名回执。
工作流不读取证书私钥，没有 PFX、client secret 或账号密码回退。

### Canonical inspection scripts（已接入）

参考源位于本机技能目录：

- `~/.claude/skills/sign-windows-artifacts/scripts/inspect-authenticode.ps1`
- `~/.claude/skills/sign-windows-artifacts/scripts/inspect-authenticode.sh`
- `~/.claude/skills/sign-windows-artifacts/scripts/fetch-microsoft-trust-bundle.sh`

产品副本已接入 `scripts/` 同名文件，保持
`pns-authenticode-inspector/v3` 字节一致；实际验签仍须在最终资产上执行。
Windows调用为：
`pwsh scripts/inspect-authenticode.ps1 -Path FILE -ExpectedProductName Unisacc
-ExpectedProductVersion VERSION`（同一命令行）。Portable诊断调用为：
`bash scripts/inspect-authenticode.sh --ca-file PRIVATE_BUNDLE -- FILE`。
trust bundle fetch helper独立生成私有bundle，shell inspector不自动调用它；
不要把bundle内容或公司配置写进公开回执。技能中的
`scripts/check-product-inspectors.sh REPO_ROOT` 检查这三个副本的完整字节一致性。
三个副本已提交进仓库；最终冻结仍需保留字节一致性及实际验签回执。

### 私有keychain操作边界

仅使用调用者明确选定的私有专用keychain；本机实测仅传 `--keychain` 不足以避免
系统密钥服务等待，需临时将该链前置user searchlist，同时保留原有全部项目。
操作前保存搜索表，任何成功、失败或超时路径均在finally恢复，并读回核对原表一致；
限时命令清理所属进程。不得导出既有私钥、修改login钥匙串ACL或输出密码、API key、
Key ID及公证提交标识；这些只在私有环境处理。一次诊断成功不替代最终载荷的签名
与信任回执，也不以关闭Gatekeeper或去掉quarantine作为通过条件。

## M11：单表候选与只读演练

`release/candidate.tsv` 是候选身份与资产 pins 的唯一来源，严格为表头加一行候选，
21 列依次为 `repo, tag, commit, version, workflow, workflow_ref, ci_run_id,
ape, ape_sha256, package, package_sha256, win_arm64, win_arm64_sha256,
record, record_sha256, build_alias, build_alias_sha256, zip, zip_sha256,
checksums, checksums_sha256`（实际文件以 TAB 分隔）。不要另建读回 pins 表，
也不要在命令或源码中另填版本、候选提交、资产名或摘要。共享 `RcCandidate`
加载器拒绝多候选、错误表头、列数、摘要及不安全或重复的资产名。

布局入口与读回入口使用同一张表：

```sh
ROOT=/absolute/path/to/unisacc
TABLE="$ROOT/release/candidate.tsv"
SOURCE=/absolute/path/to/four-candidate-assets
NEW_OUT=/absolute/path/to/absent-layout
OLD_APE=/absolute/path/to/previous-unisacc.com
READBACK_OUT=/absolute/path/to/absent-readback
sh "$ROOT/unisacc.com" "$ROOT/tools/release-layout.cx" -- --candidate "$TABLE" "$SOURCE" "$NEW_OUT"
sh "$ROOT/unisacc.com" "$ROOT/tools/release-layout.cx" -- --candidate "$TABLE" --check "$NEW_OUT"
sh "$ROOT/unisacc.com" "$ROOT/release/tools/c47-1-readback.cx" -- "$ROOT" "$TABLE" "$READBACK_OUT" "$OLD_APE" true
```

`SOURCE` 四项按表中 APE、package、win_arm64、record 文件名放置；生成先核对
这四项 SHA256，不改写源目录，再生成其余三项。生成结束及 `--check` 均核对
全部七项。六项外部 manifest 按角色顺序列出，不包含 manifest 自身。
ZIP 外部名来自表，内部成员仍严格为 `slice-win-arm64`、`pkg.bin`、`run.cmd`、
`README-win-arm64.txt`。

读回 CLI 为 `ROOT TABLE NEW_OUT OLD_APE true|false`；`NEW_OUT` 必须绝对且不存在。
它先绝对化表路径，再从表中仓库与 tag 下载四项，直接以 download 目录作布局源，
无需 canonical 副本。末参 `true` 要求 Draft，`false` 要求非 Draft 并读回 tag
对应提交；两者均断言非 prerelease、tag 与目标提交一致。它保留版本、smoke、
COMM、外部 SHA 与 ZIP 完整成员断言，每个原有子命令保留 `.stdout`、`.stderr`、
`.status` 回执（含退出码及 residue），失败即拒绝，不上传或修改 Release。
旧 `c46-3-readback.sh` 仅保留历史用途，不参与此单表接口。

M11 调度接口仅接受 `--dry-run TABLE ASSETS`：加载同一张表并校验七项后，
只打印 Draft 创建、七项上传计划、workflow/ref、输入
`ape_sha256/commit/release_tag/ape_url` 与已有 CI run id。dry-run **不对外执行**：
不调用 gh/git 网络操作，不创建 Draft、不上传、不发布，也不实际 dispatch；
打印的计划不代表远端状态或 CI 成功，其他模式必须拒绝。

命令级验收由集成者在私有目录统一执行：上述生成与检查命令都须退出 0，
读回须输出 `C47_1_READBACK PASS`，并保留每步成功回执；正式非 Draft 读回
使用新的输出目录和末参 `false`。篡改任一源资产后生成必须失败，篡改输出
七项中任一项后检查及 dry-run 必须失败；多候选、错误表头或列数、非法摘要、
重复或不安全资产名必须在共享加载阶段失败。dry-run 成功只验收本地七项与
计划输出，不能替代真实远端读回或已有 CI 证据。

本轮私有实现已用现有候选七资产实际运行 dry-run；完整 Draft 读回实跑退出0，
包含四资产 SHA、版本、smoke、COMM、布局生成/检查、外部校验和及 ZIP 成员。
`tools/check-release-dispatch.cx` 的六个拒绝合同覆盖：坏资产、缺失派生 ZIP、
多候选、不安全资产名、workflow 禁止的标签及非 dry-run 模式；拒绝时不得输出计划。
`tools/check-release-layout.cx` 保留八项检查；ZIP 附加字节负控会重钉测试候选的
全部七项摘要并先证身份匹配，仍须因固定结构不符而拒绝，不能让摘要门遮蔽结构覆盖。
独立旧布局生成的微型黄金表在 `release/testdata/layout-candidate.tsv`，仅供隔离
格式检查，不是第二份产品候选 pins。生产工具行数按当前实际计数写基线，不预留。

