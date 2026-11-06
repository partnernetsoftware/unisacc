# 慢车道试点方案：pp-negative 2 处（宏名标识符 + 头名闭合）

目标：证明「事实表 → 生成器（带 --check）→ 模型 → 包 → 候选 → pp-negative/c-negative 实跑」
整条慢车道链在 dev-lnx 布局下走得通，只做 pp-negative 2 处，不修 25 处，不安装不替换根产品。

## 1. 改哪几张事实表的哪几行

### 1a. 宏名须为标识符（`#define 3x 1`，6.10.3）
- 表：`exec/pp/macro-byte.tsv`（pp 模型宏状态机，4 行）。
- 现状：`MFIND  *  MF1  [...]` —— `MFIND` 状态 observation 是通配 `*`，任何 token 都进 MF1，
  没有「宏名非标识符」的拒绝分支，所以 `#define 3x`（数字开头）被静默接受。
- 改法：在 `MFIND` 状态前加一条「非标识符 token 类 → REJECT」的转换行。observation 的
  具体数值（lex token 类）需先确认 exec/facts/pp-*.tsv 的 token 类编码；若数字/字符串等
  非标识符类有明确枚举值，则写 `MFIND  <非标识符类>  DEAD  [["REJECT","macro name must be an identifier"]]`。

### 1b. 头名未闭合（`#include <stdio.h`，6.4.7 / 6.10.2）
- 表：`exec/pp/include-location-byte.tsv`（3 行）+ 配套 result 表。
- 现状：`INCOK * INC.location`，`INC.location 0-1/2 INC.record` —— 没有「头名在 `>` 前到达
  行尾/EOF」的 REJECT，所以 `#include <stdio.h`（无 `>`）被静默接受。
- 改法：在 `INC.location` 状态加「EOF/换行而未闭合 → REJECT」分支。

## 2. 预期生成物 diff

- 输入事实表：`exec/pp/macro-byte.tsv`、`exec/pp/include-location-byte.tsv` 各 +1 行 REJECT。
- 生成物：`weights/pp.*.unisa`（pp 模型网，经 model-gen/kernel-build 重建后字节变化）、
  `kernel/`（若 pp 在内核模型链内）、最终产品 `entry.elf` 的 pp 模型 blob 变化。
- 不手改生成物：生成物只经构造链重生成，diff 由构造链产物决定。

## 3. 重建步骤（bound 与耗时）

慢车道纪元构造链（各步独立 bound 55-58s，正规构造链允许一次调用跑完一个相位）：
1. `model-gen.cx --check` 后 `--write`：事实表 → 模型名表（<55s，1 次）。
2. `kernel-build.cx`（prepare/assemble/link 各步）：模型网 → kernel（各步 ≤55s，约 3-5 次）。
3. `release-build.cx`：p3 包（约 1-2 min，多步 bound）。
4. `entry-build.cx`：入口候选（约 35-40s，已在层1 流程用过）。
5. `selfhost-mixed.cx`（可选，若需两代验证）：4 endpoint + compare，各 ≤55s。

总耗时约 25-40 min（与 dev-methodology 的「构造 25 min」一致）。

## 4. 验证方式

全部在 `out/` 独立候选（如 out/pp-neg-candidate）验证，不安装：
- pp-negative 的 2 项转绿（macro-name-not-identifier、header-name-unclosed）。
- 其余 pp-negative/c-negative 已拒绝项不退化（对照旧候选同批结果）。
- 十二项 p1-baseline（run_argc 等，含 -run -- 透传）全绿。
- 七棘轮 7/7（srcshape 只降不升）。
- 字节等价口径：非 pp 相关输入的 `-E`/token 输出与旧候选逐字节一致（pp 只改了宏名/头名
  拒绝分支，不影响正常输入）；用 `tools/pp-differential.cx` 或 `-E` 字节回归确认。

## 5. 回滚方式

- 不改根产品，全部在 `out/` 独立候选；根产品（unisacc.com + unisacc.com.dev/）不动。
- 事实表回滚：`git checkout -- exec/pp/macro-byte.tsv exec/pp/include-location-byte.tsv`。
- 生成物不入库（weights/kernel 若在 git 跟踪则按构造链重生成回旧值）。
- 若 pp 模型的 observation token 类数值无法确认（无生成器、无 oracle），则本试点如实记录
  「宏名非标识符的 token 类枚举缺失」，回退到「先补 token 类 oracle」这一步，不改表。

## 6. 已知风险

- `exec/pp/macro-byte.tsv`、`include-location-byte.tsv` 没有生成器（archive Python 已归档，
  facts-gen.cx 不处理它们）。它们属「手工模型表」，改它们直接改模型定义，不是「事实表→
  生成器」路径。cc 要求「事实表只经生成器修改」——这两张表当前没有生成器，是试点要先
  确认的一个缺口：要么把它们纳入一个生成器（新增生成器，属快车道工具工作），要么如实
  标注为「手工模型表，慢车道纪元直接改 + 表 lint 校验」。方案建议后者（先 lint 校验，生成器化
  列为后续债务），但需 cc 确认。
- 慢车道节奏：本改动属「每周一次正规换标」；试点的候选不进矩阵证据（dev-methodology
  第 14 条：矩阵证据只绑定最近一次慢车道纪元候选）。

## 7. 分工边界

- 本项改 `exec/pp/*.tsv`、`exec/facts/*.tsv` 及生成物（weights/kernel/候选）。
- omp-go 改 tools/cxlib/productpkg.cx、tests/*、allsession；不抢文件。
- 共用文件（若有）先报 cc。

## 8. 补充裁定后（cc 批准直接改表+lint+钉子）

- token 类 oracle（已确认）：宏名合法首字符 = ASCII 65-90(A-Z) 95(_) 97-122(a-z)，
  与 lexcls.tsv 的 `A` 类一致；数字 = 48-57（`d` 类）。observation 域是 ASCII 字节值
  （不是 c 类），见 exec/pp/cli-byte.tsv `CLI.D.next 65-90,95,97-122` 同格式。
- 改法（MFIND 前插拒绝分支）：
  `MFIND  48-57,0-64,91-94,96,123-127  DEAD  [["REJECT","macro name must be an identifier"]]`
  （非标识符首字符的全部字节，不含 65-90/95/97-122；具体范围按表 lint 的 observation 区间语法）。
- 头名闭合（INC.location 前插）：INC.location 观察 EOF/换行（字节 10）而无 `>`（62）时 REJECT。
- 钉子：新增 check 登记批（记录 macro-byte.tsv / include-location-byte.tsv 的 sha256 +
  最近变更提交号/理由行），表被改而钉子未更新则失败。
- 债务：plans/ 记「两表无生成器，待生成器化；触发条件=第二次需改这两张表时必须先生成器化」。
- 构造步骤：改表 → table-lint → sim --cov（新分支覆盖）→ model-gen --check/--write →
  kernel-build → release-build → entry-build → out/ 独立候选实跑 pp-negative。
