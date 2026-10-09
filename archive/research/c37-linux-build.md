# Linux 构建适配（cdx，2026-10-09，实际跑过）

共享 `exec/c/asm/machocc.sh` 适配离线 kernel 构造。Darwin 原 cc/ld 参数保留；Linux 使用 clang、ld64.lld、llvm-nm。C/Python 构造器均走该适配，格式、未决导入、入口、slot、重定位检查不变。默认 C 构造路线不变；首台引导使用仓库既有 SEED_GEN=0/SEED_C=0。build_candidate 的已安装产品前置只在 SEED_C=1 需要。

Linux LLVM 双 ISA C/Python blob 完整同字节：arm64 7672 B，sha256 9bab049932df5fec4a9fc6848995f1663ded2eb3cedcd7cd9b43ec537a65304a；x86_64 7696 B，sha256 56bcd07127466a2201039e885b332974bddf9ba899581268afe4ef0a285aa86d。LLVM 与公开旧 Apple 链接器 blob 字节不同，未宣称相等。原生 x86 产品 ABI netcheck（含 E3 2559878 观察/9923 状态）、资源/包边界/诊断链通过；seedgen e2 首片 e2/ident/blob 3 SAME 0 DIFF。ARM 原生执行尚未验。

从当前源码导出参考 `/tmp/cdx-linux-ref`，每步命令 `python3 tests/bound.py 58 env SEED_GEN=0 SEED_C=0 UA=/tmp/cdx-linux-ref sh exec/c/buildcompiler.sh /tmp/cdx-linux-candidate STEP`；STEP 为 shared、六个 OS/ARCH、pack-prep-1/2/3、pack-models、pack-driver，全部退出 0。容器 `/tmp/cdx-linux-candidate/unisacc-next.com` 2243464 B，身份与探针见 JSON；版本仍 0.0.36，是可移植构造验证，非 0.0.37 封版证据。

产品 g4 双序/完整单文件/原型/float 五组及 C99 64/65/66 同参考 rc=0。原 diag 套件初次因未装外部 corpus 检查 0 项而失败；按仓库上游地址安装语料后重跑：31 ok/0 wrong，40 损坏语料、13 定位、14 register。未改测试或验收文字。seed tbl/net/gen/blob/ident/ape/compilerpack 七个编译成功；pack.c 在 zlib include 处拒绝，保留缺口。gen 初次人为 6 秒限时未完成，拆为独立 bound55 后成功，未提高仓库看门狗上限。

shell syntax、git diff --check、inventory --write --check 通过：408 scripts，orphan 0。候选独立克隆验收与完整发布门禁仍待执行，不记全绿。政委要求本云机继续解决发布流程；后续按实际证据推进，不发布未验收产物。
