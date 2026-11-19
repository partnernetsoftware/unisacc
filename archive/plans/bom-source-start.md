# BOM-SOURCE-START：方案③与k2组合候选（新增输入预检中）

**身份与窗口。** 私有基点`9d52725be1`，旧03受测冻结`eb2c3b3043`；cc已采信旧证据并开主树合入窗，但要求并入grk的k2补丁后重跑三步，旧03不能覆盖新输入。工具撞名main已为`cfdef121c3`（私有移植`eb2c3b3043`），不重复合入。发布矩阵不继承c46-1资格；link-map仍0.0.47 P3／UNATTRIBUTED。

**追加k2输入。** `exec/facts/k2-gen2.tsv`由正式facts-gen write/check纳入，reverse dry-run fuzz0与`/tmp/grk-k2-gen2.patch`精确一致：string.h哈希头更新及autonames多strdup。其依赖`k2-libraryenv.tsv`仅输入身份头漂移，compare-body SAME后正式重生；FACTS_CURRENT tables33 diff0。两表都映射errorparse/shared-e3/warnparse三组gen/tbl/net，须与BOM三组一起按真实reads重选闭包。生成器既有逐行扫描不解释ifdef，因此strdup进入全平台autonames；记录风险不另扩生成器修复。旧03无include的strdup复制/释放探针rc0，新组合候选及旧样例字节回归尚待实跑；若有新行为差异先报cc。

**实现。** 无强制include的C主源在P0S探测；有强制include（含`.cx`自动cx.h）的主源在CLI.INC.next的INPOP后、MARK CLI_Z前探测；每个头在INCH物理入口探测，不能在INCH→P0拼接后吞。仅完整起始EF BB BF推进3字节，短/错配原子回退、不部分消费。CLI拼接后转P0S.text，避免空前缀把第二个BOM再次当物理起点。E2为B类表规则，不加OS分支或公共读口剥离。前置cxmod为A类同源薄镜像：cxm_load把初始三字节置空格，保行列；权威`cx/cx0/plan.cx`经正式cx0-extract生成`src/cxmod.c`，EXACT sha3170aa46；`cx/compiler/cxmod.cx`同步规则。C宿主驱动与真实新产品共用夹具验证module/use闭包及拒绝，不做实现文本断言。

**合同。** 覆盖主C/cx、显式强制头、直接/嵌套头及返回父源、BOM后首行module/use、BOM+CRLF、纯BOM与空源、BOM紧接#include/shebang；普通中部/CRLF后/重复/UTF16/截断/错配不新增合法化，E1拒绝与退出码保持，字符串/注释字节语义不改。pos_exact锁定正文与记录，BOM后#error/CRLF诊断行号保持。

| 分类 | 冻结产品差异 | 实际增删 |
|---|---|---|
| B：物理入口 | `exec/pp/text-byte.tsv`、`cli-byte.tsv`、`directive-body-byte.tsv` | +9/-2、+9/-1、+8/-1 |
| A：同源镜像 | `cx/cx0/plan.cx`、`src/cxmod.c`、`cx/compiler/cxmod.cx` | 各+3/-1；C投影正式生成 |
| 派生身份 | `exec/facts/pp-gen.tsv` | +1/-1，仅修正既有pp-targets输入哈希头，正文compare-body SAME，正式write/check |
| 独立已有缺陷 | `src/drive.cx`、`src/stages.cx` | +14/-13、+10/-7；独立提交`03b109984f`，不是BOM附带代码 |

**独立缺陷。** `RUN-FORCED-INCLUDE-LOST`：“无 BOM 对照也失败”，0.0.45的-run同样丢显式头，-E却正常。st_pub重置资源后只发cx.h+接口；现在共同编译配置借用driver的cx.h+forced基列表，再追加本unit接口。driver拥有缓冲，在同步多span/多unit内稳定，下次配置先解除借用。无forced/纯C保留默认零拷贝；未前移整套dv_cli。cc仅批准drive835、stages424/ifs69，TOTAL5872/maxfn不抬。

**实际验收。** `/tmp/cdx-bom-precheck-03`正式核reads复用83份材料，shared-e2/tokenpp/warnpp-shared的gen/tbl/net九作业均rc0；新包entry/include/drive/use四探针0，Linux整片37.70秒/0，finish PASS。包`17cc8fbdc975643a0193952186671d9c1880f551777ad2d71cbe539cf796ad77`，候选`93bc0ec3d3a5270f3ed41c9df218e369a6ef59ea166e09ba0c638d17a8743ad4`。真实产品source-bom **46/0**，含forced-clean与forced-closure；BOM/去前三字节的C/cx及CRLF源、旧候选hello/fact/fib/CRLF的e2/tape/镜像均逐字节相等。E2私有精确探针正**198/0**、拒**201/0**，unverified均0；适配器明确不是独立stream发布材料。source-bom正式REGISTERED/PASS（harness记录bootstrap ae0，实际PRODUCT为私有03，不冒称同候选全批）。新矩阵格仅LOCAL_PROBE，发布资格仍MISSING。十三棘轮全过；独立C引导债保留。旧会话01/02及其失败、session.identity均保留，不重标旧材料或缓存PASS。
