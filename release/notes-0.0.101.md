# unisacc 0.0.101 发布说明

整体裁决 NOT_QUALIFIED

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交 | 8f0a0af2c4157df7944c044056dc32e187b1a427 |
| 发布 APE SHA256(unisacc.com) | bb715a8bd3ed1237bb69d42d0cd1c2594c0a27c8e223b518ae39efe6b79eac82 |
| 源快照 SID | e9d437e4c8cf8a2f71046a16e370a7f912a35a073f9a46ec873d093136e9a4c3 |
| pkg.bin | a6bd7c13a02c7bbeaf40806cc52f5aae32fa65ed07e578a7cafde13a0442c553 |
| slice-win-arm64 | 1507f5726898937b42083d7336336451af3d5b285ad0d96e0c5a88890cbfcd29 |
| SHA256SUMS | b7de4595f3f23a2844476e8a531fee88d59d4ed546bad4fb91b1a823b7aa241d |

## 严格证据

469 批:PASS 454、FAIL 13、UNKNOWN 2、INVALID 0

## 产品改动

### 逐单元共享状态接线(运行库头与驱动接线)

- (1) 修复 `strtok`、`time`（`_unisa_tmbuf`/`_unisa_tz`/`_unisa_tzn`/`_unisa_tzstate`/`_unisa_asc`）、`rand`/`srand` 种子（`_unisa_seed`）、`popen`/`pclose`（`_u_po_fd`/`_u_po_pid`/`_u_po_n`）、`setenv` 环境堆（`_unisa_envheap`/`_unisa_envcap`）在 `-c -funit` 逐单元编译下各自为政的跨单元语义缺陷,经 `RT_UNIT`/`RT_EXTERN` 接线至统一共享状态,并保证 `RT_UNIT` 定义绕过 `__UNISA_FTRIM_LIBC` 裁剪门控。
- (2) 用到 `strtok`/`time`/`rand` 种子/`popen`/环境堆等跨单元状态的程序,整闭包与逐单元产物字节会变,属有意行为修正:例如跨单元 `strtok` 场景下 `0.0.100` 整闭包产出 `TOK=NULL`, `0.0.101` 整闭包与分单元均为 `TOK=beta`(`readelf` `MemSiz` `0x022590->0x022658`); 新增回归探针 `tests/check-rt-shared-v101.cx` 实跑 6/6 PASS(`C1`..`C6`,含 `-ftrim-libc + -DRT_UNIT` 无调 `setenv`/`popen` 单元与 `-DRT_EXTERN` 调用单元跨单元链接运行)。
- (3) 设计合同措辞更正:整闭包不用新共享符号的程序字节不变(`CMP_PURE_WHOLE=0`);带 `RT_UNIT` 的分单元/`entry-build` 产物每个程序多出全部共享状态定义。对 `netregen-module-*` 与 `netregen-opt-module-*` 六目标共 12 批实测镜像指纹变更均已逐批核验并登记 `release/evidence/0.0.101/expected-fail.tsv`(`expires=0.0.102`),`lnx/x86_64` 节增量为 `LOAD RW MemSiz 0x0227b0 -> 0x022878 (+200B)`、`LOAD R E 0x002112 -> 0x0020f4 (-30B)`(反汇编区间 `0x1fa9..0x1fc6` 为第二单元去重的两条 15B 赋值 `_unisa_seed=1` 与 `_unisa_tok=0`):
  - `W0034` `netregen-module-lnx-x86_64`: `image=a1f5dac55af80b24627b2ae6992067b237e4c9ab927226aa1043c6a6fc1a1c1b->5941cc8e4364bb94f3548a25d054ba061b9ef1b818a3f273c818cc149842361a`
  - `W0035` `netregen-module-lnx-arm64`: `image=c9e02c0043904cf7470b719a1230882b7cee1dbd157a7baa4570a447e92712ab->6d4ceef1613046491cec1f623d27fa574cf291024ba7b974c7e19750b1acc946`
  - `W0036` `netregen-module-osx-x86_64`: `image=dc04aa1b7b5453b7309cee575c974452f8dd8d2226dbe5b06a5fea05f85c9a34->70ddb131cc98ad0dfb3ab7c81e55977feec38225876d2acc8836b9789aee96c5`
  - `W0037` `netregen-module-osx-arm64`: `image=0fc34d295e3ef508936a3566ebffaf1e0b63bb14ac312fe458fa36c06469b721->10a6ac9c4a94f80795e9c15f9ad00cce9d061478caecdd74ac3ce8679a8680be`
  - `W0038` `netregen-module-win-x86_64`: `image=91d648c7b7b0dfabc0ea5743d9ed6a831034bab5de08e4a98717771d3f8cd84c->0493dcff117afd2f60bb26a713a403805fd2ea090ca65ddc48f4c93cb89e1bcd`
  - `W0039` `netregen-module-win-arm64`: `image=4482cc2d89b4b1ad832eb2da6c413b6be141d6d87543c987d2082f9b004e99af->d9438645ff5c587be5db5103919aa3ddd2c2cff1b1484290dba7d09e6ab48d5b`
  - `W0065` `netregen-opt-module-lnx-x86_64`: `image=658826a7d287c9061152d12180fd46d43206e71779bcb4a28c1d9ddedd68ee1d->02a75885aef0cf5bb3c102830e4333cd71599539640b7d54c2db17e9b93699e2`
  - `W0066` `netregen-opt-module-lnx-arm64`: `image=368f383a0849db82941e3cd3eb4d311262a612545e849fea8ade11b1069bdbbb->f90bfdca32de5b62b8b18a2c174e6f358cfc267744b913f5f110b811ebca621f`
  - `W0067` `netregen-opt-module-osx-x86_64`: `image=bac3a5f2340db9b469cee929a368229070dde8e11a8aa471f3e234d471cf5aa6->afe860d499ea9ba2668bdc751cfc577c6c137c5d238de5da8849009ad0c7fc85`
  - `W0068` `netregen-opt-module-osx-arm64`: `image=44c8778db970de32e849406673a1e9653d7ee5e2e38346317050b726e872b702->cd78122a3d56131e4856be8f5743f199bc55bc90ae1f67aec89cd9b0e7e3cb62`
  - `W0069` `netregen-opt-module-win-x86_64`: `image=5f7d660c3960400a5e9df603bf21c36c0a0c9f78545ddf03388e01b85a26b905->5077f74d13469c85bb04c1960bf12203762f39cbc2cca42248eb12e1caeff005`
  - `W0070` `netregen-opt-module-win-arm64`: `image=388c9b716e6e4884de3c4104b1bb0f653c83b183ab5260c655a93fcfc48c9fae->334c4bee3d54ae1c41db0e54ad9772d1a6ee3b286ea962c177fa2ea59df52349`
