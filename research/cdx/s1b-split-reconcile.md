# S1b 拆分断言对账

`bc3070168` 只有 S1a 实现，提交树没有拆前 `tools/check-proc-own.cx` 和 TSV。拆前证据来自独立复跑的私有固定输入。夹具 SHA-256 为 `cf4c39d934fc88b8f95f373f77608540091b08b2fad7fe497f49835d4228b388`，实跑日志 `/tmp/cdx-s1-gen.log` SHA-256 为 `717c8053ec6f45526a3ccb8f280eb7cc913c7d9ef811c01efd2c7c31e0bdd871`，原行为 `CHECK_PROC_OWN	failed=0`，共 31 条 PASS。其中文件内第 65 次回收后旧句柄仍返回退出码 7 的覆盖由一条旧断言名代表。新增代次断言按各自名字单独对账。

以下命令在 S1b-2、S1b-3 提交后执行。将 `REV` 固定为同时包含三件夹具的最终提交，保存各自原始返回码与日志。拆前证据目录消失时，可以用同一来源的夹具与 TSV 在 `bc3070168` 归档树复跑，先核对夹具 SHA-256。

```sh
repo="$PWD"
REV="${REV:?先将 REV 设为包含三件夹具的最终提交}"
old=/tmp/cdx-s1-gen.log
snap=$(mktemp -d /tmp/cdx-s1b-reconcile.XXXXXX)
sha256sum "$old"
git archive "$REV" | tar -x -C "$snap"
ln -s "$repo/unisacc.com" "$snap/unisacc.com"
cd "$snap"

timeout 25s sh ./unisacc.com tools/check-proc-own-gen.cx -- > gen.log 2>&1
gen_rc=$?
timeout 25s sh ./unisacc.com tools/check-proc-own.cx -- > core.log 2>&1
core_rc=$?
timeout 25s sh ./unisacc.com tools/check-proc-own-sat.cx -- > sat.log 2>&1
sat_rc=$?
printf 'GEN_RC=%s CORE_RC=%s SAT_RC=%s\n' "$gen_rc" "$core_rc" "$sat_rc"

rg '^PASS\t' "$old" | cut -f2 | LC_ALL=C sort > old.names
rg '^PASS\t' gen.log | cut -f2 | LC_ALL=C sort > gen.names
rg '^PASS\t' core.log | cut -f2 | LC_ALL=C sort > core.names
rg '^PASS\t' sat.log | cut -f2 | LC_ALL=C sort > sat.names
LC_ALL=C sort -u gen.names core.names sat.names > new.names
wc -l old.names gen.names core.names sat.names new.names
LC_ALL=C comm -23 old.names new.names > missing.names
LC_ALL=C comm -13 old.names new.names > added.names
LC_ALL=C sort gen.names core.names sat.names | uniq -d > duplicate.names
rg -n '^(CHECK_|FAIL\t)|PROC_RESIDUE' gen.log core.log sat.log
cat missing.names added.names duplicate.names
test "$gen_rc" -eq 0 && test "$core_rc" -eq 0 && test "$sat_rc" -eq 0 &&
  rg -q '^CHECK_' gen.log && rg -q '^CHECK_' core.log && rg -q '^CHECK_' sat.log &&
  ! rg -q '^FAIL\t|PROC_RESIDUE' gen.log core.log sat.log &&
  test "$(wc -l < old.names)" -eq 31 && test ! -s missing.names
```

通过须三条命令均退出 0，三个检查实际给出各自汇总，无 FAIL 或 PROC_RESIDUE，并且 `missing.names` 为空。缺项逐条从拆前日志贴原行。若原断言由更强的表驱动夹具接替，保留原名对账仍标缺，由所有者裁定替代是否接受。对名字相同的回绕、饱和与释放关键断言另核谓词，名称相等只能证明登记口径一致。

## 草稿对账

固定 `e90e9337e` 并叠入核心草稿 SHA-256 `9aa6b5e7f8e84d5c66aa941b764a2b3bd84e79096bf909b11581b11c29695d94`。代次检查 10 条通过。核心在 24 秒窗口内仅输出前 8 条，放进 55 秒取证窗口后输出 21 条，通过汇总为 `CHECK_PROC_OWN	failed=0`。耗时瓶颈是 LOST 测例等待 `sleep 30` 自然结束。

这两件当前合并有 31 个不同名字，拆前 31 个名字中仍有 9 个不在合并结果。饱和草稿尚无固定提交。

```text
I1-65th-refused-eagain
I1-cleanup-all-zero
I1-fill-64-slots
I1-reap-one-frees-slot
I1-respawn-takes-freed-slot
I2-end-timeout-abandoned-drained
I2-end-timeout-keeps-slot
I2-end-timeout-spawn
TSV-table-rows-all-checked
```

## 固定提交复跑

以 `43b11e5ce1` 归档树实跑，gen/core/sat 各自 10/21/8 条 PASS，三个退出码均为 0，无 `PROC_RESIDUE`。39 个名字无重复，拆前 31 个名字中保留 30 个，唯一缺名为 `TSV-table-rows-all-checked`。其对应的替代批次由 `d51481c6b` 登记，私有索引资格 20.126 秒，90 行中 63 条通过、27 条具名 `UNMEASURED`。按原名严格要求，三个拆分检查的并集仍缺一项。
