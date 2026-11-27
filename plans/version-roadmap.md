# unisacc 版本边界路线(fable 裁定,2026-11-27,cc 采纳)

关键判断:M1 与迁移批 1 是两类不同的大改,必须是两个版本;tape 速度门通过之前,任何 VM 字节码单元不得进入默认路径。

依据:m53 属“迁移 VM 字节码”(tape carrier);批 1 的 6 个单元属“迁移表”(验收=golden 全枚举、逐字段 cmp)。旧 tape 证据:对原生 94x 与 114x,15x 门未过,102 档未完成(`prd_mempalace.md:410`)。M1 的 G1..G7 全 `UNCOVERED`,尚无编译产物。

| 版本 | 主题 | 定义能力 | 退出判据(真实候选上) | 不在范围 | 规模(估计) |
| --- | --- | --- | --- | --- | --- |
| v0.1.6 | tape 载体公开分发 | 公开资产包含旁置 `tapemode.so`,用户可选择启用影子对拍 | 9 件公开资产;六目标 CI 成功;`facts-gen --check` 无 DIFF | 任何权威翻转 | 今天 |
| v0.1.7 | M1 权威嵌入(未发布,内容移入新 v0.1.8) | 同旧 v0.1.7 | 作废:尺寸判据漏算 loader 固定成本,按铁律“放宽即换版本号”不发布;测量入口 `check-v017-exit 9e2ac4df6` 对 `c9275e775` | 同旧 | 不发布 |
| v0.1.8 | M1 权威嵌入(仅 lnx/x86_64) | 不依赖旁置文件,stream 的 `cxsa_valid` 由嵌入 tape 权威执行 | 三条尺寸判据:原生单元移出 `>= 3,770`B;native `.text` 净增 `<= 26,000`B;PT_LOAD R E 净增 `<= 25,000`B(只可收紧,不要求净减);其余判据同旧 v0.1.7(`cxsa_valid` 0/`cxsa_valid_s1` 已定义、其余五切片 0B 差、stream `<=2x`、G1..G6 先红后绿、G7 `UNCOVERED`);发布说明须写二进制增大(`+25,267`B `.text`、`+24,527`B PT_LOAD R E)、回本未证、v0.1.7 因判据错误未发布、仅 lnx/x86_64 | 批 1;其他五目标;默认路径 | 3-5 天;注:尺寸变化不归因 tape,driveargs -13,641B 归因未证,tapegate loader 可能自带 stdio 副本 |
| v0.1.9 | 纯静态 libc 副本去除类 | 20 个上层产品单元去掉各自静态 libc 副本,printf 家族走 L0 `sy_*`/`mm_*`/`tx_*` | PT_LOAD R E 净减 `>=40,000`B;LOCAL 20 单元实测 `-441,128`B(agyneon2 `a95f0fc86..bc2177979`);字节棘轮 `check-stdio-copies` 锁 `_u_vfmt` 等为零;本版不含 tape 迁移与大小归因 | tape 迁移;VM 单元;六目标 | 5-7 天 |
| v0.1.10 | tape 运行时范围 + 速度门(收窄) | tape 运行时(bridge、call_out、touched-span clear、guest-slim、A2、pack;w/agyneon3 13 提交加 self-audit 修复)与全部 102 单元在 tape 下运行 | 102 档 cmp EQUAL;16 档 `<=15x`,判据按身份指明**单一** fixture(2x2 测量进行中;当前 speed-gate 集 `14.58x`,check-v0110-exit 内置 fixture `15.65x-17.18x`);64/102 档 VM 单元按预设 fallback 判 stay-native(`14.58%<25%`);原生基线按候选身份冻结 | 迁移任何新单元;64/102 档 15x | 7-10 天,最不确定 |
| v0.1.11 | 下一静态重复类 | 把重复静态实现收成一份: SHA-256 收为单个 L0 副本(`sy_sha256_bytes`,约 `55KB`),`cx_read`/`cx_write` 的 heap 与 `FILE*`(约 `45KB`),deps/driveplan,heap 批;RT_EXTERN 函数共享作为判后 side item(活代码仅 `63.7KB`) | 进入时按边界规则写判据并登记红检查;只可收紧;不承接 64/102 档 15x | 无独立类时不成版 | 未估计 |
| v0.1.12 | 先拆后迁批 2 | 批 2 的 16 个单元进入迁移 | 进入时按 v0.1.11 测量重新设定 | 批 3、批 4 | 未估计 |
| v0.1.13 | 剩余批并冻结 stay-native | 剩余迁移批 | 进入时按测量设定;冻结 `plans/stay-native.tsv` | 六目标 | 未估计 |
| v0.1.14 | 六目标一致 + 出口门 | 7 个已迁单元在六目标权威 | 每目标 `cxsa_valid` 0、移出字节相等、fork 根回 1;`check-v01x-exit` 全绿 | v0.2.x | 7-14 天 |
| v0.2.0 | nn 体积与步率(暂定先做) | 权重与 tape 步率改进 | 权重 `-15%`;tape 倍率 `15x -> 10x`;逐项评估相等;保留阈值 `max(3%, 2x 噪声)` | asm 热类;六目标 | 7-14 天 |
| v0.2.1 | asm 热类 1(lnx/x86_64) | 前 5 个最热 stay-native 函数的 asm 重写 | 基准指令中位 `-10%`;`.text` 不增;其他目标保持参考实现;保留阈值按噪声带 | arm64;第二批热点 | 7-14 天 |
| v0.2.2 | arm64 asm + 第二批热点 | 第二架构与第二批 asm | 每架构 `-8%`,其他地方零回退 | 六目标统一 | 7-14 天 |

