# c47-1 构造配方

## 身份与启动条件

cc负责发布；本窗口执行构造。范围为A1–A4/A6–A8、L0/M12及M3两项；A5与TAPE-END延期。M12合入前只准备配方；cc通知冻结提交后直接启动，不再等待第二次许可。M11冻结前不实施。

版本分开核验：bootstrap为已发布0.0.46，APE SHA256=`884adfef1815e87750c86c7036d3074552a66ce18dfa855588ab279c2aa99fcf`；目标产出为0.0.47。cc已报告统一版本表同步、check-limits与13/13通过；不得拿仍装着的0.0.46去宣称新版本检查通过。
当前状态：cc已重新冻结`7b98ec2c0618df88115ae1c80cea47144e8614d3`，R2已启动，源SID=`c84ea430ab7b037d983cd0208ec6340ccf878cf3ead00b463629a80dcae53c04`，新主根`/tmp/cdx-c471-r2-Ex8782`。controller实编44.56秒、context46.08秒，仅记PREPARED；全131/P4、后续六片与不动点全部重新执行，不复用旧f748资格。cc报告新冻结主树13/13、metadata_valid=1与关键工具实编已通过，本窗口不重复确认；产品目标0.0.47、bootstrap884/0.0.46不变。

容量裁定：逐相位将slice/pkg/receipt封存到`~/.unisacc-baselines/c47-1/R2-<相位>/`并核摘要，逐目标删除临时根，不一次建五根。gen→tbl→net→routes/P4存在真实依赖，须最后消费者完成且原始ctx封存后再删这一组中间件。`/tmp`空闲<4G时，cc已授权清理`/tmp/unisacc-e3cache-1001`中90分钟未动的可再生条目；<3G暂停报告cc，不启动后续作业。封存目录与/tmp同属/dev/vda2，移动本身不增空闲，须删已授权冗余文件才回收空间，不碰其它窗口源码或输入。

