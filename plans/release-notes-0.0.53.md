# unisacc 0.0.53 -- 发布说明(未实跑一律写未跑)

发布提交 X `b80e5f4bdeb0e6aaa884dfd63e4e2fdf16d1d00f`,源提交 `7f2f32cfded04d683246473b43eef92ea0bb4df5`,候选记录 `unisacc.com.build.json 7c9ce95d7cd5e79a0525d603288d4c57afbe89976c9075e2d99579bd928dd617`(内含 commit 7f2f32cfde、sources_sha256 4001d3e2)。git 范围:`v0.0.52..b80e5f4bde`,共 18 个提交(下文逐条概括)。

冻结过程:首次冻结于 `bb59b4fcad`,冻结后发现 c7p2e 合并时误删了 s9-shards 片工作进程分派行,所有片 REFUSE usage;修复提交 `7f2f32cfde` 后重新冻结,构造源即 `7f2f32cfde`。

## 是什么

面向 C99 的编译器,自己写出 ELF、Mach-O、PE,可编译自身;产品形态为单文件 `unisacc.com`。0.0.53 是构造链与工具修复版:产品输入相对 0.0.52 零改动。上一发布版见 `archive/docs-2026-11/plans/release-notes-0.0.52.md`。

## 身份与证据

| 对象 | 身份 | 说明 |
|---|---|---|
| APE `unisacc.com` | 0842748d9663fbf97329d7ee4bfeeea0dceac2ed2a0e41d06c9290ef6985f6e2,6529476 字节 | `--version` 应输出 `unisacc 0.0.53`;APE 字节因版本槽注入 0.0.53 而与 0.0.52 不同 |
| 源 SID(sources_sha256) | 4001d3e26a177be391571b9341665af1c67af1dc27c121069396fb29a13652fd | 现算,与 0.0.52 相同 |
| 链产物 | 六片、kernel、P4 包均复用自 0.0.52 构造(构造源 `23e6a567dd`) | 本版未重跑两代不动点;不动点结论沿用 0.0.52 记录,不在本版重新声称 |
| 六片 | slice-win-arm64 3b2d34a203faf8e1f4446f02b4feab3806c1cba36ed78c57575a92e2d66970c6;其余各片见候选记录 unisacc.com.build.json 7c9ce95d7cd5e79a0525d603288d4c57afbe89976c9075e2d99579bd928dd617 | 同 0.0.52 复用链产物 |
| P4 包 | 前缀 56efea53 | 与 0.0.52 相同 |
| 其余资产 | zip 3f0643d58a6231a5bae61e219e80893fe6a5f9d826ee5658f615ffaa99e78080,SHA256SUMS e7bc35f5501877cf5bd22e06286b4eede0629f24fe53e7b84a47e8d67a41d418 | 全文见候选记录 |
| s9 | commit 档(候选 APE 0842748d,私有根 7f2f32cfde)selected=128,124 PASS,FAIL 2(host-cell-binding:显式 fixture 带 sidecar,疑克隆残留未核;s9-nightly-run:4 项内部假跑断言在 s9 片环境失败、过门时绿,原因未查清),UNKNOWN 2(real-input-check、s9-shards-inputs 并行 wave 下 55s watchdog 超时),qualification NOT_FULL;daily/release 档未跑 | 见下节 |
| 原生 CI | 同候选运行 42914663896(检出提交 X b80e5f4bde,ape_sha256 0842748d):QUALIFIED pass=42 fail=0 not_run=0 invalid=0 | 六目标原生单元格实跑;不含 s9 daily/release 档 |

## 行为变化(升级前请读)

产品输入零改动:编译器、运行库、驱动的语义相对 0.0.52 不变。本版变更全部在构造链与工具侧。APE 字节不同仅来自版本槽。

## 本版已落地(git log `v0.0.52..b80e5f4bde`)

