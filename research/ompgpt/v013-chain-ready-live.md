# v0.1.3 起链门实测

固定输入：`8b3727031b70481eed09f8bc60f5217c96d8ad69`。基线取提交归档，私有 Git 索引保留原提交身份与祖先；不以无祖先的新根提交模拟门 22、23。已装根两件用常规文件副本。`env -i` 只恢复 `HOME`、`PATH`、`TMPDIR=/tmp`、`DEVSVC_OWNER=cc` 与 `UNISACC_IDENT_HELPER=~/.cache/unisacc/prep-cache/identity-helper-b9ed85e8f3890fa8.bin`（执行时展开绝对路径）。不改共享检出、不启动整链。

## 编号隔离试验

私有 `C0a=538b843e49`：第 68 槽、计划、候选表列 2/4/6/7 同步为 `v0.1.3/0.1.3/v0.1.3/0`。`derive --write --only table-index` 返回 `WROTE`，但没有相对基线的字节变化，未提交空变更。私有 `C0b=445faf3843`：仅提交冻结表唯一活跃行，基点为上述 C0a，所有者 `cc`，截止 `1795676512`。私有远端的 `main` 固定为 `c500c0db8`。

```text
11-version-slot PASS VERSION_SLOT OK version=0.1.3 target=0.1.3 source=arg expect_self_report=unisacc 0.1.3 6487
21-verseq-gen PASS VERSEQ_SUMMARY checks=100 fail=0 120
23-freeze-armed PASS armed: freeze.tsv sid=0.1.3-FREEZE owner=cc deadline=1795676512; HEAD behind origin/main=0; numbering files unchanged after the freeze 117
```

三门均为单门实跑，不等于全门放行。

## 冻结前基线实测

基线全门外限 285 秒，退出 124，无总 `READY` 行；保留 `/tmp/v013-ready.0mz5sskx/baseline.log` 与 `prepare.log`。`DERIVE_PREPARE prepared=2 no-test-result`，只证明材料就绪。不能把未落行的门算通过。

| 门 | 原行要点 | 判定与最小处置 |
| --- | --- | --- |
| 2a | `batch-packages-gen TIMEOUT 30148 rc=142`；`DQ batch-domains fail why=generator rc=2`；`PRODUCT_MAP_QUERY REFUSE readset-changed-before-output` | 超时与拒绝分开。agy-flow 串行单跑三生成器。产品映射拒绝指出缓存读取集变化，不能当成派生漂移；静默复测仍红再追变化路径。 |
| 2b | `RATCHETS ok=18 fail=1`；定点 `dup-census: ck has 59 verbatim copies, baseline 58` | 真红。与主线对比，新增匹配文件为 `tools/check-proc-own-sat.cx`；`app` 修复已在祖先。agyneon 复用 `cxlib.ckassert`，保持断言与输出，不动基线。 |
| 5 | `rc=142 pass=106 fail=0 failed_shard=core-c(rc=142,timeout,pass=12,fail=0)` | 超时，非断言红；静默单门取证。 |
| 11 | `REFUSE CHAIN_READY_VERSION_SLOT slot-version-differs-from-target 0.1.2 0.1.3` | agyneon 用 C0a 修正，隔离实验证实可绿。 |
| 15 | `GEN_ALL_SUMMARY total=46 pass=45 diff=1 error=0 uncovered=2`；`LANE_INPUTS_GEN DIFF build/lane-inputs.tsv out of date (run --write)` | 真红。agyneon 重生 `build/lane-inputs.tsv`；另查断言器仍为 `CGAC_GENERATORS 45`，与实跑 46 不符，同步切片和断言后复验。 |
| 22 | `derive-b ... derive_refused=2`、`derive-c ... derive_refused=1` | 与 2a 同样出现 `batch-domains` 拒绝和 `product-map` 读取集变化；agy-flow 串行分片取原行，不先改产品。 |
| 23 | `FREEZE_NOT_ARMED no unexpired build/freeze.tsv row` | agyneon 用 C0b 修正，隔离实验证实可绿。 |

基线已落 PASS：1、3a 两片、3b、6（没有传 `--since`）、8、10、12、12p、13、14（旧根记录告警）、18、20、21（基线检查 97 条，不代表第 68 槽已登记）。

分段补测：`16-disk-free PASS avail_gb=59`，`17-root-public PASS Root is not a candidate install`。7、9 均按设计报 `UNMEASURED`，实际起链仍须完成手工核对。门 19 单跑 `FAIL READY=no version-plan-missing version=0.1.3 path=./plans/v0.1.3.md 60063`；这是基线第三处具名阻断，由 agyneon 的 C0a 补计划。此原行不证明 `chain-to-snap` 已通过；编号并同步派生后由 agy-flow 复验门 19。

