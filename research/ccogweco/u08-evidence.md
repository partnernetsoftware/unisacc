# U08 取证页:tools/slice-migration-gen.cx 的 source-missing 分支(预备给 agyneon 落)

登记行(build/state-unverified.tsv:U08):kind=stale-claim,主题 slice-migration-gen,缺"source-missing 是否仍在"。

## 代码事实(命令原行)

    $ nl -ba tools/slice-migration-gen.cx | sed -n '86,92p'
        86	        || cpy(x->over, STR, f[7], len[7]) || cpy(x->note, STR, f[8], len[8])) return fail("fact-col", key);
        87	    x->lines = (int)num(f[4], len[4]); x->heap = (int)num(f[5], len[5]); x->rec = (int)num(f[6], len[6]);
        88	    if (x->lines < 0 || x->heap < 0 || x->rec < 0) return fail("fact-num", key);
        89	    if (!tx_eqz(x->over, "-")) { for (i = 0; i < 4 && !tx_eqz(x->over, DEST[i]); i++) {} if (i == 4) return fail("fact-override", key); }
        90	    if (sy_access(x->src, 0) != 0) return fail("source-missing", x->src);
        91	    nfact++;
        92	    return 0;

结论:分支仍在(:90),语义是"facts 表引用的源文件不存在时以 source-missing 具名失败"。

## 覆盖缺口

- 该驱动的批次 `slice-migration-gen`(tests/batches.tsv:533,入口 tools/check-slice-migration-gen.cx,daily,rc0:CHECK_SLICE_MIGRATION_GEN)只测两件事:`--check` 的 81 单元与文档五节顺序,以及无参数 usage rc2。
- 检查源码没有 source-missing 的构造或断言:`grep -n 'source-missing' tools/check-slice-migration-gen.cx` 0 命中。
- 因此 U08 的"仍在"只有源码行证据,没有行为证据;按"未实跑不记 PASS"不能据此关闭。

## 两种关闭路径(建议给 agyneon 选)

A(最小,推荐):在 tools/check-slice-migration-gen.cx 加一个负控:把 plans/slice-units.tsv 复制到私有目录,把某一行 src 改成不存在的路径,跑 `slice-migration-gen --check`,断言 rc!=0 且 stderr 含 `source-missing`;批次断言数 +1,原断言不动。落时只改检查文件与必要私有夹具,不改生成器与 plans 真表。
B(若不想动检查):由 agyneon 用一次性私有树实跑一次缺失源场景,把 rc 与 stderr 原文作为证据行写进 U08 的 evidence 列;但这不产生回归守卫。

## 落库形态(给 agyneon)

- 若选 A:补丁只碰 tools/check-slice-migration-gen.cx(夹具字符串可在检查内生成,无需新文件);批次行 expect 不变(仍 rc0:CHECK_SLICE_MIGRATION_GEN);提交说明写"加 source-missing 负控,关闭 U08 声称"。
- 关闭 U08:在 build/state-unverified.tsv 的 U08 行改 state(按表内既有约定),evidence 列写 `tools/slice-migration-gen.cx:90;tools/check-slice-migration-gen.cx:<新断言名>`;不要改 kind。
