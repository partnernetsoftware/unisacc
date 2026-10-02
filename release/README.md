# 发布签名：契约与配置

> 现状（2026-10-02）：Windows 企业 Authenticode 签名与 Apple 签名、公证都已在发布流程中常规执行（自 0.0.10 起；最近一次 v0.0.20，回执 research/r20-release-acceptance.json）。`signing-policy.json` 为 `required`。操作步骤见 [RELEASE-PIPELINE.md](RELEASE-PIPELINE.md)；本文只保留收据格式、身份配置、检查脚本与 keychain 边界。0.0.9–0.0.10 的“延期 / 资格”叙述已移到 [archive/release/README-0009-0010.md](../archive/release/README-0009-0010.md)。

## Windows 签名

### 本地封存输入契约

以下是 qualification 与 company 签名共用的输入契约（示例取自历史 0.0.9 回执，字段不变）；每个版本须用当时冻结的版本与源码重新生成，不能继承上一版回执。

先完成本机本轮全部门禁、六平台运行与独立构建证据。不要为了测试 push。
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

`local_gate_complete` 是主人的本地验收声明，必须对应保留的本机门禁证据；
工作流验证封存声明和字节一致性，不会重新执行本机门禁。`upstream` 必须是
本仓库 `.github/workflows/ci.yml` 在 `main` 上已经成功的精确 attempt。
CI 是源码的第二意见，不是这些本机构建字节的生产者，也不替代本地证据。
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
