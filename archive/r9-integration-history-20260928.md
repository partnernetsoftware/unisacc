# R9 集成与签名资格历史记录

来源：prd.md §0（main ac1e9f5 及本轮工作树）；2026-09-28机械归档。以下为当时的候选/阻挡/分工，不作现状；当前规格见 [PRD R9](../prd.md)。无稳定规范ID被移除。

**当前本地基线**：`44fc348` 构建、`75e54ef` 收尾，`.com` 1,083,311 B，SHA256 `01e5c1d9d4528a2402d212883da0df64f63d60b8f17b5e6461d18e41ed053631`。P3压缩与单次memory已实现，本地179/179通过；Linux arm64三程序另有实际烟测，Windows/Linux x86_64本轮未验。它仍是本地候选，不能写成0.0.9已发布。
**本轮收片候选**：`5eb87bc`，根 `.com` 已重建为1,109,471 B，SHA256 `8bd775ec0a1fd7d3787526f24d34e589ebadb93cc5b764629209869b907de1c6`。相关11项双槽队列21.4秒、11/11通过，包含三条路线的371例转换与真实系统演示；不是最终全套或平台通过。[收片回执](research/r9-scoped-acceptance-20260928.json)。签后APE实际-run通过仅证明私有格式，企业信任/公证仍未达。
**macOS封装细化**：后台封装代理仅新增原生launcher/build/check文件；启动器通过系统CommonCrypto校验封存载荷SHA和manifest，属于发布完整性/IO适配，不新增编译规则；公司Developer ID只用于私有格式资格，公证上传未执行。签名先入口后bundle，记录两ISA及Gatekeeper未公证状态；不通过时不禁用Gatekeeper或去掉quarantine凑成功。

**签名首片实施（main，主代理）**：先在私有目录复制当前 SHA `01e5c1d9…` 的未签名产物，不改根 `.com`，用一次性不受信证书做 Authenticode 格式演练；验证证书目录、签名前后模型包完全相同、篡改拒绝与实际宿主执行。已确认本机有公司 Developer ID Application 身份；macOS先构建最小原生启动器携带封存 `.com`，本地签入口/app并检查运行，尚无公证成功证据。不读取/输出私钥，不复制 minicon 的产品级 OIDC 身份；证书/包尾兼容改动若触及 cc 的 driver 文件，先给出独立定位器与测试，等文件锁释放再接入。演示主代理先加强应用自采快照的结构验证与异常输入，Linux目录枚举涉及新intrinsic时单列依赖，不越域改核心。

**首轮正确性收片**：strtol/strtoul使用两个32位非负long累积，范围钳位/ERANGE、无数字endptr与不完整0x前缀已修；371个base2–36边界加atol有定义/null-end控制通过独立host/UBSan与私有根com+冻结include。Python #if明确拒绝畸形/尾token并传播内部异常，9合法/19畸形/2内部异常/2除零控制通过。后续共享生成头已重建，三条路线转换回归均已通过；最终完整门禁和平台仍待统一冻结，见收片回执。

**执行编排（2026-09-28，用户授权 main 并发）**：所有写者直接在 main，不创建分支/worktree。后台文件域：①正确性子代理仅 include/stdlib.h、unisa/front/pp.py 及专属新回归；②队列子代理仅 tests/gatequeue.py、tests/queuecheck.py 及专属依赖声明；③文档子代理仅 README/ARCHITECTURE/exec入口文档。主代理独占 prd、演示、release签名适配、索引/提交与共享生成物。cc-unisacc 独占 prune、src/main、exec/c driver 与种子接入点。子代理不 add/commit、不改共享UA/根产物、不运行全套；只做有界私有/合成验证。代码变更统一停止后，主代理按片提交、重建、冻结并跑并发队列；源码编辑与候选门禁不交叠。签名与演示涉及新的核心入口时先解除文件锁，不越域抢写。

**三批顺序与并行边界**：

1. **整理/资格批**：文档归档审计（README/ARCHITECTURE/research/prd）、正确性回归（include与前端）、签名格式/流程（release/签名适配）可以分文件并行；共享生成物统一由主代理重建，prd与索引单写者。先记录精确交付和文件域，禁止多个写者改同一生成器。签名兼容原型只用私有候选，不改根产物。
2. **实现/验证批**：落小范围正确性修复、队列闭包、真实演示与签名接入；每片先相关测试，合批后重建一次。源码编辑与门禁运行不重叠。队列改动须有故障控制，不能用缓存隐藏失败。压缩与单次memory的已验内容保持不扩张。
3. **发布批**：统一冻结、完整队列、六目标指定平台证据、真实签名/公证和签后验收、草稿/发布。网络服务以提交/有界轮询分步推进，不放宽60秒上限。外部证书/账号权限与客机可用性单列阻挡，不承诺未核实的完成日期。

