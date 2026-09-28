# 发布签名接入与剩余验收

历史0.0.9候选的封存载荷 `unisacc.com` 为1,221,182 B，SHA256
`7608a31b2f77038c2ba7516c1ce13c263dcd626343ac9e9825ce5017f273f846`，
构建来源为 `10994c2`。保留此历史身份与证据，尚无其Windows企业签名；
当前修复候选须重新冻结源码和载荷身份，不继承旧资产的验收或签名回执。

主人已明确将**仅微软企业签名延期到0.0.10**：0.0.9的Windows载荷按
unsigned方案验收，0.0.10再进行企业Authenticode签名。Apple公司签名与公证
仍在0.0.9范围；此前资格资产和最终发行资产分别记账。延期不表示已完成发布，
`release_eligible=false`，最终字节运行、Windows Defender与主人Promotion仍须验收。

## Windows：身份与格式已接入，企业签名延期到0.0.10

[windows-signing.yml](../.github/workflows/windows-signing.yml) 只消费主人在本机
构建、测试并封存的资产。Actions 不构建产物，也不发布 Release。
[signing-policy.json](signing-policy.json) 当前为 `deferred`，记录0.0.9 unsigned
与0.0.10企业签名计划；签名清单仍只有 `unisacc.com`，身份、Environment和
workflow接入保留。当前不调用企业签名服务。任何额外 PE/DLL 必须先明确加入
清单、预算与原生运行法院。

两种 dispatch mode：

- `qualification`：验证确切主线源码、成功 CI run/attempt、本机回执、ZIP
  清单、SHA256、字节数与有界 APE 包位置。不会进入 `release-signing`
  Environment，不请求 Azure OIDC、不调用公司签名服务，签名额度消耗为零。
  `versioninfo_ready=false` 时仍未满足公司签名前提；当前候选已实现
  Unisacc/0.0.9的PE VERSIONINFO，资格检查必须从确切资产重新读取。
  此模式不产生新签名；身份配置的读回核验与正式OIDC登录/服务签名分别记账。
- `company`：仅在0.0.10签名计划正式启用、策略改为 `required` 后使用；
  当前 `deferred` 会硬失败。要求PE 中 ProductName 为 `Unisacc` 且
  ProductVersion 精确匹配封存回执，再进入保护 Environment。缺任何配置、
  OIDC 登录失败、签名失败、Windows 信任或时间戳失败均停止；启用 `required`
  后的企业签名发布不退回未签名出货。0.0.9 unsigned来自明确延期决定。
  即使签名成功，回执仍为 `release_eligible=false`；当前流程还没有六平台最终
  字节运行、Defender 与主人 Promotion，不能称完整发布验收。

### 本地封存输入契约

以下保留qualification与未来company签名的输入契约；示例0.0.9可做资格检查，
不调用企业服务。0.0.10正式签名须使用当时冻结版本及源码，不能继承0.0.9回执。

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
正式OIDC登录、企业服务签名与Windows信任尚未验证。此前policy设为required
是历史接入状态；按主人延期决定现为deferred，服务调用留待0.0.10正式启用。
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

身份/Environment配置已读回，产品VERSIONINFO已实现；这些不等于服务签名成功。
0.0.9待办：最终unsigned封存字节的全部六平台运行、Windows Defender、当前
attempt完整汇总与主人Promotion；Apple最终资产的签名、公证和签后法院单独验收。
0.0.10待办：最终封存输入资格检查，政策正式启用required后的OIDC登录、公司
服务签名与Windows信任回执，再验签后同一SHA的全部六平台、Defender和Promotion。
当前policy为deferred，未调用Windows企业签名服务；company workflow保留
required硬条件。每个workflow step最多一分钟，服务签名timeout为55秒；超时即失败。

依据（2026-09-28 核对）：
[官方 Artifact Signing action](https://github.com/Azure/artifact-signing-action)、
[微软集成说明](https://learn.microsoft.com/en-us/azure/artifact-signing/how-to-signing-integrations)、
[GitHub OIDC reference](https://docs.github.com/en/actions/reference/security/oidc)。

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

## macOS 公司签名与公证资格（已通过，非发布）

Apple公司签名与公证仍属于0.0.9交付；Windows延期不改变此范围。以下旧资产
资格证据保留，当前最终资产须按其实际源码、载荷与封装身份重新签名、公证和验收。

[macosbundle.py](macosbundle.py) 与 [macos-launcher.c](macos-launcher.c)
分片构建原生入口、app与DMG；入口/app签名，内层 `.com` 的封存哈希独立验证。
具体参数见工具 `--help`。此前c499载荷的公司Developer ID时间戳与Hardened Runtime
签名已成功，签后app实际 `--version`/`-run hello` 通过。Apple对app ZIP及DMG均返回
Accepted；app/DMG均已staple并验证，Gatekeeper均为Notarized Developer ID。
完整载荷、launcher及DMG SHA和资格边界见
[公开资格回执](../research/r9-apple-signing-qualification-20260928.json)。

资格回执仍为 `release_eligible=false`：尚需最终冻结门禁、签后确切分发资产的
编译/`-run`/包读取与系统API检查、quarantine和各平台运行证据及主人Promotion。
不能称Apple已签内层APE `.com`，也不能把Apple资格通过称为Windows/Apple双签发布。
若载荷或发布封装改变，必须重新签名、公证、staple并核对最终字节身份。

### 私有keychain操作边界

仅使用调用者明确选定的私有专用keychain；本机实测仅传 `--keychain` 不足以避免
系统密钥服务等待，需临时将该链前置user searchlist，同时保留原有全部项目。
操作前保存搜索表，任何成功、失败或超时路径均在finally恢复，并读回核对原表一致；
限时命令清理所属进程。不得导出既有私钥、修改login钥匙串ACL或输出密码、API key、
Key ID及公证提交标识；这些只在私有环境处理。一次诊断成功不替代最终载荷的签名
与信任回执，也不以关闭Gatekeeper或去掉quarantine作为通过条件。
