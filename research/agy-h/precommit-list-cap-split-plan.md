# V2 工具线: `precommit-list-cap` 四片拆分方案 (预跑后实施)

维护窗口: `agy-h` | 日期: 2026-11-25 | 状态: 预跑只读设计页(零编译、未改仓库代码、未提交)
目标批: `tests/check-precommit-list-cap.cx` (`tests/batches.tsv:2032`, `tests/coverage.tsv:1649`)

---

## 1. 现状与瓶颈定位 (基于源码与 `list-cap-margin.md`)

(1) 贴限实证(`release/evidence/0.1.0/list-cap-margin.md`、`release/evidence/0.0.107/list-cap-margin.md`)
- `precommit-list-cap` 在低负载(`load1=1.43`..`1.93`)下实测仍稳定耗时 `55.49s`..`55.59s`，触碰 55 秒窗口 (`UNKNOWN rc=0 transport=-1`)，外窗约 `58.0s`。

(2) 耗时结构 (`tests/check-precommit-list-cap.cx:42-128`)
- 该检查先建 1 个临时 git 仓，随后在同一夹具内连续串行运行 4 次完整 `sh ./unisacc.com tools/precommit.cx --`:
  1. Case 1 (`line 90-92`): 大输出 (>1MiB) 含末尾 `unknown-coverage row=999 id=after-1mib` -> `precommit-list-cap.serial-after-1mib-refused`;
  2. Case 2 (`line 93-96`): 同大输出 + `UNISACC_PRECOMMIT_LIST_MAX=1048576` -> `precommit-list-cap.serial-tightened-cap-refused` (包含 `setenv-tighten`);
  3. Case 3 (`line 97-110`): 覆写并重暂存小输出 `PLAN metadata_valid=1` -> `precommit-list-cap.normal-plan-metadata-valid-passes` (包含 `setenv-default`);
  4. Case 4 (`line 111-122`): 覆写并重暂存小输出 `PLAN metadata_valid=0` -> `precommit-list-cap.small-plan-metadata-invalid-refused`。
- 四次真实提交门 + 每轮重编夹具内的 `tests/run.cx` 线性叠加导致单批必触 55 秒上限。

---

## 2. 参照候选 17 的四片独立拆分方案

按 `cc` 要求「每片小于 45 秒(参照候选 17 的 `list`/`cand17` `--part` 分片法)」，同时兼顾严格预跑 `27.5s` 余量线，在 `tests/check-precommit-list-cap.cx` 内增加 `--part <after-1mib|tighten-cap|plan-pass|plan-deny|all>` 解析（无 `--part` 时兼容全量跑 `all`，但 4 个登记批各自显式传对应分片参数）：

| 登记批 ID (`tests/batches.tsv`) | `--part` 参数 | 涵盖独立用例与夹具动作 | 保留具名断言 (`PASS`/`FAIL`) | 预期墙钟 |
|---|---|---|---|---|
| `precommit-list-cap` (原名承接首片) | `--part;after-1mib` | 建独立小仓 + 写大输出 `tests/run.cx` + 跑 1 次 `precommit.cx` | `fixture-setup`, `fixture-git-init`, `serial-after-1mib-refused` | 约 14s (`UNMEASURED`, 待实测) |
| `precommit-list-cap-tight` (新登记) | `--part;tighten-cap` | 建独立小仓 + 写大输出 `tests/run.cx` + 设 `UNISACC_PRECOMMIT_LIST_MAX=1048576` + 跑 1 次 `precommit.cx` | `fixture-setup`, `fixture-git-init`, `setenv-tighten`, `serial-tightened-cap-refused` | 约 14s (`UNMEASURED`, 待实测) |
| `precommit-list-cap-pass` (新登记) | `--part;plan-pass` | 建独立小仓 + 写小输出 `PLAN metadata_valid=1` + 跑 1 次 `precommit.cx` | `fixture-setup`, `fixture-git-init`, `normal-plan-metadata-valid-passes` | 约 14s (`UNMEASURED`, 待实测) |
| `precommit-list-cap-deny` (新登记) | `--part;plan-deny` | 建独立小仓 + 写小输出 `PLAN metadata_valid=0` + 跑 1 次 `precommit.cx` | `fixture-setup`, `fixture-git-init`, `small-plan-metadata-invalid-refused` | 约 14s (`UNMEASURED`, 待实测) |

