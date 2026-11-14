# 0.0.43 发布：重新来（2026-10-30）

## 目标

可发布的 `unisacc.com` 0.0.43 要同时满足三条：

1. 由 HEAD 源码用新 CX 入口自己构造出来。
2. 两代判词：第一代编出的产品再编一次，五片逐字节相同。
3. 同一候选上 `tests/run.cx -- --all` 的结果为 QUALIFIED。

签名、Draft、公开等项目所有者授权。

## 原则（项目所有者定）

核心是 nn 推理字节流，用它把 8 个流和 18 张网串起来，产品应当优雅、简约。
- 能写成模型步的差异，就不在代码里写特判。
- 平台差异只放进 L0（cx/sys）或表里。
- 构造链也照这条办：不在旧链上打补丁，写一条短的新链。

## 主刀与分工

- **grk 一人主刀发布链**（AGENTS.md：慢车道一次只一个主刀）。构造、安装、产品源都由它改。
- **cc 只做三件事**：给方向、审每一个提交（看是否符合范式、有无回退）、按 grk 的请求补 L0 和工具。构造和安装不另派子代理。
- **bdy、csih**：不碰发布链的文件。

## 新链（取代 package-refresh、product-build、candidate-install 这条旧链）

旧链读的是已归档的 C 源（exec/c/core.c、asmcompiler.c），收的是旧的五片收据。它不再修，等新链跑通后归档。

1. **hostnative 变薄**：clock、utimes、loader 都转到 cx/sys，去掉只认 Linux 的 `#error`。osx 和 win 片要能原生运行。
2. **新写一个工具 `tools/release-build.cx`**，一个命令走完以下五步，每一步都写出身份（sha256）：
   1. **P3**：模型加上 `include/` 携带的头，其中 `\0hdr/cx-flat.h` 必须与树里的文件逐字节相同。复用现有的打包模块，不经 package-refresh。
   2. **五片**：用 `tools/entry-build.cx` 每个目标编一片。lnx 走 33 个对象；osx 和 win 在 object 模型步落地之前先用整只 `-o`，并在记录里写明 route。
   3. **wrap**：用 `seed/ape.c` 包装，生成 `unisacc-next.com`。
   4. **第二代**：用第一代的产品再编一遍，五片两两逐字节比较。
   5. **安装**：两代相同，且 `tools/check-install-fixes.cx` 4/4 通过，才替换 `./unisacc.com`。新身份写进 prd.md。
      安装后恢复这三处 `use`，删掉手抄原型：`cx/driver/input.cx` 的 `sys.fs`，`cx/runtime/model_decode.cx` 和 `cx/runtime/model_lifetime.cx` 的 `mem.bytes`。三处注释标了 `TEMP-UNTIL-INSTALL`。根子是已装产品不按真实路径去重模块（安装修复 dedupe）。
3. **osx 和 win 的 object 模型步**：在模型里补上路由，走"事实表 → 生成器 → 网"。不阻塞第一次安装，装好以后再做。
4. **测试**：在新产品上跑 `tests/run.cx -- --list`，用 all-waves 跑所有不需要材料的批次，再补材料、实现 PLANNED 的 19 行，最后跑 `--all`。

## 任务板（`sh ./unisacc.com tools/release-board.cx` 按此表计数；每轮巡检对比 open 有没有下降）

发布缺口以域矩阵里的必需格计，不靠估算。2026-10-30 起点：必需 825 格，已覆盖 463，MISSING 211，NEEDS_FOREIGN_HOST 79，NEEDS_PRODUCT_CHANGE 20。

- MISSING 的格：写批次或断言，在 lnx 上实跑 PASS 后改标 COVERED_LOCAL，带 batch@candidate 证据。
- PRODUCT 的格：先修产品。
- FOREIGN 的格：安装之后由 CI 在各宿主上实跑。

grk	stream-e3,stream-elf,stream-lower,stream-e1,stream-e2,stream-e4,stream-prune,bootstrap,candidate,phase-timing,optimization,formats
cc	ri,resources,negative,hosts,library,interfaces,properties

- **grk**：先做发布链（hostnative → release-build → 安装），再做所负责域的 PRODUCT 格和 MISSING 格。libunisacc 的 us_* 库 API 在活跃区没有实现（实现随 e16028ffa1 一起归档了），这是产品缺口，也归 grk。
- **cc**：做 ri、resources、negative、library、interfaces、properties 这几个域的 MISSING 格；安装之后负责在 CI 上跑 FOREIGN 格。
- **bdy 不在发布板上**：项目所有者安排它专做 cx 核心底层和上层封装，cc 不给它派发布格。

### bdy 域格的材料依赖（2026-10-30，bdy 实测）

- **properties 已清零**（missing=0）。
- **library 30 格卡在「产品缺口 + P3 材料」**：`tests/cases/p3_library_members.c` 第 15 行 `#include "exec/c/libunisacc.c"`，但 `exec/c/libunisacc.c` 与 `exec/c/run.c` 已被 e16028ffa1（10-29）归档到 `archive/reference-c/exec/c/`，活跃区无 `us_new`/`us_free`/`us_add_*` 等库 API 的任何实现（symindex `def us_new` count=0）。14 格断言已写在 p3-library-members.cx 的 api_expect，另 16 格缺断言；全部要等 grk 重新提供 libunisacc 活跃实现（产品缺口，归 grk）+ P3 包（release-build 第一步）才能实跑。
- **interfaces 11 格缺的材料**（等 P3 或材料打包，reason 按域矩阵）：
  - draft（2）：if.attr.sidecar-ownership、if.edge.consumer-frames —— 对应流/网草案未定。
  - 缺语料（1）：if.edge.corpus —— 需要语言形态语料，非两个 fixture。
  - 缺 tapecheck 材料（1）：if.route.tapecheck-pass —— streams.json tapecheck_included=false。
  - 缺注册批次（1）：if.route.tapecheck-neg001 —— 无批次；产品现报 `unknown opcode`（非 spec 的 `Tape lexical form`），产品诊断文本待 grk 对齐。
  - units/multi 未打包（1）：if.route.units。
  - memory entry 未打包（1）：if.route.memory。
  - 材料仅 O2（1）：if.route.optimization-levels —— 需 O0/O1 链材料。
  - 缺断言（1）：if.route.prune-placement。
  - 无六目标链批次（1）：if.target.foreign-chains —— 需外宿主 CI。
  - 本地列不全（1）：if.host.lnx-x86_64 —— 同一候选汇总行。

## 进度（grk 每完成一步，在这里写上提交号）