以下命令每行独立调用，变量由执行器绑定；不是新增shell调度脚本。`REV`必须是cc通知的完整新冻结SHA，不用滚动HEAD；`SHORT`为其前十位。所有路径用绝对路径，克隆根创建前cwd=MAIN，其余默认cwd=P，第6节按说明用S。每次调用记录job ID、argv、cwd、输入SHA、真实PID/进程组、deadline、退出码、日志与产物SHA。阶段失败即停该依赖链，保留失败，不自动重启。

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
mkdir -p "$TARGET_ROOT/out"
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
mkdir -p "$P/out/gen1"
env UNISACC_CTOR=/home/devali/.unisacc-baselines/c46-3/native-constructor UNISACC_RUN_CACHE=0 sh "$S/unisacc.com" "$S/tools/release-build.cx" -- --rev "$REV" slice "$P" "$PKG" "$P/out/gen1/lnx-x86_64" --target lnx/x86_64
```

native输出`entry.elf`及带包`candidate`，令`G1=P/out/gen1/lnx-x86_64/candidate`。原生对象和closure对象由entry-build实际记录，本次计数不得套用c46-3的81。该正式构造器已对compile55/link45/plan50分别bound，外层防挂240秒；调用不得再套55秒。历史native整段65.4秒，不能按单作业超时误判。
原生对象路线将显式constructor直接执行；用正式c46-3/native-constructor而不是假定APE可直接exec。该构造器实测SHA=`3c660571e790370664b9b3c5e3817e610bb0803f665b9cd8542730a7166c9b63`、直接`--version`为0.0.46。它与协调器bootstrap APE分别记录身份；entry-build会给ELF构造器绑定本次PKG，不回退旧包。

下面五行表达互不冲突的独立目标命令；按cc最新容量裁定本轮逐目标调用而非一次启动五根，执行器分别绑定cwd为LA、OA、OX、WA、WX。产物与receipt核验封存后删除该目标临时根；后续pack/不动点路径绑定到封存副本。只有读取G1和PKG是共享操作，各根有自己的snapshot与输出。

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
mkdir -p "$P/out/fixedpoint"
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

令`F=P/out/fixedpoint/g3`、`APE_DIR=P/out/c47-ape-final`、`APE=APE_DIR/unisacc-next.com`。本节cwd必须为S：pack内部用相对`./unisacc.com`与`tools/ape-pack.cx`。本轮曾在P下先打包，已保留该收据并用冻结S、新目录重新正式打包；两次字节相同，但正式来源只采用后者。

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
|list|5.77秒；131组start/finish状态保存|R2 bg_257 rc0，13.20秒，131项|
|全131波次|外层wave窗口与总耗时未恢复；不能引用其他轮271.8/279秒|R2 bg_258—270：5/42/42/42全部实际start/finish/command0；总墙钟未汇总，PREPARED不是资格|
|P4|8.456秒|R2 bg_272 rc0，外层68.96秒/内层48.426秒|
|native|65.4秒，内层独立bound|R2 bg_274 rc0，57.02秒/内层41.997秒|
|lnx/arm64|20.764秒|R2 bg_278 rc0，53.12秒|
|osx/arm64|23.584秒|R2 bg_280 rc0，90.72秒|
|osx/x86_64|18.036秒|R2 bg_282 rc0，90.45秒|
|win/arm64|17.889秒|R2 bg_288 rc0，74.62秒|
|win/x86_64|16.437秒|R2 bg_285 rc0，72.91秒|
|pack / smoke|约2.42 / 2.03秒|R2 正式bg_295/297：15.49/5.87秒，smoke5/0|

历史主根1,834,496,000B；五目标根分别347,844,608/302,796,800/304,726,016/303,497,216/305,586,176B。合计约3.4GB是估计，不是测得峰值。本次启动可用6,135,128,064B；归档主根303,652,864B后剩5,664,624,640B。构造前及每阶段用`df -B1 /tmp`和`du -s -B1 ROOT`记录真实容量；每根硬界限2GiB，空闲<3G暂停报告。封存只保留后续必需产物及原始日志，目标依次构造、核验封存后删除；主根在context/fixedpoint仍使用时不得删除。

历史证据位于`~/.unisacc-baselines/c46-3-evidence/cdx-c463-KhbubY/construction-and-original-s8.tar.gz`及清理记录目录；旧构造根已删除。当前命令合同见chain-step的build/ctx/wave/routes/p3/slice/fixedpoint入口、source-job-wave、release-build、entry-build和release-ape。历史恢复不能替代本次实跑。

阶段汇报每段一行：`阶段 / REV或SID / job与真实句柄 / rc / 实测耗时 / 产物SHA与日志路径 / 磁盘`。输入或资格缺失明确记UNKNOWN/REFUSE；准备完成仅记PREPARED。

## f7484108fe已实跑与作废证据

|阶段|实测结果|边界|
|---|---|---|
|正式context/list|C003记录56fa8f14；SID a3490196227e025766f2ade16636211dde54e1c0bd2f5256759b21258b5edf84；list131|前两个上下文及失败outer均保留|
|tool/gen/tbl/net|5/42/42/42，全部真实start/finish/command状态0；final rc0|PREPARED，不是新冻结的资格|
|routes|42模型对、1823声明行、两ISA kernel；41.43秒|仅材料构造|
|P4|1665247B；SHA10c75732b926105b8ee64d0549b7002bb61ea4e9aebfe40d1b861f43e625d35a；外层68.81秒，工具内46.723920524秒|旧冻结专属|
|G1N001|rc1，13.93秒，缺out/gen1父目录|未启动entry-build编译；已补配方mkdir|
|G1N002|rc1，19.60秒，ua-cxif-59051f84b6e7f63c.h:4:35 enumerator redeclared|cc纠正为A6跨TU纪元缺失；cdx2实施A6b，等待新冻结|
|最小sys.io探针|bg_242 rc0、1.00秒、stdout=ok|该最小例未复现，不能宣称全部CX都失败|

封存目录`~/.unisacc-baselines/c47-1/cdx-f7484108fe-tzdPw5/`：`source-context.tar.gz`摘要77bb6f3e4277430c3276e6bbc19ebcc19c32af8208175d451a6834e3dfada8cb，内含全131原始输出、正式记录及controller身份；gzip校验与指定成员读取已通过。另保留旧context、P4、routes收据、原始拒绝日志、生成头、最小探针、阶段账和作废状态。含A6b评审材料的封存实测88,997,888B（<300MB）。原主根`/tmp/cdx-c471-primary-tzdPw5`已按cc裁定在新冻结启动后删除，不再作为可执行入口；删除后观测空闲5,698,330,624B，新R2根304,578,560B。

## A6b评审与受影响读者

以下源码行号指已评审私有根`/tmp/omp-cdx2-l0-enum-fix`，不是尚待合入的共享main；实际三文件差异与运行材料已封存。

- 最终产品补丁涉及三个文件：`control-result.tsv:1375-1378`将所有枚举名纳入既有`td_unit_high`；`:2339-2344`扫描包含最高ID，在TDN过滤前双清END_/ENV；`k2-control.tsv:2`仅绑定既有ENV地址35000000；`vla-result.tsv:55-57`改为零/非零判定。scope+1、UNDO13、TAG_EPOCH与A7/A8键未改。
- `control-byte.tsv:13-15`：MU4+先增加unit_epoch并将scan置1，扫描完成才由MUend进入NEXT；不会先读新TU标识符再清。TAG_EPOCH仍只负责标签描述符的既有纪元逻辑，不被普通枚举名借用。
- ENV四个语义读点：`unarycontrol-result.tsv:207/222`、`update-result.tsv:161/169`、`constexpr-result.tsv:51/54`、`conditional-result.tsv:19/22`，均由END_或enum_defined/ENDEF别名门控。`update-result.tsv:247`的LOOKUP不读ENV；补丁后的`control-result.tsv:1914`的sizeof先按END_零/非零分类（旧冻结中为1911）。
- A7的15条生成轴、A8的30条生成轴及context原表60键逐项核对：观测1是i8类型轴，不是END标记。枚举值经`unarycontrol-result.tsv:223`规范为vt=0/vb=4再交TAX，无须改context表。
- 唯一发现的旧编码消费者回归是VLA仍比较ENUM==1：合法局部`enum{N=3}; int a[N]={11,13,17};`在605e包拒绝，正式0.0.46与严格GCC C99均通过。该拒绝与完整日志保留；3aa591包的同一原例默认run0。

独立运行证据均使用已发布native-constructor（3c660571、0.0.46）显式绑定私包，不冒称已构造出0.0.47产品：

|输入|实际观察|身份/作业|
|---|---|---|
|旧TU枚举名在下一TU作普通返回/sizeof/常量数组界/VLA|旧10c包为37/0/0/0；605e包均具名拒绝rc1；两包return7正控均7，transport/residue全0|bg_247，10/10，results-605e-clean|
|原合法局部枚举初始化数组|默认run0|bg_248，3aa591|
|预先INTERN名字main在前两TU分别作枚举名，第三TU作入口并校验11+13|默认run0|bg_249，3aa591|
|局部枚举作A7下标、A8算术返回及零枚举Bool返回|默认run0|bg_250，3aa591|

最终私包SHA=`3aa59130cc66564341511f821c47cddf367e060584c3fb02dbff199cfae68410`；前包605e完整身份、脚本及原始日志封存于上述目录的`a6b-review/`。首次残值脚本误设`UNISACC_RR_TIME=0`并断言正常stderr为空，导致6项脚本FAIL但十个实际rc已符合；原结果保留，去误开关并删除该偶然日志断言后新批次0 FAIL，未改rc/transport/residue合同。

结论限于语义评审：未发现剩余语义P1；高水位线性扫描与无条件零写的极限资源成本未独测，不能据此宣称完整资源边界或发布资格通过。71/13、新冻结构造及s9仍按各自输入身份执行。

## R2正式构造与最终APE实证

冻结源`7b98ec2c0618df88115ae1c80cea47144e8614d3`，SID=`c84ea430ab7b037d983cd0208ec6340ccf878cf3ead00b463629a80dcae53c04`；bootstrap仍为0.0.46/884adfef，正式产物为0.0.47。主根`/tmp/cdx-c471-r2-Ex8782`；逐作业记录为`out/c47-build/phases.tsv`。正式pack必须在冻结S下运行，输出实际名`unisacc-next.com`；基线交付改名为`unisacc.com`，字节不改。

|最终资产|字节|SHA256|
|---|---:|---|
|APE|6215778|`fde2db1812533a9a56a66d8e0d1870c310fdcc77d444a2d33210ca96e9ef98fe`|
|完整pkg.bin|1665282|`6b99c50491d9268f4e3c5fe828885f3425fe15284ff8d65d03da10f2e21be971`|
|内嵌包body（不含16B footer）|1665266|`0d5696535d0062d038bc71a97b266904b69f174a9b799555c2981d5b182c5ad4`|
|独立G3 win-arm64|3367424|`889f36cd9cc58e9451d43e6544ffeae4b88ef3cd70e3d88273248af7b912da28`|
|unisacc.com.build.json|314|`0512a1de8ddde26442e9f55ef8dfb40c817f2e1f6c2a93263ab15d4b934dde4f`|

最终资产原件在`~/.unisacc-baselines/c47-1/R2-APE-final-7b98ec2c06-Ex8782/`。新记录不是0.0.46侧车：对冻结树重新运行既有identity-helper、核对`.release-rev`，实际测APE全文件SHA和长度，再按既有`pp_execution_record_error`的canonical合同生成并校验（bg_307）。`product_kind=ape1`，commit/SID分别绑定上述7b98/c84。初次记录器误要求修订标记末尾LF，bg_306拒绝且未写记录；按真实40B标记修正后成功，未改变产品或测试断言。

两代不动点bg_283 rc0/793.70秒：82对象、2内核、5片、产品、包逐字节相同；G2/G3 native均`37356ba5927e47bd4f38367df01af6ad4191826a50b8dd1bc7ce85e45e0c8f6e`/3546048B。独立WinARM bg_290/291为71.56/70.67秒，G2/G3都为表中889f36摘要且cmp0。`UNISACC_RUN_CACHE=0`禁用磁盘输出及展开模型缓存，不禁用M12进程内不可变资源缓存，不能说所有缓存关闭。FP归档`R2-fixedpoint-7b98ec2c06-Ex8782/g2-g3.tar.gz`摘要`4d8a22e116d82d8f3754a0b1ebd492bfc8211503017ba83fb632f1091a837505`。

|最终APE本机检查|作业/实测|实际结论|
|---|---|---|
|release-ape smoke|bg_297，5.87秒|5 PASS/0 FAIL|
|COMM旧负控/新产品|bg_298，8.32秒|旧63/新rcprobe.c；memfd、argv0、参数、fd9、stdout、rc37全核对|
|安装修复合同（不执行安装）|bg_299，29.32秒|6项PASS，failed=0|
|C99 VT/FF|bg_300，11.55秒|12 PASS/0 FAIL|
|59个C负例|bg_301，12.52秒，rc1|45 PASS/14 FAIL；完整失败原件保留，不记全绿|
|A6b最终APE正控|bg_302，rc0|局部enum固定初始化数组、A7下标/A8算术及零Bool、预先INTERN main的三TU全部默认run0|

14个`.product-refuses`失败：`assign-to-const`、`incompatible-pointer-init`、`duplicate-parameter`、`int-to-pointer-init`、`conflicting-function-types`、`zero-array-size`、`void-value-used`、`write-through-const-pointer`、`int-plus-assign-string`、`distinct-pointer-compare`、`local-redefinition`、`discards-const`、`void-parameter-named`、`multiple-storage-classes`。计数与先前开发轮同，不把未经逐例对比的集合等同当作新证明；不删断言、不缩覆盖、不改退出码。

source/context、P4、各G1片、两代FP、WinARM独立G2/G3均已分相位封存并校验gzip/成员/SHA；只在routes/P4最后消费者结束后删除jobs/models。各目标临时根封存后删除；主根保留冻结源和后续收据。磁盘阈值沿用本配方，不靠跨目录移动伪装释放。

cc本轮明确授权建立私有draft `v0.0.47-c47-1-candidate`并触发`candidate-native.yml`，不是公开正式发布。bg_307创建draft并上传表中四个文件资产（内嵌body仅作身份说明，不另上传）；bg_308下载四资产与基线逐个cmp0，SHA全部一致。GitHub draft暂用untagged展示URL，实际tagName仍为上述候选名，targetCommitish为7b98。CI通过`release_tag`鉴权取draft，不把匿名URL当已公开资产；CI期间不修改APE或四资产。s9由grk负责，本机smoke与构造不替代s9或原生矩阵资格；14个C负例FAIL不改写。

CI正式dispatch（bg_309 rc0/2.14秒）：workflow `candidate-native.yml`，ref=`ci/c47-1`，`commit=7b98ec2c0618df88115ae1c80cea47144e8614d3`，`ape_sha256=fde2db1812533a9a56a66d8e0d1870c310fdcc77d444a2d33210ca96e9ef98fe`，`release_tag=v0.0.47-c47-1-candidate`；`ape_url=https://github.com/partnernetsoftware/unisacc/releases/download/v0.0.47-c47-1-candidate/unisacc.com`仅作未使用后备（非空release_tag优先）。bg_310获得run **[42408207540](https://github.com/partnernetsoftware/unisacc/actions/runs/42408207540)**，headSha为冻结7b98，首次观察`in_progress`、conclusion为空。终态由cc读取；此处不等待、不重复触发、不冒认CI PASS。draft发布对象ID=`RE_kwDOUhBsCM4aGFng`，四资产ID依次APE721758240/pkg721758239/WinARM721758237/记录721758238。

