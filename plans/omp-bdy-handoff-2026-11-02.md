# omp-bdy 交接说明 2026-11-02

> 全新会话续做只需本文件 + `git log`。本文写于重启前，事实截至 2026-11-02。

## 当前任务（慢车道纪元试点 pp-negative）

目标：在私有根 `out/epoch-pp1/` 上把慢车道纪元（换标）构造链跑通，用新候选实跑 `pp-negative` 的 2 项转绿，作为矩阵换标证据的第一棒。

## 本次已闭合的根因（receipt-bytes-mismatch）

`tools/source-package.cx` job 与 `--verify-context` 报 `REFUSE receipt-bytes-mismatch` 的完整根因链：

1. **go 已落 split2**（提交 `416c8dfd70` 及之前）：`pp_identity_kind(root)` 现在返回 `"split2"`（旧读作 `legacy-split1`），`pp_product_digest` 用 `sy_lstat`（旧 `lstat` 配 `struct stat` 在这条链上不可靠：`sy_exists` 说存在、`lstat` 报 ENOENT，导致 DBGT 12 → `pp_product_digest` rc=-1）。
2. **我的 `source-package.cx` 未随 split2 更新**：`observe_inputs`/`prepare_context` 仍用 `digest(product)`（整文件 sha，旧 split1 口径），`root_epoch_execution_record` 里硬编码 `"split1"`。已改成 `pp_product_digest(root, …)` + `pp_identity_kind(root)`（staged，未提交）。
3. **私有根 `build.json` 仍是旧 split1 值**：`out/epoch-pp1/root/unisacc.com.build.json` 的 `product_kind="split1"`、`product_digest=6b95204e…`（旧口径）。split2 正确值应为 `product_kind="split2"`、`product_digest=14b906fd0541cee61a617f8d56c00ecaef1f63b9c0106bc82782768c7c710b04`（已探针实跑确认，与 go 验证值一致）。
4. **`context.tsv` 是旧 split1 digest**：`root-product-sha=6b95204e…`（prepare 时旧口径写的），需删 context 重跑 `prepare-context`。

验证证据：`pp_product_digest(out/epoch-pp1/root)` 现在 `rc=0 digest=14b906fd… kind=split2`。

## 接口约定（split2，定稿，来自 omp-go 信封）

四个入口全在 `tools/cxlib/productpkg.cx`：

- `int pp_product_digest(const char *root, char hex[65])` — 0 成功 / -1 拒绝
- `const char *pp_identity_kind(const char *root)` — `"ape1" | "split2" | "none" | "error"`
- `int pp_member_list(root, names[][64], shas[][65], long *bytes, int max)` — 成员数 / -1
- `void pp_digest_or_die(root, hex)` / `void pp_digest_path_or_die(path, hex)`

算法（split2 拆分布局）：成员 = 启动器 `unisacc.com` + `ape-receipt.txt` 第二字段的 basename；无 receipt 时固定 `entry.elf`、`p3-package.bin`；按 basename 字典序排，每成员 `"name<TAB>sha256\n"` 拼接，`sha256(串)`。**bytes 不进摘要**。拒绝：>16 成员 / 名长≥64 / receipt 空 / 成员缺失或符号链接 / 布局判不出（EACCES/ENOTDIR 算错，仅 ENOENT 算不存在）。

**build.json 建议写两项**：`product_kind = pp_identity_kind(root)`、`product_digest = pp_product_digest(root)`。**不要写 split1**（它不是可产出值，只是历史读法）。

## 工作区未提交改动（staged，9 文件，需归属拆分）

`git status --short`（M 首列=staged）：

| 文件 | 归属 | 说明 |
|---|---|---|
| `tools/source-package.cx` | bdy | 工具修复(1)：`+use cxlib.productpkg`；`digest(product)→pp_product_digest(root)` 两处；`"split1"→pp_identity_kind(root)`。**待提交** |
| `exec/pp/macro-byte.tsv` | bdy | 慢车道试点表：MFIND 加 256→REJECT + 65-90,95,97-122 行 + `*`→REJECT。cc 批准直接改表。**待提交** |
| `exec/pp/include-location-result.tsv` | bdy | 慢车道试点表：INC.location 加 256→REJECT。**待提交** |
| `.gitignore` | 共享? | `+tests/.register.lock`（register 工具相关，可能 go 的）。用 commit-own 分辨 |
| `plans/materials-rebuild-2026-11-02.md` | 共享 | 材料重建路径评估（只读） |
| `plans/orca-install-plan-2026-11-02.md` | 共享 | Orca 安装方案 |
| `plans/product-identity-2026-11-02.md` | 共享 | 产品身份实现 |
| `plans/product-identity-review-2026-11-02.md` | 共享 | 产品身份第 0-4 步评审 |
| `plans/v0.0.44.md` | 共享 | 0.0.44 计划 |

