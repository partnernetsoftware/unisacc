# 0.0.106 路径 B 人工证明

## R40 B-t 六文件实核

冻结链提交为 `2e8a19a342251910d53010f51002b14a4ca7f916`，最终补跑头为 `8ca2f5e4b89b38d9b5dff074c127989572f45833`，直接父提交为 `879e446640`。沿用第 (2) 和 (3) 节的链 helper、回执、归档与 SID 重算。本节以下列数组替换三文件命令中的路径和冻结范围比较。

    paths=(build/freeze.tsv tests/check-fwdstub-replay.cx tests/env-semantics.tsv tools/check-chain-ready-freeze.cx tools/check-gen-all-check.cx tools/check-prepush.cx)
    git -C "$W" diff --name-only -z "$F" "$H" > "$S/paths.z"
    printf '%s\0' "${paths[@]}" | cmp -s - "$S/paths.z"
    git -C "$W" diff --name-only -z "$F" "$H" -- src cx ext exec include seed kernel tests release > "$S/frozen.z"
    printf '%s\0' tests/check-fwdstub-replay.cx tests/env-semantics.tsv | cmp -s - "$S/frozen.z"

NUL 精确比较通过。冻结路径内只含 G51 放行的 W0191 夹具与 G52 放行的 W0206 活表。前者 blob 为 `39a46182bba3ab020e7bb25cc8a7afc7b0037121`，后者为 `ae0dcb7eb4c84defd67022a720301cca7d65949f`。W0189 的 `build/freeze.tsv` 单独把 deadline 刷新至 `1795546800`，其余字段维持不变。构造钉项零差异。

    PATH_B_DIFF	PASS	chain=2e8a19a342251910d53010f51002b14a4ca7f916	head=8ca2f5e4b89b38d9b5dff074c127989572f45833	files=6	frozen_paths=2	freeze_deadline=1795546800	support_diff=0
    PATH_B_SID	PASS	head=8ca2f5e4b89b38d9b5dff074c127989572f45833	sid=c6eeca9f10d66039ce49f5f74ee1ec63727bd85e552f342965d8445136bd812d

六条输入排除原行见 `research/cdx/r40-evidence/six-file-proof.log`。真实补跑与发布门预演独立留证。

    REASON='R40 B-t 六路径：W0087 与 W0148 工具夹具，W0189 树内冻结续期，W0191 APE 启动夹具，W0206 活表登记。两个 tests 路径经 G51/G52 放行，六路径均不在 SID，重算与原链一致。证据 path-b-proof.md'

## R40 B-t 五文件历史车头

当前车头 `879e446640` 的路径范围为以下五项。三文件节保留 `d12d2087bd` 的历史核验。W0191 的测试夹具属于 R40 明确批准的 B-t 范围。W0189 仅更新树内冻结截止值，重跑必须使用该 Git 树。

    paths=(build/freeze.tsv tests/check-fwdstub-replay.cx tools/check-chain-ready-freeze.cx tools/check-gen-all-check.cx tools/check-prepush.cx)
    git -C "$W" diff --name-only -z "$F" "$H" > "$S/paths.z"
    printf '%s\0' "${paths[@]}" | cmp -s - "$S/paths.z"
    git -C "$W" diff --quiet "$F" "$H" -- src cx ext exec include seed kernel release tests ':(exclude)tests/check-fwdstub-replay.cx'
    git -C "$W" diff --quiet "$F" "$H" -- ':(glob)tests/**/*.tsv'

SID 仍以同一链内 helper 的 `list-current` 与从 H 重算的完整摘要为准。R40 的 P1、P2 与批准路径见 `research/ompgpt/w0191-bt-proof.md`。W0150 的 r39c 补丁属于第二车。

## 补跑与发布陷阱

W9998 的 PATH 使 `moltbaby/bin/cc` 遮蔽系统编译器。夹具调用 `cc -std=c99` 后输出 `--provider required for -std=c99 mode`。其他贴限行同时受补跑并发影响，须以原窗并发口径复测。补跑、dry 和正式发布均先载入干净 PATH，核实系统编译器和发布客户端。

    export PATH="/usr/bin:/bin:$HOME/.local/bin"
    test "$(command -v cc)" = /usr/bin/cc
    command -v gh

