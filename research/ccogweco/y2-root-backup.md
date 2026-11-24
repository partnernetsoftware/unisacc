# Y2 前 Root 备份与 Y2 后登记清单(给 agy-flow;只读核对,2026-11-25 ccogweco)

依据 tools/install-ape.cx(备份/回滚/发布闸)与 tools/release-post.cx(第 5 步 root-upgrade),以及
tools/old-product-assets.tsv 的 R33 登记要求。命令原行与期望字面量均取自源码,行号为当前 main。

## 0 结论

- Y2 前必须有的不是"再备份一次",而是 ROOT/.unisacc-root-backups/index.tsv 里有当前 Root 的备份条目,
  且 release-post 第 5 步的 `--rollback` 干跑能打出 `INSTALL_APE	ROLLBACK_DRYRUN	<sha>`;否则 root-upgrade 直接 FAIL。
- 备份由 install-ape 自己维护:<sha256>/ 目录 + manifest.tsv(name/sha256/bytes 三列)+ index.tsv(最旧在前,
  最多三份,超出删最旧,删不掉按名拒绝)。候选安装(candidate 模式)在替换 unisacc.com 之前自动备份当前 Root。
- Y2 后登记:把新公开 APE 作为旧产物基线写进 tools/old-product-assets.tsv 一行(六列),
  并在本机预热 ~/.cache/unisacc/prep-cache/old-product-cache/<sha256>/unisacc.com;回滚就是删这一行。

## 1 Y2 前:核对备份存在且可回滚

(1) 当前 Root 必须是公开件(无候选标记)。命令与期望原行(install-ape.cx:523):

    sh ./unisacc.com tools/install-ape.cx -- <ROOT> --release-gate
    期望: INSTALL_APE	PUBLIC_ROOT	<ROOT>            (rc 0)
    若 rc 1 并打印 root-candidate:先按 0.0.106 的候选流程收尾;release-exec 与 release-run --pack 也会拒同一标记。

(2) 干跑一次备份(不写;install-ape.cx:402 打印):

    sh ./unisacc.com tools/install-ape.cx -- <ROOT> --backup
    期望: INSTALL_APE	BACKUP_DRYRUN	<当前 unisacc.com 的 sha256>	<bytes>	files=<清单>
    若打印 BACKUP_SKIP no-current-root(install-ape.cx:352):ROOT 下没有 unisacc.com,先装公开 Root。

(3) 确认备份已在索引里(真实备份:install-ape.cx:385):

    sh ./unisacc.com tools/install-ape.cx -- <ROOT> --backup --write
    期望: INSTALL_APE	BACKUP	<sha256>	<bytes>	kept=<1..3>
    再核 <ROOT>/.unisacc-root-backups/index.tsv 含该 sha256(最旧在前,最多三份);
    目录内应有 manifest.tsv 与 IA_NSAVED 个文件(unisacc.com、build record、receipt、sha record、候选标记),
    每个文件与 manifest 的 sha256/bytes 逐项相等(ia_backup 内已自核,install-ape.cx:345-390)。

(4) 回滚干跑(release-post 第 5 步用的就是这一行,install-ape.cx:476):

    sh ./unisacc.com tools/install-ape.cx -- <ROOT> --rollback
    期望: INSTALL_APE	ROLLBACK_DRYRUN	<最新备份的 sha256>
    没有备份时(install-ape.cx:716): REFUSED no-candidate-backup -> 不许继续 root-upgrade。

(5) Y2 之前还要确认 backup-evidence 已生成:release-post 第 3 步调 release-chain backup-evidence --verify
    (release-post.cx:142-158,读 release/evidence/<V>/backup-evidence.tsv 或 --backup-dir manifest);
    第 5 步的门(release-post.cx:187-191)在 `--rollback` 干跑 rc!=0 或不含 ROLLBACK_DRYRUN 时打印
    `POST_ROOT_BACKUP_GATE	FAIL`,通过时打印 `POST_ROOT_BACKUP_GATE	PASS`,随后才 install-ape --write。

## 2 Y2 后:R33 旧产物登记

(1) 登记行格式(tools/old-product-assets.tsv 表头与现有行):

    version	repo	tag	asset	bytes	sha256
    <新版本>	partnernetsoftware/unisacc	v<新版本>	unisacc.com	<公开 APE 字节数>	<公开 APE sha256>

  追加到表末即可;不要改旧行。登记值取公开 release 资产本身,不以本机安装件代替。

(2) 选择规则(old-product-material.cx:43-56):predecessor 逐行解析三段版本号,取严格小于候选且数值最高的一行;
  R33 的对比基线就是这行。登记后跑一次材料准备核对:

    sh ./unisacc.com tools/old-product-material.cx -- <ROOT> <NEW_OUT> <COMPILER>
    期望: 正常 PREPARED 输出且 old-version 解析为新登记行的前一版(低于候选)。

(3) 本机缓存预热(prd_mempalace.md:144 的既有做法):

    ~/.cache/unisacc/prep-cache/old-product-cache/<sha256>/unisacc.com
    按内容哈希寻址;目录内容应与登记行的 sha256 与 bytes 一致。

(4) 回滚:删除 tools/old-product-assets.tsv 中新增的那一行,即回退到上一基线;缓存目录按内容哈希寻址,可留。

## 3 失败处置

- 备份缺失或 manifest 不一致:install-ape 按名拒绝(backup-not-removable / cannot read a Root file for the backup),
  此时不得继续 root-upgrade;先把 index.tsv 与目录对齐(删掉孤儿目录用 ia_drop_backup 的规则:只删清单内名字再 rmdir)。
- ROLLBACK_DRYRUN 不出现:release-post 打印 POST_ROOT_BACKUP_GATE FAIL 并停在第 5 步;补备份后重跑 --step 5。
- 登记行版本比较异常(数值相等或高于候选):old-product-material 报 no-producer=old-product;
  核 version 三段数字与候选表版本,不得手改候选版本迁就。
- 任何拒绝都在原行保留,不写 PASS、不改判据。
