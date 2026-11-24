# unisacc 0.0.105 发布说明

整体裁决 NOT_QUALIFIED

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交 | 414a024ea5e52462b22e7ed9d11f6c44b1d6c2e8 |
| 发布 APE SHA256(unisacc.com) | 934c7c0bc09a9a4d2e107b8d6bdc855553cf05f64dbb7d6c1e5fefdb42117dd1 |
| 源快照 SID | 171326df6a0af39c760618641728def72d101bf12788cccd7814d11b650c2dd2 |
| pkg.bin | a6bd7c13a02c7bbeaf40806cc52f5aae32fa65ed07e578a7cafde13a0442c553 |
| slice-win-arm64 | eaf5dac0ed3ea56e0b5fc35e1762e702b6ec6818a2e16126ef81453a9b47ac3d |
| unisacc.com.build.json / build.json | 0a04e5ca6144916cba30dce80e51d104a6b4ef45b9ce209b70437985dcfc4f74 |
| SHA256SUMS | 5d93b9e32f4c175872429816f9a33ca5de07014b2fc3a797f78f8da87fc4081d |

## 严格证据

474 批(195 窗):PASS 460、FAIL 12、UNKNOWN 2、INVALID 0(strict.tsv sha256=a023ac81677e0dfb4c46f3e1619fc40abb6643fc594a3f8ca8be5a04c001827a)

## 产品改动

### 运行缓存精确钉住(rcident + runcache)

- (1) 重做 `cx/runtime/rcident.cx` 与 `src/runcache.cx` 的 `__FILE__` 精确钉住判定:词法三态区分真实代码、注释与字符串字面量(注释或字符串里的 `__FILE__` 不再过钉);拼接续行(LF 与 CRLF 续行)先做再扫,续行拼出的 `__FILE__` 正确钉住而行注释续行吞掉的不钉;未闭合字面量(如 `#if 0` 里的撇号)保守回退为钉住。
- (2) 补齐跨文件与头内 `##` 片段合并(`CAT(__FI,LE__)` 同文件、头内、源与头跨文件以及 `-DW=__FILE__`)与 `memo` 位图恢复:修复头文件新增 `__FILE__` 后单元缓存永久冷态(双钉键尝试)以及 `memo` 命中回写丢失 `##`/片段位导致同缓存第二目录误复用第一目录对象的缺陷。
- (3) 新增回归测试 `tests/runcache-path-key.cx` 扩充至 28 条断言(对象路径与不变式 6 条 + 负控 3 条 + `-run` 路由精确钉住与边界 19 条),在候选 APE 实跑 `CHECK_RUNCACHE_PATH_KEY ok=28 failed=0`。
- (4) 整链固定点(`0bca20928b`, 499/499 步、`compare` + `compare-recheck` 四同证明通过)与 `gen1` 全工具 190 划冒烟(`GEN1_190_COMBINED total_shards=190 obj_same=1506 obj_diff=0`)全部通过。

## 已知缺口

