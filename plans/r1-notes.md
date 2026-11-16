# R1 证据页:prune 原样判定前置(0.0.71 速度一期)

状态:结论与补丁已出,产品表未改。补丁 `plans/patches/r1-prune-early-fallback.patch`(一行)待集成者裁定一处合同差后应用。本页数字全部为 2026-11-16 本机实测(lnx/x86_64,已装 0.0.67,APE a99ea71de7edf8b2,load1 1.9 至 2.9,属 noisy,只看相对值);带"估计"的没有跑过。快车道结果只标 LOCAL,不是候选证据。

## 1 结论

(1) prune 的约 100 毫秒不在装载也不在输出拷贝,在整条 tape 的逐行解析加闭包:模型解压 1.4 至 2.8 毫秒;把回落提前到第 3 行或第 809 行时整相只要 19 至 23 毫秒(固定段),完整走到 CLOSEFACT.b126 要 83 至 109 毫秒。

(2) 最省的做法不是在 prune 之前加门,而是 prune 网自己在解析途中遇到 callr 或 callm 就转 FALLBACK(与它对 .hostcall、.librarycall、.hostaddr 的现有处理同一行、同一机制)。hello 的第一条 callr 在 11212 行中的第 809 行(exit 里)。改动是 `exec/prune/prune-result.tsv` 的一行,不动驱动,不加阶段,判定完全是模型步。

(3) 实测收益:prune 推理中位 101.4 -> 22.2 毫秒(省 79 毫秒),全部推理相合计中位 454.7 -> 376.8 毫秒(省 78 毫秒);hello 类产物逐字节相同。

(4) 代价是一处合同差,所以没有直接提交产品表:tape 里有 callr 或 callm 但它们全在不可达单元里时,现在的网会裁,改后整条原样输出。这与任务书"须裁样本输出字节与改前相同"在这一族上不符,也与经典参考实现 `archive/reference-c/src/tapeprune.c`(按可达性判,第 112、117 行)在这一族上出现差异。其余输入(含全部现有 27 个 prune-cases 用例的形态)行为不变。

## 2 分相实测

hello(无头文件声明 printf,流间 tape 188883 字节 11212 行 388 个标签,prune 输入输出逐字节相同):

| 测法 | prune 推理毫秒 |
|---|---|
| 已装产品 -o,两次 | 89.1、145.6(第二次机器忙) |
| stream prune 原 tape,三次 | 82.7、101.1、86.6 |
| 第 809 行插入 .hostaddr(在该处回落),两次 | 23.4、22.2 |
| 第 3 行插入 .hostaddr(开头即回落),两次 | 19.3、19.6 |
| 模型解压(model 行) | 1.4 至 2.8 |
| 对照:tapecheck 网扫同一 tape | 25.8 |

所以可省的是第 809 行之后的解析与其后的名字、单元、边、闭包各遍,约 65 至 80 毫秒;剩下约 20 毫秒是 INIT 加 FALLBACK 的整段输出,要再省须在驱动侧整相跳过(见第 6 节)。

## 3 补丁内容与原理

    -main	INSDONE.b27	33,37,34	FALLBACK	[]
    +main	INSDONE.b27	33,37,34,31,32	FALLBACK	[]

算子编号取自 `exec/facts/tape-shape.tsv`(非注释行序):callr 31、callm 32、.hostcall 33、.librarycall 34、.hostaddr 37;与 EDGEOP.b115 行"31,32 -> INDIRECT"一致。FALLBACK 的动作序列不变(OCLR、INPUSHXE、SPAN2、ACCEPT,原样输出)。改后 EDGEOP.b115 的 INDIRECT 分支与 CLOSEFACT.b126 的回落行不再可达,补丁保留它们不删,回滚只需撤这一行。

安全性:FALLBACK 是 prune 对任何不认识或保守情形的既有出口,输出即 e3 的原 tape,lower 一直接受(现在所有带 exit 的程序都走这条)。改后只会少裁,不会多裁。

## 4 快车道验证(已装产品加 cx-dev 私有包,只重建 shared-prune 一张网)

隔离方式:cx-dev 独立包 `/tmp/r1/cxdev`,比较树为已装版本干净树的副本 `/tmp/r1/wt` 加这一行,不含主检出里他人的未提交改动;重建摘要 nets=1,gen 1.0 秒、lint 1.0 秒(无新 ERROR)、tbl 1.1 秒。

