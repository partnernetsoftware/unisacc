# CXM_MAXU 256 私有重放:证据(ccogweco, 第二车 0.0.107 候选)

补丁: archive/research-2026-11-25/ccogweco/cxm-maxu-256.patch(29 文件)。基线: v0.0.106 车头在制作期间两次前移(4258ae1f83 -> f8c0de2157 -> 55d9badfac22), 本补丁在最新 55d9badfac22 的干净 archive 上 git apply --check 整份通过。
做法: git archive 车头到私有目录(不 clone, 不动 v0.0.106 工作树), 顺序重放 63ee420bd5 的 17 文件, 补 plans/v0.0.106.md 第 4 项漏改, 派生表与接口表由工具重算。
范围决定(按 cc G31 与代价测量): CXM_MAXU/DV_MAXU 128 -> 256 在前端与计划层落地; 链接层磁带序数预算 UNIT_MAX 128 不整体抬高(它触序数预算与模型网身份, 约 10 个消费者), 改名为具名容量拒绝, 留待后续。故本件在 -o 层仍是 128。

## 先红(只重放 17 文件、未补漏改, 私有树实测)

    FAIL	forward-original-128	status=5
    FAIL	forward-original-129	status=5
    MODULE_UNITS	failed=2

## 后绿(补漏改后)

    units-127	cxmod=0	driveplan=0	count=127	PASS
    units-128	cxmod=0	driveplan=0	count=128	PASS
    units-129	cxmod=0	driveplan=0	count=129	PASS
    units-255	cxmod=0	driveplan=0	count=255	PASS
    units-256	cxmod=0	driveplan=0	count=256	PASS
    units-257	cxmod=1	driveplan=1	count=257	PASS(按预期拒)
    MODULE_UNITS_CHECK	status=PASS
    CXMOD_TRI	passed=46	failed=0	gaps=0
    HEADROOM	CXM_MAXU(units)	used=99	limit=256	pct=38.6	status=PASS
    CX0_EXTRACT	EXACT	src/cxmod.c 与 cx/compiler/cxmod.cx 均逐字
    DERIVE_SUMMARY	ok=17	drift=0

## 链接层(保守方案, 本次新增)

tapelink 两处容量拒绝由 `reject: tape truncated` 改为 `reject: link unit ordinal bound exceeded (tokenlocations.UNIT_MAX=128)`; docs/tapebin-v1.md:46 同步。
tools/check-link-cap.cx: 上限读表; cap-1/cap+1 走实际 walk; 放行 cap+1 记 FAIL; 旧产品出现的 SKIP 一律非零退出(SKIP 不再可能被批次 rc0 记成 PASS); 新增 129 单元 -o 具名拒绝断言。
在装有旧产品的私有树实测(旧产品仍 128):

    SKIP	cap-minus-1-passes-walk	product-carries-an-older-cap
    SKIP	cap-plus-1-refused-named	product-carries-an-older-cap: unisacc: model driver: too many sources (limit 128)
    SKIP	unit-129-refused-by-name	product-carries-an-older-front-cap
    CHECK_LINK_CAP	failed=0	older-product     (rc=1)

变异证据(把私有树表内 FRONT_UNIT_MAX 临时改 64, 模拟"新产品放行 cap+1"):

    FAIL	cap-plus-1-refused-named
    FAIL	unit-129-refused-by-name	status=1 ... more than 128 units in one program
    CHECK_LINK_CAP	failed=2     (rc=1)

新产品(前端 256)上的三项绿与 129 单元具名拒绝: UNMEASURED, 须重建产品后实跑。

## 产品级仍红(旧产品未重建, 记 UNMEASURED)

    CHECK_MAXU	failed=2	cap=256
    UNIT_BOUNDARY	fail	size-255 build-refused: too many sources (limit 128)

## 补改与派生

- tools/check-maxu.cx 改为读表上限(cap-1 计划 rc0, cap+1 拒且点名 "more than <cap> units")。
- tools/check-unit-limit.cx 豁免串改 "too many sources (limit %d)"。
- tests/module/units.cx 与 boundary.tsv 扩到 257, 边界行 127/128/129/255/256/257。
- tests/coverage.tsv interfaces.module-units 描述写明 FRONT_UNIT_MAX=256。
- 包号按现行源现算: cx.plan 0.1.14, cx.compile 0.2.26; tlib-packages、layer-api/cx.compile.tsv、iface-hash、doc-links 由工具重算, derive --check drift=0。

## 未动与说明

- d_members 与 ENTRY_LIMIT 对齐: 计划第 5 项写明两者量不同, 本补丁不含该对齐(计划标题已改"不含")。
- cx-lint 仅两条与 HEAD 同类的 R1-product 报告级。

## 提交门清理(2026-11-24 第二轮,按 blocked=10 意见)

- 补丁只含源与自己的表行: 删除临时工具 tools/genunits-tmp.cx; 移除 4 张派生表(build/iface-hash.tsv、build/tlib-packages.tsv、build/layer-api/cx.compile.tsv、build/doc-links.tsv),包号改为落车时现取(本次在私有 v0.0.107 树现算到 cx.plan 0.1.14、cx.compile 0.2.25)。
- 棘轮: archive/research-2026-11-25/2026-11-23/own-flow-caps 下 7 个夹具源由 .cx 改为 .cxf(不再被 cxraw/cxlibc 扫描); src/driveargs.cx 的拒文案改用 cx_fmt 并保持 if 数与行数不变; tests/unit-boundary.cx 两处丢弃式 snprintf 改 cx_fmt。私有树棘轮结果 18 项 ok,唯一红项 dup-census 的加数在 base 文件(devsvc-core/prepush),不在本补丁路径。
- 私有 v0.0.107 archive(非工作树)自测: precommit-preview 对补丁路径 + 落车时现取的派生表 = blocked=2,两条都来自 v0.0.107 车头自带的重复 coverage 行(c99.extern-array-incomplete 在车头出现 2 次;git show v0.0.107:tests/coverage.tsv 计数为 2),与补丁无关;补丁路径本身 0 条阻塞。
- 补丁规模: 24 文件(17 重放 + 5 漏改 + 2 新增行),在 v0.0.107 车头 ad5c3e72 的干净 archive 上整份 git apply --check 通过。