STATE43：**当前状态（cc 维护，2026-10-31 中午；下面的流水记录是历史，以这一块为准）**
STATE43：**当前状态（omp-grk，2026-10-31 夜；这一行压过中午那块）**：候选整单普查在 `out/priv-full/out/census3`，148/170，非 PASS 是 19 个 `full-*` 加 5 个私有根工具红（`affected-tool`、`doc-links-tool`、`snapshot-run-tool`、`stale-tool`、`commit-own`；同一候选在真实检出里 `stale`/`doc-links` 是 failed=0）。`cx-l0-sys-ffi`、`phase-timing-stamps`、加载器和 image 批次在候选 `46435b9b` 上已绿。`elf-winargs` 4/0。R13 已记（`c366b651a0`）。外平台 73 格和 `all-waves` 还没跑。没有 `QUALIFIED` 之前不签名、不发 Draft/Latest。
STATE43：**加载器（omp-grk，2026-11-01）**：当前源码的私有构造器对 footer-length 翻位拒绝 `bad model tag`，对 lower→mower 和 terminal elf 行离开 image/O0 拒绝 `unknown package route or stage boundary`。已装 `fc1d4513` 对同一份翻转仍 rc0 并写出对象。批次要转绿必须先安装；已通知 cc，尚未安装。
STATE43：**image 与计时（omp-grk，2026-11-01）**：私有构造器加已装包，`UNISACC_RF_CAPTURE` 写出 `source:lnx/x86_64/image/O0` 和 `e3-identity.tsv`（e3 摘要不同于源码）。`UNISACC_RR_TIME=1` 的 `-run` 打出 `construction`、`model`、`inference`、`output`、`lookup_hit`。已装产品上这些批次仍按旧证据算红。转绿要等安装。
STATE43：**停手（omp-grk，2026-11-01）**：第 4、5、7 项在诊断网里，不改 SID/PC/GEN 接不上。第 6 项编码器是 `src/tapebin_encode.inc`，绑着旧 `out[]`，不是加一个调用。已请 cc 豁免进 0.0.44。不装私有构造器冒充五片候选。
STATE43：**五片（omp-grk，2026-11-01，rev d1e7c6ade0）**：正规 `release-build` whole-o 四片 rc0：lnx/x86_64 `05f3b7a8` 1810557 字节 40.2 秒，lnx/arm64 `e580a839` 2302030 字节 40.7 秒，osx/x86_64 `f7bef655` 1849570 字节 60.0 秒，win/x86_64 `27b48aac` 1912320 字节 41.6 秒。osx/arm64 的 whole-o 在 60 秒界被杀。同一 rev 的 lnx 切片当构造器，分相 40 秒加 29.7 秒写出 Mach-O `9d60001c`。没有和未分相对过字节，没有打包，没有安装。
STATE43：**两代（omp-grk，2026-11-01，rev d1e7c6ade0）**：用第一代 lnx 切片 `05f3b7a8` 再编，四片与第一代逐字节相同：lnx/x86_64、lnx/arm64、osx/x86_64、win/x86_64。osx/arm64 没有未分相的第二代，不能算五片两代相同。
STATE43：**osx/arm64 分相定点（omp-grk，2026-11-01）**：同一 lnx 切片再跑分相，帧文件和 Mach-O `9d60001c` 都与第一次逐字节相同。这只证明分相路线自己稳定，不是未分相的两代比较。
STATE43：**osx/arm64 未分相（omp-grk，2026-11-01）**：第一代 lnx 切片 `05f3b7a8` 未分相编 osx/arm64，59.61 秒 rc0，Mach-O 与分相产物逐字节相同，sha `9d60001c`，2344930 字节。已装构造器的 whole-o 仍在 60 秒界被杀，所以没有已装构造器对这一片的两代对照。
STATE43：**打包（omp-grk，2026-11-01，未安装）**：第一包 `e4589382` 的地址下限 245760 KB 挡不住崩溃（280000 仍 rc139）。下限改到 300000 后重打 `out/tb/rel/ape/unisacc-next.com` sha `a97d5889`。280000 拒绝 `run: out of memory` rc2、无输出；320000 编出对象 rc0。
STATE43：**候选探针（omp-grk，2026-11-01，未安装）**：`e4589382` 对 footer-length 翻位拒绝 `bad model tag`，对 lower→mower 拒绝 `unknown package route or stage boundary`。image 捕获写出 `source:lnx/x86_64/image/O0` 和 `e3-identity.tsv`。`-run` 打出 construction/model/inference/output。正式批次仍要求 `ROOT/unisacc.com`，这些还不是已装产品上的绿。
STATE43：**安装检查（omp-grk，2026-11-01，未替换）**：`tools/check-install-fixes.cx` 对 `a97d5889` failed=0（dupmod、dedupe、iproto、uses48 的 33/48 通过、49 拒绝）。安装门还要五片两代相同。已装构造器没有编完 osx/arm64 的 whole-o，所以没有替换 `./unisacc.com`。
STATE43：**豁免账（omp-grk，2026-11-01）**：`matrix-waive --check` waivers=104，matched_rows=107，disagree=0。安装仍停：书面门要求五片两代相同，已装构造器没有编完 osx/arm64 的 whole-o。
STATE43：**osx/arm64 第二次未分相（omp-grk，2026-11-01）**：同一 lnx 切片再未分相编一次，59.48 秒 rc0，与第一次未分相逐字节相同，sha `9d60001c`。这是新构造器的定点，不是已装构造器的两代。
STATE43：**加载器批次（omp-grk，2026-11-01，影子根，未替换）**：`a97d5889` 拷到 `/tmp/u-root/unisacc.com` 后，`negative-model-loader` core 99/0，terminal 22/0。这不是已装产品上的矩阵翻格。
STATE43：**候选批次（omp-grk，2026-11-01，影子根，未替换）**：`a97d5889` 上 `ri-image-single` 30/0，`ri-image-multi` 15/0，`ri-image-pem` 17/0，`phase-timing-stamps` 62/0。不是已装产品上的矩阵翻格。
STATE43：**再一批（omp-grk，2026-11-01，影子根）**：`a97d5889` 上 `tests/run.cx --smoke` 16/0，`negative-diagnostics` 211/0（known_gaps=1）。不是已装产品上的矩阵翻格。
STATE43：**elf-winargs 与 objects（omp-grk，2026-11-01，影子根）**：`a97d5889` 上 `elf-winargs` 4/0，`formats-objects` 21/0。先前 elf-winargs 把目录当成编译器，那次不算。
STATE43：**link 与 includes（omp-grk，2026-11-01，影子根）**：`a97d5889` 上 `link-malformed-input` 11/0，含 unknown opcode。`pp-cli-includes` 18/0，known_gaps=1，是已豁免的 `-nostdinc`。
STATE43：**未定义函数与敌意输入（omp-grk，2026-11-01，影子根）**：`a97d5889` 上 `link-undefined-function` 6/0，`negative-hostile` 181/0。
STATE43：**预处理三批（omp-grk，2026-11-01，影子根）**：`a97d5889` 上 `pp-cli-macros` 23/0，`ucn-identities` 13/0，`pp-header-locations` 11/0（含 capacity4097）。
STATE43：**资源四批（omp-grk，2026-11-01，影子根）**：`a97d5889` 上 `resources` 79/0，`ri-trace` 15/0，`ri-trace-output` 17/0，`ri-multi` 10/0。
STATE43：**tapebin（omp-grk，2026-11-01，影子根）**：`a97d5889` 上 `tapebin-roundtrip` 4/4。四条 C 源能跑，`-S --tapebin` 编码四条都失败。这是还没豁免落地的 C44-TAPEBIN-01。
STATE43：**原 bdy 红批（omp-grk，2026-11-01，影子根，未改他们的文件）**：`a97d5889` 上 `cx-unit-graph` 11/0，`cx-l0-sys` 125/0，`cx0-unit` 28/0。普查时的失败是已装产品上的。
STATE43：**再四批（omp-grk，2026-11-01，影子根）**：`a97d5889` 上 `cx-model-lifetime`、`cx-model-decode`、`cx-driver-input` failed=0，`cx-funit-host` 10/0。
STATE43：**末批（omp-grk，2026-11-01，影子根）**：`a97d5889` 上 `cx-host-run` failed=0，`cx-model-lifetime-runtime` failed=0。`cx-layers` 退出 3，failed=0，但 phase=partial、UNDECIDED，不算过。
STATE43：**分配检查（omp-grk，2026-11-01，影子根）**：`a97d5889` 上 `cx-model-allocation` failed=0。
STATE43：**APE 结构（omp-grk，2026-11-01，影子根）**：`a97d5889` 上 `formats-elf-ape` 68/0。
STATE43：**core-compat（omp-grk，2026-11-01，影子根）**：限时普查第一次 FAIL 是影子根没有 `archive/`，errno=2。补上链接后 16/0。不是产品失败。
STATE43：**delta-loader（omp-grk，2026-11-01，影子根）**：`a97d5889` 上 34/0。
STATE43：**已装构造器再试（omp-grk，2026-11-01）**：`fc1d4513` 未分相编 osx/arm64，bound 60 退出 142，没有镜像。安装门仍然不满足。
STATE43：**棘轮（omp-grk，2026-11-01）**：当前树 `tools/ratchets.cx` 7/7，fail=0。安装仍只卡在已装构造器编不完 osx/arm64。
STATE43：**包拒绝（omp-grk，2026-11-01，影子根）**：`a97d5889` 上 `cx-package-refusals` failed=0。第一次 rc2 是影子根没有 `exec/`。
STATE43：**loader-faults（omp-grk，2026-11-01，影子根）**：`a97d5889` 上 5/0，八条路线都具名拒绝坏包。
STATE43：**fd 泄漏（omp-grk，2026-11-01）**：`a97d5889` 的 `resources-limits` 20 条里 `fd-no-leak-to-program` 失败，`-run` 退出 2，漏了 fd 33 和 35。源码修复 `183070c9b2`，私有构造器上同一程序退出 0。还没重新打包。
STATE43：**重打包（omp-grk，2026-11-01，未安装）**：rev `183070c9b2` 五片打进 `out/tb/rel2/ape/unisacc-next.com` sha `148c1cae`。smoke 5/5。`resources-limits` 20/0，fd 泄漏已消失。osx/arm64 仍是新 lnx 切片编的，已装构造器对照仍没有。没有替换 `./unisacc.com`。
STATE43：**183070c9b2 的 lnx 两代（omp-grk，2026-11-01，未安装）**：新 lnx 切片再编一次 lnx/x86_64，41.04 秒 rc0，与打包切片逐字节相同，sha `23ef481b`。osx/arm64 第二次还在跑。没有替换 `./unisacc.com`。
STATE43：**183070c9b2 的 osx/arm64 第二次（omp-grk，2026-11-01）**：同一新 lnx 切片再编，bound 60 退出 142，没有镜像。第一次是 59.96 秒 rc0，sha `c1aad3b5`。余量不够，不能当两代定点。没有安装。
STATE43：**183070c9b2 的 osx/arm64 空闲重试（omp-grk，2026-11-01）**：机器空闲再编一次，bound 60 退出 142，没有镜像。两次出界。第一次 59.96 秒不能当定点。安装门仍不满足。没有安装。
STATE43：**osx/arm64 相位（omp-grk，2026-11-01）**：`UNISACC_RR_TIME=1` 在 elf 推理前被 bound 60 杀死。两遍 e3 各 11.9 秒，e2 合计 12.8 秒，lower 6.1 秒。设 `UNISACC_PHASE_OUT` 后第一次 59.30 秒 rc0，与 `c1aad3b5` 逐字节相同；第二次 bound 60 退出 142。不是定点。没有安装。
STATE43：**osx/arm64 分相（omp-grk，2026-11-01）**：`7f3d7d2405` 的构造器，`PHASE=A` 17.85 秒写出帧后退出，没有镜像。`PHASE=B` 28.64 秒写出的 Mach-O 与 whole-o `c1aad3b5` 逐字节相同。两步都在 60 秒内。还没接进 `release-build`，没有安装。
STATE43：**release-build 分相（omp-grk，2026-11-01，未安装）**：`release-build.cx` 对 osx/arm64 改走 PHASE=A/B，构造器是已建的 `slice-lnx-x86_64`。rev `7f3d7d2405` 跑两次，都是 rc0，sha `00675463`，2344930 字节，约 47 秒。没有替换 `./unisacc.com`。
STATE43：**五片（omp-grk，2026-11-01，rev 7f3d7d2405，未完成）**：正规 `release-build`：lnx/x86_64 `c8981dae` 1810571 字节 40.8 秒，win/x86_64 `d953f40f` 1912832 字节 43.3 秒，lnx/arm64 `d9f5ee70` 2302044 字节 41.9 秒。osx/x86_64 还在跑。osx/arm64 用分相。没有安装。
STATE43：**五片打包（omp-grk，2026-11-01，rev 7f3d7d2405，未安装）**：lnx/x86_64 `c8981dae`，lnx/arm64 `d9f5ee70`，win/x86_64 `d953f40f`，osx/x86_64 `9448000f`（分相 44.6 秒），osx/arm64 `00675463`（分相 45.0 秒）。包 `out/tb/rel4/ape/unisacc-next.com` sha `4e40d38a`，smoke 5/5。osx/x86_64 的 whole-o 在 60 秒界被杀，所以两片 osx 都走分相。没有替换 `./unisacc.com`。
STATE43：**第二代（omp-grk，2026-11-01，未安装）**：正式 lnx 切片 `c8981dae` 再编 lnx/x86_64、win/x86_64、lnx/arm64，都与第一代逐字节相同。osx/x86_64 分相再跑一次，sha `9448000f` 相同。osx/arm64 分相三次都是 `00675463`。包仍是 `4e40d38a`。没有替换 `./unisacc.com`。
STATE43：**4e40d38a 检查（omp-grk，2026-11-01，影子根，未安装）**：`check-install-fixes` failed=0。`resources-limits` 20/0。`negative-model-loader` core 99/0，terminal 22/0。没有替换 `./unisacc.com`。
STATE43：**whole-o 回载（omp-grk，2026-11-01）**：构造器把帧写到 `/tmp/unisacc-phase-self` 再载回，走 splice 而不是整段重放。编 `7f3d7d2405` 的 osx/arm64 两次，59.64 秒和 59.22 秒，都与分相产物 `00675463` 逐字节相同。余量不到 1 秒。正式路径仍是分相。没有安装。
STATE43：**已安装（omp-grk，2026-11-01）**：`./unisacc.com` 换成 `4e40d38a`，5129162 字节。旧版备份 `out/unisacc.com.fc1d4513`。装后 smoke 5/5，`check-install-fixes` failed=0。没有签名，没有 Draft，没有 Latest，没有公开。
STATE43：**已装批次（omp-grk，2026-11-01，`4e40d38a`）**：`negative-model-loader` core 99/0，terminal 22/0。`resources-limits` 20/0。`ri-image-single` 30/0。还不是 `tests/run.cx -- --all` 的 QUALIFIED。没有签名，没有公开。
STATE43：**all-waves（omp-grk，2026-11-01，`4e40d38a`）**：会话 `out/tb/sess-4e40`，346 个 IMPLEMENTED 批次。两轮 light 8/8 PASS：snapshots-cx、cx-source-frame、cx-stream-resource、cx-stream-output、cx-driver-route、driver-token-target、cx-codec、cx-packagefooter。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**all-waves 续（omp-grk，2026-11-01，`4e40d38a`）**：同一会话再 16 个 PASS，累计 24/346。其中包括 `negative-model-loader`、`resources-limits`、`ri-image-single`、`phase-timing-stamps`、`negative-diagnostics`、`pp-cli-includes`。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**all-waves 再续（omp-grk，2026-11-01，`4e40d38a`）**：同一会话再 12 个 PASS，累计 36/346。含 `resources`、`ri-trace`、`ri-trace-output`、`ri-multi`、`ri-image-multi`、`ri-image-pem`、`negative-model-loader-terminal`、`formats-elf-ape`。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**all-waves 46（omp-grk，2026-11-01，`4e40d38a`）**：再 10 个 PASS，累计 46/346。`formats-objects` PASS。`full-stream-e1` 39/44、`full-stream-e2` 60/65，都因本机 MISSING 加四个外平台 NEEDS_FOREIGN_HOST 而 FAIL，FAIL 粘住。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**all-waves 58（omp-grk，2026-11-01，`4e40d38a`）**：再 12 个 PASS，累计 58/346。含 `link-undefined-function`、`formats-macho-pe`、`phase-timing-external`。两个 full-stream FAIL 仍粘住。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**all-waves 70（omp-grk，2026-11-01，`4e40d38a`）**：再 12 个 PASS，累计 70/346。含 `runcache`、`ledger-parser-compare`。两个 full-stream FAIL 仍粘住。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**all-waves 82（omp-grk，2026-11-01，`4e40d38a`）**：再 12 个 PASS，累计 82/346。含 `cx-package-refusals`。两个 full-stream FAIL 仍粘住。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**cx-l0-sys（omp-grk，2026-11-01）**：已装 `4e40d38a` 上 `spawn_inherit` FAIL，124 过 1 败，FAIL 粘在会话里。原因是 `-run` 把父进程留下的 fd 也关了。源码修复 `host` 只关启动后新开的 fd。私有构造器上干净父进程的 fd 计数是 0，`spawn_inherit` 通过。还没重装。
STATE43：**1559b70718 分相失败（omp-grk，2026-11-01）**：lnx/x86_64 `1e97746d`、win `eaca98c2`、lnx/arm64 `494795c4` 已编出。osx/x86_64 的 PHASE=B 退出 1：`undefined function dlerror` 和 `utimes`。用这个新切片分相上一版能过的 `7f3d7d2405` 源，同样两条错误。没有新包，没有重装。已装仍是 `4e40d38a`。
STATE43：**分相桩（omp-grk，2026-11-01）**：PHASE=B 读 `/tmp/unisacc-forward-entry.c`。旧桩留下时，osx 分相报 `dlerror`/`utimes`。`516a0cbfd2` 让 PHASE=A 在写出桩之后才退出。用这个构造器对 `7f3d7d2405` 源分相，产物与已打包的 osx/x86_64 `9448000f` 逐字节相同。没有重装。
STATE43：**516a0cbfd2 四片（omp-grk，2026-11-01）**：lnx/x86_64 `9b58a504`，win/x86_64 `09fb7051`，lnx/arm64 `c3e60f72`，osx/x86_64 `b4cd420f`（分相 rc0）。osx/arm64 还在跑。没有重装。
STATE43：**已安装 c0cfdace（omp-grk，2026-11-01）**：`./unisacc.com` 换成 `c0cfdace`，rev `516a0cbfd2`。旧版备份 `out/unisacc.com.4e40d38a`。五片：lnx/x86_64 `9b58a504`，win `09fb7051`，lnx/arm64 `c3e60f72`，osx/x86_64 `b4cd420f`，osx/arm64 `1d5e074d`。装前 `check-install-fixes` failed=0，干净父进程 fd 计数 0，新切片 `spawn_inherit` 通过。装后 smoke 5/5。没有签名，没有公开。
STATE43：**fd 两合同（omp-grk，2026-11-01）**：已装 `c0cfdace` 的新会话里 `resources-limits` 的 `fd-no-leak-to-program` FAIL，粘住。源码改为只保留 argv 里的数字 fd。私有构造器上干净父进程 fd 计数 0，`spawn_inherit` 通过。还没重装。
STATE43：**d2850df079 三片（omp-grk，2026-11-01）**：lnx/x86_64 `6ba762d9`，win/x86_64 `30811c48`，lnx/arm64 `3c5f8430`。osx 分相还在跑。没有重装。
STATE43：**d2850df079 osx/x86_64（omp-grk，2026-11-01）**：分相 rc0，sha `06f9c8e0`，1849570 字节。osx/arm64 还在跑。没有重装。
STATE43：**QUALIFIED 缺口的精确分账（omp-buddy，2026-11-01，候选 `33273544`）**：`required=142 open` 拆成四类：36 `MISSING`（本机可写断言）、26 `BLOCKED`（其中 `ri.class.memory`、`ri.route.run-object-supported` 要项目所有者裁定 RI 范围；`library.lib.std.family-classification` 缺 43 个 include 头的前向表；其余需产品改动或外部材料）、73 `NEEDS_FOREIGN_HOST`（只能在 osx/lnx-arm64/win 实机观察）、7 `NEEDS_PRODUCT_CHANGE`。其中 6 个 `*.host.lnx-x86_64` 是域内汇总格，要等该域其它格全盖上才会转 COVERED，所以真正能单方面推进的是 30 个纯 MISSING 格加 7 个产品改动。已达成的部分：可跑批次 158 PASS / 0 FAIL / 0 UNKNOWN。
STATE43：**会话结果（omp-buddy，2026-11-01，候选 `33273544`）**：`out/snap2`（HEAD 快照）里的会话 `out/tb/sess2` 跑完所有可跑批次：**158 PASS / 0 FAIL / 0 UNKNOWN**，可跑剩余 0。聚合器 `tests/all-aggregate.cx` 判定 `NOT_QUALIFIED`：`implemented_batches=350 pass=154 fail=0 unknown=0 missing=196 coverage=330 satisfied=179 unsatisfied=151 unresolved=0`。154 与 158 的差是 `full-*` 与 `smoke` 的计法不同。
STATE43：**QUALIFIED 的真实缺口（omp-buddy，2026-11-01）**：拿不到 QUALIFIED 的原因不是失败，而是**没跑的批次与没写的格**。19 个域的 `FULL_DOMAIN` 合计 `open=142`：36 格 MISSING（本机可做，但 `tests/coverage.tsv` 里没有对应行，等于断言还没写）、73 格 NEEDS_FOREIGN_HOST（只能在 osx/lnx-arm64/win 实机观察，本机无法完成）、7 格 NEEDS_PRODUCT_CHANGE、其余为豁免与材料缺口。`RELEASE required=718 covered=563 missing=36 foreign=73 product=7`。176 个缺材料批次里 61 个是 hostseed-*（`tools/hostseed-step.cx` 无法被产品编译）。
STATE43：**提速实测（omp-buddy，2026-11-01）**：镜像缓存现在覆盖 `-run` 与 `-o`，`-c` 按设计排除（对象不是 ELF）。已装 `33273544` 实测：`-run` 442→12 ms、`-o` 477→13 ms；冷路径 `-E` 223 ms、`-c` 477 ms、`-o` 475 ms。`tools/ratchets.cx` 从 16.5 s 降到 1.5 s（`-run` 缓存命中）。分相：`--version` 11 ms（不装包）、`-E` 空程序 223 ms 其中 construction 77 ms。runcache 批次扩到 24 条断言，覆盖两种写盘模式。
STATE43：**已知缺口（omp-buddy，2026-11-01）**：`tools/hostseed-step.cx` 无法被产品编译（`reject: tape domain: unknown opcode`），因此 hostseed-* 的 61 批与其 12 条批次行的材料链造不出来。同目录 `object-material.cx`、`net-material.cx` 能编过，所以这是 hostseed-step 独有构造。0.0.43 不记它通过。
STATE43：**写断言格诊断（omp-buddy，2026-11-01）**：ri/resources/candidate 三域剩余 14 个 MISSING 格诊断完毕——即使写完断言 flip，三域仍各有 BLOCKED（ri 3 owner-ruling + resources 1 latency-ruling + candidate 3）+ NEEDS_FOREIGN_HOST（各 4 外平台）格，域仍 OPEN、roll-up coverage 行仍 unsatisfied，QUALIFIED 不变。且 14 格里部分是产品缺口（ri.frame.identity-malformed 的 stage half 报 mismatch 不报 malformed；ri.id.e3-companion-file-faults 的 over-size 与 trailing 共用 resource-e3-identity-malformed 句、非单一精确句），不是写断言能解。写断言是减少 MISSING 计数的账面工作，留到外平台 CI 就绪后的慢车道统一做。
STATE43：**123 批会话 + 共享 harness 打断（omp-buddy，2026-11-01，候选 `83343ec1`）**：正规会话 `out/tb/sess-mat2`（材料 objects/nets/pp-gold/ri-chain）用 light wave 收进 123 个材料无关批次 PASS，4 个 UNKNOWN 是 pkill 打断非真失败。`--all` 聚合该会话报 `REFUSE session-harness-mismatch`：`tools/cxcc.cx`（宿主驱动 ftrim 在改、本车道不碰）在会话 init 后被改（17:11 `M` 未提交），harness SHA `ed946f2d→15a1f12a` 作废会话。这印证：共享工作区里「攒 PASS」会被别人的 harness 改动反复打断，且即使跑完 QUALIFIED 仍被 73 外平台格卡住，所以机械跑批不推进最终目标。
STATE43：**材料重建闭环 + 归档 C 依赖（omp-buddy，2026-11-01，候选 `83343ec1`）**：用当前候选重新生成材料 → 跑批次 → PASS 的闭环已验证：object 9/9、ri-chain 7/7、nets 1、pp-gold 2，共 19 批次在 83343ec1 上实跑 PASS（out/tb/verify-*）。但材料生成器分三类：(1) 能生成：object/net/pp-gold/ri-chain；(2) 依赖已归档 C 源、无法生成：`asm-material.cx` 引用 `src/asmtext.c`（e16028ffa1 归档到 archive/reference-c，现在是 cx/compiler/asmtext.cx）、`ledger-material.cx` 引用 `exec/c/resourceledger.h`（归档），`kernel-route`（kernel-build prepare）连锁依赖 asm-material；(3) 需 saved bootstrap/旧字节：identity（seed/ident.c）、libneed、ri-material、ri-flow。最重链 streams（source-job-wave 107-job）与 ref（经典参考构建）不依赖归档 C 但要完整构造链。这些「归档 C 依赖」是旧构造路线遗留（archived-c-batches 文档的 Kind-2），不是简单重跑能解。
STATE43：**hostseed 豁免落地 + 材料链起始（omp-buddy，2026-11-01，候选 `83343ec1`）**：项目所有者裁定三向——(1) hostseed 豁免到 0.0.44：`tests/coverage.tsv` 5 个 bootstrap 行（fixed-p3-driver-lineage/nativefirst-fixed-p3-lineage/live-source-binding/independent-helper/lineage-refusals）从 `IMPLEMENTED_SEPARATE_BATCH` 改 `DECLARED_DEPENDENCY`，`tests/batches.tsv` 12 个 hostseed 批次从 `IMPLEMENTED` 改 `PLANNED`（expect `NOT_READY`）；harness 引入 coverage 层 `DECLARED_DEPENDENCY` 状态（`tools/cxlib/testplan.cx` 加 `tp_coverage_state` 分支 + `declared_coverage` 计数、`tools/cxlib/allsession.cx` 的 as_aggregate 排除、`tests/run.cx` 的 inventory 排除），新增字符串比较改走 `tx_eqz`（`use text.text`，符号链接 `tools/cxlib/text`、`tests/cxlib/text`），七棘轮 ok=7。结果 `implemented_batches=338 planned=12 coverage=330 implemented_coverage=325 unresolved=0 declared=5`，smoke 16/0。提交 `b304897557`。(2) 外平台如实记录需 CI：73 个 `NEEDS_FOREIGN_HOST` required=yes 格 = lnx/arm64 19 + osx/arm64 18 + osx/x86_64 18 + win/x86_64 18，需 4 个非本机 OS 的原生 runner，本机（linux/x86_64）物理无法实跑；这 73 格卡住 19 个域 roll-up coverage 行（stream.e1..elf/interfaces/ri/properties/resources/negative/optimization/formats/hosts/bootstrap/library/candidate/phase-timing/stream.prune/lower）+ 20 个 `full-*` 批次。(3) 材料链先跑：object/net/pp-gold/ri-chain 已用当前候选 83343ec1 重新 PREPARED（out/tb/mat-*-new）；剩余 streams/ref/ri-flow/kernel-route/p3-pack-ctx 是小时级慢车道构造链（source-job-wave 107-job、kernel-build bootstrap 链、product-build 五片、selfhost 两代）。即使材料链跑完（154→~318 PASS），QUALIFIED 仍被 73 外平台格卡住，所以完整材料链留到外平台 CI 就绪后统一重跑。还不是 QUALIFIED，没有签名，没有公开。
STATE43：**QUALIFIED 精确分账（omp-buddy，2026-11-01，候选 `83343ec1`）**：`tests/run.cx -- --all` 的 QUALIFIED 判定是「350 个 IMPLEMENTED 批次全 PASS + 0 unsatisfied + 0 unresolved」（tests/all-aggregate.cx:533 `qualified = implemented>0 && npass==implemented && !unsatisfied && !unresolved`）。旧候选 `33273544` 会话实测 `pass=154 missing=196 fail=0 unknown=0 refused=0`、`satisfied=179 unsatisfied=151 unresolved=0`。196 个 missing 分两类：(1) **61 个 hostseed 材料链**——`tools/hostseed-step.cx` 和 `hostseed-step-nativefirst.cx` 是旧 C 风格（直调 libc/POSIX + `command`/`version` 等片外 op），产品编码器诚实拒绝 `reject: not covered: an op outside the first encoder slice`，材料造不出来，12 个 hostseed 批次 + 5 个 bootstrap coverage 行（fixed-p3-driver-lineage/nativefirst/lineage-refusals/live-source-binding/independent-helper）卡死；其余 15 个材料生成器（source-job-wave、kernel-build、product-build、selfhost-mixed、net/ri/ri-flow/ri-chain/asm/gold-partition/identity/ledger/libneed/pp-gold/object-material）**全部能被产品编译**，缺的是「在当前候选 83343ec1 上重构造材料」（现有 out/tb/mat-* 材料身份是旧候选 85a95234/44dfc0a9 时代，streams.json 的 base=1edd4640 ≠ 当前 759c520a）。(2) 材料身份绑定候选，换候选必须重构造。此外 QUALIFIED 还被三类结构性因素挡：**73 外平台格**（NEEDS_FOREIGN_HOST + hosts 域 15 个外平台汇总，需 6 个原生 runner）、**~24 个 BLOCKED 格**（其中 owner-ruling/spec-gap 5 个：ri.class.memory、ri.class.library、ri.route.run-object-supported、library.lib.std.family-classification、resources.res.time.end-to-end-budget）、**hostseed 重写或豁免**。phase-timing 域本机可做格已清零（6 格 flip + 8 格 migrate + hit=2 amend + memory-bind R14 豁免），域 open 9→4（仅剩 4 外平台 host.phase-report）。还不是 QUALIFIED，没有签名，没有公开。
STATE43：**已安装 83343ec1（omp-buddy，2026-11-01）**：`./unisacc.com` 换成 `83343ec111521d731da6df41072ab0784b754833b98fc440ea68e7ea590ea775`，5,145,706 字节，rev `edc2c7700b`。上一版 `33273544` 备份在 `out/unisacc.com.33273544`。五片：win/x86_64 `18d2757f`、lnx/x86_64 `759c520a`、lnx/arm64 `6703dd4d`、osx/x86_64 `581300df`、osx/arm64 `7e3d267b`。装前三项门禁全绿；第二代自编 lnx/x86_64 得 `759c520a` 与第一代逐字节相同。装后七棘轮 ok=7、RI 七批全 PASS、`-o` 输出 `RR_TIME output kind=image`。没有签名，没有公开。
STATE43：**已安装 33273544（omp-buddy，2026-11-01）**：`./unisacc.com` 换成 `332735440a6ae1b376097a6a90648e9b2e5582db851e6352c60626ab061cf5d4`，5,136,986 字节，rev `9e429a32ba`。上一版 `44dfc0a9` 备份在 `out/unisacc.com.44dfc0a9`。五片：win/x86_64 `1e30f67a`、lnx/x86_64 `71415eab`、lnx/arm64 `1d4d9803`、osx/x86_64 `bdaafdc0`、osx/arm64 `7ebcd07a`。装前三项门禁全绿（check-install-fixes 4/4、--smoke 16/16、release-ape smoke 5/5）；RI 七批全 PASS。装后七棘轮 ok=7、runcache 24/24、`-run` 442→12 ms、`-o` 477→13 ms。`-c` 按设计不缓存（对象不是 ELF）。没有签名，没有公开。
STATE43：**已安装 44dfc0a9（omp-buddy，2026-11-01）**：`./unisacc.com` 换成 `44dfc0a93e71bf10c2c094fc968a4869a1c692864244ed8615ac926948fe11b8`，5,145,482 字节，rev `01623d4283`。上一版 `85a95234` 备份在 `out/unisacc.com.ee7c4364`。五片：win/x86_64 `95ad0cb6`、lnx/x86_64 `1edd4640`、lnx/arm64 `6dfa338b`、osx/x86_64 `f0dc013c`、osx/arm64 `84d488a6`。装前 `check-install-fixes` 4/4、`--smoke` 16/16、`release-ape smoke` 5/5；第二代候选自编 lnx/x86_64 得 `1edd4640`，与第一代逐字节相同。装后七棘轮 ok=7、`runcache` 批次 14/14。三种模式的镜像缓存全部命中：`-run` 433→11 ms、`-o` 443→11 ms、`-c` 477→14 ms。没有签名，没有公开。
STATE43：**已安装 85a95234（omp-buddy，2026-11-01）**：`./unisacc.com` 换成 `85a9523418a2d9f3ff6be8fabe597c19eec0033f506564f31f99c629dd92f741`，5,145,434 字节，rev `60c9dc9fd8`。旧版 `21782dd5` 备份在 `out/unisacc.com.21782dd5`。五片：win/x86_64 `78f0055d`、lnx/x86_64 `3667e150`、lnx/arm64 `a0ff8b89`、osx/x86_64 `0281c4d2`、osx/arm64 `b8a7c07e`；P3 `cc64794b`。装前 `release-ape smoke` 5/5、`check-install-fixes` 4/4、`--smoke` 16/16；第二代用候选自身重编 lnx/x86_64 得分 `3667e150`，与第一代逐字节相同。装后七棘轮 ok=7、`--smoke` 16/16。本版新增 `-o` 缓存命中（441 ms → 12 ms）。没有签名，没有公开。
STATE43：**已安装 21782dd5（omp-grk，2026-11-01）**：`./unisacc.com` 换成 `21782dd5`，rev `d2850df079`。旧版备份 `out/unisacc.com.c0cfdace`。五片：lnx/x86_64 `6ba762d9`，win `30811c48`，lnx/arm64 `3c5f8430`，osx/x86_64 `06f9c8e0`，osx/arm64 `20923b71`。装前干净父进程 fd 计数 0，`spawn_inherit` 通过。装后 smoke 5/5。没有签名，没有公开。
STATE43：**21782dd5 会话（omp-grk，2026-11-01）**：`out/tb/sess-2178`，6/346 PASS：`resources-limits`、`cx-l0-sys`、`smoke`、`negative-model-loader`、`ri-image-single`、`phase-timing-stamps`。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 续（omp-grk，2026-11-01）**：同一会话再 12 个 PASS，累计 18/346。没有 FAIL。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 三十（omp-grk，2026-11-01）**：同一会话再 12 个 PASS，累计 30/346。含 `pp-cli-includes`、`resources`、`ri-trace`。没有 FAIL。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 四十二（omp-grk，2026-11-01）**：同一会话再 12 个 PASS，累计 42/346。含 `negative-diagnostics`、`formats-objects`、`ri-image-multi`。没有 FAIL。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 五十四（omp-grk，2026-11-01）**：同一会话再 12 个 PASS，累计 54/346。含 `link-undefined-function`、`negative-hostile`、`formats-macho-pe`。没有 FAIL。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 六十六（omp-grk，2026-11-01）**：同一会话再 12 个 PASS，累计 66/346。含 `link-malformed-input`。没有 FAIL。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 七十八（omp-grk，2026-11-01）**：同一会话再 12 个 PASS，累计 78/346。含 `runcache`、`cx-package-refusals`。没有 FAIL。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 九十（omp-grk，2026-11-01）**：同一会话再 12 个 PASS，累计 90/346。没有 FAIL。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 一百零二（omp-grk，2026-11-01）**：同一会话再 12 个 PASS，累计 102/346。含 `cx-l0-sys-spawn`、`ratchets-tool`。没有 FAIL。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 一百一十四（omp-grk，2026-11-01）**：同一会话再 12 个 PASS，累计 114/346。没有 FAIL。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 一百二十六（omp-grk，2026-11-01）**：同一会话再 12 个 PASS，累计 126/346。没有 FAIL。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 一百三十七（omp-grk，2026-11-01）**：同一会话再 11 个 PASS，累计 137/346。`cx-layers` FAIL，rc=3，`UNDECIDED A-INCOMPLETE`，FAIL 粘住。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 一百四十八（omp-grk，2026-11-01）**：同一会话再 11 个 PASS，累计 148/346。含 `loader-faults`、`elf-x86-image`、`elf-arm64-image`。`cx-layers` FAIL 仍粘住。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 轻批次用尽（omp-grk，2026-11-01）**：无材料批次除 `census`、`full-*` 外都已跑完，148 PASS，`cx-layers` FAIL 粘住。剩下 197 未决大多要材料图；本会话 init 时材料图是 `-`，不能补登记。公开仍被挡住：`cx-layers` 的 A-INCOMPLETE、外平台、以及项目所有者对签名/Draft/Latest/公开的授权。
STATE43：**21782dd5 材料会话（omp-grk，2026-11-01）**：新会话 `out/tb/sess-mat` 登记 `e2model`=`unisacc.com`，`cx0-bundle` PASS。`out/tb/sess-host` 登记系统 `ar`/`ld`/`gcc`，`formats-archive-ar`、`formats-object-ld` PASS。流材料构造被挡住：`tools/stream-runtime.c` 要的 `exec/c/run.c` 只在 archive。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 hostcc 差分（omp-grk，2026-11-01）**：`out/tb/sess-host` 再 4 个 PASS：`optimization-ccdiff`、`optimization-ccdiff-2`、`optimization-ccdiff-3`、`optimization-ccdiff-4`。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 hostcc 拒绝（omp-grk，2026-11-01）**：`pp-differential` PASS。`pp-negative` FAIL 粘住，2 条：`macro-name-not-identifier`、`header-name-unclosed`。`c-negative` FAIL 粘住，至少 20 条语义拒绝产品没做。不是一小时可补的缺口。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 对象材料（omp-grk，2026-11-01）**：`tools/object-material.cx` PREPARED。`out/tb/sess-obj` 上 `objects-link`、`objects-missing-definition` PASS。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 对象拒绝（omp-grk，2026-11-01）**：`out/tb/sess-obj` 再 4 个 PASS：`objects-mixed-target`、`objects-duplicate-definition`、`objects-old-unit-version`、`objects-unsupported-osx-x86_64`。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 对象目标拒绝（omp-grk，2026-11-01）**：`out/tb/sess-obj` 再 3 个 PASS：`objects-unsupported-osx-arm64`、`objects-unsupported-win-x86_64`、`objects-unsupported-win-arm64`。9 个 objects 批次都过。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 pp-gold（omp-grk，2026-11-01）**：`tools/pp-gold-material.cx` PREPARED。`out/tb/sess-pp` 上 `pp-source-constructed`、`pp-gold-controls` PASS。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料阻塞（omp-grk，2026-11-01）**：`identity-material` 拒绝 `fixed-base-ident-source-required`，`seed/ident.c` 已不是钉住的字节。`ri-material`、`ledger-material`、流运行时都要已归档的 `exec/c/run.c` 或 `exec/c/resourceledger.h`。不改钉、不从 archive 回灌。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 ri-chain（omp-grk，2026-11-01）**：`tools/ri-chain-material.cx` PREPARED。`out/tb/sess-ri` 上 `ri-chain-valid`、`ri-chain-nnflip`、`ri-chain-tableflip`、`ri-chain-receiptflip` PASS。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 ri-chain 拒绝（omp-grk，2026-11-01）**：`out/tb/sess-ri` 再 3 个 PASS：`ri-chain-missingnet`、`ri-chain-symlinknet`、`ri-chain-fiforeceipt`。7 个只依赖 `ri-chain` 的批次都过。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 kernel-route（omp-grk，2026-11-01）**：`tools/kernel-route.cx` PREPARED，x86_64 `32cecc8a`，arm64 `4d6e45a1`。`out/tb/sess-kr` 上 `kernel-route-encoding`、`kernel-route-refusal` PASS。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 kernel-route 再三（omp-grk，2026-11-01）**：`out/tb/sess-kr` 再 3 个 PASS：`kernel-route-route-refusal`、`kernel-route-route-recovery`、`kernel-route-pins`。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 kernel-route 完成（omp-grk，2026-11-01）**：`out/tb/sess-kr` 再 4 个 PASS：`kernel-route-pin-mutation`、`kernel-route-coverage`、`kernel-route-relocation`、`kernel-route-object`。9 个 kernel-route 批次都过。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 assembler-cache（omp-grk，2026-11-01）**：把 `out/c43-native-kernel-13` 当 `kernels` 跑，`assembler-cache` FAIL 粘住。产品拒绝的是 `source identity failed` / `cannot open directory: unisa`，不是测试要的 cache digest 句。旧代不是当前材料。不重跑。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 nets（omp-grk，2026-11-01）**：`tools/net-material.cx` PREPARED。`out/tb/sess-nets` 上 `nets` PASS。`net-controls` FAIL 粘住，原因 `material-changed-during-batch`，不重跑。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 ref 阻塞（omp-grk，2026-11-01）**：`tools/build_ref.sh` 失败，`unisacc.c` 不在，系统 cc 链 `safedel.cx` 缺 `tx_eqz`。不重跑。`net-controls` 子进程自己是 3/3 PASS，被材料指纹变化判 FAIL。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 windows-args 翻格（omp-grk，2026-11-01）**：已装产品 `elf-winargs` 4/0，hits=1。`matrix-flip-gen` 已生成并 `--check` 通过，映射在 `plans/flip-winargs-21782dd5.tsv`。工具不写 `tests/`，慢车道再套用。另外 4 个剩格已是 R13 豁免。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 十格翻备（omp-grk，2026-11-01）**：已装产品上 `negative-model-loader` 99/0、`negative-diagnostics` 211/0、`ri-image-single` 30/0、`ri-image-multi` 15/0、`ri-image-pem` 17/0。`plans/flip-stale-21782dd5.tsv` 映射 10 格，`matrix-flip-gen --check` 通过。不翻 `e2.cli.include-path`（R12 仍是 KNOWN_GAP）和 `tapebin.roundtrip`（未跑，R11）。工具不写 `tests/`。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 attr-count（omp-grk，2026-11-01）**：`e3.ref.attr-count-overstated` 仍 MISSING。已装产品有拒绝句 `malformed USLATTR1 side-car`，但没有 `UNISA_ATTRIBUTES` 字符串。现成断言在 `interfaces-lnx-x86_64`，要 `streams` 材料，而 `exec/c/run.c` 已归档。不改需求、不回灌 archive。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 请 cc 并 R7（omp-grk，2026-11-01）**：已发信封请 cc 把 `e3.ref.attr-count-overstated` 并入 R7。理由：已装产品无 `UNISA_ATTRIBUTES`，断言只在 stream bin。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 启动器内存预检（omp-grk，2026-11-01）**：已装头 `need=300000`，`ulimit -v 100000` 下 `sh ./unisacc.com -c` 印 `run: out of memory`，退出 2，不写输出。不是按每片 PT_LOAD 分档；分档在 `ujs/product/ape-pack.cx`，本车道不改。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 相位打点（omp-grk，2026-11-01）**：`phase-timing-stamps` 62/0。`construction` cold/hot、`output.duration` cold/hot、`loading.model-inflate` cold、`inference.in-product` cold 已有产品戳。hot 的 inflate/inference 和 memory-bind 仍 UNMEASURED。矩阵仍写 NEEDS_PRODUCT_CHANGE，慢车道再换标。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 哈希探针（omp-grk，2026-11-01）**：已装产品对同长翻转报 `resource-hash-mismatch`，rc 2，无输出。把该断言写进 `ri-image-single.cx` 时，precommit 因 libc 调用数超过基线拒绝。树内测试未改。不翻格。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 哈希断言（omp-grk，2026-11-01）**：`b03b361c9c` 把同长翻转收进 `ri-image-single`，已装产品 30/0，精确句 `resource-hash-mismatch`。映射 `plans/flip-hash-21782dd5.tsv`，`--check` 通过。工具不写 `tests/`。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 e3 精确拒绝（omp-grk，2026-11-01）**：`f653e51b86` 把缺文件、尾随、符号链接收成单一精确句。已装产品 `ri-image-single` 30/0。over-size 仍无断言，`ri.id.e3-companion-file-faults` 不翻。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 e3 超长（omp-grk，2026-11-01）**：已装产品对超长 `e3-identity.tsv` 报 `resource-bytes-length-mismatch`，rc 2，无输出。与尾随同一句。不新增测试调用，不翻 `ri.id.e3-companion-file-faults`。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 翻格移交（omp-grk，2026-11-01）**：11 格映射已 `--check`。已请 cc 在慢车道套用。不写 `tests/`。不翻 R11、R12。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 预处理拒绝（omp-grk，2026-11-01）**：已装产品对 `#define 3x` 和未闭合 `#include <stdio.h` 都编过指令，只在后面报 `not covered: no main`。这是模型缺口，不改 SID/PC/GEN。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 相位换标（omp-grk，2026-11-01）**：`phase-timing-stamps` 62/0。`construction` 冷热和 `output.duration` 冷热已测到正数。热的 inflate 和 inference 是 `skipped-by-reuse`。矩阵仍是 `NEEDS_PRODUCT_CHANGE`。`matrix-flip-gen` 报 `cell-not-missing`。已请 cc 换标。不写 `tests/`。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 地址限制（omp-grk，2026-11-01）**：已装产品 `-c`、`-S`、`-b lnx/x86_64 -o` 在 `ulimit -v 100000` 都印 `run: out of memory`，rc 2，无输出。`ulimit -v 800000` 三路 rc 0，产物与不设限逐字节相同。`res.mem.compile-as-limit` 仍 MISSING：没有已登记批次，L0 没有 setrlimit。不翻格。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 地址限制批次（omp-grk，2026-11-01）**：`c6cf7932f7` 新增 `compile-as-limit`，已装产品 9/0。`res.mem.compile-as-limit` 已翻 `COVERED_LOCAL`。登记 `91698232fa`，覆盖 `6172071792`，矩阵 `b475cec6f5`。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 run 地址限制（omp-grk，2026-11-01）**：`cc91cc5422` 新增 `run-reserve`，已装产品 3/0。低限 `-run` 是 `run: out of memory`，rc 2，stdout 空。前后不设限都返回 7。`res.mem.run-reserve` 已翻 `COVERED_LOCAL`。登记 `a41ab2ec53`，覆盖 `186f5b4428`，矩阵 `d031d42648`。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 地址恢复（omp-grk，2026-11-01）**：`4ce4b1ef5a` 新增 `mem-recovery`，已装产品 15/0。`-c`、`-S`、`-b` 在低限拒绝后，不设限重跑与基线逐字节相同。`res.mem.recovery` 已翻 `COVERED_LOCAL`。登记 `edcaad1588`，覆盖 `4dfdb42385`，矩阵 `7868537fdb`。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 bound 142（omp-grk，2026-11-01）**：`519937fdf1` 新增 `bound-rc142`，2/0。`tools/bound 1 sleep 3` 退出 142，分类行 `CLASSIFY rc142 UNKNOWN`。完成的命令仍是 rc 0。`res.runner.bound-rc142` 已翻 `COVERED_LOCAL`。登记 `33d659a693`，覆盖 `a22c70f77f`，矩阵 `40518c2d07`。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 十二格已翻（omp-grk，2026-11-01）**：`matrix-flip-gen --check` 通过后写入矩阵。negative `f15c603cc4`：`diag.error-limit`、`loader.footer`、`loader.d-row`。RI `3e48b6011b`：hash、sealed header、e3 states、source MISSING、inc-quote、image、multi-tu、front-blob。elf `d248053a8e`：`windows-args-template`。不翻 R11、R12。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 身份行（omp-grk，2026-11-01）**：坏十六进制在 `resources` 79/0 里已是 `resource-identity-malformed`。`stage=zz:...` 仍是 `resource-identity-mismatch`，rc 2，无输出。`ri.frame.identity-malformed` 不翻。改解析会让已绿的 `wrong-stage` 失败，要重装。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 空源（omp-grk，2026-11-01）**：空文件不设账本报 `not covered: no main`。把旧 image 捕获的源码行改成空并改头哈希后，严格重放报 `resource-trace-key-mismatch`，不是空文件对照。`ri.class.source.EMPTY-ABSENT` 不翻。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 属性行（omp-grk，2026-11-01）**：旧 image 捕获里有 `\0cli/attributes`。改成空行后严格重放报 `resource-trace-key-mismatch`，不是 exit 0。`ri.class.attributes.cli` 不翻。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 宿主头（omp-grk，2026-11-01）**：`library-cells` 25/0。stat、timespec、timeval 是 PASS。dirent 仍是 `OBSERVED`，不是与 glibc 布局相等。`lib.layout.host-headers` 不翻。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 公开阻塞（omp-grk，2026-11-01）**：已发信封给 cc。本车道已绿格已翻。公开仍停：`cx-layers` A-INCOMPLETE、外平台、SID 语义拒绝、流运行时要归档源、签名和公开没有授权。不装。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 新四批（omp-grk，2026-11-01）**：会话 `out/tb/sess-new4` 一波 4/4 PASS：`compile-as-limit`、`run-reserve`、`mem-recovery`、`bound-rc142`。身份 STABLE。不是全批会话。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 格数（omp-grk，2026-11-01）**：必需格 718。COVERED_LOCAL 563，MISSING 36，NEEDS_PRODUCT_CHANGE 7，NEEDS_FOREIGN_HOST 73，BLOCKED 26，DECLARED_DEPENDENCY 10，OBSERVED 3。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 三十六格（omp-grk，2026-11-01）**：MISSING 里没有还能在已装产品上单独翻的格。余下是引导链、身份材料、宿主汇总、语义加载器和已记过的账本重放。不重跑已知材料失败。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 产品格（omp-grk，2026-11-01）**：7 个 NEEDS_PRODUCT_CHANGE 里 6 个是相位格，已请 cc 换标。剩下 `ri.windows-snapshot-and-e3-companion` 要 Windows 上的 snapshot 和 E3 伴随，本机不能实跑。不改。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 本机宿主格（omp-grk，2026-11-01）**：`ed6cef27ea` 让宿主格接受已豁免的 win/arm64 `required=no`。已装产品 13/0，`HOST_CELL status=OBSERVED`。三行证据已写。外平台仍 BLOCKED。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 宿主发射（omp-grk，2026-11-01）**：本机测试已打 `HOST_CELL`。`.github/workflows/candidate-native.yml` 没有 `HOST_CELL` 行，且只接受手动 dispatch。不改工作流，不派发。`ci.host-cell-emitter` 不翻。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 宿主会话（omp-grk，2026-11-01）**：登记批次要 `candidate-profile`。当前没有对 `21782dd5` 的 profile。SHA 模式 13/0 不进 `all-waves`。不造 profile。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 资格门（omp-grk，2026-11-01）**：`tests/run.cx -- --all out/tb/sess-2178` 烟测 16/0，聚合拒绝 `session-registry-mismatch` 和 `session-harness-mismatch`。`ALL NOT_QUALIFIED`，rc 2。旧会话对不上当前登记。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 空会话门（omp-grk，2026-11-01）**：`out/tb/sess-gate` 对上当前 harness。`--all` 烟测 16/0。聚合 `pass=0 missing=350 unsatisfied=330`。`ALL NOT_QUALIFIED`，rc 3。不把已知会粘住的失败批跑进这个会话。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话四批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 第一波 4/4 PASS：`compile-as-limit`、`run-reserve`、`mem-recovery`、`bound-rc142`。身份 STABLE。其余 346 个 IMPLEMENTED 批次仍无回执。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话八批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 4/4 PASS：`negative-model-loader`、`resources-limits`、`loader-faults`、`runner-faults`。身份 STABLE。会话里 8 个 PASS 回执。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话十二批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 4/4 PASS：`ri-image-single`、`phase-timing-stamps`、`negative-diagnostics`、`smoke`。身份 STABLE。会话里 12 个 PASS 回执。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话十六批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 4/4 PASS：`elf-x86-image`、`elf-arm64-image`、`link-undefined-function`、`formats-objects`。身份 STABLE。会话里 16 个 PASS 回执。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话二十批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 4/4 PASS：`negative-hostile`、`pp-cli-includes`、`negative-model-loader-terminal`、`formats-elf-ape`。身份 STABLE。会话里 20 个 PASS 回执。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话二十四批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 4/4 PASS：`formats-macho-pe`、`link-malformed-input`、`runcache`、`cx-l0-sys`。身份 STABLE。会话里 24 个 PASS 回执。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话二十八批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 4/4 PASS：`ri-image-multi`、`ri-image-pem`、`elf-winargs`、`cx-package-refusals`。身份 STABLE。会话里 28 个 PASS 回执。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话三十二批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 4/4 PASS：`ratchets-tool`、`ucn-identities`、`input-refuse`、`matrix-waive`。身份 STABLE。会话里 32 个 PASS 回执。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑（omp-grk，2026-11-01）**：`out/tb/sess-gate` `--heavy affected-tool` PASS。身份 STABLE。无材料批已尽，开始重跑 `21782dd5` 上已绿、当前会话还没有回执的材料批。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑四批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`callback-wrapper-index`、`capsule-export`、`com-tree`。加上 `affected-tool`，重跑 4/4。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑六批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`commit-own`、`core-compat`。重跑 6/6。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑十批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`cx-attribute-frame`、`cx-bounded`、`cx-codec`、`cx-core-alu`。重跑 10/10。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑十二批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`cx-core-bytes`、`cx-core-output`。重跑 12/12。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑十六批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`cx-core-sparse`、`cx-core-stack`、`cx-core-stack-empty`、`cx-core-transition`。重跑 16/16。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑二十批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`cx-driver-archive`、`cx-driver-args`、`cx-driver-cli`、`cx-driver-compat`。重跑 20/20。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑二十四批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`cx-driver-deps`、`cx-driver-environment`、`cx-driver-input`、`cx-driver-output`。重跑 24/24。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑二十八批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`cx-driver-path`、`cx-driver-route`、`cx-funit-host`、`cx-host-run`。重跑 28/28。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑三十二批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`cx-l0-arena`、`cx-l0-crypto-aesgcm`、`cx-l0-crypto-b64`、`cx-l0-crypto-bn`。重跑 32/32。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑三十六批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`cx-l0-crypto-chacha20poly1305`、`cx-l0-crypto-ct`、`cx-l0-crypto-der`、`cx-l0-crypto-hkdf`。重跑 36/36。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑四十批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`cx-l0-crypto-p256`、`cx-l0-crypto-rsa`、`cx-l0-crypto-sha256`、`cx-l0-crypto-tls13`。重跑 40/40。身份 STABLE。旧会话已绿、当前还没有回执的还剩 80。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑四十四批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`snapshots-cx`、`cx-source-frame`、`cx-stream-resource`、`cx-stream-output`。重跑 44/44。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑四十八批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`driver-token-target`、`cx-packagefooter`、`cx-unit-graph`、`delta-loader`。重跑 48/48。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑五十二批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`cx0-unit`、`pp-cli-targets`、`pp-cli-macros`、`pp-header-locations`。重跑 52/52。身份 STABLE。旧会话已绿、当前还没有回执的还剩 68。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑五十七批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`kernel-link`、`owned`、`resources`、`ri-trace`、`ri-trace-output`。重跑 57/57。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑六十二批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`ri-multi`、`ri-route-refuse`、`optimization-native`、`optimization-run`、`optimization-tape`。重跑 62/62。身份 STABLE。旧会话已绿、当前还没有回执的还剩 58。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑六十六批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`formats`、`library-abi`、`library-cells`、`stdio-close`。重跑 66/66。身份 STABLE。旧会话已绿、当前还没有回执的还剩 54。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑七十批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`snapshots`、`properties`、`phase-timing-external`、`stream-prune-route`。重跑 70/70。身份 STABLE。旧会话已绿、当前还没有回执的还剩 50。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑七十四批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`link-edge-cases`、`driver-diagnostics`、`cx-stream-attributes`、`cx-model-number`。重跑 74/74。身份 STABLE。旧会话已绿、当前还没有回执的还剩 46。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑七十八批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`cx-library-build-check`、`cx-model-number-runtime`、`cx-package-structure`、`cx-model-payload`。重跑 78/78。身份 STABLE。旧会话已绿、当前还没有回执的还剩 42。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑八十二批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`cx-library-payload`、`cx-model-lifetime`、`cx-model-lifetime-runtime`、`cx-network-structure`。重跑 82/82。身份 STABLE。旧会话已绿、当前还没有回执的还剩 38。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑八十六批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`cx-model-allocation`、`cx-model-reference`、`cx-process-group-osx`、`ledger-parser-compare`。重跑 86/86。身份 STABLE。旧会话已绿、当前还没有回执的还剩 34。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑九十批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`cx-model-decode`、`cx-package-decode`、`cx-package-lifetime`、`cx-l0-text`。重跑 90/90。身份 STABLE。旧会话已绿、当前还没有回执的还剩 30。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑九十四批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`cx-l0-sys-net`、`cx-l0-sys-guard`、`cx-l0-sys-ffi`、`cx-l0-sys-spawn`。重跑 94/94。身份 STABLE。旧会话已绿、当前还没有回执的还剩 26。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑九十八批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`refactor-tool`、`tape-gap-tool`、`cx-l0-fmt`、`register-tool`。重跑 98/98。身份 STABLE。旧会话已绿、当前还没有回执的还剩 22。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑一百零二批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`own-diff-tool`、`symindex-tool`、`cx-l0-sys-sig`、`doc-links-tool`。重跑 102/102。身份 STABLE。旧会话已绿、当前还没有回执的还剩 18。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑一百零六批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`snapshot-run-tool`、`stale-tool`、`tape-neg`、`cx-stream-name`。重跑 106/106。身份 STABLE。旧会话已绿、当前还没有回执的还剩 14。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑一百一十批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`cx-net-name`、`route-chains`、`nametab-gen`、`use-path`。重跑 110/110。身份 STABLE。旧会话已绿、当前还没有回执的还剩 10。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话重跑一百一十四批（omp-grk，2026-11-01）**：`out/tb/sess-gate` 再 PASS：`cx-l0-crypto-x25519`、`cx-l0-crypto-x509`、`out-gc`、`stream-e3-pos`。重跑 114/114。身份 STABLE。旧会话已绿、当前还没有回执的还剩 6。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话实计（omp-grk，2026-11-01）**：旧会话已绿的批已全部重跑进 `out/tb/sess-gate`。`--all` 烟测 16/0。聚合 `pass=152 missing=198 fail=0 unknown=0`，覆盖 `satisfied=177 unsatisfied=153`。`ALL NOT_QUALIFIED`，rc 3。无材料批已尽。剩下的 198 个 IMPLEMENTED 批在这只候选上没有旧 PASS。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 门会话加两批（omp-grk，2026-11-01）**：`census`、`full-domain-check` 在抛弃会话里先绿，再写入 `out/tb/sess-gate`。身份 STABLE。`full-stream-e1/e2/e3/e4` 在抛弃会话里 FAIL，原因是格子 `MISSING`，没有写入正式会话。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 对象材料探针（omp-grk，2026-11-01）**：`out/tb/probe-obj` 用现成 `objects` 材料。`objects-link`、`objects-missing-definition` PASS。身份 STABLE。`out/tb/sess-gate` 的材料表是空的，而且已经锁死，这两批不能补进那个会话。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 对象材料六批（omp-grk，2026-11-01）**：`out/tb/probe-obj` 再 PASS：`objects-mixed-target`、`objects-duplicate-definition`、`objects-old-unit-version`、`objects-unsupported-osx-x86_64`。加上前两批，对象材料 6/6。身份 STABLE。还不能并进 `sess-gate`。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 对象材料九批（omp-grk，2026-11-01）**：`out/tb/probe-obj` 再 PASS：`objects-unsupported-osx-arm64`、`objects-unsupported-win-x86_64`、`objects-unsupported-win-arm64`。对象材料 9/9。身份 STABLE。还不能并进 `sess-gate`。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话（omp-grk，2026-11-01）**：新开会话 `out/tb/sess-qual`，材料表 10 条，从一开始就能带材料。已登记 `objects`。`objects-link`、`census`、`full-domain-check`、`smoke`、`ratchets-tool` PASS。身份 STABLE。`sess-gate` 继续作无材料证据，不并表。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话十一批（omp-grk，2026-11-01）**：`out/tb/sess-qual` 再 PASS：`objects-missing-definition`、`objects-mixed-target`、`compile-as-limit`、`run-reserve`、`mem-recovery`、`bound-rc142`。会话里 11 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话十三批（omp-grk，2026-11-01）**：`out/tb/sess-qual` 再 PASS：`objects-duplicate-definition`、`objects-old-unit-version`。会话里 13 个已确认 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话十七批（omp-grk，2026-11-01）**：`out/tb/sess-qual` 再 PASS：`negative-model-loader`、`resources-limits`、`loader-faults`、`runner-faults`。会话里 17 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话二十二批（omp-grk，2026-11-01）**：`out/tb/sess-qual` 再 PASS：`objects-unsupported-osx-x86_64`、`ri-image-single`、`phase-timing-stamps`、`negative-diagnostics`、`elf-x86-image`。会话里 22 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话二十七批（omp-grk，2026-11-01）**：`out/tb/sess-qual` 再 PASS：`objects-unsupported-osx-arm64`、`elf-arm64-image`、`link-undefined-function`、`formats-objects`、`pp-cli-includes`。会话里 27 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话三十二批（omp-grk，2026-11-01）**：`out/tb/sess-qual` 再 PASS：`objects-unsupported-win-x86_64`、`negative-hostile`、`negative-model-loader-terminal`、`formats-elf-ape`、`formats-macho-pe`。会话里 32 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话三十七批（omp-grk，2026-11-01）**：`out/tb/sess-qual` 再 PASS：`objects-unsupported-win-arm64`、`link-malformed-input`、`runcache`、`cx-l0-sys`、`ri-image-multi`。对象材料 9/9 已在这个会话里。会话里 37 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话四十五批（omp-grk，2026-11-01）**：`out/tb/sess-qual` 再 8/8 PASS：`ri-image-pem`、`elf-winargs`、`cx-package-refusals`、`ucn-identities`、`input-refuse`、`matrix-waive`、`affected-tool`、`core-compat`。会话里 45 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话五十三批（omp-grk，2026-11-01）**：`out/tb/sess-qual` 再 8/8 PASS：`cx-attribute-frame`、`cx-bounded`、`cx-codec`、`cx-core-alu`、`cx-core-bytes`、`cx-core-output`、`cx-core-sparse`、`cx-core-stack`。会话里 53 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话六十一批（omp-grk，2026-11-01）**：`out/tb/sess-qual` 再 8/8 PASS：`cx-core-stack-empty`、`cx-core-transition`、`cx-driver-archive`、`cx-driver-args`、`cx-driver-cli`、`cx-driver-compat`、`cx-driver-deps`、`cx-driver-environment`。会话里 61 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话六十九批（omp-grk，2026-11-01）**：`out/tb/sess-qual` 再 8/8 PASS：`cx-driver-input`、`cx-driver-output`、`cx-driver-path`、`cx-driver-route`、`cx-l0-arena`、`cx-l0-crypto-aesgcm`、`cx-l0-crypto-b64`、`cx-l0-crypto-bn`。会话里 69 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话七十七批（omp-grk，2026-11-01）**：`out/tb/sess-qual` 再 8/8 PASS：`cx-l0-crypto-chacha20poly1305`、`cx-l0-crypto-ct`、`cx-l0-crypto-der`、`cx-l0-crypto-hkdf`、`cx-l0-crypto-p256`、`cx-l0-crypto-rsa`、`cx-l0-crypto-sha256`、`cx-l0-text`。会话里 77 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话八十五批（omp-grk，2026-11-01）**：`out/tb/sess-qual` 再 8/8 PASS：`cx-l0-crypto-tls13`、`cx-l0-crypto-x25519`、`cx-l0-crypto-x509`、`cx-l0-fmt`、`cx-l0-sys-net`、`cx-l0-sys-guard`、`cx-l0-sys-ffi`、`cx-l0-sys-spawn`。会话里 85 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话九十三批（omp-grk，2026-11-01）**：`out/tb/sess-qual` 再 8/8 PASS：`cx-model-allocation`、`cx-model-decode`、`cx-model-lifetime`、`cx-model-number`、`cx-model-number-runtime`、`cx-model-payload`、`cx-model-reference`、`cx-network-structure`。会话里 93 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话一百零一批（omp-grk，2026-11-01）**：`out/tb/sess-qual` 再 PASS：`callback-wrapper-index`、`capsule-export`、`com-tree`、`commit-own`、`cx-funit-host`、`cx-l0-sys-sig`、`cx-library-build-check`。`cx-host-run` 第一次被 24 秒看门狗标 UNKNOWN，加长后重跑 PASS。会话里 101 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话一百零九批（omp-grk，2026-11-01）**：`out/tb/sess-qual` 再 8/8 PASS：`cx-library-payload`、`cx-model-lifetime-runtime`、`cx-net-name`、`cx-package-decode`、`cx-package-lifetime`、`cx-package-structure`、`cx-packagefooter`、`cx-process-group-osx`。会话里 109 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话一百一十七批（omp-grk，2026-11-01）**：`out/tb/sess-qual` 再 8/8 PASS：`cx-source-frame`、`cx-stream-attributes`、`cx-stream-name`、`cx-stream-output`、`cx-stream-resource`、`cx-unit-graph`、`cx0-unit`、`delta-loader`。会话里 117 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话一百二十五批（omp-grk，2026-11-01）**：`out/tb/sess-qual` 再 8/8 PASS：`doc-links-tool`、`driver-diagnostics`、`driver-token-target`、`e3-warnings`、`formats`、`kernel-link`、`ledger-parser-compare`、`library-abi`。会话里 125 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话一百三十三批（omp-grk，2026-11-01）**：`out/tb/sess-qual` 再 8/8 PASS：`library-cells`、`link-edge-cases`、`matrix-stale`、`nametab-gen`、`optimization-native`、`optimization-run`、`optimization-tape`、`out-gc`。会话里 133 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话一百四十一批（omp-grk，2026-11-01）**：`out/tb/sess-qual` 再 8/8 PASS：`own-diff-tool`、`owned`、`phase-timing-external`、`pp-cli-macros`、`pp-cli-targets`、`pp-header-locations`、`properties`、`refactor-tool`。会话里 141 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话一百四十九批（omp-grk，2026-11-01）**：`out/tb/sess-qual` 再 8/8 PASS：`register-tool`、`resources`、`ri-multi`、`ri-route-refuse`、`ri-trace`、`ri-trace-output`、`route-chains`、`snapshot-run-tool`。会话里 149 个 PASS。还差 14 个旧绿批没重跑。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话重跑完成（omp-grk，2026-11-01）**：`out/tb/sess-qual` 再 14/14 PASS，旧无材料会话的 154 个 PASS 已全部重跑进来。另有对象材料 9/9。会话里 163 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话 nets（omp-grk，2026-11-01）**：`nets` 先在 `out/tb/probe-nets` 绿，再登记进 `out/tb/sess-qual` 并 PASS。会话里 164 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话 pp-gold（omp-grk，2026-11-01）**：`pp-source-constructed`、`pp-gold-controls` 先在 `out/tb/probe-pp` 绿，再登记进 `out/tb/sess-qual` 并 PASS。会话里 166 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话 kernel-route（omp-grk，2026-11-01）**：`kernel-route-encoding` 先在 `out/tb/probe-kr` 绿，再登记进 `out/tb/sess-qual` 并 PASS。会话里 167 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话 kernel-route 三批（omp-grk，2026-11-01）**：`kernel-route-relocation`、`kernel-route-object` 先在探针绿，再写入 `out/tb/sess-qual` PASS。会话里 169 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话 kernel-route 五批（omp-grk，2026-11-01）**：`kernel-route-refusal`、`kernel-route-route-refusal` 先在探针绿，再写入 `out/tb/sess-qual` PASS。会话里 171 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话 kernel-route 七批（omp-grk，2026-11-01）**：`kernel-route-route-recovery`、`kernel-route-pin-mutation` 先在探针绿，再写入 `out/tb/sess-qual` PASS。会话里 173 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话 kernel-route 九批（omp-grk，2026-11-01）**：`kernel-route-coverage`、`kernel-route-pins` 先在探针绿，再写入 `out/tb/sess-qual` PASS。kernel-route 九批都在这个会话里。会话里 175 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话 ri-chain（omp-grk，2026-11-01）**：`ri-chain-valid` 先在 `out/tb/probe-ri` 绿，再登记进 `out/tb/sess-qual` 并 PASS。`ri-chain-nnflip` 探针已绿，还没写入正式会话。会话里 176 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话 ri-chain 四批（omp-grk，2026-11-01）**：`ri-chain-nnflip`、`ri-chain-tableflip`、`ri-chain-receiptflip` 写入 `out/tb/sess-qual` PASS。会话里 179 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话 ri-chain 六批（omp-grk，2026-11-01）**：`ri-chain-missingnet`、`ri-chain-symlinknet` 写入 `out/tb/sess-qual` PASS。`ri-chain-fiforeceipt` 探针已绿，还没写入正式会话。会话里 181 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话 ri-chain 七批（omp-grk，2026-11-01）**：`ri-chain-fiforeceipt` 写入 `out/tb/sess-qual` PASS。ri-chain 七批都在这个会话里。会话里 182 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话 hostar（omp-grk，2026-11-01）**：`formats-archive-ar` 先在 `out/tb/probe-host` 绿，再登记 `hostar` 进 `out/tb/sess-qual` 并 PASS。会话里 183 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话 hostld（omp-grk，2026-11-01）**：`formats-object-ld` 先在探针绿，再登记 `hostld` 进 `out/tb/sess-qual` 并 PASS。会话里 184 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话 e2model（omp-grk，2026-11-01）**：`cx0-bundle` 先在 `out/tb/probe-e2` 绿，再登记 `e2model` 进 `out/tb/sess-qual` 并 PASS。`pp-negative` 在宿主 gcc 探针里 FAIL（`macro-name-not-identifier`、`header-name-unclosed`），没有写入正式会话。会话里 185 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料探针拒绝（omp-grk，2026-11-01）**：`assembler-cache` 在现成 `kernels` 上 FAIL（`executable-digest-refusal`，`kernel-build: source identity failed`）。`net-controls` 在 `probe-nets` 里材料中途变了，FAIL。两批都没有写入 `sess-qual`。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 c-negative 探针（omp-grk，2026-11-01）**：`c-negative` 在宿主 gcc 上 FAIL，23 条产品拒绝没盖住，包括 `assign-to-const`、`case-outside-switch`、`struct-redefinition`。没有写入 `sess-qual`。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 材料会话被拒（omp-grk，2026-11-01）**：`tests/run.cx -- --all out/tb/sess-qual` 烟测 16/0，聚合 `REFUSE session-material-changed nets`，rc 2。`out/tb/mat-nets/netcore.cx` 在登记后被改过（mtime 06:08）。这个会话不能再当资格门。`sess-gate` 仍是无材料的 154 PASS 证据。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 无材料门复测（omp-grk，2026-11-01）**：`out/tb/sess-gate` `--all` 烟测 16/0。聚合 `pass=154 missing=196 fail=0 unknown=0 refused=0`，覆盖 `satisfied=179 unsatisfied=151`。`ALL NOT_QUALIFIED`，rc 3。已把当时的 `mat-nets` 复制到 `out/tb/frozen-nets-20261101`，避免再被共享树改掉。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 冻结材料会话（omp-grk，2026-11-01）**：新开会话 `out/tb/sess-frozen`，材料是私有副本，不指共享 `mat-nets`。已登记 `objects`。`objects-link` PASS。身份 STABLE。旧的 `sess-qual` 仍是被拒会话，不再当门。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**会话进度明细已归档**：omp-grk 2026-11-01 的 frozen/clean-nets/full-material 逐批打卡（共 49 行）移至 [archive/plans/release-0.0.43-session-log-20261101.md](../archive/plans/release-0.0.43-session-log-20261101.md)；当前只保留该摘要，最终全材料会话 87 PASS，仍未 QUALIFIED、未签名、未公开。
STATE43：**21782dd5 冻结材料四份登记（omp-grk，2026-11-01）**：`out/tb/sess-frozen` 再登记冻结副本 `nets`、`pp-gold`、`kernel-route`、`ri-chain`，都是 PREPARED-identity-only。`nets` PASS，`pp-gold-controls` PASS。会话里 149 个 PASS。`net-controls` FAIL，原因 `material-changed-during-batch`，这份 `nets` 副本被该批改过，后续依赖 `nets` 的批不能再在这个会话跑。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 第二份 nets 会话（omp-grk，2026-11-01）**：`out/tb/sess-nets2` 用未再改的 `out/tb/frozen-nets-20261101` 登记 `nets`。内容哈希与 `sess-frozen` 登记时相同（`c3962ea4`），指纹不同因为路径不同。`nets` PASS。不跑 `net-controls`，避免再改这份副本。这是另一份会话，不并进 `sess-frozen` 的 177 个 PASS。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 干净 nets 会话九批（omp-grk，2026-11-01）**：`out/tb/sess-nets2` 再 8/8 PASS：`census`、`full-domain-check`、`smoke`、`ratchets-tool`、`compile-as-limit`、`run-reserve`、`mem-recovery`、`bound-rc142`。加上 `nets`，9 个 PASS。不跑 `net-controls`。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 干净 nets 会话十七批（omp-grk，2026-11-01）**：`out/tb/sess-nets2` 再 8/8 PASS：`negative-model-loader`、`resources-limits`、`loader-faults`、`runner-faults`、`elf-x86-image`、`elf-arm64-image`、`link-undefined-function`、`formats-objects`。会话里 17 个 PASS。不跑 `net-controls`。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 干净 nets 会话二十五批（omp-grk，2026-11-01）**：`out/tb/sess-nets2` 再 8/8 PASS：`pp-cli-includes`、`ri-image-single`、`ri-image-multi`、`ri-image-pem`、`elf-winargs`、`cx-package-refusals`、`ucn-identities`、`input-refuse`。会话里 25 个 PASS。不跑 `net-controls`。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 干净 nets 会话三十三批（omp-grk，2026-11-01）**：`out/tb/sess-nets2` 再 8/8 PASS：`matrix-waive`、`cx-bounded`、`cx-codec`、`cx-core-alu`、`cx-core-bytes`、`cx-core-output`、`cx-core-sparse`、`cx-core-stack`。会话里 33 个 PASS。不跑 `net-controls`。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 干净 nets 会话四十一批（omp-grk，2026-11-01）**：`out/tb/sess-nets2` 再 8/8 PASS：`cx-core-stack-empty`、`cx-core-transition`、`cx-driver-archive`、`cx-driver-args`、`cx-driver-cli`、`cx-driver-compat`、`cx-driver-deps`、`cx-driver-environment`。会话里 41 个 PASS。不跑 `net-controls`。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 干净 nets 会话四十九批（omp-grk，2026-11-01）**：`out/tb/sess-nets2` 再 8/8 PASS：`cx-driver-input`、`cx-driver-output`、`cx-driver-path`、`cx-driver-route`、`cx-l0-arena`、`cx-l0-text`、`cx-l0-fmt`、`cx-l0-sys`。会话里 49 个 PASS。不跑 `net-controls`。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 干净 nets 会话五十七批（omp-grk，2026-11-01）**：`out/tb/sess-nets2` 再 8/8 PASS：`cx-l0-crypto-b64`、`cx-l0-crypto-bn`、`cx-l0-crypto-ct`、`cx-l0-crypto-der`、`cx-l0-crypto-hkdf`、`cx-l0-crypto-sha256`、`cx-l0-sys-net`、`cx-l0-sys-guard`。会话里 57 个 PASS。不跑 `net-controls`。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 干净 nets 会话六十一批（omp-grk，2026-11-01）**：`out/tb/sess-nets2` 再 4/4 PASS：`cx-l0-crypto-tls13`、`cx-l0-crypto-x25519`、`cx-l0-crypto-x509`、`cx-l0-sys-ffi`。会话里 61 个 PASS。不跑 `net-controls`。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 干净 nets 会话六十九批（omp-grk，2026-11-01）**：`out/tb/sess-nets2` 再 8/8 PASS：`cx-l0-sys-spawn`、`cx-model-allocation`、`cx-model-decode`、`cx-model-lifetime`、`cx-model-number`、`cx-model-number-runtime`、`cx-model-payload`、`cx-model-reference`。会话里 69 个 PASS。不跑 `net-controls`。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 干净 nets 会话七十七批（omp-grk，2026-11-01）**：`out/tb/sess-nets2` 再 8/8 PASS：`cx-net-name`、`cx-network-structure`、`cx-package-decode`、`cx-package-lifetime`、`cx-package-structure`、`cx-packagefooter`、`cx-process-group-osx`、`cx-source-frame`。会话里 77 个 PASS。不跑 `net-controls`。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 干净 nets 会话八十五批（omp-grk，2026-11-01）**：`out/tb/sess-nets2` 再 8/8 PASS：`cx-stream-attributes`、`cx-stream-name`、`cx-stream-output`、`cx-stream-resource`、`cx-unit-graph`、`cx0-unit`、`delta-loader`、`doc-links-tool`。会话里 85 个 PASS。不跑 `net-controls`。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 干净 nets 会话九十三批（omp-grk，2026-11-01）**：`out/tb/sess-nets2` 再 8/8 PASS：`driver-diagnostics`、`driver-token-target`、`e3-warnings`、`formats`、`kernel-link`、`ledger-parser-compare`、`library-abi`、`library-cells`。会话里 93 个 PASS。不跑 `net-controls`。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 干净 nets 会话一百零一批（omp-grk，2026-11-01）**：`out/tb/sess-nets2` 再 8/8 PASS：`link-edge-cases`、`matrix-stale`、`nametab-gen`、`optimization-native`、`optimization-run`、`optimization-tape`、`out-gc`、`owned`。会话里 101 个 PASS。不跑 `net-controls`。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 干净 nets 会话一百零九批（omp-grk，2026-11-01）**：`out/tb/sess-nets2` 再 8/8 PASS：`phase-timing-external`、`pp-cli-macros`、`pp-cli-targets`、`pp-header-locations`、`properties`、`refactor-tool`、`register-tool`、`resources`。会话里 109 个 PASS。不跑 `net-controls`。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 干净 nets 会话一百一十七批（omp-grk，2026-11-01）**：`out/tb/sess-nets2` 再 8/8 PASS：`ri-multi`、`ri-route-refuse`、`ri-trace`、`ri-trace-output`、`route-chains`、`snapshot-run-tool`、`stale-tool`、`tape-neg`。会话里 117 个 PASS。不跑 `net-controls`。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 干净 nets 会话一百二十七批（omp-grk，2026-11-01）**：`out/tb/sess-nets2` 再 10/10 PASS：`tape-gap-tool`、`use-path`、`symindex-tool`、`own-diff-tool`、`stream-e3-unmap`、`stream-prune-route`、`stream-e3-gold`、`stream-e3-pos`、`stream-e3-ref`、`stream-e3-scope`。会话里 127 个 PASS。不跑 `net-controls`。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 干净 nets 会话一百三十五批（omp-grk，2026-11-01）**：`out/tb/sess-nets2` 再 8/8 PASS：`callback-wrapper-index`、`com-tree`、`cx-l0-sys-sig`、`core-compat`、`cx-l0-crypto-chacha20poly1305`、`cx-l0-crypto-p256`、`cx-l0-crypto-aesgcm`、`cx-l0-crypto-rsa`。会话里 135 个 PASS。不跑 `net-controls`。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 干净 nets 会话一百四十批（omp-grk，2026-11-01）**：`out/tb/sess-nets2` 再 5/5 PASS：`formats-elf-ape`、`negative-diagnostics`、`negative-model-loader-terminal`、`stdio-close`、`phase-timing-stamps`。会话里 140 个 PASS。不跑 `net-controls`。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 干净 nets 会话一百四十二批（omp-grk，2026-11-01）**：`out/tb/sess-nets2` 再 PASS：`snapshots`、`negative-hostile`。会话里 142 个 PASS。不跑 `net-controls`。身份 STABLE。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 干净 nets 官方门（omp-grk，2026-11-01）**：再 4/4 PASS：`cx-funit-host`、`cx-library-build-check`、`cx-library-payload`、`capsule-export`。`tests/run.cx -- --all out/tb/sess-nets2` 烟测 16/0。聚合 `pass=146 missing=204 fail=0 unknown=0 refused=0`，覆盖 `satisfied=188 unsatisfied=142`。`ALL NOT_QUALIFIED`，rc 1。会话仍被接受，`nets` 没被改。不跑 `net-controls`。还不是 QUALIFIED。没有签名，没有公开。
STATE43：**21782dd5 干净 nets 会话一百四十八批（omp-grk，2026-11-01）**：再 PASS：`commit-own`、`cx-host-run`。官方聚合 `pass=148 missing=202 fail=0 refused=0`，覆盖 `satisfied=190 unsatisfied=140`。会话仍被接受。不跑 `net-controls`。还不是 QUALIFIED。没有签名，没有公开。