- `5f3ff057dc` 0.0.52 收尾:本机根 install-ape 换 0.0.52,根身份记录与 ape-receipt 随之更新;账本 REL052-PUBLISHED。
- `39b1874170` 账本补回被 commit-own 键前缀误匹配删除的 REL052 行。
- `62db4ddef0` candidate-chain pack 相位切片取址修复(0.0.52 MANUAL 5):osx 取 `chain_work/gen1/<t>-b2/image`,win 取 `chain_work/gen1/<t>-image/image`,与 g 相位共用 pack_slice;旧位置不再接受;新增 `--slice-path` 夹具入口与 4 负例。pack 相位本身未实跑。
- `05ea576fa8` C2p1:新增 cxlib.modroot(模块根种类库,KIND:PATH),缺参/坏种类/相对路径具名拒;check-modroot;不改链与 src(C2p2 驱动 `--modroot` 属产品输入,另候选)。identity 批次缺材料 DEFERRED。
- `730caa0b43` C4p1:新增 cxlib.relsha,发布用提交号须 40 位小写 hex,短 sha 拒;release delete 与 tag -d 拒,edit --tag/--target 放行;check-relsha。
- `ea658459c4` C4p2+C4p3:新增 `tools/release-guard.cx` 与 check-release-guard(NEXT 行 `--target`/`commit=` 须 40 位);release-run NEXT 的 workflow commit 改用发布提交 X;负例 ci-x-eq-freeze-refuse;补登 tools-compile-091(92 片)。
- `d4c9779a9a` release-run 批次材料登记:materials 改为 release-fixture,release-candidate,tier 由 commit 改 daily(该批次在 commit 档自 0.0.51 起从未真跑过);缺材料时 DEFERRED needs-materials;有材料真实运行未验证(无 0.0.49 fixture)。
- `b4c934cc4e` pp_copy_product 复制产品时一并复制 `ROOT/unisacc.com.build.json`(若存在);此前集成门克隆只带 APE,读 build record 的批次在门内必红。
- `fe909d26f1` commit-own `--keyed` 精确字段匹配(own-diff 新增 `--field`);三处无上限子进程调用换 `cx_runv_timeout`。
- `9d0bb49011` whole-o-deadline 判据修复:affected 为 subreaper 时被杀孙进程成僵尸,原判据把僵尸当存活;改为 wd_alive(存在且非 Z)并扫描进程组,断言与 30x20ms 上限不变;affected 实跑三次含满载 PASS。
- `da0bbe80f3` integrate-patch phase 5 可续跑:记录已得结论批次,头部绑定 base/patch/product/tree,任一变即作废;修 `--until-done` 下 rc=3 误退出;新批次 integrate-patch-affected(约 42s)。
- `0289b3a111` 候选链残留修复合集 resid053d:(1) release-build 快照前建 out/(M1);(2) ctx 相位未 DONE 残留在 resume 前清理(M2);(3) 作业有 record 无 finish 时清 finish-job outer(M3);(4) routes 未 DONE 时清 asm-source OUTPUT.s/kernel-route/routes(M4);check-no-out-snap 取路径不含 out/ 字面量;含 kill-resume 负例。
- `33711017d8` s9-nightly step 预算与出口修复:`--step-s` 默认 290,step 子进程带同钟 deadline,s9-shards step 可在预算内收尾(原 55s 必被杀 rc=99);出口改为本步无新发片才转 aggregate;UNKNOWN 片 retry 一次。真跑 3 步 252.7s/87.3s/12.7s,29 片全完成,aggregate FAIL 7(release-run 旧基仍在 commit 档、s9-nightly-run watchdog 超时,余 5 未具名)。
- `a5f6656780` 0.0.53 B 段 M1-M4 条目登记到 `agentwork/items.tsv`(仅条目与 gate 命令,不含实现)。
- `74a741ba72` s9-nightly-run 运行目录改为每次新目录且不预建(原预建使 s9 init 必 REFUSE,实测 65s FAIL step=init);目录已存在则拒。
- `bb59b4fcad` C7 s9 release 档与夜报修复(c7p2e):s9-shards init 支持 `--tier release`(27 条),s9-nightly 透传 tier;FAIL 输出补 report=;check-s9-nightly-run 测被测工作树版本。该提交合并时误删 s9-shards 片工作进程分派行(见下一条)。
- `7f2f32cfde` s9-shards 回归修复:补回片工作进程分派 `if(is_child){...child(argv[4],argv[5]);return 2;}`(置于 list-tier 分支之前);此前候选 s9 提交档 128/128 failed_or_unknown。门内 s9 检查均用假 s9 夹具,未覆盖真实工作进程,真实冒烟检查另补。本版构造源即此提交。
- `b80e5f4bde` 提交 X:`release/candidate.tsv` 记候选 v0.0.53(七资产 sha 由 release-run --pack 现算回填;ci_run_id=0 待 CI)。

