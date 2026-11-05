# STAGE-REJECT 网重生比对(2026-11-05)

对象:提交 `5bb4cfd61f`(e4/prune/lower 网入口拒绝)对 11 张读这三张表的网的影响。
方法:仓外克隆 `/tmp/bdy-st`,用 `cc -std=c99 -O2 -w -I seed seed/gen.c` 现编的 seed-gen
在提交前后各生成一次全部网 JSON,逐张 sha256 比对。**不用 tools/affected.cx**(它只看批次输入,
看不到网对事实列的读;ENC-ONEFACT 的教训)。

| 网 | 改前 sha256 | 改后 sha256 | 结论 |
|---|---|---|---|
| shared-e4 | 20c288d801ab7ca08f1a2221d0192b9a91da5afa63e7a9a168ec1b35f2a06504 | 6b7749c412f9f58688e7d6afe4dcb218792797bd6e378bfe72da41b8eb90fd49 | DIFF(预期) |
| shared-o1 | 6e740f294876925ef9736271e8f9adfb3fe4d758bb9efe27ee8b98841d41783a | 5d3b13214e2e0694378438a083ac1ab2afcc040ab9427cf2d1407c18890c5213 | DIFF(预期) |
| shared-prune | 048d415d42047197afddc2d1e1a3e8f80947efd753487e317680a58626134f48 | 2c69794f198c060699e490cd4382b6c1c8dce66bbbc80c54e2925fe9a2daa262 | DIFF(预期) |
| lnx-x86_64-lower | fe909c1f6b27a9e2b0743ff61b92f793010a141f128daf1d60563bbb3df2e31a | 19af205eef53062ce926ae23d58ab6b5c4a22fa7bc1565bedd028ce4b826dcbe | DIFF(预期) |
| lnx-arm64-lower | f31d79b58063caa583e5d294b7d8c68aa0ff85bb6fe8f3266a090723a4ae457b | e81077ccda2f99cc942280800c2d70ab5105381611242bd71f5fa3d0651544ca | DIFF(预期) |
| osx-x86_64-lower | 11bb63a8ab02b5124fe21a5328b808a3fa102e0f1a626d8f7d727cd6ca6b04b0 | f4bf228ab4d94f53501d4d55f9f45acbefc5c82ce1396bc7d5593600638e1818 | DIFF(预期) |
| osx-arm64-lower | f664866e4261a68237bdc9539f1f0199d9e6740043a6ae4e60e924a836de1155 | 1e83cd07b878cd482b35a66f9fd93d9292d4f735ec70d899b212b09e77eb76b2 | DIFF(预期) |
| win-x86_64-lower | d8ae4039747029902d5f6e415eadffe992691474a108561e1a6cbd8ed4949d4a | 34629e2092b678ec6e4aeceb6ebb279fbd8e491a354900b4ebe44e414a4dc05c | DIFF(预期) |
| win-arm64-lower | bd09869999946b5011dea3466e6137c3559d51d91f19ea0f638036919eea6690 | bc74a81ba8b8648c381dd86eeacccfb2f153b4945cc032c4af012b9e651e34b8 | DIFF(预期) |
| object-lower-x86_64 | aa6a7a127138229b72364fc37a348fda709189e65af7cfc406479df046f6dbd3 | 3f7a1bf1f255aea3ce28ccc78bf282f641588d4d49be1c5c718ac0cbaf4f06a0 | DIFF(预期) |
| object-lower-arm64 | e2d08edd2f81d861f6c28d86d718a9392520937e4a564798f9b517a5bcf49cbe | 0e3a93f11479e72efd1ba953373707b971ea36f74b022463f5d71ce981db025c | DIFF(预期) |

11/11 变化,与预期一致:三张表是这三族网的输入。行为等价由网级实测背书:
合法输入(三段各自的 `in`)输出与改前逐字节相同 —— prune 608B、e4(shared-o1)1300B、
lower 1480B;三类坏输入全部 `reject: not covered: malformed tape text`。

这批网要随新一轮 chain 重生进包,已装产品上的 `check-stages` 在三段拒绝项上 FAIL 属预期。


## 用途(给 go 的 chain 对照)

不用另写重生流程:`tools/chain-step.cx` 的 gen/tbl/net 三个相位本来就从 rev 里的表重生
这 11 张网。本文件的上表用作**对照基线** —— chain 跑完后把它的 net-* 作业 `result.net`
的 sha 与上表"改后"一列逐张比,两边一致才说明新网确实进了包。

## v2 基线（HEAD `fc252c5a44` 实生成）

v1（上表"改后"列）只校验末字节；v2（`933300c047`）改成逐字节校验首行 `_start:\n` + 末字节换行，
所以网 sha 又变了一次。go 的 chain 跑完后应比**这一列**（不是上表那一列）。

方法：仓外克隆 `/tmp/bdy-vfy`（HEAD `fc252c5a44`），`cc -std=c99 -O2 -w -I seed seed/gen.c` 现编
seed-gen，逐张生成。

| 网 | v2 sha256（chain 应与此一致） |
|---|---|
| shared-e4 | 65eda7e85c4478b4783b12db30aab1c18d85cb99c49ad378b2dcb7c7c28a658d |
| shared-o1 | 58995bf781609d19ee95ceb31a2a6cf06e8fa4c8ceb5dab66b0b2780fd0d6baa |
| shared-prune | de607e6bc7d5ac1abe24abfda9675af3e987ebdbd2e209153cbeb7fbe41fde76 |
| lnx-x86_64-lower | f990b04413d6ad5ac3d0c8cc565824050c765582895f804cbeaaa68a86878fac |
| lnx-arm64-lower | 8de05f68c20aebb3718587dde60f39feea7a5ebe5c3df1c8e658549e504aca76 |
| osx-x86_64-lower | af628c33b85c9f17165135311a3ba666f3702556d0423e3a023625877dc8b5d2 |
| osx-arm64-lower | ff195903a643715f8993b2cdba68d1812a01d7366fd0cd16b3ccc8413ec2f459 |
| win-x86_64-lower | 8f17ecacdaa4c316aef8f32b31e580518f11d57f7b12349564bcd55c2d22210c |
| win-arm64-lower | cf02656893c33753a5b8973e6810d7944619295ba0eec134803112f61ce4270b |
| object-lower-x86_64 | e37188f90af31b80da686c0eedca9d63f5d0aec41450cbbd7ac99881b73722a5 |
| object-lower-arm64 | 65b593b410cd208f5855435f944b591133615c5c3e3cbc4aa7624a2b6657ec1c |

v2 与 v1 的差异全部来自入口状态链（首行 8 个字节状态 + 尾部校验），行为等价仍由
`plans/` 里记录的网级实测背书：合法输入输出与 v1 逐字节相同，三类坏输入全拒。
