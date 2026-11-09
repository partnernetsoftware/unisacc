这是签后Rosetta挂起调查的独立最小对照，不是已证修复。`exec7.c`保持已实跑原字节，SHA256为`ca77f647b80cf5291680724ec87fd4b885852ad5ddeed1dad2824d67f9a775ed`。

源只映射16KiB、写入x86_64 `mov eax,7; ret`并调用，覆盖三模式：`rw-rx`复用当前kernel的RW mmap(flags0x1002)→copy→mprotect RX→call；`rwx`直接RWX；`map-jit`为RWX映射增加0x800。每个阶段写stderr并flush，返回7表示机器码实际运行，返回20/21分别表示mmap/mprotect明确拒绝。

这是任务明确允许的Apple ABI参考：依赖系统Apple clang、SDK头和libSystem，不是unisacc自产或宿主C依赖清零证明。原实测Apple clang21.0.0，`-arch x86_64`；ARM Mac上通过Rosetta执行，不称Intel真机。

必须为每轮使用全新私有输出目录，不能覆盖任何已执行过的MachO；签名也只对本轮新副本操作。下列`NEW_ABS_DIR`必须替换为尚不存在的绝对目录；`mkdir`拒绝后应停止，不能继续复用旧目录。

```sh
mkdir -m 700 NEW_ABS_DIR
xcrun clang -arch x86_64 -O0 -Wall -Wextra -Wl,-no_adhoc_codesign research/c48-osx-sign-rosetta/exec7.c -o NEW_ABS_DIR/unsigned
tools/bound 5 /usr/bin/arch -x86_64 NEW_ABS_DIR/unsigned rw-rx
tools/bound 5 /usr/bin/arch -x86_64 NEW_ABS_DIR/unsigned rwx
tools/bound 5 /usr/bin/arch -x86_64 NEW_ABS_DIR/unsigned map-jit
```

每个执行子上限5秒；预期机器码返回7，不能按shell的“非零即失败”把它改写为0。出现超时或signal时保留原stderr、真实退出/transport/residue，并清理该子进程所属后代；不能仅凭已有日志或marker判完成。

私有ad-hoc对照只使用`--sign -`，不使用Developer ID证书、私钥、公证或真签workflow。先从本轮unsigned复制到尚不存在的新文件（保持新inode），再签名；后续换profile必须再造新副本，不能对已执行过的文件原地重签。例：

```sh
cp -p NEW_ABS_DIR/unsigned NEW_ABS_DIR/runtime-unsigned-kernel
/usr/bin/codesign --force --sign - --options runtime --entitlements NEW_ABS_DIR/unsigned-kernel.plist NEW_ABS_DIR/runtime-unsigned-kernel
tools/bound 5 /usr/bin/arch -x86_64 NEW_ABS_DIR/runtime-unsigned-kernel rw-rx
```

`unsigned-kernel.plist`最小内容如下；`allow-jit.plist`将key换为`com.apple.security.cs.allow-jit`。两权限对照同时放两个true key；空配置用空dict。每种配置均独立新MachO文件。普通ad-hoc对照用新副本执行`codesign --force --sign - NEW_COPY`，不加runtime。

```xml
<?xml version="1.0" encoding="UTF-8"?>
<plist version="1.0"><dict>
<key>com.apple.security.cs.allow-unsigned-executable-memory</key><true/>
</dict></plist>
```

本轮已实际执行18格，数字是真实退出码，不是预期表：

| 私有策略 | rw-rx | rwx | map-jit |
| --- | ---: | ---: | ---: |
| unsigned | 7 | 7 | 7 |
| ad-hoc，无runtime | 7 | 7 | 7 |
| ad-hoc runtime，空配置 | 7 | 20 | 20 |
| runtime，unsigned-kernel权限 | 7 | 7 | 7 |
| runtime，allow-jit权限 | 7 | 20 | 7 |
| runtime，两权限 | 7 | 7 | 7 |

三个20均为mmap的errno13/Permission denied，没有进入写入或call，不冒称挂起。所有7均到call-done。本机每格都找到两个pthread JIT API符号，但`pthread_jit_write_protect_supported_np()`实际为0；代码因此没有调用`pthread_jit_write_protect_np(0/1)`。符号存在不等于此Rosetta支持切换，toggle路径明确未覆盖。

原证据为`out/osx-sign-rosetta/mre-cdx-01/status.tsv`的23行子作业：5个仅ad-hoc签名构造、18个执行，全transport0/residue0；诊断runner会话11295实际退出0，表示采集完成，不表示所有格都可执行。每格`.stdout/.stderr`保留stage及拒绝，编译日志、codesign展示、clang版本、源码/映像SHA在同目录；总审查见`out/osx-sign-rosetta/mre-review.md`。这些out路径只是原证据索引，本MRE源码运行不读取它们。没有复制硬编码路径runner，也不建立新tools入口。

实验边界：没有对Developer ID材料执行本MRE，没有重现生产hang；真实UNIKERN1、service callback、产品ABI/包阶段、签名信任和责任应用上下文均未等价覆盖。主线后续已另证DeveloperID原路径8秒挂，而同SHA的`cp -p`新inode运行退出0；这是父车道独立结果，不是本18格直接证明。本轮据此不改`kn_load`、映射权限或安全设置；签后原字节重包/新缓存key由主线另做，本MRE不替代其资格证据。
