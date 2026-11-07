# CI-RUN-3 结果对照 rc2 预期表

数据:`scratchpad/ci3/evidence-*/<os>-<arch>.tsv`(六格 × 7 项),候选 APE `346e3cc0`,commit `f7a534c7`。
预期表:`plans/rc2-ci-expectations-2026-11-06.md`。**本文件只对照,不预判 rc3。**

## 1. 逐格对照

| 格 | 项 | 预期 | CI-RUN-3 实测 | 判定 |
|---|---|---|---|---|
| lnx/x86_64 | 七项 | 全 PASS | 全 PASS | **预期成立** |
| lnx/arm64 | run | 应 PASS(rc2 主目标) | PASS | **预期成立** |
| lnx/arm64 | version/hello/stdio/smoke/identity | 应 PASS | 全 PASS | 预期成立 |
| lnx/arm64 | stream-pipe | 应 PASS | **FAIL**(`B plain-image-pipe-cmp`) | **被证伪** |
| osx/x86_64 | run | 应 PASS(次要目标) | PASS | **预期成立** |
| osx/x86_64 | hello/smoke/identity | 应 PASS | 全 PASS | 预期成立 |
| osx/x86_64 | version | 应 PASS | **FAIL**(stdout 15B 全对,stderr 26B `tail: stdout: Broken pipe`) | **被证伪** |
| osx/x86_64 | stdio-crossunit | 应 PASS | NOT_RUN(rc3,无 /dev/full) | 预期未写;见 §3 |
| osx/x86_64 | stream-pipe | 应 PASS | **FAIL**(同 lnx/arm64) | **被证伪** |
| osx/arm64 | 同 osx/x86_64 | 同上 | 同上 | 同上 |
| win/x86_64 | version/hello/smoke | 应仍 FAIL | 仍 FAIL(version rc1;hello/run/smoke 0xC0000005) | **预期成立** |
| win/x86_64 | stdio/stream-pipe | 应仍 NOT_RUN | 仍 NOT_RUN | 预期成立 |
| win/arm64 | 七项 | 应仍 NOT_RUN | 全 NOT_RUN("APE has no win/arm64 slice") | 预期成立 |

**统计**:预期成立 5 格(lnx/x86_64、lnx/arm64 除 stream-pipe、osx 两格的 run/hello/smoke/identity、win 两格全部);
被证伪 3 项(lnx/arm64 stream-pipe、osx 两格 version、osx 两格 stream-pipe)。

## 2. 被证伪的三项:真因(不是预期表写错,是产品缺陷)

### 2.1 osx 两格 version —— LOADER-STDERR

预期表写的是「`--version` 应 PASS」。实测:stdout **完全正确**(15 字节,与 `want=15` 相等),
但 stderr 有 26 字节 `tail: stdout: Broken pipe`。判定条件要求 `stderr_len == 0`,故 FAIL。

根因(cc 已定位):APE 的 shell 头 `tools/ape-pack.cx:124,139`
`tail -c +$o "$0" | head -c $n | gzip -dc` —— `head` 取够字节即退出,`tail` 收到 SIGPIPE;
BSD tail 在 macOS 上把这句话写进 stderr(GNU tail 在 arm64 runner 也写过类似的话)。
所以**每次首次解包都会向 stderr 吐一句误导信息**,不是偶发。
→ 条目 LOADER-STDERR(P1,进 rc3)。修法与验收见 cc 的信;本文件不重复。

### 2.2 lnx/arm64 与 osx 两格的 stream-pipe —— STREAM-HOST-TARGET

预期表写的是「应 PASS」。实测三格都是 `FAIL B plain-image-pipe-cmp`:
`tests/check-stream-pipe.cx:83-84,105,119,123,132` 把 route 写死成 `lnx/x86_64/...`,
于是在 arm64 主机上:产品 `-o` 用本片目标编出 arm64 镜像,而管道用写死的 lnx/x86_64 route 编出 x86_64 镜像,
两者逐字节不等 → B 格红。

这不是测试写错,是**产品侧** `src/drivestream.cx:34` `static const char *ds_tgt = "lnx/x86_64";`
把 `unisacc stream` 的默认目标写死了。→ 条目 STREAM-HOST-TARGET。

注意一个细节:测试自己写死 route,所以即使产品修好 `ds_tgt`,`check-stream-pipe` 仍会红 ——
**产品修 + 测试改成取本片目标,两件事都要做**。

### 2.3 win/x86_64 —— 判据已明确(预期成立但原因更新)

预期表写「仍 FAIL」,成立。CI-RUN-3 把原因收窄到 argv:
Linux 交叉编的 `argv.exe` 带 `ZZQQ` 在 Windows 上输出 `(null)` → `argv[1]` 是 NULL,
win PE 启动代码没正确构造 argv(`winargs` 路径)。unisacc 读到垃圾 argv 于是走「未知选项」分支,
打印字符串池里的 `-version`/`--help` 字面量;`direct.exe ZZQQ` 直接 0xC0000005。
原假设 H1(WriteFile 第 5 参)已被 `hi.exe` 管道输出正常证伪。

## 3. 预期表未覆盖、需要补的一条

**osx 两格的 stdio-crossunit 是 NOT_RUN(rc3),不是 FAIL**:
`tests/stdio-crossunit.cx:95-99` 检测到本机没有 `/dev/full`(macOS 确实没有)就把两行记 NOT_RUN 并 rc3。
预期表把它写成「应 PASS」,但按批次自己的合同,osx 上应当是 NOT_RUN —— 这是**预期表写得不准**,不是产品问题。
判读时:osx 的 stdio-crossunit 记 NOT_RUN 属正常;若出现 FAIL 才是缺陷。

