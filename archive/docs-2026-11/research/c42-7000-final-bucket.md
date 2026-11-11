# c42 定点闸 /tmp/c42-7000 FINAL 分桶（2026-10-13 22:06 SGT 结证，只读分桶）

性质：分桶记账，不是验收通过。`queue.sh` final rc=1。不改验收措辞，不放宽限时。

## 1. 运行身份

| 项 | 值 |
|---|---|
| GATE | `/tmp/c42-7000/cand.queue`（fresh start，未 resume，env-pin 未动） |
| 候选 | `unisacc.com` sha256 `ba11d636b54adcdc…`，与 `release/candidate.json`（0.0.42，GHCR `…@sha256:9da796385d67…`）是同一份字节 |
| 源 | 封存自 `1fe598af`；闸的工作树 `f871dbaf`（只多封存记录）；产品闭包一致 |
| comboot | stage1–3、fixedpoint rc=0（stage2 首次因同机孤儿队列挤占超时，清锁后重跑通过） |
| 结证 | 722/722；通过 632；rc1 30；rc2 7；rc77 14；rc142 39 |
| 出口表 | PASS 632；BLOCKED 28；HOST_TIMEOUT 11；NEEDS_RULING 9；RESOURCE_UNKNOWN 7；RULED_BASELINE 21；UNVERIFIED_HOST 14（`exittable.py --h1 archive/plans/v0.0.38.md`） |

墙钟说明：本闸 17:13 启动、22:06 结束，其中约 100 分钟是宿主机在会话空闲时被整机挂起（`uptime` 不走），不是队列慢。

## 2. 产品真红：无

- `lib-lifecycle` rc=0（`resident_range_bytes` 0，guest 映射检查通过）。
- `com-multi` rc=0（`multi-unit ok`）、`com-forward-multi` rc=0（`ok 3 wrong 0`）：Linux 自建镜像保持静态，宿主转发走按需 exec（memfd）。
- 没有任何一项的失败签名是输出不一致、编译错或 sanitizer 报告。

## 3. 本轮修复在正式闸中的确认

`exec-native-stages` rc=0（read-dir）；`seedmemory` rc=0；`freezecheck` rc=0；`ledgercheck` rc=0；`realprog` rc=0（6 ok，0 skip，0 wrong）；`queuestart-launch`、`queuetimeout`、`queuetimer`、`observation` rc=0；`seed-construct-parse2-20..22` 不再超时。

## 4. rc1（30）

- 前驱连锁 28（BLOCKED，未执行）：`rowcov-parse2-build` 超时带出 `rowcov-parse2-1..24` 与 `rowcov-parse2-union`；`rowcov-lower-3` 超时带出 `rowcov-lower-union`；其余见出口表。
- 有既有裁定行的 2 项：`exec-driver-language-2`（宿主 gcc 14 拒绝参考探针 `enum_forward.c`）、`exec-bridge-linux`（宿主 clang 不接受 arm64 `.S`）。

## 5. NEEDS_RULING（9，全部 rc142，无输出错误）

| 套件 | 备注 |
|---|---|
| `com-declmatrix-1..4` | 本轮新出现的超时；此前各轮通过 |
| `seedgen-optconsumer-2`、`-3`、`seedgen-pruneconsumer-1` | 本轮新出现的超时 |
| `seedgen-ppconsumer-1` | 上一轮也超时 |
| `rowcov-lower-3` | 旁路单跑通过，见第 7 节 |

## 6. 其它类别

- HOST_TIMEOUT 11（H1 预授权）：`closure-c1..4`、`stages-1/2`、`exec-e5`、`exec-f1-attributes`、`lib-callable-variadic-model`、`shared-e2-plain`、`tools-2`。
- RULED_BASELINE 21：`rowcov-enc-1..8`、`rowcov-parse2-build`、`rowcov-pp*`、`com-tapebin-1..3`、`com-luatests-build`、`com-fb12-multi`、`exec-arm`、`shared-e2-located` 的超时，加第 4 节的 2 项。`rowcov` 的结构问题留 0.0.43（瓶颈在 `exec/finite_rules.py` 的行来源追踪，单步约 52 秒）。
- RESOURCE_UNKNOWN 7：`seedgen`、`seedgen-2..4`、`seedparse2-1/-2`、`com-seedgen`。冷缓存分支与未测 flag 组合按设计为 UNKNOWN，`seedmemorycheck.py` 断言如此。
- UNVERIFIED_HOST 14：
  - 6 项已有远端证据（`darwin-gate` run 38387935866，逐项 rc=0，见 `research/c42-remote-evidence.md`）：`exec-bootstrap-osxarm`、`exec-bootstrap-osxx86`、`lib-stack-arm`、`lib-stack-x86`、`lib-stack-x86-hostabi`、`lib-windows-gp-native`。
  - 8 项义务仍开（缺 Lima 客机、zig 或 macOS 链接器/宿主）：`ccinterop`、`syscall6`、`hosthdr`、`elfobj`、`com-elfobj`、`forward`、`com-forward`、`windows-resolver-host`。能跑的格子都跑了，0 个 wrong。

## 7. 旁路单跑（不写回闸的结果，仅作归因证据）

| 套件 | 位置 | 结果 | 读法 |
|---|---|---|---|
| `tools-2` | 主检出，`gate.sh --com --suite` | rc=0，4 pass，45 s | 闸内超时是负载 |
| `rowcov-lower-3` | 闸的工作树（有预构建缓存） | rc=0，34 s | 闸内超时是负载 |
| `rowcov-lower-union` | 同上 | rc=0，“above the baseline 450” | 覆盖未缩水 |

（`rowcov-lower-3` 在主检出里单跑会超时，因为那里没有 `rowcov-lower-build` 的缓存，不能据此判断。）

## 8. 语料棘轮（RELEASE-PIPELINE §10 第 7 项）

- corpus：四个分片各 pass 54，合计 216，等于基线 216。
- tools：`tools-1` 4、`tools-3` 3 在闸内通过；`tools-2` 4 来自旁路单跑。合计 11，等于基线 11。正式结证里 `tools-2` 仍是 rc142。
- realprog：闸内 6 ok（基线 5 个名字全部 ok，新增 `sqlite`）。`tests/realprog.baseline.list` 本次追加 `sqlite`。0.0.41 的 realprog ok 数在仓库里没有记录，下界是基线的 5 个名字。

## 9. 到 Draft 还缺什么

- 主人对第 5 节 9 项和 RESOURCE_UNKNOWN 7 项的裁定；对 8 项仍开的 UNVERIFIED 义务的处置。
- 按“本地 rc=0”的字面结项条件，本宿主做不到（rc77 与 H1 超时）；本版的可辩护依据是出口表全归类且无产品红，加远端六格与 darwin-gate 证据。是否以此开 Draft 由主人定。
- rc 标签、unsigned zip 与回执、Draft 本身：均未做。
