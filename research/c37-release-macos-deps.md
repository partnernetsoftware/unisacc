# 0.0.37 发布侧 macOS 依赖排查（cc，2026-10-09，只读；cdx 负责 blob/buildcompiler 守卫）

结论：候选构建的 Darwin 依赖可以去掉（cdx 在做 LLVM Mach-O 交叉 + 0.0.36 内嵌 kernel 逐字节对拍）；发布侧有三处是**真依赖或权限**，不是守卫：Apple 签名/公证/dmg、m4pro 全门禁里的 macOS 原生套件、GHCR 写权限。

| 步骤 | 宿主依赖 | 云机 Linux 能否做 | 可复用通道 / 阻塞 |
|---|---|---|---|
| 封存 GHCR（seal_candidate.sh） | `oras` + gh token | 工具可装；**gh token 无 `write:packages`**（现 scopes: gist, read:org, repo, workflow；账号对仓 admin） | 阻塞：需主人在本机 `! gh auth refresh -s write:packages`（交互） |
| rc_tag / 草稿 release / 上传 | git、gh | 能 | 现成 |
| release-check.yml（六格候选实跑，含 macos-14） | GitHub runner | 能触发（push） | 现成：这是已批的 macOS **测试**证据通道（runner 不编译） |
| Windows 资格/公司签名 | windows-signing.yml（Azure secrets，environment `release-signing`） | 能：`gh workflow run` + approve_signing.sh | 现成；环境审批需有审批权限的账号 |
| Apple 签名 + 公证 + dmg（apple-sign.sh / macosbundle.py） | `security`/`codesign`/`xcrun notarytool`/`stapler`/`hdiutil`/`spctl`，p12 与 notary 凭据在 m4pro `~/.private_keys` 与钥匙串 | **不能**：云机无 `~/.private_keys`，工作流里没有 Apple secrets | 真依赖。publish.sh 硬要求 `unisacc-macos-universal.dmg`。可选通道（都需主人批+放密钥）：(a) m4pro 只跑 apple-sign.sh 一步；(b) 新增 GitHub macOS 签名工作流 + Apple secrets（仿 windows-signing，签名非构建）；(c) Linux `rcodesign` 签名+API key 公证，但 dmg/hdiutil、spctl 评估无对等物，需改流程 |
| 全门禁 `release.sh --com` | 约 90 个套件脚本引用 Darwin/osascript/Terminal/rosetta/xcrun；`UNISACC_FFI_X86_PROVIDER`（osx/x86_64 libffi 提供者）是 gate.sh 硬前提 | **不能等价**：Linux 上缺 macOS 原生/rosetta/Terminal 套件 | 真依赖。政委 10-09 规则：Linux 绿不顶替 m4pro 全门禁。是否以 release-check macos runner 六格 + 云机 Linux 全量替代 m4pro 全门禁，须政委明确改规则 |
| install_root / publish.sh / smoke | gh、shasum、curl | 能 | 现成（等签名资产齐） |

## 去掉 Darwin 守卫后云机能独立推到哪

候选构建 → 自举定点 → fb12multi/comdemo → （GHCR 权限到位后）封存 → rc_tag → release-check 六格 → Windows 签名 → 草稿。停在 Apple 签名与 macOS 全门禁两处，需主人/政委给通道。