STATE43：已装 0.0.43 = `fc1d4513`（10-31 14:19，5,025,194 字节，五片都是整程序 -o，冻结 rev cd1a95ebcc，P3 aec1a0bf；上一版 43fbbb81 备份在 out/unisacc.com.43fbbb81，再上一版 79fb84e6 在 out/）。结构和体积用 `tools/com-tree.cx` 看。
STATE43：[x] 1 hostnative（28d0cba49c）。
STATE43：[x] 2 构造链：release-build 五片、release-ape 打包与冒烟、安装（check-install-fixes 4/4，`--smoke` 16/16，冒烟 5/5）。
STATE43：[x] 3 **RI 迁移**（fb89faddde 防呆、db6938d868 严格重放与捕获、fbd9cb64c7 与 f6c3ef3dd0 trace 与 multi）：已装的 fc1d4513 上，tools/affected.cx 跑 resources、ri-trace、ri-trace-output、ri-multi 四个批次 4/4 PASS（断言 74、15、17、10，全部 0 失败），cc 独立复跑确认。旧产品对同一测试 rc2、捕获不写文件，对照成立。rf-capture-guard 作废（见文末），不登记。
STATE43：[x] 4 **两代比较，五片全部逐字节相同**（冻结 rev cd1a95ebcc；第一代构造器 79fb84e6，第二代构造器已装的 fc1d4513，各片 tools/bound 60 内编完，空闲机器上只跑一次）：lnx/x86_64 0ae1986b（53.2 秒）、lnx/arm64 14abd693（54.9 秒）、osx/x86_64 d4e43786（56.8 秒）、osx/arm64 16b6dd58（58.9 秒，离界只差 1 秒）、win/x86_64 1b0c3353（39.0 秒）。cc 对五对文件逐一 cmp 确认。注意 osx 两片已经贴着 60 秒界，构造器再慢就编不完，这是 C44-SPEED-01 的急事。
STATE43：**待产品改 20 格分诊（cc，2026-10-31，grk 列的清单）**：
STATE43：(a) 小改，攒成一个候选一起做（0.0.43 内）：resources 2 格（UNISA_MAXSTEPS 的 abc、-1、0 要按名字拒绝；输出限额比镜像小要报错而不是留下截断文件还报成功）、phase-timing 7 格（构造冷热、热路径写明跳过了模型解压和推理、输出耗时冷热，都是往 RR_TIME 里补行，不能拖慢）、library 的 invalid-options 1 格（私有运行时已修，要进产品）。
STATE43：(a+) cc 在已装 fc1d4513 上实测又找到两个产品缺口，加进 (a)：① 编译时地址空间限制（ulimit -v）低于约 225 MB，产品段错误（rc 139，旧 79fb84e6 也一样）；格要求具名拒绝 `run: out of memory` rc2、没有输出文件，信号死亡算失败（res.mem.compile-as-limit）；250 MB 以上都正常。② `-run` 的用户程序在地址空间限制下 malloc 失败时，运行时崩溃（rc 139），格要求程序看到 NULL 并用自己的退出码退出（res.mem.user-malloc-null，复现：ulimit -v 1500000，程序 malloc(3GB)）。
STATE43：(b) 位置映射 2 格（stream-e3 与 negative 各 1 格：宏展开里的错误，列号要指到用户源码上的列，现在钉在展开后的 5:34，合同要 5:24）。grk 估了大小：约一天，要改公开格式 UNIPP1（头上多一个计数，宏替换时记「展开偏移对上源偏移」）、parse2 的 tokenlocations 十二张、diagnostics 五张、pp 的 location 三张，诊断摘录要改读原始源，改完 SID 会变，不是快车道。**待项目所有者定**（和 (c)(d) 一起）。
STATE43：(c) 库产品（libunisacc）3 格：递归回调签名、union/位域/long double 过 API、恰好导出 29 个 US_API 符号的产物。活实现缺失，是整块工作，**待项目所有者定范围**。
STATE43：(d) win/arm64 原生 6 格（host.phase-report、host.diag-and-loader-corpus、ci.native、ri.windows-snapshot-and-e3-companion 的 win/x86_64 与 win/arm64 两条、E12.native-chain）：产品现在发布的五个目标是 win/x86_64、lnx/x86_64、lnx/arm64、osx/x86_64、osx/arm64，没有原生 ARM64 PE；矩阵要求的外平台主机集合含 win/arm64。**待项目所有者定：做原生 ARM64 PE，还是把必需主机集合改成产品真正发布的五个目标。** 在项目所有者定之前不改矩阵。
STATE43：**范围决定（2026-10-31，项目所有者原话「你来安排」，cc 采纳下面的评估）：0.0.43 以五片单文件 unisacc.com 的范围发布；A、B、C 三组明文列为已知缺口，各挪到 0.0.44（C44-LIBUNISACC-01、C44-WIN-ARM64-01、C44-DIAG-COLUMN-01）。矩阵里 54 条豁免记录在 tests/waivers-0.0.43.tsv，应用后必需格 825 变 768，已覆盖 509，缺 136，待外平台 73，待产品改 9；由 tools/matrix-waive.cx --check 保证记录和矩阵一致；发布说明用 [release-0.0.43-known-gaps.md](release-0.0.43-known-gaps.md)。**
STATE43：**范围评估原文（fable 子代理只读评估，cc 核对了其中两处事实）**
STATE43：A libunisacc → 建议明文列为已知缺口挪到 0.0.44。依据：公开发布物历来只有 unisacc.com（release/README.md:17、archive/docs-2026-11/research/r42-release-acceptance.json:151-154）；库矩阵头写明它测的是宿主 GCC 加 libffi 编的私有 C 库、没有 recipe 造 libunisacc 产物（tests/library-domain-matrix.tsv:2-7）；tests/p3-library-members.cx:3-4 写明已装 APE 没有库路线；活实现随旧 C 归档，头文件自己写着递归回调、union/位域/宽浮点仍在开发中（exec/c/libunisacc.h:20-25）；工作量以周计，会重开冻结候选。发布说明一句：libunisacc 嵌入式 API 为开发期接口，未随 0.0.43 发布。cc 核对：cx-library-build-check 批次在已装产品上是 PASS，评估里「应当会失败」的推断不成立。
STATE43：B win/arm64 → 建议挪 0.0.44，并把矩阵必需主机集合改成产品真实发布的五片（需项目所有者批准）。依据：论文 A 的五目标（含 win/arm64）是自举身份测量过的宿主集合，产品五片是另一个集合，论文自己写清了（main.tex:90、191）；spec T-5 是六个编译目标；seed/ape.c、ape-pack.cx、release-build.cx、CI（.github/workflows/candidate-native.yml:63-68）都已按「Windows on ARM 跑 x86_64 PE 模拟」设计，矩阵是唯一按原生 ARM64 PE 写的地方；矩阵里含 win/arm64 的约 22 行，不止 6 格；做原生要 ARM64X 混合 PE 或另发 exe，约 1 至 2 周并触及签名链。ri.windows-snapshot-and-e3-companion 的 win/x86_64 一条与 ARM64 无关，是 Windows 快照桩。建议改法是 required=no 加明文理由，不删行。发布说明一句：Windows on ARM64 运行 x86_64 PE（系统模拟），原生入口为 0.0.44 目标；-b win/arm64 写出的用户镜像是原生 ARM64。
STATE43：C 宏展开列号 → 建议挪 0.0.44，发布说明写一句。依据：spec.md 里没有 A-40 正文条款，只有 197 行脚本摘要一行；exec/pipeline/formats.md:126-128 明文禁止按 token 文本回找位置，所以没有不触 SID 的小做法；唯一的做法是 plans/dev-methodology-2026-10-28.md:60 的三步，约一天加一次纪元换证。现状行号正确，列号和摘录行按展开后文本。发布说明一句：宏展开内的诊断列号与摘录行按展开后文本计数（行号正确），0.0.44 改为用户源码位置。
STATE43：(a) 的更正：library invalid-options 不放进 (a)（产品里没有库路线，随 A 走）；phase-timing 在 lnx 上是 6 格（host.phase-report 的 win/arm64 一格属于 B）；resources MAXSTEPS 一格，runexec.cx:11,71 里有具名拒绝文本，但 cc 在已装 fc1d4513 上实测 UNISA_MAXSTEPS 为 abc、-1、0 时 -run 照常 rc7、-S 照常 rc0，没有拒绝，所以要查那段检查为什么没走到。
STATE43：[ ] 5 object 模型步（lnx 回到 33 对象依赖 C44-ENTRY-HOSTIMPORT-01，不挡 0.0.43）。
STATE43：[ ] 6 **测试到 QUALIFIED**。判定（tests/run.cx:28-33）：`tests/run.cx -- --all` = 本地 smoke 加 `tests/all-aggregate.cx` 对一次 `tools/all-waves.cx` 会话的汇总，要求每个 IMPLEMENTED 批次在**同一个候选**上有 PASS 回执，且没有未满足或未解决的覆盖行。矩阵头也写着：COVERED_LOCAL 的证据是候选专属的，旧候选（@94b549d1、@da4e7ef1、79fb84e6 等标签）的证据只是历史，不盖当前候选。所以步骤（cc 2026-10-31 理的）：
STATE43：6a 把还缺的格做完：当前必需 736 格，已覆盖 543，缺 73，待产品改 7，待外平台 73（10-31 傍晚；grk 在做 elf.pos、lower.pos 和普查红批次的分类，我做 resources、ri、interfaces、negative、library 剩余）。豁免记录 R1–R10，见 tests/waivers-0.0.43.tsv。
STATE43：**普查（10-31，tools/census.cx，报告 [census-2026-10-31.md](../archive/plans/2026-11-01/census-2026-10-31.md)）**：已装 fc1d4513 上 166 个无材料批次，120 PASS，46 FAIL（19 个 full-* 汇总 + 27 个真红）。真红里：ri-image-single/multi/pem 是 RI 迁移漏了 image 路线（source 阶段 + e3-identity.tsv），已判 A 补进最后候选；negative-model-loader 的措辞/退出码已改到新模型驱动，仍有 3 处加载器接受坏包（页脚长度翻位、D 行阶段名、terminal elf 行）是产品缺口；negative-diagnostics 是 -ferror-limit 被忽略；其余 formats-objects/formats-tapebin/link-*/pp-*/ucn-identities 等待 grk 分类 A/B/C；cx-* 六个待 bdy 判定。引用红批次的矩阵格已降 STALE-RED（matrix-stale），批次转绿后翻回。批次 negative-model-qprefix 已退役（依赖已归档的 run.c），格 loader.qprefix-malformed 并入 R10。
STATE43：6b 最后一个候选要收的产品改动（10-31 晚，按风险排序，每项走快车道、对应红批次转绿为验收、≤1 小时，做不动就豁免进 0.0.44）：① link-undefined-function：直接 -o 用到未定义函数时退出 0、出镜像、运行时 127（悬空成功）；② image 路线补 source 阶段 + e3-identity.tsv（ri-image-single/multi/pem）；③ 加载器校验页脚长度、D 行阶段边界、terminal elf 行（negative-model-loader、-terminal；cx/runtime 里，不加 drive.cx）；④ -ferror-limit=N 生效（negative-diagnostics）；⑤ -nostdinc 不再展开自带头（pp-cli-includes）；⑥ **6a（新发现，最高）已装产品不接受 .tape/.tapebin 作 -run 与 -b … -o 的输入（79fb84e6 rc7，现报 top-level construct，磁带被当成 C 送进前端），link-malformed-input 的坏磁带断言因此是假绿**；6b -S --tapebin 写 UTAPEBIN（formats-tapebin；cx 里没有编码器，一小时接不上就豁免 tapebin.roundtrip 进 C44-TAPEBIN-01）；⑦ -run bad.tape 报 unknown opcode（link-malformed-input）；⑧ phase-timing 六个 lnx 机内打点；⑨ 启动器 ulimit -v 预检（res.mem.compile-as-limit）。普查九批分类已出：formats-objects、pp-cli-macros、pp-header-locations、ucn-identities 是测试钉着旧退出码/句子（A，已改，断言条数不减，提交 3281ba27c2）。**这是最后一个候选**：之后冻结，冻结后对同一候选再跑一次普查。已决定不做：user malloc（R5）、MAXSTEPS 校验（R6）、-Werror（C44-WERROR-01）。
STATE43：6c 实现 19 个 `full-*` 汇总批次（现在是 PLANNED、NOT_READY，tests/pending 里文件还不存在；每个负责一个域：该域所有必需格都是 COVERED_LOCAL，外平台格都有同候选的 CI 观察）。cc 来写一个通用的 `tests/full-domain.cx DOMAIN`，读矩阵、逐格报哪些没盖上。
STATE43：6d 对冻结候选跑一次完整的 `all-waves` 会话：314 个 IMPLEMENTED 批次（其中有一批要先准备材料，176 个标 UNPREPARED），每个批次一次作业 60 秒内，轻量批次一轮四个并发；然后用 `matrix-flip-gen --migrate` 把证据标签从旧候选迁到冻结候选。这一步会跑一两个小时，机器要独占。
STATE43：6e 五个外平台（osx/x86_64、osx/arm64、lnx/arm64、win/x86_64 加 CI 上的 lnx）在同一候选上实机观察，需要把候选推到 CI（项目所有者已批准推送）；73 格。
STATE43：6f 最后 `tests/run.cx -- --all` 输出 QUALIFIED。
STATE43：[ ] 7 项目所有者授权发布（签名、Draft、Latest、公开）。目前一律不碰，等 0.0.43 做完再安排。
STATE43：不计入 0.0.43 完成度：名表铺设（nametab-gen 290 行规格、约 580 个模块，产品还没用）、通用工具、体积优化，都是 0.0.44 的准备，见 [v0.0.44.md](../archive/research-2026-11d/plans/v0.0.44.md)。名表铺设已于 2026-11-02 停止生成并删除（零消费者，字节等价验证 entry.elf 648a47ca，见 [w4-dedupe-2026-11-02.md](../archive/plans-2026-11d/w4-dedupe-2026-11-02.md)）。

