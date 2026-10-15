# 2c3 E3 callsite：提升后标量尾参模型见证

E3 资格为 depth0/shape0，既有整数 base4|8 保持；double base64 仅变参尾部接受。默认实参提升复用既有 LV 表（float→double、char/short→int），同一 CS 资格表与 P2 失败载荷。固定前缀与结果仍限原整数域；指针、聚合、其他未覆盖类型具名拒。此模型见证不扩称 SC1/P1–P6 或跨宿主 ABI 完成。

复用：LT/LV/MS/LX/MG、P2 err 载荷、既有 typed-reader 准备/缓存 owner 与 ResourceInput；能力集由本 manifest flags/alias 行派生，模型和唯一参考均用 `--librarytyped --librarycallsite`，旧 typed-mode 是表中别名，不另起布尔分支。声明 canonical 与调用点 stack transport 分账：concrete var0/mode1 沿用 libraryvariadic-result.tsv:71/:91，原 MS 不改；USLSIG3 末字节保持0，不能冒称 ABI 已核。

权威布局仅 `librarycallsite-layout.tsv`；facts stem `../parse2/librarycallsite-layout`。九行按 plan/slots/result/count/ordinal/reserved/word_width/args_base/stack_args_base 重序列化为 `=key<TAB>json<TAB>十进制整数<LF>`。基线 file SHA131545875247133b74e580c49af9a5904ab1a7c4577ef5bc697b987456d0b415；156B row SHA2ee06abb0146a76580690a740aa2d437f69e5376a2f64ba541d9faabea77ea5a。E3打印与lower匹配从同一 facts 读九值；ordinal32→40时E3新Tape实际改变，lower旧Tape模型/参考均具名 frame-layout 拒。该变异会与reserved40重叠，只证明依赖，不是有效新ABI。

USLCSF1兄弟流：8B magic、u64 record count；每记录五u64为记录长、ordinal、symbol ID、fixed prefix数、concrete签名长度，然后USLSIG3；记录长含后四字段与签名字节。无地址、无plan handle。Tape span=symbol ID，ordinal留调用帧；基线 PLAN0@[r7+0]/ordinal@[r7+32] 由表推导。lower消费资源key为NUL+library/callsites；MISSING≠ABSENT≠EMPTY，不以磁盘回退补缺。

具名作业 `exec-librarycallsite-reader-model`：动态C读取 `librarycallsite-cases.tsv`，10例（8收2拒），tbl/net×两个观测provider；接受时完整Tape/core/side冻结字节与单一参考均相同，拒绝rc1/空stdout/具名原因。provider只改变宿主观测日志，不进入阶段合同。tokens与签名来自真实C源的产品E1/签名输出，source.c原字节嵌入TSV；生产签名来自真实 `long hostprint(long fmt,...)` 原型（未手造正例签名）。golden只对应lnx/x86_64窄标量源，不宣称跨宿主ABI运行。

冷准备各自独立、≤55s，不能合批或抬bound：
```
sh exec/parse2/librarycallsitecheck.sh --prepare-tools
sh exec/parse2/librarycallsitecheck.sh --prepare-base-c
sh exec/parse2/librarycallsitecheck.sh --prepare-off-c
sh exec/parse2/librarycallsitecheck.sh --prepare-off-p
sh exec/parse2/librarycallsitecheck.sh --prepare-typed-c
sh exec/parse2/librarycallsitecheck.sh --prepare-typed-reference
sh exec/parse2/librarycallsitecheck.sh --prepare-on-c
sh exec/parse2/librarycallsitecheck.sh --prepare-on-p
sh exec/parse2/librarycallsitecheck.sh --prepare-tables
sh exec/parse2/librarycallsitecheck.sh --prepare-net
sh exec/parse2/librarycallsitecheck.sh check
```
默认关E3与既有parse2同字节，单开typed与2c2同字节。仓外9构造模式/42旧入口168步/2693006穷举已跑；主检出重跑见提交说明。readonly本笔不碰7000/env-pin、不发布。

统一seed冻结证据：joint基点812281fe、shared source ee424742；114单步38路C/P trace与C直接wait4均rc0，max34.343s。生产方 `tests/seedmemoryevidence.py` 调用共用 model-reseal.source_hash 和消费方 seedmemory.reference_key；输出同格式读集与C_SHA/BINARY_SHA/REFERENCE_KEYS/C_RSS草稿。gen/parse2成员779/380不变，键d25429cc8f1d8f10/506f259d917b2ac9。消费方派生更新后779实际读源字节逐项不变。串行采证不等于并行守卫通过：gen4/parse2两组仍UNVERIFIED77，冷参考UNKNOWN2；不修改余量/并行数。

六问：复用有限表/执行器/typed缓存与P2载荷；不新增产品读写/资源/生命周期；原型与callsite语义分账、共用校验；三态不混；split-at增加通用owner-prefix，空/-逐字节保留旧图；重构优先版本，不发布。

Promoted-double 实跑：旧七例与新增 double literal、signed-char+float 局部变量、short 例，tbl/net×A/B 与单一参考精确相等。真实 C 源 double floating(double d){return d;} 经实验产品导出的 USLSIG3 double 描述符为 base64/class3/width8/rank2/format2/known3/origin1，产品字节用于核定参考；最初参考附加字段错误轮不计 PASS。参考局部声明观察限单标量与简单数值初始化，不作通用 C 表达式实现。仓外接线实验 V0/V1 已实跑接受，属于单独 Linux 窄运行时证据，不把纯 E3 作业扩大为运行时全链。