| 样本 | 改前 prune 毫秒 | 改后 | 产物 |
|---|---|---|---|
| hello(声明 printf) | 88.3 | 22.3 | 相同 9442831accc2aa07 |
| hello2(stdio 加一个死函数) | 109.5 | 21.5 | 相同 84fc00d99b02cc2a |
| dead.c(须裁) | 1.7 | 1.9 | 相同,仍被裁 |
| ext.c(须裁) | 1.9 | 1.9 | 相同,仍被裁 |
| deadcallr.c(死函数经函数指针调用) | 1.8 | 1.6 | 不同:改前裁,改后原样;两者运行 rc 0,镜像同为 4096 字节 |

hello2 交错 5 次(A 已装,B 改后,每次新 TMPDIR,产物 5 次全部字节相同),推理合计/prune 毫秒:A 440.0/101.4、461.1/107.4、460.1/101.5、432.2/93.9、454.7/97.7;B 400.8/22.0、360.1/21.9、357.4/22.3、376.8/23.3、383.4/22.2。

八个真实工具源 -o 产物改前改后逐字节相同:tools/ratchets.cx、affected.cx、prune-identity.cx、refactor.cx、edit-span.cx、status.cx、derive.cx、tests/run.cx。

stream prune 直接对拍(输出、退出码、stderr 文案三项):原 hello tape、dead、ext、hello2 相同;畸形 tape 五种(截断在行中、乱码、未知算子、缺参数的 callr、空输入)输出、退出码与拒绝文案("reject: tape truncated"、"stream: prune: no input")全部相同;只有 deadcallr 的 tape 输出不同(改前裁,改后原样)。

普查(tools/prune-identity.cx,tests 与 examples 前 150 个路径中前段流成功的 52 个样本):含 callr 且原样 35,不含且原样 16,不含且被裁 1,含 callr 且被裁 0。即这批真实样本里没有落在合同差那一族的;该族可构造(deadcallr.c),属无头文件且带未用的间接调用函数的小程序。

## 5 未做与风险(应用补丁的人须补)

(1) 未跑:tools/run-startup-bench.cx 的 first 中位(它走已装产品,换不了私有包;按推理相差值估计改善约 78 毫秒,基线 796 毫秒);七个基线源 -o sha(清单出处未找到,以八个工具源代替);bootstrap-0.0.59 多单元编译;tools/affected.cx、tools/ratchets.cx、derive --check(主检出的表没改,跑了也不反映补丁)。
(2) 预计要动的钉:tests/literal.tsv 第 137 行 prune-result.tsv 的字面量计数 165 可能加 2;凡钉 prune 网或表摘要的材料(stream-prune-sha、stream-model-identity 的 e4:prune)在慢车道重算。
(3) 合同差需要落成用例:tools/prune-cases-gen.cx 的三方预言机要求经典实现与网相等,死单元含 callr 的用例会判 ORACLE_SPLIT;若接受合同差,要给该工具加一种"网比经典保守"的用例形态(断言经典裁、网原样),并在 tests/stream-prune-domain-matrix.tsv 加一格。现有 27 例都不在这一族(三个 indirect-call-preserved 用例的 callr 都可达)。
(4) 只在 lnx/x86_64 实跑;prune 网是各目标共用的 shared-prune,其余五目标未验证。O1 以上路线(e4 之后进 prune)未单独测。SID、PC、两代 GEN 未动。
(5) 若不接受合同差:精确做法是独立的判定(沿 v0.0.69 工作目录里未提交的 tools/prune-skip-gen.cx 的闭包算法),但它必须读完整条 tape(根 main、__main_ret 在 tape 末尾),按 tapecheck 的 25.8 毫秒估计,门网本身约 25 至 35 毫秒,净省约 45 至 55 毫秒(估计),且要新增阶段网、路由表行与驱动读表,全部属慢车道;收益小于本补丁,工作量大得多。

## 6 后续可做

(1) 回落路径剩下的约 20 毫秒(INIT 的算子名表加 188 千字节原样输出)可在网内再压,或由驱动在"prune 输出即输入"时免拷贝。
(2) 本方案不改 src,驱动没有净删行,腾不出 srcshape 的行数给 X1。

## 7 应用与回滚

    git apply plans/patches/r1-prune-early-fallback.patch
    sh ./unisacc.com tools/derive.cx -- --check
    sh ./unisacc.com tools/ratchets.cx
    sh ./unisacc.com tools/affected.cx -- --files exec/prune/prune-result.tsv --run
    sh tools/cx-dev /tmp/r1/t/hello2.c -o /tmp/r1/t/x      (UNISACC_RR_TIME=1 看 stage=prune)

回滚:`git apply -R` 同一补丁,或把该行改回 `33,37,34`。实验材料在 /tmp/r1(tape 样本、畸形 tape、隔离包),不入库。