最终本机负例/空白与COMM原件及完整阶段账封存于`~/.unisacc-baselines/c47-1/R2-local-verification-7b98ec2c06-Ex8782/local-checks.tar.gz`，gzip检查通过，SHA=`169960c75b957399416fee2773c9817534d34b46163c70f33fe49b9282b20c9c`；同时保留活目录全部14个失败文件。封存后主根445026304B，空闲6622429184B，未触发3G暂停。资产保持原件不变。

## 候选资产只读读回演练

新入口`release/tools/c47-1-readback.cx`复用旧读回顺序，不修改`release/tools/c46-3-readback.sh`。参数为`ROOT TAG NEW_OUT OLD_APE VERSION COMMIT PINS true|false`：最后一项指定预期draft状态；版本、源commit、tag和资产名均不硬编码在程序中。`release/tools/c47-1-readback.tsv`按ape/package/win-arm64/record角色固定本轮四资产名与SHA；输入可用其他角色映射表，下载后仅在新目录规范化本地文件名。正式release模式还核tag指向；本轮只执行draft模式，不冒称正式发布演练已实跑。

```sh
tools/bound 55 sh ./unisacc.com release/tools/c47-1-readback.cx -- \
  /home/devali/repos/unisacc v0.0.47-c47-1-candidate NEW_ABSOLUTE_OUT \
  /home/devali/.unisacc-baselines/c46-2/unisacc-c46-2.com \
  0.0.47 7b98ec2c0618df88115ae1c80cea47144e8614d3 \
  /home/devali/repos/unisacc/release/tools/c47-1-readback.tsv true
```

