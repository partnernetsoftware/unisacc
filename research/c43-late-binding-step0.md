# 晚绑定第0步（只是设计；未执行，待cc复核）

**产出位置/字节格式。** lower旁生符号流；在同一条已验Tape经过优化/prune后、lower主流产出之前，以独立有限表/共享执行器识别`.hostaddr`需求。它读取同一输入与显式target/program模式，不读provider；主lower输出不改。不伪造最终字节reloc，不把`.hostcall rA,rB`改成槽调用。
侧流严格为：`UNISYM1\n`（8B）；target:u8（1=lnx/x86_64、2=lnx/arm64、3=osx/x86_64、4=osx/arm64）；mode:u8（首刀0=program）；count:u8（0..4）；indirect_hostcall:u8（0/1）。随后count个8B记录：id:LE32（slot+1）、slot:u8（0..3）、kind:u8（1=loader函数取址）、abi:u8（0=legacy/untyped）、reserved:u8（0）。严格按slot升序、去重，无尾随字节。固定名字映射0=dlopen/1=dlsym/2=dlclose/3=dlerror；没有地址、路径、时间、源文本或字节偏移。零需求仍输出12B空合同。abi=0仅证明槽能力，绝不计签名/ABI一致性；本刀不做signature-mismatch计证。

**绑定输入/输出与位置。** 新`exec/c/bind.*`只消费侧流与显式provider结构（target、后端kind、四槽available/address）；先完整校验，成功才给出四槽绑定计划。-o用目标镜像导入provider：available表示既有loader导入能力，地址留未解析；-run用本机provider，或者现有静态Linux的匿名镜像fallback provider，按实际选定执行路线确定，不能以编译器无本机loader错误否定fallback。地址只在绑定计划与既有encoder/装配资源中出现，loader资源延后至绑定成功后供编码阶段读取。挂点在lower之后、enc/image之前；文件创建/截断、memory_map/commit和执行均在成功之后。Tape输出模式不做宿主能力绑定。

**黑盒接口/控制。** 新`--host-provider FILE`只允许削减默认provider能力，不允许文件伪造地址。文件恰12B：`UHPROV1\n`（8B）、target:u8、mask:u8（低4位准许槽；高位0）、reserved:2B（0）。有效能力=默认后端能力∧mask；target必须一致。参考适配器读取相同表；表不是模型输入。新`--emit-host-symbols FILE`支持-S/-c以及-o（首刀不与-run同用），供确定性对拍。lower/索引/provider文件语法/侧流IO失败不创建或改写侧流；lower接受后才写收据，后续绑定失败允许保留侧流、禁止镜像写出/执行。黑盒-o+mask13须取得与mask15相同侧流但绑定rc1。先前版本只允许-S/-c且笼统规定失败不写，已按cc复核(a)纠正：含宿主函数的C源-S会拒，不能凭它观察转发合并流。
合法控制采用forwardmulti既有`int creat(const char*,int); long sysconf(int); int main(void){creat("ran",0644);return sysconf(29)>0?0:3;}`单文件形状，与旧产品分别比-o镜像和-run stdout/退出码。fixture必须确认其完整lower输入确有`.hostaddr ...,1`；若没有则另选明确取槽1的合法输入，不把空探针记绿。正常provider(mask=15)为控制；missing-provider(mask=13，缺slot1)要求rc1、stdout空、输出哨兵逐字节不变、未创建新镜像/未执行；诊断`unisacc: bind: missing-provider symbol 2 slot 1`。provider文件畸形另属适配器输入错，不能充缺槽证据。换地址的单位实验仅调用绑定模块，不执行伪地址：同Tape合同/lower字节相同，绑定计划地址不同；公用CLI不开放任意地址注入。

**回归/边界。** exec-driver-core-modes、forward-multi/com-forward-multi（本机可跑）；forward/com-forward在此Linux宿主rc77，列宿主义务，不算绿。模型自测覆盖slot4/−1、Win目标与参考同拒；每命令≤60s，冷准备单列，不缩比较、不整闸。cc黑盒文件tests/latebindcheck.cx不碰。`library/bindings`资源与`exec/lower/codeprivate-manifest.tsv:4–5`嵌地址路径均不删除，属于未迁移债；Windows L-4另案。本页是实现顺序/接口提案，不是合同已运行或候选已重建的证据。

第①步实际观察：原creat/sysconf控制的转发合并Tape只含槽0/1；增加既有__hostaddr2内建后，仓外捕获器确认合并流line37含.hostaddr r0,2（未执行程序，非候选验收）。侧流模型85例自测rc0，74接受/11拒绝，C/Python构造同字节；地址变化时侧流及冻结lower主输出不变是窄见证，不替代后续fresh黑盒。

黑盒负例经cc复核改为真实转发必需的slot1（mask13）；slot2内建仅作为可选模型观察，不放进黑盒控制。
