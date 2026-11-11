# macOS分发与缓存收尾

X4核心已实核：正规release入口默认有界构造并嵌入两个Mac CX缓存管理器；真实六槽私有APE已打包并分别在ARM宿主和Rosetta x64运行成功。用户授权的旧缓存迁移已执行，136个解包体降至4个，活跃旧映像保留。未安装新产品，未改签名、公开Release资产或系统安全策略。

## 正规载体与检查

父真实pack会话23818实际rc0，最终元数据版本收据为`evidence/full-six-slot-ape-receipt.txt`：六个真实目标输入、包body SHA `0d569653…`，管理器源码`9990884b…`，构造产品`f069d4c9…`，模式BUILT_CURRENT_PRODUCT。私有APE为6,272,866字节，SHA256 `c959543ef41e2ad2102a889c7a47f3bce8ce153a76ba90803f91a488ef19d341`。这才是最终六槽打包证据；早期hello/复用占位槽的私有APE只证明接入，不能算分发候选。

正式cache-retain批父会话48778实际rc0，BATCH_RUN为PASS/rc0/transport0，见cache-register.log；独立检查16/0，原10项保留。覆盖最近N/当前、额外活跃租约、dry-run、压缩键legacy迁移与真实PID租约、同N淘汰、目录/锁/保护清单拒绝。独立私有APE锁忙与坏marker均实际隔离回退hello7，正常退出临时目录剩0；低层generic无manager模式与改前实际cmp0。早期openat未暴露和参数缺失实编失败均保留在进展说明，未删覆盖。

## 真实缓存迁移

包装器已精确改为每次dry/apply调用内先执行新的lsof（子界12秒），解析去重的文件/PID，再直接调用冻结manager（子界45秒），外层60秒。不能复用旧空清单。wrapper源码SHA `d7ebe32953b5ff8d0dfe0550bff12f88a819c82f21ecc7148ce7e22df96ae778`，native SHA `a45d09be1ae94bc69000bfd32664a79a75062deca30e2a1afab21a8f69e0c887`。

本轮fresh dry会话94078实际0：136managed/132计划删除/active0。随后fresh apply会话15562实际0：136managed/132实际删除/active1/keep4；apply重新发现PID18545正映射旧项`unisacc-9e379afec4e07216`，已保护，未沿用dry空集合。原始lsof与解析清单、完整计划和独立盘点都已归档。

| 独立盘点 | 文件数 | 解包体字节 |
|---|---:|---:|
| 迁移前 | 136 | 101662096 |
| 迁移后 | 4 | 9693448 |
| 实际减少 | 132 | 91968648 |

独立postverify会话49376实际0：逐项确认132个DELETE路径均ENOENT、4个KEEP全部实际重算完整SHA一致，目录恰好4个严格命名解包体，审计的1个活跃旧路径保留。当前raw-key文件完整SHA为`e5f80facc67e6b419e8824e2fc81efaa24b0f4936250e9ad7a6518c09e3de5b1`。

真实六槽载体热次--version会话92949实际0，输出0.0.47，实测real1.27秒。Rosetta私有XDG会话53482实际0，real2.17秒，产生x64 raw-key `42ce67ed2f5864d6`的管理标记与lease，证明嵌入x64 manager也实际执行；没有重建载体或污染真实缓存。

## 公开下载与系统边界

公开v0.0.47由gh实际下载成功，6,215,778字节，SHA `fde2db1812533a9a56a66d8e0d1870c310fdcc77d444a2d33210ca96e9ef98fe`，与candidate pin相同；公开资产与上述私有新策略载体身份不同。curl和Safari直连GitHub均出现504。随后官方同字节资产经仅127.0.0.1临时HTTP转存，由Safari下载产生天然quarantine；天然属性首跑前后相同。该测试不等于Safari直连公开资产成功。测试HTTP服务器真实外bound60终态142为清理，不是下载失败或产品依赖。

