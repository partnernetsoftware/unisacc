# opt-run 红根因：转发型 c99 探针撞 tape（-c）模式（tip 旧红，归 ledger 线）

2026-10-22 00:xx，0.0.43 全量定点闸首轮暴露；与候选无关——自 10433612（2026-10-21 11:35，c99 ledger 探针落主）起任何完整 opt-run 都红，本轮是该提交后的第一次全量。

## 实测链（全部可复现）

- `opt-run-1/2/5/6/7/8` 红源头是 `tests/c99/75_fenv.c`、`76_wctype.c` 在 `UA -c`（tape 文本模式）下 `undefined function 'fesetround'` 等 11+ 件，`-O0/-O2` 皆然、确定性。
- 同探针在**全构建**模式（`-o`，安装版 `unisacc.com` 与同源参考 ua44）编译并运行通过（75_fenv 退出 0，全部检查过）。
- 机制：`src/front_parse.c:7295` 一带，宿主转发只在 `fwdrun` 时做（`fwd_stub`）；`-c` tape 模式不做转发（lower 侧同口径：`libraryfn requires library mode`）。fenv/wctype 族是纯转发名（glibc），78/79/80 等探针用的名字可在 tape 内解析，所以只这两件红。
- 10433612 自己的验收没跑完整 opt-run（分片抽样 + 当时无人全量），红潜伏至今。

## 补丁草案（二选一，归 ledger 线/裁定；本会话不代落）

1. **opt.sh 增加「转发名探针」出口**：仿 `DIFFTEST_KEYS` 第三张名单（如 `tests/opt.tape-refuse`，一行一探针名，注明"forward-only; tape mode refuses by design"），`opt.sh` 在 tape build 失败且名单命中时记 `KNOWN`。不动 difftest 语义（`difftest.knownfail` 列入会反向红：这俩在全构建是过的，而那张表要求列入者必败）。
2. **探针自隔离**：75/76 两件移到 `tests/c99/forwardonly/`（或命名前缀），`opt.sh` 的 glob 不收；c99/difftest 全构建套件照跑。改动更小，但目录语义要在 SEAL/ledger 文档里点名。

两案都不改产品；案 1 机制更直白（显式名单、不能静默腐烂——可加"名单内探针若 tape 可过则红"的反向断言，同 knownfail 纪律）。

## 队列口径

本轮 state（/tmp/c43-rel/cand.queue）里 opt-run-1/2/5/6/7/8 的红按「tip 旧红（10433612），非候选红」分桶；待补丁落主后这些套件输入变更、自动重跑。

**落地（2026-10-22）**：案 1 已落 c3f86999——`tests/opt.tape-refuse` 名单 + `opt.sh` 的 KNOWN 出口与反向复活断言（名单内探针 tape 可过即 FAIL）。单探针分片 338/339 验 KNOWN、临时加通过探针验 FAIL、340 验无回归。
