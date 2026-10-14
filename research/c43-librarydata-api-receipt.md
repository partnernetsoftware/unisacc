# 2c1 数据API／provider接线收据（本机实际跑过）

条款LB-8，四非稳定接口：us_declare_injected_data、us_register_data_provider、us_library_data_fragment、us_library_data_lower_unbound。USLCON1需求、owner4及E3/UNILOW1片段不含provider地址；地址与独立供给类型／extent只在relocate比较并按span接线。已迁移数据片段不等于完整lower无地址；typed/variadic/callback与运行桥债未迁移，Windows待CI／另案。

旧只读检查编码的是旧“不支持只读数据”语义，按已审2c1/LB-8同时更新：read()==99与external check(5)==104，宿主值不变；补readonly-write ID/name、无导出、宿主不变NEG。wrong-width/wrong-kind/high-depth保留。自动dlsym回退只算可信声明，不能据成功记供给ABI/extent已核。

候选：83e6e7e4624c1a6d06771af3347f152187468a01893665e0972860961ca2dafb，隔离tree-r4基于ec66f323＋限定12路径；source-r4-digests.json另绑定来源。r2/r3首红都保留，旧闸不回填、不resume。r4 fresh12步各原窗rc0，新鲜度verified；同源UA自举compiler.c五目标均rc0。

我侧七lib（bindings/registry/resolver/native-exports/module-exports/lifecycle/callable-catalog）＋data均rc0；exec-driver-core-modes rc0/21s，原27模式＋6内存运行控制；forward-multi rc0/1s、com-forward-multi rc0/8s。core独立准备首次55s142，原样55s重试0，随后套件原60s窗0，未减项/放宽。cc侧库黑盒2a/2b/2c1与两生命周期、其余lib收据独立复核；本机UNVERIFIED目标留CI，不代填PASS。日志在仓外 /tmp/c43-librarydata-api/r4-*.log。

## 冻结清单的准确范围

共8路径；API7指下列除prd.md之外七路径。最初八路径冻结后，prd.md因政委与cc新授权更新，并原样带上cdx3范式块、去掉重复自写段；其他七路径从r2到r4保持摘要不变。下面是提交前准确快照，不把文档授权变化说成API变更。

| 路径 | SHA256 |
|---|---|
| exec/c/librarylatebind.h | `304d0b3811757d94d95c9ae820fec44c1c8069c7eab466e73235d7efdadef7e5` |
| exec/c/libraryrelocate.h | `1dc25a84702027d3965bc5a671012a7345a1a3b0ee4de03893f81719b3973cbe` |
| exec/c/libunisacc.c | `fee2ad2337c75e3ab4c479b0802f2ca547a65ad839fa5d41bdb8c48833d6e85e` |
| exec/c/libunisacc.h | `fdd09cd2dde8b03374792ca28c47c73cd15c748a39adf47d7ee294671fb9b87a` |
| prd.md | `ca2808cbb4052bfa8eb3f9afb0f01852a89a467457a1ae554934b77c542a65ee` |
| spec.md | `a8e9b537463b42e4cda9acf7cd70572f4f872e62edff61e40219353d080773de` |
| tests/libunisaccdatacheck.py | `36ea9ff7cc0b03d54386f91a9520fcbe8f1d7ddf246d0b0affb8e1a95c9bb5ee` |
| tests/libunisaccresolvercheck.py | `bd31a979a8657170a7cf217d0c5dbfa4b6ec0a4bb383e41078656fe7d19cbeb2` |

派生在HEAD干净导出＋本笔八路径生成：ABI438项（新增四export与三资源，既有资源编号规范重排），facts30表无变化，reviewed快照随笔刷新。seedmemory读集键与已提交模型笔相同，无API输入读取变化，不重复RSS度量。原始宿主路径／trace只属观测层，未进核心流身份。

后续顺序沿plans范式节：2c1合流后W按kind固化接线层（行为逐字节不变），fixed typed为首个复用者，再S显式流接口；本笔不提前实现后续或发布。

提交闸补记：主检出首轮core/forward/ABI合批rc142且未提交；按同样55s单独准备主检出cache（首142、原样重试0），再按原命令原限时重跑。隔离候选验收与提交闸记录分账，不将准备超时当产品通过。
