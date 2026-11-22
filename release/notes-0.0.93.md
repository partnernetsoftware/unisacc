# unisacc 0.0.93 发布说明

整体裁决 NOT_QUALIFIED

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交 | 78346c972b23c7c6ae0990e3e4e12ae889428409 |
| 候选提交 | a206d09fa16994e25166bcc893d96e1927df8309 |
| 合并提交 (v0.0.93 -> main) | f182b600689a42f4a938e645365bf106b367e925 |
| 发布 APE SHA256(unisacc.com) | c561e7e0f24d9b82e71b40c86920bc92c12b7035751c8d6e958930bbfe1dd843 |
| 源快照 SID | 40af64c7f09d9b169a7c38fe6ce76fe8fcdb6cc0a4d8f81a178fe65aacef77f1 |
| pkg.bin | 50bf450063e8bd1acc186957c11de23b6a0174dd776e5afce558fc1d553667f8 |
| slice-win-arm64 | e3c540b054a45f7e1edfc667c5a4c5d23d66f1fb86e7a9ed9355fbb05fc0685a |
| SHA256SUMS | 84c40b7362258400474196c534a96424ba251029251e3485b9e170f53bb42850 |

| 对象 | 身份与字节数 | 回执来源路径 |
|---|---|---|
| 最终冻结基点 (F2 = T+2) | `78346c972b23c7c6ae0990e3e4e12ae889428409` (`T..F2` 两次 C 类测试提交: `F=288e3d80b4` `tools/check-gen-all-check.cx` 44->45 与 22+23 奇数分片, `F2=78346c972b` `tests/runcache-roots.cx` 同步 0.0.93 K4 UPI1 pid 合同; `src/`/`cx/`/`exec/`/`kernel/` 0 改动, 同 `SID=40af64c7f09d9b169a7c38fe6ce76fe8fcdb6cc0a4d8f81a178fe65aacef77f1` 与同候选 APE `c561e7e0f24d9b82e71b40c86920bc92c12b7035751c8d6e958930bbfe1dd843`) | `git rev-parse 78346c972b` / `release/candidate.tsv` |
| 候选整链与预跑基点 (T) | `a206d09fa16994e25166bcc893d96e1927df8309` (`release/candidate.tsv` 切 `0.0.93`, 紧接合并提交 `f182b60068`) | `git rev-parse a206d09fa1` / `release/candidate.tsv` |
| 链产出 APE (`chain_bytes.ape` / `unisacc-next.com`) | `c561e7e0f24d9b82e71b40c86920bc92c12b7035751c8d6e958930bbfe1dd843` (`8997165 bytes`, 相对 0.0.92 `8992973 bytes` `+4192 B`) | `~/.cache/unisacc/chains/bg-b1/a206d09fa16994e25166bcc893d96e1927df8309/ape-final/ape-receipt.txt` |
| 源快照 SID (`source-packages-v3`) | `40af64c7f09d9b169a7c38fe6ce76fe8fcdb6cc0a4d8f81a178fe65aacef77f1` | `~/.cache/unisacc/chains/bg-b1/a206d09fa16994e25166bcc893d96e1927df8309/seal.tsv` |
| 链产出权重包 (`chain_bytes.p3` / `compiler.p4.pkg` / `pkg.bin`) | `50bf450063e8bd1acc186957c11de23b6a0174dd776e5afce558fc1d553667f8` (`2904461 bytes`, 相对 0.0.92 `2904381 bytes` `+80 B`) | `~/.cache/unisacc/chains/bg-b1/a206d09fa16994e25166bcc893d96e1927df8309/ape-final/ape-receipt.txt` |
| 链产出包体 (`chain_bytes.p3_body` / `p3-package.body`) | `0a6e8256f1d96d70bbce6900fa164d42ebd30c666bcd7e95065fc6067b07128b` (`2904445 bytes`, 相对 0.0.92 `2904365 bytes` `+80 B`) | `~/.cache/unisacc/chains/bg-b1/a206d09fa16994e25166bcc893d96e1927df8309/ape-final/ape-receipt.txt` |
| 发布资产 `slice-win-arm64` (`asset_bytes.slice_win_arm64`) | `e3c540b054a45f7e1edfc667c5a4c5d23d66f1fb86e7a9ed9355fbb05fc0685a` (版本注入后; 链产出原始切片 `e84c3311cb23826d95757fc6227e7ecaca9d15adeb0593c44d6b189dc2761efb`, `4575232 bytes`, `+2560 B`) | `release/candidate.tsv` / `release/evidence/0.0.93/pack-run.out` |
| 发布资产 `SHA256SUMS` | `84c40b7362258400474196c534a96424ba251029251e3485b9e170f53bb42850` | `release/candidate.tsv` |