## 4. CI-RUN-2 → CI-RUN-3 的变化(同一 APE 346e3cc0,不同 runner 批次)

| 格·项 | CI-RUN-2 | CI-RUN-3 | 说明 |
|---|---|---|---|
| osx 两格 stdio-crossunit | FAIL `devfull-errno-visible-to-caller` | NOT_RUN rc3 | ci2 那轮 `/dev/full` 被 stat 到了但内容不对;ci3 记 NOT_RUN。两者都不算 PASS |
| 其余 | 与 ci3 相同 | 相同 | lnx/arm64 与 osx 两格 stream-pipe 两轮都红 |

## 5. 新发现的产品缺陷(单列)

### STREAM-HOST-TARGET

- 位置:`src/drivestream.cx:34` `static const char *ds_tgt = "lnx/x86_64";`
- 影响:`unisacc stream STAGE` 在非 lnx/x86_64 主机上默认仍按 lnx/x86_64 出 route,
  于是管道产物与本片 `-o` 产物不是同一个目标 → `check-stream-pipe` 在 lnx/arm64、osx/x86_64、osx/arm64 三格红。
- 与 NATIVE-RUN-HOST-TARGET 的关系:后者修的是 `-run`(`src/drive.cx:505` 已改 `tn_self()`);
  **`stream` 这条路径当时漏了**,是同一类缺陷的第二个实例。
- 修法方向:默认取 `tn_self()`(`cx/sys/hostid.cx`),`--target` 仍可覆盖;测试同步改成不写死 route。

## 6. 全仓 `"lnx/x86_64"` 字面量扫描(产品代码,排除表/夹具/注释)

扫描范围:`src/`、`cx/`、`include/` 下的 `.cx`/`.c`/`.h`。判定标准:是**默认值或硬编码分支**才算残留写死目标;
纯粹用于「识别这个字符串等于哪个目标」的查表/比较不算(那是必要的名字映射)。

### 6.1 真·写死默认目标(应修)

| file:line | 代码 | 说明 |
|---|---|---|
| `src/drivestream.cx:34` | `static const char *ds_tgt = "lnx/x86_64";` | **STREAM-HOST-TARGET**,本轮新发现;`unisacc stream` 在非 lnx/x86_64 主机上默认仍按 lnx/x86_64 出 route |

### 6.1b 经复核属有意设计(不改,已立条目跟踪)

| file:line | 代码 | 结论 |
|---|---|---|
| `cx/driver/route.cx:391` | `if (!tgt) tgt = mode2 ? "lnx/x86_64" : in->native;` | **有意设计,不是遗漏**。README.md:165-166 写明不带 `-b` 的 `-c`/`-S` 写的是「与目标无关的中间 tape」;实测四种 `-b` 下 tape 的 sha 相同,所以这个默认标签不参与产物。与 stream-pipe 变红无关。是否改为取本机由 **DEFAULT-TARGET-S-C**(0.0.45)先写清设计再动 |

### 6.2 名字映射/校验(合理,不算缺陷)

| file:line | 用途 |
|---|---|
| `src/entrybind.cx:132` | `bx_as_arch` 把目标名映射成 arch 号 |
| `src/entrybind.cx:234` | `hb_target_id` 同上的 host 绑定版 |
| `src/runcache.cx:647` | 缓存只在 lnx/x86_64 生效(显式限制,有注释) |
| `cx/compiler/asmtext.cx:2191` | 从 `# unisacc -S lnx/x86_64` 注释行反推 arch |
| `cx/compiler/cellname.cx:11` | 生成的 cell 名表第 0 项(表驱动产物) |
| `cx/runtime/cellname.cx:11` | 同上 |
| `cx/compiler/hostbind.cx:47` | `hb_target_id` |
| `cx/compiler/tapelink.cx:316` | 从 ELF machine 反推目标名 |
| `cx/cx0/bundle.cx:21` | 六个合法目标名的白名单 |
| `cx/driver/path.cx:72` | `cxp_tapebin_target` 六目标名→号 |
| `cx/sys/hostid.cx:6` | **生成物**:`#define SELF_TARGET "lnx/x86_64"`(按宿主宏选中的那一行) |
| `cx/workspace/driver.cx:95` | workspace 工具只接受 lnx/x86_64 参数(显式限制) |

### 6.3 帮助文本/注释(不改)

`src/entrybind.cx:159,162`(usage 文本)、`cx/compiler/asmtext.cx:1636,2180`(生成的注释行)、
`cx/driver/route.cx:75,103,109,111`(文档注释)、`cx/runtime/runexec.cx:937`(注释)、
`cx/driver/path.cx:20`(注释里的编号表)。

### 6.4 表与夹具(排除)

`exec/facts/lower-code.tsv`、`exec/facts/pp-gen.tsv` 共 3 处,属事实表内容,不是代码。

**结论**:产品代码里真正的写死默认目标只有 **1 处**(`src/drivestream.cx:34`,STREAM-HOST-TARGET);
`cx/driver/route.cx:391` 经复核是有意设计(见 §6.1b),是否改动由 **DEFAULT-TARGET-S-C**(0.0.45)先写清设计再定;
其余 12 处是名字映射/显式限制/生成物,不应改动。
