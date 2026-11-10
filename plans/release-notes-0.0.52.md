# unisacc 0.0.52 -- 发布说明(未实跑一律写未跑)

候选构造源 `23e6a567dd`,候选记录提交 `90f5144a31`。下面记已进 main 的构造体系事实,以及明确未完成的缺口。git 范围:合入提交自 `b751e8f51e` 至 `90f5144a31`(origin/main)。

## 是什么

面向 C99 的编译器,自己写出 ELF、Mach-O、PE,可编译自身;产品形态为单文件 `unisacc.com`。当前已发布仍是 0.0.51(见 `plans/release-notes-0.0.51.md`)。

## 身份与证据

| 对象 | 身份 | 说明 |
|---|---|---|
| APE `unisacc.com` | 1bffabbaf45ee3495ab5a750e8f3d9918ccf2a6cf2c5d57147baff45ff55ee19,6529460 字节 | `--version` 输出 `unisacc 0.0.52` |
| 构造源 | 23e6a567dd3ab5e481dbe3c98491bc87dd2a3b95,源 SID(sources_sha256)4001d3e26a177be391571b9341665af1c67af1dc27c121069396fb29a13652fd | 候选记录于提交 X `90f5144a31beabf773994d4f25420252e5aaeaf8`(release/candidate.tsv) |
| 六片 | sha 前缀:lnx-x86_64 e6f40686、lnx-arm64 accc75b0、osx-x86_64 3be1542c、osx-arm64 345aebd4、win-x86_64 e2016465、win-arm64 749617b4;slice-win-arm64 a4f71b32...(3485184 字节) | 其余资产:build.json 97ace00e...,zip 988eb956...,SHA256SUMS af342391...(均为前缀,全文见 release/candidate.tsv) |
| 不动点 | compare:objects=same 85、kernels=same 2、slices=same 6、product=same | 已实跑 |
| P4 包 | pkg.bin 56efea531e104b4114b314d5bdb4e5efc2a6debb96f974fa5d6e32675a637db8,1763636 字节 | 与 0.0.51 相同 |
| s9 | commit 档手工逐步实跑 129 批,128 PASS、1 FAIL(release-run 缺 RELEASE_RUN_FIXTURE 材料,harness 缺口);qualification NOT_FULL;daily/release 档未跑 | 不称全批通过;A7 夜跑工具真实调用必败(55s 调 300s step),夜报未跑 |
| 原生 CI | 同候选运行 42881292964(检出提交 X 90f5144a31,ape_sha256 1bffabba):QUALIFIED pass=42 fail=0 not_run=0 invalid=0 | 六目标原生单元格实跑;不含 s9 daily/release 档 |

## 行为变化(升级前请读)

`23e6a567dd` A8 步0 是本版**唯一已合入的产品输入**(src/drive.cx、driveunits.cx 行数回收,Y0-SRCSHAPE-EXEMPT 撤销)。候选须重建并核身份。why 补丁未进。其余合入多为工具/链/表,不在此声称产品语义。

## 本版已落地的构造体系(git log `b751e8f51e`..`23e6a567dd`)