## 严格证据

459 批:PASS 457、FAIL 0、UNKNOWN 2、INVALID 0

- **候选整链(`T = a206d09fa16994e25166bcc893d96e1927df8309`, `F2 = 78346c972b23c7c6ae0990e3e4e12ae889428409`, `T..F2` 零 A 类变动、同 `SID=40af64c7f09d9b169a7c38fe6ce76fe8fcdb6cc0a4d8f81a178fe65aacef77f1`)**: 一次性 `DONE` (`iterations=8`, `rc=0`, 回执路径: `~/.cache/unisacc/chains/bg-b1/a206d09fa16994e25166bcc893d96e1927df8309/exit.tsv` `EXIT 1795349022 pid=3935777 rc=0 why=exited` 与 `bg.log` `SUPERVISE STOP iterations=8 rc=0`)。
  - `chain_bytes` 与 `0.0.92` (`96d0c6dd2e` `ape-receipt.txt`) 逐项对拍:
    - **真实有差(满足无字节差版本不单独公开规则)**:
      - 全部 6 个固定点执行切片(因 K1/K2 改双架构汇编核 + `sparse`/`rcident`/`CoreModel` 152->160):
        - `lnx/x86_64`: `448459cc1a3872629b0b0c5c246c080889b61dc5df545c07d939795dc809812b` (`2379494 B`, `+4112 B`)
        - `lnx/arm64`: `f0c0c0f73d17f0a08e56b11f99b5602c9b93f369d6ec3ca71ab6ec7e69305c23` (`2571950 B`, `+16 B`)
        - `osx/x86_64`: `da3d85fbd5188ddc7e6d46edcea192cf7a3e17e249993c96168bc8217067c1aa` (`3104482 B`, `DIFF`)
        - `osx/arm64`: `65b5455955f481e3b54378b04b88e9a3344f4e899d57e77c69d3bad8641051e5` (`4194274 B`, `DIFF`)
        - `win/x86_64`: `62f9c68d6550705808365170109db2886e32aff3f213a7d3ef631811c80994c8` (`3416064 B`, `+2048 B`)
        - `win/arm64`: `e84c3311cb23826d95757fc6227e7ecaca9d15adeb0593c44d6b189dc2761efb` (`4575232 B`, `+2560 B`)
      - `unisacc-next.com`: `c561e7e0f24d9b82e71b40c86920bc92c12b7035751c8d6e958930bbfe1dd843` (`8997165 B`, `+4192 B`, 含 K4 头前 8 KiB `UPI1` `pid` 行)
      - `p3` (`compiler.p4.pkg`) / `p3-package.body`: `50bf450063e8bd1acc186957c11de23b6a0174dd776e5afce558fc1d553667f8` (`2904461 B`, `+80 B`) / `0a6e8256f1d96d70bbce6900fa164d42ebd30c666bcd7e95065fc6067b07128b` (`2904445 B`, `+80 B`)
    - **逐字节恒等**:
      - `cache-manager/osx/x86_64`: `847748bb7afcb04c611390411a1ff6c6e264d79c444d9b1fadf5119c2c0a2d8d` (`247906 B`, `SAME`)
      - `cache-manager/osx/arm64`: `f41542f382d920a8938f32850de772e709e8b1668c6f4691951e99b02dd0f7dc` (`264418 B`, `SAME`)
