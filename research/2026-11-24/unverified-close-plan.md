# 可本周关闭未验证项分组方案(2026-11-24)

依据: `research/2026-11-24/unverified-census.md` 核对后的全部 `21` 条「可本周关闭」项(小节 A「仍未验证或未关闭」`8` 行 + `build/state-unverified.tsv` `11` 行 + 小节 B 额外 `2` 行, 无重复计入)。每行恰好归入一个关闭窗口, 不作优先级排序, 无有效证据入口者标明「无证据,需先查」。

## 1. cc(integrator / 发布工具线, 共 7 行)

关闭方案一句话: 补齐 `release-flow` image= 解析与 `release-exec` 陈旧裁决检查, 并逐一核销历史已修或小单条目(`old-product-assets`、`tls13-get`、`status.cx`、`U27`、`U30`)。

| 序号 | 来源标识 | 行摘要 | 关闭动作(一句话,依据证据入口) | 预计耗时(分钟) |
| --- | --- | --- | --- | --- |
| 1 | L112 | release-flow stage-a 未支持 expected-fail image= | 按 `tools/release-flow.cx:84,232` 补齐 `image=` 解析并跑通自测 | 25 |
| 2 | L116 | tls13-get 修复前所取外部内容未重验真 | 按 `7f277e7344` 核查修复前外部拉取记录并重拉验真后关闭 | 15 |
| 3 | L117 | old-product-assets 待登记 0.0.101 基线 | 核对 `tools/old-product-assets.tsv` 与 `c4a154dfea` 已落盘行后关闭记忆宫殿行 | 5 |
| 4 | U11 | tools/status.cx 测试夹具违规内容重审 | 只读复审 `tools/status.cx` 夹具内容并同步更新 `state-unverified.tsv` | 15 |
| 5 | U27 | 额度中断期间三张小单未验收(不采纳半成品) | 依据 `archive/plans/local-product-0.0.71.md` 逐单核对重派或作废关闭 | 20 |
| 6 | U30 | release-chain 端到端 --dry 真实形态实跑 | 在私有夹具用 `tools/release-chain.cx` 按真实证据形态跑通五步到 `READY_TO_EXECUTE` | 25 |
| 7 | L128 | 发布裁决文件 decision.txt 停在 0.0.74 陈旧 | 在 `tools/release-exec.cx` 第 4 步加本候选版本号断言并补自测负控 | 20 |

## 2. agy-flow(提交门 / 测试流线, 共 6 行)

关闭方案一句话: 补齐 `affected` 缺行负控与 `precommit` 有界批次触发, 并实跑或复核积压门检查(`U06`、`U08`、`U09`、`U21`)。

| 序号 | 来源标识 | 行摘要 | 关闭动作(一句话,依据证据入口) | 预计耗时(分钟) |
| --- | --- | --- | --- | --- |
| 1 | L113 | precommit 不实跑 affected 所选批次(E4) | 无证据,需先查(原 E4 评审在会话临时目录已佚, 需先定 `precommit` 接 `affected --run` 子集范围) | 待估 |
| 2 | L114 | affected 缺 batch-product-inputs 漏行拒控负控 | 依据 `c1709e5506` 在对应测试补一条表缺行具名拒绝负控 | 15 |
| 3 | U06 | tsv-lint 同日 PASS/FAIL 不稳定因 | 复核 `tools/tsv-lint.cx` 并发读写或下次红时留现场定因后关闭 | 20 |
| 4 | U08 | slice-migration-gen 声明过时待复核 | 只读核对 `tools/slice-migration-gen.cx` 是否仍报 `source-missing` 并更新或关闭行 | 15 |
| 5 | U09 | affected-iface2 批次未取得实跑结果 | 依据 `908df53919` 实跑 `affected-iface2` 批次并留证关闭 | 10 |
| 6 | U21 | 全套入口分片聚合端到端验收 | 依据 `release/notes-0.0.71.md` 在新候选上实跑分片聚合入口并核对总计 | 30 |

## 3. cdx(只读核查与独立验证线, 共 6 行)

关闭方案一句话: 只读静态核验分车道未核项、0.0.82 x7 状态、缓存键及 `core-compat`/`interfaces` oracle, 并先查无证据的 `U19` 与 `L132`。

| 序号 | 来源标识 | 行摘要 | 关闭动作(一句话,依据证据入口) | 预计耗时(分钟) |
| --- | --- | --- | --- | --- |
| 1 | L110 | 分车道三项未核(APE字节/14个.o/sysx别名) | 依据 `plans/reviews/2026-11-20-layer-lanes.md` 跑别名使用扫描与私有构建字节比对 | 20 |
| 2 | L111 | arm64 -funit x7栈在sp之下可被异步信号覆盖 | 依据 `build/pkg-gaps.tsv:183` 核对 `0.0.82` 提交与反汇编, 确认是否已修可关 | 15 |
| 3 | U19 | compile-as-limit 低限额具名拒绝失败 | 无证据,需先查(原表 evidence 为 `-`, 需先在当前候选实跑复现确定过或补合同) | 待估 |
| 4 | U20 | core-compat 参考源 P4/SBLOAD 独立核验 | 依据 `cacb65b83e`、`release/notes-0.0.71.md` 与 `tests/core-compat.cx` 独立核验新转换与负控 | 30 |
| 5 | U25 | interfaces-lnx-x86_64 既有 oracle 两轮FAIL | 依据 `release/notes-0.0.71.md` 与 `tests/interfaces-lnx-x86_64.cx` 迁移接口 oracle 并独立核验 | 45 |
| 6 | L132 | 单元缓存与预构建对象键是否含模块接口摘要 | 无证据,需先查(先读单元缓存与预构建对象键源码定位是否含接口摘要) | 待估 |

## 4. agyneon(devsvc 源码线, 共 1 行)

关闭方案一句话: 补齐 `register.cx` 材料列支持并重走 `freeze-scope` 三批次登记校验。

| 序号 | 来源标识 | 行摘要 | 关闭动作(一句话,依据证据入口) | 预计耗时(分钟) |
| --- | --- | --- | --- | --- |
| 1 | L106 | freeze-scope 手工登记缺 register 材料列校验 | 依据 `tools/register.cx:1458` 补支持材料列后按 `~/.local/share/unisacc/research/freeze-scope/supplement/` 重走登记校验 | 30 |

## 5. ccogweco(0.0.94/0.0.96 遗留核销, 共 1 行)

关闭方案一句话: 核对 `0.0.96` 已落地的 `sy_proc_kill` 状态位与回归证据后关闭 `U33`。

| 序号 | 来源标识 | 行摘要 | 关闭动作(一句话,依据证据入口) | 预计耗时(分钟) |
| --- | --- | --- | --- | --- |
| 1 | U33 | sy_proc_kill 回收后 pid 复用竞态(0.0.94遗留) | 依据 `plans/v0.0.94.md` 与 `cx/sys/proc.cx` 核对 `0.0.96` 落地提交与 200 轮回归证据后关闭 | 15 |

## 6. 各窗口行数合计

| 窗口名 | 可本周关闭行数 | 已估项合计耗时(分钟) | 含「待估」项数 |
| --- | --- | --- | --- |
| cc(integrator / 发布工具线) | 7 | 125 | 0 |
| agy-flow | 6 | 90 | 1 (`L113`) |
| cdx | 6 | 110 | 2 (`U19`、`L132`; 注 `L111` 15分仅核查) |
| agyneon | 1 | 30 | 0 |
| ccogweco | 1 | 15 | 0 |
| 合计 | 21 | 370 | 3 |
