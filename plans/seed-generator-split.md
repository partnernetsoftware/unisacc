# seed/gen.c 拆分设计（BASE-5）

结论：先建立共享值树、图与清单解释器的边界，再逐个迁出阶段入口和 inspect 命令；不要直接按 42 配方复制生成器。本文只有静态设计与实际普查，未拆分产品、未重建任何网，也未证明编译加速或增量构造收益。

## 已有设计与身份

先核对 `plans/v0.0.48-unisacc.md` §14/B5（G1，只读拆分设计）、`seed/GEN-DESIGN.md`（清单/TSV、插入顺序及字节等价合同）。既有 `tools/func-callgraph.cx` 明示源自 G1 的 cg.py，本次复用该能力；未在受查 plans 文件集发现已交付的同名拆分文档，因此这里补可复算矩阵，不重写 DSL 设计。`GEN-DESIGN.md` 的历史实现状态不能当当前状态。

冻结输入：

| 文件 | SHA-256 |
|---|---|
| seed/gen.c | b5f054db4b3d3a964dc7d4435bcb213d7159dea74a2718fdbe4e18411f9d840b |
| tools/func-callgraph.cx | c6c46741cdbb93bd69fff5fbcb2bf298df9abc04b1759d091f0c623e18f2b1d0 |
| tools/cxlib/sourcepackage.cx | 87e60e8a51484d6f02b1eff96b1b9178c952db31164d3d81c901904eae54d987 |

源文件 9083 行；`--funcs` 默认识别 281 个多行函数，`--oneline` 识别 293 个，新增 12 个单行函数。完整矩阵包含全部 293 个识别项，逐项校验名字/起止行/行数与独立 `--funcs --oneline` 输出一致，覆盖函数体 8963 行，余 120 行属于函数外声明、类型、注释等，未强行分配。该扫描器不是 C AST；“完整”指本工具识别集合，宏生成函数/条件编译后的实际符号集合仍 UNKNOWN。

## 矩阵口径

`seed-generator-functions.tsv` 为一函数一行，前 7 列保留 G1 工具分类，后 42 列按 `sourcepackage.cx:63–101` 的实际 recipes 顺序展开。三个值：`MAY_STATIC_CANDIDATE` 表示该配方所在入口组的词法闭包包含此函数，`UNKNOWN` 表示没有足够依据归给该配方，`DISPATCH` 仅用于 main。绝没有把 UNKNOWN 写成不可能调用，也没有把候选写成运行覆盖。

| 网族/入口组 | 配方数 | 绑定依据 |
|---|---:|---|
| pp | 3 | stage=pp：shared-e2/tokenpp/warnpp-shared |
| lex | 3 | stage=lex：shared-e1/warnlex/tokenlex |
| parse2 | 3 | stage=parse2：shared-e3/warnparse/errorparse |
| opt | 2 | shared-e4/shared-o1 |
| nativeabi | 1 | shared-nativeabi |
| prune | 1 | shared-prune |
| lower-enc | 20 | stage 恰为 lower、enc、enc/arm；含六目标 lower/enc 与部分 object |
| 通用清单 | 9 | warnunits、四个 foreign object-enc、tapecheck、hostsymbols、librarycontract、librarypartition |

42 是构造配方，不是 18 张 gold 网，也不等同于安装包模型索引。尤其 foreign object-enc 的 stage=`enc/unitcontainer-*`，走通用清单分支，不能因名字含 enc 就并入 lower-enc。组内目标 flags/when 未求值，不能据本表证明某函数只影响某目标。

既有工具有两项显著局限，矩阵保留原分类以可追溯，不据此删除代码：一是入口行段硬绑定当前 9083 行源码，lex 8875–8889 带入 inspect-delta-template，opt 8897–8945 带入 inspect-mapseq，因此 `inspect_delta_template` 等被标成生产族候选。二是所谓调用边是函数名标识符命中，未要求后随左括号；调用边扫描只去字符串，未完整去注释/字符常量。回调、宏、全局状态和 flags 也没有精确执行语义。它是耦合候选图，不能宣称严格不漏的运行可达上界。后续精化应从 main 的实际分支/函数名锚点提取根，并对命名直接调用与回调注册分别记边，避免继续维护裸行号。

实际静态结果：122 个共享候选函数（A/C），41 个单族候选，129 个没有生产组证据、仅 inspect 闭包出现，另 main 1 个。按函数体行数，A=3106、C=180；单族 pp=411、lex=215、opt=197、nativeabi=198；inspect-only=3574；main=1082。数字是词法分组及物理行量，不能当可删除代码、语义决策份额、运行频次或编译时间占比。

## 耦合边与切口

以下边已读实际源码；只是结构事实，不是动态命中证据。

