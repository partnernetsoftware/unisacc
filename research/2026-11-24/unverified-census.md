# 未验证与未关闭项清点清单(2026-11-24)

核对范围: `prd_mempalace.md` 的 `仍未验证或未关闭`(第 104-118 行)与 `仍未验证(缺什么 / 清零动作)`(第 119-135 行), 以及唯一真源 `build/state-unverified.tsv`(共 24 行: 表头 1 行 + 数据 23 行)。只读清点, 未改任何已有文件, 不作优先级裁定。

## 1. prd_mempalace.md「仍未验证或未关闭」逐行清点(共 10 行)

| 序号 | 原文行号 | 摘要(40字内) | 类别 | 证据入口 | 是否可本周关闭 | 谁能关闭 |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 106 | freeze-scope 手工登记缺 register 材料列校验 | 缺陷 | `tools/register.cx:1458`、`~/.local/share/unisacc/research/freeze-scope/supplement/` | 是 | agyneon / cc |
| 2 | 108 | moltbaby system-cleanup 删超100M会话与记忆 | 外部依赖 | `~/repos/moltbaby/systemd/scripts/system-cleanup.sh`(仓外脚本) | 需外部 | 所有者 + moltbaby维护者 |
| 3 | 110 | 分车道三项未核(APE字节/14个.o/sysx别名) | 未验证 | `plans/reviews/2026-11-20-layer-lanes.md` | 是 | cc / agy-flow / cdx |
| 4 | 111 | arm64 -funit x7栈在sp之下可被异步信号覆盖 | 缺陷 | `build/pkg-gaps.tsv:183` (`OBJABI-ARM64-X7-STACK-BELOW-SP-SIGNAL`) | 是 | cc / cdx |
| 5 | 112 | release-flow stage-a 未支持 expected-fail image= | 缺陷 | `tools/release-flow.cx:84,232` | 是 | cc(发布工具线) |
| 6 | 113 | precommit 不实跑 affected 所选批次(E4) | 缺陷 | 无(E4 评审在会话临时目录已佚) | 是 | agy-flow |
| 7 | 114 | affected 缺 batch-product-inputs 漏行拒控负控 | 缺陷 | `c1709e5506` | 是 | agy-flow / cdx |
| 8 | 115 | run-batch 零断言/计数盲区(E9) | 未验证 | 无(E9 zero-candidates 清单在会话临时目录已佚) | 否 | cc / agy-flow |
| 9 | 116 | tls13-get 修复前所取外部内容未重验真 | 待补材料 | `7f277e7344` | 是 | cc |
| 10 | 117 | old-product-assets 待登记 0.0.101 基线 | 待补材料 | `tools/old-product-assets.tsv`、`c4a154dfea` | 是 | agy-flow / cc |

注: 第 10 项(L117)经查 `tools/old-product-assets.tsv` 已由 `c4a154dfea` 写入 0.0.101 行(`prd_mempalace.md` 第 144 行已记), 仅第 117 行表述未同步关闭。

## 2. build/state-unverified.tsv 全表逐行清点(共 23 行)

