# cx-dev:本机快速迭代车道(第一期)

状态:第一期已落地(2026-11-16)。NOT_RELEASE:cx-dev 的一切结果只标 LOCAL,不是候选证据,不进任何资格。设计来源 [archive/plans/local-loader-2026-11-16.md](../archive/plans/local-loader-2026-11-16.md),对外名字由 local 改为 cx-dev。

## 1 是什么

`cx-dev xxxx.cx -- ARGS` 与 `sh ./unisacc.com xxxx.cx -- ARGS` 参数与行为相同(默认即 -run,`-c`、`-o` 等原样透传),区别只有一处:它是一个独立的 loader 可执行文件,运行期不经过 unisacc.com,由 loader 读同目录的 manifest,校验后直接 exec 本机 lnx/x86_64 切片,并把模型包指向私有包。改了 exec 下的网表之后,下一次调用自动重建受影响的网并换进私有包,所以改表后直接再跑一次就用上新模型。

## 2 用法

(1) 构造一次(只有这一步使用已装产品,属构造期;tools/cx-dev.cx 或已装产品变了要重做):

    sh ./unisacc.com tools/cx-dev.cx -- --build

产出自包含目录(默认 `$TMPDIR/cx-dev-UID/wt<检出路径>`,可用 `CXDEV_HOME` 指定;不在仓内、不在 out/):loader `cx-dev`、`manifest.tsv`、`base.bin`(本机切片)、`pkg.bin`(私有包)、`pkg.installed`、`base-tree/`(已装版本的干净树)、`tools/`(原生 table-lint 与 net-deps)、`cache/ctor/`(构造器 gen、tbl、net,gen 用 -O2 档)、`ecache/`(私有运行缓存)。

(2) 日常:

    sh tools/cx-dev xxxx.cx -- ARGS        # 或把 tools/ 加进 PATH 后直接 cx-dev xxxx.cx -- ARGS

薄壳 tools/cx-dev 只 exec loader;loader 缺失或比 tools/cx-dev.cx、已装产品旧时具名拒绝并给出构造命令,不回落到 unisacc.com。

(3) 可选子命令:`cx-dev --status`(会话摘要)、`cx-dev --reload`(强制重新比较并重建,逐配方打印结果,有未换入的配方时 rc 2)、`cx-dev --close`(丢弃本地状态、已建的网与私有运行缓存)。

(4) 环境变量:`CXDEV_VERBOSE=1` 打印重建摘要与 `CXDEV_RUN` 行;`CXDEV_WT` 指定要比较的工作树(默认构造时的检出);`CXDEV_STEP_S` 每步上限(1 到 55,默认 55);`CXDEV_PAR`、`CXDEV_ABSENT` 见文件头。

## 2.1 假根:让检查与批次把 cx-dev 当产品(2026-11-16)

多数检查与批次硬跑 `sh ./unisacc.com`,所以改了 exec 下的表之后,默认测到的是已装产品,看不到新网。`tools/cx-dev-root DIR` 造一个假根:顶层条目全是指向本检出的符号链接,只有 `unisacc.com` 换成转发到 cx-dev 的壳(带 `UNISACC_LOCAL=1`)。

    sh tools/cx-dev-root /tmp/cxd-root
    cd /tmp/cxd-root && sh ./unisacc.com tools/check-bench-gencode.cx -- /tmp/cxd-root 3 8

实测(2026-11-16):`gold-table --check` PASS 20 stages;`check-bench-gencode` valid=6 invalid=0 failed=0,三档校验值逐字节相同。结论只是 LOCAL 证据,不是候选结果;`candidate=` 身份相关的批次(要求已装产品身份)在假根里不适用。

## 3 每次调用做什么

(1) 读 manifest(首行 `NOT_RELEASE<TAB>cx-dev-v1`,其后 name、kind、arch、sha256、size、order、origin、trust 八列)。base 行与全部 trust=verified 的成员逐个核 sha256 与字节数,不符具名拒绝且不执行;trust=local 的成员(由工作树的表建出的网)只记录摘要不比对。
(2) 工作树在且 stat 指纹有变化时,把 exec、seed、src、cx、kernel、weights/gold 与已装版本的干净树逐文件比较(含未提交改动),按"阶段目录规则加 net-deps --touch"选出受影响的本机配方,逐张 gen -> table-lint -> tbl -> net -> UNINETB1 -> M 记录,写新包文件后原子改名,并重写 manifest。
(3) 配方到模型序号:用干净树建出的基线 M 记录与包内 M 记录逐字节相同才认序号,否则 recipe-not-located。包内没有独立 M 的配方(shared-e1、shared-e3,取自 tools/p3-member-consume.cx 的 DEDUPE 行)标 NOT_DEPLOYED 不建。
(4) table-lint 门是相对的:已装表本身就有 ERROR 存量,所以只有"已装表没有的新 ERROR 行"才拦下换包。
(5) exec 切片:`UNISA_CONTAINER` 指私有包,`UNISACC_LOCAL=1`,`UNISACC_LOCAL_SCOPE` 为当前 scope,运行缓存指 bundle 自己的 0700 目录(local 条目不进共享缓存;每次换包都是新文件,同源不会命中旧条目)。

