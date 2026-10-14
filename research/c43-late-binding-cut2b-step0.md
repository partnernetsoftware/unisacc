# 晚绑定2b第0步｜普通函数单路径（仅设计，待审）

**范围/证明边界。** POSIX，`kind=0, callkind=0`；typed/variadic/callback、数据、Windows及全部运行桥留旧路径，不比较ABI签名。cc已认可片段级验收：普通函数合同、E3包装片段及未绑定lower片段无地址且A/B相同；不声称整个lower无地址。旧输入仍有mapping/argv/process_slots（`exec/c/libunisacc.c:1006–1007`），process地址可进入输出（`exec/lower/librarymodule-result.tsv:25,32`）。不得把片段投影计作整流封存。

**a｜唯一归属。** 输入为USLCON1声明、显式target与启用2b的协议版本；模型按kind/callkind分类，不看地址/解析是否成功。普通函数独占新路；其余已知类别独占USBIND3旧路；未知类别/版本拒绝，不降级。适配器按模型产出的分区冻结旧资源，普通函数记录不得再带入旧USBIND；源内定义优先，未引用声明不生成绑定需求。负例：同一ID同时出现在新需求与旧普通记录→duplicate-owner；符合新类却缺合同记录→missing-owner；缺provider不得变成旧路。调用约定欠覆盖显式拒绝，不以回退掩盖。

**b｜数据与改点。** E3在`exec/parse2/libraryimports-result.tsv:235–254`旁增USLCON1解码；普通包装的`:534`地址立即数改`.libraryfn r1, ID`，`:535`仍`.librarycall r1,r0`；`:505,516` typed/variadic保持旧。新操作有意不借用0..3的.hostaddr，需追加exec/tapecheck及unisa/tape.py的操作数形状合同（索引合法性/模式/合同存在由lower拥有），申请这两处必要扩域。共享绑定解码新增无地址分支；lower/libraryimports继续验证参数，新增libraryaddr有限表输出占位立即数与重定位span，普通路径不用codeprivate私有桥。参考同步对应库导入与lower逻辑。

UNILOW1框架小端：`magic[8]=UNILOW1\n,textN:u64,count:u32,text,records`；每记录`offset:u64,length:u32,id:u32,role:u8=0,reserved[3]=0`，span恰指向占位ASCII `0`。模型同步给出普通包装片段的起点/长度；绑定器仅按span校验范围、不重叠、ID所属与占位，再逆序替换地址十进制，不扫描Tape。Provider是绑定层ID→地址表，不进新模型资源；普通函数宿主地址首次进入迁移路径在us_relocate的替换产物，之后enc/memory。旧桥地址仍更早存在，明确未迁移。与四槽UNISYM1独立。

**c｜黑盒接口。** 非稳定只读`us_library_e3_fragment(c,&n)`、`us_library_lower_unbound(c,&n)`：返回模型标定的普通包装片段快照，后者附对应span（偏移相对片段），有效至下一compile/free；失败NULL、len0、us_error。不是全Tape/全lower API，完整旧us_tape仍可另观测。A/B相同声明/源、不同hostadd地址：USLCON1及两片段逐字节同、无两方地址窗口；relocate后分别得到A/B结果。新增非稳定`us_declare_injected_import`声明普通函数且不注册地址：compile与未绑定片段成功；relocate具名missing-provider（ID+name），无可调用镜像、无副作用；旧us_add_symbol作为声明+provider组合保持兼容。来源策略不得伪装签名校验。

**d｜验收/顺序。** 模型分区/参考对拍/双归属及缺归属NEG（base）→接线→仓外fresh→cc库黑盒。七项lib-bindings、lib-bindings-registry、lib-resolver、lib-native-exports、lib-module-exports、lib-lifecycle、lib-callable-catalog，加lib-latebind、exec-driver-core-modes、forward-multi逐项≤60s；carrier-import-model另窗。五非本机目标自举编译仅编译证据；forward/com-forward本宿主rc77、Windows/macOS执行另列义务，不计通过。历史2a秒数仅参考，不充本批证据。未动产品；请审接口、片段框架及必要扩域，回信后才实现，不seal/整闸/推送。

操作名修正：先前版本误用了已存在的六操作数数据.libraryaddr；现用独立.libraryfn，不改变旧数据语义。
