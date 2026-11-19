# Q-P(产品资格)口径草案:0.0.83 首次尝试版的分母与门(待评审)

起草:2026-11-19,0.0.81 预跑期间。依据:0.0.80 整域首测 609 批(PASS 493 -> 老件转绿后 523、FAIL 7 -> 1、UNKNOWN 109 -> 79)。

## 要解决的问题

整域分母 609 里混着四种东西:应测且可测(PASS/FAIL 有意义)、应测但缺材料(no-producer)、本平台不适用(平台不适用 15)、尚未实现的源(非 IMPLEMENTED SOURCE 9),以及环境缺失(kernels 多相备料 1、缺主机 tcc 1)。笼统说 "UNKNOWN=0" 会逼人把不适用的批放宽成 PASS 或删掉,违反不缩覆盖。需要一张具名分类让分母诚实。

## 分类(每批在登记表里有且只有一个资格状态)

| 状态 | 含义 | 计入 Q-P 分母 | 发布门 |
|---|---|---|---|
| required | 应测,本机可实跑 | 是 | 必须 PASS,FAIL 只许在 expected-fail(带到期) |
| required-external | 应测,但需要外目标执行环境(arm64、osx、win 执行格) | 是,按格 | 本机格 PASS;外目标格由同候选 CI 六目标证据绑定,没有 CI 证据记 UNVERIFIED 并使 Q-P 不能称满 |
| not-applicable | 本平台或本候选结构上不适用(例如只对 win 的批在 lnx 跑) | 否,但列名单 | 名单变化须评审 |
| not-implemented | 合同还没有实现的源(IMPLEMENTED 之外) | 否,但列名单与目标版本 | 名单只许缩小,新增须评审 |
| env-missing | 环境缺失(主机 tcc、多相备料) | 是 | 补环境或具名退役,到期未补则发布门红 |

约束:状态只能由登记表工具改,不能由批次自己或发布单覆盖;降级(required -> not-applicable)需带理由与评审人,并在 prd_mempalace 留一行;不得因为 FAIL 改状态。

## 门(0.0.83 首次尝试版:报告不阻塞,0.0.84 起阻塞)

(1) 同候选整域实跑一次(609 批全分片,jobs 2 到 3,单写者),产出 SUMMARY:required PASS、FAIL(逐个在 expected-fail 且未到期)、UNKNOWN(只许 env-missing 带到期与 required-external 无 CI 证据)、INVALID=0。
(2) 与上一版对账:required 的 PASS 集合只许增,不许减(减了必须在发布说明列名并有理由)。
(3) required-external 的 CI 绑定:六目标 CI 的每个运行实际跑过哪些批,用 CI 日志里的 BATCH_RUN 行核对,而不是信 CI success。
(4) 整域耗时与慢批账:超过 33 秒的批(0.0.80 首测 21 批)每版只许减;modelcache-budget-lru 与 concurrent 近 60 秒要先拆。

## 与现有工具的衔接

- 状态列加入 tests/batches.tsv 或旁表(旁表 tests/qualification-state.tsv,事实表 -> 生成器 -> 登记),G1 读取。
- expected-fail 与 expected-unmeasured 合并成一张带到期列的表,G1 同步。
- dvqualify 的 SUMMARY 增加按状态分列的计数,并输出 required 的 PASS 集合哈希用于版本间对账。

## 风险与待定

(1) required-external 的 CI 绑定要求 CI 日志里有逐批行,目前 CI 六目标只跑 44 或 42 个格(candidate-native.yml),与 609 批不是同一个集合,需要先对账哪些批在 CI 里跑。(2) 状态降级可能被滥用,需要版本间 diff 门。(3) ref 类外目标执行格长期 UNVERIFIED,Q-P 不能称满的状态可能持续多个版本,发布说明必须如实写。