Gatekeeper保持enabled，carrier和提取MachO评估均实际exit3/rejected；没有移除quarantine或关闭保护。终端sh入口版本0/hello7不等于Gatekeeper信任、Finder双击或Developer ID公证。codesign strict0仅证明ad-hoc签名结构；LC_BUILD_VERSION的minos/sdk13.0是镜像声明。实际平台26.5.1；macOS13本身未实跑，不能记兼容PASS。天然Safari与早期显式模拟隔离的原始小证据分别保留。

## 合同限制

N默认4，当前计入并touch为最近；额外保留存活lease，PID复用宁保留。legacy旧压缩SHA16名字不能当rawSHA，V2标记另绑定原名/type/fullSHA；实际迁移同N处理。未来旧launcher生成的新无标记条目仍保守保留，需再次fresh审计adopt；不宣称所有旧launcher已永久受限。lsof是同一次调用的快照，不是停止世界或恶意同UID/ABA证明。异常信号及解包失败时的临时目录回收未覆盖；已验证正常退出的锁忙/manager失败回退。

小日志、收据、脚本与身份清单见evidence/；大载体、helper、下载和缓存不复制进research。HASHES排除自身，使用CX逐文件重算并复核。迁移包装器和盘点/验证算法均为CX；本轮未改共享管理器、打包器或主文档源码，未提交。

## 冻结执行源码归档

本次仅将已执行的三份CX分析快照原字节移至archive，未改算法、未重新执行迁移，也未移出现行缓存工具或16项检查合同。原路径无工具/测试/注册调用者，静态失败日志仍保留原历史路径。执行身份不因归档迁移：

- [cache-inventory.cx](../../archive/research/c48-macos-distribution/evidence/cache-inventory.cx)：017d4fbcc3c588a3a4c345dace905e370b6248816a9d9e30102236e7d8c73fc3
- [cache-migration.cx](../../archive/research/c48-macos-distribution/evidence/cache-migration.cx)：d7ebe32953b5ff8d0dfe0550bff12f88a819c82f21ecc7148ce7e22df96ae778
- [cache-post-verify.cx](../../archive/research/c48-macos-distribution/evidence/cache-post-verify.cx)：c766ce83f02f722c816eaa538c457c892f51fdd4e913c2b16b90b0c1dd5495c2

这是冻结执行快照归档，不是新增工具、缩减活跃覆盖或放宽安全基线。HASHES以本报告目录为根，仅这三个标准相对路径外指本仓archive；其余仍在本目录。所有旧结果继续绑定上述历史源SHA，未冒充当前工具新实跑。

## 最终安全补丁与新私有载体

为消除本刀新增的真实安全门债，管理器/检查器动态格式改为检查cx_fmt；动态复制改为检查mm_copy，dcap取protected真实65537容量、active_names单槽256、Item.name256及Item.key17。原容量、16断言、时限和安全基线均未放宽。新管理器源码SHA `c3d1eaf2340bddf2f1a205968234bc6acba29af46b89d2b8960a353313a1c424`，检查器源码SHA `9906a6b8082241811f81570953ed1a52bc33fefdf32679efd895bdd82861d904`。旧9990884b…源码和136→4真实迁移仍保持其历史身份，未改旧证据SHA或重演删除。

正式cache-retain会话48059实际PASS/rc0/transport0，16/0；源码冻结后同六个真实slice与P3重新默认pack，会话65827实际0。新私有载体6,274,578字节，SHA `a9c138ec1fc44daea10648ed6c685857d3abe02126084900068901741790de3f`；当前根产品和公开资产均未改。ARM与Rosetta私有XDG实际版本均0、hello均7，且各自产生正确raw-key标记/租约。完整正式日志、默认检查日志、新pack收据/日志、四次运行状态及stdout/stderr、marker/lease摘要与sourceSHA见safety-final/。最终全局13门由父另行收口，本节不将尚未收到的结果记PASS。
