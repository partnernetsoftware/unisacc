# cc 起链当天执行脚本(02:35-02:55;2026-11-25 ccogweco)

依据:agy-flow stage-c run 记录(私有日志,结论已引)、cdx car2-chain-ready-gates.md、本窗口
pre-chain-crosscheck.md、freeze-107-row.md。全部命令前缀 `DEVSVC_OWNER=cc`;冻结行 deadline 用
**1795559400**(2026-11-25 06:30 +0800)。共享检出只提交自己的 build/freeze.tsv。

## 0 统一环境(每个命令前先执行一次)

    export PATH="/usr/bin:/bin:$HOME/.local/bin"
    export DEVSVC_OWNER=cc
    export TMPDIR="$S/tmp"; mkdir -p "$TMPDIR"
    unset UNISACC_INTEGRATOR DVQUALIFY_NATIVE_RUN_BATCH UNISACC_RUN_CACHE UNISACC_UNIT_CACHE UNISACC_FREEZE_NO_FETCH
    command -v cc            # 期望 /usr/bin/cc
    echo "$PATH" | tr ':' '\n' | grep -F 'moltbaby/bin'   # 期望无输出

## 1 02:35 起链前核对(约 2 分钟)

    cd ~/repos/unisacc
    git status --porcelain
    git rev-parse HEAD origin/main
    git rev-list --count origin/main..HEAD      # 期望 0(ahead)
    git rev-list --count HEAD..origin/main      # 期望 0(behind)

判据:工作树干净;两 SHA 相同;ahead/behind 都是 0。若 ahead>0(之前做过本地合并提交):先 `git log --oneline origin/main..HEAD`
看是哪几笔,按其派生表重算或由 cc 决定前向 revert,不得带 ahead 起链。

## 2 02:38 取冻结锁(约 10 秒)

    DEVSVC_OWNER=cc sh ./unisacc.com tools/devsvc.cx -- lock acquire freeze --owner cc --ttl 5400

判据:输出 acquire 成功;`devsvc lock list` 可见 freeze/cc。失败处置:owner 已持有则先 list 确认是否本窗口残留,
再 release 后重取;不抢他人锁。

## 3 02:40 写 0.0.107 冻结行(约 1 分钟)

先看当前文件内容(106 行删除后应只剩注释):

    cat build/freeze.tsv

    # old(若空文件)不要用空 old(edit 要求旧文本恰一次):改用整文件写
    #   devsvc f write build/freeze.tsv --from /tmp/freeze-new.txt --expect-sha <空文件 sha>

写入与提交:

    DEVSVC_OWNER=cc sh ./unisacc.com tools/devsvc.cx -- f edit build/freeze.tsv --old-file /tmp/freeze-old.txt --new-file /tmp/freeze-new.txt
    DEVSVC_OWNER=cc sh ./unisacc.com tools/commit-own.cx -- -m "0.0.107: 写冻结行(base=<BASE>,owner=cc,deadline=1795559400)" --whole build/freeze.tsv
    DEVSVC_OWNER=cc sh ./unisacc.com tools/devsvc.cx -- git push "$(git rev-parse HEAD)" --cwd .

判据:`grep -P '^0\.0\.107-FREEZE\t' build/freeze.tsv` 四段以上且 deadline=1795559400;push 成功。
失败处置:f edit ENOMATCH 说明旧文本与实文件不符,重取旧行再改;push 被 prepush 拒(说明还有旧 active 行)
则先按 freeze-107-row.md 清 106 行,不删新行重写。

## 4 02:45 复核同 SHA 与 ahead/behind(约 1 分钟)

    git rev-parse HEAD origin/main
    git rev-list --count HEAD..origin/main         # 期望 0
    git rev-list --count origin/main..HEAD         # 期望 0(冻结行提交已推)

判据:三条全真。失败处置:behind>0 先 `devsvc git merge origin/main`(会改 SHA,需重写 base 并复跑第 3 步);
ahead>0 检查 push 是否真的成功。

门 5 预备(全量之前先各跑一次;写法以 tools/check-candidate-chain.cx 现行源码为准,不是旧手册的 "OUT CACHE --prepare"):

    DEVSVC_OWNER=cc sh ./unisacc.com tools/check-candidate-chain.cx -- --prepare "$TMPDIR/unisacc-cr-prep-cache" tool
    DEVSVC_OWNER=cc sh ./unisacc.com tools/check-candidate-chain.cx -- --prepare "$TMPDIR/unisacc-cr-prep-cache" generator

判据:两条都打 PREPARED candidate-check tool|generator,重跑出现 reused=1 reason=identity-hit;全门里门 5 才能 <54 秒。

门 22 提醒:stamp 命中只省预合树生成;premerge-union.cx:672 起的 prep 仍重查生成器,不能在全门预算里把重复 prep 算零。

## 5 02:47 chain-ready 全量(约 5 分钟)

    DEVSVC_OWNER=cc sh ./unisacc.com tools/chain-ready.cx -- --repo "$PWD" --root "$PWD" \
        --rev "$(git rev-parse HEAD)" --version 0.0.107

判据:末行 `READY yes failed=none`;23 号 `armed: freeze.tsv sid=0.0.107-FREEZE owner=cc deadline=1795559400;
HEAD behind origin/main=0; numbering files unchanged after the freeze`。
失败处置:任一门 FAIL 只重跑该门;FREEZE_BEHIND_MAIN -> 回第 4 步;FREEZE_TLIB_AFTER_FREEZE -> 编号改动须在
冻结前完成,已冻结后取号要重写冻结行并复跑 23;不跳过判据起链。

## 6 02:55 起链(约 28 分钟到链止)

    S=$(mktemp -d /tmp/cc-chain-XXXXXXXX)
    DEVSVC_OWNER=cc sh ./unisacc.com tools/bg-candidate.cx -- start --rev "$(git rev-parse HEAD)" \
        --deadline-s 5400 --root "$PWD" --base "$HOME/.cache/unisacc/chains/bg-b1" -- --parallel 4

判据:start 立即返回并写 `BASE/<rev>/owner.tsv`;`bg-candidate status <rev>` 显示 RUNNING;`bg.log` 首行出现
`CHAIN RUN`;链止后 `exit.tsv` rc=0、fixedpoint 四同。
失败处置:OWNER 存在且进程活 -> 不重起,用 status 看相位;首相位 5 分钟无输出 -> 保留原行报 cc,不换说法重试;
budget 用尽(-deadline-s 5400)一律按失败记录,不延长。

## 7 时间账(基准)

| 步 | 时点 | 预计 |
|---|---|---|
| 核对 | 02:35 | 2 分 |
| 锁 | 02:38 | 10 秒 |
| 冻结行 + 提交 + push | 02:40 | 1-2 分 |
| 复核 | 02:45 | 1 分 |
| chain-ready 全量 | 02:47 | 5 分 |
| 起链 | 02:55 | 28 分(链止约 03:23) |

之后按 chain-day-roster.md 每 10 分钟值守;硬线 45(链止到 X)与 120(起链到公开)不动。
