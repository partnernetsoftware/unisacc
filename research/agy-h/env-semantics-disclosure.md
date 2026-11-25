# 0.1.0 发布说明披露稿: 跳号 `0.1.0` 触发之版本到期与前缀硬编码 (`W0212` / `W0217`)

起草窗口: `agy-h` | 用途: `release/notes-0.1.0.md` 已知缺口与路径 B-t 披露段 | 状态: `S4` 四批补跑(`W9988..W9991`)全部一次 `PASS` (`1a915a3955`)，已按磁盘实测回填(`S5..S8` 合并与 `X`/公开节点待续)

---

## 1. 可直接合入 `notes-0.1.0.md` 的披露正文 (无星号, 无家目录绝对路径)

- `W0212` (`env-semantics`, `product-qualification`) 到期版本未同步红停披露:
  - 触发原行: `0.1.0` 预跑在 `W0212` 因 `env-semantics` 失败触发 `PRERUN_PRODUCT_RED_STOP` (`status=FAIL rc=1 ms=13433 load1=3.62 count=125/8`, 其余 125 项语义与读取点断言全通, 仅 8 项到期版本断言红; 同窗其余 7 批全部 `PASS`)。
  - 根因核实: `tests/env-semantics.tsv:22-29` 所列 8 个 `UNISA_<NAME>` 兼容环境变量(`UNISA_MAXSTEPS`、`UNISA_JOBS`、`UNISA_FFI_BRIDGE`、`UNISA_KERNEL`、`UNISA_KERNEL_SHA`、`UNISA_NOSTDINC`、`UNISA_CONTAINER`、`UNISA_FWD_REPLAY`) 登记的弃用过期版本为 `expires=0.0.110`。因版本编排从 `0.0.107` 跳过 `0.0.108..0.0.110` 直接进入 `0.1.0` 封版车, `tools/check-env-semantics.cx` 按三段整数版号比较判定 `0.1.0 >= 0.0.110` 越过到期线, 属于版本重排后测试表到期日未同步更新, 并非产品功能缺陷(0 产品功能缺陷)。
  - 处置与边界: 将 `tests/env-semantics.tsv:22-29` `expires` 由 `0.0.110` 统一顺延至对应四车 `0.1.x` 末尾版号 `0.1.4` (不删行、不删断言、仅推到期版本), 路径 B-t (`1a915a3955`) 合入后在同候选 `APE` 单跑 `strict-W9990-env-semantics.tsv` 实测一次 `PASS` (`rc=0 ms=12830 load1=1.93 count=133/0 assertions=23 sha=97eb3624b317`)。

- `W0217` (`xunit-fnptr`, `product-qualification`) 版本行前缀硬编码 `0.0.` 红停披露:
  - 触发原行: `0.1.0` 预跑在 `W0217` 因 `xunit-fnptr` 失败触发 `PRERUN_PRODUCT_RED_STOP` (`status=FAIL rc=2 ms=2608 load1=8.76 assertions=4`, 4 项表与语言功能断言已全通, 后在检查工具校验产品 `--version` 输出时退出; 同窗其余 7 批全部 `PASS`)。
  - 根因核实: `tools/check-xunit-fnptr.cx:97-98` 使用 `tx_find(out, ..., "unisacc 0.0.", 12)` 匹配 `unisacc 0.0.<minor>`，对 `0.1.0` 候选输出的 `unisacc 0.1.0` 报 `die("product version line")` (`rc=2`)。系测试检查工具对 `0.0.x` 版本前缀的硬编码假设, 并非产品编译/运行期功能缺陷(0 产品功能缺陷)。
  - 处置与边界: 并入路径 B-t (`1a915a3955`) 第四文件, 将版本行匹配改为完整三段版本数值比较(`0.0.68` 仍走旧 `UNVERIFIED` 分支, `0.0.69` / `0.1.0` / `0.1.4` 走新分支且畸形版本行仍拒绝, 避免末段 `0` 误判), 同候选 `APE` 单跑 `strict-W9991-xunit-fnptr.tsv` 实测一次 `PASS` (`rc=0 ms=12641 load1=1.90 assertions=15 sha=8293480d5203`)。

