# F6：新E4候选三阶段固定点

冻结源码提交 `d09bd1decb109213910232e6ae885ce564f11c99`；宿主冻结根 `/tmp/unisacc-c43-e4-summary-final-20261024`，执行环境将同一树映射为 `/mnt`。实际seed、stage2、stage3均2,670,411字节，SHA256均 `2974978e6deec45abf38bfac2ac24e3031969b1c65009798270491d8b1e6dd1f`；源码S为 `c90f0002f02366d510e4c8c22d4757f934edf8c0719852e706b575a7137b47df`，配对build侧车SHA256均为 `b88e3a1c227e72e621cf810cc5975e0922ba34cf34aa9e51288142aaaf94480a`。

## 构建范围与实际分解

stage2重新构造shared、六目标模型及pack-prep-1/2/3、compiler.pkg。原pack-driver的JOBS=1将五目标编译聚合进一个窗口，窗口9在外层60秒退出rc142；该窗口仍为失败，原日志保留于冻结根 `out/f6-e4-stage2-window9.log`，没有改写为成功。

之后逐目标按原50秒子进程界执行相同的 `UA -O2 -b TARGET exec/c/asmcompiler.c`。stage2五切片实际由seed编译，stage3五切片实际由stage2产品编译；十项均rc0，实测elapsed为24.749–28.962秒。两阶段对应切片逐项SHA相同，各自重新编译的来源由argv、输入/输出SHA和状态收据支持，不仅依赖stages.json中的built_by字段。

stage3模型、kernel与compiler.pkg按完整文件哈希及七个原manifest检查只读复用stage2材料；未复制代码切片、step marker或测试verdict，未声称stage3重新运行整个模型构造流水线。材料来源及前后校验留在冻结根 `out/f6-e4-stage3-material-origin.txt`、`out/f6-e4-stage3-reused-materials.sha256` 和material-check/material-final-check日志；必要身份保留于accept-inputs.sha256。两阶段均使用各自ape/ident、原封装argv、同一包SHA、七manifest检查及ident写入/校验，再登记实际阶段并执行官方fixedpoint。

## 保留的收据与边界

本目录仅保留六份小收据：stages.json、candidate.build.json、accept-inputs.sha256、final-proof.txt、comboot-final-proof.log及本说明。final-proof.txt提取本轮实际命令和十切片时间/状态；输出摘要取自两阶段package日志。原逐项材料位于冻结根 `out/f6-e4-stage{2,3}-final-*.{time,status,inputs.sha256,output.sha256,argv.nul}`，不把全部临时日志或二进制副本加入仓库。收据整理只读取既有产物和日志，没有再次编译或运行重测试；本轮未执行installpair，不沿用旧收据中的installpair成功叙述。

这是复用同身份模型数据、重新编译全部代码切片的实际自举固定点；不是完整M7、候选封存、六平台公开CI、Darwin/Windows发布验收、签名、公证或公开下载验证。

旧4f0b493c/c4618639固定点收据可从提交 `b1abb0184f` 的同路径恢复：

```sh
EVIDENCE=$(mktemp -d)
git archive b1abb0184f -- research/c43-f6-final-evidence | tar -x -C "$EVIDENCE"
```

公开重建入口见release/RELEASE-PIPELINE.md及exec/c/comboot.sh；旧路径和旧结果不能作为本轮输入或通过证据。
