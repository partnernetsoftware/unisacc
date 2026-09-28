# 发布签名准备

## Windows：先资格检查，再公司签名

[windows-signing.yml](../.github/workflows/windows-signing.yml) 只消费主人在本机
构建、测试并封存的资产。Actions 不构建产物，也不发布 Release。
[signing-policy.json](signing-policy.json) 当前为 `off`，签名清单只有
`unisacc.com`。任何额外 PE/DLL 必须先明确加入清单、预算与原生运行法院。

两种 dispatch mode：

- `qualification`：验证确切主线源码、成功 CI run/attempt、本机回执、ZIP
  清单、SHA256、字节数与有界 APE 包位置。不会进入 `release-signing`
  Environment，不请求 Azure OIDC、不调用公司签名服务，签名额度消耗为零。
  回执中的 `versioninfo_ready=false` 是尚未满足公司签名前提；此模式没有产生
  新签名，也没有证明 Azure 身份已部署。
- `company`：要求策略改为 `required`、PE 中 ProductName 为 `Unisacc` 且
  ProductVersion 精确匹配封存回执，再进入保护 Environment。缺任何配置、
  OIDC 登录失败、签名失败、Windows 信任或时间戳失败均停止，不退回未签名出货。
  即使签名成功，回执仍为 `release_eligible=false`；当前流程还没有六平台最终
  字节运行、Defender 与主人 Promotion，不能称完整发布验收。

### 本地封存输入契约

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
  "product_version": "0.0.8",
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
的 Entra application/service principal 或 AZURE_CLIENT_ID**。为 unisacc 独立配置
Entra 身份，精确绑定本仓库 `release-signing` Environment 的 OIDC `sub`，
issuer 为 `https://token.actions.githubusercontent.com`，audience 为
`api://AzureADTokenExchange`，只授予 profile 范围的
`Artifact Signing Certificate Profile Signer`。

`release-signing` Environment 应要求主人审核并限制 `main`。配置以下名称；
检查配置时只列名称，不输出值：

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

### 签名证据与后续门槛

公司流程在 `windows-2025` x64 上以 SHA256、微软 RFC3161 timestamp 签清单中的
确切文件。签前输入先存入 immutable Actions artifact；签后用 Windows
`Get-AuthenticodeSignature` 要求 `Valid`、公司 `O=`、签名者与时间戳证书、
VERSIONINFO 不变、大小在预算内，并用 [apeinspect.py](apeinspect.py) 验证模型包
SHA256 不变。格式检查不是认证判定，Windows 是权威信任法院。

公开签名回执保留 source、local build、上游/签名 run attempt、封存 id、
签前后 SHA/bytes、subject/issuer/thumbprint/validity、时间戳证书事实与服务 URL。
时间戳证书有效期不是实际 RFC3161 genTime；当前回执不声称已提取 genTime。
provider/OIDC coordinates 不进入回执；公司诊断材料另存受限位置。

待办：真实仓库身份与 Environment 部署；VERSIONINFO 资源；实际服务签名与信任
回执；签后同一 SHA 的全部六平台运行、Windows Defender、当前 attempt 的完整
汇总，以及主人审核后的封存字节 Promotion。此片没有创建外部资源、push 或实际
云签名。每个 workflow step 最多一分钟，服务签名 timeout 为 55 秒；超时即失败。

依据（2026-09-28 核对）：
[官方 Artifact Signing action](https://github.com/Azure/artifact-signing-action)、
[微软集成说明](https://learn.microsoft.com/en-us/azure/artifact-signing/how-to-signing-integrations)、
[GitHub OIDC reference](https://docs.github.com/en/actions/reference/security/oidc)。

### Canonical inspection scripts（父代理接入）

参考源位于本机技能目录：

- `~/.claude/skills/sign-windows-artifacts/scripts/inspect-authenticode.ps1`
- `~/.claude/skills/sign-windows-artifacts/scripts/inspect-authenticode.sh`
- `~/.claude/skills/sign-windows-artifacts/scripts/fetch-microsoft-trust-bundle.sh`

产品副本应分别放到 `scripts/` 同名文件，保持
`pns-authenticode-inspector/v3` 字节一致。Windows调用为：
`pwsh scripts/inspect-authenticode.ps1 -Path FILE -ExpectedProductName Unisacc
-ExpectedProductVersion VERSION`（同一命令行）。Portable诊断调用为：
`bash scripts/inspect-authenticode.sh --ca-file PRIVATE_BUNDLE -- FILE`。
trust bundle fetch helper独立生成私有bundle，shell inspector不自动调用它；
不要把bundle内容或公司配置写进公开回执。技能中的
`scripts/check-product-inspectors.sh REPO_ROOT` 检查这三个副本的完整字节一致性。
此片只列引用路径，没有复制或修改这些脚本。

## macOS 私有演练

[macosbundle.py](macosbundle.py) 与 [macos-launcher.c](macos-launcher.c)
用于本机分片构建/签名/DMG/评估：签原生入口和 app，内层 `.com` 的封存哈希独立
验证。具体参数见工具 `--help`。本机签名不是公证成功；尚需 quarantine/Gatekeeper、
notarization/staple 与六平台最终字节证据，不能称 Apple 已签内层 `.com`。
