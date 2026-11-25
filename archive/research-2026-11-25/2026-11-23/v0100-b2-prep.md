# 0.0.100 B2: 17 个上限入 `exec/facts/k2-caps.tsv`、生成器与 `cap-probe` 先红后绿验收

2026-11-23,agy-flow。版本分支 `v0.0.100`,工作树 `.claude/worktrees/v0.0.100`(基于 `main` 头 `0333a056ec`)。完整可应用补丁存于 `research/2026-11-23/v0100-b2-series/0001-b2-k2-caps-table-gen-probe.patch`(856 行)。

## 1. `exec/facts/k2-caps.tsv` 表结构(17 行 x 4 列:名字、当前值、来源文件行、用途)

| # | name | value | source | purpose | 旧常量别名(保留一版) |
|---|---|---|---|---|---|
| 1 | `ERR_TEXT_MAX` | `283` | `exec/pp/directive-scan-result.tsv:12` | `not covered: #error text longer than 283 bytes` | - |
| 2 | `INCLUDE_SPLICE_MAX` | `4000` | `exec/pp/directive-body-byte.tsv:43` | `not covered: more than 4000 #include splices` | - |
| 3 | `CONTINUATION_MAX` | `4096` | `exec/pp/directive-body-byte.tsv:79` | `not covered: more than 4096 continuation lines in headers` | `MAXISPL` |
| 4 | `INT_CONST_BITS` | `63` | `exec/parse2/unarycontrol-result.tsv:177` | `not covered: constant beyond int` | - |
| 5 | `ENUM_POOL` | `128` | `exec/parse2/enumtypes-result.tsv:8` | `not covered: enum descriptor pool exhausted` | `ENUM_LIMIT` |
| 6 | `SIGNATURE_POOL` | `8192` | `exec/parse2/functiontypes-result.tsv:28` | `not covered: function signature pool exhausted` | `SBB` |
| 7 | `SETJMP_EXPR_DEPTH_MAX` | `40` | `exec/parse2/setjmpcontrol-result.tsv:86` | `not covered: setjmp inside an expression nested too deeply` | - |
| 8 | `STRUCT_ID_MAX` | `2049` | `exec/parse2/control-result.tsv:510` | `not covered: structure id capacity` | `STRUCT_LIMIT` |
| 9 | `STRUCT_MEMBER_MAX` | `512` | `exec/parse2/control-result.tsv:575` | `not covered: structure member capacity` | - |
| 10 | `FORWARD_CALL_MAX` | `8192` | `exec/parse2/forward-result.tsv:17` | `not covered: host-forwarded call count exceeds 8192` | - |
| 11 | `RELOC_MAX` | `262144` | `exec/enc/x86object-result.tsv:30` | `not covered: object relocation capacity` | - |
| 12 | `LAYOUT_SID_MAX` | `128` | `exec/parse2/layoutfacts-result.tsv:10` | `not covered: ordered source layout facts limit or resource` | - |
| 13 | `SIG_EQ_DEPTH_MAX` | `31` | `exec/parse2/functiontypes-result.tsv:388` | `not covered: recursive signature equality limit` | - |
| 14 | `SETJMP_TEMP_MIN` | `0` | `exec/parse2/setjmpcontrol-result.tsv:89` | `not covered: setjmp temporary depth` | - |
| 15 | `SHAPE_SPAN` | `67108864` | `exec/parse2/shape-result.tsv:120` | `not covered: shape descriptor capacity` | `POSSPAN` |
| 16 | `MODELGRAPH_NODE_MAX` | `17409` | `exec/modelgraphequality-result.tsv:12` | `not covered: signature graph equality budget` | - |
| 17 | `TOKEN_POS_SPAN` | `67108864` | `exec/parse2/control-result.tsv:2231` | `not covered: token input exceeds position domain` | `POSSPAN` |

## 2. 生成器 `tools/k2-caps-gen.cx` 与旧常量一版别名保留

