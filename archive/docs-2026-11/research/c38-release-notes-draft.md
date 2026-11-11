# unisacc 0.0.38

主题：集中流水线提效（P1–P7）与 SC1 种子生成器修片。产品验收措辞未改、限时未抬、测试未删。

候选：unisacc-next.com sha256 ba3f40cb…（sources 5782090e），comboot stage2=stage3 定点成立；封存 ghcr.io/partnernetsoftware/unisacc-candidate@sha256:9b9c1022…（sealed_from_commit 2298c31e），rc/v0.0.38 → 4790de9d。出口：本机 Linux/x86_64 完整 queue 650/650，565 PASS；证据包 ~/.unisacc/evidence/c38-final。

## 变更
- 流水线：调度截止/在途/停滞处置、尝试类别持久化、按身份历史、重项单令牌、实时内存准入、出口表与预授权分类、阶段墙钟日志、预热与 pair 安装幂等（P1–P7）。可度量对照见 research/c38-pipeline-baseline.md。
- SC1：seed/gen.c parse2_startup_control 绑定 k2-control 常量（TDN/TDE），seed-construct-parse2 三项对参考逐字节一致。
- seed 内存证据按最终生成器身份刷新（53fe285f）。

## 非通过项（列明，不计 PASS）
- 宿主基线（H1 超时，具名、限时不变）：closure-c1、closure-c2、closure-c3、closure-c4、exec-e5、exec-f1-attributes、lib-callable-variadic-model、shared-e2-plain、stages-1、stages-2、stages-3
- 已裁基线（H1/H2，rulings.tsv）：ccinterop、com-difftest_o-1、com-elfobj、com-fb12-multi、com-forward、com-forward-multi、com-luatests-build、com-multi、com-tapebin-1、com-tapebin-2、com-tapebin-3、elfobj、exec-arm、exec-bridge-linux、exec-driver-language-2、exec-winx86self、forward、hosthdr、lib-lifecycle、realprog、rowcov-enc-1、rowcov-enc-2、rowcov-enc-3、rowcov-enc-4、rowcov-enc-5、rowcov-enc-6、rowcov-enc-7、rowcov-enc-8、rowcov-parse2-build、rowcov-pp-locations、rowcov-pp-shared、shared-e2-located、syscall6、windows-resolver-host
- 宿主不适用（rc77，覆盖义务保留）：exec-bootstrap-osxarm、exec-bootstrap-osxx86、lib-stack-arm、lib-stack-x86、lib-stack-x86-hostabi、lib-windows-gp-native
- 未执行义务（内存证据 UNKNOWN，rc2）：com-seedgen、seedgen-2、seedgen-3、seedgen-4、seedparse2-1、seedparse2-2
- 前驱失败未执行（BLOCKED 27，随根处置）：挂 rowcov-parse2-build、rowcov-enc 等根。
- gate-infra：出口时 NEEDS_RULING（gatedeps 已审核哈希过时）；经独立内容重审确认七个 exec-driver 套件契约未改，机房主任裁定结案（RESOLVED）；版本号变更改变字节与源身份，七套件本窗未复跑。

## 顺延
K2a、K1、F4″、COV1、K2b、K2c、H37、X3、L2、L1′、L1b′、A1、N1 → 0.0.39；W2、E57 → 有 Windows 双 ISA 真宿主的版本；H1/H2 跨版进行。

## 签名与资产
- 资产：unisacc.com（Windows Authenticode 签名，Valid；签名者 PARTNERNET SOFTWARE PTY LTD，Microsoft ID Verified CS EOC CA 04）。签名前 sha256 ba3f40cb…（2243480 B，封存候选），签名后 579f6525ce101ef35100269bcd57ea74e2e3b38bded38ab9dcf014856640d9da（2259256 B）；签名 run 38024592099。
- Apple 签名与 dmg 本版未提供（发布机为 Linux，无 Apple 签名环境）。
- 构建依赖仓外宿主 cc 启动器（仅对 seed/compilerpack.c 加 -D_XOPEN_SOURCE=700），内容与 sha 见 release/c38-host-launcher.json；源码修正为 H37，顺延 0.0.39。
