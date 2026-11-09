# OSX-SIGN-ROSETTA 独立身份与现象取证

结论：本轮重现原路径Developer ID x64版本挂起，但**同完整签名字节的私有副本**在相同干净环境和同P4下成功。不能把签名Team、hardened runtime或kn_load的RW→RX先定为单一根因。更直接的已证差异是执行路径/inode与可能的系统翻译状态；后者仍是待查假设。

## 实读身份

|材料|字节|完整SHA256|实际签名|
|---|---:|---|---|
|run2/osx-x86_64|2462816|3679a611cbc1eb1204e1ff298c5ba2ba2306565792c5a74f8583a3833f2b4ec6|Developer ID，runtime，Team L2N7M5M544|
|run2/adhoc64|2462672|1344b08444c8a432ce358bc6d5ef1de990ab0b5bab408b6f69de0c0920d87648|ad-hoc + runtime，Team未设置|
|run2/dev64|2444002|f44512ec290c9dee944fc7c42ae350cdc371dba04773f866bb4ccddc299069fd|原构造ad-hoc，无runtime，非Developer ID|
|run2/slice-osx-x86_64|2444002|f44512ec290c9dee944fc7c42ae350cdc371dba04773f866bb4ccddc299069fd|同上一行，全字节相同|

名字`dev64`不能当Developer ID身份证明。四者实读均为thin Mach-O x86_64、755、uid501、nlink1，`xattr -l`均空，无quarantine。codesign strict verify均实际退出0。真正Developer ID证书链为Developer ID Application/Developer ID Certification Authority/Apple Root CA，runtime13.0；Developer ID与adhoc-runtime两者仅有`com.apple.security.cs.allow-unsigned-executable-memory=true` entitlement；原构造ad-hoc未输出entitlements。完整codesign/verify/xattrs原日志分别保留，不把ad-hoc结构有效称Apple信任。

`cp -p`仅复制到本目录，未重新签名、改xattr或改原权限。私有`dev-id64`完整SHA等于3679…，strict verify0、755且xattr空；原inode94308528，副本94312735，flags均无。其它副本也逐完整SHA核对。材料及P4结束后重新SHA，与初始`SHA256SUMS.txt`逐字节相同。

## 实际受限对照

全部版本命令使用同一环境：

```
tools/bound 8 /usr/bin/env -i PATH=/usr/bin:/bin HOME=/Users/wjc TMPDIR=/tmp UNISA_CONTAINER=/Users/wjc/repos/unisacc/out/osx-launch-slow/released/pkg.bin /usr/bin/arch -x86_64 ABS_SLICE --version
```

P4完整SHA256 `fffa2311189d97d7dd0868a59a47ff788a6af17259f2664454f9a8351c2ac842`，不是不同外包混测。没有DYLD/UNISA_KERNEL等继承环境。

|执行对象|真实终态|观察|
|---|---|---|
|私有Developer ID同字节副本dev-id64|直接退出0，chunk0d86b9|0.513秒；输出unisacc 0.0.48.1|
|私有adhoc-runtime64|直接退出0，chunk46d6c6|0.391秒；同版本|
|私有original-adhoc64|直接退出0，chunk589645|0.265秒；同版本|
|原run2/osx-x86_64|会话20898，实际退出142|8秒上限，stdout空|
|原路径再次仅为采样启动|会话2840，实际退出142|8秒上限，stdout空|

首次超时PID81811的采样启动太晚，sample退出255，该失败原日志保留，不算栈证据。第二次PID82661在启动后约0.5秒立即采样1秒，sample自身bound3，实际退出0：769次采样全部是`Rosetta Runtime Routines`同地址，无应用符号栈、Binary images不可用、footprint200K。完整原文`original-dev-id-fresh.sample`，不能据这份栈宣称已定位到mmap/mprotect或证明应用从未进入。两个8秒watchdog终态已实际poll；结束后的ps无81811/82661行，没有遗留采样或版本进程。

## 源码事实与推断边界

已读`plans/macos-signing.md`末节；其中历史55秒挂起、ARM真签可运行及公证结果属于既有材料，本轮未重新提交或评估公证。当前`cx/sys/kernel.cx:54–119`确有RW映射、服务地址补丁、mprotect RX再函数指针调用；`src/drive.cx:247`的version分支直接输出版本并返回。但本轮Rosetta采样没有把挂起连接到这条kernel路径，同签名副本又可成功，因此RW→RX只能列待验证假设，不能据源码存在机制就认定因果。

本轮没有运行hello/全量资格、没有Intel实机证据、没有改源/git/安全设置、没有摘quarantine、没有签名或对外写。当前所有进程已终态。最小后续应在授权下继续区分原路径/inode/翻译缓存与签名策略的变量，不能先改宽entitlements来制造绿。
