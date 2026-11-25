# 0.0.107 路径 C 后的两轮链事实分表与公开说明固定说法

编写窗口: `agy-h` (只写研究页; 第一轮事实严格仅作定因, 第二轮占位指标留待 `F2` 实跑后按实填报)
适用范围: `0.0.107` 发布说明、证据索引及 `0.1.0` 承接背景口径

---

## 1. 第一轮链 (`F1`) 与第二轮链 (`F2`) 事实分表

| 事实维度 | 第一轮链 (`F1 = 28b6a518fe`, 只作定因与历史留证) | 第二轮链 (`F2` 新候选, 唯一公开与资格口径) | 第一轮可否进 `107` 证据 |
|---|---|---|---|
| 候选提交 `F` 与 `SOURCES_SID` | `F1=28b6a518fe9fac6b117f31adeb25e1c8ab51701f`; `SID=9b83432e63a84a7fb00a9bb5fcb33533230342c61499903ab13e29b37d4677b6` | 合入 `c99-n2-exitcode` 产品修复及对应派生表后的新提交 `F2` 与新 `SID_2` (`PENDING`) | 否 (`SID` 已因修产品改变) |
| 候选 APE (`SHA256` / 字节) | `9a1e2630100deace2b6a8d82a1b6c47fdf57b872e82f3cd4e1ee55756c2be544` (`8991184B`) | `F2` 整链产出的新 `unisacc-next.com` 与新 `APE_SHA_2` (`PENDING`) | 否 (字节身份不同) |
| 整链实况 (`owner -> exit`) | `02:49:02 -> 03:15:16` (`26.2m`, `1574s`, `499/499 rc=0`, `PGID=1984486`) | `F2` 重新走通 `chain-ready` 全量与 `499/499` 整链 (`PENDING`) | 否 (只作第一轮耗时历史) |
| 预跑区间与结果 | `03:21:56 -> 04:16:43` (`54.8m`, `2-lane`, `done=202/247`), 因 `W0201` 触发 `PRERUN_PRODUCT_RED_STOP` 停机; 第 `30m` `105<110` 触发 `fable` 转保守档 (`03:53`) | 基于 `F2` 与 `APE_SHA_2` 新建独立 `prerun` 目录全量重跑 `~247` 窗至 `PRERUN_RUN status=DONE` (`PENDING`) | 否 (旧 `202` 窗严格 `strict-Wxxxx.tsv` 全部作废不复用) |
| `W0199 fwd-cap` 与旁证 `7` 批 | 首轮 `W0199` 实跑 `PASS` (`assertions=28`, `19846ms`); `cdx` 私有旁证 7 批 `PASS` | 必须在 `F2` 新候选全量预跑与回归矩阵里重新实跑取行 (`PENDING`) | 否 (仅用于定因排除, 不能代第二轮出数) |
| `W0201` (`c99-array-param-typedef`) 与邻近矩阵 | 首轮定因实锤: `ARRPARAM_SUMMARY passed=9 failed=1`, `N2` 输出 `error: expected ']'` 却 `rc=0` 并写出 `N2.o` (`2144B`); 邻近 7 种缺 `]` 构造同类误放 | 验证 `F2` 上 `N2` 变 `rc!=0` 无 `.o`、`ARRPARAM_SUMMARY passed=10 failed=0`、且 `c99-warn`/`negative-diagnostics`/`c99-whitespace` 等无回归 (`PENDING`) | 保留作第一轮缺陷坐实与转路径 C 决策依据 |
| `CI`、`fixedpoint`、打包、`publish`、读回 | 未执行 (止步预跑 `W0201`, 未生成 `--pack` 与草稿) | 全套仅绑定 `F2` 与 `APE_SHA_2` 执行并落盘 `release/evidence/0.0.107/` (`PENDING`) | 仅第二轮产生正式证据 |
| 准入第 `6` 条 (`45m` / `120m`) | 首轮链止 `03:15:16` 到 `04:00:16` 已跨 `45m` (`OVER_45M`) | 按真实重起链消耗更明确超 `45m` 与 `120m`, 依 `R37` 与 `decision-24h-fable.md` 如实记未满足 | 两轮合并记实为未满足 |
| 目录保留处置 | `~/.cache/unisacc/chains/bg-b1/28b6a518fe9fac6b117f31adeb25e1c8ab51701f` 与 `~/.cache/unisacc/prerun/v0.0.107-28b6a518fe` 原样封存不动 | 新起独立 `bg-b1/<F2>` 链目录与 `v0.0.107-<F2>` 预跑目录 | 第一轮只作只读历史封存 |

