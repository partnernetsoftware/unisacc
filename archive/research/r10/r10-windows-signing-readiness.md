# 0.0.10 Windows 企业签名接入核查

2026-09-28；只读审阅形成的设计交付。审阅基线 `cbb3316`，没有运行公司签名、
没有查询或改写 Azure 身份、没有调度 Actions、没有重建编译器。

## 现状与证据边界

| 项目 | 仓库已有内容 | 仍须完成 |
| --- | --- | --- |
| 产品策略 | `release/signing-policy.json`，不是 minicon 的 `release-policy.json`；Windows 当前 `mode=deferred`，0.0.9 未签名，0.0.10 接续 | 最终版本完成资格核对后，再有意切换 `required`；不得用旧回执证明新资产 |
| 工作流 | `.github/workflows/windows-signing.yml`：资格模式不请求 OIDC；公司模式使用保护 Environment、Azure 登录及固定版本的 Artifact Signing action | 当前策略会在 Azure 登录前拒绝公司模式；正式登录、服务签名与信任链尚未实证 |
| 仓库身份 | PRD 开头与 `release/README.md` 记录专属 Entra app/SP、immutable OIDC subject、profile 范围 Signer 角色与 main-only 审核约束已读回 | 本轮没有外部配置读回，因此这属于历史记录，不能据本文声称外部对象当前仍有效；签名前需再次只读核对 |
| 格式资格 | PRD 记录一次性不受信证书演练与实际 `-run`；`release/apeinspect.py` 检查 PE、证书尾、模型包定位与包 SHA；`tests/apeversioncheck.py` 独立读取 VERSIONINFO | 旧演练不覆盖最终 0.0.10 字节；`apeinspect.py` 明确 `authentication=not_checked`，不是 Windows 信任判定 |
| 权威验签 | `scripts/inspect-authenticode.ps1`、portable `.sh` 与 trust-bundle helper 已在仓库；工作流另执行 `Get-AuthenticodeSignature` | 对真实签后资产执行；不能用自签 CA 或包 SHA 代替企业信任 |
| 发布资格 | 公司签名回执仍标 `release_eligible=false` | 签后同一 SHA 的六目标运行、Defender、发布审核；未测项如实保留 |

0.0.9 公开最终回执 `research/r9-release-acceptance.json` 明确 Windows 未签名。
Apple 已发布签名 DMG 是另一条分发链，不构成 Windows Authenticode 证据。
`release/README.md` 仍有历史 0.0.9 候选/待办说明；后续整理应保留历史来源，
加上最终发布回执链接，不将旧资格材料改名为 0.0.10 实签证据。

## 从 minicon 学到的可复用机制

只读参照了 minicon 的 `.github/workflows/company-signing.yml`、
`release-policy.json`、`release/self-sign-rehearsal.sh` 与验签工具。

- 复用签名前后资产身份、精确输入清单、上游 run/attempt、保护 Environment、
  无密码 OIDC、profile 级最小角色和最终 Windows 信任判定。
- 不复制 minicon 的应用/SP/CLIENT_ID、资产清单、版本、尺寸上限或生产者。
  unisacc 保持本机生产，Actions 只消费主人封存资产。
- minicon 的自签脚本是一次性证书的机制演练，明确不是发布信任证据；脚本中
  产品名、版本、ZIP 假设与尺寸均是 minicon 专属，不能直接执行后称 unisacc 通过。
- unisacc 已有独立封存契约，暂不需要再建第二套签名工作流。

## CI 是实际硬门槛

工作流资格阶段要求：本仓库 `ci.yml` 在 main 上同一完整 SHA 的指定 attempt
已 `completed/success`，且调度 SHA、checkout、远端 main、封存回执全部一致。
失败或不完整的 CI 会在下载资产/公司登录之前拒绝；本机绿不能跳过该断言。
因此本次 CI 修复必须先在本机完成；正常批量 push 后取得同源成功的第二意见，
再封存/调度签名。不能调度旧成功 run 来绕过新源码失败，也不能为了测试反复 push。

