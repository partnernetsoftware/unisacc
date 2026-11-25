# 107 证据收集模板预建(2026-11-25 ccogweco)

参照 release/evidence/0.0.106/(实为 33 个文件,cc 记 24 是主文件口径)与 INDEX.tsv 行类型。
本页只列文件、来源命令、产出时点、产出人、是否链后才有,并给空壳头;不预写任何结论。

## 0 产出总表

| 文件 | 来源命令原行 | 产出时点 | 谁产出 | 链后才有 |
|---|---|---|---|---|
| ape-receipt.txt | 从 `<chain>/ape-final/ape-receipt.txt` 脱敏拷贝(devsvc f write) | 链止后 | agy-flow | 是 |
| fixedpoint-6targets.tsv | 链 `<chain>/fixedpoint/g2,g3` 六目标逐对 sha + `compare` 原行 | 链止后 | agy-flow | 是 |
| pack-run.out | `release-run --pack` 的原输出(inputs 的 pack-log) | pack 后 | agy-flow | 是 |
| cache-manager.tsv | release-flow prep 的 evidence 输出 | prep 后 | agy-flow | 是 |
| ratchets.txt / rollback.txt / evidence-rows.tsv | release-flow prep/stage-a 的 evidence 输出 | prep/stage-a | agy-flow | 是 |
| strict.tsv | `release/evidence/v0.0.107-strict-<DATE>.tsv`(预跑合并后) | 预跑完 | agy-flow | 是 |
| strict-W*.tsv(补跑窗) | 每窗单独 `strict-W<ID>-<name>.tsv` | 补跑时 | agy-flow | 是 |
| expected-fail.tsv | release-flow prep 生成 | prep 后 | agy-flow | 是 |
| expected-unmeasured.tsv | 预跑/无生产者项登记输出 | 预跑完 | agy-flow | 是 |
| cx-booter-ape-deferred.tsv / tools-default-regression-deferred.tsv | 对应批 deferred 输出 | 预跑完 | agy-flow | 是 |
| prerun-supervise.log / prerun-windows.tsv | 预跑目录克隆 | 预跑完 | agy-flow | 是 |
| warm-<key>.tsv | `release-exec --warm` 收据 | pack 后/公开前 | agy-flow | 是 |
| draft-readback.txt | 草稿 `gh release download` 后由 release-exec 写 | 草稿读回 | agy-flow | 是 |
| public-readback.txt | 公开后读回 | 公开后 | agy-flow | 是 |
| ci-<run>.json / ci-<run>.log(每次 run 一对) | `gh run view <id> --json ...` 与日志下载 | CI 完 | cdx/agy-flow | 是 |
| backup-evidence.tsv | release-chain backup-evidence --verify | X 前 | agy-flow | 是 |
| release-post-state.tsv | release-post 输出 | 公开后 | agy-flow | 是 |
| INDEX.tsv | 下列各步骤写五列行 | 全程 | agy-flow | 是 |
| path-b-proof.md / post-chain-merge-provenance.txt | 仅当有链后非 SID 合入 | 链后 | 对应窗口 | 条件 |
| rollback/回滚相关 | 若有回滚,保留 exec-6-rollback 行与两次 publish 行 | 事件时 | agy-flow | 条件 |

INDEX 行类型(106 实证):fixedpoint-6targets、ape-receipt、post-chain-merge-provenance、exec-1-post-chain-merge、
exec-1-X-commit、exec-2-draft-readback、exec-3-ci-dispatch(可多次)、exec-3-ci、exec-3b-ci-dispatch-2、exec-3-ci-prior、
exec-4-Y1、exec-4-Y1-frozen、exec-5-publish(可多次)、exec-6-rollback、exec-6-public-readback、exec-7-Y2、exec-7-Y2-done、
post-publish、r40-bt-rerun-*、r40-w0150-*。五列:item、time、verdict、path_or_ref、note。

## 1 空壳头(只写头,不写结果)

ape-receipt.txt(拷贝后保持链原格式,首行示意;实际以链文件为准):

    # source=<chain>/ape-final/ape-receipt.txt; paths scrubbed to ~/; do not edit values

fixedpoint-6targets.tsv:

    # target<TAB>g2_sha256<TAB>g3_sha256<TAB>same<TAB>bytes<TAB>chain_rev
    # (六行由链 fixedpoint/g2,g3 逐对生成;compare 四同原行另附)

strict 文件头(与 106 同格式):

    # devsvc qualify: strict evidence bound to candidate APE bytes.
    batch	domain	status	rc	candidate	ms	load1	retries	note
    # source=<COMMIT_40HEX> layout=<LAYOUT> set=ids seed=<SEED> jobs=<N> window_s=55 outer_s=60 ape_sha256=<APE_SHA256>

INDEX.tsv 头:

    item	time	verdict	path_or_ref	note

warm 收据:

    WARM	<step>	READY	key=<KEY>

pack-run.out:保持 `release-run --pack` 原输出,不加工。

## 2 106 踩过的格式坑(107 必须避开)

- strict 文件名:106 用 `v0.0.106-strict-2026-11-24.tsv`(含日期),再由 release-flow 消费;107 必须只有一个当版本
  文件,日期按实际;字典序上不要让旧日期文件排在前面被误取。
- superseded 计数按**原始 FAIL 行**计(106 为 7:六批七行),不是按批;折叠规则不改原行。
- ape-receipt 与 fixedpoint 来源不同:前者是链 `ape-final/ape-receipt.txt`,后者由 g2/g3 逐对摘要加 compare 原行生成;
  不能互相替代,也不能用 fixedpoint-sample 的 release 全量模式(需 GEN2/all.lst)。
- prerun-supervise.log / windows.tsv 是从预跑目录克隆,不是改写;含 `window-lanes=` 与 `PRERUN_RUN` 行。
- 每次 CI run 保留一对 json/log;不用后一次覆盖前一次(106 的 45442318936 与 45446325698 并存)。
- deferred 文件只对无生产者批生成;本车若 #5/#6 入车转绿,则不再生成对应 deferred,但保留原行说明。

## 3 产出时点串行

链止 ->(ape-receipt、fixedpoint-6targets)-> pack ->(pack-run.out、cache-manager)-> prep/stage-a
->(ratchets/rollback/evidence-rows/expected-fail)-> 预跑 ->(strict、deferred、prerun 日志)-> X
->(backup-evidence)-> 草稿 ->(draft-readback、ci 对)-> Y1 -> 公开 ->(public-readback、release-post-state)-> Y2。

## 4 禁止

- 不预写任何结果行、不预填 sha、不把空壳当证据;INDEX 行由执行器写,人不代写。
- 不把 106 的数字抄进 107;不把 deferred 文件在转绿后继续当证据。
