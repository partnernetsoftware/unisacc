# 1906 续测旁核（cdx）

结论：身份及续测终表吻合；固定点仅参考 UA 绿，产品 UA 红保留。**rc tip 漂移误报**：tag 对象与提交混比。本次实际核过材料、哈希与本地 git 对象，未重跑测试、未查询或操作线上发布。

| 核项 | 证据与判断 |
|---|---|
| identity | 本机 `unisacc.com` 实算 SHA256 `50ab4c213feedb2f1a4f1f39c8bfca7d8d63c74a1d6b50915db8ceb233f5cf58`、2243662 B，与 `release/candidate.json`、`release/evidence/v0.0.40/unsigned-receipt.json` 及公开发布回执一致；`meta.txt` 与 `o-run-SUMMARY.txt` 记 version `unisacc 0.0.40`。此处按本地公开资产记录核对，未重新下载线上资产。 |
| stale sidecar | 实读 `unisacc.com.build.json` 的 artifact SHA=`b7128a0f…`、2225534 B、旧 commit `688ac083…`，与现 com 不符，确认 stale。只记录，不改 sidecar/资产。 |
| bootstrap 产品 UA | `bootstrap-full.log`、`B-product-UA.log`、`bootstrap-SUMMARY.txt` 一致：rc1，B=1/U=0，Darwin 产品路由拒绝 object/tape；B tape 空。U tape 独自为 `9266b52f…` 不能补成产品固定点绿。 |
| bootstrap 参考 UA | `bootstrap-ref-full.log`、`bootstrap-ref-SUMMARY.txt`：rc0、`ok B=C=U 257c4ef6944103ef (421304 lines)`，三 tape SHA256 均 `9266b52f953646af278b9fb28862e873da51728336280e37dac10fcd175424a1`，与1532的 `release/evidence/v0.0.40/m4pro-flex-1532-SUMMARY.txt` 完全相同。`tests/lib.sh` 的 ua_ready 默认参考路径有源码 stamp；未独立重建/复跑。 |
| acceptance 10-* | 五步 `10-Btape/Bimage/Ctape/Uimage/compare` 均 rc0，日志 `B = C = U same`；`tests/acceptance.sh` 193–196 行比较三份 tape。`accept-state-HASHES.txt` 三 tape 同上述9266；B/U 镜像 SHA 分别0b7a79cc…/f588ba47…，**不是镜像逐字节相等**。初次无 ACCEPT_STATE 的两项 rc2 保留，属缺态调用，后续绿链有明确独立记录。 |
| -o/run | `o-run-suite.log` 与 SUMMARY 一致：hello/fact/fib/ptr/switch/struct/do/host，8例×2路径，PASS 16、FAIL 0，-o+exec 与 -run 输出相同。是本机抽样，不扩为全量/六目标验收。 |
| tag 纠偏 | `git cat-file -t rc/v0.0.40`=`tag`；原始对象 `a82b9bda598557c1c854a7fcb4baec154aa726da` 指向 commit `1db3083506e7af178ed604052532d4226511ad01`。`git rev-parse rc/v0.0.40^{}` 与 `v0.0.40^{}` 同该提交，与公开回执 rc_commit 一致。原 receipt/paste 所称漂移不能成立，原材料留存、不改 tag。 |

材料完整性：`HASHES.sha256` 所列21份文件全部逐项实算匹配。大 tape/镜像已从材料目录剔除，其字节哈希仅能核对留存记录，未重新计算原大产物。`v0.0.40`→当前 `cfabc81b…` 的 src/exec/unisa/include/kernel/weights/unisacc.c 无提交差异；seed 有 K6 文件变化，不能把整个仓库称为未变。

边界：本旁核只做本地读取与 prd/本短记落盘，未改源码、未重跑全量、未发布、未补签、未切资产/Latest、未改 build.json 或 rc tag，未操作 K6 窗。1906 回执自报未扩发布/未抢 K6，所审材料未见反证；没有全程操作审计，故不把自报提升为独立证明。acceptance 日志明确出现 B/U `replacing existing signature`，脚本为 `codesign -f -s -` 的测试镜像 ad-hoc 签名；不能写“完全没有签名操作”，也不能混称发行资产补签。本旁核未执行任何签名。

仍 OPEN：产品 UA 的 Darwin object/tape 路由、发行签名、全量 release --com 验收。已有18:04公开发布是前序事实，不是本刀授权或本刀执行；本轮绿不扩发布结论。所谓 rc 漂移从卡点中排除。续测/K6 调度按并行只读不抢窗边界保留。