`NEW_ABSOLUTE_OUT`必须不存在。每个外部子命令最多45秒，外层55秒；下载、版本、smoke、COMM、布局生成/校验、外部校验和及ZIP成员各保存stdout/stderr/rc/residue。没有release create/edit/upload、签名、tag写入或资产写入命令。

本轮bg_315实际rc0/42.22秒，干净目录`/tmp/cdx-c471-readback-oegCoL/run`：draft=true/prerelease=false/target7b98匹配；四SHA与固定原件一致；`unisacc 0.0.47`；smoke5/0（含C退出7）；COMM旧63/新rcprobe.c并保留memfd/argv0/参数/fd9/stdout/rc37。`layout/`为新生成文件，未上传，不是从远端下载ZIP后声称验收。

布局工具新增可选`--version VERSION`：`tools/release-layout.cx -- --version 0.0.47 SOURCE NEW_OUT`与`-- --version 0.0.47 --check OUT`，旧两参数调用仍保持0.0.46合同。实际生成与独立check均`RELEASE_LAYOUT status=PASS`；外部`sha256sum --check SHA256SUMS`六项OK，ZIP四成员恰为`slice-win-arm64/pkg.bin/run.cmd/README-win-arm64.txt`，`build.json`与配对侧车同SHA。新ZIP `unisacc-0.0.47-win-arm64.zip` SHA=`9c7102d3b06c52162b3e6f229a7d14a9a024d05654e40a9f900664c1854acfae`。bg_316原布局八断言全过，包含包一字节损坏与重钉摘要后ZIP附加字节拒绝；同作业历史语料分类表与冻结7b98的完整性gate rc0，不重跑已知GCC拒绝。