仪式:五个版本都会改公开二进制;每个版本一次完整预跑加六目标 CI,且只有在私有候选上退出判据已全绿后才冻结。72 小时的 M1 目标是私有 APE 证明,不是 v0.1.7、也不是新 v0.1.8 的切版。

cc 执行的边界规则:

(1) 准入:一版一类,按 `plans/slice-units.tsv` 的目的地列(表 / VM / 先拆后迁 / 留原生)加目标集判定。跨类、跨目标或不服务本版退出判据的产品改动进下一版;工具与检查进 main,不占版本。

(2) 切点:只有当退出判据的每个数字都在同一候选身份上实测为绿才冻结;不做按日期降级的发布(0.1.5 以 `NOT_QUALIFIED` 发布,不重复)。到估计两倍仍不绿:缩范围并重写判据,绝不带缺口发布。

(3) 判据先行:开版时把退出判据写成可执行检查并登记为批次,先红。没有红检查就没有版本。判据中途只能收紧;放宽即换新版本号。

最不确定:先 v0.1.11 还是 v0.2.0(若 15x 门两周内破不了就对调;便宜的证伪:在 v0.1.8 候选日重测 16/64 档逐步倍率,高于 40x 即对调);批 1 是否真的绕开 tape(证伪:先只迁 m25 测热启动差);“6 单元 48,602B”与生成器把 route.o 放批 2 需在开 v0.1.9 前对齐。

## 追加范围决定(v0.1.9 单元清单,cc 在 agyneon2 对账后,2026-11-27)

v0.1.9 = 全部 6 个 `FINITE`、`dest==1` 的表迁移单元,合计 `48,602`B:`route.o` 12,979、`m25.o` 9,405、`m20.o` 8,435、`modelnumber.o` 7,106、`driveargs.o` 6,701、`m54.o` 3,976。默认路径先做:route、m25、modelnumber、driveargs,合计 `36,191`B。依据:版本边界是改动类别(`plans/slice-units.tsv` 的目的地列),不是 `tools/slice-migration-gen.cx:110` 的生成器规模启发式 `batch=(text<=10000 && no sys use)`——该启发式把 `route.o` 放进了批 2,与本决定冲突时以目的地列为准。