| 边/状态 | 源码位置 | 拆分约束 |
|---|---|---|
| Graph → state_index/seq/label/once → finish/output_graph | 类型18–24、graph_state1275–1303、finish3567–3584、output_graph5023–5044 | 状态/边/动作首次出现编号与补 DEAD 顺序是输出字节合同，不能在模块边界排序 |
| Value → facts/actions/template/manifest | value_free155–161、expanded_action199起、load_fact384起 | 值树混用共享与独占引用；value_free 注释仅允许独占临时树，不能统一递归free |
| manifest_walk_block → visit回调 → manifest_graph_visit_row → 递归子清单 | 3870–3971、4191–4200、4201–4736、4537附近 | walker与图执行回调分离，但 env/flags/accum 的修改与递归顺序必须保持；536行 visitor 不是天然无状态接口 |
| pp_install_prefix → pp_install_standard → 通用walker/visitor | 2787–2822、4737–4743 | pp不是全独立叶；迁移时保留共享清单引擎接口，勿复制一份 |
| construct_lex → install_lex_table/template/sourcefacts → finish_lex | 2409–2456 | lex有独立finish策略与envelope/start选择，不可强行替换为通用finish |
| construct_opt → install_prn_call/install_numout_call + fresh_count | 3463起、2570–2629 | opt借用parse/prn与numout；fresh分配顺序/重置必须在同一次构造上下文 |
| nativeabi_rows → fresh_bindings/direct_bindings_ex/install | 4883起 | nativeabi仍共享facts/绑定机制，198行单族候选不是整个依赖闭包 |
| 全局 Reader rd[8]、pp_shared_predefines、parse2base_procedures | 288、2830、3736及9064 | 先封装成每次构造上下文并明确初始化/close；目前不支持据此宣称可重入或多线程 |
| main generic branch → parse2base_procedures + build_parse2_token_graph | 9027–9075 | manifest头决定base/start/flags，不能按文件名硬猜入口拥有者 |

建议顺序（设计，均未实施）：

1. 固定当前 JSON/tbl/net 字节与错误样例，补以函数名定位的精化图；保留全部 inspect CLI。冻结42配方参数和读集身份，不能把当前候选图直接接增量缓存。
2. 抽取内部类型/所有权契约、Buffer/Value/Reader 和 Graph/序列输出层。先保持单一构造可执行入口及原调用顺序，不求立刻按配方独立编译；接口声明与实现沿仓库模块方向，不新增上层 include 聚合。seed现为遗留C99，具体CX迁移由独立实现任务裁定。
3. 抽取 manifest reader/evaluator/walker，再抽 graph visitor。共享解释器服务20+9配方及pp/nativeabi等调用者；此层拆分收益大但回调、借用树、递归env风险最高，先测序列编号、fresh/once和拒绝一致性。
4. 迁移 nativeabi、opt、lex 的专用构造入口（依次小规模验证）；再迁 pp。每一族只薄入口加profile，保持对共享引擎调用。不能为隔离复制Value/Graph/facts机制。
5. 最后把 inspect 命令分派移到诊断模块，把main缩成参数验证/显式dispatch；生产仍保留原CLI合同。129个inspect候选不是删减清单，parse2诊断辅助大量互调，需逐命令比对。
6. 结构等价稳定后才做细粒度摘要与按网族链接。当前 net-deps 对 seed/gen.c 为42/42共享依赖；拆文件不会自动证明波及面下降，需让摘要实际只纳入语义闭包且用抽样重建检验复用。

每刀验收：同冻结输入所有受影响配方 δ JSON、tbl、net逐字节一致；重复构造确定性；非法清单/facts/动作及资源拒绝不变；所有 inspect命令覆盖列表不丢；再由正常慢车道核完整42及六目标。本文没有执行这些重构验收，也没有把已有T1的42配方等价结果挪作本设计实测。

## 实际证据与重算

实际执行：func-callgraph矩阵 session69226 rc0；两个函数清单 session2209 rc0；扩表初次81028 rc2（诊断解析器把flags误认recipe，已保留），修正后61900 rc0，输出 `functions=293 recipes=42 function_lines=8963 shared_candidates=122 no_production_group=129 inventory_exact=1`。所有命令外bound50，未运行构造器或产品构建。

证据：`out/c47-driver-fix/seed-function-matrix.tsv` SHA1831a4a89c0a9d048b9aa12d516f5d86a768b09879d3f431f9d6ac9950dbb897；完整函数清单 `seed-functions-all.tsv` SHA1f5c18e9b634068ec81d4349895caad2613b99ce77adcb539ddbcc6c56d826cc；最终矩阵 SHAc290358f4bc41d939aba6adfc349256267be380bd280385d8f8ba24081c1ecb7。临时扩表程序SHA f5c922012d4a4a0b2a0eb694c1902224f17219c775bce2c8fdc56e4cb03190a7，源码完整附后以免依赖out残留。

先核上表三输入SHA；任一变动不得沿用固定行段/计数结论。将附录保存为 `out/c47-driver-fix/seed-matrix-expand.cx` 后执行：

