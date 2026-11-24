# W0191 取证:fwdstub-replay 批与转发桩隔离提交的对照史(只读,2026-11-24 ccogweco)

## 结论(先)

1. 该批在 0.0.105 时**不存在**:`release/evidence/0.0.105/strict.tsv` 里 `fwdstub-replay` 出现 0 次,
   没有状态也没有耗时可抄。
2. 它是 661de08935("隔离转发桩并验证显式分相收据")在本次区间(414a024ea5..2e8a19a342)内新建的:
   同一提交同时加了 tests/check-fwdstub-replay.cx(270 行)、tests/manifests/fwdstub-replay.tsv、
   tests/batches.tsv 一行、tests/coverage.tsv 一行、batch-domains 与 batch-product-inputs 各一行。
   也就是说,批的输入与期望**在隔离改动里一次性写成**,没有"先有批、后改产品"的时间差。
3. 区间内该批的两个登记文件只有 661de08935 碰过
   (`git log 414a024ea5..2e8a19a342 -- tests/check-fwdstub-replay.cx tests/manifests/fwdstub-replay.tsv` 只此一笔);
   之后唯一再动到它 coverage 源列文件的是 b150aa37e2(给 include/unisacc_ffi.h 加 `#ifndef _WIN32` 包层),
   该提交**没有**同步改批或改期望。
4. 因此 W0191 的红不是"批跟着旧期望跑在隔离后的树上":批的期望就是隔离后写的;要查的是 b150 的头包装
   与运行环境(今晚的红是 `B refuses a missing forwarding source and receipt` 一条断言,伴随 4 次
   OSError Errno 8,路径指向 qualify 树的 `unisacc.com`)。

## 详情

### 1 批与登记(2e8a19a342 树上的原行)

    fwdstub-replay	IMPLEMENTED	product	tests/check-fwdstub-replay.cx	SOURCE	lnx/x86_64	55	-	tests/check-fwdstub-replay.cx	{root};{candidate};{out}	product.fwdstub-replay	rc0:CHECK_FWDSTUB_REPLAY	product	commit

coverage 源列(六个文件,全部是隔离改动的对象):

    tests/check-fwdstub-replay.cx;src/drive.cx;src/driveroute.cx;cx/compiler/fwdstub.cx;include/unisacc_ffi.h;include/sys/_ffi_forward.h

### 2 区间内这些文件各自的提交

    git log --oneline 414a024ea5..2e8a19a342 -- <file>

- cx/compiler/fwdstub.cx:只有 661de08935
- cx/driver/fwdreplay.cx:只有 661de08935(新文件,134 行)
- include/sys/_ffi_forward.h:只有 661de08935(新文件,279 行)
- include/unisacc_ffi.h:661de08935(-275/+4 拆分到 _ffi_forward.h)与 b150aa37e2(+2,`#ifndef _WIN32` 包层)
- src/drive.cx:只有 661de08935(-23/+…,改显式分相)
- src/driveroute.cx:只有 661de08935
- cx/compiler/fwdmark.cx、cx/compiler/routepipe.cx:区间内**零提交**
- tests/batches.tsv 的 fwdstub-replay 行、tests/coverage.tsv 行、manifest:只有 661de08935

### 3 夹具与材料

- 该批不读仓内夹具文件:check-fwdstub-replay.cx 在运行期把三段 C 源(main-0.c、main-1.c、extra.c)
  与 phase 文件写进 `{out}` 目录(源码 :200-220 的 write_text 段),再用 `{candidate}` 编、跑。
  所以"夹具是否在隔离改动里被改过"的答案是:夹具随批一起新建,没有旧夹具需要同步。
- 材料:manifest 的 materials 列为 `-`(无 NAME-tool 材料),批不在材料的宏扫描影响面内。

### 4 期望是否随改动更新

- 首次也是唯一一次:"B refuses a missing forwarding source and receipt"、"B refuses a changed forwarding source"、
  "A publishes source and receipt beside checkpoint"等断言与隔离后的四相行为同提交写成(源码 :78-110、:240-260)。
- b150aa37e2 只加 `#ifndef _WIN32` 包层,没有动批:它把 `uffi_set_bridge_mode`/`uffi_last_error` 的定义
  限定到非 Windows,预期对 lnx/x86_64 上的行为无影响;但这是"改了 coverage 源而批未动"的唯一一处,
  若 W0191 的红与桥模式定义缺失有关,应先核这条。

### 5 今晚的实测(0.0.106 预跑,W0191)

- 窗口:attempt=1 wall 11843 ms;attempt=2 wall 4623 ms;批次 FAIL rc=2,attempt2 批次 ms=2609。
- 原行:`PRERUN_PRODUCT_RED_STOP batch=fwdstub-replay domain=product-qualification status=FAIL rc=2 retries=2 window=W0191`
  与 `RAW BATCH_RUN id=fwdstub-replay status=FAIL rc=2 marker=CHECK_FWDSTUB_REPLAY assertions=3`。
- 断言明细:`FAIL B refuses a missing forwarding source and receipt`;stderr 四次
  `phase A transport=-1 rc=0 residue=0 OSError: [Errno 8] Unknown error: '<qualify tree>/unisacc.com'`
  与三次 phase B 同形,说明失败发生在执行候选产品这一步(Exec 失败),不是断言删改。
- 0.0.105 strict:无该批(0 次命中),不能拿 0.0.105 的绿作对照;对照只能是隔离提交建批时的私有记录。

### 6 范围提醒

- 本页只覆盖 414a024ea5..2e8a19a342(第一车链)。0.0.107 分支上的 ac483247c7(F1 容量检查)等提交
  同样改 cx/compiler/fwdstub.cx,但不在本区间;若在 107 候选上重跑该批,必须把那些提交一并纳入对照。