## v0.1.x 收尾与 v0.2.x 循环(fable 采纳,2026-11-27,所有者方向:v0.2.x = 持续自优化,asm 切片做到极限加 nn 迭代)

- **v0.1.x 出口门**(一个检查 `check-v01x-exit`,开版即红):`plans/slice-units.tsv` 每个单元要么“留原生”要么“已迁且权威”;六目标各自 `cxsa_valid 0`、移出字节相等、fork 根回到 1;tape 16 档 `<=15x`,64/102 档 15x 项由预先定义的 stay-native 判定取代——证据:`agyneon` 真实 tier 64 `416.2M` 步、load1 2.43,全部 `[r6-OFF]` frame-slot ld/st `14.58%`(r6_64 `6.95%` + r6_n `7.62%`),加 `[r7+OFF]` 后 `24.3% < 25%`,故判 stay-native;这是应用预设 fallback,不是放宽;热启动 `<=1.10x` 新 v0.1.8。
- **v0.1.x 尾段**:v0.1.6 载体;v0.1.7 作废(内容移入 v0.1.8);v0.1.8 M1 嵌入 lnx;v0.1.9 表批 1(6 单元 48,602B 且 native .text 净减 >=40,000B);v0.1.10 tape 速度门(102 cmp EQUAL、16 档 15x,64/102 VM 判 stay-native);v0.1.11 版本槽保留给下一迁移类或按类规则并入;v0.1.12 先拆后迁批 2;v0.1.13 剩余批并冻结留原生清单;v0.1.14 六目标一致加出口门(六目标一致从 v0.2.0 移到这里)。
- **v0.2.x 循环**:账本 `build/opt-ledger.tsv` 绑定候选身份(每切片每目标 `.text` 字节;每基准指令数与中位墙钟,load1>2 记 NOISY;nn 权重字节与节点;tape 步数/原生指令)。基准集 `tests/bench/` 加 `build/bench-set.tsv`,v0.1.x 末冻结 12 项,只追加。候选:A=对性能最热的“留原生”函数做 asm 重写;B=从 gold 构造的 nn 结构/权重改动(不训练)。验证:A 以旧实现为 oracle,全枚举或 golden 字节 cmp,变异先红;B 逐项评估相等;两者都过字节等价回归。仅当目标指标超过噪声带 `>=3%`、其他回退 `<=1%`、六目标验证全绿才保留,否则当天回退并记 REJECTED。每个 v0.2.N = 一个改进类加一个目标增量。
- **前三个**(阈值提议,v0.1.14 重新基线):v0.2.0 nn 体积与步率(暂定先做:权重 `-15%`,tape 倍率 `15x -> 10x`,评估相等);v0.2.1 lnx/x86_64 asm 热类 1(前 5 函数,基准指令中位 `-10%`,`.text` 不增,其他目标保持参考实现);v0.2.2 arm64 asm 加第二批热点(每架构 `-8%`,其他地方零回退)。
- **v0.1.x 期间并行建设**(工具进 main):agy-flow 基准 harness、opt-ledger 与 devsvc 保留/回退门;agyneon 每函数指令画像(扩展 perf-hot);agyneon2 asm 切片构建路径(参考实现与 asm 双轨加字节比较器);agyneon3 nn 重建路径(gold 到权重到逐项评估,一条命令);agy-h 六目标 asm 验证矩阵与 CI 成本;cdx 对保留/回退门做对抗复核;ccogweco 出口门检查加留原生冻结清单生成器;omp-grk 基准噪声带(9 次重复)。
- **风险**:优化即将迁移的切片(由留原生冻结清单阻断);六目标 asm 成本(每版只做一架构,参考实现留作 oracle);基准作弊(冻结、只追加的基准集)。
- **最不确定**:64/102 档 15x 全档门已在 v0.1.10 前按预设 fallback 判 stay-native(14.58%<25%);若后续测量推翻该判定再按新版本号处理;留原生清单是否有足够时间份额做 asm(证伪:立即从 slice-units + perf-hot 算留原生时间份额,低于 30% 则 v0.2.0 先做 nn/tape)。
- **cc 排期**:72 小时 M1 目标优先;v0.2.x 基础设施现在只在 M1 关键路径之外的窗口起步(ccogweco、omp-grk 静态部分、agy-flow 在 v0.1.6 后);M1 所有者(agy-h、agyneon3、agyneon、agyneon2、cdx)在 M1 私有 APE 跑完后接自己的 v0.2.x 基础设施项。

