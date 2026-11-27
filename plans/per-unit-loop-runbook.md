# 单单元循环手册

一个窗口,一个单元,一次循环. 结果只标 LOCAL 或 PREPARED. 不标 PASS, 不标 QUALIFIED. 私有 APE 不是候选证据.

共同前置: `mkdir -p .work/tmp && export TMPDIR="$PWD/.work/tmp"`. 命令限 55 秒. 不写共享 main, 不写冻结路径.

## (1) 编对象

两条类共用:

```
sh ./unisacc.com -O2 -c -b lnx/x86_64 -funit -o "$TMPDIR/UNIT.o" SRC.cx
```

期望 `rc=0`. 出处: w-agy-h `.work/report-p3prime-step3.md` 对 `cx/driver/stream_s1.cx` 与 `src/drivestream.cx` 的 `FUNIT_COMPILE_OK`.

## (2) 磁带类 (agy-h M1)

重链加打包, 等 `tools/priv-relink.cx` 落地后只跑这一条, 不手写链接:

```
sh ./unisacc.com tools/priv-relink.cx -- \
  --chain-dir CHAIN_DIR --out "$TMPDIR/unit-priv.com" \
  --compile KEY.o=SRC.cx --baseline-layer-api DIR
```

未声明改动的对象, sig_sha 漂移, 外来 F 记录, 符号越界, 都是 `REFUSE PRIV_RELINK`, rc=2. 空改动时 entry.elf 与 APE 必须与基线逐字节相同. 工具头写明 `status=PREPARED scope=LOCAL`. 本分支还没有这只工具, 落地前停在对象步.

差分: 跑该单元的私有核对, 打包体内的包与声明的 body 逐字节同 sha. M1 样本是 w-agy-h `.work/m1-ape/m1_priv_verify.cx`. 对不上就停, 不进墙钟.

墙钟: 9 次, 排序后取第 5 个为中位, 起止读 `/proc/loadavg` 第一字段.

```
sh ./unisacc.com .work/m1-ape/m1_wall_bench.cx --
```

期望一行 `SUMMARY ... repeats=9 ... native_med_ms= ... priv_med_ms= ... load1_start= ... load1_end=`. 出处: `m1_wall_bench.cx` 第 104 到 131 行. 这只脚本路径写死在 agy-h 工作树, 换单元要改 WORK 与 PRIV_APE, 不把那次数字记成 PASS.

## (3) 表类 (agyneon m54, 提交 7d69626da, 6687fb2ab, 1f9838efd)

事实表在 `plans/UNIT-table-facts.tsv`. 生成器先查后写:

```
sh ./unisacc.com tools/table-gen-UNIT.cx -- --check
sh ./unisacc.com tools/table-gen-UNIT.cx -- --write
```

m54 的生成器拒绝同时给 `--write` 和 `--check` (`TABLE_GEN_M54 ERR`). 全枚举金样:

```
sh ./unisacc.com tools/check-table-UNIT.cx --
```

m54 读 `plans/m54-table-facts.tsv`, 读不到是 `REFUSE`, rc=2. 过了才加路由, 不手改生成表:

```
lnx/x86_64:MODULE -> cx/.../UNIT_tbl.cx
```

m54 是 `lnx/x86_64:compiler.cellname -> cx/compiler/cellname_tbl.cx`, 落在 `tools/cxlib/chainfacts.cx` 的 `cf_mt_*`, 再走 `tools/chain-facts-gen.cx` 与 `tools/entry-build.cx`. 磁带类已有的 `driver.stream_s1 -> cx/driver/stream_s1_m1.cx` 留在同一函数里, 不删.

## (4) 账本行

不自己改 `prd_mempalace.md`. 行由 ccogweco 的 `tools/ledger-add.cx` 追加, 六列: 日期, id, 文本, 提交, 证据路径, 角色.

```
sh ./unisacc.com tools/ledger-add.cx -- --add --id UNIT-LOOP --text "LOCAL PREPARED" \
  --commit SHA --evidence .work/report-UNIT.md --role WINDOW --ledger FILE
```

`FILE` 由 ccogweco 指定. 期望 `LEDGER_ADD`. 没有这行, 循环不算交.

## (5) 停手

对象编译失败, priv-relink rc=2, 金样 rc=2, 墙钟任一次 rc 非 0, 都停. 不换口径重跑第二次. 数字只进 `.work/` 报告, 不进发布说明.