同候选、同补跑头逐批执行，BATCH 为本次唯一批名，STRICT_OUT 为本次全新 TSV 路径。任何批次的并发上限沿用原窗。默认短 TMPDIR，保持单批 55 秒与 5 秒清理限时。

    cd "$FROZEN_ROOT"
    TMPDIR=/tmp timeout -k 5s 55s sh ./unisacc.com tools/dvqualify.cx -- . --candidate "$APE" --ids "$BATCH" --out "$STRICT_OUT" --jobs 1 --segment-batches 1

## W0150 第一车补跑与披露

r39c 排入 0.0.107 追加集第 (11) 项。第一车补跑与严格表合并由 agy-flow 执行。

第一车预跑结束且环境空闲后，在冻结源码与同一候选 APE 上，经正式 `tests/run-batch.cx` 使用默认短 TMPDIR 串行复跑 `release-exec-c` 三次。各次使用全新输出目录，沿用 55 秒限时，每次须见 `BATCH_RUN id=release-exec-c status=PASS rc=0 transport=0 assertions=55`、同一候选摘要及 `CHECK_RELEASE_EXEC PASS part=c failed=0`。保留原窗口 FAIL 与独立补跑原行。

    cd "$CANDIDATE_LAYOUT"
    TMPDIR=/tmp timeout -k 5s 55s sh ./unisacc.com tests/run-batch.cx -- "$CANDIDATE_LAYOUT" "$NEW_OUT_PARENT" release-exec-c

第一车发布说明与补跑来源记录。W0150 原预跑车道的长临时路径使 `sha256sum` 输出超过 `release-exec` 的 200 字节摘要捕获缓冲，导致草稿下载回读误报摘要不符。以同候选、同批次在默认短 TMPDIR 补跑，原 FAIL 保留。长路径修复排入 0.0.107。补跑成绩以实际回执填写。

## 三文件复核增量

第二笔夹具修复落地后，以下代码替换第 1 节。第 2 节保留 helper、收据、归档与输入清单生成步骤，单路径 grep 改用下方三路径循环。第 3 节仍从新 HEAD 的同一归档重算完整 SID。单文件的实核回执保留作历史。

    set -euo pipefail
    export GIT_OPTIONAL_LOCKS=0
    W="$HOME/repos/unisacc/.claude/worktrees/v0.0.106"
    F=2e8a19a342251910d53010f51002b14a4ca7f916
    R39=c698ef0091ea11b9bcf7f29e46d213ab69b7ba57
    CHAIN="$HOME/.cache/unisacc/chains/bg-b1/$F"
    H=$(git -C "$W" rev-parse --verify HEAD)
    S=$(mktemp -d /tmp/unisacc-path-b-XXXXXXXX)
    test "$(git -C "$W" rev-parse "$R39^")" = "$F"
    test "$(git -C "$W" rev-parse "$H^")" = "$R39"
    paths=(tools/check-chain-ready-freeze.cx tools/check-gen-all-check.cx tools/check-prepush.cx)
    git -C "$W" diff --name-only -z "$F" "$H" > "$S/paths.z"
    printf '%s\0' "${paths[@]}" | cmp -s - "$S/paths.z"
    git -C "$W" diff --name-status "$F" "$H"
    git -C "$W" diff "$R39" "$H" -- "${paths[@]}"
    git -C "$W" diff --quiet "$F" "$H" -- src cx ext exec include seed kernel tests release
    printf 'PATH_B_DIFF\tPASS\tchain=%s\thead=%s\tfiles=3\tfrozen_paths=0\n' "$F" "$H"

期望三个 M，顺序为 check-chain-ready-freeze、check-gen-all-check、check-prepush。第二笔只改两处夹具文件中的四个链接点。完成第 2 节的 inputs.paths 生成后执行以下循环。

    for path in "${paths[@]}"; do
        rc=0; grep -Fxq "$path" "$S/inputs.paths" || rc=$?
        test "$rc" -eq 1
        printf 'PATH_B_INPUT\tPASS\tfile=%s\tin_sid=0\n' "$path"
    done