bg_317工作区十三闸`ok=13 fail=0 elapsed_ms=7640`，工具行数80298≤80302，未修改任何基线。读回只是Linux本机与格式验证，不授予Windows原生执行或同候选CI资格，不改变已有14个C负例FAIL。

初版bg_315虽实跑通过，但bg_318生产hook拒绝其新文件cxraw25/cxapis2/cxlibc35，未产生提交；初版未跟踪时的bg_317不作为最终门禁。未增补或抬高基线，改用既有L0接口：PIN以sy_read_full/tx_field解析、sha256sum一次核四文件、sy_copy只写新目录、sy_open_new保留收据、sy_*/tx_*输出比较。bg_319误引sys.proc导致sy_exit未定义，编译rc1且未运行；实际声明位于sys.sig，修正引用后重新实跑。

**最终L0实现**bg_321 rc0/21.54秒，输出目录`/tmp/cdx-c471-readback-oegCoL/l0-final`，完整SHA/version/smoke/COMM/layout生成与check/六项SHA256SUMS/ZIP四成员均PASS；新COMM原件`/tmp/cx-run-comm-xqqvzd`。bg_320将新文件纳入真实暂存输入后十三闸13/0、8234ms，新增raw/cxapis/libc均0。旧c46-3-readback.sh对冻结7b98的diff rc0；产品源码/产品表/冻结资产从未改写。

