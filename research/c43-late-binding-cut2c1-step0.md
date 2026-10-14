# 晚绑定2c1｜数据符号第0步（只是设计，待审）

范围：仅POSIX库数据kind=1；不迁typed/callback/运行桥，不做Windows/L-4，不声称整份lower无地址。保留六操作数`.libraryaddr r,name,depth,class,width,unsigned`，不借`.libraryfn`。新检查第一笔登记；有限表＋共享执行器产流，C适配器只解帧、核provider及按span绑定。

**合同与分区。** 沿USLCON1既有规范序、ID=256+记录序号及kind=1的name/extent/writable/平坦类型六字段；声明与provider分离。扩展模型分区给数据独占新owner，ordinary owner与旧typed owner不变；未知类型/目标具名未覆盖，不按地址存在与否选择路径。模型从合同验证E3数据引用及读写权限，lower按name→ID输出零占位与UNILOW1 role=0 span。数据/普通函数片段分别由模型标定，全输出继续沿2b封套，绑定器只读span不扫描Tape。借用地址首次出现于relocate替换结果；旧数据地址资源必须从这条模型路径删除，不把provider派生supported藏入合同。

**接口（均非稳定；返回0成功，非零失败；getter失败NULL、长度0、具名us_error）。**
- `us_declare_injected_data(ctx,name,const us_signature *requirement)`：kind=1、callkind=0、无参数；无地址声明，禁宿主解析回退。
- `us_register_data_provider(ctx,name,void *address,const us_signature *offer)`：独立登记结构合法供给、借用对象；不改声明/合同，不提前吞掉供需冲突。对象活至对应可调用镜像失效；更换provider使镜像失效、须重新relocate。旧us_add_symbol保持声明＋provider组合语义及兼容性。
- `us_library_data_fragment(ctx,&length)`：compile后读取模型标定E3数据引用；不惰性执行。
- `us_library_data_lower_unbound(ctx,&length)`：**先relocate再查询**，与2b一致；成功或provider拒绝后读取模型标定UNILOW1数据片段。缺provider保留收据；模型拒绝不返回旧成功收据。getter借用期至下次compile/声明或provider配置变更/free；测试即时复制。

**拒绝归属与诊断。** 未绑定lower先拒写readonly需求：`library data: readonly-write symbol ID name NAME`；不得查询provider或公布可调用镜像。relocate在完整span/owner核验后比较独立offer：类型六字段须一致，否则`bind: data-type-mismatch symbol ID name NAME`；offer.extent至少requirement.extent且两者≥类型width，否则`bind: data-extent-mismatch symbol ID name NAME`；requirement可写而offer只读另拒`data-permission-mismatch`。无供给：`bind: missing-provider symbol ID name NAME`。同一供需多错按type→extent→permission处理，NEG各只造一错；结构畸形在登记/模型验证时拒，不计绑定NEG。这是平坦数据类型/权限校验，非USLSIG3完整ABI资格证明。

**验收与拆笔。** A/B同源逻辑名、独立int32对象及同供给描述：合同、模型E3数据片段、未绑定lower/span逐字节同且无两方地址，绑定后读写各自对象/guard不变。readonly写入于lower拒，extent(需求8/供给4)、type(只变signedness)、injected-only缺provider在绑定拒；不得发布导出或改宿主对象。cdx3草稿须把positive/mismatch/missing的lower查询移到relocate之后，readonly先compile再relocate取得lower拒绝；缺provider时仍核E3与lower收据，不把超时/信号/缺工具计拒绝。分笔：模型/参考/NEG(base)→API/接线→隔离fresh。回归七lib＋lib-latebind/core/forward、Tape78格与tapebin；各≤60s，自举五目标。派生从干净HEAD＋本笔文件生成；不seal/整闸/push，审批前不动产品。
