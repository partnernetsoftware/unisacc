# DEP-SPLIT：共享facts及生成器拆分

结论：先拆实际消费，再改依赖声明。当前首刀已在HEAD `0794f90f5772b9bd1d9a85f15a696cd63f7e4666`的out隔离git archive中完成AUT/AUD小视图及真实消费迁移，shared-e3的JSON/TBL/NET字节对拍；这不构成全方案完成。当前全图facts仍30/42，numeric-names仍27/42，generator仍42/42。主树exec/src/kernel/include/weights/seed/cx没有修改。

## 实测与复算

复用`tools/net-deps.cx`及BASE5的`plans/seed-generator-functions.tsv`（293×42）、`seed-generator-split.md`、`shared-facts-generator-deps.md`，没有另造调用图。静态闭包为42网/925文件、0缺失，条件when/flags未求值；以下不能称运行覆盖。

| 当前共享源 | 总度 | 前端等 | lower | encoder |
|---|---:|---:|---:|---:|
| exec/facts/parse-constants.tsv | 30 | 6 | 12 | 12 |
| exec/facts/numeric-names.tsv | 27 | 3 | 12 | 12 |
| seed/gen.c | 42 | 18 | 12 | 12 |

工具：`sh ./unisacc.com tools/dep-split-probe.cx -- --facts GRAPH`消费net-deps原5列，不重建图；`--autoscan ISOLATED_ROOT FACT_SHA MANIFEST_SHA`要求显式隔离marker、两原文件完整SHA、唯一AUT/AUD、精确3个旧stem引用、新view不存在；遇身份/形状失配先拒绝。输出仅ACTUAL_CONSUMER_CHANGED_NOT_EQUIVALENCE，不自行赋予等价资格。必须在没有并发写入的私有快照调用，不能把marker当恶意文件系统防护或生产原子事务。独立check复用全新临时构建的当前工具，覆盖真实改写、选中行顺序/完整内容、声明和绑定同迁移、重复应用、两SHA错、隔离marker错、缺AUD、consumer形状错，共9项；没有覆盖所有可能图语法，不能称通用图验证器。

完整canonical图的195个facts中，排除上述30/27两表后，实算最大为top-modelsignature-const.tsv的16。因此条件方案若真正把两高共享表分别拆至≤12，同时保留其它真实边，预测完整facts max为16/42=38.10%，满足≤25。这个数是明确前提下的设计预测，不是当前图实测；TK ABI若仍需30，预测即不成立，当前不得认全方案可达已证。

复算（每命令与子命令bound≤50秒；先对全新archive操作，不能在已应用快照重复）：

```sh
sh ./unisacc.com tools/net-deps.cx > out/dep-split/before-graph.tsv
sh ./unisacc.com tools/dep-split-probe.cx -- --facts out/dep-split/before-graph.tsv
git archive --format=tar --output=out/dep-split/head.tar HEAD
# 解到全新out私有目录，添加字面marker DEP_SPLIT_ISOLATED_V1 加LF。
sh ./unisacc.com tools/dep-split-probe.cx -- --autoscan ISOLATED_ROOT 76a801015e4bc00c2a9d8f217e22cc95acfdae8cfe499d4a9fc864c89ca465ec fbf37c0f7f850e9030b746268f131c161ac62bc591904585132ff66c57fa26b6
sh ./unisacc.com tools/net-deps.cx -- --root ISOLATED_ROOT --touch exec/facts/parse-autoscan-private.tsv
sh ./unisacc.com tools/net-deps.cx -- --root ISOLATED_ROOT --summary
sh ./unisacc.com tools/check-dep-split-probe.cx
```

## 首刀证据摘要（实际跑过）

| 输入/产物 | 字节 | 两侧相同SHA256 |
|---|---:|---|
| shared-e3 JSON | 64072256 | 7b83525bb70b2b11d4477086f47b5bd59e61de15bfc1e301854e0bc7e3c452c5 |
| shared-e3 TBL | 955087 | baa1128d40ee2b33907329d847293da20b4d34f9c47090f852d2018fa144ea8f |
| shared-e3 NET | 910321 | a3b191bc5d1c7edb242e4263ec5fe45b1e7fb61dfd98ee4a3d5d281d14af731d |

独立证据脚本会话71203真实exit0，重读三对产物完整memcmp和SHA。probe/check源码SHA分别9bc0c57a657a693653be9ad213821d02ef5f577c6a007ea6bf2f9af5ff360e44、c298ff6f0348d9a50744b169dbdffe9b20d72192e5ba3b433ac85391afb442da。检查会话84327真实exit0/9项；正式登记会话37877真实exit0，dep-split-probe为PASS/rc0/transport0，候选f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82。该批验证工具合同，不是Linux全批或发行资格。全图195个facts中去除两高共享表后max16（top-modelsignature-const）；该统计只用于设计预测，未改变分类与42分母。大产物及原日志在out/dep-split，不作可跨候选复用的PASS缓存。

## 首刀实际做了什么

`exec/parse/autoscan-manifest.tsv:3`原facts列parse-constants及两个bindmap引用迁至parse-autoscan-private；新表只含原字面AUT19000000/AUD20000000。没有删net-deps边，net-deps源码不变。新view实际touch为shared-e3/warnparse/errorparse共3/42；原parse-constants仍30/42，global没有过门。autoscan tokens配置还令`manifest_graph_visit_row`在`seed/gen.c:4313`硬读parse-constants来解token类，所以不能谎称autoscan完整运行读集只有新2键；后续需迁移这个真实消费，图里保守旧边此时是必要的。