## 下一项最小实现范围

1. 收齐本次 CI 修复及 0.0.10 产品变化，冻结源码、版本与本机构建字节；
   本机门禁全绿，同源 CI 成功。当前本文没有代替上述条件。
2. 使用已配置身份，按仓库技能检查工具只读复核联邦 subject、保护 Environment、
   secrets/variables 名称与 profile 角色；不在公开报告输出值、令牌或私钥。
3. 对最终字节做格式检查和 VERSIONINFO 核对，生成独立私有临时目录中的
   `unisacc-unsigned.zip`、`unsigned-receipt.json`，上传既有草稿 seal。
4. 先执行零额度 `qualification`，确认封存输入及版本合格；随后变更策略
   `deferred -> required`，以新最终 SHA 重新完成验收/封存，才调度 `company`。
   策略变更本身改变源码 SHA，不能沿用变更前封存身份。
5. 收回签后 artifact，所有后续法院只读取该组签后最终字节。工作流不发布 Release，
   发布编排继续在本机完成。

以下是条件满足之后的命令模板，**本轮未执行**。变量须来自最终封存记录；
每个命令独立限制 60 秒，不把异步 Actions 等待塞进同一长命令。

```sh
python3 tests/bound.py 20 python3 release/apeinspect.py "$R10_UNSIGNED_COM"

python3 tests/bound.py 30 gh workflow run windows-signing.yml --ref main \
  -f mode=qualification -f source_sha="$R10_SOURCE_SHA" \
  -f seal_tag="$R10_SEAL_TAG" -f receipt_sha256="$R10_RECEIPT_SHA256" \
  -f upstream_run_id="$R10_CI_RUN_ID" -f upstream_attempt="$R10_CI_ATTEMPT"

# 仅 required 策略、重新冻结身份、资格与审核均完成后：
python3 tests/bound.py 30 gh workflow run windows-signing.yml --ref main \
  -f mode=company -f source_sha="$R10_SOURCE_SHA" \
  -f seal_tag="$R10_SEAL_TAG" -f receipt_sha256="$R10_RECEIPT_SHA256" \
  -f upstream_run_id="$R10_CI_RUN_ID" -f upstream_attempt="$R10_CI_ATTEMPT"

# Windows 上，以真正签后文件执行：
python tests/bound.py 55 pwsh scripts/inspect-authenticode.ps1 \
  -Path "$R10_SIGNED_COM" -ExpectedProductName Unisacc \
  -ExpectedProductVersion "$R10_VERSION"
```

## 最终资产验收清单

- 签前/后完整 SHA256、字节数、源码/本机构建身份、CI 与签名 run/attempt、
  seal release/asset id 全部绑定；变化不能被覆盖重写。
- PE VERSIONINFO 不变、Security Directory 有界且新增，模型包 SHA 不变。
- Windows 权威验签为 Valid；企业组织、签名者/issuer/thumbprint/有效期与
  RFC3161 时间戳证书齐全。当前工作流仅记录时间戳证书事实，不声称提取 genTime。
- 篡改应拒绝；最终签后包加载、`--version`、实际编译与 `-run`、六目标运行、
  Defender 扫描分别留下证据；模拟/跳过不能算原生通过。
- 同一签后 `.com` 若被放入 Apple `.app`，应重新封存载荷并对最终 app/DMG
  重新签名、公证、staple、quarantine 验收；不能让外壳仍绑定签前 SHA。
- 每个执行步骤不超过 60 秒；发布仍由最后审核推进，不将工作流产出的
  `release_eligible=false` 擅改成发布许可。

本交付实际做过：源码/策略/工作流/参考文件读取。未做过：外部身份核验、
最终资产格式执行、自签演练、OIDC 登录、公司签名、签后平台运行。