- (4) 跨目标验证范围:`lnx/x86_64` 原生与 `lnx/arm64` `qemu-aarch64-static` 两单元实跑 `ERRNO=42 TOK=beta CB1 CB2`;其余四目标(`osx/x86_64`、`osx/arm64`、`win/x86_64`、`win/arm64`)编译链接通过、运行标 `UNMEASURED`(另由 GitHub Actions `candidate-native.yml` 覆盖六目标原生冒烟格)。

## 已知缺口

- (1) 整体裁决 `NOT_QUALIFIED`:全套 `s9` `daily`/`release` 长跑未执行,原生 CI 仅覆盖冒烟子集,不宣称全批发布资格。
- (2) 分单元实验开关下同目录头改动可能命中旧对象:已立案 `plans/runsplit-local-header-key.md`,排下一驱动类版修复。
- (3) `承担单元无条件出全部定义`:已立案为 `0.0.102` 之后单项 `plans/rt-unit-header-scoped-def.md`,目标改为只在对应头被包含时出定义。
- (4) 严格预跑非 PASS 项均已逐项核验登记:
  - `FAIL=13`:`precommit-coverage-root`(`W0012` 低负载单窗干净重跑 `count=4/1`, `expires=0.0.102`)与上述 12 批 `netregen-*module*`(`image=OLD->NEW`, `expires=0.0.102`),均登记于 `release/evidence/0.0.101/expected-fail.tsv`。
  - `UNKNOWN=2`:`cx-booter-ape` 与 `tools-default-regression`(`no-BATCH_RUN rc=3`),均为既有无生产者挂账项,已按 `expected-unmeasured.tsv` 与对应 `*-deferred.tsv` 见证登记。
- (5) 冻结提交合入 `origin/main` 工具提交(`1f6774aa36`、`580dc9ae8c`、`5068fd9b36`)不触 SID 闭包与 `.github/`(源快照 SID `e9d437e4...` 与 `99aa998d87` 整链逐字节一致),并在冻结提交上以同候选 APE 追加实跑 `segment 2`(`premerge-selfcheck-main`、`premerge-selfcheck-rd`、`selfcheck-item12-flip`、`slow-split`)全绿补齐 `tests/split-parents.tsv` 覆盖。
- (6) 工具缺口登记:`tools/release-flow.cx:84,232` 仅解析 `count=` 尚未支持 `image=OLD->NEW` 豁免格式,本次经底层正式工具链 `release-run --pack` + `release-exec --warm` + `release-prep` + `release-exec --execute` 完备执行并全过机检。

## 回滚方案

公开后如需撤回,按 `release/rollback.txt` 执行:(1) 将 `v0.0.101` release 转回 draft 并把 Latest 移回 `v0.0.100` 后读回;(2) 在 `main` 按逆序前向 `git revert` Y2/Y1/X 及相关提交并推送,绝不删 tag 或 release;(3) 使用 `tools/install-ape.cx` 从既有 `0.0.100` 备份回装本机 Root 并核对版本。
