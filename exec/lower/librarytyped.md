# Typed lower 笔A：模型见证（未接线）

基点 967dde1e。仅新增 typed 自有有限表，在 lower/gen-manifest.tsv 末尾追加一个 fact:librarytyped 门控调用；原 LFN/MS 表、通用执行器、W 未修改。构造复用 parsebase，签名校验复用 MS，片段/重定位与调用授权复用 LFN。未增加产品读写、资源或生命周期机制。

定义域是 E3 已验并裁剪的 libraryfn→mov r0,r7→librarycall 模板；不是任意源程序或全部 lower 流证明。仅验证 USLCON1 fixed typed 需求及模板片段；不宣称 SC1 实现、运行时接线、P1–P6 或发布通过。外部 provider/宿主地址不参与本笔模型输入；签名中不允许 variadic/callback。

十二个 full/object×lnx/osx/win×x86/arm 构造变体逐个冷准备；每项均包含 C/P 构造对拍、去门控基点与 off 的 JSON/tbl/net 对拍、on 网络全观察域检查。两侧路径、长度与摘要见 artifact-receipt.json；冷准备墙钟见 cold-results.json。Windows typed 模板按当前目标合同拒绝，不记为 Windows 接线通过。

1029 固定用例覆盖记录 callkind/signature-version/nargs/owner 各256字节轴及模板控制/覆盖/标签/跳转。四片逐字节参考对拍、tbl/net、目录与环境复放，另各目标9例抽样。参考工具沿用独立 USLSIG3 解码器，不创建 plan。域外 Tape 的参考 AssertionError 归机制/定义域错误，不伪装语义拒绝。

采证包装器曾因漏传 MODEL_COM 返回75，已补参数重跑；早期 oracle 二进制与输出同名、jmp 非法夹具已在最终用例前修正，旧运行不作最终证据。所有最终结果以 final-jobs.json 和对应日志为准。

seedmemory 八条受影响路线实测读集不变、C/P及旧构造输出相同；只因读取的 manifest 内容变化而更新 gen 键，RSS按实际较高值更新；parse2键不变。原 seedparse2 两个内存门禁作业的 UNVERIFIED 不因此转绿。

交付五问：复用 parsebase/MS/LFN；不另建机制；模型语义与参考观测分账；未涉及缺key/ABSENT/EMPTY，未折叠；范式进展限于 typed 作为同一有限表构造与片段合同的复用者。重构优先版本收口，下一步先问登记窗；B0 默认空release钩子与共享closure纯重构先于运行时接线。
