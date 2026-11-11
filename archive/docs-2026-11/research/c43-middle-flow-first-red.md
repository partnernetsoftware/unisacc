# 中段流身份实测：首红停止

实际跑过提取和一个E3入口；没有跑完整叶子、没有执行lower。数据仓外 /tmp/c43-middle。预算截至首红约15次工具调用，未绕过红重试。

两候选实际SHA：7000 ba11d636b54adcdc519a81af7553d15ab882ecb85622bdc16968d966acdb8f5f；3000 85cc9b33c2154df9323b4deba4659a2aa7e55c69f0eaf5a70ad2479ed47ee3f8。tests/modelstreamextract.cx 在产品 -run 下执行，两者正式尾部P3模型提取均rc0，各28模型/1236路由行。lnx/x86_64/image/O0的e3索引4、lower索引12，解压长度/CRC校验通过。

|流|7000与3000哈希（相同）|
|---|---|
|E3模型|3ddcd3b3438275b04dc2289374516b4ec736486304c1cf2f04b7dcd96d68b5ca|
|lower模型|bbdb5695e641164855a0e3b10e9afad30de4b2abedc4734a0f4b20ef51075fe5|
|dump-tokens输出|f04c38da76cf2af53a66397a4b711b05de29282b24d4e803a58f37c6a864a5bb|

源为 enum E { A=7 }; int main(void){return A;}，物理位置固定 /tmp/c43-middle/enum.c。共享执行器由当前exec/c/run.c用cc -O2构建；它不是两个候选各自的ISA执行器，所以未来通过也只会是独立模型合同见证，不能混充候选整套件绿。

首个调用：run 7000/e3.net 7000/enum.tokens leaf/enum.c。rc1，stdout文件为空（SHA256 e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855）；stderr终端诊断为 reject: not covered: malformed token locations（未单独采集原始stderr字节，不编造其哈希）。随后停止，没有执行3000 E3、lower、拒绝枚举或新接口重试。

只读定位：exec/c/compiler.c:610 在dump模式选择tokens路由，而正常image路由选择目标/.../image/O0；二者输出协议不能混同。exec/parse2/tokenlocations-result.tsv:2 明确UNITOK1/2和UNIPP1帧协议，tokenlocations-byte.tsv:3–7和:13–14拒绝格式错误；tokenlocations-manifest.tsv:18定义对应诊断。实际enum.tokens是普通token行并带“19 tokens”尾行，并不是产品E3所需的定位帧。这首先证明探针选错阶段接口，尚不能指认模型非确定性或宿主泄漏。

独立入口确实可提取，执行器也可载入模型并拒绝；接受输入合同尚未接齐。还需定义E1→E3的真实定位/属性流、cli/source等显式资源，并核对候选自己的共享执行器语义；exec/c/run.c:754–764目前还合成cli/f1、cli/attributes、cli/source并读UNISA_ATTRIBUTES。这是必须显式列出的适配输入，不能只哈希裸token输出声称“相同输入”。需cc确认下一刀后另做新探针，本刀首红停止。

结论：独立模型可提取；是否相同完整输入与模型产生逐字节相同输出，尚未得证。零新增摘钩资格，旧闸不改。

执行器观测身份：原生run SHA256 4e082dd699c7b2b12453f06d08da8bb8e8a6bb7c4baa56203692ee7994ccbc64；exec/c/run.c源码eb221857e5966f41047933914437e28c89e2d75fc8e0db68dedd3b4795b39d54；core.c源码6845345a6d2a38bd97221eaf657970f755b69226e4670aab8d624711ea5d24d1。
