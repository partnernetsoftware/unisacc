# unisacc 0.0.78 发布说明

状态:同候选原生 CI run 44401593695(六目标 success,05:27 派发,05:31 通过)。整体裁决 NOT_QUALIFIED:全套 s9 本版未跑,产品资格域未整域实跑,严格证据只覆盖下列批次,不得称全批资格。

## 真实身份

| 对象 | 身份 |
|---|---|
| 候选点 T(整链输入) | f3359f46548d72f2c3a5a9e2bc3fdfbe55d2dd62(整链 499/499 rc 0,03:57 到 04:25,一次通过无重启) |
| 冻结提交(发布标签目标) | 8c4f101d1bbc7e96a72eb55acd83ae80dcb571f5(相对 T 产品输入零改动:src cx ext include exec weights seed kernel 无差,候选键相同) |
| 发布 APE SHA256(unisacc.com) | 18047439f4cabcc9b37c60d7150e3fd289f4bf7149376744ec19838ac75cb1e0(7387189 字节,比 0.0.77 的 7327477 多 59712 字节;--version 打印 unisacc 0.0.78;打包后与链产物同字节,PACK_APE SAME) |
| 源快照 SID | 898bd49a169456fd31ede0f90f25a9ebb7c9490a465eb8887c6aa7d93a1bea31(VERIFIED,T 的链快照) |
| pkg.bin | 72df86d11d1e015832ff228e040c30c21d988926b75a4da950b3d66c168092d2(与 0.0.77 同字节) |
| CI run | 44401593695 |

## 产品改动(cxmod 内存安全乙期二段,全部 opt-in)

产品输入三处:cx/cx0/plan.cx(权威)及投影 src/cxmod.c 与 cx/compiler/cxmod.cx,三源 EXACT。

(1) safe 单元静默漏判修复:词法失败具名报告(`unisa safe: too large or unbalanced for the flow check in unit`);内层同名遮蔽按保存栈恢复外层状态;变量 64、标签 32、循环与 switch 嵌套 64 超限具名拒绝,goto 走到稳定(最多 64 遍);无括号 sizeof 不算使用。
(2) E1 缺声明拒绝:safe 单元里非 static 函数 return 交出分配块而无 owns 或 unsafe 豁免时拒绝,文案 `function NAME returns an owned block; declare it with #pragma unisa owns NAME`。流不敏感;经局部变量返回单列 KNOWN-LIMIT。
(3) E1 后续修复:属性后缀 `__attribute__`/`__asm__` 不再误认函数名;输出参数 `*out = alloc` 不再误拒。
(4) 未声明 safe 的单元字节不变:17 份 -o 产物与 0.0.77 候选 17/17 sha256 相同;4 个未声明 safe 的真实文件 -c 目标文件相同。
(5) 验证:own-flow 产品侧夹具 75/75 failed=0,KNOWN-LIMIT 10(对 0.0.77 候选 59/75);CXMOD_TRI passed=38 failed=0;cx0-extract 两份 EXACT;棘轮 15/0;srcshape 5780/5854 未抬基线;原型 oracle oracle_passed=47。三轮独立复核均为可提交带条件无阻塞,条件登记为缺口 OWN-FLOW-* 与 OWN-E1-*。
(6) 兼容性:旧产品(0.0.77 及更早)不认识 safe/owns/unsafe 的本版语义,0.0.76 及更早不认识这三个 pragma(`unknown unisa pragma`);仓内 .cx 采用这些 pragma 之前,不得再用旧 Root 引导。

## 已知缺口(均已登记 build/pkg-gaps.tsv 的 OWN-FLOW-* 与 OWN-E1-*)

(1) break/continue 经遮蔽块出块时误拒。
(2) 宏体内的 return 与 K&R 风格定义漏判。
(3) E2 流检查 cxf_unit 带属性的函数认名仍是旧逻辑。
(4) owns 指向返回 int 的函数时无反向检查。
(5) 分配函数名在两处硬编码。
(6) E1 流不敏感,经局部变量返回的情形为 KNOWN-LIMIT。
(7) 未做 E3 与 moves,顺延下一版。
(8) 别名、字段、全局、函数指针仍漏判(沿用 0.0.77 的 KNOWN-LIMIT)。

## 开发工具链(不进产品)

(1) register --tool 行级回滚。
(2) kit 预热。
(3) Q-10 准入门 load-wait-timeout。
(4) G1 未测项门:最终 UNKNOWN 必须在 release/evidence/V/expected-unmeasured.tsv 逐批登记,reason 只许 no-producer/load-wait-timeout/outer-timeout,no-producer 须引用同批真实 DEFERRED 行证据。
(5) Q-01 原生 run-batch 默认关闭。

## 严格证据