三文件终稿理由在新头实核后使用。INDEX 五列结构沿用第 4 节，ref 和说明里的 HEAD 均填最终头，链头仍为 F。

    REASON='路径 B，仅修 W0087 与 W0148 的三个检查夹具文件。冻结路径无差异，SOURCES_SID 重算相同。证据 path-b-proof.md'

发布说明句式。0.0.106 整链后修正 W0087 与 W0148 的检查夹具，仅涉及三个工具检查文件，冻结路径零差异且 SOURCES_SID 重算与链头逐字一致，提交 X 的头差异见 `exec-1-post-chain-merge`。最终头未落地前，本节仅为待执行步骤。

## 单文件历史方案

只在 agyneon 的单文件夹具修复提交落地后执行。冻结链提交是 `2e8a19a342251910d53010f51002b14a4ca7f916`。保留原候选 APE、打包源和冻结提交；修订 HEAD 只给后补验证和提交 X 使用。任一步不符即停。下列命令用 Bash 执行，留存标准输出与退出码。

## (1) 固定提交身份与精确写域

    set -euo pipefail
    export GIT_OPTIONAL_LOCKS=0
    W="$HOME/repos/unisacc/.claude/worktrees/v0.0.106"
    F=2e8a19a342251910d53010f51002b14a4ca7f916
    CHAIN="$HOME/.cache/unisacc/chains/bg-b1/$F"
    H=$(git -C "$W" rev-parse --verify HEAD)
    P=tools/check-gen-all-check.cx
    S=$(mktemp -d /tmp/unisacc-path-b-XXXXXXXX)
    test "$H" != "$F"
    test "$(git -C "$W" rev-parse "$H^")" = "$F"
    git -C "$W" diff --name-only -z "$F" "$H" > "$S/paths.z"
    printf '%s\0' "$P" | cmp -s - "$S/paths.z"
    git -C "$W" diff --name-status "$F" "$H"
    git -C "$W" diff "$F" "$H" -- "$P"
    git -C "$W" diff --quiet "$F" "$H" -- src cx ext exec include seed kernel tests release
    printf 'PATH_B_DIFF\tPASS\tchain=%s\thead=%s\tfiles=1\tfrozen_paths=0\n' "$F" "$H"

期望唯一一行 `M<TAB>tools/check-gen-all-check.cx`，正文只改最小仓夹具。冻结范围以当前批准行核对，以上九项对应本车 `build/freeze.tsv`。若修订树后来多一个提交，先重新核范围与授权，勿直接移除单父检查。

## (2) 核对真正的 SID 输入与构造钉项

使用原链已构造的 `build/identity-helper`。其正常模式对应该版 `seed/ident.c:278–312`；第 101–115 行是历史投影，也列相同 13 个 tools 支撑项。用 `list-current` 直接查实际范围，并额外比较 sourcepackage 构造支撑与验证模块。后者有重叠，须按路径核，不混作 SID 的定义。

    IH="$CHAIN/build/identity-helper"
    CTX="$CHAIN/ctx/context.tsv"
    SID=c6eeca9f10d66039ce49f5f74ee1ec63727bd85e552f342965d8445136bd812d
    printf '%s\n' "$SID" > "$S/expected.sid"
    test -x "$IH"
    grep -Fqx "rev=$F" "$CHAIN/build/identity-helper.receipt"
    IH_SHA=$(sha256sum "$IH" | cut -d' ' -f1)
    grep -Fqx "output_sha=$IH_SHA" "$CHAIN/build/identity-helper.receipt"
    grep -Fqx "$(printf 'identity-helper\t%s' "$IH_SHA")" "$CTX"
    grep -Fqx "$(printf 'source\t%s' "$SID")" "$CTX"
    mkdir "$S/head"
    timeout -k 5s 55s bash -o pipefail -c 'git -C "$1" archive "$2" | tar -xf - -C "$3"' bash "$W" "$H" "$S/head"
    timeout -k 5s 55s "$IH" "$S/head" list-current > "$S/inputs.tsv"
    test -s "$S/inputs.tsv"
    cut -f1 "$S/inputs.tsv" > "$S/inputs.paths"
    rc=0; grep -Fxq "$P" "$S/inputs.paths" || rc=$?
    test "$rc" -eq 1
    git -C "$W" diff --quiet "$F" "$H" -- \
      tools/bound tools/bound.c tools/cxbin tools/safedel.cx tools/lib.sh \
      tools/build_ref.sh tools/export_ref.sh tools/refshim.h tools/reffoot.h \
      tools/sourceflat.cx tools/ape-pack.cx tools/release-ape.cx tools/cxlib/releaseversion.cx \
      tools/cxlib/netpack.cx tools/source-package.cx tools/source-package-pack.cx \
      tools/cxlib/sourcepackage.cx tools/cxlib/kernelsha.cx tools/net-deps.cx
    printf 'PATH_B_INPUTS\tPASS\tfile=%s\tin_sid=0\tsupport_diff=0\n' "$P"