**签名的已核实风险与路线**：

- 实际读取minicon的`company-signing.yml`、`macos-signing.yml`、`release-policy.json`、`self-sign-rehearsal.sh`与签名回执/验签工具。复用公司发布身份及方法；unisacc的repo级Environment、Entra app/OIDC与profile级角色授权独立核对，不能借用minicon的客户端身份冒充已配置。本计划不创建第二个公司身份，不记录私钥/凭据。
- 当前`.com`已有空PE Security Directory，但缺VERSIONINFO；`exec/c/run.c`从EOF-16读UNIPKG footer。Authenticode附加证书会移动EOF，必须先定义有界的证书/包定位并验证签名前后**模型包内容不变**、截断/错误偏移拒绝及所有宿主启动路径。不靠搜索任意magic定位可信包，不直接照搬minicon的ZIP检查。
- 按用户校准，macOS优先试签名原生入口的`Unisacc.app`携带封存`.com`，再公证并staple app/dmg；实际编译由`.com`执行。APE不是原生Mach-O，不称其已获得Developer ID签名。必须验证内层执行/缓存与完整bundle公证；失败再评估同源原生内层。minicon签的是内层Mach-O及app，不是只给任意载荷套壳。
- 苹果Hardened Runtime可能限制当前匿名内存转RX与FFI。用实际签名封装候选验证内层启动、编译、-run、系统API调用和最小必要运行配置，不能因为minicon无需entitlement便假定unisacc也不需要。公证Accepted、codesign strict、staple及适用于最终分发形态的Gatekeeper判据分别记录；裸CLI的判据不能套成app判据。
- 本机负责构建，签名工作流只消费哈希冻结的本地产物，不搬入CI编译。资格演练明确非正式发布；正式签名后重新记录产物SHA与来源侧车/签名回执，发布只提升这些字节。签名服务、凭据资格及真实信任结果尚未核实，不把仓库模板当作签名已经成功。
- 官方依据：[Microsoft Artifact Signing角色](https://learn.microsoft.com/en-us/azure/artifact-signing/concept-resources-roles)、[GitHub OIDC接入](https://github.com/Azure/artifact-signing-action/blob/main/docs/OIDC.md)、[PE Certificate Table](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format)、[Apple分发签名](https://developer.apple.com/documentation/xcode/creating-distribution-signed-code-for-the-mac)、[Apple公证](https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution)。实现时对照当前官方文档，不照抄归档计划的过期状态。

**整理的首批明确清单**：README/ARCHITECTURE仍以旧9a0ae470或1dbac50候选和“未发布”作现状；exec/README与rules仍含“产品尚未采用”。这些更新为v0.0.8发布+当前P3/单次memory候选。prd早期训练交付定义、prd.map的14阶段/7,052 B、旧E3研究记录标历史；手写键数与生成表核对，不批量改生成块。`exec/parse/gen.py`仍是被导入的共享组装器，parse/probes仍在keep清单，经典src/unisa/kernel与E0论文复现实验必须保留。归档只迁过期现状/叙事并同步引用，不进行大目录搬迁或凭年龄删除代码。

**本版不捆绑**：库AOT、跨阶段二进制流、重复网络结构合并、系统libc全面转发、新语言/目标、mmap内存重构、全域T2/T3证明。AOT如做只独立测量，不阻挡这份清单；不能同步改参考实现只为凑字节相同。各项完成并验证后即停止扩范围、进入发布。

**签名接入下一片决定**：新增独立unisacc的Windows资格/company工作流与显式签名政策，默认off且缺配置/VERSIONINFO在额度调用前失败；真正落地由主代理检查并配置repo专属OIDC及profile窄权限，不把配置工作甩给用户。签名前PE版本资源必须在构造APE的PE head时追加，不对已打包容器插字节；纯打包IO，原text/data/reloc字节与RVA不动，拒绝已有resource/证书/overlay、header不足及超宽字段，产品名Unisacc与冻结版本精确对应。Linux演示的目录枚举作为R9-5最小切片，只增加getdents64事实与有界目录适配、不加入execve/fork；不支持OS明确拒绝，E3复用INTRINSIC，通用核不加语言原语，生成表/权重由主代理统一重建。

**微软身份实际接入**：已创建 unisacc 专属 Entra app/SP（无密码），绑定 GitHub 实际返回的 immutable repo/environment subject；只授 profile 级 Artifact Signing Certificate Profile Signer，读取核验恰一项。GitHub release-signing 已配置主人审核与 main-only，OIDC三标识存 Environment secrets，provider 坐标存变量；值不写源码/回执。已读回核验联邦配置、profile角色、审核/分支保护及变量配置，并设置身份验证标志。尚未调用企业签名服务，政策仍 off，真实企业签名/额度路径待资格和最终候选验证。

**候选版本校准**：源码版本改为0.0.9供本轮签名资格与真实产物验证，不表示发布完成。新增libneed资源后Windows进口上限由245改244，预留固定9槽与3个尾部CLI资源，保持ResourceInput[256]边界；默认不开libneed。首次整体重构建触及55秒外层预算，按已有shared/六目标/pack入口分批，超时不计通过、不放大预算。

**本轮接入决定**：E2 `-libneed` 增加 CLI 资源传递，默认关闭；闭包与函数体集合来自同一声明表，seen 银行独立于自动头 called/defined。参考缺少新 dirent.h 的2项差异保持待复验，不计通过。APE仅在发布编译器构建时显式增加 Unisacc/当前源码版本的 PE VERSIONINFO，两遍头布局均先增加资源再计算 Unix 偏移；普通用户程序不自动冒用产品名。Linux目录枚举、版本资源及其限定回归收片后统一重建，再冻结正式产物。

**苹果诊断实测**：显式--keychain仍20秒超时；随后仅临时将专用链前置user searchlist并保留原项，同一identity的私有无时间戳签名0.06秒rc0，finally恢复并读回原搜索列表一致。本机已存minicon-notary profile通过真实认证。尚待最终载荷timestamp签名与公证，不把该诊断算最终信任。

**苹果阻挡的最小处理**：minicon已验证配方注明公司P12/密码在本机私有vault，且minicon-notary已存为钥匙串profile。核验文件类型与0600权限后只导入一次性专用钥匙串，设置该新钥匙串的codesign分区权限；不导出现有私钥、不改login钥匙串ACL、不把密码/Key ID写日志。签名工具增加显式--keychain，仅作用调用者选定的私有链，用后删除；先私有诊断，再用最终冻结载荷签名/公证。

**苹果签名阻挡定位**：额外私有入口以`--timestamp=none`诊断45秒仍超时，已清理所属进程组；实时sample显示停在`SecKeyCreateSignature → SecurityServer::generateSignature → mach_msg`，因此阻挡在系统签名/私钥服务等待，不能归因为时间戳HTTP。是否有钥匙串授权提示已向用户核实；未导出私钥、修改ACL或假称签名成功。微软产品级签名workflow/policy正在独立准备，尚未配置仓库Entra/OIDC资格，不复用minicon身份。

**并发收片实测（2026-09-28）**：队列仅docs/bound两个审计闭包实现选择性失效，未知族仍全局失效；合成3项复用约0.025秒，输入/候选/环境/缺项与未审代码变化控制通过，不能称179项均已选择性复用。真实演示新增winlist与进程/映射/窗口结构校验；合成控制通过，最终产品实跑待统一冻结。Windows签后footer定位器在冻结产品和host ASan/UBSan各160例通过，接入将复用已读内存而非二次打开文件，格式校验不代替签名信任。macOS原生双架构入口/app/DMG私有ad-hoc格式、参数/退出/运行/FFI/篡改验证通过；Developer ID带时间戳两次20秒超时，Gatekeeper拒绝、公证/staple未做，企业签名未完成。所有格式演练均不改变根产物。

**收片补充验证**：新driver已实际读取私有签前/签后同包，hello流水线输出1075字节完全一致（SHA `09e8e1fe6a814600c33170f9fc3e880cc959249f1e7b6b25c2568ca76cd10b81`）。当前头文件生成物已统一重建；转换371例在Python种子及私有经典C参考实跑通过，host UBSan和#if回归通过；最终模型产物验证仍待重建。libneed默认关闭的参考切片来自57db2eb，修正其测试不使用Perl、两边编译必须成功且超时不可计一致；私有经典参考14例行为一致且均减少标签，模型δ尚未接入，不称产品速度收益。

