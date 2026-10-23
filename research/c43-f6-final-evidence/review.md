# F6 最终固定点实证审查

本报告区分两类证据：我只读了父会话真实构造/证明日志，并独立执行轻量sha256sum/stat/cmp及marker文本核对；**没有独立执行构建、comboot、ident或测试**。全部读取在scratch cwd，未改tracked、未触外部服务、未干扰正在跑的F3。

结论：当前实际文件与保留证据足以支持F6“候选构建＋comboot三SHA相等”完成，未发现产品身份、大小、sidecar或最终step证明的矛盾。它不证明F3/F4/F5完成，也不授权Draft/Latest/签名/公开。

## 独立只读文件核对结果

实际五份二进制均为2670104字节，SHA256统一：

`4f0b493c7a83d73a4ae1b348c8400ea3b7eda0ea190051b54a84c712a6730eda`

路径：root unisacc.com；out/f3-model/unisacc-seed.com；out/f3-model/stage3/unisacc.com；comb-build/stage2/unisacc-next.com；comb-build/stage3/unisacc-next.com。

seed、root、已安装stage3、stage2候选、stage3候选及f6-final-evidence/candidate.build.json的完整sidecar统一SHA：

`72989230668551cd13360e79c2339c4cefed71cdb25fe1838e7ebdfbe2ba9a39`

所以不仅二进制相等，实际完整build.json也没有commit/settings差异。两候选包compiler.pkg亦同SHA：

`9165964de3e4249572ccf17110ef5410d87ea40cbd32094b50a159d49c36e496`

这与accept-inputs.sha256中的已冻结值一致。当前Git HEAD为`1a26da092bc56c5b88c38cac16d59bf9c20799d6`，只读git status无tracked差异。

## sidecar与source

实际stage2/stage3 JSON均为schema1，artifact_sha256及bytes匹配上列真实文件，commit为1a26da09完整值，settings仅PYTHONHASHSEED=0，sources_sha256为：

`c4618639cc3fcfac4f8ce52a11df3bdab93b2c9376eecee63fd70e6550adc57f`

父会话final-proof日志有root与stage3候选双ident verified。comboot-final-proof.sh是set-eu，先cmp seed/stage2/stage3与sidecar，再官方fixedpoint、installpair、root/model双cmp、双ident、实际三文件sha、官方report；日志完整到stage3报告，未见中途失败后继续打印通过的逻辑。归档版final-proof.log与out/edge-audit原日志hash相同。

我没有重新计算seed/ident.c的源码身份算法，因此source freshness属于读取真实双ident日志、并核对当前receipt/HEAD/marker的一致性；不能把它叙述成本代理独立ident执行。

## stages.json与built_by链

out/comboot/stages.json与f6-final-evidence/stages.json逐字节相同。三项bytes均2670104、sha均4f0b完整值，path分别映射seed、root(stage2安装)、SEED_DIR/stage3安装。

stage2.built_by_sha256=stage1.sha256；stage3.built_by_sha256=stage2.sha256，均为同一4f0b。comboot.sh的实际路径中，stage2构造前export UA=SEED，stage3前export UA=STAGE2；故built_by不是仅三个无关产物事后凑相同hash，源码流程确实以其前一阶段为构造工具。

限制：stages.json的built_by字段本身由stage()读取前项填入，并非独立签名的执行跟踪。本轮有构造窗口、真实步骤proof及源码流程作支撑；不将这个普通本地收据扩大成更强的防篡改证据。

## 每步marker与proof

stage2、stage3各12项marker都存在：shared；六目标lnx/arm64、lnx/x86_64、osx/arm64、osx/x86_64、win/arm64、win/x86_64；pack-prep-1/2/3；pack-models；pack-driver。

独立只读核对24个marker的source前缀均为c4618639完整值，第二字段都等于comboot.sh step_manifest所指真实proof文件的当前SHA：shared/六目标manifest.json、pack-prep-N.done、compiler.pkg或unisacc-next.com.build.json。未出现缺marker、旧source或proof hash不匹配。

审查时首次工具文本归一化误将x86_64目录写成x86-64，产生六条“路径不存在”输出；随后按真实目录lnx-x86_64/osx-x86_64/win-x86_64重新读取，六项均MATCH。这是审查命令路径错误，**不是构建证据错误**，不把前述误报混进正式产品结论。

manifest还包含inputs及outputs逐产物hash；本代理核对的是marker→整个manifest/proof文件身份，没有重新遍历每个manifest里的全部下游产物去替代官方构造检查。最终实际打包二进制/包/sidecar已独立hash一致，满足本次F6范围。

## 窗口退出与未把超时计完成

真实stage2窗口01、03–07与stage3窗口01–08有明确pending/exit75消息；这些都未被本报告算作阶段完成。stage2最终08打印stage2记录和built_by验证；stage3最终09打印stage3记录，final-proof随后完成官方fixedpoint及双ident。

stage2窗口02日志确实0字节。README明确记它是外层超时、未计通过；单凭空日志本代理不能独立复原那次具体rc/终止时刻。但是当前24个真实marker与proof匹配、后续最终阶段记录/证明完整，不依赖“02空日志算绿”。本轮没有发现跳过失败而手造完成marker的证据。

构造窗口中的47s、37s等数字是到步骤边界退出的墙钟报告，不表示单个步骤限时被放宽；各步由comboot原生bound50/55限定。本文未重放计时，不能仅从日志证明每次外层调度都小于60秒；父会话保留原调度状态为其执行证据。

## 还缺什么

按F6既定判据，**没有发现还需要新增产品验证的实质缺口**：实际三阶段文件、大小、全sidecar、source、built_by与最终固定点均有相互一致证据。

为使正式归档更独立易审，建议在不伤当前F3的只读整理阶段补齐两项收据材料（不重跑构建）：

1. 将每个窗口实际rc/外层watchdog状态（特别stage2-02超时、stage2-08/stage3-09/final-proof为0）汇成附时间戳的原始调度表；目前evidence目录靠README叙述02，最终成功日志靠set-e流程解释，无单独逐窗rc文件。
2. 把24个marker及其proof文件摘要、最终proof脚本摘要一并冻结进evidence清单；当前markers/manifests仍只存在构造目录，evidence目录主要保留阶段日志与最终JSON。保持原始历史，不以当前值重写旧失败窗口。

这些是收据可搬运性补强，不是要求把已完成F6重做，也不改变计划验收口径。F3/F4继续按其完整集合、实际运行与具名host/ruling证据独立收口；正式公开边界仍归主人。
