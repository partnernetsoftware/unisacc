# 验证(基线 a95623fb29 + 本补丁的私有 git 树实测)

    DOC_LINKS_GEN	rows=5248	inline=692	tick=4556	bare=0	broken=81	baseline=80	anchor_miss=1	orphans=111	result=FAIL
    no-home-path: archive/research-2026-11-25/2026-11-24/R31-plan24h.md has 1 lines, baseline 0
    no-home-path: archive/research-2026-11-25/omp-sonnet/handoff.md has 1 lines, baseline 0
    RATCHETS	ok=17	fail=2

对照:同一基线不加补丁时 doc-links 也是 rows=5248 tick=4556 broken=81 result=FAIL;no-home-path 两条同样存在。结论:本补丁对两项检查零新增(doc-links 行数与命中数不变,broken 仍为基线既有的 81;no-home-path 无本补丁新增行);两项整体红是基线既有问题,不由本补丁引入。