**已安装（2026-10-31 01:19）：** `./unisacc.com` sha=43fbbb81…，4631130 字节，冻结 rev c98f757f，五片都走 whole-o，P3 是 aec1a0bf；check-install-fixes 4/4、`--smoke` 16/16、release-ape smoke 5/5、七个棘轮全绿。旧版备份在 out/unisacc.com.79fb84e6。

**头号待办（cc 复核时发现，挡第 4 步 QUALIFIED）：** CX 入口还没迁移资源身份（RI）。
- `--strict-resource-flow` 和 `--resource-ledger` 报 "model driver: option not migrated"（src/entrybind.cx:86）。
- 设置了 `UNISACC_RF_CAPTURE`，编译照常 rc0，却一个文件都不写，等于悄悄绕过账本，违反 ri 合同。
- resources、ri-trace、ri-trace-output、ri-multi 四个批次在新产品上全红。ri、resources 两个矩阵里的 COVERED_LOCAL 证据都是在 79fb84e6 上实跑的，新产品上不成立。
要做的：把 RI 的选项、捕获和严格重放迁移进 CX 入口；在迁移完成之前，带 `UNISACC_RF_CAPTURE` 的运行必须按名字拒绝，不能悄悄忽略。
已作废：rf-capture-guard 是 RI 迁移完成之前的防呆（设了 UNISACC_RF_CAPTURE 就按名字拒绝），捕获迁移之后它的断言不再成立（设了就真的捕获；空值等于没设，和 resources.cx 的用法一致），2026-10-31 删除，不登记；捕获的拒绝路径由 resources 批次覆盖。

