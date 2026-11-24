# unisacc 0.0.106 发布说明:链后补跑与已知缺口(草案 v2 数字订正,2026-11-25 ccogweco)

依据 R40 第 5 项与 R39。格式沿 release/notes-0.0.105.md 的已知缺口节:每条一句症状 + 定因 + 处置;
未测的写 UNMEASURED,不写成通过;原 FAIL 行保留。

## 链后补跑:工具与夹具缺陷(链在 2e8a19a342 起;第二批到第六笔为路径 B/B-t 链后提交)

共 **六个批、七条原始 FAIL 行**被后补 PASS 取代(SUPERSEDED=7):

- (1) W0087 `check-gen-all-drift`:最小仓夹具缺 tests/ 时,新加 affected-canary 门读不到 tests/batches.tsv,
  affected 以 rc=2 拒绝(`tools/affected.cx:534-535`),夹具把该拒绝当非预期而转红;修复 c698ef0091 在
  check-gen-all-check.cx:61-73 的 rm 清单加 tools/affected.cx,precommit.cx:3762-3768 的夹具仓分支改打
  `AFFECTED_CANARY skipped=1 reason=no-tools/affected.cx (fixture repo)`。
- (2) W0148 `check-ln-noderef`:四处 `ln -s` 未加 `-n`/`-T`,目标已是目录或链接时落错位置;修复 d12d2087bd
  只改 tools/check-chain-ready-freeze.cx 与 tools/check-prepush.cx 的四处写法,不删断言。
- (3) W0150 `release-exec-c`:`sha_file` 用 `char o[200]` 捕获 `sha256sum -- <path>` 整行,长 TMPDIR 车道
  路径超 200 字节时截断缓冲、哈希取空(`tools/release-exec.cx:233-243`);修复在 107 的
  r39c-release-exec-sha.patch(改 `char o[80]` + `exec sha256sum < "$1"`,严格 67 字节核验)。本版只披露。
- (4) W0191 `fwdstub-replay`:批直接 execve 候选 APE,未注册 binfmt 的主机得 ENOEXEC(今晚 stderr 四次
  `OSError: [Errno 8] .../<tree>/unisacc.com`);该批 0.0.105 不存在(0.0.105 strict 命中 0 次);
  修复 a46b98d713 用 sh 启动候选 APE(tests-only)。
- (5) W0189 与 W9998 `devsvc-git-c-operations`:同一批的两条原始 FAIL 行(冷编在并发窗被杀);
  W0189 同时暴露树内冻结行过期,修复 879e446640 只把 build/freeze.tsv 的 deadline 刷新到 1795546800,
  owner 与冻结路径列不变。
- (6) W0206 `env-semantics`:cx/driver/fwdreplay.cx 新增四个环境变量(UNISACC_PHASE、UNISACC_PHASE_OUT、
  UNISACC_PHASE_IN、UNISA_FWD_REPLAY)而活表 tests/env-semantics.tsv 未登记;修复 8ca2f5e4b8 只追加四行
  (八列,UNISA_FWD_REPLAY 的 expires=0.0.110 不动)。

计数口径:批 = 6;原始 FAIL 行 = 7(W0087、W0148、W0150、W0189、W0191、W0206 各一行,加 W9998 同批一行);
SUPERSEDED = 7。除上述外,0.0.106 预跑 148 个以上的 tool-selfcheck 窗均通过。

## 产品字节与身份

- 产品闭包零改动:`git diff --name-only 2e8a19a342 HEAD -- src cx ext exec include seed kernel tests release`
  只含被放行的两个 tests 文件(tests/check-fwdstub-replay.cx、tests/env-semantics.tsv)与 build/freeze.tsv;
  path-b-proof R40 B-t 六文件节的 `PATH_B_DIFF PASS files=6 frozen_paths=2 freeze_deadline=1795546800`;
  重算 SOURCES_SID `c6eeca9f10d66039ce49f5f74ee1ec63727bd85e552f342965d8445136bd812d`,与链上下文逐字相等;
  候选 APE sha256 仍为 `5f83ac93d201b252ea0b56098475be9e30045875cdf1ce692451cda095ceeb3c`。
- 链后修复以 `release-exec --post-chain-merge` 记录,理由用 path-b-proof 的 REASON 原句(六路径版):
  "R40 B-t 六路径:W0087 与 W0148 工具夹具,W0189 树内冻结续期,W0191 APE 启动夹具,W0206 活表登记。
  两个 tests 路径经 G51/G52 放行,六路径均不在 SID,重算与原链一致。证据 path-b-proof.md"。
  INDEX 记 `exec-*-post-chain-merge` 五列行。

## 严格表

- 六个批、七条原 FAIL 行保留,不改写、不删除;后补 PASS 标 SUPERSEDED(按 R40 折叠规则:同批号加同断言文本
  折叠为一行,以复跑结果替换,原行留证)。
- 补跑仍红的窗按 FAIL 记,不转 WAIVED;补跑必须带新 source 戳(新车头 frozen-root),旧窗不复用。
- 严格表会出现"主体在旧 frozen rev、补跑窗在新 rev"的混合来源,由 release-exec --strict 是否接受在落表前核。

## 数字核对清单(待 agy-flow 的 release-exec --dry 实际输出后逐项核)

| 数字 | 本页值 | 核对来源 |
|---|---|---|
| 链后补跑批数 | 6 | release-exec --dry 输出的 post-chain-merge INDEX 行与 strict 表 SUPERSEDED 行 |
| 原始 FAIL 行数 | 7 | 同上,逐批数原行(W0189 与 W9998 同批两行) |
| SUPERSEDED | 7 | strict 表 SUPERSEDED 行计数 |
| 冻结路径内差异文件 | 2 | PATH_B_DIFF 原行;G51/G52 放行记录 |
| 六文件清单 | 6 | PATH_B_DIFF 原行与 path-b-proof 六路径数组 |
| SID / APE | c6eeca9f… / 5f83ac93… | PATH_B_SID 原行与候选表 |
| 148+ 通过窗 | 148+ | 预跑 supervise.log 的 tool-selfcheck 窗统计 |

## 已知缺口

- (1) W0150 的 200 字节缓冲缺陷修复只在 107(r39c);0.0.106 在长 TMPDIR 车道复跑仍可能复现。
- (2) check-split-parents 实测 54947 ms,贴 55000 ms 限(0.0.105 为 38490 ms,差 16457 ms 未定因),
  余量缺口登记 0.1.x,不放宽时限。
- (3) 未跑/未完成的窗记 UNMEASURED;产品级验证(B1 重放、CXM_MAXU 产品段等)按当次候选 strict 表口径另行披露。
