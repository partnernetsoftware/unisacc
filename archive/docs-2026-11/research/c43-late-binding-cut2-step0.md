# 晚绑定第二刀第0步｜方向已审，按2a/2b分拆

cc裁定：2a仅立无地址声明合同与快照，消费者不动；2b仅普通函数kind=0/callkind=0，typed/variadic/callback、数据、全部运行桥另案。下文是整体方向，不能作为一次落地范围。2a接口改为`us_binding_contract(ctx,target,&length)`显式target，编译前可查询；typed旧V2合同字节也保留但不计ABI。

**范围纠偏：** 真实冻结是USBIND3；E3已写地址/dispatcher/handle入Tape（`exec/parse2/libraryimports-result.tsv:505,516,534`）。需追加parse2/libraryimports、共享modelbindings、libraryresolver及lower的libraryexit/memory/module/imports表与对应参考，否则仅改codeprivate达不到目标。cc黑盒独占名建议`tests/liblatebindcheck.cx`；Windows/L-4、数组指针案不在本刀。

**a｜确切格式。** 全小端无填充。两头均`magic[8],target:u8(1..4),version:u8=1,reserved:u16=0,count:u32`。合同magic=`USLCON1\n`，从全部声明而非解析成功集合生成，名字字节排序，ID=256+序号，1..16保留运行桥。
合同记录`len:u32,id:u32,kind:u8,callkind:u8,sigv:u8,flags:u8,nameN:u32,sigN:u32,extent:u64,nargs:u32,variadic:u8,reserved[3],name,sig`；len=32+nameN+sigN、不含len自身。kind=函数0/数据1/桥2；callkind=普通0/typed1/variadic2/callback3；flags位0可写、位1injected-only。sigv0：原48字节result/args类型描述，base/shape本地ID归零；sigv3：完整USLSIG3。未知位/目标、重复ID、坏长度拒绝，上限4096。
Provider magic=`USPROV1\n`；记录`len:u32,id:u32,kind:u8,callkind:u8,sigv:u8,reserved:u8,address:u64,dispatch:u64,handle:u64,sigN:u32,sig`，len=36+sigN。仅绑定层持有，**不进模型RI**；来源优先策略显式声明，宿主解析/可用性/计划/借用地址寿命归provider。

|消费者|迁移读入|
|---|---|
|librarybindings.h:116；libraryresolver.h:126–156,194–228|声明→合同；地址/dispatcher/计划/解析→provider|
|libunisacc.c:619→E3 libraryimports-result.tsv:235–254|合同；输出稳定ID，不验地址是否存在|
|libunisacc.c:987→lower；codeprivate-manifest.tsv:4–5私有桥|合同/桥ID；不再setreg真实地址|
|libunisacc.c:687–688 typed/callsite；:820–841 native/variadic dispatch|模型稳定引用；绑定后映射provider计划/模板，保留清理所有权|
|libunisacc.c:598,623–624,975–980其它运行桥|exit/mmap/munmap/process/callable同迁ID；memory/argv执行上下文延后进入绑定/enc，不用假地址自证|

**b｜绑定位置/输出。** E3先以符号操作替代地址立即数；lower模型输出`UNILOW1\n,textN:u64,relocN:u32,text,records`，text待绑定立即数固定`0`；每记录`offset:u64,id:u32,role:u8,width:u8=1,reserved:u16=0`，role=地址0/dispatcher1/计划2。绑定器校验边界、不重叠和占位字节，逆序替换给定span，不扫描Tape语法；成功才进enc/memory、公布镜像。库流独立于UNISYM1四槽；模型/参考/旁流对拍。所有前lower RI须审计，失败清理预留映射、不执行。

**c｜库黑盒接口。** 非稳定只读`us_binding_contract(c,&n)`/`us_lower_unbound(c,&n)`保存无地址快照（绑定失败也可读）。两个上下文同声明/源，注入hostadd的A/B地址：合同、E3 Tape、lower逐字节同，绑定后各执行A/B结果。负例用显式injected-only声明而未注册provider：compile/lower成功，relocate rc1、`bind: missing-provider symbol ID name hostadd`，无可调用镜像/副作用；不靠随机名字假定进程缺符号。需新增非稳定来源策略和独立provider注册接口，旧API一致性检查保持兼容。

**d｜签名不符可计证条件。** 调用方要求A与独立provider声明B，非零、同版本3/目标/调用约定、描述完整，绑定器实际比较类型、布局、嵌套回调、variadic（图ID非ABI）；不等则绑定步signature-mismatch。版本号、复制provider作要求、legacy/缺描述/未覆盖不能计绿。

**e｜回归/落刀。** 7000历史rc0秒数（只读，非本批证据；均lib-前缀）：context3.77/format6.54/bindings2.87/bindings-registry0.30/resolver3.57/resolver-host0.70/native-exports2.90/module-exports2.90/word640.15/sig2-host1.12/typed-host1.18/variadic-host0.85/callplans-host1.98/variadic-resolver-host1.05/callable-catalog8.77/lifecycle8.24/ffi-provider0.30/carrier-import-model35.71。合同模型base→绑定接线→仓外fresh/黑盒；逐套件≤60s，自举编五目标并点名core-modes。请审追加域/API/运行桥边界；回信前不动产品、不提交设计、不seal/整闸/发布。