核心隔离规则:
- 第一轮 (`28b6a518fe`) 产出的一切 `strict-Wxxxx.tsv`、`ape-receipt.txt`、`W0199` 与旁证 `7` 批结果, 全部只归属于第一轮故障定因与路径 C 依据; 不得将其中任何一行 `PASS` 或耗时数字拼入 `0.0.107` 正式 `strict.tsv`、发布说明资格栏或 `0.1.0` 承接表。

---

## 2. 公开说明中"为何公开版是第二轮候选"的三句固定说法

可直接纳入 `0.0.107` 发布说明(已知背景/候选溯源节)与 `0.1.0` 交接说明:

1. 首轮拦截事实: `0.0.107` 首轮候选 (`28b6a518fe`, APE `9a1e263010...`) 于 `2026-11-25 04:16:43 +0800` 在 `2-lane` 严格预跑推进至 `W0201` (`c99-array-param-typedef`, `202/247`) 时, 因编译器对非法形参 `N2` (`typedef int row[3]; void f(row r[2);`) 发出 `error: expected ']'` 后仍返回 `rc=0` 并写出对象文件 (`2144` 字节), 触发 `PRERUN_PRODUCT_RED_STOP` 停机并经独立复核坐实为产品缺陷。
2. 路径 C 隔离决策: 依项目铁律(严禁改检查迁就、严禁跨候选复用严格合格证据), 于 `04:21` 裁定执行路径 C, 在修复产品报错后退出码与产物拦截路径的基础上以新提交 `F2` 重新起链并全量重跑全套严格窗口与跨平台 CI; 首轮链目录及 `202/247` 预跑日志仅原样保留作定因证据, 不计入任何正式 `PASS` 统计。
3. 公开版唯一绑定: `0.0.107` 公开发布产物及全套 `strict`、`fixedpoint-6targets`、六格原生 `CI` 与公开读回验收证据均严格唯一绑定第二轮候选 (`F2` 及其真实 `APE_SHA256`), 且第 `6` 条时限指标 (`T2->X > 45m` 与 `T1->pub > 120m`) 均按实情记录为未满足。

---

## 3. 配套证据入口清单

- 事件与路径 C 重估页: [path-c-event-107.md](path-c-event-107.md)、[chain-to-public.tsv](chain-to-public.tsv)、[prerun-rate-107.tsv](prerun-rate-107.tsv)
- 首轮 `W0201` 停机原始证据 (`~/.cache/unisacc/prerun/v0.0.107-28b6a518fe/`):
  - `strict-W0201.tsv` (`FAIL rc=1`, `marker=ARRPARAM_SUMMARY`, `assertions=0`, `ms=3610`)
  - `strict-W0201.tsv.stderr` (`QUALIFY_BATCH c99-array-param-typedef FAIL ms=3610`)
  - `supervise.log` (`PRERUN_PRODUCT_RED_STOP` 与 `PRERUN_STOP` 原行)
  - `~/.cache/unisacc/chains/bg-b1/28b6a518fe9fac6b117f31adeb25e1c8ab51701f/exit.tsv` (`EXIT 1795547716 pid=1984486 rc=0 why=exited`)
- `cdx` 独立直证页 (`~/.local/share/unisacc/research/cdx/`):
  - `w0201-independent-review.md` (`ARRPARAM_SUMMARY passed=9 failed=1 total=10`, `N2.o 2144B`, 宿主 `cc` `rc=1`, 邻近 7 构与旁证 7 批)
  - `n2-regression-matrix.md` (N2 公共退出路径静动态约束与第二轮修复矩阵)
