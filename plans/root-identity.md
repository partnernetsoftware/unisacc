# 本地根产品身份（2026-11-08 开发修复）

当前版本工作目录的本地根产品是 0.0.71 APE,仅用于生成与测试,未发布。共享 main 的已装产品不因此改变。

构造输入提交为 `2044f903fd25b51d0e526a5de2f7e0dd27e054d9`,源 SID 为 `93a9fe8956306c4b5c38d56578ab25e6d923ed225777836364f020cc30f6c7fb`。测试修复提交不能冒充构造提交。

<!-- identity-begin -->
product_kind	ape1
product_digest	f6cb014376d7d4944c9a3f7a9252d156f7f32c08a2c0d5beb22f7098ae19e30e
unisacc.com	f6cb014376d7d4944c9a3f7a9252d156f7f32c08a2c0d5beb22f7098ae19e30e	8789646
<!-- identity-end -->

## 身份判定规则

(1) `tools/check-root-identity.cx` 现场计算产品摘要、成员与字节数,逐行校验上块。APE 身份取整个文件的 SHA256;split 身份取成员集合,不能只比薄入口。

(2) `unisacc.com.build.json` 须具有有效 JSON、实际字节数、同一 product_kind/product_digest、真实构造提交与源 SID。`release/candidate.tsv` 的 APE 与记录摘要来自本地正式打包生成物,CI run_id=0 表示尚无本候选 CI 证据。

(3) 源 SID 须由身份 helper 对冻结源重新计算。未提供 helper 与源目录时必须记 NOT_RUN,不能拿侧车自报值代替重算。

(4) 入口为 `sh ./unisacc.com tools/check-root-identity.cx -- ROOT --identity-helper HELPER --sources SNAP`。批次名 `cx-root-identity`。身份一致不代表功能测试通过,也不迁移旧候选的平台资格。

## 候选模式(0.0.87 S4,公开前装候选为本机 Root)

目的:整链产出的候选 APE 在公开之前就装成本机 Root 做开发验证,把消费者破坏左移;可回退、可核身份、不污染发布证据。

(1) 安装:`sh ./unisacc.com tools/install-ape.cx -- ROOT --candidate DIR --write`。DIR 是 bg-candidate 的候选目录 `BASE/<rev>`(目录名 40 位十六进制)。安装前逐项核对,任一不符具名拒绝且不动 ROOT:chain-run.identity 含 `rev=<rev>`、seal.tsv 存在、ape-final/ape-receipt.txt 最后一行 unisacc-next.com 指向 DIR/ape-final/unisacc-next.com 且 sha256 与字节同实物、APE 答出的版本在 ROOT/release/candidate.tsv 的 version 列中。不带 --write 只报 DRYRUN。

(2) 备份:替换前把当前 Root(unisacc.com、build.json、ape-receipt.txt、unisacc.com.ape.sha256、候选标记)复制到 `ROOT/.unisacc-root-backups/<sha256>/`,index.tsv 记先后;最多 3 份,超出删最旧(BACKUP_PRUNED)。

(3) 标记:写 `ROOT/unisacc.com.candidate.tsv` 一行 `CANDIDATE rev= sha= bytes= version= source= not=PUBLISHED`;不重写本文身份块(受控文档不记未公开产品),不写 release/evidence。公开件正常安装(--write)时删除该标记(CANDIDATE_CLEARED)。

(4) 回退:`install-ape.cx -- ROOT --rollback --write` 恢复最新一份备份:先按备份内 manifest.tsv 核全部文件(unisacc.com、build.json、ape-receipt、sha 记录、标记)的 sha 与字节,并核 unisacc.com 与目录名一致,任一不符具名拒绝;全部先写到临时名并复核,再逐个 rename,被替换的旧文件先移到旁边,中途失败时把已替换的放回;备份中没有的文件(如标记)一并删除;恢复后核 Root sha,再删备份目录(只删备份集内的文件名加 rmdir,失败具名报错且索引保留),最后出索引。索引不存在或为空时拒绝 `no-candidate-backup`(没有候选备份);原 split 时代回退改为显式 `--rollback-split`。淘汰超过 3 份的最旧备份同样检查删除结果,删不掉时拒绝安装并在索引里保留它。

(5) 防误用:`install-ape.cx -- ROOT --release-gate` 有标记 rc 1 `REFUSED root-candidate`;release-exec(含 --dry)遇仓根标记拒绝 `REFUSE RELEASE_EXEC_GATE_ROOT_CANDIDATE`,release-run --pack 遇 ROOT 标记拒绝 `REFUSE RELEASE_PACK_ROOT_CANDIDATE`;chain-ready 第 17 条 `17-root-public` 有标记时记 PASS 加 `WARN root-candidate ... RELEASE_BLOCKED`,只列不拦起链。发布必须以公开件为基线。

(6) 检查:tools/check-install-ape.cx 的 cand-* 用例(批次 install-ape),全部在私有临时目录夹具中运行,末项核真实仓根 unisacc.com sha 不变。

## 证据入口与回滚

完整构造与比较证据位于 `/tmp/unisacc-session-v71-full/2044f903fd25b51d0e526a5de2f7e0dd27e054d9/`,本地正式布局位于 `/tmp/unisacc-session-v71-full/local-layout/`。布局重新打包所得 APE 与原候选逐字节身份相同。

当前完整测试仍未通过。此文只对齐本地候选身份,不宣称 QUALIFIED。共享 main 的已装产品未替换;版本目录回滚须经 `tools/install-ape.cx` 同步产品、build.json 与安装收据,再重跑身份检查。
