# 0.1.0 链止后回填取值脚本草案(只读;2026-11-25 ccogweco)

对象:research/ccogweco/notes-0.1.0-final.md 的"整链"与身份/固定点占位。F5=a11d28fc0a。
本页只给命令串与期望原行,链止后原样跑,不预填值、不改仓。

## 0 变量

    F=a11d28fc0a61939ee71345d732a7dcf205b261f1
    CHAIN=~/.cache/unisacc/chains/bg-b1/$F
    DRAFT=research/ccogweco/notes-0.1.0-final.md

## 1 计时点 T1/T2(写入"整链与固定点"与条件 6)

    # T1:owner.tsv 首行 epoch(或链目录建立时间)
    head -1 "$CHAIN/owner.tsv"                  # 期望 OWNER ... <epoch>
    # T2:exit.tsv
    cat "$CHAIN/exit.tsv"                        # 期望 EXIT <epoch> pid=... rc=0 why=exited
    # 用时:两个 epoch 相减,分钟保留一位

- 填入句:"T1=<...> -> T2=<...>,rc=0,<n> 分钟"。

## 2 链内固定点四同与 package

    grep -h 'CHAIN	fixedpoint' "$CHAIN"/*.stdout | head -8
    # 四同与 package 在 chain-run-<N>.stdout(107 为 chain-run-245.stdout),不在 bg.log;
    # package=present sha=<64hex> 以实际 grep 行取

- 填入"整链与固定点"节;package sha 同步到身份表附近(如列)。

## 3 六目标 g2/g3 逐对(条件 1 第三版)

路径映射(107 演练实测;0.1.0 若不同按 ls 实际改):

    lnx-x86_64/entry.elf        lnx-arm64/entry.elf
    osx-x86_64/slice-osx-x86_64 osx-arm64/slice-osx-arm64
    win-x86_64/slice-win-x86_64 win-arm64/slice-win-arm64

    for r in lnx-x86_64/entry.elf lnx-arm64/entry.elf osx-x86_64/slice-osx-x86_64 \
             osx-arm64/slice-osx-arm64 win-x86_64/slice-win-x86_64 win-arm64/slice-win-arm64; do
      echo "$r $(sha256sum "$CHAIN/fixedpoint/g2/$r" | cut -d' ' -f1) $(sha256sum "$CHAIN/fixedpoint/g3/$r" | cut -d' ' -f1)"
    done

- 期望:每行两个 sha 相同;不同即停,报 cc。

## 4 APE5 与 SID5(身份表)

    sha256sum "$CHAIN/ape-final/unisacc-next.com"          # APE5
    head -3 "$CHAIN/ape-final/ape-receipt.txt"             # 六切片 path/sha/bytes
    # 链阶段没有 pack:p3.pkg 不存在(ape-final 只有 p3-package.body),不要提前找;
    # SID5 留到 pack:grep -m1 'RELEASE_RUN	SOURCES_SID	VERIFIED' <pack>/pack-run.out

- 链止后只能填 APE5 与回执;SID5 留到 pack 后填,不提前用 107 的 SID。


    sha256sum "$CHAIN/ape-final/unisacc-next.com"          # APE5
    head -3 "$CHAIN/ape-final/ape-receipt.txt"             # 六切片 path/sha/bytes
    # SID5 以 pack 输出为准(链阶段没有 pack):
    #   grep -m1 'RELEASE_RUN	SOURCES_SID	VERIFIED' <pack>/pack-run.out

- 链止后只能填 APE5 与回执;SID5 留到 pack 后填,不提前用 107 的 SID。

## 5 INDEX 与后续(链止后可能还没有)

    grep -E 'exec-1-post-chain-merge|exec-1-X-commit' release/evidence/0.1.0/INDEX.tsv

- 没有输出 = 尚未到该步,保持占位;不得用手写行补。

## 6 回填映射

| notes 位置 | 取值 | 上线时点 |
|---|---|---|
| 整链与固定点:T1/T2/用时 | §1 | 链止即填 |
| 固定点四同与 package | §2 | 链止即填 |
| 条件 1 第三版叙述 | §3 | 链止即填 |
| 身份表 APE5 | §4 | 链止即填 |
| 身份表 SID5 | §4 的 pack 行 | pack 后 |
| 条件 6 T1/T4 与两口径 | §1 + INDEX exec-5-publish | 公开后 |
| strict 计数 | 合并 strict SUMMARY | 预跑后 |

## 7 边界

- 全部只读;命令不写仓、不写缓存;发现任何值与 107 基准不符(如切片不等)立即停并报 cc。
- 本页不预写任何结果;占位在值取到前保持。