`4-p1 PASS 2774 files planned by cx0_plan, rejected(any text)=34, new-text rejections=0, negative fixtures still refused=8, child failures=0 46629`。

`5-candidate-chain` 串行复验通过：`rc=0 pass=139 fail=0 (baseline=137 shards=5: core-a=44,core-b=27,core-c=45,objects=16,watch=7) 52741`。全门首次的 core-c 超时不列作确定缺陷。

## 串行复验与第四处确定性缺陷

`DERIVE batch-packages-gen OK 26874`，`DERIVE product-map OK 5098`，`PREMERGE_UNION_SUMMARY verdict=PASS shard=derive-c ... derive_refused=0 ... ms=6819`。首次并跑的对应两项不再阻断。

`DERIVE batch-domains REFUSED 4210 rc=4 DQ batch-domains fail why=generator rc=2`；`PREMERGE_UNION_SUMMARY verdict=FAIL shard=derive-b conflict=0 gen_fail=0 derive_drift=0 derive_refused=1 check_derive_fail=0 ms=16276`。直接运行生产者拿到 `BATCH_DOMAINS REFUSE domain-missing-declaration id=check-nn-external; add an explicit batch-domain exception if the contract is insufficient`，退出 2。归 `check-nn-external` 登记作者与 cc 共享表写手：按批次合同补明确域声明，再验生产者与门 2a、22；不以调度规避。

## 更新头 `404142b275d9572b443a69fbcd91b7c0b60abc31`

全门实际退出 1，原行保存在 `/tmp/v013-ready-new.RPTdUBxQ/baseline.log`（全部门，无截断），原生预备在同目录 `prepare.log`：

```text
2b-ratchets PASS RATCHETS ok=19 fail=0 elapsed_ms=8436 (roster=19) 8641
15-gen-all PASS GEN_ALL_SUMMARY total=46 pass=46 diff=0 error=0 uncovered=2 ms=6323 6490
5-candidate-chain PASS rc=0 pass=139 fail=0 (baseline=137 shards=5: core-a=44,core-b=27,core-c=45,objects=16,watch=7) 53000
READY no failed=2a-derive,11-version-slot,22-premerge-union,23-freeze-armed,19-chain-preflight 214629
```

没有新增红门。11、19、23 仍是这棵尚未编号冻结的树；2a、22 均只剩 `batch-domains` 拒绝，不与旧版已消退的查询争用混记。

## 距离起链还差什么：`05efce746`

干净环境下先准备原生材料，再逐门实测，原行见 `/tmp/v013-domain.g84nbfmG/gates.log`：

```text
2a-derive PASS shards=6/6 drift=0 refused=0 (DERIVE_SUMMARY ok=4 drift=0 refused=0 skipped=12 elapsed_ms=18479) 18929
22-premerge-union PASS shards=5/5 (PREMERGE_UNION_SUMMARY verdict=PASS shard=prep conflict=0 gen_fail=0 derive_drift=0 derive_refused=0 check_derive_fail=0 ms=13218) 22837
```

(1) agyneon 落 C0a：候选四列、第 68 槽、主题与计划三机械字段。未编号头仍有 `11-version-slot FAIL ... slot-version-differs-from-target 0.1.2 0.1.3` 与 `19-chain-preflight FAIL ... version-plan-missing`。先前私有 C0a/C0b 的 11、21、23 通过原行见上；不冒充新版正式头结果。

(2) agyneon 单笔落 C0b，cc 持冻结写窗：唯一有效冻结行，基点指 C0a，确认当前远端主线为祖先。未冻结头的门 23 原行为 `FREEZE_NOT_ARMED no unexpired build/freeze.tsv row`。

(3) agy-flow 用最终头重跑门 19，必须取得 `chain-to-snap` 与严格预检实测，不沿用缺计划时的裁决；再取同一头全门 `READY yes`，完成 7、9 的手工核对才起链。已测门 19 耗 49.4 秒；前一头完整门实测 214.6 秒，供排窗，不承诺后续耗时。

其余门的逐条通过原行在 `404142b27` 的完整 `baseline.log`，未声称重跑在 `05efce746`。其中 2b 为 19/0、15 为 46/46，门 4 为 `new-text rejections=0`；门 5 为 139/0。7、9 按设计 `UNMEASURED`。当前尚不放行。

补证：`404142b27` 门 5 自己的作业串行两轮均 `PASS rc=0 pass=139 fail=0`，52,806 / 52,824 ms；启动 load1 为 18.00 / 15.30，并非机器独占静默，不作低负载余量证据。原行见同目录 `gate5-1.log`、`gate5-2.log`。编号两笔准备耗时估计约 5 分钟，不计等待共享写窗；最终全门按已测约 4 分钟预留。