## 追加裁定(2026-11-27:stay-native 暂定、K 复用、噪声带)

### stay-native 暂定(cc,10:4x)

- omp-grk `213ed07bd` 静态测得 stay-native **字节份额** `515,445 / 2,441,257 = 21.1%`,低于 30%;这是代理口径,**时间份额**待 agyneon 每函数画像,记 `UNMEASURED`。
- 暂定顺序:v0.2.0 先做 nn/tape 类(权重与步率),asm 热类移到 v0.2.1;最终顺序以时间份额实测为准。

### K 复用裁定 A(fable,10:5x)

- 公共资格计数**永不**复用证据;K 只是影子实验与重跑排序辅助;v0.1.6 全量实跑,发布说明写“full real run N batches, REUSED 0”。
- 若影子实验无反例,可向所有者提案修订铁律文本;所有者改写前,`REUSED` 永不计为 PASS。
- cc 更正:此前“跨版本等 K 份额 0% < 30%,丢弃”的结论撤回——K 含整个 APE sha,0% 是构造使然,什么都没量到。保留 K 记录;后续用两项测量决定:(a) 同 APE 重冻结(仅注释头变化、APE 字节相同)的等 K 份额;(b) 细 K 只哈希批次实际触达的切片/单元,使未触达变更单元的批次跨版本保留 K;后者可作为 v0.2.x 工具候选。公共资格仍不得计 REUSED。

### 噪声带与保留阈值(omp-grk b540813b7,原生 0.1.6 ed50199f,9x3,load1 2.60)

- 实测噪声带:`stream_e4_c1` 2.790%、`stream_e3_c1` 0.388%、`cli_compile_c1` 10.727%。
- v0.2 每基准保留阈值=`max(3%, 2 x 该基准实测噪声带)`;因此 `cli_compile_c1` 需 `>= 21.5%` 才可保留,在降噪前不能作为保留指标(可增加重复或加长负载)。

## 追加裁定(2026-11-27:速度门收窄、slot promotion、逐单元宿主开销)

### 速度门与超指令

- 超指令原型(agyneon3 安静机):top-3 融合使 16 档 `14.91x`(PASS),64 档 `18.33x`;top-10 上限约 `16.7x`。后置 regalloc 原型停止(`f5d1ab783` 只保留为静态统计工具)。
- v0.1.10 收窄:全部 102 单元在 tape 下、102 档 cmp EQUAL、16 档 `<=15x`;64/102 档 15x 移入新 v0.1.11。
- 新 v0.1.11(证伪后重定义):64/102 档 VM 单元判 **stay-native**,原 slot-promotion 速度目标失去依据——slot promotion 最多去掉约 `15%` 步数,不足以把 `18.3x` 拉进 `15x`。v0.1.11 只保留版本槽,装下一个独立迁移类;无可独立成类者按类规则并入相邻版本。`check-v01x-exit` 的 tier 64/102 15x 项由该 stay-native 判定取代,依据:agyneon 真实 tier 64 `416.2M` 步、load1 2.43,`[r6-OFF]` `14.58%`、加 `[r7+OFF]` `24.3% < 25%`;这是应用预设 fallback,不是放宽。原生基线仍按候选身份冻结。
- 下一步:agyneon 定位 slot 发射点(file:line)并测 tier 64 非取址标量局部 ld/st 动态份额;agyneon3 分片 tier 102 取 cmp EQUAL。

### 逐单元宿主开销(证伪结果)

