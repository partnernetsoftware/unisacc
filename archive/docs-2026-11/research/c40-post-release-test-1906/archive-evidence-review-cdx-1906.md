# 1906 续测材料归档复核（cdx）

结论：现存材料支持历史续测终表；产品 UA 红与参考 UA 绿必须分账。本次仅复核归档，不把当前已装产品认作历史 0.0.40，也不重新认证线上发布状态。

证据已迁入 [`archive/research/c40-post-release-test-1906/`](../../archive/research/c40-post-release-test-1906/)。本次实际运行该目录 `shasum -a 256 -c HASHES.sha256`：21/21 匹配，未重跑测试。

| 核项 | 核对结果 |
|---|---|
| identity | `meta.txt`、`o-run-SUMMARY.txt` 和历史公开发布回执一致：SHA256 `50ab4c213feedb2f1a4f1f39c8bfca7d8d63c74a1d6b50915db8ceb233f5cf58`，2243662 B，`unisacc 0.0.40`。这是归档身份交叉核对，未重新下载或执行历史资产。 |
| build.json | 历史 sidecar 记录 SHA `b7128a0f…`、2225534 B，与上述产品不符；stale 结论保留，不改 sidecar 或资产。 |
| 产品 UA bootstrap | rc1，B=1/U=0；B tape 为空，产品 Darwin object/tape 路由拒绝。U 单支 tape 不构成固定点通过。 |
| 参考 UA bootstrap | rc0，`ok B=C=U 257c4ef6944103ef (421304 lines)`；三 tape SHA 均 `9266b52f953646af278b9fb28862e873da51728336280e37dac10fcd175424a1`，与 1532 SUMMARY 相同。大产物未留存，本次只核记录与日志。 |
| acceptance 10-* | 有 ACCEPT_STATE 的五步均 rc0，日志 `B = C = U same`；三 tape 同上述 SHA。B/U 镜像 SHA 不同，不得表述为镜像相等。最初缺态调用 rc2 仍保留。 |
| -o/run | hello/fact/fib/ptr/switch/struct/do/host 八例双路径，16/0；只证明当时本机抽样，不扩为全量或六目标通过。 |

历史发布回执见 [`receipt-publish-latest-v0.0.40-1804.md`](../c40-publish-unsigned-1804/receipt-publish-latest-v0.0.40-1804.md)。前序 18:04 发布不算本刀操作。归档回执自报未扩发布、补签、切资产或抢 K6 窗，所审材料未见反证；没有完整操作审计，不能提升为独立证明。acceptance 日志确有测试镜像 `replacing existing signature`，需与发行资产补签区分。

附带纠偏：本次实读本地 git 对象，`rc/v0.0.40` 是 annotated tag；其 peeled commit 与 `v0.0.40^{}` 均为 `1db3083506e7af178ed604052532d4226511ad01`。原回执把 tag 对象 `a82b9bda…` 与提交混比的“rc 漂移”不是有效卡点。

本次只新增本短记；未改源、未改资产/build.json/tag、未重跑全量、未执行签名或发布命令、未操作 K6 调度窗。产品 UA 路由、发行签名和全量 release 验收的历史 OPEN 状态保留。
