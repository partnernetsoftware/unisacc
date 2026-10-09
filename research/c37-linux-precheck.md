# 0.0.37 云机 Linux 预检（cc，2026-10-09；非发版证据）

宿主：Debian 13 x86_64（云机），独立克隆 ~/repos/unisacc-cc。参考 UA = `tests/build_ref.sh` 自 main 666bb0bc 同源；产品 = 公开 v0.0.36 unisacc.com（sha256 baf296dd…）。0.0.37 候选 .com 本机构建不了：`exec/c/buildcompiler.sh:9` 要求 Darwin（kernel seed assembler），候选构建/自举定点/封版/全门禁按切口 A 留 m4pro。

## C1（tests/c99/64–66；产品 `-run`，参考编译后运行）

| 探针 | 产品 0.0.36 | 参考 |
|---|---|---|
| 64_register | 0 | 0 |
| 65_predefined_stdc | 0 | 0 |
| 66_struct_return_call | 1：`UNCOVERED e3 … expr.call.ret-struct` | 0 |

C1b：`tests/diag.sh`（参考，corpus/c-testsuite 已拉）ok 31 wrong 0，含 register 取址段；产品侧须在正式候选上复跑。

## C3 种子工具（`-fno-trim-libc seed/X.c -o …`）

| 工具 | 产品 0.0.36 | 参考 |
|---|---|---|
| tbl/net/gen/blob/ident | 0 | 0 |
| ape | 1：`seed/ape.c:201:12 struct return expression outside local lvalue`（C1a） | 0 |
| compilerpack | 0（Linux 上通过；macOS 上旧 .com 失败于 realpath，见 plans） | 0 |
| pack | 1：`seed/pack.c:19` zlib 头缺（按 C2 记） | 1（同） |

realpath 探针：参考 `/usr/../etc//hostname`→`/etc/hostname`，不存在路径 NULL/errno 2；产品 0.0.36 `undefined function 'realpath'`（0.0.37 头补丁之前的发布物，符合预期）。

## C2 g4

见 research/n1-gaps/README.md 末段（c4e5001f）。修片在 cdx 的 callcontrol.py 生成器侧，已 envelope 交 cdx；本机只做验收。当前 main 已含 cdx 5a632535 g4 表修，上表产品列是公开 0.0.36 旧行为。验收命令（候选 .com 到手后）：`sh $COM -run research/n1-gaps/g4_a.c research/n1-gaps/g4_b.c`、单文件变体（`int f8();` 声明 + 调用 + 其后带原型定义 `int f8(int a,…,int h){…}`；K&R 定义参考本身不收，不作反例）、原型对照三者 rc=0，且与参考一致。

## F4′

钉住 v0.0.32 csih 15 单元在 Linux 上不可作同口径计时：产品经宿主转发调 libcurl，静态产品报 `host libc forwarding needs a dynamic compiler image`；`-t osx/arm64` / `-t lnx/x86_64` 交叉编到符号解析失败（`curl_slist_free_all` 等）用时约 4.95 s / 5.15–5.20 s（冷缓存，未产出二进制，仅作量级参考）。≤5 s 验收须在 m4pro 同机冷编。

## 云机 Linux 构造的预候选（cc 独立验收，2026-10-09 15:0x；非正式候选）

对象：cdx 在本机用 machocc（clang/ld64.lld/llvm-nm）构造的 `/tmp/cdx-linux-candidate/unisacc-next.com`，sha256 `8d147e0fc5eb7ce241fc6e9ac93f5a4db1d4fd4a5ff972a22c1ca4e49dd3d067`，2,243,464 B；`--version` 仍报 0.0.36（版本号未提交），构建改动当时未提交、无 seed 对。kernel 与旧 Apple blob 字节不同（cdx 报）。以下只作预检，正式候选（版本号提交 + 构建改动入库 + seed + 自举定点）上须重跑。

| 项 | 命令 | 结果 |
|---|---|---|
| g4 | `-run g4_a.c g4_b.c`（双序）、`-run g4_one.c`、`-run g4_prototype.c`、`-run g4_float.c` | 全 rc=0 |
| C1 | `-run tests/c99/64–66` | 全 rc=0（66 在 0.0.36 产品上是 UNCOVERED） |
| C1b | `UA=<包装 sh 候选> tests/diag.sh` | ok 31 wrong 0（含 register 取址段与 damaged corpus） |
| C3 | `-fno-trim-libc seed/{tbl,net,gen,blob,ident,ape,compilerpack}.c` | 7/7 rc=0（ape 即 C1a 形状） |
| C3 | `seed/pack.c` | rc=1 `pack.c:19` zlib 头（按 C2 记） |
| C3 realpath | `-run rp.c` | `/etc/hostname`；不存在路径 NULL errno 2 |