- **冻结前门禁与字节等价复核(`a206d09fa1` / `78346c972b`)**:
  - `tools/derive.cx -- --check`: `DERIVE_SUMMARY ok=14 drift=0 refused=0 skipped=0`
  - `tools/ratchets.cx`: `RATCHETS ok=18 fail=0` (`tsv-lint errors=0 unresolved=10764`)
  - `tools/chain-ready.cx -- --version 0.0.93`: `11-version-slot PASS`, `19-chain-preflight PASS (a_files=5/16)`, `15-gen-all PASS (45/45)`, `5-candidate-chain PASS (139/139)`, `20-ident-helper PASS`, `READY yes failed=none`
  - K1/K2 私有核 `UNISA_KERNEL` (`x86_64.kernel sha256=9468a403...`, `7232 B`) 对拍 `0.0.92`:
    - `netregen` 30/30 (`5 案例 x 6 目标`): `NETREGEN_BYTES PASS shards=30 scope=cross-produced-images-only`
    - 20 项 `-c -o` 字节对拍 (`14 tests/cases + 6 a1a`, 2 MiB 零截断): `O_EQUIV_20 same=20 diff=0 err=0`
- **`ci-early` 六目标原生 CI**: `run_id=45020659842` 一次性 `6/6 PASS` (`verdict=PASS pass_cells=6 fail_cells=0`, 覆盖 `lnx/osx/win` x `x86_64/arm64` 全部 6 格原生宿主, `CI_EARLY_LOG_SCRUBBED home_paths_redacted=194`)。
- **三阶段证据绑定说明 (`F2 = T+2`)**: 候选整链、`ci-early` (`45020659842`) 与其余 151 窗预跑证据绑定 `T=a206d09fa16994e25166bcc893d96e1927df8309`; `T..F2` 共有两次 C 类测试例外提交:
  - (1) `F=288e3d80b451ac4dfa2ed412c9cd623d0a89977c` (`tools/check-gen-all-check.cx` 44->45 与 22+23 分片): 受影响批次 `W0008` (`check-gen-all-check`)、`W0121` (`check-gen-all-part2`)、`W0124` (`check-gen-all-split`) 重跑 PASS 并入;
  - (2) `F2=78346c972b23c7c6ae0990e3e4e12ae889428409` (`tests/runcache-roots.cx` 同步 0.0.93 K4 UPI1 pid 合同: 原样拷贝同 pid 断言 hit 正向覆盖 K4, `0<->1` 小写 hex 翻转 UPI1 pid 首字符构造异编译器身份断言 miss, 保留无 UPI1 头 legacy stat fallback): 受影响窗口 `W0123` (含 `runcache-roots` 17/17)在 `F2` 上重跑 `4/4 PASS` 并入;
  `T..F2` 在 `src/`/`cx/`/`exec/`/`kernel/` 下 0 改动, `source_sid=40af64c7f09d9b169a7c38fe6ce76fe8fcdb6cc0a4d8f81a178fe65aacef77f1` 与候选 APE `c561e7e0f24d9b82e71b40c86920bc92c12b7035751c8d6e958930bbfe1dd843` 恒等。
- **严格预跑(459 批 156 窗, DONE)**:
  - `PRERUN_MERGE windows=156 batches=459 PASS=457 FAIL=0 UNKNOWN=2 SUPERSEDED=0` (`strict-final.tsv sha256=e3d4b121cee1db60ef4990849b04bdca1d96812041207a824082d9633ffddf1a`)
  - 复跑并入明细: `W0000` (`opt-frame4-o2-negative-label`) 第二轮 `--resume` 转 `PASS` (`2509ms`); `W0008` (`check-gen-all-check`, `38051ms`)、`W0121` (`check-gen-all-part2`, `15779ms`)、`W0123` (`runcache-roots`, `35485ms`)、`W0124` (`check-gen-all-split`, `2366ms`) 在 `F`/`F2` 重跑全部 `PASS` 并入。
  - `expected-fail`: 0 项; `expected-unmeasured`: 2 项 `no-producer` (`cx-booter-ape`、`tools-default-regression`), 两份 `*-deferred.tsv` 绑定 `candidate_sha256=c561e7e0f24d9b82e71b40c86920bc92c12b7035751c8d6e958930bbfe1dd843` 与 `strict_sha256=e3d4b121cee1db60ef4990849b04bdca1d96812041207a824082d9633ffddf1a`。
