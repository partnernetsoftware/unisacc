# c47-1 构造配方

## 身份与启动条件

cc负责发布；本窗口执行构造。范围为A1–A4/A6–A8、L0/M12及M3两项；A5与TAPE-END延期。M12合入前只准备配方；cc通知冻结提交后直接启动，不再等待第二次许可。M11冻结前不实施。

版本分开核验：bootstrap为已发布0.0.46，APE SHA256=`884adfef1815e87750c86c7036d3074552a66ce18dfa855588ab279c2aa99fcf`；目标产出为0.0.47。cc已报告统一版本表同步、check-limits与13/13通过；不得拿仍装着的0.0.46去宣称新版本检查通过。
冻结输入已通知：`f7484108febaf3ac7d0eb5d909887d6a46c313cf`。cc随后收紧磁盘调度：只先建立主根及lnx目标根，逐目标封存slice/pkg/receipt到`~/.unisacc-baselines/c47-1/`并核对摘要后删除该目标临时根；不要一次建立五个目标根。`/tmp`可用空间小于3G即暂停并报告cc清理，不自行删其他窗口材料。封存目录与/tmp实测同属/dev/vda2，移动本身不增加文件系统空闲，须在核验保留材料后删除已授权的冗余目标根。

以下命令每行独立调用，变量由执行器绑定；不是新增shell调度脚本。`REV`必须是cc通知的完整冻结SHA，不用滚动HEAD；`SHORT`为其前十位。所有路径用绝对路径，默认cwd为私有主根`P`。每次调用记录job ID、argv、cwd、输入SHA、真实PID/进程组、deadline、退出码、日志与产物SHA。阶段失败即停该依赖链，保留失败，不自动重启。

## 0. 私有根、bootstrap与快照

一个主根`P`，五个独立目标根`LA/OA/OX/WA/WX`；共享git对象及只读PKG/bootstrap，不共享可写snapshot。不用`--no-hardlinks`六份复制。`MAIN`为共享main检出，`B=P/out/c47-build`，`C=P/out/c47-context`，`R=P/out/c47-routes`，`PKG=P/out/c47-final-pkg.bin`，`S=P/out/release-snap-SHORT`。

```sh
git clone --shared --single-branch "$MAIN" "$P"
git -C "$P" rev-parse --verify "$REV^{commit}"
cp "$MAIN/unisacc.com" "$P/unisacc.com"
cp /home/devali/.unisacc-baselines/c46-3/unisacc.com.build.json "$P/unisacc.com.build.json"
sha256sum "$P/unisacc.com" "$P/unisacc.com.build.json"
sh "$P/unisacc.com" --version
mkdir -p "$B" "$S"
cp "$P/unisacc.com" "$B/Root41-bootstrap.com"
cp /home/devali/.unisacc-baselines/c46-3/unisacc.com.build.json "$B/Root41-bootstrap.com.build.json"
sha256sum "$B/Root41-bootstrap.com" "$B/Root41-bootstrap.com.build.json"
git archive "$REV" | tar -x -C "$S"
cp "$P/unisacc.com" "$S/unisacc.com"
cp "$P/unisacc.com.build.json" "$S/unisacc.com.build.json"
```

bootstrap版本须精确为`unisacc 0.0.46`且APE摘要如上。实跑发现当前为独立APE，无`unisacc.com.dev`；旧chain-step snap无条件复制dev目录，不能用于此输入。改用上面的真实git archive与两件产品材料复制，归档成功后由文件写入工具将完整40字节REV写入`S/.release-rev`，不造空dev目录。`Root41-bootstrap.com`只是工具文件名。release-build本身通过pp_copy_product支持独立APE；同一snapshot的刷新写操作保持串行。
context前必须给bootstrap APE准备配对build.json：来自`~/.unisacc-baselines/c46-3/unisacc.com.build.json`，摘要为`56fa8f14317f6150d0e6f182e0a51bf09ffe4168f36166216561477bdf2086e5`，product_digest须等于bootstrap的884adfef完整摘要。注意当前共享main的记录摘要aca03458虽也写相同product_digest，却标commit=3b32f21c、sources_sha=ce291cc3，不是正式发布存档的44182b40/95e34c70；本次只在私有根用正式存档替换，未改共享记录。首个缺sidecar上下文及第二个aca身份上下文均保留，切换配对记录后新建context、保留旧outer目录，不能混用旧身份或以目录存在当本次list成功。

每到一个目标再执行`git clone --shared --single-branch "$MAIN" "$TARGET_ROOT"`及`git -C "$TARGET_ROOT" rev-parse --verify "$REV^{commit}"`，复制下面两件启动材料；`TARGET_ROOT`依次为LA、OA、OX、WA、WX。所有根保留main；源输入由`git archive REV`冻结，不靠切分支。构造入口统一读取S内冻结工具，各目标只写自己的snapshot与输出。