**裁定（cc，2026-10-31，项目所有者授权 cc 决定）：先做完发布链，CX 面的广度铺设移到 0.0.44（C44-CX-SURFACE-01）。** 已经落地的 stream-gen、net-gen、sys.fd 保留。从现在起，grk 只推第 2 步剩下的部分，顺序是：osx/x86_64 和 win/x86_64 收进 release-build 并做第二代 → lnx/arm64 和 osx/arm64 两片 → seed/ape 包装 → 安装（`check-install-fixes` 四项全绿）；然后是第 3、4 步。src/stream.cx 和 src/network.cx 在安装之后用 `--write-src` 收尾。

- [x] 1 hostnative 28d0cba49c（sy_reg_size 与 check-cx-sys 那一行 sy_eprintf 在 08da2e8863，索引竞态先被别的提交带走；precommit blocked=0）
- [ ] 2 release-build：P3 / 五片 / wrap / 二代 / 安装
  lnx/x86_64 一片已跑通，route=33 个对象。P3 只拼在 candidate 上，entry.elf 是链接结果。osx/win、其余四片、wrap、安装未开始。candidate 直接 --version 打印 unisacc 0.0.43，rc=0。runcache 未跑。
  p3 sha=aec1a0bf27b9d1f2e26a1b86b52cb3694bc66a906415bcc0435c11278c6deabf bytes=2039850 flat=match ns=373396397
  entry.elf sha=332756bb1662080520bb3c9973bedd027bc89227bcda1d191fcc67f76b855cf4 bytes=1398144 link_ns=7907732090
  candidate sha=99ea6d0e85734f15c99de4c8272c3d39d3afc30ef967cd581d6d513ec5b24126 bytes=3437994
  工具时钟：六只 -O1 --only ns=30663464644；其余 27 只 --only ns=24263170737；--link-only ns=9189543474
  二代：用候选 99ea6d0e 再编 lnx/x86_64，entry.elf 与 332756bb 不同。g2 entry.elf sha=0b56f314eb36db3d6706327f099dd3ba2b5875ebdedc340e88cb6363c0e0fb02 bytes=1398144，长度相同，从第 24 字节起不同。33 个对象里 32 个与第一代逐字节相同，只有 drive.o 不同（92814bf2 bytes=1373880 → 724e2bb8 bytes=1378472），多出来的全局符号是 tx_count。cx/text/text.cx 的 tx_count 在 ce3f7018fd（23:04）落地，晚于第一代 drive.o（22:59）。drive.o 是唯一携带 text.text 的对象。
  同一棵树再对一次 drive.o（text.cx 停在 476a09a592）：已装编译器与候选编出的 drive.o 逐字节相同，sha=556f24115e28c3077bad3be9aeeb63752184a84c5be3790e299ae9910a6174f6 bytes=1382392。
  二代工具时钟：drive.o ns=13246925759；其余五只 -O1 ns=21998369411；14 只 ns=11757854979；13 只 ns=14514585606；link ns=8151159408。同树对照：已装 ns=7294579227，候选 ns=8587396907。
  冻结 rev 两代（`--rev`，git archive 到 out/release-snap-<sha10>/，不读共享工作区）：rev=5ed7ba69bf9e86b91843c608ce00a1daff231dc8。P3、第一代、第二代同一个 rev。33 个对象、entry.elf、candidate 都逐字节相同。
  entry.elf sha=fb2237f23f4b7a50dd8d3d2e92ad2f56da623fdd5c0ec2f12193973004337775 bytes=1402240
  candidate sha=a6ddb03bd6dd356bc194ede1ed7c66a350bd011f4020de2e6634443061ddb975 bytes=3442090
  p3 sha=aec1a0bf27b9d1f2e26a1b86b52cb3694bc66a906415bcc0435c11278c6deabf
  第一代构造器是快照里的已装 unisacc.com（via=sh，sha=79fb84e65a10b55ce40be5f68746748acbd38f441944046c0a5dfa5718bc0c32）。第二代构造器是第一代 candidate（via=exec）。
  工具时钟：第一代六只 -O1 ns=31485383890，27 只 ns=24720043367，link ns=9093827509。第二代六只 -O1 ns=29852339534，27 只 ns=26229984423，link ns=8744013468。
  同一冻结快照上手跑整程序 -o（还没收进 release-build，收据外记 route=whole-o，镜像不带 P3）。33 个源含 hostnative.cx。osx/x86_64：Mach-O，sha=52dcc0252ee6d71324df36cc637d34774e4c0e0a4ef29633c9271cb7c95460af bytes=1552354，rc=0，落在 55 秒界内。win/x86_64：PE32+，sha=9ae0cf6d034ad1cea71ddde2eb6c790acfc8de298e67a25034daf9a381e86034 bytes=1589760，formats-macho-pe --pe 12/0，rc=0。arm64 未跑。
  WHOLE-O-TOOL：slice --target 已收 osx/x86_64 与 win/x86_64，route=whole-o。冻结 rev=5ed7ba69bf9e86b91843c608ce00a1daff231dc8。工具写 OUT/receipt.txt。镜像不带 P3。子作业 tools/bound 55，外层不再套 bound。
  WHOLE-O-TOOL：osx/x86_64 sha=52dcc0252ee6d71324df36cc637d34774e4c0e0a4ef29633c9271cb7c95460af bytes=1552354 ns=36426580344 rc=0。与手跑整程序 -o 逐字节相同。
  WHOLE-O-TOOL：win/x86_64 sha=9ae0cf6d034ad1cea71ddde2eb6c790acfc8de298e67a25034daf9a381e86034 bytes=1589760 ns=22000334071 rc=0。与手跑整程序 -o 逐字节相同。
  ARM64-SLICE：lnx/arm64 走 33 个对象，osx/arm64 走 route=whole-o。冻结 rev=5ed7ba69bf9e86b91843c608ce00a1daff231dc8。工具写收据。外层不再套 bound。
  ARM64-SLICE：lnx/arm64 entry.elf sha=dabcfa2337de8036b47c528210c4d851128d8f0e011fe33815e49258d9c3bee0 bytes=1385808。candidate sha=8b138088558666154bec3ac59aa3ecbb5dad10f8b2f397f6aa55f24789e69170 bytes=3425658，已按 entry-build 拼上 P3（p3 sha=aec1a0bf27b9d1f2e26a1b86b52cb3694bc66a906415bcc0435c11278c6deabf bytes=2039850）。六只 -O1 ns=25754945902，27 只 ns=23086225727，link ns=8472766756，rc=0。
  ARM64-SLICE：osx/arm64 sha=2a6c571cf24d62494ca403049e8b233e772a7b41a9da925ea363d264a5c182ab bytes=1981666 ns=37489425891 rc=0。镜像不带 P3。
  APE-PACK：tools/release-ape.cx 照 csih-ape 的 pack。五片是 release-build 的输出：lnx 两片用 entry.elf，不用 candidate；osx 两片和 win 用 whole-o。pack 写出启动器加五片后，在末尾追加冻结 P3（aec1a0bf27b9d1f2e26a1b86b52cb3694bc66a906415bcc0435c11278c6deabf）和 16 字节 UNIPKG1。verify 调 release/apeinspect.cx，对 package_sha256 和包尾是否在最后 16 字节。子作业 tools/bound 55。这一步不安装。
  WHOLE-O-LNX：cc 裁定 0.0.43 的 lnx/x86_64 与 lnx/arm64 也走 route=whole-o，rev=c98f757f4c93ea9cfe91bef18fd88a929ad7ba96。收据写 route=whole-o，镜像不带 P3。lnx/x86_64 sha=477393de82de27764b714502db8cc410e3f5daf213afa99cc278b3ca2b1ce49a bytes=1537472 ns=35346772463 rc=0。lnx/arm64 sha=75d2d4b7659e8cfa91acfbd6bf775762d79e7f56c896216a807a8813d975e1ba bytes=1959313 ns=36257376370 rc=0。pack：out/release-ape-c98/unisacc-next.com sha=43fbbb819aae028e8151a9f8eec621514a1264259d1b7a926d85f7dd952a5f00 bytes=4631130。smoke ok=4 failed=1（hostc.cx 写在 out/release-ape-smoke/，use 在该文件旁边找 sys/io.cx）。同一源放在 cx/ 下再跑，打印 release-ape-hostc，rc=0。未安装。
  INSTALL-C98：已装 ./unisacc.com sha=43fbbb819aae028e8151a9f8eec621514a1264259d1b7a926d85f7dd952a5f00 bytes=4631130 mode=755。备份 out/unisacc.com.79fb84e6 sha=79fb84e65a10b55ce40be5f68746748acbd38f441944046c0a5dfa5718bc0c32 bytes=3035300。check-install-fixes failed=0（dupmod、dedupe、iproto、uses48）。tests/run.cx --smoke assertions=16 failed=0。
- [ ] 3 object 模型步
- [ ] 4 测试到 QUALIFIED
- [ ] 项目所有者授权发布