- **`runcache` 三批候选 APE 联跑冒烟**: `SUMMARY batches=3 PASS=3 FAIL=0 UNKNOWN=0 INVALID=0 total_ms=11982` (`runcache` 24 + `runcache-units` 6 + `runcache-roots` 17 = `47/47 PASS`)。

## 产品改动

- **K1(稀疏内存初容 65536->256 与只清 used 段)**: `cx/core/sparse.cx`、`exec/c/asm/memory_{x86_64,arm64}.S`、`exec/c/asm/memorycheck.c`、`tools/check-core-sparse.cx` (`CXCORE_SPARSE failed=0 first_touch_us=14`, 含脏内存分配器用例)。
- **K2(E1 释放判空与 E2 核内清零)**: `exec/c/asm/{run,memory,intern}_{x86_64,arm64}.S`、`exec/c/asm/runcheck.c` (`asm_null_free=0 asm_memset=0`, 3 变异全杀)。
- **K3(core-abi 事实表生成与 CoreModel 预留指针)**: `exec/facts/core-abi.tsv`、`tools/core-abi-gen.cx`、`tools/check-core-abi-gen.cx` (`10/10 PASS`), `CoreModel` 尾追加预留指针 (`152->160`), `layout.h`/`actions.h` 生成物化。
- **K4(rcident UPI1 内容身份)**: `cx/runtime/rcident.cx`、`tools/ape-pack.cx`、`tools/check-rcident-upi1.cx` (`6/6 PASS`), 头前 8 KiB 写 `UPI1 pid`, 忽略 `cache/` 命名空间, 导出 `rk_pid`, 无 `pid` 旧包退回 `stat`。
- **K5(四速度探针样本入库)**: `tests/cases/speed/` (`deps-bc-{chain,flat}.c`、`sha-bc.c`、`tapelink-{a,b}.c`、`jobs.tsv`) 与 x86_64 七轮中位入 `build/speed-baseline.tsv`。

## 已知缺口

### 未跑范围(`UNMEASURED`)
- (1) 全套 `s9` `daily`/`release` 长跑未运行, 整体裁决保持 `NOT_QUALIFIED`;
- (2) 已知未测项 (`expected-unmeasured` = 2): `cx-booter-ape` 与 `tools-default-regression` (`reason=no-producer`, 两份 `*-deferred.tsv` 均绑定 `candidate_sha256=c561e7e0f24d9b82e71b40c86920bc92c12b7035751c8d6e958930bbfe1dd843` 与 `strict_sha256=e3d4b121cee1db60ef4990849b04bdca1d96812041207a824082d9633ffddf1a`, `expected-fail=0`);
- (3) `arm64_native` 实测: 已在 `ci-early` (`run_id=45020659842`) 6/6 原生宿主通过(`lnx/arm64`、`osx/arm64`、`win/arm64` 全 PASS), 正式发布仍以正式 `candidate-native.yml` 6/6 结果复核。

### 已知产品与工具缺口
- (1) 证据边界与已知缺口: `T=a206d09fa1` 到 `F2=78346c972b` 仅含两次 C 类测试提交(`F=288e3d80b4` 与 `F2=78346c972b`, 非产品代码, 同 `SID` 同 `APE`), 未在 `F`/`F2` 上整链重编与全量 153 窗重跑, 仅分别在 `F`/`F2` 重跑 `W0008`、`W0121`、`W0123` (`4/4 PASS`, 含 `runcache-roots` 17/17)、`W0124` 并入;
- (2) K4 同步缺口留痕: K4 合入时未同步 `tests/runcache-roots.cx`, 根因为 `tests/batch-product-inputs.tsv:49/57/59` 未声明 `cx/runtime/rcident.cx`、`tools/ape-pack.cx`、`src/runcache.cx`, 导致 `tools/affected.cx` 未命中 `runcache-roots`; 该依赖表缺口排入 `0.0.93` 公开后首刀修复;
- (3) E3 机器复用明确不进 `0.0.93`, 留后续版本槽。