(1) 冻结前预跑(origin=prefreeze-strict,T2=f3359f4654 布局,候选 APE 18047439,dvqualify --jobs 3):190 批首轮 PASS 186、FAIL 1、UNKNOWN 3、INVALID 0。证据 release/evidence/0.0.78/strict-prefreeze.tsv。
  - FAIL unit-frontcache-equiv3:rc=0/0/0 deps=DIFF(两次缓存运行后 run 缓存目录项数不同,产物 bin 与 rc 一致),发生在 load1 约 6、jobs 3 并发下,见 ufe3-fail.log。同批在 load1 低于 2 的单批重跑 2 次 PASS,重跑段已追加,FAIL 行与重跑行都保留,末态 PASS。原因判为负载下 run 缓存并发写差异(同族缺口 UNIT-CACHE-PARALLEL-FIRST-COMPILE-LOSES-STORES),低载未复现,未定因,不标预期。
  - UNKNOWN cx0-e2、cx-booter-ape、tools-default-regression:affected.cx 报 no-producer,材料无生产者,确定性,记 UNMEASURED,不记 PASS。
  - 首轮另有一次作废:外层循环终止后其 dvqualify 仍在写,另起的 --resume 并发写同一 TSV 致截断与丢行,整份丢弃(缺口 DVQUALIFY-RESUME-NO-WRITER-LOCK)。
(2) 复用与补差:T2 -> F 的非白名单差只有 build/pkg-gaps.tsv。affected.cx --files build/pkg-gaps.tsv 选出 380 批,与预跑 190 批交集 164 批。
(3) 冻结后补差(origin=postfreeze,冻结提交 F 的布局,同一 APE,--jobs 2):164 批全部重跑,终态 163 PASS、0 FAIL、1 UNKNOWN、0 INVALID。首轮(05:12-05:24,墙钟 679 秒)PASS 161,module-units 与 notcovered 因同时进行的 pack 与 N4 预热把 load1 抬到约 6 而 outer-timeout-60s/load-wait-timeout,单个 --resume 进程续跑后均 PASS;tools-default-regression 仍 UNKNOWN(rc 3 无 BATCH_RUN,与预跑同为材料无生产者),记 UNMEASURED。超时行与续跑行都保留。证据 release/evidence/v0.0.78-strict-2026-11-19.tsv。unit-frontcache-equiv3 不在补差集内,其预跑 FAIL 与重跑 PASS 只在 strict-prefreeze.tsv。整合严格文件按复用段原样拼入预跑全部段再接补差段,唯一删减是 equiv3 首轮 FAIL 行及其 RAW 两行(G1 对任何 FAIL 行要求 expected-fail 登记,而该失败原因未复现,登记为预期不实),删减在文件头注释写明,原行保留在 strict-prefreeze.tsv。
(4) 复用 26 批:预跑中不受 T2 -> F 差影响的批次,结果与 RAW 保留在 strict-prefreeze.tsv(其中 cx0-e2、cx-booter-ape 为 UNKNOWN,记 UNMEASURED)。
(5) 合计本版严格证据覆盖 190 批:PASS 187、FAIL 0(终态;预跑 equiv3 的 1 行 FAIL 保留在案,低载重跑末态 PASS)、UNKNOWN 3(UNMEASURED)。棘轮:冻结提交 git archive 上 15/15 ok。release-prep 末行 verdict=NOT_QUALIFIED(rc 1,uncovered 为无 RAW 的超时行与 no-producer 行,与 0.0.77 同性质)。
(6) G1 未测项门首次实战(0.0.77 公开后入库):最终 UNKNOWN 3 批逐批登记在 release/evidence/0.0.78/expected-unmeasured.tsv,reason 均为 no-producer,各引用同批真实 DEFERRED 行(cx0-e2-deferred.tsv、cx-booter-ape-deferred.tsv、tdr-deferred.tsv)。expected-fail 本版为空。
(7) N4 读回预热 WARM READY(5 个目标 HIT)。release-exec --dry 末行 DRY_GATES_OK。

## 整链

499/499 rc 0,03:57 到 04:25,一次通过无重启。链目录 /tmp/unisacc-session-v078-chain/f3359f46548d72f2c3a5a9e2bc3fdfbe55d2dd62。

## 未解决项(沿用)

(1) s9 全套本版未跑;产品资格域未整域实跑;严格证据所列批次之外为 UNMEASURED。
(2) -funit 对象与系统 ld/gcc 互操作的既有缺陷:NN-VERIFY-ELF-INIT、NN-VERIFY-WEAK、NN-VERIFY-FUNIT-SYSV。
(3) unit-frontcache-equiv0 deps=DIFF(UNIT-FRONTCACHE-EQUIV0-CX-DEPS-DIFF);本版 equiv3 同类现象见上。
(4) release-prep 棘轮批冷缓存接近 55 秒限(Q-09)。
(5) -S -funit 返回 NOT_IMPLEMENTED(0.0.71 起如此,不是回归)。
(6) 其余开放项见 build/pkg-gaps.tsv。

## 证据入口

release/evidence/0.0.78/INDEX.tsv;release/evidence/0.0.78/strict-prefreeze.tsv;release/evidence/v0.0.78-strict-2026-11-19.tsv;release/evidence/0.0.78/expected-unmeasured.tsv;release/evidence/0.0.78/expected-fail.tsv;release/evidence/0.0.78/ratchets.txt;release/evidence/0.0.78/pack-run.out;release/evidence/0.0.78/cache-manager.tsv;release/evidence/0.0.78/warm-*.tsv。撤回方案:release/rollback.txt(回退 Latest 到 v0.0.77,不删标签与 release)。
