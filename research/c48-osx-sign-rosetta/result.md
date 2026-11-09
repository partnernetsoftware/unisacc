# OSX-SIGN-ROSETTA：诊断与签后重包候选

结论：当前证据不支持改 `kn_load` 的 RW→RX 或扩大 entitlements。首签裸成员启动时段的 `syspolicyd` 日志明确出现 `Prompt shown (5, 0), waiting for response`，标识为团队 L2N7M5M544 的 `osx-x86_64`、`NOT_A_BUNDLE`；保留 quarantine 的私有裸成员也在对应时段等待。源码正常 `--version` 路由直接早退，不调用 `kn_load`。系统采样停 Rosetta Runtime 不能单独视为 JIT 死锁。日志中的路径已哈希，尚不将每一次后续无 xattr 的超时都断为同一等待，也不声称已证明 Rosetta 缓存损坏。

## 同签名字节的最小对照（本机实际跑过）

原 `out/osx-sign/run2/osx-x86_64` 完整 SHA 为 `3679a611cbc1eb1204e1ff298c5ba2ba2306565792c5a74f8583a3833f2b4ec6`。Developer ID/runtime/唯一 `allow-unsigned-executable-memory` 均严格验证有效。原路径在干净环境与固定 P4 下两次8秒超时142；`cp -p` 到新的私有 inode 后，完整签名字节相同，版本实际退出0。原路径与该副本均无 xattr，没有重签或清除属性。

复现命令如下，副本目录必须新建；本机状态相关，不宣称别的宿主必然重现。运行前后用 `shasum -a 256`、`codesign --verify --strict` 与 `xattr -l` 核对，不以文件名当签名身份证明。

```sh
# 在仓根，原路径/P4均是本轮固定材料，不修改原件。
tools/bound 8 env -i PATH=/usr/bin:/bin HOME=/Users/wjc TMPDIR=/tmp \
  UNISA_CONTAINER=/Users/wjc/repos/unisacc/out/osx-launch-slow/released/pkg.bin \
  /usr/bin/arch -x86_64 /Users/wjc/repos/unisacc/out/osx-sign/run2/osx-x86_64 --version
# 本机实际：142，stdout空。
# 任选尚不存在的私有目录，再复制保留原有属性；不是覆盖已执行的文件。
mkdir -m 700 /tmp/osx-sign-compare-new
cp -p out/osx-sign/run2/osx-x86_64 /tmp/osx-sign-compare-new/dev-id64
cmp out/osx-sign/run2/osx-x86_64 /tmp/osx-sign-compare-new/dev-id64
codesign --verify --strict /tmp/osx-sign-compare-new/dev-id64
tools/bound 8 env -i PATH=/usr/bin:/bin HOME=/Users/wjc TMPDIR=/tmp \
  UNISA_CONTAINER=/Users/wjc/repos/unisacc/out/osx-launch-slow/released/pkg.bin \
  /usr/bin/arch -x86_64 /tmp/osx-sign-compare-new/dev-id64 --version
# 同方法两个新basename实际：0，输出unisacc 0.0.48.1；非全机型保证。
```

独立低层机制对照见 [README-mre.md](README-mre.md) 与 [exec7.c](exec7.c)。六种私有签名策略下 RW→RX 均返回7；该C最小程序没有复现生产挂起，不冒充 Developer ID 最小触发器。当前 Rosetta 的 `pthread_jit_write_protect_supported_np()` 为0，未调用 toggle。Apple 的 MAP_JIT/allow-jit 机制可作为另行设计，但本轮没有证据需要迁移。

## 签后重包候选（实际原字节重包，不改源）

- 路径：`out/osx-sign-rosetta/candidate/unisacc.com`
- 完整 SHA：`5e110594033cbaa9e81d59c8677eef4c2727efa79269d35366ff556b37ec177c`
- 大小：6450974字节；产品版本0.0.48.1；产品源SID仍 `5f0f183f3897129a82bf65743d2c729be350f607ede45bdfdaab096d7fad0bd8`。
- 五角色清单：候选同目录 `members.tsv`，SHA `96ab2e58be8e5a3fd6cb63c3ce7f686d652c45f7aafb7056aa4423c36615af81`，跟踪副本 [members-signed.tsv](members-signed.tsv)。生成与独立strict check均实际退出0。
- 四Mac成员取自run42556245736的公证DMG。实际DMG SHA `24aedbaff7d3528fdb2a92c5dae20387ab531192ec21023cdaf82cea38a8082c`。复制仅补执行位，保留 quarantine；原DMG以只读方式挂载后已卸载。
- `release-ape pack --osx-slices X64 ARM64 --cache-manager-osx X64 ARM64` 把四成员原字节嵌入，独立提取逐字节核同、四个签名strict verify均0、原runtime/team/entitlements保留。P4完整footer包仍fffa2311…，Linux两压缩片与WinARM压缩片和原bdd候选整字节同。没有重签成员、清xattr、改变系统安全设置或动cc的三个签名路径。

正常用户入口为 `sh out/osx-sign-rosetta/candidate/unisacc.com …`。现有包装器在私有目录以内容key首次解压、临时文件完成后rename，固定复用签名后成员inode；不覆盖历史执行过的裸MachO来冒修复，也不每次重写manager。

| 本机实际检查 | ARM | Rosetta x64 |
|---|---:|---:|
| 新缓存冷 --version | 0，0.0.48.1 | 0，0.0.48.1 |
| 新缓存冷 / 随后热 -run r7 | 7 / 7 | 7 / 7 |
| 经新APE的系统FFI探针 | 0 | 0 |
| 独立缓存回归 | 108/0 | 108/0 |
| 热version / run中位数，各10样本 | 12 / 96ms | 45 / 254.5ms |

性能沿用已登记的 Terminal.app 既有责任应用上下文，未改系统设置。实际 `-run` 会进入模型执行内核，故它是团队签名下现有RW→RX能工作的直接反证；不仅测版本早退。四个实际缓存成员的签名也重新严格验证为0。所有子进程均有8秒或原55秒预算，已取得真实终态，无残留。

## 资格与交接边界

这是 **LOCAL-CHECKED 签后重包候选**，没有转移未签bdd候选的42/0，没有发布新tag/资产，没有把新APE或改后DMG冒称已公证。cc若据此集成最终DMG，需要将这份签后重包APE与对应同字节四成员一起装入，再按新身份补六格CI及DMG签名/公证。首签DMG中的APE仍是未签成员版本bdd，不能把裸四成员已签等同于APE已嵌入签后成员。

原裸CLI阻塞证据保留；原宿主的具体路径/inode/系统评估状态并未被全盘修复或清除。没有为假定JIT故障修改冻结的cx/sys/kernel.cx或添加allow-jit。

原始证据：`out/osx-sign-rosetta/{identity-review.md,signed-inputs-review.md,signed-repack-review.md}`、`signed-repack/cache-signed-review.md`、`parent/{signed-repack.log,repack-*-run.log,perf-*.log,loader-system-log.txt}`。只读加载器/官方资料判断见 [loader-review.md](loader-review.md)；系统原文见 [gatekeeper-system.log](gatekeeper-system.log)。
