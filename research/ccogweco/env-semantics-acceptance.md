# 第六笔落车后三消费者验收表(tests/env-semantics.tsv 追加四行;2026-11-25 ccogweco)

前提:第六笔只追加 UNISACC_PHASE、UNISACC_PHASE_OUT、UNISACC_PHASE_IN、UNISA_FWD_REPLAY 四行;
候选 APE 仍为 5f83ac93…、SID 不变。三个消费者批都要在同一候选 APE 与新车头 frozen-root 上正式 run-batch 复跑。
任一批红 -> 立即转路径 C,不登记预期失败、不改断言。

## 0 公共复跑命令(同一候选 APE)

    cd <新车头 frozen-root>            # frozen-source.rev 指新车头
    APE_DIR=<ABS 目录>                 # 目录内 unisacc.com 为 5f83ac93… 的候选 APE
    sh <repo>/unisacc.com <repo>/tools/affected.cx -- \
        --ids env-semantics,envcompat,premerge-selfcheck --tier commit --run \
        --candidate "$APE_DIR" -j 1

- `--candidate` 为绝对目录(affected.cx:100 的 candidate_dir;:1076-1078 把 {candidate} 映射为
  `<dir>/unisacc.com`),候选身份按目录内 APE 字节核。
- 输出认 `BATCH_RUN id=<id> status=PASS rc=0` 原行;没有 BATCH_RUN 行不算跑过。
- 三个 id 在 tests/batches.tsv 的行(已核存在):
  env-semantics、envcompat、premerge-selfcheck(最后一列 tier 均为 commit)。

## 1 check-env-semantics(批 id:env-semantics)

- 断言分母:现 125/4(共 129)。补四行后每变量正反各记一条,共 +8,应为 **133/0**;
  不是 129/0,也不允许只加正面一条。
- 期望原行(tools/check-env-semantics.cx:331 打印):
      CHECK_ENV_SEMANTICS	assertions=133	failed=0
  同时不得出现以下具名失败(源码行):
  - `FAIL	unregistered	<name>	<file>`(:139)——四变量任一读取点与表对不上;
  - `FAIL	stale	<name>	<file>`(:143)——表里有行但产品不再读;
  - `FAIL	expired	<name> deprecated until ...`(:127)——UNISA_FWD_REPLAY 的 expires 被误改;
  - `FAIL	table	columns=... want 7 or 8`(:93)——八列格式被破。
- 失败即转 C 的判据:断言数不是 133、failed 非 0、或出现上述任一行。

## 2 check-envcompat(批 id:envcompat)

- 核对点:
  - 八列:每一条弃用名单行必须 `table_columns(tbl, legacy[i]) == 8`(tools/check-envcompat.cx:95),
    名单来自 `ec_legacy_name` 的 7 个名字(:88),四行新变量不得破坏列数;
  - 弃用名单:仅 UNISA_NOSTDINC 允许无孪生行(:102-110 的 only-UNISA_NOSTDINC 断言);
  - 双拼写写读(:111-130)不受影响,但整批必须全绿。
- 期望原行(:163 打印):
      CHECK_ENVCOMPAT	assertions=<N>	failed=0
  N 以本次实跑为准(不在本页写死),但 failed 必须为 0;每条 PASS 行形如 `PASS	envcompat.<name>`(:22)。
- 失败即转 C 的判据:failed 非 0,或任一 legacy 八列断言红。

## 3 check-premerge-selfcheck(批 id:premerge-selfcheck)

- item 14 的语义源自源码字面量(tools/check-premerge-selfcheck.cx:335-341):
  - 新读取无行:`item=14 name=env-unregistered verdict=FAIL`(:335);
  - 有行且列了读取文件:`item=14 name=env-semantics verdict=PASS`(:336);
  - 只在注释里出现:`verdict=NA`(:337);
  - 表行未列读取文件:`env-unregistered` FAIL 且带 `file:line`(:338)。
- 对本补丁加 661de08935 的产品 diff(四个读取点在 cx/driver/fwdreplay.cx):
  期望是 **PASS,不是 NA**(四行都登记且 readers 列写 fwdreplay.cx);item14 是否 NA 的最终结论由 cdx 判,
  但 cdx 判 PASS 的依据必须是这条原行,不得口头豁免。
- 期望末行(:394 打印):
      CX_PREMERGE_SELFCHECK	checks=<N>	failed=0
  N 以实跑为准(分片时用 `CX_PREMERGE_SELFCHECK_PART part=main|rd checks=.. failed=0`,:393)。
- 失败即转 C 的判据:failed 非 0,或 item14 出现 FAIL/NA(NA 与"没有登记"等价,不接受)。

## 4 读者穷举(新读者出现即转 C)

    git grep -n 'env-semantics.tsv'          # 在新车头上跑,路径不限(含注释与夹具)
    git grep -n 'env-semantics'  -- '*.cx' '*.tsv' '*.md' | head -50

已知读者(逐条核):
- tools/check-env-semantics.cx:83 —— 主消费者,读 tests/env-semantics.tsv;
- tools/check-envcompat.cx:77 —— 八列与弃用名单;
- tools/check-premerge-selfcheck.cx 的夹具行(当前 main 为 :174-180;cc 记的 :947 是 107 头行号,落车前以实树为准);
- 登记覆盖:tests/batches.tsv 的 env-semantics 行(entry/args/expect 列)与 tests/coverage.tsv 的
  interfaces.env-semantics 行;tests/batches.tsv 中 premerge-selfcheck 行的 args 为空(不引用该表)。
- 非消费者:cx/runtime/modelcache.cx:219 只是注释提到该表,不读。

处理:
- 若 grep 出现上述以外的读者:先读它怎么用表(列数、列序、字符集),再决定是否在新补丁里同步;
  只要它可能因四行新增而改变结论,就按第 2 条"任一消费者红"处理,直接转路径 C。
- 若读者只读注释或文档:记录原行,零回归即可,不转 C。

## 5 三批全绿后的落库与披露

- 三批 BATCH_RUN 原行 + 第 4 节 grep 原行写入补跑证据;strict 表按 SUPERSEDED 折叠(原 FAIL 行保留)。
- 发布说明写"补登记 tests/env-semantics.tsv 四行;候选 APE 与 SID 未变;W0206 与三个消费者批在候选 APE 上复跑通过"。
- 任一步失败:转路径 C(新车头加 exec/facts 补登记、整链整预跑),不保留本 B-t 扩展的结论。