- 为避免改动 `tools/facts-gen.cx` 自身字节身份而触发其余 37 张 `exec/facts/*.tsv` 表头 `# input tools/facts-gen.cx sha256:...` 整体漂移(并跨过单版 A 类 15 文件上限),参照 `tools/signal-targets-gen.cx` 与 `tools/core-abi-gen.cx` 设独立生成器 `tools/k2-caps-gen.cx`(配 `tools/check-k2-caps-gen.cx` 与 `tests/manifests/k2-caps-gen.tsv`):
  - `sh ./unisacc.com tools/k2-caps-gen.cx -- --write` 读取 `exec/facts/k2-caps.tsv`,校验 17 行 4 列 schema、整数值、`file:line` 真实存在性及守卫行/旧常量表同步性,写出 `exec/facts/k2-caps-consts.tsv`:
    - 17 条标准常量行:`=ERR_TEXT_MAX	int	283` .. `=TOKEN_POS_SPAN	int	67108864`
    - 5 条旧常量一版别名行:`=MAXISPL	int	4096`、`=ENUM_LIMIT	int	128`、`=SBB	int	8192`、`=POSSPAN	int	67108864`、`=STRUCT_LIMIT	int	2049`
    - 汇总字典行:`=CAPS	json	{"ERR_TEXT_MAX":283,...,"MAXISPL":4096,"ENUM_LIMIT":128,"SBB":8192,"POSSPAN":67108864,"STRUCT_LIMIT":2049}`
  - `sh ./unisacc.com tools/k2-caps-gen.cx -- --check` 逐行核对 `exec/facts/k2-caps.tsv` 与 17 处消费源表行(`source` 列)、5 处旧常量定义表(`pp-layout.tsv`、`enumtypes.tsv`、`functiontypes.tsv`、`gen2-constants.tsv`、`k2-control.tsv`)及 `exec/facts/k2-caps-consts.tsv` 逐字节一致性。

## 3. `tools/cap-probe.cx` 全量读表与先红后绿实测

- 修改 `tools/cap-probe.cx`:
  - 启动时默认读取 `<cwd>/exec/facts/k2-caps.tsv`(可通过 `--caps PATH` 指定替代表),校验全部 17 行的 `name`、`value`、`source`、`purpose`;
  - 移除 9 个探针生成函数(`gen_error_text`、`gen_include_splices`、`gen_continuation`、`gen_const_beyond_int`、`gen_enum_pool`、`gen_signature_pool`、`gen_setjmp_expr`、`gen_struct_id`、`gen_struct_member`)中的硬编码阈值字面量,全部改由 `cap_vals[0..8]` 动态计算 `at-cap` 与 `past-cap` 输入;
  - `--list` 对全部 17 项输出 `CAP_FACT	<id>	<cap_name>	<value>	<source>`。
- 先红后绿与自检实测原行(在 10:00 负载提醒前已完成实跑):

| 检查项 | 实测结果原行 |
|---|---|
| `sh ./unisacc.com tools/k2-caps-gen.cx -- --write` | `K2_CAPS_GEN	WROTE	caps=17	aliases=5	out=./exec/facts/k2-caps-consts.tsv` |
| `sh ./unisacc.com tools/k2-caps-gen.cx -- --check` | `K2_CAPS_GEN	SAME	caps=17	aliases=5	out=./exec/facts/k2-caps-consts.tsv` |
| `sh ./unisacc.com tools/check-k2-caps-gen.cx` | `CHECK_K2_CAPS_GEN	assertions=8	failed=0` |
| `sh ./unisacc.com tools/check-cap-probe.cx` | `CHECK_CAP_PROBE	assertions=18	failed=0` |
| 先红(改表 `ERR_TEXT_MAX` `283 -> 280`,跑 `--list --caps mut`) | `PASS	mutating ERR_TEXT_MAX 283 -> 280 in table changes only that row in --list` |
| 先红(改表 `ERR_TEXT_MAX` `283 -> 280`,跑 `--only error-text-283 --caps mut`) | `PASS	mutating ERR_TEXT_MAX 283 -> 280 in table makes cap-probe error-text-283 turn RED`(`rc=1`,`CAP	error-text-283	FAIL	past-cap rc=1`) |
| 后绿(恢复原表 `exec/facts/k2-caps.tsv` 实跑 8 条 fast caps) | `PASS	real product: 8 fast caps MEASURED, none failed`(`CAP_PROBE	measured=8	unreachable=8	skipped=1	failed=0`) |
| `sh ./unisacc.com tools/reg-manifest.cx -- --check` | `REG_MANIFEST	manifests=735	bad=0	hard=0` |
| `sh ./unisacc.com tools/tsv-lint.cx -- --check` | `tsv-lint: ok: unresolved=10764 baseline=10764 ceiling=13260 errors=0` |
