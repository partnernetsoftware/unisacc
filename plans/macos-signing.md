# macOS 签名、公证接入方案

决策：首轮真签绑定下一次 **0.0.48 重建后的、主人审阅的 Draft 候选**，由人工 `workflow_dispatch` 启动受保护 CI 环境。本机只执行显式私有副本的 ad-hoc 测试与不执行命令的 dry-run。此方案和新工具没有创建 workflow、签 Developer ID、向 Apple 提交、公证、安装或改 Release 资产。

## 当前身份与已测边界

实际读取 `src/version.h` 仍0.0.47，`release/candidate.tsv`仍指0.0.47候选；本仓0.0.48计划§16明确e586是内部候选，不是0.0.48发布件。不能拿这两个旧声明去签成0.0.48。本机现有 Developer ID 的存在不构成本任务使用该证书的授权。

本轮独立check先31537/exit0、17项；最终51940/exit0、18项，保留全部原断言并增加未审泛权限profile拒绝：标准MachO、manifest全成员SHA与角色/CPU、版本/Draft/源commit语法、重复/缺失、CR/NUL、符号链接、旧输出及签写失败传播。夹具仅一致性，不是当前候选资格。

真实G3副本采用 `codesign --force --sign - --options runtime --timestamp=none --entitlements 空dict`：

| 架构 | 输入SHA | 实际结果 |
|---|---|---|
| ARM64 | 9083a6ccd9794388443949f1b25ddfd7535c8d0716bdfa1a86f9359b797cc2d5 | 75520/exit2；结构验证0、version0；hello默认运行时编译器进程SIGKILL，np rc=-9，25ms、stderr空。不能记可签运行通过 |
| x86_64（Rosetta） | 1da67e0ac615c80f3daa08ab1dc117d944b2355a0742bfeb49134f284d5262f0 | 62195/exit0；结构验证0、version0、hello7、系统libffi探针0；spctl3、裸MachO stapler65，无信任/公证结论 |

两者均固定外包f069完整载体/0d569正文，输入执行前后实际SHA核同，未修改G3原文件。x64签后SHA为8dc96cec370061a7c24e57b3c73f74d5d890fde3fc6585a140e8fb54d8587635。小日志与原始命令在 `out/c48-macos-signing/`，本机26.5.1；没有macOS13或Intel硬件新实测。

## hardened runtime 的实际阻塞

`cx/sys/kernel.cx kn_load`并非只解释数据：把包内UNIKERN1机器码映射RW，补服务函数地址，`mprotect(...,5)`改RX，再通过函数指针执行。该映射不使用MAP_JIT；源码还保留可显式替换的外部kernel路径。`src/host.cx hs_forward_exec`与`cx/sys/runlaunch.cx`的Darwin默认运行则写临时文件、fork/execv，不应把二者混称同一种JIT。

ARM系统原始IPS已取到 `SIGKILL (Code Signature Invalid)`、`CODESIGNING/Invalid Page`，故障位于VM_ALLOCATE的r-x页，见 `arm64-empty-entitlements.ips`。结合上述源码，空entitlements不能作为当前ARM编译器的发布配置。Apple说明 allow-jit对应MAP_JIT，而 allow-unsigned-executable-memory对应不受MAP_JIT限制的执行内存；改变映射实现与单独授权该例外是两条不同选择。[Apple JIT entitlement](https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.security.cs.allow-jit)、[Apple unsigned executable memory entitlement](https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.security.cs.allow-unsigned-executable-memory)。

经父先登记批准，已增加显式 `unsigned-kernel` profile，仅添加 `com.apple.security.cs.allow-unsigned-executable-memory=true`，保留runtime及原失败。首次32377/68922均实际0；最终工具启用干净env后48342/7823再次实际0：ARM原生及x64/Rosetta均version0、hello默认-run为7、系统FFI为0。还故意向外层传错UNISA_KERNEL/UNISA_CONTAINER，子进程使用env -i的显式包及私有TMPDIR，两者仍0；本机实测根仍f069；同步进main的3d0安装记录属于其它宿主，本测试仅认显式固定f069字节。最终ARM签后SHA72777013b7c4edc20658f80d39cc7db2f40b9423f25e086efb0b4199e74fcc07，x64为824b34ed4ed0501c4701e0e0d61be8380916817bf0c03a5bd4fa037773cdc656，spctl仍3、裸stapler仍65。这是本机ad-hoc兼容对照，未证明Developer ID、公证、Intel硬件或macOS13资格。