| 序号 | ID | 摘要(40字内) | 类别 | 证据入口 | 是否可本周关闭 | 谁能关闭 |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | U02 | check-stdio-buffer 缓存/tty/六目标覆盖缺口 | 未验证 | `prd_mempalace.md`(含#仍未验证) | 需外部 | integrator / cdx |
| 2 | U05 | osx cache-manager 借用来源与 macOS 实跑 | 待补材料 | `5c4ec0e227`、`b54d7db899` | 需外部 | integrator |
| 3 | U06 | tsv-lint 同日 PASS/FAIL 不稳定因 | 未验证 | `tools/tsv-lint.cx` | 是 | integrator / agy-flow |
| 4 | U07 | wt-gate-probe PLANNED(6断言与win证据) | 待补材料 | `tests/batches.tsv` | 需外部 | integrator |
| 5 | U08 | slice-migration-gen 声明过时待复核 | 未验证 | `tools/slice-migration-gen.cx` | 是 | integrator / agy-flow |
| 6 | U09 | affected-iface2 批次未取得实跑结果 | 待补材料 | `908df53919` | 是 | integrator / agy-flow |
| 7 | U11 | tools/status.cx 测试夹具违规内容重审 | 未验证 | `tools/status.cx` | 是 | integrator |
| 8 | U12 | cx 模块别名语法 use m::sym as alias 缺口 | 缺陷 | 无(原列 `prd.md 版本表`) | 否 | integrator / cdx |
| 9 | U13 | cx 模块软链身份 realpath 规范化缺口 | 缺陷 | 无(原列 `prd.md 版本表`) | 否 | integrator / cdx |
| 10 | U14 | 仓库根 out/ (a)类写者迁移与 csih 信封 | 缺陷 | 无(原列 `prd_mempalace.md#仍未验证`) | 需外部 | integrator / csih |
| 11 | U18 | 大型二维自动数组编译器报 not covered | 缺陷 | 无(所列 `repro-large-2d-auto-array.cx` 仓内未见) | 否 | integrator / cdx |
| 12 | U19 | compile-as-limit 低限额具名拒绝失败 | 缺陷 | 无(原表写 `-`) | 是 | integrator / cdx |
| 13 | U20 | core-compat 参考源 P4/SBLOAD 独立核验 | 未验证 | `cacb65b83e`、`release/notes-0.0.71.md`、`tests/core-compat.cx` | 是 | integrator |
| 14 | U21 | 全套入口分片聚合端到端验收 | 未验证 | `release/notes-0.0.71.md` | 是 | integrator / agy-flow |
| 15 | U22 | 118材料族全套材料与外部主机证据缺口 | 待补材料 | `release/notes-0.0.71.md` | 需外部 | integrator |
| 16 | U23 | stages 五项字节差异缺独立金样 | 待补材料 | `release/notes-0.0.71.md`、`tests/check-stages.cx` | 否 | integrator |
| 17 | U25 | interfaces-lnx-x86_64 既有 oracle 两轮FAIL | 缺陷 | `release/notes-0.0.71.md`、`tests/interfaces-lnx-x86_64.cx` | 是 | integrator |
| 18 | U26 | c-negative/pp-negative 17项非法输入静默接受 | 缺陷 | `release/s9-assert-0.0.71-summary.txt`(原列 `/tmp` 路径) | 否 | integrator / cdx |
| 19 | U27 | 额度中断期间三张小单未验收(不采纳半成品) | 未验证 | `archive/plans/local-product-0.0.71.md` | 是 | integrator |
| 20 | U28 | ujs compiler_min.cx cxraw 实测 96 须压回 | 外部依赖 | `ujs/prd.md`、`d473474f14` | 需外部 | ujs-lane |
| 21 | U29 | ujs/product 九文件棘轮调用数须减半 | 外部依赖 | `03f220a935` | 需外部 | ujs-lane |
| 22 | U30 | release-chain 端到端 --dry 真实形态实跑 | 未验证 | `tools/release-chain.cx` | 是 | integrator / cc |
| 23 | U33 | sy_proc_kill 回收后 pid 复用竞态(0.0.94遗留) | 缺陷 | `plans/v0.0.94.md`、`cx/sys/proc.cx` | 是 | ccogweco / integrator |

## 3. prd_mempalace.md「仍未验证」未入 tsv 的额外项(共 4 行)

注: `prd_mempalace.md` 第 119-135 行中, 第 123-127、129-131 行已对应 `U20-U30`, 第 134 行汇总指向 `U02-U19`, 余下以下 4 行未在 `build/state-unverified.tsv` 建行:

| 序号 | 原文行号 | 摘要(40字内) | 类别 | 证据入口 | 是否可本周关闭 | 谁能关闭 |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 128 | 发布裁决文件 decision.txt 停在 0.0.74 陈旧 | 缺陷 | `tools/release-exec.cx` | 是 | cc(发布工具线) |
| 2 | 132 | 单元缓存与预构建对象键是否含模块接口摘要 | 未验证 | 无 | 是 | cdx / agy-flow |
| 3 | 133 | ext/sysx/linuxns.cx arm64实跑与去dock | 未验证 | `build/pkg-gaps.tsv:209` (`L0-LACKS-SY-SYSCALL6`) | 需外部 | integrator / 库窗口 |
| 4 | 135 | forward 350无名失败与450段错误(0.0.126) | 缺陷 | `research/2026-11-24/R26-ruling.md` | 否 | cdx / ccogweco |

## 4. 两来源差异对照

(1) 在 `prd_mempalace.md` 而不在 `build/state-unverified.tsv`(共 `14` 条):
- `### 仍未验证或未关闭` 全节 `10` 条(第 106、108、110、111、112、113、114、115、116、117 行)。
- `### 仍未验证(缺什么 / 清零动作)` 中未入真源表的 `4` 条(第 128、132、133、135 行; 其中第 128、132、135 行明确标了 `未入 state-unverified.tsv,待补行`)。

(2) 在 `build/state-unverified.tsv` 而不在 `prd_mempalace.md` 正文直接展开(共 `13` 条):
- `U33`(第 24 行 `sy_proc_kill` 0.0.94 遗留行): 在 `build/state-unverified.tsv` 仍为 `OPEN`, 但 `prd_mempalace.md` 第 134 行与正文均未提及 `U33`。
- `U02、U05、U06、U07、U08、U09、U11、U12、U13、U14、U18、U19` 共 `12` 条: `prd_mempalace.md` 仅在第 134 行列出了编号(`其余不阻塞发布,只见真源`), 未在正文展开描述。

## 5. 汇总

(1) 行数核对:
- `prd_mempalace.md`「仍未验证或未关闭」: `10` 行(可本周关闭 `8` 行, 否 `1` 行, 需外部 `1` 行)。
- `build/state-unverified.tsv` 数据行: `23` 行(可本周关闭 `11` 行, 否 `5` 行, 需外部 `7` 行)。
- `prd_mempalace.md`「仍未验证」未入 `tsv` 额外行: `4` 行(可本周关闭 `2` 行, 否 `1` 行, 需外部 `1` 行)。
- 两来源合并去重总行数: `37` 行(`10 + 23 + 4`), 其中可本周关闭合计 `21` 行(`8 + 11 + 2`)。