## 4 铁律与标注

- 不静默用旧模型:某配方没换成(超过每步上限记 DEFERRED、出现新的 lint ERROR、构造失败、定位不到、seed 变了而构造器未重建),该配方保留已装模型,且每次调用都在 stderr 打 `CXDEV_STALE<TAB>recipe=..<TAB>reason=..<TAB>model=installed`;退出码仍是程序自己的。
- 基座陈旧:差异触及 src、cx、exec/c、kernel、weights/gold 时,重载打 `BASE_STALE`,scope 变为 LOCAL-PARTIAL,第一期没有绕过开关(切片仍是已装的,这些源码改动不会生效)。
- scope:LOCAL-BASELINE(无本地成员)、LOCAL、LOCAL-PARTIAL;有未换入配方时 `CXDEV_RUN` 行写 LOCAL-STALE。
- 只在 lnx/x86_64 实跑;其余目标的配方标 NOT_CLAIMED,不建不跑。
- 只缓存构造物(构造器、网、配方序号),不缓存任何运行结论。输出里不出现通过字样,只有 LOCAL_OK、LOCAL_FAIL。
- install-ape 对 bundle 的包实跑拒绝(ape-does-not-answer-version),在检查里是负控。

## 5 检查与实测

`sh ./unisacc.com tools/check-cx-dev.cx`(或 `-- --cases a|b|c|d|e`,批次 cx-dev-a 至 cx-dev-e),末行 `CHECK_CX_DEV<TAB>assertions=N<TAB>failed=0`(标记后用制表符,是 tests/run-batch.cx 的批次合同)。2026-11-16 实跑:a 6、b 14、c 13、d 7、e 3 项断言,失败 0。a 构造 bundle;b 基线(拷到别的目录独立运行、与 unisacc.com 对同一输入的镜像、目标文件、stdout、stderr、退出码逐字节相同、execve 序列里没有 unisacc.com、成员被改即具名拒绝);c 在夹具(已装版本树的副本,不动仓内网表)改词法模板表一行后下一次普通调用即见新诊断文案而 unisacc.com 不变、换包后同源不命中运行缓存;d 为 BASE_STALE、DEFERRED 回落与 --close;e 为 recipe-not-located。b 至 e 在 bundle 缺失时会自己构造,先跑 a 才能保证每组不超 55 秒。

实测(2026-11-16,各 1 次,load1 1.0 至 1.5):无改动热调用 10 次 cx-dev 495 毫秒对 unisacc.com 65 毫秒,每次额外约 43 毫秒;改词法模板表一行后首次调用(重建两张网连同两张基线网,串行,加执行)27.9 秒,其后每次 76 毫秒;基线网已缓存时同类改动的重建约 12 至 15 秒(估计,由分项推算,未单独实测);--build 约 30 至 45 秒。对照:unisacc.com 冷编译 8 至 13 秒(build/speed-baseline.tsv)。检查输出的 `CXDEV_TIMING` 行带 load1,只报告不设门。

## 6 已知缺口与第二期待办

(1) 产品运行库的 tmpfile(include/cx-flat.h)用固定序列的 `/tmp/unNNN.tmp` 命名,无 pid、无 O_EXCL:两个由产品编出的构造器同时运行会互读临时表(实测出现 `delta table column count` 与静默不同的网)。修复(09ca7d6c7a,排它创建加栈地址熵)属 0.0.71 产品输入,尚未安装。cx-dev 的回避方式已改:gen 构造器编为 `genp`,生成一个包装源,用按 pid 命名的私有 tmpfile 后再 `#include` 原 `seed/gen.c`,种子不动、缓存键不变;tbl 与 net 本来不用 tmpfile。因此 gen、tbl、net 不再取全局锁,跨作业真并发,lint 与 tbl 到 net 也并行,长网按上次耗时优先起跑。
(2) 实测(2026-11-16,同一夹具,改 parse2 的 control-reject 一行,CXDEV_PAR=4):改前 96.85 秒且有作业被锁拒、需再重载一次(LOCAL_FAIL);改后 52.98 秒、stale=0(LOCAL_OK),阶段合计约 176 秒压进 53 秒(约 3.3 倍)。单张 parse2 网下限仍是 gen 约 36.5 秒,再降要拆 seed/gen.c 的按子表分片或增量。13 张网全量重建改后耗时未测(UNVERIFIED);改前为 138 秒。lint 并行后瞬时进程数最多为 CXDEV_PAR 的两倍。
(3) 热调用额外开销约 40 至 50 毫秒,主要是每次核 manifest 成员摘要与工作树 stat 指纹;可再降。
(4) 第二期:devsvc 子命令 `devsvc cx-dev` 与 flow 节点 local.run、local.reload;本机基座增量构造(改 src、cx 后换切片);写事件触发的后台预重建。第三期:头文件等包内资源替换、gold 网。
(5) 事实表(exec/facts)改动:第一期只对已是生成结果的表生效(按 net-deps 选网);"事实表 -> 生成器 --write 到覆盖层"未做。
(6) seed 下构造器源改动后需重新 `--build`,否则受影响配方记 CXDEV_STALE。
