# 预跑完到 X 的逐分钟清单(可照抄;2026-11-25 ccogweco)

依据:agy-flow 的 car2-parallel-prep.md 第 1-4 分钟(该页不在本仓,以其既有命令为准)、cdx 的 gates/CI 页、
本窗口 rounds 与 freeze-107-row.md。命令在本页可照抄;与 agy-flow 页重复处以该页为准。

前置:预跑 `PRERUN_RUN status=DONE`;预跑目录 `~/.cache/unisacc/prerun/v0.0.107-28b6a518fe`;
车树 `.claude/worktrees/v0.0.107`;候选 APE `9a1e2630…`;F=`28b6a518fe`。

## T+0(补跑 18 窗,在 merge 之前)

    # 逐批串行,原窗并发口径;候选 APE 与预跑同;clean PATH;短 TMPDIR
    export PATH="/usr/bin:/bin:$HOME/.local/bin"; unset DVQUALIFY_NATIVE_RUN_BATCH
    BATCH=<id> sh ./unisacc.com tools/dvqualify.cx -- --jobs 1 --segment-batches 1
    # 判据:每批 BATCH_RUN id=<id> status=PASS rc=0;原 FAIL 行保留
    # 18 窗清单以 agy-flow 预跑暂存清单为准;全部完成才进 T+1

- 失败处置:任一批仍红 -> 停,报 cc,不 merge、不 pack;原行入 strict 的 SUPERSEDED 只在复跑绿后写。
- 负责:agy-flow;cdx 复核原行。

## T+1(merge origin/main + F1 终核)

    git -C .claude/worktrees/v0.0.107 fetch origin
    git -C .claude/worktrees/v0.0.107 merge origin/main
    sh ./unisacc.com tools/tables-gen.cx -- --write && sh ./unisacc.com tools/derive.cx -- --write
    sh ./unisacc.com tools/tables-gen.cx -- --check        # 期望 stale=0 hard=0
    sh ./unisacc.com tools/check-fwd-cap.cx -- <ROOT> <CANDIDATE>   # F1 判据:354 过、355 具名拒绝、413 起不崩

- 判据:merge rc=0;派生表自检通过;F1 三条按 plans/v0.0.107-chain-start.md 第 3 节原行。
- 失败处置:merge 冲突 -> 按 car2-next 的 four-table 解法(iface-hash/links/tlib/batch-product-inputs),不跳;
  F1 红 -> 停,报 cc(候选问题)。
- 负责:agy-flow;cdx 核 F1 原行。

## T+2(--pack 与 --warm)

    sh ./unisacc.com tools/release-run.cx -- --pack <ROOT> <CAND> <SRC_DIR> <OUT> --chain-out <CHAIN> [--snap <SNAP>]
    # 期望:RELEASE_RUN MODE PACK v0.0.107、SOURCES_SID VERIFIED 9b83432e…、FIXEDPOINT VERIFIED、
    #       PACK_OK、六目标与 p3 逐行 sha
    sh ./unisacc.com tools/release-exec.cx -- --warm <OUT>/candidate.tsv --layout <OUT>/layout --out <STATE>/warm
    # 期望:WARM ... READY key=<64hex>;同 argv 第二次毫秒级

- 失败处置:pack 拒绝 -> 看 REFUSE 类(ASSET_IDENTITY/PACK_RECEIPT)逐项核源目录;warm 非 READY -> 重跑一次,
  仍不就停,不公开。
- 负责:agy-flow;cdx 核 SOURCES_SID 与 APE。

## T+3(--dry 与 precommit candidate.tsv 预热)

    sh ./unisacc.com tools/release-run.cx -- --dry-run <ROOT> <CAND> <LAYOUT> <OUT_DRY>
    git -C .claude/worktrees/v0.0.107 add release/candidate.tsv
    sh ./unisacc.com tools/precommit.cx     # 期望 PRECOMMIT staged_cx=<N> blocked=0
    sh ./unisacc.com tools/devsvc.cx -- git unstage release/candidate.tsv   # 仅在需要撤回暂存时,车树属主执行

- 判据:dry-run 无 REFUSE;precommit 仅候选表的提交 blocked=0;不改候选表内容。
- 失败处置:dry 有 REFUSE -> 按门名修输入,不跳过;precommit 红 -> 先看是否远程表漂移(derive),不删断言。
- 负责:agy-flow。

## T+4(Step 1 X + Step 2 草稿 + 五资产核对)

    sh ./unisacc.com tools/release-exec.cx -- --execute release/candidate.tsv --layout <LAYOUT> --from-step 1
    # 期望:STEP 1 X-commit DONE;INDEX exec-1-X-commit
    # 草稿与五资产核对(派 CI 前):
    gh release view v0.0.107 --json isDraft,assets --jq '{isDraft,names:[.assets[].name]}'
    # 期望 isDraft=true,五个资产名齐
    gh release download v0.0.107 -p slice-win-arm64 -D /tmp/guard-107 --clobber
    sha256sum /tmp/guard-107/slice-win-arm64   # 与候选表 win_arm64_sha256 相等
    # 期望:STEP 2 draft DONE、五资产匹配;INDEX exec-2-draft-readback

- 失败处置:资产 404 -> 等 30 秒重试一次,仍缺不派 CI;X 失败 -> 读门名,不推快。
- 负责:agy-flow;cdx 核五资产与 CI 前置一至四。

## 归档前的 rm 前置(在证据归档开始前)

    sha256sum release/evidence/0.0.107/prerun-windows.tsv | tee /tmp/prerun-windows.sha
    # 记录后删除证据目录里的预拷件(以正式克隆件为准;由证据归档负责人执行)
    rm -f release/evidence/0.0.107/prerun-windows.tsv

- 判据:记录 sha 后删;正式归档时从预跑目录重新克隆,不沿用预拷件。
- 失败处置:未记录就删 -> 补记 sha 再删;不得留旧拷贝冒充正式件。

## 时间与硬线

- 四分钟是串行预算;任一步超出只顺延,不改时限。T2+45(04:00:16)已按保守档记必超。
- 本页只写清单,不执行;补跑 18 窗位于 T+0,排在 merge 之前。