```sh
cp "$P/unisacc.com" "$TARGET_ROOT/unisacc.com"
cp "$P/unisacc.com.build.json" "$TARGET_ROOT/unisacc.com.build.json"
```

## 1. controller、context、host、list

```sh
sh "$S/unisacc.com" "$S/tools/chain-step.cx" -- build "$REV" "$S" "$B"
sh "$S/unisacc.com" "$S/tools/chain-step.cx" -- ctx "$REV" "$S" "$B" "$C" /usr/bin/x86_64-linux-gnu-gcc-11
sh "$S/unisacc.com" "$S/tools/source-job-wave.cx" -- "$S" "$C" "$B" host
sh "$S/unisacc.com" "$S/tools/source-job-wave.cx" -- "$S" "$C" "$B" list
```

controller实质为cwd=B执行`S/tools/bound 55 sh B/Root41-bootstrap.com -O2 S/tools/source-package.cx -o B/source-package-O2.elf`。context使用`prepare-context S B/Root41-bootstrap.com HOST_CC C`，记录真实HOST_CC身份；缺该编译器则报告原始错误，不换未记录的宿主工具。

host只准备host-libz和host-inputs.tsv，不调用controller，不要求不存在的outer/host.status。list才调用controller。预期清单为131项：tool5、gen42、tbl42、net42；若冻结树实际清单不同，保留差异交cc，不把历史计数冒充本次结果。

## 2. 全131作业，不走旧ctx增量

c46-3旧ctx/jobs已删除，`chain-precheck init`的`REFUSE jobs directory`不是可恢复输入。必须新建context，完成全131项。每个wave先start-job再finish-job；`chain-step wave`已按此顺序调用两次，下面每行是一整个窗口。FIRST是phase内索引，不是slice编号；不用未证明的“wave×五片”说法。

```sh
env UNISACC_WAVE_REUSE=0 UNISACC_RUN_CACHE=0 sh "$S/unisacc.com" "$S/tools/chain-step.cx" -- wave "$S" "$C" "$B" tool 0 5
env UNISACC_WAVE_REUSE=0 UNISACC_RUN_CACHE=0 sh "$S/unisacc.com" "$S/tools/chain-step.cx" -- wave "$S" "$C" "$B" gen 0 16
env UNISACC_WAVE_REUSE=0 UNISACC_RUN_CACHE=0 sh "$S/unisacc.com" "$S/tools/chain-step.cx" -- wave "$S" "$C" "$B" gen 16 16
env UNISACC_WAVE_REUSE=0 UNISACC_RUN_CACHE=0 sh "$S/unisacc.com" "$S/tools/chain-step.cx" -- wave "$S" "$C" "$B" gen 32 10
env UNISACC_WAVE_REUSE=0 UNISACC_RUN_CACHE=0 sh "$S/unisacc.com" "$S/tools/chain-step.cx" -- wave "$S" "$C" "$B" tbl 0 16
env UNISACC_WAVE_REUSE=0 UNISACC_RUN_CACHE=0 sh "$S/unisacc.com" "$S/tools/chain-step.cx" -- wave "$S" "$C" "$B" tbl 16 16
env UNISACC_WAVE_REUSE=0 UNISACC_RUN_CACHE=0 sh "$S/unisacc.com" "$S/tools/chain-step.cx" -- wave "$S" "$C" "$B" tbl 32 10
env UNISACC_WAVE_REUSE=0 UNISACC_RUN_CACHE=0 sh "$S/unisacc.com" "$S/tools/chain-step.cx" -- wave "$S" "$C" "$B" net 0 16
env UNISACC_WAVE_REUSE=0 UNISACC_RUN_CACHE=0 sh "$S/unisacc.com" "$S/tools/chain-step.cx" -- wave "$S" "$C" "$B" net 16 16
env UNISACC_WAVE_REUSE=0 UNISACC_RUN_CACHE=0 sh "$S/unisacc.com" "$S/tools/chain-step.cx" -- wave "$S" "$C" "$B" net 32 10
sh "$S/unisacc.com" "$S/tools/source-job-wave.cx" -- "$S" "$C" "$B" final
```

本次nproc=16、可用内存20,302,360,576B，冻结工具硬上限WAVE_JOBS_MAX=16，因此按16/16/10切分42项、五工具同窗；这是本轮选择，不冒称c46-3历史切分。工具旧文件头COUNT1..4是历史注释，实际硬上限见宏与wave范围检查。历史归档保留131组start/finish内层argv与退出状态。每子项bound55、传输58秒不变，chain-step包含start与finish两个动作，调用器外层180秒只用于正规有界构造窗口，不放宽任何子项。每阶段核对实际完成清单与失败项。