## s9

- 夜跑工具修复后首份真实夜报(`74a741ba72`,commit 档;是冻结前 main 上的手动夜跑,所用 s9 工具尚未含 c7p2e,不是本候选的 s9 结果):3 step 266/179/13s;aggregate FAIL。
  - host-cell-binding:真实用例失败(显式 fixture 带 sidecar,疑夜跑克隆残留,未核)。
  - s9-shards-inputs:并行 wave 下 55s 超时,状态 UNKNOWN。
- 0.0.53 候选 s9 结果:commit 档(候选 APE 0842748d,私有根 7f2f32cfde)selected=128,124 PASS,FAIL 2(host-cell-binding:显式 fixture 带 sidecar,疑克隆残留未核;s9-nightly-run:4 项内部假跑断言在 s9 片环境失败、过门时绿,原因未查清),UNKNOWN 2(real-input-check、s9-shards-inputs 并行 wave 下 55s watchdog 超时),qualification NOT_FULL;daily/release 档未跑。
- daily/release 档:未跑。不称 QUALIFIED。

## 已知限制与未声称的性质

- A1 一条命令:0.0.53 修了 M1-M5(out/、ctx、finish-job、routes、pack 切片路径),但修后整链未从干净状态重跑验证;不称一条命令过链。
- stages 自 0.0.51 起红 13 项,未修。
- e3 空 body 根因已静态定位,修复未入(顺延 0.0.54)。
- why 补丁未入。
- g2 拆分未入(g2 余量约 3s);p3 拆分未入。
- yb-all(执行器检查点)顺延 0.0.54。
- release-run 批次由 commit 档改 daily,需材料;有材料真实运行未验证。
- c3p2/c3p3 收据库未入。
- check-s9-shards 改真根源码缺陷未修。
- M1-M4 模型层仍未开工(本版仅登记条目)。
- 本版未重跑两代不动点;链产物复用 0.0.52。

## 未跑范围

整链从干净状态重跑;两代不动点;pack 相位实跑;s9 daily/release 档;release-run 有材料真跑;stages 修复;冒烟与复现命令。CI 以运行 42914663896(QUALIFIED 42/0)为准;候选 s9 以上节 commit 档结果为准,daily/release 档未跑。

## 验证入口

1. 核哈希:`sha256sum unisacc.com` 应为 0842748d9663fbf97329d7ee4bfeeea0dceac2ed2a0e41d06c9290ef6985f6e2(6529476 字节)。
2. `sh ./unisacc.com --version` 应输出 `unisacc 0.0.53`:打包产物与草稿回读均实测输出 `unisacc 0.0.53`。
3. 冒烟:未跑。
4. 复现:`release-run --pack`(冻结 rev 7f2f32cfde,SRC_DIR 复用 0.0.52 链产物)实跑至 ape-pack/inject/layout-gen/backfill/sums 全部 DONE,SOURCES_SID VERIFIED;最后 commands 步因需 RELEASE_X_COMMIT 而停(按 c4p3 设计)。草稿回读 `sha256sum --check SHA256SUMS` 6/6 OK。
5. 原生 CI:https://github.com/partnernetsoftware/unisacc/actions/runs/42914663896(QUALIFIED pass=42 fail=0)。
