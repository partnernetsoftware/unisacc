# X3：osx/arm64 原生两代不动点与无Python预检缺口

结论：本轮真实执行 Mach-O arm64 G1→G2→G3，G2与G3完整memcmp0、SHA256相同；三代裸片均为3071458B，SHA256 `e5f80facc67e6b419e8824e2fc81efaa24b0f4936250e9ad7a6518c09e3de5b1`。范围为44根源码、固定外部模型/头资源包的原生编译器裸片自举，不是重建模型、宿主工具、完整六片APE或清零引导依赖。

## 本轮执行身份

- 固定REV `58093906dcb356d92df13553776dcb19cbff796a`，使用 release-build 正规 whole-o 44根绝对源与 `out/release-snap-58093906dc` 冻结快照；未修改source/seed/exec/include。
- G1：`out/c48-x2/osx-arm64/slice-osx-arm64`，实际Mach-O arm64。G2实际直接exec G1；G3实际直接exec G2。没有通过Root APE代替两代构造器执行。
- 编排器复用父已实编 `out/c48-x2/release-build`。Root产品与完整包容器均为f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82/5818178B。
- 指定冻结包body为 `out/c47-driver-fix/ape/p3-package.body`，1665266B，SHA256 `0d5696535d0062d038bc71a97b266904b69f174a9b799555c2981d5b182c5ad4`。经现有packagefooter解析器定位 `out/c47-driver-fix/ape/unisacc-next.com` 的package span，与body长度及全字节cmp0，见 `input-identity.tsv`。实际UNISA_CONTAINER使用完整有footer容器，没有替换包内容。
- 首次误将裸body直接作为PKG，真实exit1，产品具名拒绝 `bad or missing package footer`，原 `out/c48-native-selfhost/gen2.{log,stderr}` 保留。修正是载体身份要求，不改模型/资源。

## 可恢复分步命令与实际终态

准备独立输出/TMPDIR后，每次只执行一个阶段；下列ROOT、G1/G2、PKG、OUT、TMP均实际使用绝对路径：

```text
tools/bound 55 env -i PATH=/usr/bin:/bin UNISA_JOBS=1 TMPDIR=TMP2 UNISACC_CTOR=G1 out/c48-x2/release-build --rev 58093906dcb356d92df13553776dcb19cbff796a slice ROOT PKG OUT2 --target osx/arm64
tools/bound 55 env -i PATH=/usr/bin:/bin UNISA_JOBS=1 TMPDIR=TMP3 UNISACC_CTOR=OUT2/slice-osx-arm64 out/c48-x2/release-build --rev 58093906dcb356d92df13553776dcb19cbff796a slice ROOT PKG OUT3 --target osx/arm64
```

OUT2=`out/c48-native-selfhost/gen2-valid`，OUT3=`out/c48-native-selfhost/gen3`，TMP2/TMP3为同任务目录下独占tmp2/tmp3。G2 session81311实际exit0，receipt whole-o ns=30269952000；G3 session67559实际exit0，ns=30419658000，均在外bound55内完成。不得将receipt子阶段墙时冒称编排器编译/快照准备/校验总时长；未独立测量的相位标UNMEASURED。

独立比较 `out/c48-native-selfhost/compare.cx` 默认-run session98728实际exit0，核三代Mach-O magic/arm64机器号、完整SHA与两代全memcmp；`fixedpoint.tsv` 为实际输出。不是仅比较旧receipt。没有缓存PASS、没有额外运行整条hostseed链。三代大产物保留out，不纳入研究目录提交。

## 去PATH Python的实际预检

本轮实际实编当前 `tools/chain-precheck.cx` 为out私有native程序，session88846 exit0。专门构造了清楚标为兼容性夹具的独立git根/会话元数据，构造器位置复制真实G1，随后以 `env -i PATH=.../no-python-bin` 执行真实 `chain-precheck ... probe entry`。PATH目录仅包含指向/usr/bin/git的git，显式核python与python3均ENOENT。

结果真实exit2，`CHAIN_PRECHECK REFUSE constructor must be a native ELF`，见 `precheck-no-python.tsv`。因此证明去PATH Python可运行到真实Darwin兼容性拒绝，不能证明完整生产chain-precheck通过，也不能以手工夹具元数据冒充正规init/正式构造收据。

实读限制：`resolve_constructor`只接受ELF；job原生工具另硬校ELF；probe entry/use、slice及finish固定lnx/x86_64/ELF。现有check-chain-precheck还使用/bin/false、host cc建ELF夹具，不适合作为Darwin全链门禁。

最小后继修复先分清host与target：保留默认Linux合同及已有拒绝，在公共格式判断处严格接受与真实宿主OS/CPU一致的Mach-O arm64构造器及job工具，不能接受任意Mach-O或只删ELF四字节判断。已有Linux目标probe可明确保留为交叉probe，产出ELF不能称Darwin原生；无需为让构造器实际执行就先改全部slice/finish目标。先给Mach-O正例、错CPU/格式/不可执行/ENOTDIR拒绝，再以真实G1和包运行去Python的probe include/entry，后续完整目标与收据扩展另派。该修复本轮未实现；不伪报Darwin预检绿。

## 残留依赖分账

| 范围 | 仍存在的依赖/边界 | 本轮事实 |
|---|---|---|
| 本次原生裸片两代 | 已构造G1及冻结P3包；动态宿主ABI/系统运行库；release-build编排与git/sh/tar/cp | G1→G2→G3实跑相同；未重造G1/模型包或证明hermetic |
| 本次watchdog | `tools/bound` 的cc、uname、openssl及bound.c构造/缓存机制 | 实读明确，未宣称host-C已消除 |
| 正规source-package准备 | `tools/source-package.cx:139` 用显式host CC编译seed/ident.c；context绑定CC/version；sourcepackage声明CC子工具/系统头/链接器/库/zlib非hermetic | 仅实读；未重跑此准备 |
| 完整hostseed重建 | hostseed-step/nativefirst 的bootstrap-ident与bootstrap-host仍明确调用host CC，nativefirst也保留先前helper信任根 | 仅实读；本轮不把裸片自举升级为完整引导链自举 |
| Python来源/协调债 | sourcepackage complete_inputs明示classic18权重与Python源码仍为完整SID输入，不宣称已重构造；旧Python构造协调依赖未在本次两代里重验 | 本轮没有运行Python；去PATH预检在ELF合同处拒绝，不能外推全链无Python |

本轮没有新增通用repo工具、没有修改现有工具或共享文档/表、未注册/提交。后续父可据裸片不动点与明确预检缺口分别更新X3，不能把后者记DONE。