设计检查要点:
(1) 4 个业务核心断言（`serial-after-1mib-refused`、`serial-tightened-cap-refused`、`normal-plan-metadata-valid-passes`、`small-plan-metadata-invalid-refused`）100% 不遗漏、不重复；`setenv-default` 为跨组清理，若拆片独立起仓并在需要时设定，不丢失任何覆盖语义（也可在 `plan-pass` 中显式设定 `UNISACC_PRECOMMIT_LIST_MAX=16777216` 并保留 `setenv-default` 断言）。
(2) `precommit-list-cap` 原批名本身通过 `--part;after-1mib` 成为第 1 片，避免在 `tests/batches.tsv` 里保留一个 55 秒必定超时的旧全量批拖垮预跑或日常全跑（与候选 17 `affected-list` 收窄做法完全同形）。
(3) 每片仍保留 55 秒登记窗（绝不放宽窗口，反而是四片各以 `~14-16s` 稳步运行于 45s / 27.5s 余量线以下）。

---

## 3. 登记表新增/修改行草稿 (`commit-own --keyed`)

### 3.1 `tests/batches.tsv` (1 改 + 3 增)
```tsv
precommit-list-cap	IMPLEMENTED	interfaces	tests/check-precommit-list-cap.cx	SOURCE	lnx/x86_64	55	-	tests/check-precommit-list-cap.cx	--part;after-1mib	interfaces.precommit-list-cap	rc0:CHECK_PRECOMMIT_LIST_CAP	tree	daily
precommit-list-cap-tight	IMPLEMENTED	interfaces	tests/check-precommit-list-cap.cx	SOURCE	lnx/x86_64	55	-	tests/check-precommit-list-cap.cx	--part;tighten-cap	interfaces.precommit-list-cap-tight	rc0:CHECK_PRECOMMIT_LIST_CAP	tree	daily
precommit-list-cap-pass	IMPLEMENTED	interfaces	tests/check-precommit-list-cap.cx	SOURCE	lnx/x86_64	55	-	tests/check-precommit-list-cap.cx	--part;plan-pass	interfaces.precommit-list-cap-pass	rc0:CHECK_PRECOMMIT_LIST_CAP	tree	daily
precommit-list-cap-deny	IMPLEMENTED	interfaces	tests/check-precommit-list-cap.cx	SOURCE	lnx/x86_64	55	-	tests/check-precommit-list-cap.cx	--part;plan-deny	interfaces.precommit-list-cap-deny	rc0:CHECK_PRECOMMIT_LIST_CAP	tree	daily
```

### 3.2 `tests/coverage.tsv` (1 改 + 3 增)
```tsv
interfaces.precommit-list-cap	interfaces	IMPLEMENTED_SEPARATE_BATCH	precommit 读缓冲扩容至 16MiB 检出 1MiB 后 unknown-coverage 拒绝(after-1mib 分片, 整输出读入上限非流式)	tools/precommit.cx tests/check-precommit-list-cap.cx
interfaces.precommit-list-cap-tight	interfaces	IMPLEMENTED_SEPARATE_BATCH	precommit UNISACC_PRECOMMIT_LIST_MAX=1048576 收紧上限具名拒绝(tighten-cap 分片)	tools/precommit.cx tests/check-precommit-list-cap.cx
interfaces.precommit-list-cap-pass	interfaces	IMPLEMENTED_SEPARATE_BATCH	precommit 正常输出 PLAN metadata_valid=1 放行验证(plan-pass 分片)	tools/precommit.cx tests/check-precommit-list-cap.cx
interfaces.precommit-list-cap-deny	interfaces	IMPLEMENTED_SEPARATE_BATCH	precommit 小输出 PLAN metadata_valid=0 具名拒绝验证(plan-deny 分片)	tools/precommit.cx tests/check-precommit-list-cap.cx
```

(并同步更新 `tests/manifests/precommit-list-cap.tsv` 及新增三份清单以保持 `register.cx` 与清单校验一致。)

---

## 4. 预跑结束后验收流程

(1) 在私有或工具开发分支编辑 `tests/check-precommit-list-cap.cx` 加上 `--part` 参数分片。
(2) 冷态实测 4 片各自墙钟（每片独立运行 3 次取最大，要求每片 `< 27.5s` 且 `< 45s`），并执行先红后绿反证（`UNISACC_PRECOMMIT_LIST_MAX` 检查故障注入后 `after-1mib` / `tighten-cap` 必红）。
(3) 确认 `tests/check-precommit-list-cap.cx`、`tests/batches.tsv`、`tests/coverage.tsv`、`tests/manifests/precommit-list-cap.tsv` 与 `tests/manifests/precommit-list-cap-{tight,pass,deny}.tsv` 一致且 `affected` / `cx-lint` 通过后，按 `V2` 工具线 `commit-own --keyed` 仅提交自己行与路径。