相同当前seed/gen.c由宿主cc编译，cwd均为同私有archive；前后`gen parse2 OUT --locations`实际rc0，JSON同SHA `7b83525bb70b2b11d4477086f47b5bd59e61de15bfc1e301854e0bc7e3c452c5`；随后用同seed/tbl.c/net.c生成两侧TBL/NET作实际比较。宿主C引导依赖明确保留，不冒充自身无C构造。仅实跑一个配方/flags，warnparse/errorparse及全定义域/坏表拒绝没有借此记PASS。材料哈希、真实退出和比较见`out/dep-split/`小报告；大JSON/TBL/NET不入研究归档。

## 全方案的具体切口与定量目标（设计，尚未实施）

1. **把parse-constants拆成真正独立的声明域。** HEADER/FOOTER为程序startup域；GMARK/LOC/FND/UNDO/FR/VS及TDD/TDB/TDN/ARR/PTR/BASE/CUNK/FRD/FRB/DPR/VAR/AUT/AUD/STAG/SSZ/MOF/MSZ/MPT/MBS为前端存储/类型域；SBB/STRUCT_MAX/SIG_RADIX/CTY/UNS/DBL/FLT/FPB/PUSH/POP1/AUTOINC_HEADERS为前端布局/类型/代码域；TK_ID/TK_NUM/TK_BADNUM/TK_OTHER/TK_STR/TK_FNUM为真实token ABI。不能只改facts列：`load_fact`384–456的完整加载/缓存、4313的tokens隐式加载、5198/6073/6398/6564/7357等专用构造入口都必须迁实际读取及严格声明。
2. **把数值routine内部参数与外部token ABI分开。** numeric-result的DIG是scratch bank，TK_FNUM在routine里作为返回kind；`exec/parse/numeric-manifest.tsv:3–5`、prn:4、numout:3、fconv:3、lower/data:4是明确消费点。先证明lower/encoder没有把这个kind输出成外部token协议；若有，必须保留共同ABI或加显式适配，不能把共享值复制后假装独立。按实际所有权分front、lower、encoder三个私有数值参数文件；默认字面可相同，但应是允许独立变化的内部参数，不能继续从一个共同输入文件生成再隐去共同依赖。
3. **numeric-names按routine和拥有者声明，不按目标六份乱复制。** 原prn3行、numout4行、fconv10行，顺序是fresh分配合同。前端/opt/units的调用位于parse2/units:7–8、opt/gen:5/7；encoder位于enc/gen:13、enc/arm:13（含object条件）；lower/data:4。通过三个拥有者的薄manifest绑定至同numeric-byte/result表，freshrows改读该拥有者完整有序names；保留公共算法表真实30边。通用manifest引擎不加凭目标文件名猜测的隐藏特判。
4. **定量预测必须列所有真依赖。** 在上述内部参数独立性与所有真实加载点完成迁移的前提下，现parse-constants30可拆为前端域≤6、lower私域12、encoder私域12；numeric-names27变为前端3、lower12、encoder12。新增私域最大12/42=28.57%，原其它facts保持原完整图的度，不得通过排除文件降分母。若token ABI证据要求跨族30共享，则该ABI文件仍30，方案未达成facts max≤25，必须继续做真实生产者/消费者显式ABI适配或报告阻塞，不能宣布12。验收工具必须遍历全部facts求max≤25/42，而非只看新view；公共numeric-byte/result仍30，table维度不能因此记过门。

## 生成器拆口及不可消除的共享边

沿BASE5既有矩阵：Value/facts reader（load_fact/load_facts_expr）、Graph/state/sequence/output_graph、manifest_walk_block及manifest_graph_visit_row属于真正共享机制。pp_install_prefix/pp_install_standard、construct_lex、construct_opt、nativeabi_rows及lower/enc入口应以独立上下文和显式接口迁出；inspect-only129函数候选另分诊断二进制。122共享候选/41单族候选只是词法调查，不是可删代码和可靠运行闭包。`rd[8]`、fresh_count、pp_shared_predefines、parse2base_procedures必须明确每次构造所有权与初始化，不能先并发化。

按族exe的设计量：pp3、lex3、parse2核心3、opt2、nativeabi1、prune1、lower12、encoder12、其余generic5；generic具体入口仍须按当前42配方逐项对齐，不能凭此合计值替换原recipes。薄入口编辑预测≤12；真实公共解释器/Value/Graph/序列化改动仍42。compilerpack/pack从raw-model输入身份迁到真实下游包阶段，但最终产品仍完整依赖它们；当前单exe未拆前generator42不变。若要求连真实公共引擎也≤25，当前架构无法靠模块化消除这条真实依赖，必须报告该目标未完成，不能改类别排除它或把函数矩阵冒充编译读集。

## 执行顺序与验收

先补全实际facts读集（包括专用旧入口）→内部数值参数/外部ABI逐键用途审计→独立拥有者manifest和有序names→迁移实际加载及依赖声明→42原配方全部JSON/TBL/NET字节对拍及全定义域→坏表/错view/缺键/顺序/类型/宽度/隐式读取负控→完整facts max重新计算。先前坏无关行会令全表拒绝的合同需迁到明确的全局校验阶段，不得靠复用旧view绕过坏表。新view的主表投影器若仍把共同主表SHA传播给30网，就没有达到真实独立。

生成器拆分另依BASE5顺序，接口/ABI、实际exe/模块源闭包、recipe flags、输入与输出SHA分层绑定。任一真正公共组件变更仍按其完整闭包失效。首刀JSON/TBL/NET等价不能迁为全42、六目标产品等价或C0.3生产复用资格。当前状态：局部试拆已验证；完整facts≤25和generator降波及尚未实现，C0.3不得抢跑。