- **基线修复**:`b751e8f51e` tools-compile-sources 补登 13 个新工具、tools-compile-089 分片;部分分片单跑 PASS,其余未重跑;runner-platform 与 osx-* 需另一原生宿主、tools-default-regression 缺材料,均未记 PASS。
- **N3 提交器**:`feac30a7cf` `tools/submit-patch.cx`;`1fa6c22395` 新 `.cx` 必须登 tools-compile-sources 并核 `--shards`。
- **A0 pm-split**:`6614c48421` 默认 product-map 只读检查 3.28s;4 个 daily 夹具批次,prd-red 冷 38.72s、批次上限 55。
- **A3 chain_sha 收窄**:`e13eedcf4e` 收据 v4 增 product_sha,chain_sha 降为诊断;receipt-upgrade v3->v4;门 fixedpoint-sample/-v4 实跑(提交说明)。『改编排后 --resume 仍可续』待链演练,未记 PASS。
- **A7 s9 夜跑工具**:`44b6461455` `s9-nightly.cx`;`1ab7aa41fb` 档/截断/wall_ms;`567432bcb7` `s9-nightly-run.cx` 与 systemd 模板(未安装);`629511ef11` 截断负例。夜报首跑结果待填,三档未跑,不称 QUALIFIED。
- **A8 步0**:`23e6a567dd` srcshape 5854,豁免撤销,coreapi new=0,实编 drive/driveunits/driveroute rc=0。why 未进。
- **A1 前置(非整包增量)**:`6d46366dad` 失败相位清独占;`c3aad71f45` P1 Mac/Win 相位进链;`deb6304b7b` P2+P2b prepare,plan phases=35;`78e5d3699a` chainfix(残留清理、完成判据、空 body 拒绝)。G2 增量调度(A1 本体)未做。
- **integrate-patch**:`05a154977e` la4;`9d30ff6b3d` cxlib `-c`;`38672dbbc4` DEFERRED needs-materials。
- **pins**:`4e4ebe33c0` 行号与补登,pins 与 sourcepackage-pins 提交说明记 PASS;`60b9eea8e0` pins 批次 args 分号路径。
- **账本 72h**:`0b5b21a4a9` 记 A1/A3/A7 未全过、B 段顺延 0.0.53、不阻发布。裁决正文仍由 cc 填下节占位。
- **M0 对照**:`44b6461455` research/m0-0052(逻辑分布 14.9s、包 1763620B、42 网 1.45s;网输入取自 c49 产物仅对照)。
- 此前已在 main:`08f65bc124` N1;`39f59c7dee` 顺序域;`2f9b4b79a5` A1 缺口 2/3/6;`08dd41cee5` N6;`1f85946110` A0 阶段一;`8f43bdfe1e` newcx-check;`4b919b20ea` 门预算。

## 已知限制与未声称的性质

- **g2 余量约 3s 或拆分落地视结果**:未在本草稿记 PASS;以链实跑为准。
- **p3 42.9s**:贴线/余量未在本范围用全链重测关闭;warmup(`2f9b4b79a5`)后本体是否仍贴线未跑完整发布资格。
- **e3 空 body 根因未明**:`78e5d3699a` 增加非空拒绝与 `out/sub-e3-span.md`;根因未关闭。
- **why 补丁未进**:不与 A8 同候选(srcshape 叠 why 红)。
- **A1 增量未做**:G2 增量 <=5 分钟实测未做。
- **A1『一条命令』不成立**:候选链 MANUAL 1-4(out/ 父目录、ctx 残留、jobs finish-job outer、routes OUTPUT.s)与自动挪残留次数;不能写成一条命令过链。
- **A2 未做**:单元前端缓存未做。
- **M1-M4 顺延 0.0.53**。
- **夜报首跑结果待填**:工具在库,systemd 未装,三档未跑。
- **s9 未全跑**:commit 档 128/129,daily/release 档未跑,qualification NOT_FULL,不称 QUALIFIED。
- **原生 CI**:运行 42881292964 结果待填;六片/不动点/P4/APE 身份见上表。
- **affected-run 间歇 transport=-1**:根因未关闭。
- **stages 0.0.51 起即红 13 项**:10 正向输出不符、3 拒绝合同 e3/elf/tapecheck;源与测试相对 `bc5b04bf14` 无改动,非回归。见 out/grk2-stages-baseline.md。

## 72 小时裁决

- A1:未过。一条命令不成立,MANUAL 1-7。
- A3:部分。v4 收据落地,resume 实测未做。
- A7:未过。夜跑工具真实调用必败(55s 调 300s step),夜报未跑。
- B 段 M1-M4 顺延 0.0.53;0.0.52 只做 M0(收据 research/m0-0052)。
- 不阻发布;账本见 `0b5b21a4a9`。

## 未跑范围

外层先 55s 分段后改逐相位 `--until`(外层 <=300s 挂死保险,子进程 55s 不变;依据 AGENTS 正规构造链工具例外类推)。非一条命令六片。MANUAL 1-4 与自动挪残留见上。s9 daily/release 档;s9 夜报;resume 实测;0.0.52 六目标 CI 结果(运行 42881292964 待填)。tools-compile 除已单跑分片外未重跑。stages 基线红 13 项见已知限制。

## 验证入口

1. 核哈希:`sha256sum unisacc.com` 应为 1bffabba...ff55ee19(6529460 字节)。
2. `sh ./unisacc.com --version` 输出 `unisacc 0.0.52`。
3. 冒烟:未跑。
4. 复现:`release-run --pack` 未跑。
5. 原生 CI:未跑。