**提交口径**：自己的行用 `tools/commit-own.cx --keyed`（bdy 三件：source-package.cx、两 tsv）；共享文件只带自己的行。提交前查 `tools/own-diff.cx` 看非自己改动。

## 待对接项（与 omp-go）

- **无阻塞**：split2 接口已定稿落地，bdy 直接接 `pp_product_digest`/`pp_identity_kind`。
- 待办（bdy）：`release-ape.cx` 的 devlnx 产出时机器写 `build.json`（工具修复(2)，用 pp_product_digest + pp_identity_kind，不硬编码 split1）。当前 `release-ape.cx` 的 devlnx 只写 `ape-receipt.txt`，不写 build.json。

## 未决问题 / 下一步

1. **重写私有根 build.json 为 split2**（product_kind=split2、product_digest=14b906fd…）。cc 裁定 build.json 由工具机器写、不由人写——优先在 `release-ape.cx` 落机器写，或临时用 pp_digest 输出正确值重写 `out/epoch-pp1/root/unisacc.com.build.json` 再跑。
2. **删 `out/epoch-pp1/context/` 重跑 prepare-context**（旧 context 是 split1 digest，`--verify-context` 必 mismatch）。
3. 继续慢车道纪元：`source-job-wave.cx` 107 job（5 tool + 34 gen + 34 tbl + 34 net）→ kernel-build → release-build → entry-build → 独立候选实跑 pp-negative。
4. 每个构造步骤发进度信封（耗时、产物 sha、receipt 路径，失败即停）。

## 私有根纪元结构（out/epoch-pp1/）

- `Root41-bootstrap.com`（sha d93c1b24…，薄头 copy）
- `root/`：符号链接 tools/seed/include/exec/weights/cx/src/kernel/unisa/iterate → 仓内对应目录；`unisacc.com`（薄头 d93c1b24）；`unisacc.com.dev/{entry.elf(de54dd80), p3-package.bin(47a2275d)}`；`ape-receipt.txt`；`unisacc.com.build.json`（旧 split1）
- `context/`：prepare-context 产物（旧 split1 digest）
- `ident-helper/`：identity-helper 编译产物

## 关键环境

- 仓根 `/home/devali/repos/unisacc`，moltbaby 仓 `/home/devali/repos/moltbaby`（只改文件不提交，主人提交）。
- 单命令 ≤60s 用 `timeout`/`prlimit` 包裹；护栏只在子 shell/单命令（`prlimit --nproc=N cmd`），不在常驻 shell ulimit。
- 命令：`sh ./unisacc.com -run tools/source-package.cx -- --verify-context $ROOT $CTX`（`-run` 直接跑改后源码，别用 `sh ./unisacc.com tools/...` 旧产品编译）。
- 提交：`git -c user.name=wanjo -c user.email=wanjo@asu.edu commit -q -F msg.txt -- <路径>`；共享文件 `commit-own --keyed`。
- procguard 看门狗常驻（>100 同 key 先 SIGSTOP 再 SIGKILL）；跑前查 `~/.cache/procguard/log`。
- 七棘轮 `tools/ratchets.cx` 必须 ok=7；srcshape 只降不升（当前 5326<5331）。

## proxy 隔离调查（「 字符来源）

**移到重启后新会话做**。cc 已记录旧会话现象，不必再追。任务：moltbaby 仓 `skills/omp/assets/cx-harness/buddy-native-proxy.ts`（端口 18931）加只读调试开关（默认关），把上游原始 SSE 与下发 omp 的 SSE 各 tee 到 `out/proxy-trace/`，对比 reasoning_content 里 `「` 数量/位置。相同=上游/模型问题；下发多=代理 bug（定位 buddy-native-proxy.ts 约 180-200、383-444 行 reasoning/delta 合并）。上游问题则记进 `moltbaby/skills/omp/SKILL.md` 踩坑表。moltbaby 只改文件不提交。
