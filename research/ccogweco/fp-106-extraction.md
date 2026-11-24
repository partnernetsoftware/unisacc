# 第一车(0.0.106)六目标不动点取证(只读)

链路: REV 2e8a19a342251910d53010f51002b14a4ca7f916,目录 ~/.cache/unisacc/chains/bg-b1/2e8a19a342251910d53010f51002b14a4ca7f916/,exit.tsv rc=0。私有树基线 v0.0.107 头 d750ad123c 的 git archive(git archive|tar 解到 /tmp),未写仓库 release/evidence。

## 工具验证

    sh ./unisacc.com tools/fixedpoint-sample.cx -- release --sample
    REFUSE	release-candidate-forbids-sample        (rc=2,拒绝路径成立)
    sh ./unisacc.com tools/fixedpoint-sample.cx -- release <chain>/fixedpoint/g2 <chain>/fixedpoint/g3
    release: need GEN2/all.lst                      (rc=2,链目录布局无 all.lst,该工具的 release 全量模式不能直接吃链目录)

结论: fixedpoint-sample 的 release 全量抽取对链目录不适用(只认 GEN2/all.lst);但链自己的固定点相位已写出判定,且六目标终片可直接逐对核摘要。

## 链的固定点判定(chain-run-249.stdout 原行)

    CHAIN	fixedpoint	objects=same	count=102
    CHAIN	fixedpoint	kernels=same	count=2
    CHAIN	fixedpoint	slices=same	count=6
    CHAIN	fixedpoint	product=same
    CHAIN	fixedpoint	package=present	sha=50c2333e2aa9747e3f65dd16f0c4579c59dda5b2fb5d2164722987fb526148a4
    RECEIPT	compare	wall_ms=3812
(checkpoint.tsv: `DONE compare wall_ms=3812`)

## 六目标 g2 与 g3 终片逐对摘要(同目录比对)

| 目标 | g2 sha256 前 16 | g3 sha256 前 16 | 相等 |
|---|---|---|---|
| lnx/x86_64 (entry.elf) | a1d8b21e821230a6 | a1d8b21e821230a6 | 是 |
| lnx/arm64 (entry.elf) | 8a1875325992b298 | 8a1875325992b298 | 是 |
| osx/x86_64 (slice-osx-x86_64) | 49e9564498360a2f | 49e9564498360a2f | 是 |
| osx/arm64 (slice-osx-arm64) | 2403451edcd2a152 | 2403451edcd2a152 | 是 |
| win/x86_64 (slice-win-x86_64) | a63f59212afbf1b9 | a63f59212afbf1b9 | 是 |
| win/arm64 (slice-win-arm64) | 7a2586471c7d02ef | 7a2586471c7d02ef | 是 |

## 回执(ape-final/ape-receipt.txt 原行)

    win/x86_64  .../fixedpoint/g2/win-x86_64/slice-win-x86_64  sha=a63f59212afbf1b9c58877d626bebde22dd94d6a2b464968d6642625a5b0e3cb bytes=3480576
    lnx/x86_64  .../fixedpoint/g2/lnx-x86_64/entry.elf          sha=a1d8b21e821230a62bc948d70e8ad23b7025237a4c076718710c57cdaf640da1 bytes=1929534
    lnx/arm64   .../fixedpoint/g2/lnx-arm64/entry.elf           sha=8a1875325992b298809fbcef893e9b6572cb47f9a3c86a20a7a35ecccec20f8b bytes=2081030
    osx/x86_64  .../fixedpoint/g2/osx-x86_64/slice-osx-x86_64   sha=49e9564498360a2f49ab439655cfeee46d6cb420eb5ce75aec10c3de21f42844 bytes=3170530
    osx/arm64   .../fixedpoint/g2/osx-arm64/slice-osx-arm64     sha=2403451edcd2a152022e58b66021d02f5d04201f35967f20c3fb16c1d3974916 bytes=4276834
    win/arm64   .../fixedpoint/g2/win-arm64/slice-win-arm64     sha=7a2586471c7d02ef6fe19c151e53b5dd031d7ed3e8d818fab3f875a468b27813 bytes=4662272

## 能不能取出

能。第一车(106)的六目标 g2==g3 由链内固定点相位直接给出(slices=same count=6),六对终片摘要逐对相等,ape-final 回执含六目标 g2 切片的 path/sha/bytes。缺的是归档动作(把上面 claim 行、回执行与六对摘要写进 release/evidence/0.0.106/),不是取证能力;fixedpoint-sample 的 release 全量模式需要 GEN2/all.lst,本链布局没有,故没有用它,也不该把它的 rc=2 当失败——是布局不适用。
