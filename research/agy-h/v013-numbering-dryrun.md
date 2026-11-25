# v0.1.3 编号预演与起链缺口清单

- 维护者: `agy-h` | 日期: `2026-11-25` | 依据规程: [SKILL.md](file:///home/devali/repos/unisacc/skills/unisacc-orch-release-train/SKILL.md) 第 (2)、(8) 节
- 预演对象: `git archive` 导出的 `v0.1.2` 树 (`.claude/worktrees/v0.1.2 HEAD 4e8aa13c6`) 解压至私有临时目录 `/tmp/agy_h_work/v013_dryrun`
- 交付入口: [v013-numbering-dryrun.md](file:///home/devali/repos/unisacc/research/agy-h/v013-numbering-dryrun.md)
- 隔离声明: 所有编号模拟、`git` 历史构造、`derive` 写入与 `chain-ready` 干跑全部发生在 `/tmp/agy_h_work/v013_dryrun`, 未改共享 `main` 与 `.claude/worktrees/v0.1.2`, 不提交。

---

## (1) 严格遵循 SKILL.md 第 (2) 节的「v0.1.3 编号提交一次做全」预演步骤与同步清单

### 1.1 起链环境与根副本基线

- **工作树根文件 (`12-root-files`, `12p-preflight-entry`)**:
  使用 `cp --remove-destination -L` 复制共享检出的 `unisacc.com` (`8,842,611` bytes) 与 `unisacc.com.build.json` (`314` bytes) 为常规文件副本 (不得为符号链接)。
- **固定 `env -i` 干净环境 (`10-envhygiene`, `20-ident-helper`)**:
  `env -i HOME="$HOME" PATH="/usr/bin:/bin:$HOME/.local/bin" TMPDIR=/tmp DEVSVC_OWNER=cc UNISACC_IDENT_HELPER="$HOME/.cache/unisacc/prep-cache/identity-helper-b9ed85e8f3890fa8.bin"`
  注意: 实测本机默认路径 `~/.cache/unisacc/ident-helper/identity-helper-real5` **不存在**, 必须显式指定 `prep-cache` 下已核二进制; 且严禁继承 `UNISACC_FREEZE_NO_FETCH`、`UNISACC_RUN_CACHE`、`UNISACC_CACHE_DIR` 与过期 `UNISACC_BOUND_DEADLINE_MONO`。

### 1.2 「编号提交一次做全」修改清单 (分两笔紧接提交)

依据 `SKILL.md` 第 (2) 节与 [chain-ready.cx](file:///home/devali/repos/unisacc/tools/chain-ready.cx#L1521-L1642) 门 `23-freeze-armed` 要求 (`build/tlib-packages.tsv`、`build/layer-api`、`build/iface-hash.tsv` 不得晚于冻结行):

1. **第一笔 (`C0a` 编号 + 派生物同步提交, 在所有编号文件落齐后一次提交)**:
   - [build/version-seq.tsv](file:///home/devali/repos/unisacc/.claude/worktrees/v0.1.2/build/version-seq.tsv): 确认前版第 67 槽 `0.1.2` 已标 `已公开` (由 `v0.1.2 release-post` 完成), 新增第 68 槽:
     `68	0.1.3	进程生命周期修复与 tape 切片车 M1(C3 进程生命周期修复重放 + tape M1 最小执行链路)	进行中	编排者 2026-11-25 立项 0.1.3;前置 0.1.2;计划 plans/v0.1.3.md`
   - [release/candidate.tsv](file:///home/devali/repos/unisacc/.claude/worktrees/v0.1.2/release/candidate.tsv): 版本列从 `0.1.2` 改为 `0.1.3` (满足门 `11-version-slot` 等于 `--version 0.1.3` / 分支名 `v0.1.3`)。
   - `plans/v0.1.3.md` (参考 [v013-numbering.md](file:///home/devali/.local/share/unisacc/research/agyneon/v013-numbering.md)): 首行 `# 0.1.3 计划:进程生命周期修复与 tape 切片车 M1` 与 `version-seq.tsv` class 子串一致 (`verseq-gen` R1/R2), 且必须含 `19-chain-preflight` 机械检查三字段 (`planned_units=8`, `a_class=进程生命周期修复与 tape 切片车 M1`, `a_files_cap=24`, `freeze_at=TBD`) 与正确回滚自指行 (`R5`)。
   - **`build/table-index.tsv` (本次干跑新发现必同步项)**: 因改 `build/version-seq.tsv` 等 `build/*` 文件触发 `derive` 的 `table-index` (`tool-index.cx -- --check-tables`), 必须同步运行 `tools/derive.cx -- --write --only table-index` 消除 `v0.1.2` 遗留漂移并随 `C0a` 提交 (见第 (3) 节缺口 #1)。
2. **第二笔 (`C0b` 冻结封口提交, 单独最后一笔, 以 `C0a` 40 位 SHA 为 `base_commit`)**:
   - [build/freeze.tsv](file:///home/devali/repos/unisacc/.claude/worktrees/v0.1.2/build/freeze.tsv): 确认 `0.1.2-FREEZE` 行已注释解冻, 新增 `0.1.3-FREEZE <C0a_BASE_HEX40> cc <DEADLINE_EPOCH_S> src cx ext exec include seed kernel tests release`。

### 1.3 需要同步的文件与派生物总表 (`C0` 编号期 vs `v0.1.3` `C3+M1` 合车起链前)

在干跑树上运行 `tools/derive.cx -- --check --changed ...`、`tools/derive.cx -- --check --touched <c3m1.touched> --plan` 及核对 `exec/facts/derive.tsv` 与 `SKILL.md` 第 (8) 节得出两阶段完整同步清单:

| 阶段 | 生成器 / 检查入口 | 输出目标文件 | 干跑实测状态 / 说明 |
| --- | --- | --- | --- |
| `C0` 编号期必跑 | `tools/verseq-gen.cx -- --check` | `version-seq.tsv`, `plans/v0.1.3.md`, `prd_mempalace.md` | 模拟 `0.1.3` 后实测 `VERSEQ_SUMMARY checks=100 fail=0` |
| `C0` 编号期必跑 | `tools/release-packcheck.cx -- slot . --version 0.1.3` | `release/candidate.tsv` | 模拟后 `VERSION_SLOT OK version=0.1.3 target=0.1.3` |
| `C0` 编号期必跑 | `tools/derive.cx -- --write --only table-index` | `build/table-index.tsv` | **未写则 `DRIFT` (`DIFF line=17`)**; 写后 `OK 242` |
| `C0` 编号期必跑 | `tools/no-home-path.cx -- --check` | 全仓文档与计划表 | 实测 `ok: files=134 lines=3502 over=0` |
| `C0` 编号期必跑 | `tools/affected.cx -- --files <4个编号文件> --list-only` | 回归影响清单 | 实测 `changed=4 batches=620 scanned=2014` |
| `C3+M1` 合车必同步 (`derive --touched`) | `tools/layer-api.cx -- --bump-pending` (及 `--bump cx.os,ext.sysx`) | `build/layer-api/*`, `build/tlib-packages.tsv` | `C3` 修改 `cx/sys/*`, `ext/sysx/*` 必触发; 须在 `C0b` 冻结行前完成 |
| `C3+M1` 合车必同步 (`derive --touched`) | `tools/iface-hash.cx -- --write` | `build/iface-hash.tsv` | 紧随 `layer-api` 更新; 须在 `C0b` 冻结行前完成 |
| `C3+M1` 合车必同步 (`derive --touched`) | `tools/tables-gen.cx -- --write`, `st-gap-gen.cx`, `chain-phases-gen.cx`, `libc-decl-scan.cx`, `cx0-diff.cx` | `tests/{batches,coverage}.tsv`, `build/{st-gap,chain-phases,libc-decls,cx0-three-source-diff}.tsv` | `--touched` 对 `C3(7)+M1` 全部实测命中 (`SELECT_SUMMARY selected=tables-gen,layer-api,iface-hash,st-gap,chain-phases,libc-decls,table-index,cx0-three-source`) |
| `C3+M1` 合车必同步 (`SKILL.md` 第 (8) 节) | `tools/product-map.cx -- --write`, `batch-packages-gen.cx -- --write`, `batch-domains-gen.cx -- --write`, `tool-index.cx -- --write-tools`, `links-gen.cx -- --write`, `doc-links-gen.cx -- --write` | `build/{product-map,batch-packages,tool-index,links,doc-links}.tsv`, `tests/batch-domain-index.tsv`, `prd_productmap.md` | `batch-packages-gen` 须在 `derive` `--prepare-native` 后静默单跑 (`21.4s` OK, 并发下超时 `30s`) |
| `C3+M1` 产品/批表手工与生成器同期 | `tests/core-interface.tsv`, `tests/daily-compile-smoke.cx --write`, `tools/lane-inputs-gen.cx --write`, `tools/gen-all-check.cx --list`, `tools/register.cx`, `tests/tools-compile-sources.tsv`, `tests/batch-product-inputs.tsv` | 对应表文件与 `CGAC_GENERATORS` | 基线 `daily-compile-smoke --check` `ok=8 failed=0` (`717ms`), `lane-inputs-gen --check` `SAME bytes=98204`, `gen-all-check --list` `total=45 uncovered=2` |

---

## (2) `chain-ready.cx` 干跑原始输出记录 (编号前 vs 模拟 v0.1.3 编号后)

干跑命令形式: `nice -n 19 taskset -c 10-11 timeout -k 3s 45s env -i ... UNISACC_IDENT_HELPER="$HOME/.cache/unisacc/prep-cache/identity-helper-b9ed85e8f3890fa8.bin" sh ./unisacc.com tools/chain-ready.cx -- --version 0.1.3 --only <GATE>` (注意: `tools/chain-ready.cx:1728` 的 `--only` 为单前缀匹配, 不支持逗号拼接多门名)。

### 2.1 阶段 A: 纯 `v0.1.2` `git archive` 基线 (尚未写入 `v0.1.3` 编号) 原行

| 门名 | 状态 | 逐字原始输出行 (`stdout` 原样粘贴) |
| --- | --- | --- |
| `11-version-slot` | `FAIL` | `11-version-slot	FAIL	rc=2 REFUSE CHAIN_READY_VERSION_SLOT slot-version-differs-from-target 0.1.2 0.1.3	8456` |
| `18-parse2-gen` | `FAIL` | `18-parse2-gen	FAIL	READY=no parse2-gen-identity base=refs/tags/v0.1.1 fix: 修复指名事实并提交后重跑 sh ./unisacc.com tools/chain-ready.cx -- --only 18 --rev HEAD	22` |
| `23-freeze-armed` | `FAIL` | `23-freeze-armed	FAIL	FREEZE_ORIGIN_MAIN_UNKNOWN git fetch origin failed	27` |
| `8-inflight` | `FAIL` | `8-inflight	FAIL	1 chain process(es) running, first: 1890812 /home/devali/.cache/unisacc/unisacc-96fa989705997477 tools/candidate-chain.cx -o /home/devali/.cache/unisacc/pf-	51` |
| `19-chain-preflight` (默认全表) | `TIMEOUT` | 在 2 核下超过 60s 规则限退出; 单独拆测子项 `chain-to-snap` `cts_rc=124` (超过 45s 限) |
| `15-gen-all` | `TIMEOUT` | `rc=124` (2 核 + 在途链负载下 50s 超时) |
| `2b-ratchets` | `PASS` | `2b-ratchets	PASS	RATCHETS ok=19 fail=0 elapsed_ms=22789 (roster=19)	23257` |
| 其余秒级基线门 | `PASS` | `1-rev` (`branch v0.1.3`), `10-envhygiene`, `12-root-files`, `12p-preflight-entry`, `13-snap-cache`, `14-root-record` (`WARN version=0.0.105`), `16-disk-free`, `17-root-public`, `20-ident-helper`, `21-verseq-gen` (`checks=97 fail=0`), `3b-cx0-extract` |

### 2.2 阶段 B: 模拟完成 `v0.1.3` 编号 (`C0a` 提交 `d6146047` -> `C0b` 独立 `0.1.3-FREEZE` 提交 `f0d72ca9`, 模拟 `origin/main` 祖先与基线 `v0.1.2` tag)

```text
1-rev	PASS	no uncommitted or untracked paths under src cx ext exec/facts; branch v0.1.3	96
10-envhygiene	PASS	no inherited deadline and no forced cache variable in this environment	0
11-version-slot	PASS	VERSION_SLOT OK version=0.1.3 target=0.1.3 source=arg expect_self_report=unisacc 0.1.3	47
12-root-files	PASS	unisacc.com and unisacc.com.build.json under . are regular files	59
12p-preflight-entry	PASS	preflight entry record ./unisacc.com.build.json is a regular file	58
13-snap-cache	PASS	no snapshot cache for rev f0d72ca927 at /home/devali/.cache/unisacc/chains/snap/f0d72ca927-a28e8815: the chain builds a fresh one	33
14-root-record	PASS	product_digest and bytes match the APE; WARN root-record-stale-fields version=0.0.105 has no release/candidate.tsv row: commit and sources_sha256 not verified	35
16-disk-free	PASS	avail_gb=69 (>=20G); disk=/home/devali/.cache/unisacc/chains	0
17-root-public	PASS	Root is not a candidate install (no unisacc.com.candidate.tsv): release steps may use it as the baseline	0
18-parse2-gen	PASS	base=389efc634ca33c89ab5604a4b256e7aefc3cc2a6 rev=f0d72ca9270e160266f06f781ebdd0eafbca7e91 parse2-inputs-unchanged	12
20-ident-helper	PASS	UNISACC_IDENT_HELPER=/home/devali/.cache/unisacc/prep-cache/identity-helper-b9ed85e8f3890fa8.bin executable	0
21-verseq-gen	PASS	VERSEQ_SUMMARY checks=100 fail=0	58
23-freeze-armed	PASS	armed: freeze.tsv sid=0.1.3-FREEZE owner=cc deadline=1795640000; HEAD behind origin/main=0; numbering files unchanged after the freeze	60
3b-cx0-extract	PASS	CX0_EXTRACT EXACT bytes=118485 sha256=0a92fb19f87540cb6ae2af2776601eca89101f0da0a8fa8299a576d8fe0b7e73 output=src/cxmod.c authority=cx/cx0/p	73
8-inflight	PASS	no candidate-chain process	59
6-affected	PASS	no changed .cx files (dirty tree and --since range)	0
3a-cxmod-tri	PASS	CXMOD_TRI passed=25 failed=0 gaps=0 shard=core	821
3a-cxmod-tri-exports	PASS	CXMOD_TRI passed=21 failed=0 gaps=0 shard=exports	249
```

追加专项与派生实测原行:
```text
# derive --check --changed (编号 4 文件触发子集):
DERIVE	product-map	OK	41829
DERIVE	doc-links	OK	19176
DERIVE	batch-packages-gen	TIMEOUT	30132	rc=142	bound: timeout after 30 seconds: /tmp/unisacc-register-native/b825a3222a59b0045f8eba2a555d9efa840887bd3dda0d05cc2d703867d39e83/batch-packages-gen
DERIVE	table-index	DRIFT	1263	rc=1	DIFF build/table-index.tsv line=17
DERIVE_SUMMARY	ok=2	drift=1	refused=1	skipped=12	elapsed_ms=41836

# 单独静默重跑 batch-packages-gen (--only batch-packages-gen):
DERIVE	batch-packages-gen	OK	20944
DERIVE_SUMMARY	ok=1	drift=0	refused=0	skipped=15	elapsed_ms=21389

# 先 derive --write --only table-index 再验证:
DERIVE	table-index	WROTE	255
DERIVE	table-index	OK	242

# 19-chain-preflight (使用含 4 必选 M2/M3 行 --preflight-table pf_mini.tsv 避开 >50s sixtarget/chain-to-snap):
19-chain-preflight	PASS	CHAIN_PREFLIGHT rev=HEAD verdict=PASS failed=- nodes=4 ms=52 mode=strict; scope version=0.1.3 planned_units=8 a_class=进程生命周期修复与 tape 切片车 M1 a_files=0/24	97

# 其余专项检查:
SOURCEPACKAGE_PINS	assertions=24	failed=0
no-home-path: ok: files=134 lines=3502 over=0 (fix: tools/scrub-home-paths.cx --write FILE)
CHECK_DAILY_COMPILE_SMOKE	ok=8	failed=0	ms=717
LANE_INPUTS_GEN	SAME	bytes=98204	path=build/lane-inputs.tsv
GEN_LIST_SUMMARY	total=45	uncovered=2
```

超时/未完整通过项原行:
```text
2a-derive (--only 2a-derive, 2核50s限): rc2a=124 (冷 derive --prepare-native 首轮也报 prep_rc=124)
5-candidate-chain (--only 5-candidate-chain, 2核50s限): rc=124
22-premerge-union (--only 22-premerge-union, 2核50s限): rc22=124
19-chain-preflight (默认全表, --item chain-to-snap): cts_rc=124
```

---

## (3) `v0.1.3` 起链前的真实缺口清单 (实际红门与原因)

1. **`table-index` 基线已漂移 (`2a-derive` 阻断红)**:
   - 原行: `DERIVE table-index DRIFT 1263 rc=1 DIFF build/table-index.tsv line=17`。
   - 根因: `v0.1.2` (`4e8aa13c6`) 中提交的 `build/table-index.tsv` 仍含已移出 `git ls-files` 的 `build/doc-links.tsv` (第 17 行) 与 `build/links.tsv` (第 51 行), 即使在本地重新 `--write --only doc-links,links` 生成两张未跟踪表文件, `--check --only table-index` 仍报 `DRIFT`。`v0.1.3` `C0a` 编号提交时须一并执行 `tools/derive.cx -- --write --only table-index` 将其清零并提交。
2. **`2a-derive` 高并发下 `batch-packages-gen` 必撞 30s 超时 (`TIMEOUT rc=142`)**:
   - 原行: `DERIVE batch-packages-gen TIMEOUT 30132 rc=142`。
   - 根因: `exec/facts/derive.tsv:9` 为 `batch-packages-gen` 设定 `bound_s=30`, 与 `product-map` (`41.8s`)、`doc-links` (`19.2s`) 同入 `--check` 4 路池并发时超过 30 秒; 单独静默实跑耗时 `20.9s OK`。起链前必须先跑完 `derive.cx -- --check --prepare-native` (仅备原生运行缓存), 再静默单跑 `--only batch-packages-gen`。
3. **重型前置门 (`15-gen-all`, `5-candidate-chain`, `22-premerge-union`, `19-chain-preflight` 全表) 在受限 2 核与冷缓存下全部超时 (`rc=124`)**:
   - 包括 `5-candidate-chain` (文档要求静默单跑两轮)、`19-chain-preflight` 里的 `chain-to-snap` (`cts_rc=124`) 与 `sixtarget` 四节点 (`44..55s`)、`22-premerge-union` (冷态两分片合计超 50s)。`v0.1.3` 正式起链前须在非限核静默窗口分步运行并完成全套 `--prepare-native` 预热, 且 `4-p1` (`--fast` 时 `SKIPPED`) 在起链前不得跳过, 须跑完不带 `--fast` 的完整 `chain-ready`。
4. **默认 `UNISACC_IDENT_HELPER` 路径不存在 (`20-ident-helper`)**:
   - 实查 `~/.cache/unisacc/ident-helper/identity-helper-real5` 缺失, 必须在 `env -i` 显式指向 `~/.cache/unisacc/prep-cache/identity-helper-b9ed85e8f3890fa8.bin` (或先用 `seed/ident.c` 重建后 `cmp` 验证), 与 [toolline-roadmap.md](file:///home/devali/repos/unisacc/research/agy-h/toolline-roadmap.md) `S3` 待修一致。
5. **前版依赖与远程门前置尚未落地 (`11`, `18`, `23`, `8` 现时阻断)**:
   - `v0.1.2` 尚未 Y2 公开与解冻: 共享 `build/freeze.tsv` 仍含活跃 `0.1.2-FREEZE`, `version-seq.tsv` 缺第 68 槽 `0.1.3`, `release/candidate.tsv` 仍为 `0.1.2`, 仓库缺 `refs/tags/v0.1.2` (`18-parse2-gen` 需对应 `已公开` 标签) 与 `plans/v0.1.3.md` (`check_version_scope` 必填 `planned_units` / `a_class` / `a_files_cap`); 干跑基线时刻宿主机还存在真实在途链 (`8-inflight FAIL pid=1890812`)。
6. **已装 Root 与候选记录表滞后告警 (`14-root-record`)**:
   - 原行: `WARN root-record-stale-fields version=0.0.105 has no release/candidate.tsv row: commit and sources_sha256 not verified`。目前共享根目录安装件仍为 `0.0.105`, 待 `v0.1.2` 公开并升级 Root 后, 开 `.claude/worktrees/v0.1.3` 时须用 `cp --remove-destination -L` 同步新 Root 常规副本并清 `~/.cache/unisacc/chains/snap/` 旧快照。
7. **`C3 + M1` 合车之后的待补充派生同步 (`SKILL.md` 第 (8) 节)**:
   - 本次干跑仅覆盖 `C0` 编号骨架; 一旦在 `v0.1.3` 分支并入 `C3` (7 个产品文件) 与 `M1` (tape 工具与批), 必须按第 1.3 节续跑 `layer-api --bump cx.os,ext.sysx`、`iface-hash --write`、`tests/core-interface.tsv`、`batch-packages-gen --write`、`tables-gen --write`、`product-map --write`、`links` 与 `register.cx` (均须早于 `C0b` `0.1.3-FREEZE` 提交)。
