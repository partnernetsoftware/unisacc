# CXM_MAXU 256 私有重放:证据(ccogweco)

补丁: research/ccogweco/cxm-maxu-256.patch(26 文件, 1885+, 977-)。
基线: v0.0.106 车头 f8c0de2157(S0 入车后)。最早一版是对旧车头 4258ae1f83 做的; 车头被 S0 推进后, 在 f8c0de2157 上重打并重算派生表, 现补丁 `git apply --check` 在 f8c0de2157 的干净 archive 树上通过(整份, 无排除项)。
做法: git archive 车头解到私有目录(不 clone, 不动 v0.0.106 工作树), 顺序重放 63ee420bd5 的 17 文件, 再按 plans/v0.0.106.md 第 4 项补 frontA 漏改, 最后派生表与接口表由工具重算。

## 先红(只重放 17 文件、未补漏改时, 私有树实测)

    FAIL	forward-original-128	status=5
    FAIL	forward-original-129	status=5
    MODULE_UNITS	failed=2

原因: cxmod 已是 256, 旧边界表仍期待 128/129 被拒, 实际 130 单元已放行。

## 后绿(补漏改后, 现行车头 f8c0de2157 的私有树上实测)

    units-127	cxmod=0	driveplan=0	count=127	PASS
    units-128	cxmod=0	driveplan=0	count=128	PASS
    units-129	cxmod=0	driveplan=0	count=129	PASS
    units-255	cxmod=0	driveplan=0	count=255	PASS
    units-256	cxmod=0	driveplan=0	count=256	PASS
    units-257	cxmod=1	driveplan=1	count=257	PASS(按预期拒)
    MODULE_UNITS_CHECK	status=PASS
    CXMOD_TRI	passed=46	failed=0	gaps=0   (CXM_MAXU-200-units rc0, CXM_MAXU-257-units rc1, cx0 与 prod 两路)
    HEADROOM	CXM_MAXU(units)	used=99	limit=256	pct=38.6	status=PASS
    CX0_EXTRACT	EXACT	output=src/cxmod.c	sha256=a971dd84d7b15322fcc43df6be76a3a6ad550125b6fd3f1a088b23772c57ed3c
    CX0_EXTRACT	EXACT	output=cx/compiler/cxmod.cx	sha256=63039b59aae6ccf6f2e853c6eae8642ed170749e87ece10f87b6846ba4b1acf2
    CHECK_LINK_CAP	failed=0   (两行 SKIP: 旧产品仍报 limit 128, 按要求 SKIP 不当 PASS)
    DERIVE_SUMMARY	ok=17	drift=0	refused=0

## 产品级仍红(旧产品未重建, 按要求留到不压载整链机器时跑)

    FAIL	cap-minus-1-modules-plan-rc0
    FAIL	cap-modules-refused-named
    CHECK_MAXU	failed=2	cap=256
    FAIL	size-255	build-refused: unisacc: model driver: too many sources (limit 128)
    UNIT_BOUNDARY	rows=9	pass=6	known-red=0	fail=3

这两项须新产品才转绿, 属计划 R1(129/200/256 单元 -o 全链, 含 osx/win)与大测, 现记 UNMEASURED。

## 补改清单(计划第 4 项)

- tools/check-maxu.cx: 上限改从 exec/facts/limits.tsv FRONT_UNIT_MAX 读; cap-1 模块 -cx-plan rc0, cap+1 拒且点名 "more than <cap> units"; 不再写死 127/128。
- tools/check-unit-limit.cx: 豁免串随 driveargs.cx 动态文案改为 "too many sources (limit %d)"。
- tests/module/units.cx 与 tests/module/boundary.tsv: 数组与行上限扩到 257, 边界行加 127/128/129/255/256/257。
- tests/coverage.tsv: interfaces.module-units 描述写明 FRONT_UNIT_MAX=256 与边界行。
- cx.compile 包号按现行车头现算 0.2.25 -> 0.2.26; tlib-packages、layer-api/cx.compile.tsv、iface-hash、doc-links 由工具重算; derive --check drift=0。

## 未动与说明

- d_members 与 ENTRY_LIMIT 对齐: 计划第 5 项写明 layoutfacts.tsv ENTRY_LIMIT=128 与库模式成员 512 是不同量, 由开版评审定, 本补丁不顺手改。
- R1 tape 序数预算(128)未验证; 129/200/256 的 -o 全链(含 osx/win whole-o)待新产品整链。
- cx-lint 对本补丁改动文件仅两条 R1-product 报告级("%s/unisacc.com" 填 argv), 与 HEAD 同类, 未新增。