- agyneon2 `96633e74`:m25 叠在 v0.1.7 链上使 PT_LOAD R E `+3,315`B;tl_dce 后活代码仅约 `2.5KB`,而每单元 string bridge+cache+拒绝约 `5.8KB`,宿主开销真实。
- 决定:(1) TapeRuntime 通用共享 bridge(字符串取值+值调用),把每单元宿主成本降到几百字节(agyneon3);(2) 迁移顺序按 tl_dce 后链接入口里的活代码字节排序,不按对象 `.text`(omp-grk);(3) 表批(新 v0.1.9)在宿主开销实测变小前不开。

### 编译器尺寸候选(记录)

- `__init_u` 把 brace 初始化静态聚合下沉为 `.bss`+逐元素 store:lnx/x86_64 占 `133,891`B `.text`(4.92%)与 `134,424`B `.rela.text`,lnx/arm64 `143,012`B;`m49.o` 单独 `78,148`B。含 const 与非 const;归 v0.2.x 编译器尺寸类,按 opt-ledger 逐切片度量,全量预跑验字节等价。

## 追加裁定(2026-11-27:v0.1.9 = 去静态 libc 副本)

- v0.1.9 定义能力:上层产品单元去掉各自的静态 libc 副本,printf 家族改走 L0 `sy_*`/`mm_*`/`tx_*`;退出判据 PT_LOAD R E 净减 `>= 40,000`B(对新 v0.1.8)。
- 首证 LOCAL:driveroute+tapegate+drivelink 合计 `-99,193`B,诊断字节相同(agyneon2 `6d6bc885b`)。字节棘轮 `check-stdio-copies` 锁产品上层 `.o` 的 `_u_vfmt`/`_u_ffmt`/`_u_dexp`/`fprintf`/`fputs`/`snprintf` 等为零,cxlibc 对 `src/` 与 `cx/` 保持硬门。
- falsifier 已过:`nm` 在 v0.1.7 链 g2 对象中发现 21 个单元带静态 stdio core,共 `322,888`B 对象 `.text`(DCE 前);第一周 driveroute、tapegate、drivelink 做 nm before/after 回执,三单元合计 PT_LOAD R E 净减 `<45KB` 即外推失败。
- tape 迁移不再按大小判定:改按 `tl_dce` 后活逻辑字节加 golden 全枚举;尺寸只报告,不参与判定。v0.1.8 的 6 单元表迁移验收同改此口径。
- v0.1.8 说明须保留:尺寸变化不归因 tape;driveargs `-13,641`B 归因未证;tapegate loader 可能自带 stdio 副本。
- 后续版本(v0.1.10 起)不因本裁定改号或改序。

## 追加裁定(2026-11-27 末段:v0.1.9 纯 libc、v0.1.10 运行时、v0.1.11 静态重复)

- v0.1.9 为**纯** libc 副本去除类:20 个上层单元,`a95f0fc86..bc2177979` 范围,LOCAL 实测 PT_LOAD R E `-441,128`B;原 6 单元表迁移不并入本版,按类规则另行排槽,后续版本号不改。
- v0.1.10 = tape 运行时范围(bridge、call_out、touched-span clear、guest-slim、A2、pack;w/agyneon3 13 提交加 self-audit 修复)加收窄速度门。tier 16 `<=15x` 的判据必须写明**一个** fixture 身份,不能拿不同 fixture 的数字互比;2x2 测量进行中,现有 speed-gate 集 `14.58x`、check-v0110-exit 内置 fixture `15.65x-17.18x` 只作背景。102 档 cmp EQUAL;64/102 档 VM 单元仍按预设 fallback 判 stay-native。
- v0.1.11 = 下一静态重复类:SHA-256 收为单个 L0 `sy_sha256_bytes`(约 `55KB`),`cx_read`/`cx_write` 的 heap 与 `FILE*`(约 `45KB`),deps/driveplan,heap 批;RT_EXTERN 函数共享只作判后 side item(活代码 `63.7KB`)。
- 退出判据只可收紧,不可放宽;v0.1.12 及以后保持原样。
