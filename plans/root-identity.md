# 本地根产品身份（2026-11-08 开发修复）

当前版本工作目录的本地根产品是 0.0.71 APE,仅用于生成与测试,未发布。共享 main 的已装产品不因此改变。

构造输入提交为 `2044f903fd25b51d0e526a5de2f7e0dd27e054d9`,源 SID 为 `93a9fe8956306c4b5c38d56578ab25e6d923ed225777836364f020cc30f6c7fb`。测试修复提交不能冒充构造提交。

<!-- identity-begin -->
product_kind	ape1
product_digest	371a4c741f8b0229092e21b2eb1afd8ba5cb5f23dabcddd2a8dcb9f77103e977
unisacc.com	371a4c741f8b0229092e21b2eb1afd8ba5cb5f23dabcddd2a8dcb9f77103e977	7391587
<!-- identity-end -->

## 身份判定规则

(1) `tools/check-root-identity.cx` 现场计算产品摘要、成员与字节数,逐行校验上块。APE 身份取整个文件的 SHA256;split 身份取成员集合,不能只比薄入口。

(2) `unisacc.com.build.json` 须具有有效 JSON、实际字节数、同一 product_kind/product_digest、真实构造提交与源 SID。`release/candidate.tsv` 的 APE 与记录摘要来自本地正式打包生成物,CI run_id=0 表示尚无本候选 CI 证据。

(3) 源 SID 须由身份 helper 对冻结源重新计算。未提供 helper 与源目录时必须记 NOT_RUN,不能拿侧车自报值代替重算。

(4) 入口为 `sh ./unisacc.com tools/check-root-identity.cx -- ROOT --identity-helper HELPER --sources SNAP`。批次名 `cx-root-identity`。身份一致不代表功能测试通过,也不迁移旧候选的平台资格。

## 证据入口与回滚

完整构造与比较证据位于 `/tmp/unisacc-session-v71-full/2044f903fd25b51d0e526a5de2f7e0dd27e054d9/`,本地正式布局位于 `/tmp/unisacc-session-v71-full/local-layout/`。布局重新打包所得 APE 与原候选逐字节身份相同。

当前完整测试仍未通过。此文只对齐本地候选身份,不宣称 QUALIFIED。共享 main 的已装产品未替换;版本目录回滚须经 `tools/install-ape.cx` 同步产品、build.json 与安装收据,再重跑身份检查。