## 3. routes、声明资源与P4包

```sh
sh "$S/unisacc.com" "$S/tools/chain-step.cx" -- routes "$REV" "$S" "$C/jobs" "$R"
sh "$S/unisacc.com" "$S/tools/chain-step.cx" -- p3 "$REV" "$S" "$R/compiler.pkg" "$PKG" --nets "$C/jobs" --routes "$R/declaration/routes.tsv"
```

第二条由cwd=P进入release-build，其ROOT是P而不是已冻结的S，避免snapshot套snapshot。`--nets`及`--routes`不可省略；输入资源来自本次context、冻结include、predefines和两ISA kernel，不沿用旧包替代新网。工具动作名仍叫p3，默认产物格式为P4。

## 4. native及五个独立目标

```sh
env UNISACC_CTOR="$P/unisacc.com" UNISACC_RUN_CACHE=0 sh "$S/unisacc.com" "$S/tools/release-build.cx" -- --rev "$REV" slice "$P" "$PKG" "$P/out/gen1/lnx-x86_64" --target lnx/x86_64
```

native输出`entry.elf`及带包`candidate`，令`G1=P/out/gen1/lnx-x86_64/candidate`。原生对象和closure对象由entry-build实际记录，本次计数不得套用c46-3的81。该正式构造器已对compile55/link45/plan50分别bound，外层防挂240秒；调用不得再套55秒。历史native整段65.4秒，不能按单作业超时误判。

下面五行可并发，执行器分别绑定cwd为LA、OA、OX、WA、WX，分别保留真实句柄；只有读取G1和PKG是共享操作。每根release-build有自己的snapshot和输出。

```sh
env UNISACC_CTOR="$G1" UNISACC_RUN_CACHE=0 sh "$LA/unisacc.com" "$S/tools/release-build.cx" -- --rev "$REV" slice "$LA" "$PKG" "$LA/out/gen1" --target lnx/arm64
env UNISACC_CTOR="$G1" UNISACC_RUN_CACHE=0 sh "$OA/unisacc.com" "$S/tools/release-build.cx" -- --rev "$REV" slice "$OA" "$PKG" "$OA/out/gen1" --target osx/arm64
env UNISACC_CTOR="$G1" UNISACC_RUN_CACHE=0 sh "$OX/unisacc.com" "$S/tools/release-build.cx" -- --rev "$REV" slice "$OX" "$PKG" "$OX/out/gen1" --target osx/x86_64
env UNISACC_CTOR="$G1" UNISACC_RUN_CACHE=0 sh "$WA/unisacc.com" "$S/tools/release-build.cx" -- --rev "$REV" slice "$WA" "$PKG" "$WA/out/gen1" --target win/arm64
env UNISACC_CTOR="$G1" UNISACC_RUN_CACHE=0 sh "$WX/unisacc.com" "$S/tools/release-build.cx" -- --rev "$REV" slice "$WX" "$PKG" "$WX/out/gen1" --target win/x86_64
```

lnx输出`entry.elf`和`candidate`；whole-o输出`slice-osx-arm64`、`slice-osx-x86_64`、`slice-win-arm64`、`slice-win-x86_64`。当前rb_slice将四个osx/win目标直接交rb_whole，一次整片构造；文件头历史PHASE A/B说明不是当前入口行为，不手工插入分段。osx/win whole-o上限120秒为既有例外，不擅自提高；其余子项不超过60秒。

## 5. 两代不动点

```sh
env UNISACC_RUN_CACHE=0 sh "$S/unisacc.com" "$S/tools/chain-step.cx" -- fixedpoint "$REV" "$S" "$PKG" "$G1" "$P/out/fixedpoint" --target lnx/x86_64
```

该正式入口内部串行完成g2/g3各五片及两ISA kernel，并比较objects、kernels、slices、product、package五项。每代lnx两片使用上一代product，whole-o三片使用本代native；不能把参数target误读为只构建native。此入口不包含win/arm64，不能据其成功宣称六片全闭环。`G2=P/out/fixedpoint/g2/lnx-x86_64/candidate`、`G3=P/out/fixedpoint/g3/lnx-x86_64/candidate`。

```sh
env UNISACC_CTOR="$G2" UNISACC_RUN_CACHE=0 sh "$WA/unisacc.com" "$S/tools/release-build.cx" -- --rev "$REV" slice "$WA" "$PKG" "$WA/out/gen2" --target win/arm64
env UNISACC_CTOR="$G3" UNISACC_RUN_CACHE=0 sh "$WA/unisacc.com" "$S/tools/release-build.cx" -- --rev "$REV" slice "$WA" "$PKG" "$WA/out/gen3" --target win/arm64
cmp "$WA/out/gen2/slice-win-arm64" "$WA/out/gen3/slice-win-arm64"
```

