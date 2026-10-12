# c41 env-pin GATE 结证（1409 SGT）+ 假红定点修

**墙钟**：2026-10-12 ≈14:09–14:16 SGT  
**GATE**：`/tmp/c41-0843-envpin/cand.queue`  
**pin**：`env_pin_sha256=89bb83562ff9b3685bd7f84381cbbb25981e8f0a1d46f9dc536e2073ee3bc007`  
**cand**：`bcd2b9b75129cd511e5bb7fa7acad317e7a54d13bb6750ab4c540bbced07b2b9`  
**对照**：1146=`/tmp/c41-0843-csmith/cand.queue`（只对照；**未 void**）  
**tip（写回执时）**：`bb811af1`；`version.h`=0.0.41；Latest=**仍 v0.0.40**；H1=INHERIT；#256 已合。

## 结证摘要

| 项 | env-pin | 1146 对照 |
|---|---|---|
| completed | **721/721** | 720/720 |
| PASS rc0 | **627** | 597 |
| rc1 | **58** | 83 |
| rc142 | **20** | 24 |
| rc2 | **10** | 10 |
| rc77 UNVERIFIED | **6** | 6 |
| final | local acceptance **FAILED (rc=1)**；release NOT ready | 同 |
| vs 1146 | +30 PASS；rc1−25；rc142−4；假红回退 3 | — |

## 分桶（禁 void 1146）

### rc77 DEFER/UNVERIFIED（6；宿主不适用；覆盖义务保留）
exec-bootstrap-osxarm, exec-bootstrap-osxx86, lib-stack-arm, lib-stack-x86, lib-stack-x86-hostabi, lib-windows-gp-native

### rc2（10；seedgen 内存证据 UNKNOWN 面）
com-seedgen, seedgen, seedgen-2, seedgen-3, seedgen-4, seedgen-ppchain-1, seedgen-ppchain-2, seedgen-ppchain-3, seedparse2-1, seedparse2-2

### rc142（20；超时/看门狗面；多数已裁 H1/H2 基线）
closure-c3, closure-c4, com-luatests-build, com-tapebin-1, com-tapebin-2, com-tapebin-3, exec-f1-attributes, rowcov-enc-1, rowcov-enc-2, rowcov-enc-3, rowcov-enc-4, rowcov-enc-5, rowcov-enc-6, rowcov-enc-7, rowcov-enc-8, rowcov-parse2-build, rowcov-pp-locations, rowcov-pp-shared, shared-e2-located, shared-e2-plain

### 假红回退（相对 1146 PASS→rc1；env-pin 语义；已定点修）
gate-infra, gate-infra-38, queuestart — 根因：
1. `gate-infra`/`queuecheck`：`run()/main()` 粘性 `_ENV_PIN` + `execution_settings()` 仍读 live 打进 stamp；UNISA_MAXSTEPS 断言与 pin 合同冲突。
2. `gate-infra-38`/`queuecheck38`：预置 `results.json` 无 `env-pin.json` → resume 拒。
3. `queuestart`/`queuestartcheck.sh`：backup fixture 缺 `env-pin.json`。

### 改善（1146 红→env-pin 绿；节选）
n=32；含 lib-union*、lib-union16*、draftguard、ape-version、asmtext、closure-c2、com-fb12-multi、exec-e5、warningdriver Wall/Wextra 等。

## 定点修（有 pin 可安全 resume；未乱 resume）

改：`tests/gatequeue.py`（`execution_settings` 认 pin）、`tests/queuecheck.py`、`tests/queuecheck38.py`、`tests/queuestartcheck.sh`。  
本机已绿：queuecheck / queuecheck38 / queuestartcheck.sh / queuestartcheck.py / envpincheck。

## Draft/Latest 材料态（政委授推到 42 公开）

| 材料 | 态 |
|---|---|
| version.h 0.0.41 | 齐 |
| cand bcd2b9b7… + comboot fixedpoint | 齐 |
| H1=INHERIT | 齐 |
| queue 结证分桶 | **本文件** |
| seal / rc_tag / Draft create | **待开** |
| Latest / 签 | unsigned 可对齐 0.0.40 先例；签政策 `release_eligible:false` 另记 |

## 禁区遵守
未 void 1146；未无 pin resume；未删 py。

*结证 2026-10-12 ≈14:16 SGT*