没有添加disable-library-validation、disable-executable-page-protection、allow-dyld-environment-variables或get-task-allow。cache helper默认空entitlements，仍须真实签后测试。系统libffi通过 `/usr/lib/libffi.dylib`加载、使用ffi_call；本轮没有发现ffi_closure分配，但未宣称任意第三方动态库兼容。Apple要求hardened runtime，并指出开发调试entitlement可能导致公证拒绝。[Apple notarization issues](https://developer.apple.com/documentation/security/resolving-common-notarization-issues)。

## 发布顺序及格式

1. 读现有owner seal，固定source commit、实际0.0.48版本、Draft release/asset ID、未签APE/两Mac slice/两manager/完整models SHA；真实校验关系和候选资格，不能只信manifest自述。
2. 私有复制两Mac slice及两个helper，按各自已审entitlements显式签Developer ID Application、runtime、secure timestamp；逐个verify，再真实编译器version/hello/default-run/FFI与ARM/Rosetta检查。不用递归`--deep`代替成员清单。Apple规定Developer ID、hardened runtime和时间戳等前置，失败日志必须保留。[Apple notarization workflow](https://developer.apple.com/documentation/security/customizing-the-notarization-workflow)。
3. 用既有 `release-ape pack --cache-manager-osx` 把签后四Mac成员嵌入，原四非Mac片及models保持owner seal绑定；gzip CRC、偏移、raw/key和所有收据/SHA重新生成，签前旧SHA不再用于签后验证。若还要Windows签外层PE，必须在APE最终重包后按已有Windows流程执行，之后才造DMG，不能重包破坏其签名。
4. DMG中除最终APE，还明确放四份**同字节签后MachO**以供Apple检查；不能假定notary服务能识别自定义APE内gzip。此布局是否被Apple接受仍待首真提交，不以设计冒接受。创建UDZO DMG，再对DMG签Developer ID和时间戳。
5. `notarytool submit SIGNED.dmg --key FILE --key-id ID --issuer TEAM_ISSUER --no-wait --output-format json`；将实际submission ID持久化。以后每次独立`info ID`读取≤55秒，In Progress记PENDING，拒绝保存`log ID`，不盲重传。上传超时且未取得ID为UNKNOWN_SUBMISSION，不冒未提交或失败后自动重交。notarytool的等待超时不取消服务端处理；不用本机无限`--wait`。[Apple workflow](https://developer.apple.com/documentation/security/customizing-the-notarization-workflow)。
6. 只在Accepted后`stapler staple`、`stapler validate` DMG，核Gatekeeper并重新计算**staple后DMG SHA**。裸MachO/APE/zip不支持直接staple，本机Xcode stapler帮助已实际核其支持UDIF、可执行bundle、signed flat pkg；裸文件65是边界，不可删断言凑绿。发布前再做真实下载、天然quarantine与运行验证；不建议移除xattr或关闭系统策略。

## CI secrets 与人工权限

拟新增独立macOS workflow，`workflow_dispatch`无push自动真签；环境`macos-release-signing`设人工review，`concurrency`按固定候选SHA且不取消在途提交。资格job仅读/核现有Draft（仓内旧流程读私有Draft需contents:write，不能把该权限理解成允许发布）；签名job只下载资格job产出的固定hash材料，无Release发布步骤。

Secret建议：P12 base64、P12密码、临时keychain随机密码、Developer ID证书SHA1（不是模糊名字）、App Store Connect team API key的p8/key ID/issuer UUID（issuer不是Developer Team ID）。CI受保护环境注入；不入git、不写plan、不上传keychain/私钥/ENV快照、不输出密码或认证verbose。临时keychain仅runner目录、授权codesign访问；`always`清理keychain及已知私钥临时文件，恢复runner原search list；自托管runner尤其不得遗留。GitHub官方给出secrets导入临时keychain及清理方案。[GitHub macOS certificate guide](https://docs.github.com/en/actions/how-tos/deploy/deploy-to-third-party-platforms/sign-xcode-applications)。

workflow/action采用仓库已有固定commit pin模式，source checkout固定40位commit，禁将未校验输入插进shell代码。所有签名/打包/submit/info/staple子各≤60秒；不可用或处理未完即分相位持久化，不能提高watchdog或缓存PASS。首真签得到实际Accepted、可验证ticket及发布后运行证据之前，目标仍未完成。

## 当前工具接口与限制

`tools/macos-signing.cx --dry-run CONFIG NEW_OUT`只验配置与实际成员、生成identity.tsv/完整流程计划。配置严格TSV：`MACOS_SIGNING\t1`；release_version/source_commit/draft_release_id各一次；artifact行依次ROLE、绝对PATH、实际SHA，六角色必须恰一：osx-arm64、osx-x86_64、cache-arm64、cache-x86_64、models、candidate。工具拒绝旧0.0.47发布计划、坏hash/CPU/重复缺失/NUL/CR/符号链接及覆盖。它**不证明Draft真实存在、source↔candidate↔成员来源关系或实际二进制版本**；未来资格job必须调用现有seal验证补齐，plan含待提供材料的REPACK_REQUIRED，不冒可直接执行的CI工作流。

`--adhoc-test osx/ARCH INPUT SHA MODELS MODELS_SHA NEW_OUT`仅签私有复制，硬固定sign `-`/无timestamp/空entitlements；尾加`--entitlement-profile unsigned-kernel`才启用唯一已审例外，其它profile拒绝。实际version、hello默认-run、系统FFI使用干净环境和固定材料，收集只读spctl/裸stapler状态。任何Developer ID身份、submit、staple写入或上传入口均未暴露。默认check不隐读out材料，真实G3测试由显式CLI单独执行。ARM空配置明确失败保留，单例外本机可跑也不包装成全流程可签发布。
- 工作流：`.github/workflows/macos-sign.yml`（workflow_dispatch，环境 release-signing；entitlements 见 `release/entitlements-unsigned-kernel.plist`；未运行）