独立win/arm64还须记录两次构造器SHA与产物SHA；不得只比较同一文件两个别名。完整资格使用g3六片打包。

## 6. pack、smoke、comm与C负例

令`F=P/out/fixedpoint/g3`、`APE_DIR=P/out/c47-ape`、`APE=APE_DIR/unisacc.com`。

```sh
sh "$S/unisacc.com" "$S/tools/release-ape.cx" -- pack "$APE_DIR" "$PKG" "$F/win-x86_64/slice-win-x86_64" "$F/lnx-x86_64/entry.elf" "$F/lnx-arm64/entry.elf" "$F/osx-x86_64/slice-osx-x86_64" "$F/osx-arm64/slice-osx-arm64" "$WA/out/gen3/slice-win-arm64"
sh "$APE" --version
sh "$S/unisacc.com" "$S/tests/check-limits.cx"
sh "$S/unisacc.com" "$S/tools/release-ape.cx" -- smoke "$APE"
sh "$S/unisacc.com" "$S/tools/check-run-comm.cx" -- "$S" "$APE" /home/devali/.unisacc-baselines/c46-2/unisacc-c46-2.com
sh "$S/unisacc.com" "$S/tools/check-install-fixes.cx" -- "$APE"
```

本节命令统一cwd=S，确保check-limits、pack及安装修复夹具读取冻结树，而非共享工作树。版本输出须精确为`unisacc 0.0.47`；check-limits与smoke共同核验冻结版本事实和实际APE，不跳过版本项，不拿bootstrap版本当产出期望。comm入口为ROOT NEW_PRODUCT OLD_PRODUCT，工具自身核对旧c46-2负控SHA=`c1309d112ba1d389a296ef701f864d56cc06fe281eea98f621a51d8d2b429caf`；旧控comm=63、新控comm=rcprobe.c，并观察rc37、EXE=/memfd、SEMANTICS及ARGV0，不把历史日志当本次通过。

`W=P/out/c47-candidate-root`，只放本次APE，避免c-negative误测仍为0.0.46的P/unisacc.com；`NEG=P/out/c47-negative`必须尚不存在。

```sh
mkdir -p "$W"
cp "$APE" "$W/unisacc.com"
sh "$S/unisacc.com" "$S/tools/c-negative.cx" -- "$W" "$NEG" /usr/bin/gcc
```

当前源码59例的旧开发候选结果为45/14，不是发布PASS；本次真实结果、具名失败及rc1原样保留。固定43曲线由cdx2的固定输入另报，不能与新增用例总数混算。将本次APE/PKG/REV及收据交grk启动s9全批；不重用已失效s8结论。发布、安装、CI资格与公开资产由cc统筹，不在此擅自执行。

## 界限、历史测量与证据

|阶段|c46-3证据/耗时|c47-1状态|
|---|---|---|
|list|5.77秒；131组start/finish状态保存|UNMEASURED|
|全131波次|外层wave窗口与总耗时未恢复；不能引用其他轮271.8/279秒|UNMEASURED|
|P4|8.456秒|UNMEASURED|
|native|65.4秒，内层独立bound|UNMEASURED|
|lnx/arm64|20.764秒|UNMEASURED|
|osx/arm64|23.584秒|UNMEASURED|
|osx/x86_64|18.036秒|UNMEASURED|
|win/arm64|17.889秒|UNMEASURED|
|win/x86_64|16.437秒|UNMEASURED|
|pack / smoke|约2.42 / 2.03秒|UNMEASURED|

历史主根1,834,496,000B；五目标根分别347,844,608/302,796,800/304,726,016/303,497,216/305,586,176B。合计约3.4GB是估计，不是测得峰值。本次启动可用6,135,128,064B；归档主根303,652,864B后剩5,664,624,640B。构造前及每阶段用`df -B1 /tmp`和`du -s -B1 ROOT`记录真实容量；每根硬界限2GiB，空闲<3G暂停报告。封存只保留后续必需产物及原始日志，目标依次构造、核验封存后删除；主根在context/fixedpoint仍使用时不得删除。

历史证据位于`~/.unisacc-baselines/c46-3-evidence/cdx-c463-KhbubY/construction-and-original-s8.tar.gz`及清理记录目录；旧构造根已删除。当前命令合同见chain-step的build/ctx/wave/routes/p3/slice/fixedpoint入口、source-job-wave、release-build、entry-build和release-ape。历史恢复不能替代本次实跑。

阶段汇报每段一行：`阶段 / REV或SID / job与真实句柄 / rc / 实测耗时 / 产物SHA与日志路径 / 磁盘`。输入或资格缺失明确记UNKNOWN/REFUSE；准备完成仅记PREPARED。