- (1) 整体裁决 `NOT_QUALIFIED`:全套 `s9` `daily`/`release` 长跑未执行,原生 CI 仅覆盖冒烟子集,不宣称全批发布资格。
- (2) 12 批 `netregen-module-*` 与 `netregen-opt-module-*` 相对历史基线 `0.0.83`(`tools/old-product-assets.tsv` 登记的 `old-product`)产生固定镜像差异,与 `0.0.101` 产出镜像哈希逐字节一致(0.0.105 自身零代码生成变化),均已逐批核验登记于 `release/evidence/0.0.105/expected-fail.tsv`(`expires=0.0.106`):
  - `netregen-module-lnx-x86_64`: `image=a1f5dac55af80b24627b2ae6992067b237e4c9ab927226aa1043c6a6fc1a1c1b->5941cc8e4364bb94f3548a25d054ba061b9ef1b818a3f273c818cc149842361a`
  - `netregen-module-lnx-arm64`: `image=c9e02c0043904cf7470b719a1230882b7cee1dbd157a7baa4570a447e92712ab->6d4ceef1613046491cec1f623d27fa574cf291024ba7b974c7e19750b1acc946`
  - `netregen-module-osx-x86_64`: `image=dc04aa1b7b5453b7309cee575c974452f8dd8d2226dbe5b06a5fea05f85c9a34->70ddb131cc98ad0dfb3ab7c81e55977feec38225876d2acc8836b9789aee96c5`
  - `netregen-module-osx-arm64`: `image=0fc34d295e3ef508936a3566ebffaf1e0b63bb14ac312fe458fa36c06469b721->10a6ac9c4a94f80795e9c15f9ad00cce9d061478caecdd74ac3ce8679a8680be`
  - `netregen-module-win-x86_64`: `image=91d648c7b7b0dfabc0ea5743d9ed6a831034bab5de08e4a98717771d3f8cd84c->0493dcff117afd2f60bb26a713a403805fd2ea090ca65ddc48f4c93cb89e1bcd`
  - `netregen-module-win-arm64`: `image=4482cc2d89b4b1ad832eb2da6c413b6be141d6d87543c987d2082f9b004e99af->d9438645ff5c587be5db5103919aa3ddd2c2cff1b1484290dba7d09e6ab48d5b`
  - `netregen-opt-module-lnx-x86_64`: `image=658826a7d287c9061152d12180fd46d43206e71779bcb4a28c1d9ddedd68ee1d->02a75885aef0cf5bb3c102830e4333cd71599539640b7d54c2db17e9b93699e2`
  - `netregen-opt-module-lnx-arm64`: `image=368f383a0849db82941e3cd3eb4d311262a612545e849fea8ade11b1069bdbbb->f90bfdca32de5b62b8b18a2c174e6f358cfc267744b913f5f110b811ebca621f`
  - `netregen-opt-module-osx-x86_64`: `image=bac3a5f2340db9b469cee929a368229070dde8e11a8aa471f3e234d471cf5aa6->afe860d499ea9ba2668bdc751cfc577c6c137c5d238de5da8849009ad0c7fc85`
  - `netregen-opt-module-osx-arm64`: `image=44c8778db970de32e849406673a1e9653d7ee5e2e38346317050b726e872b702->cd78122a3d56131e4856be8f5743f199bc55bc90ae1f67aec89cd9b0e7e3cb62`
  - `netregen-opt-module-win-x86_64`: `image=5f7d660c3960400a5e9df603bf21c36c0a0c9f78545ddf03388e01b85a26b905->5077f74d13469c85bb04c1960bf12203762f39cbc2cca42248eb12e1caeff005`
  - `netregen-opt-module-win-arm64`: `image=388c9b716e6e4884de3c4104b1bb0f653c83b183ab5260c655a93fcfc48c9fae->334c4bee3d54ae1c41db0e54ad9772d1a6ee3b286ea962c177fa2ea59df52349`
- (3) `UNKNOWN=2`(`cx-booter-ape` 与 `tools-default-regression`, `no-BATCH_RUN rc=3`)为既有无生产者挂账项,已登记于 `release/evidence/0.0.105/expected-unmeasured.tsv` 与对应 `*-deferred.tsv`。
- (4) 冻结提交 `414a024ea5` 在零产品闭包差异(`0bca20928b..414a024ea5` 在 `src cx include exec kernel seed weights` 零改动,源快照 SID 逐字节保持 `171326df6a...`)下合入最新工具链与测试提交(含 `tests/env-semantics.tsv` 迁移与 `check-release-flow` 动态行数),并在冻结提交上以同候选 APE 追加实跑 Segment 2(`W0901..W0906`, 18 批全部 PASS)合并入严格证据。
- (5) 冻结期漏钉场景后续改进(共享 `text.pinscan` 判据)已立项排入 `0.0.124`(`plans/v0.0.124.md`)。

## 回滚方案

公开后如需撤回,按 `release/rollback.txt` 执行:(1) 将 `v0.0.105` release 转回 draft 并把 Latest 移回 `v0.0.101` 后读回;(2) 在 `main` 按逆序前向 `git revert` Y2/Y1/X 及相关提交并推送,绝不删 tag 或 release;(3) 使用 `tools/install-ape.cx` 从既有 `0.0.101` 备份回装本机 Root 并核对版本。