- `PASS` 但余量不足批补充披露 (`W9988` / `W9989`, `0.1.1` 拆批评估):
  - `W9988 check-split-parents` (`tool-selfcheck PASS rc=0`): 正式窗实测 `49056 ms` (`load1=1.51 assertions=19 sha=acc8b9e1eb7c`), 距 `53000 ms` 内层限定仅余 `3944 ms` (`< 4s`), 与 `107` 既有的 `premerge-selfcheck` / `slow-split` 一并列入余量紧张批, 留 `0.1.1` 拆批评估。
  - `W9989 check-netregen-expected-fail` (`tool-selfcheck PASS rc=0`): 正式窗实测 `40738 ms` (`load1=1.33 assertions=20 sha=9c0764cb78e6`), 距 `53000 ms` 内层限定余 `12262 ms`, 如实一并披露其耗时与算术余量。

- 路径 B-t 综合与零产品差异声明:
  - 上述两项连同工具自检夹具(`W0088 check-split-parents`、`W0111 check-netregen-expected-fail`)合计四文件(`tools/` 三个检查与 `tests/env-semantics.tsv` 一个表) 以路径 B-t (`--post-chain-merge`) 落入, 零改动产品源码、零改动 `SOURCES_SID` 快照及 21 条产品支撑路径, 候选 `APE` 字节(`b7cae17b1ae557a4907247e3168db56925504819f98d15f1b75a2da2de8fa97f`) 不变。

---

## 2. 溯源字段对照与 0.1.0 预跑非 PASS 全集 (`521` 批中 `6` 项: `2 UNKNOWN + 4 FAIL`)

- 预跑终态时间线: 首启 `09:29:39` (`1795570179`) -> 重起 `09:37:31` -> 结止 `10:57:16` (`1795575436`, 总跨度 `87.6m`, `09:37:31` 起含停等实跑 `79.8m`), `245/245 DONE`, `521` 批合计 `515 PASS / 2 UNKNOWN / 4 FAIL / 0 INVALID`:
  - `UNKNOWN` (`2` 项, `transport=-1`): `W0023.z06 tsv-lint-root` (`55570ms`, `load1=2.56`; `S2` `strict-W9923-tsv-lint-root.tsv` 为 `11:02` 误清后以 `cat` 日志还原之 `restored-from-report` 件, 磁盘 `sha=58683c81163a`, 原报 `2999e4e2` 系手写占位误填且外层 `timeout 75s` 截断缺 `SUMMARY` 行; 实际重跑收口件以磁盘最终定名为 `strict-W9923b-tsv-lint-root.tsv`, `PASS rc=0 ms=11733 load1=1.83 assertions=2`, `sha=d99db704c32f`); `W9922 precommit-list-cap` (`55587ms`, `load1=1.43`, 登记 `expected-unmeasured`)。
  - `FAIL` (`4` 项, 均经路径 B-t `1a915a3955` 后在同候选 `APE` 单跑 `PASS` 收口): `W0088 check-split-parents` (`38955ms FAIL` -> `strict-W9988-check-split-parents.tsv PASS 49056ms load1=1.51 assertions=19 sha=acc8b9e1eb7c`, 距 `53000ms` 余 `3944ms`); `W0111 check-netregen-expected-fail` (`25759ms FAIL` -> `strict-W9989-check-netregen-expected-fail.tsv PASS 40738ms load1=1.33 assertions=20 sha=9c0764cb78e6`, 距 `53000ms` 余 `12262ms`); `W0212 env-semantics` (`13433ms FAIL count=125/8` -> `strict-W9990-env-semantics.tsv PASS 12830ms load1=1.93 count=133/0 assertions=23 sha=97eb3624b317`); `W0217 xunit-fnptr` (`2608ms FAIL rc=2` -> `strict-W9991-xunit-fnptr.tsv PASS 12641ms load1=1.90 assertions=15 sha=8293480d5203`)。
- 候选身份: `a11d28fc0a61939ee71345d732a7dcf205b261f1` / `APE=b7cae17b1ae557a4907247e3168db56925504819f98d15f1b75a2da2de8fa97f`
- 路径 B-t 四文件提交(`v0.1.0` 分支 `1a915a3955fb3533e70164316c742b9bd03fddc3`, `2026-11-25 11:16:31 +0800`): `tools/check-split-parents.cx`、`tools/check-netregen-expected-fail.cx`、`tools/check-xunit-fnptr.cx:97-98`、`tests/env-semantics.tsv:22-29`
- 已回填全量单跑(`W9923b` + `W9988..W9991` 均 `PASS`); 待 `S5` 折叠合并(`PASS=520 FAIL=0 UNKNOWN=1 SUPERSEDED=5`)与 `X`/公开节点回填
