# unisacc 0.0.48.1 发布说明（待公开）

修复 macOS 启动时反复写出缓存管理器导致的延迟，管理器按内容缓存并复用，保留最近四个载荷及活跃进程保护，热调用清理过期lease。

## macOS 下载

附 `unisacc-0.0.48.1-macos.dmg` 及同名 `.sha256` 校验文件。该DMG来自 [macOS签名工作流42561596269](https://github.com/partnernetsoftware/unisacc/actions/runs/42561596269) 的 `macos-stapled-dmg`，已完成 Developer ID 签名、签后四成员重包、Apple公证 Accepted 和 staple。下载后可在所在目录校验：

```sh
shasum -a 256 --check unisacc-0.0.48.1-macos.dmg.sha256
```

DMG完整SHA256：`6e25e7c9f9efec717c1a0cf33bc7a35ad8c2bfccc2a0b2ab9e807ff2b40f0cc0`。

挂载DMG后，从终端以 `sh unisacc.com …` 使用其中的CLI。它包含签后原字节的两Mac切片及两缓存管理器，不需要关闭系统安全策略或清除quarantine。

已完成本机Gatekeeper验收，以及Apple Silicon原生arm64和Rosetta x86_64的默认 `-run` 返回7测试；cc记录热 `-run` 约0.22–0.25秒、`--version` 约0.01秒。这些结果限于已测宿主及上下文。

**Intel 实机/macOS13 未测。** Rosetta通过不等同Intel实机通过，也不代表已验证macOS13兼容性。

## 发布装配记录

公开0.0.48.1时必须附上述run的最终DMG，不能换用首签旧run或未staple的DMG。固定身份见 `release/macos-0.0.48.1.tsv`；staple后的DMG共5475910字节。DMG内签后APE SHA为 `988a528de14ec4874583923cd082cacd998245563c7ddfbff84bf1899f15547a`，与现草稿裸APE资产bdd52246…分账；不把未签APE的旧42/0迁给签后新字节。本文及当前附件准备不代表已公开发布。