```sh
./tools/bound 50 sh ./unisacc.com tools/func-callgraph.cx -- seed/gen.c --oneline > out/c47-driver-fix/seed-function-matrix.tsv 2> out/c47-driver-fix/seed-function-matrix.summary
./tools/bound 50 sh ./unisacc.com tools/func-callgraph.cx -- seed/gen.c --funcs --oneline > out/c47-driver-fix/seed-functions-all.tsv
./tools/bound 50 sh ./unisacc.com out/c47-driver-fix/seed-matrix-expand.cx -- out/c47-driver-fix/seed-function-matrix.tsv out/c47-driver-fix/seed-functions-all.tsv tools/cxlib/sourcepackage.cx > plans/seed-generator-functions.tsv
```

附录仅本次冻结格式的诊断扩表器，非新增生产算法；遇清单/数量/函数清单不一致显式拒绝。SOURCE SHA由前置命令核验，不声称扩表程序自己重新计算SHA。

```c
#pragma unisa use cxlib.netpack
static void die(const char *s){fprintf(stderr,"seed-matrix-expand: %s\n",s);exit(2);}
static char *readfile(const char *p){size_t n;char *s=cx_read(p,&n);if(!s||!n||s[n-1]!='\n')die("input missing or incomplete");return s;}
static int fields(char *s,char **f){int n=1;f[0]=s;for(;*s;s++)if(*s=='\t'){*s=0;f[n++]=s+1;if(n>64)die("fields");}return n;}
static int has(const char *list,const char *key){size_t n=strlen(key);while(*list){const char *e=strchr(list,'|');size_t k=e?(size_t)(e-list):strlen(list);if(n==k&&!memcmp(list,key,n))return 1;if(!e)break;list=e+1;}return 0;}
static char names[42][100],stages[42][100];static const char *family[42];
int main(int argc,char **argv){if(argc!=4)die("MATRIX ALL_FUNCTIONS SOURCEPACKAGE");char *mat=readfile(argv[1]),*inv=readfile(argv[2]),*sp=readfile(argv[3]);char *p=strstr(sp,"static const SpModel recipes[]={"),*end;int nr=0,nf=0,nshared=0,nunknown=0;long lines=0;
if(!p||(end=strstr(p,"\n};"))==0)die("recipe scope");while((p=strstr(p,"{\""))&&p<end){char *a=p+2,*b=strchr(a,'\"'),*c=b?strchr(b+1,'\"'):0,*d=c?strchr(c+1,'\"'):0;if(!b||!c||!d||nr==42||b-a>=100||d-c-1>=100)die("recipe shape");memcpy(names[nr],a,b-a);memcpy(stages[nr],c+1,d-c-1);const char *s=stages[nr];family[nr]=!strcmp(s,"pp")?"pp":!strcmp(s,"lex")?"lex":!strcmp(s,"opt")?"opt":!strcmp(s,"parse2")?"parse2":!strcmp(s,"nativeabi")?"nativeabi":!strcmp(s,"prune")?"prune":(!strcmp(s,"lower")||!strcmp(s,"enc")||!strcmp(s,"enc/arm"))?"lower-enc":"通用清单";nr++;p=strchr(d,'}');if(!p)die("flags end");p++;}if(nr!=42)die("not42");
printf("# source=seed/gen.c source_sha=b5f054db4b3d3a964dc7d4435bcb213d7159dea74a2718fdbe4e18411f9d840b\n# cells=MAY_STATIC_CANDIDATE|UNKNOWN|DISPATCH; not runtime coverage; grouped flags not resolved\nfunction\tstart\tend\tlines\tgroup_recipe_upper_bound\tlegacy_class\tfamilies");for(int i=0;i<nr;i++)printf("\t%s",names[i]);puts("");
p=strchr(mat,'\n');if(!p)die("matrix header");p++;while(*p){char *e=strchr(p,'\n'),*ie=strchr(inv,'\n'),*f[64],*v[64];if(!e||!ie)die("inventory count");*e=0;*ie=0;int n=fields(p,f),vn=fields(inv,v);if(n!=7||vn!=4)die("shape");for(int i=0;i<4;i++)if(strcmp(f[i],v[i]))die("inventory mismatch");printf("%s\t%s\t%s\t%s\t%s\t%s\t%s",f[0],f[1],f[2],f[3],f[4],f[5],f[6]);for(int i=0;i<nr;i++)printf("\t%s",!strcmp(f[0],"main")?"DISPATCH":has(f[6],family[i])?"MAY_STATIC_CANDIDATE":"UNKNOWN");puts("");nf++;lines+=atol(f[3]);if(strstr(f[5],"A-")||strstr(f[5],"C-"))nshared++;if(!atoi(f[4])&&strcmp(f[0],"main"))nunknown++;p=e+1;inv=ie+1;}
if(*inv||nf!=293)die("incomplete functions");fprintf(stderr,"MATRIX\tfunctions=%d\trecipes=%d\tfunction_lines=%ld\tshared_candidates=%d\tno_production_group=%d\tinventory_exact=1\n",nf,nr,lines,nshared,nunknown);return 0;}

```