`sourcepackage.cx:862` 使用的五项为 bound、bound.c、netpack、source-package 与 source-package-pack；另覆盖模块自身、:269 的 kernelsha 和 :530 的 net-deps。期望 grep 不命中，且支撑项无差异。

## (3) 重算完整 SID 和候选绑定

    timeout -k 5s 55s "$IH" "$S/head" > "$S/head.sid"
    cmp "$S/expected.sid" "$S/head.sid"
    cat "$S/head.sid"
    sha256sum "$CHAIN/ape-final/unisacc-next.com"
    printf 'PATH_B_SID\tPASS\thead=%s\tsid=%s\n' "$H" "$SID"

期望 SID 原行逐字等于 `c6eeca9f10d66039ce49f5f74ee1ec63727bd85e552f342965d8445136bd812d`。已在原链快照上用同一 helper 只读复核，实得上述 SID，墙钟约 0.324 秒。修订 HEAD 尚待实际执行，不能沿用原链的实测结果。期望 APE SHA256 为 `5f83ac93d201b252ea0b56098475be9e30045875cdf1ce692451cda095ceeb3c`，再与候选表及最终 strict 对账。打包后将实际 `SRCDIR/sources.tsv` 的唯一非注释摘要与同一 SID 对比，要求打包原行 `SOURCES_SID VERIFIED`。打包的候选表 commit 仍为 `F`。

    grep -v '^#' "$SRCDIR/sources.tsv" > "$S/pack.sid"
    cmp "$S/expected.sid" "$S/pack.sid"

封存证据留 git diff、两头 SHA、helper SHA、SID 原行与输入清单摘要。公开归档用相对证据引用，省略主机绝对路径。

## (4) 豁免参数、INDEX 与说明

    REASON="R39 路径 B，仅修 W0087 检查夹具；单文件差异、冻结路径与 SOURCES_SID 已人工复核。证据 path-b-proof.tsv"
    sh ./unisacc.com tools/release-exec.cx -- --execute release/candidate.tsv \
      --layout "$LAYOUT" --strict "$STRICT" --out "$DRY_OUT" --dry \
      --post-chain-merge "$REASON"

从修订 HEAD 根执行，布局与合并后的 strict 都指同一 APE。参数为一行且无制表符，最多 399 字节。这里只给 dry 原型。正式执行沿既定发布授权。dry 的 INDEX 写入独立 `DRY_OUT`，正式 INDEX 写入本版证据目录。若 X 已完成，头相等门可能按收据跳过，需要核既有豁免行。

期望门原行包含 `GATE 1 F_CHAIN_REV PASS` 及 `HEAD=<H> chain_rev=<F> waived: <REASON>`。真实 INDEX 五列如下，分隔符为制表符。

    exec-1-post-chain-merge    <UTC 时间戳>    WAIVED    <H 全长>    HEAD=<H> chain_rev=<F> waived: <REASON>

若同键已存在，执行器不再追加；现有行的 H、F 和证据引用必须一致。单独归档 W0087 原 FAIL 与后补 PASS 原始 TSV、两套 source 提交、同候选摘要和最终 `SUPERSEDED`。切勿用豁免解释其他红项。

发布说明句式。第一车整链后仅在 `<H>` 修正 W0087 检查夹具，冻结产品路径零变化且重算 SOURCES_SID 与 `<F>` 逐字一致；保留原 FAIL 与同 APE 后补 PASS，并以 `exec-1-post-chain-merge` 记录提交 X 的头差异。
