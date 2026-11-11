# c43 seedmemory 重钥：LLP64 三刀派生身份级联（2026-10-17）

## 读集与键

LLP64 三刀改变 classic 源；派生补交更新 tests/abi.manifest 以及 exec/facts/k2-gen2.tsv、exec/facts/k2-libraryenv.tsv。两张事实表仅输入 SHA 注释级联更新，payload 逐字节不变，详见本笔事实表归因记录。实测构造读集仍为 gen 779、parse2 380 个文件；文件内容变化导致参考键变化，不能旧 RSS 重键。

| 族 | 旧键 | 新键 |
|---|---|---|
| gen | `fe4a57ac88dbf7cc` | `4166585d2c3d745f` |
| parse2 | `aedee163340f633e` | `877e2506839a3f13` |

## 逐路线 C RSS 对比

单位：C_RSS 为 MiB（ru_maxrss KiB 向下取整，消费方保留原 +1 MiB 与余量）。x/e3、errors/errorparse 两对共享 C_RSS 槽，写回值取两族实测最大值；本路线原始峰单列。

| 族/路线 | 参数 | 原 C_RSS MiB | 新 C_RSS MiB | 本路线峰 KiB | 结论 |
|---|---|---:|---:|---:|---|
| gen/arm-elf | `enc/arm --elf` | 142 | 142 | 145740 | 不变 |
| gen/arm-macho | `enc/arm --macho` | 208 | 208 | 213708 | 不变 |
| gen/arm-pe | `enc/arm --pe` | 264 | 264 | 270636 | 不变 |
| gen/e1 | `lex --typed` | 330 | 330 | 338696 | 不变 |
| gen/e2 | `pp --shared-predefines` | 186 | 186 | 191020 | 不变 |
| gen/e3 | `parse2` | 1267 | 1268 | 1298800 | 实采更新 |
| gen/e4 | `opt --o2` | 40 | 40 | 41260 | 不变 |
| gen/enc-elf | `enc --elf` | 142 | 142 | 145408 | 不变 |
| gen/enc-macho | `enc --macho` | 208 | 208 | 213656 | 不变 |
| gen/enc-pe | `enc --pe` | 263 | 263 | 270228 | 不变 |
| gen/errorparse | `parse2 --errors` | 2134 | 2134 | 2185704 | 不变 |
| gen/lower-lnx-a | `lower --full --arm64` | 266 | 266 | 272908 | 不变 |
| gen/lower-lnx-x | `lower --full` | 241 | 242 | 247912 | 实采更新 |
| gen/lower-osx-a | `lower --full --osx --arm64` | 262 | 262 | 268328 | 不变 |
| gen/lower-osx-x | `lower --full --osx` | 235 | 235 | 240932 | 不变 |
| gen/lower-win-a | `lower --full --win --arm64` | 207 | 207 | 212688 | 不变 |
| gen/lower-win-x | `lower --full --win` | 180 | 180 | 184536 | 不变 |
| gen/nativeabi | `nativeabi` | 298 | 298 | 305600 | 不变 |
| gen/o1 | `opt` | 14 | 14 | 15012 | 不变 |
| gen/obj-enc-a | `enc/arm --object` | 159 | 159 | 163504 | 不变 |
| gen/obj-enc-x | `enc --object` | 156 | 156 | 160280 | 不变 |
| gen/obj-lower-a | `lower --full --object --arm64` | 270 | 270 | 276708 | 不变 |
| gen/obj-lower-x | `lower --full --object` | 245 | 245 | 251888 | 不变 |
| gen/prune | `prune` | 32 | 32 | 33696 | 不变 |
| gen/tokenlex | `lex` | 119 | 119 | 122544 | 不变 |
| gen/tokenpp | `pp --shared-predefines --no-autoinc` | 39 | 39 | 40272 | 不变 |
| gen/warnlex | `lex --locations` | 742 | 741 | 759696 | 实采更新 |
| gen/warnparse | `parse2 --warnings --errors` | 2148 | 2148 | 2199952 | 不变 |
| gen/warnpp | `pp --locations --shared-predefines` | 182 | 182 | 187136 | 不变 |
| gen/warnunits | `parse2/units --locations` | 196 | 196 | 201536 | 不变 |
| parse2/errors | `parse2 --errors` | 2134 | 2134 | 2185544 | 不变 |
| parse2/locations | `parse2 --locations` | 1346 | 1346 | 1378700 | 不变 |
| parse2/locations,errors | `parse2 --locations --errors` | 2134 | 2134 | 2185744 | 不变 |
| parse2/locations,warnings | `parse2 --locations --warnings` | 1360 | 1360 | 1392968 | 不变 |
| parse2/locations,warnings,errors | `parse2 --locations --warnings --errors` | 2148 | 2149 | 2200624 | 实采更新 |
| parse2/warnings | `parse2 --warnings` | 1360 | 1360 | 1393004 | 不变 |
| parse2/warnings,errors | `parse2 --warnings --errors` | 2148 | 2148 | 2200524 | 不变 |
| parse2/x | `parse2` | 1267 | 1268 | 1298612 | 实采更新 |

