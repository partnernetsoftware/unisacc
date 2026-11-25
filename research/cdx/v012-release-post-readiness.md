# v0.1.2 发布收尾就绪核对

## 当前判定

截至本次取证，远端 main 和 v0.1.2 均指向 X `d71c949779`，远端没有返回 `v0.1.2` 标签，版本树 INDEX 尚无公开、公开读回或 Y2 完成行。CI 六格已在 attempt 1 通过。收尾尚未就绪，须先完成 Y1、公开、公开读回及 Y2。R33 七件输入都已存在，可以在版本树单独准备备份。

## 执行位置和顺序

(1) 在 `.claude/worktrees/v0.1.2` 完成 release-exec 第 4 至第 7 步。第 7 步核对 Y1 冻结的 `release/evidence.tsv` 与 `release/decision.txt`，重算决定、核 origin/main 祖先关系并提交推送 Y2。真实 Y2 提交后才把 `exec-7-Y2-done` 附到工作区 INDEX，该行由下一笔提交带上。公开读回须先确认说明逐字节一致和资产哈希通过。对应 `tools/release-exec.cx:1620`、`:1678`、`:1693`、`:1698`。

(2) 在版本树先核 INDEX 的 `exec-5-publish PASS`、公开读回结果、`exec-7-Y2-done`、本地标签可解到提交，且远端 main 已包含真实 Y2。随后执行 `release-post run --version 0.1.2 --dry`，再执行其 `--execute`。第 2 步按版本号唯一匹配 slot 67，把 `build/version-seq.tsv` 的状态从进行中改为已公开，补齐 INDEX 的 Y2 封口与 post-publish 行，再派生并提交 `build/version-cadence-git.tsv`、`build/version-cadence.tsv`、`build/north-star.tsv`。工具先 dry 后 commit，默认不推。执行者核对路径后从版本树快进推 origin/main。对应 `tools/release-post.cx:113`、`:127`、`:163`，`tools/release-chain.cx:893`、`:913`、`:936`、`:1080`、`:1092`、`:1142`。全流程执行还会写状态文件，可用 `--state-file` 指到会话临时目录。第 5 步仅在显式指定 `--root-upgrade` 时安装，先核回滚门。第 6、7 步只打印提示。

(3) R33 由 `release-chain backup-evidence` 创建。输入是冻结提交 `115e5720de` 的链目录中的 `heartbeat.tsv`、`chain-run.tsv`、`exit.tsv`、`bg.log`、`ape-final/unisacc-next.com`，以及同候选预跑目录中的 `supervise.log`、`strict-final.tsv`。七件已实查存在。当前默认私有备份和版本树均缺 `backup-evidence.tsv`。创建时传实际 `--chain-dir`、`--prerun-dir`，备份目录须在仓和缓存之外，默认 `~/.unisacc-release-evidence-backups/0.1.2`。工具在该目录复制七件、写大小和 SHA-256，同时把清单写到版本树 `release/evidence/0.1.2/backup-evidence.tsv`。清单含本机绝对路径，核验时给 release-post 传 `--backup-dir` 读取私有清单，避免把本机路径版清单纳入公开提交。release-post 第 3 步只验证，不生成，期望原行为 `BACKUP_VERIFY PASS files=7`。对应 `tools/release-chain.cx:1380`、`:1396`、`:1431`、`:1458`、`:1464`，`tools/release-post.cx:142`。

(4) slot 67 标记后，在版本树另做解冻与旧产物登记。将 `build/freeze.tsv:5` 的 `0.1.2-FREEZE` 原行注释化，保留 base、owner、deadline 和 paths，尾注公开日期与真实 Y2。将已公开下载并验证的 `unisacc.com` 登记到 `tools/old-product-assets.tsv`，版本 `0.1.2`、仓库 `partnernetsoftware/unisacc`、标签 `v0.1.2`、文件 `unisacc.com`、大小 `8992558`、SHA-256 `e31c413a1287a935c62df9e5c9820f91d7d760ea7e3c2c1480f99f8fc6a7c269`。当前表只有到 0.1.1，新行尚无。先例为 `7b49a9e47`，消费入口见 `tools/old-product-material.cx:47`、`tools/check-affected-oldproduct.cx:8`。这两项由执行者按路径提交及快进推送，release-post 没有对应写步骤。

## 共享检出及下一版

共享 main 仍实见 19 个暂存文件，全部在 research 和 skills，与 slot 67、INDEX、指标表、freeze 和旧资产表的写域无交集。暂存内容与已在 v0.1.3 上的 `7eb0d06a4` 比较无差异。保留共享暂存区，由版本树完成收尾；避免把共享检出当发布执行目录。防止收尾期间其他窗口推 origin/main。

公开读回完成后，cc 先核真实 Y2 已推和完成行已写，再执行上述收尾。待最后一笔收尾已推，v0.1.3 在自己的版本树合入新的 origin/main，保留 `7eb0d06a4` 及正在进行的扫除。合入前协调该树的写窗口，合入后复核 slot 67 已公开、0.1.2 已解冻、旧产物登记和版本派生表；共享 main 的 19 条暂存由其所有者按已保存提交处置。
