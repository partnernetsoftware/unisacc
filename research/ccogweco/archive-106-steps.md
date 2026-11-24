# 0.0.106 证据归档步骤(给 agy-flow 执行;只读草案,未写 release/)

链:REV 2e8a19a342251910d53010f51002b14a4ca7f916;SOURCES_SID c6eeca9f10…;APE sha 5f83ac93d201b252ea0b56098475be9e30045875cdf1ce692451cda095ceeb3c(8932113 B);链 exit.tsv rc=0。暂存目录:~/.local/share/unisacc/research/agy-flow/evidence-0.0.106/。

## 四项清单与现状

| # | 项 | 目标位置 | 现状 | 做法 |
|---|---|---|---|---|
| 1 | ape-receipt 脱敏拷入 | release/evidence/0.0.106/ape-receipt.txt | 暂存目录已有(12 行,路径已用 ~ 简写) | 经 devsvc 写入:`sh ./unisacc.com tools/devsvc.cx -- f write release/evidence/0.0.106/ape-receipt.txt --from ~/.local/share/unisacc/research/agy-flow/evidence-0.0.106/ape-receipt.txt`;写前核 12 行、含 p3/六目标/cache-manager/p3-package.body/unisacc-next.com 各行,路径不含家目录明文 |
| 2 | fixedpoint-6targets.tsv | release/evidence/0.0.106/fixedpoint-6targets.tsv | 暂存目录已有(10 行:3 行 # 头 + 6 目标行 + 1 行尾注释) | 同上 f write;写前核 chain_rev=2e8a19…、sources_sid=c6eeca9f…、ape_sha256=5f83ac93…,六行 verdict 全 SAME |
| 3 | INDEX 追行 | release/evidence/0.0.106/INDEX.tsv | 缺(目录尚未建 INDEX) | 建两行: 第一行 `fixedpoint-6targets	<YYYY-MM-DD HH:MM +0800>	PASS	fixedpoint-6targets.tsv	chain_rev=2e8a19a342, slices=same count=6, objects=same count=102, kernels=same count=2, product=same, package sha=50c2333e…`; 第二行 `ape-receipt	<同刻>	PASS	ape-receipt.txt	APE sha=5f83ac93… bytes=8932113; 六目标 g2 切片 path/sha/bytes 与 fixedpoint-6targets.tsv 逐行一致`(对齐 release/evidence/0.0.101/INDEX.tsv 的四列:TAB 分隔 项/时间/状态/文件/说明) |
| 4 | notes 语句 | release/notes-0.0.106.md | 缺(notes 尚未生成) | 在"证据"节加一句:`- (n) 六目标自举不动点:2096 链 compare 相位 slices=same count=6、objects=same count=102、kernels=same count=2、product=same,六目标 g2/g3 终片逐对 sha 相同;回执与逐目标摘要见 release/evidence/0.0.106/{ape-receipt.txt,fixedpoint-6targets.tsv},链 REV 2e8a19a342、APE 5f83ac93…` |

## 核对表(暂存目录 vs 链真实文件)

- ape-receipt.txt:暂存 12 行,与链 ~/.cache/unisacc/chains/bg-b1/2e8a19a342…/ape-final/ape-receipt.txt 逐行比对同内容(路径前缀同为 ~/.cache/unisacc/chains/bg-b1/2e8a19a342…);已备。
- fixedpoint-6targets.tsv:六目标 sha 与链 fixedpoint/g2|g3 终片 sha256sum 实测一致(本日核对:lnx-x86_64 a1d8b21e…, lnx-arm64 8a187532…, osx-x86_64 49e95644…, osx-arm64 2403451e…, win-x86_64 a63f5921…, win-arm64 7a258647…);已备。
- INDEX 与 notes:缺,按上表补。
- 未备:严格预跑与 CI 行的 INDEX 行不属本四项(fixedpoint/ape-receipt),需要时另按 105 INDEX 的 prefreeze-strict/postfreeze-strict/exec-3-ci 模板补。

## 校验命令(执行人用)

    sha256sum ~/.cache/unisacc/chains/bg-b1/2e8a19a342251910d53010f51002b14a4ca7f916/fixedpoint/g2/*/{entry.elf,slice-*}
    grep -c "" ~/.local/share/unisacc/research/agy-flow/evidence-0.0.106/ape-receipt.txt      # 期望 12
    grep -c "SAME" ~/.local/share/unisacc/research/agy-flow/evidence-0.0.106/fixedpoint-6targets.tsv  # 期望 6