## 测量方法与身份

使用仓内 tests/seedmemoryevidence.py 原生产者；每条路线依次跑 C trace、Python trace、C RSS，单机串行。38 条路线共 114 项成功观察；原生产者汇总核每路线三份模型 SHA 一致、二进制/源码身份和真实读文件摘要一致。C RSS 由原 os.wait4 生产者取 ru_maxrss，C 构造器由原配方构建，SHA `96cf5ebbc66e5f2092cc5b68c4bf29695c8c7fdb0d110c15c9e2f12a23ec7fc7`，seed/gen.c SHA `c1be454cf86c7d4c01af985d437c21116e5f69b880bd763b370a1461ae426187`。准备外 bound55、原内部 bound45 不改。

测量源树为 39b33cd641c8f402233081c581b66c0fa75952ce；测量 source_hash `4d19a3485c44af0b6fb758d5876c8147ac0fb6971c4819bf27e1c6e4f0a643e4`。消费常量和本摘要在测量完成后写回，实际构造读集与最终树逐文件同字节，不倒填测量身份。旧18项用统一保守阈值；后89个原成功项及本次7项使用 route-assess 现场 available 和原余量。Python cold、COM、并行内存仍 UNKNOWN。

## 第108步历史与 rerun

原 parse2 locations,errors C RSS 在外窗 52.035 秒 rc142，起跑 available=3140616192 B、requirement=2798387200 B，原件保留，不改绿。Owner 裁为当时宿主并发（load 约4.6，cc2 prepare 同时跑，随后停止）。单次背靠背 A/B 对照：旧基 bff95d1f 9.230 秒、新树 6.722 秒，均 rc0；支持并发判断，不声称证明换页。

安静宿主按原命令重采第108步，rerun rc0/6.117 秒，起跑 load1=1.223、available=3934437376 B；其后6项均 rc0。原107条成功观察只在新汇总副本重定位 model_path，模型字节不变，逐条新旧 SHA 见 RELOCATION.json；原失败和旧索引未改。

## 仓外原件

原始 JSON 不入库（.gitignore 第78–82行）。仓外索引收据：`/home/box/c43/test-design-0043/llp64-seed-preflight/r2/seedmemory-evidence.json`，SHA256 `c477b00c08e74f2fef305f39ddca28df2d538fe290f0ee712be670df3ece829a`。此 JSON 绑定114项测量 record SHA、模型 SHA、源身份及1159条读文件 SHA；原 trace/RSS/result 保留于同目录 rerun-final-observations/，旧142与A/B在其父目录历史中。未冒称 Windows、Darwin、产品全清单或收口已通过。
