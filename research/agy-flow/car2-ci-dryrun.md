# 第二车 0.0.107 CI 起跑前核对清单与红时分诊表

- 适用范围:第二车 `0.0.107`(tag `v0.0.107`,分支 `v0.0.107`)起链前静态核对、Step 2 草稿后至 Step 3 派发 `candidate-native.yml` 前的五条必过挡板,以及 Step 3 CI 出现红格时的四分类定因与 `G4` 处置规程。
- 依据与来源:`~/.local/share/unisacc/research/cdx/ci-triage-106.md`、`research/ccogweco/loss-guards-107.md`、`.github/workflows/candidate-native.yml`、`tools/release-exec.cx:1119-1276`、以及 `0.0.106` 真实 CI 证据 `release/evidence/0.0.106/ci-45442318936.{json,log}`(首轮 `fetch` 瞬态 404 漏 `slice-win-arm64`)与 `ci-45446325698.{json,log}`(同候选重派发 `42/42 QUALIFIED`)。本页只读预演,不触发 CI。

---

## (1) 起链前即可静态核实的四项 CI 前置现状

(1) `candidate-native.yml` 的 `fetch` 任务对草稿资产的依赖与软失败盲区(`.github/workflows/candidate-native.yml:47-97`):
- 第 `67` 行:`gh release download "$RELEASE_TAG" -p 'unisacc.com' --repo "$GITHUB_REPOSITORY" --dir candidate` 为硬失败(`set -eu`),并在第 `75-76` 行硬校验 `sha256sum candidate/unisacc.com == $APE_SHA`。
- 第 `81-82` 行:`gh release download "$RELEASE_TAG" -p 'slice-win-arm64' ... || echo "::warning::release has no slice-win-arm64 asset"` 与 `-p 'pkg.bin' ... || echo "::warning::..."` **仍为软失败吞错**;第 `90-97` 行上传 `slice-win-arm64` workflow artifact 时设了 `if-no-files-found: ignore`,且第 `223-228` 行 `windows-11-arm` 下载该 artifact 时设了 `continue-on-error: true`。
- 结论:新建草稿并上传资产后若立即派发 CI(106 间隔仅 `15 秒`),GitHub Release API 最终一致性偶发返回 `release not found`(404),被第 `81` 行吞掉后会导致 `slice-win-arm64` artifact 只含 `pkg.bin` 不含 `slice-win-arm64`,进而在 `windows-11-arm` 格报 `win/arm64 slice asset missing` 判红。**必须在 Step 2 草稿创建后经挡板五实测独立下载 `slice-win-arm64` 校验 SHA256 相等后再进 Step 3**。

(2) 六格对 artifact 的期望与 `42/42` 总判据(`.github/workflows/candidate-native.yml:188-265,694-745`):
- 前置三个 Linux 任务(`ubuntu-24.04`):
  - `fetch`:产出 `candidate-ape`(`candidate/unisacc.com`)与 `slice-win-arm64`(`slice/slice-win-arm64` + `slice/pkg.bin`);
  - `ffi-lnx`:检出 `inputs.commit`,用候选 APE 编译并运行 `tests/cases/win-ffi-{strdup,pread}.c`,产出 `ffi-ref`(`ffi-ref/*.out`);
  - `cell-tools`:检出 `inputs.commit`,调 `sh ./unisacc.com tools/ci-candidate-check.cx -- --release-version "$PWD"` 输出 `RELEASE_VERSION`(`0.0.107`),并用候选 APE 交叉编译 `cell-tools/candidate-qualify-osx-{x86_64,arm64}` 与六目标 `cell-tools/surf-probe-{lnx,osx,win}-{x86_64,arm64}[.exe]`。
- 六个原生目标格(`candidate` 矩阵):
  - `ubuntu-24.04`(`lnx/x86_64`, `native_slice=yes`)、`ubuntu-24.04-arm`(`lnx/arm64`, `native_slice=yes`):直接跑 `sh ./unisacc.com tests/candidate-qualify.cx -- cell "$TARGET" "$APE_SHA" "$COMMIT" "$EVIDENCE"` 加 `surf-probe`;
  - `macos-15-intel`(`osx/x86_64`, `native_slice=yes`)、`macos-15`(`osx/arm64`, `native_slice=yes`):跑 `cell-tools/candidate-qualify-osx-*` 原生二进制(含 `CLOCK_MONOTONIC`、`50ms nanosleep`、`libc99posix.tsv` 同源事实)加 `surf-probe`;
  - `windows-2025`(`win/x86_64`, `native_slice=yes`):下载 `candidate-ape`、`cell-tools`、`ffi-ref`,由 PowerShell 适配器执行 7 项(`identity,version,compile,run,stream,smoke,ffi`)加 `surf-probe-win-x86_64.exe`;
  - `windows-11-arm`(`win/arm64`, `native_slice=file`):下载 `candidate-ape`、`cell-tools`、**`slice-win-arm64`**(`slice/slice-win-arm64` + `slice/pkg.bin`),以原生 ARM64 PE 切片运行 `version,compile,run,stream,smoke` 加 `surf-probe-win-arm64.exe`。
- `report` 任务总判据:汇总六格 `evidence/*.tsv`(共 `6 * 7 = 42` 项)与六格 `surf-*.tsv`,日志必须打出 `CANDIDATE_QUALIFY	verdict=QUALIFIED	pass=42	fail=0	not_run=0	invalid=0`。

(3) `ape_sha256`、`commit`、`release_tag`、`workflow_ref` 四项输入绑定(`tools/release-exec.cx:1221-1222`):
- `release-exec.cx` 派发命令固定为:
  `gh -R partnernetsoftware/unisacc workflow run candidate-native.yml --ref <cand.workflow_ref> -f release_tag=<cand.tag> -f ape_sha256=<cand.sha[0]> -f commit=<xsha>`
- 其中 `cand.tag`=`v0.0.107`、`cand.version`=`0.0.107`、`cand.workflow`=`candidate-native.yml`、`cand.workflow_ref`=`v0.0.107`(取自 `release/candidate.tsv` 第 2、4、5、6 列),`cand.sha[0]` 为候选 `unisacc.com` 的 64 位小写 SHA256(第 9 列),`xsha` 为 Step 1 已推送到远端的 `X` 提交 40 位 SHA。

(4) `v0.0.107` 分支上 `candidate-native.yml` 与 `release-exec.cx` 的版本状态(2026-11-25 02:15 实查):
- `git -C ~/repos/unisacc diff main..v0.0.107 -- .github/workflows/candidate-native.yml` 输出为空(最近提交为 `0090ca3614` "修复 candidate-native.yml SURF 探针在 macOS 缺 timeout 命令导致退出 127"),即 workflow 本身在 `v0.0.107` 无改动。
- `v0.0.107` 的 `tools/release-exec.cx` 已合入 `#4` `5b4f77f33e`(`r39c-release-exec-sha.patch`,修复长 `TMPDIR` 路径下 `sha_file` 200 字节缓冲截断);`#14 v2`(`readback-prewarm-gate-v2.patch`, `sha256=303b0703...`,含 Step 5 公开前 `readback_prewarm` 二次确认、Step 6 `cur_step < from_step` 免下载读回、`exec-6-rollback` 自动写 `INDEX.tsv`、以及脱敏 `warm-<key>.tsv` 识别与补写)已通过 `agy-flow` 独立复核,若在 `02:35` 前合入 `v0.0.107`,则 `0.0.107` 发布直接启用 `READBACK_PREWARM` 门。

---

## (2) CI 起跑前五条只读核对命令原行与期望输出

### 核对一(起链前 / 阶段 C):核车树 workflow、候选槽与 release-exec 版本状态

```sh
W=$HOME/repos/unisacc/.claude/worktrees/v0.0.107
cd "$W"
awk -F'\t' 'NR==2 {printf "tag=%s version=%s wf=%s ref=%s\n", $2, $4, $5, $6}' release/candidate.tsv
git log -1 --oneline -- .github/workflows/candidate-native.yml
git log -3 --oneline -- tools/release-exec.cx
```

- **期望原行**:
  - `tag=v0.0.107 version=0.0.107 wf=candidate-native.yml ref=v0.0.107`
  - `0090ca3614 修复 candidate-native.yml SURF 探针在 macOS 缺 timeout 命令导致退出 127`
  - `tools/release-exec.cx` 含 `5b4f77f33e v0.0.107: release-exec long path sha buffer fix (r39c)`(及 `#14 v2` 合入提交)。

### 核对二(打包 `--pack` 后 / Step 1 `X` 前):核候选表七哈希、布局六资产与 `ci-candidate-check` 版本号

```sh
cd "$W"
APE="$LAYOUT/unisacc.com"
APE_SHA=$(awk -F'\t' 'NR==2 {print $9}' release/candidate.tsv)
SLICE_SHA=$(awk -F'\t' 'NR==2 {print $13}' release/candidate.tsv)
PKG_SHA=$(awk -F'\t' 'NR==2 {print $11}' release/candidate.tsv)
printf '%s\n' "$APE_SHA" | grep -Eqx '[0-9a-f]{64}' && echo "APE_SHA_FORMAT=OK ($APE_SHA)"
(cd "$LAYOUT" && sha256sum -c SHA256SUMS)
test "$(sha256sum "$LAYOUT/unisacc.com" | awk '{print $1}')" = "$APE_SHA" && echo "LAYOUT_APE_MATCH=OK"
test "$(sha256sum "$LAYOUT/slice-win-arm64" | awk '{print $1}')" = "$SLICE_SHA" && echo "LAYOUT_SLICE_MATCH=OK"
test "$(sha256sum "$LAYOUT/pkg.bin" | awk '{print $1}')" = "$PKG_SHA" && echo "LAYOUT_PKG_MATCH=OK"
timeout 55 sh "$APE" tools/ci-candidate-check.cx -- --release-version "$W"
```

- **期望原行**:
  - `APE_SHA_FORMAT=OK (<64位小写hex>)`
  - `unisacc.com: OK`、`pkg.bin: OK`、`slice-win-arm64: OK`、`unisacc.com.build.json: OK`、`build.json: OK`
  - `LAYOUT_APE_MATCH=OK`、`LAYOUT_SLICE_MATCH=OK`、`LAYOUT_PKG_MATCH=OK`
  - 末行输出 `0.0.107`(与 `candidate-native.yml:158` `cell-tools` 读取方式完全一致)。

### 核对三(打包 `--pack` 后 / Step 1 `X` 前):本机只读预演 CI 的 `cell-tools` 与 `ffi-lnx` 交叉编译

```sh
cd "$W"
APE="$LAYOUT/unisacc.com"
CI_DRY=$(mktemp -d /tmp/ci-dryrun-107-XXXXXXXX)
mkdir -p "$CI_DRY/tmp" "$CI_DRY/out"
export TMPDIR="$CI_DRY/tmp"

# 第 1 批(<= 45s):win-ffi 两个基准程序编译运行 + macOS 两目标 candidate-qualify(双路并行 ~21s)
timeout 30 sh "$APE" -b lnx/x86_64 -o "$CI_DRY/out/win-ffi-strdup" tests/cases/win-ffi-strdup.c
"$CI_DRY/out/win-ffi-strdup" > "$CI_DRY/out/strdup.out"
timeout 30 sh "$APE" -b lnx/x86_64 -o "$CI_DRY/out/win-ffi-pread" tests/cases/win-ffi-pread.c
"$CI_DRY/out/win-ffi-pread" > "$CI_DRY/out/pread.out"
for t in osx/x86_64 osx/arm64; do
  timeout 55 sh "$APE" -b "$t" -o "$CI_DRY/out/candidate-qualify-${t%/*}-${t#*/}" tests/candidate-qualify.cx &
done
wait

# 第 2 批(<= 45s):六目标 surf-probe 交叉编译(可串行 ~42s 或并行 ~12s)
for t in lnx/x86_64 lnx/arm64 osx/x86_64 osx/arm64 win/x86_64 win/arm64; do
  ext=""
  case "$t" in win/*) ext=".exe" ;; esac
  timeout 45 sh "$APE" -b "$t" -o "$CI_DRY/out/surf-probe-${t%/*}-${t#*/}${ext}" tools/surf-probe.cx
done
ls -la "$CI_DRY/out"
rm -rf "$CI_DRY"
```

- **期望原行**:10 个目标二进制全部 `rc=0` 生成且非空(提前在本机证伪 `ffi-lnx` 与 `cell-tools` 任务的一切编译期故障)。
- **106 APE 预演实测回执(106 APE 预演,107 候选到后重跑一次)**:
  - 预演输入:0.0.106 公开 APE `/tmp/v106-verify/unisacc.com`(`sha256=5f83ac93d201b252ea0b56098475be9e30045875cdf1ce692451cda095ceeb3c`),私有导出树 `v0.0.107`(`REV=e2f0539e2ae0a9c5d6787c7a2e75c25a5d36c74a`),全新隔离 `TMPDIR=/tmp/ci-dryrun-107-priv/tmp`(无预热冷跑)。
  - 结论与时限发现:
    - `(1)` **无需预热或特定 `TMPDIR`**:在全新空 `TMPDIR` 下 10 个目标二进制 + `ci-candidate-check --release-version` 全部一次 `rc=0`;
    - `(2)` **单命令耗时均远低于 60 秒**(`0.55s -> 20.70s`),但 **10 个二进制完全串行总耗时为 `88464 ms`(`88.5 秒`)**,若合在单个 60 秒外层 shell 调用里串行跑会触 60 秒外限!因此按上方命令拆为两批(或对两个 `candidate-qualify-osx-*` 并行 `wait`),每批墙钟均稳定在 `22-42 秒` 内(`<= 60 秒`)。
  - 实测逐项原行与耗时:

```text
5f83ac93d201b252ea0b56098475be9e30045875cdf1ce692451cda095ceeb3c  /tmp/v106-verify/unisacc.com
V0107_HEAD=e2f0539e2ae0a9c5d6787c7a2e75c25a5d36c74a
CI_CANDIDATE_CHECK version=0.0.107 rc=0 ms=3647
FFI_LNX target=lnx/x86_64 bin=win-ffi-strdup compile_rc=0 run_rc=0 bytes=41068 out_bytes=3 ms=549
FFI_LNX target=lnx/x86_64 bin=win-ffi-pread compile_rc=0 run_rc=0 bytes=45269 out_bytes=4 ms=692
CELL_QUALIFY target=osx/x86_64 bin=candidate-qualify-osx-x86_64 rc=0 bytes=710242 ms=20701
CELL_QUALIFY target=osx/arm64 bin=candidate-qualify-osx-arm64 rc=0 bytes=776290 ms=20644
SURF_PROBE target=lnx/x86_64 bin=surf-probe-lnx-x86_64 rc=0 bytes=354045 ms=5530
SURF_PROBE target=lnx/arm64 bin=surf-probe-lnx-arm64 rc=0 bytes=386813 ms=6771
SURF_PROBE target=osx/x86_64 bin=surf-probe-osx-x86_64 rc=0 bytes=380002 ms=7421
SURF_PROBE target=osx/arm64 bin=surf-probe-osx-arm64 rc=0 bytes=413026 ms=7375
SURF_PROBE target=win/x86_64 bin=surf-probe-win-x86_64.exe rc=0 bytes=407552 ms=7203
SURF_PROBE target=win/arm64 bin=surf-probe-win-arm64.exe rc=0 bytes=437248 ms=7866
TOTAL_10_BINARIES_SERIAL_MS=88464
```

### 核对四(Step 1 `X` 提交后 / Step 2 草稿前):核 `X` 提交远端可达性与 CI 检出文件完整性

```sh
cd "$W"
X_SHA=$(git rev-parse HEAD)
git fetch --quiet origin
git merge-base --is-ancestor "$X_SHA" origin/v0.0.107 && echo "X_ON_BRANCH=OK ($X_SHA)"
git merge-base --is-ancestor "$X_SHA" origin/main && echo "X_ON_MAIN=OK ($X_SHA)"
git cat-file -e "$X_SHA:tests/candidate-qualify.cx" && \
git cat-file -e "$X_SHA:tests/cases/darwin-monotonic.c" && \
git cat-file -e "$X_SHA:tools/surf-probe.cx" && \
git cat-file -e "$X_SHA:.github/workflows/candidate-native.yml" && echo "X_CI_TREE_FILES=OK"
```

- **期望原行**:`X_ON_BRANCH=OK`、`X_ON_MAIN=OK`、`X_CI_TREE_FILES=OK`(确保 `--ref v0.0.107 -f commit=$X_SHA` 在远端所有 runner 上 `actions/checkout` 可达)。

### 核对五(Step 2 草稿上传后 / Step 3 派 CI 前,`loss-guards-107.md` 损耗一必过挡板):核草稿六资产与 `slice-win-arm64` 独立下载

```sh
cd "$W"
TAG=v0.0.107
SLICE_SHA=$(awk -F'\t' 'NR==2 {print $13}' release/candidate.tsv)
PKG_SHA=$(awk -F'\t' 'NR==2 {print $11}' release/candidate.tsv)
gh -R partnernetsoftware/unisacc release view "$TAG" --json isDraft,assets \
  --jq '{isDraft:.isDraft,count:(.assets|length),names:[.assets[].name]|sort}'
rm -rf /tmp/guard-107 && mkdir -p /tmp/guard-107
gh -R partnernetsoftware/unisacc release download "$TAG" -p slice-win-arm64 -p pkg.bin -D /tmp/guard-107 --clobber
test "$(sha256sum /tmp/guard-107/slice-win-arm64 | awk '{print $1}')" = "$SLICE_SHA" && \
test "$(sha256sum /tmp/guard-107/pkg.bin | awk '{print $1}')" = "$PKG_SHA" && \
echo "GUARD_107_DRAFT_SLICE_READY=PASS"
rm -rf /tmp/guard-107
```

- **期望原行**:
  - `{"count":6,"isDraft":true,"names":["SHA256SUMS","build.json","pkg.bin","slice-win-arm64","unisacc.com","unisacc.com.build.json"]}`
  - `GUARD_107_DRAFT_SLICE_READY=PASS`(若首次下载报 `release not found` 404,等待 15-30 秒重试一次至 `PASS` 后再进 Step 3 派发 CI;若仍 404 则停手查草稿上传,绝不带 404 盲派 CI)。

---

## (3) CI 红时按四类失败形态的分诊与处置表(基于 0.0.106 两次真实 run 日志原行)

| 失败类别 | 发生任务 / 步骤 | `0.0.106` 真实日志特征原行(或 workflow 对应报错行) | 根因判定(瞬态 vs 候选缺陷) | 最小处置命令与路径(`G4 redispatch` vs `retry-ci` 替换候选) |
|---|---|---|---|---|
| **类别 1:`fetch` 瞬态 404 或漏 `slice-win-arm64` / `pkg.bin`** | `fetch` 任务 (`:67,:81-82`) -> 连带 `windows-11-arm` (`win/arm64`) 格 `version` 红与 `report` 红 | 1) `fetch` 日志 (`ci-45442318936.log`, job `138616555184`):<br>`release not found`<br>`::warning::release has no slice-win-arm64 asset`<br>`Artifact slice-win-arm64.zip ... 1 file uploaded` (`2908129B` 仅 `pkg.bin`,缺 `4662272B` `slice-win-arm64`)<br>2) `windows-11-arm` 格日志 (`job 138617412444`):<br>`win/arm64	identity	PASS	0 ...`<br>`win/arm64	version	FAIL	1	UNMEASURED ... win/arm64 slice asset missing (release_tag needs a slice-win-arm64 asset)`<br>3) `report` 汇总行 (`ci-45442318936.log:17:09:11Z`):<br>`CANDIDATE_QUALIFY	verdict=FAIL	pass=36	fail=1	not_run=0	invalid=5` | **GitHub 最终一致性瞬态**(`fetch` 在草稿新建后数秒内调 `gh release download` 撞 404 被 `:81` 软失败吞掉;其余五格 `35/35 PASS` 且 `win/arm64` `identity PASS`,非 ARM64 原生代码缺陷) | **走同候选显式 `G4 redispatch` 一次**(严禁在 run 未结束时取消,严禁调 `gh run rerun --failed`):<br>1) 等整轮 run 自然结束为 `completed/failure`;<br>2) 跑核对五确认 `gh release download v0.0.107 -p slice-win-arm64` 已 `rc=0` 且 SHA256 匹配;<br>3) 执行一次显式重派发(保留旧 run 记录):<br>`DEVSVC_OWNER=cc sh ./unisacc.com tools/release-exec.cx -- --execute release/candidate.tsv --layout "$LAYOUT" --strict "$STRICT" --out "$S/exec" --from-step 3 --redispatch`<br>4) 核 `INDEX.tsv` 写入 `exec-3b-ci-dispatch-2` (`supersedes=<OLD>`)与 `exec-3-ci-prior` (`FAIL <OLD> superseded-by=<NEW>`)。 |
| **类别 2:格内 `download-artifact` 或 `actions/checkout` 传输失败 / runner 瞬态失联** | 任一格的第 2–5 步 (`actions/checkout` 或 `actions/download-artifact`) | 1) `download-artifact` 报 `Error: Unable to download artifact(s): Artifact not found` 或网络 `ECONNRESET` / `503`;<br>2) 或第 `240` / `324` 行宿主传输校验报 `::error::host hash <got>, expected <APE_SHA>`(而 `fetch` 第 `76` 行已通过同一 `APE_SHA`);<br>3) 此时尚未进入 `candidate-qualify` 格内执行,无 `TARGET	check	FAIL` 业务行。 | **GitHub Actions Runner / Artifact 传输瞬态**(前提:`fetch` 第 `76` 行已证实草稿 APE SHA256 正确,且 `X` 提交在远端可读) | 1) 等整轮 run 自然结束为 `completed/failure`;<br>2) 核对四与核对五确认远端 `X` 提交与草稿六资产完好;<br>3) 报 `cc` 批准后执行一次同候选 `--from-step 3 --redispatch`;若同一错误连续第二次出现则停手转 `cc`/`fable` 定因,不连点。 |
| **类别 3:`cell-tools` 或 `ffi-lnx` 编译失败,或格内 `compile` 步失败** | `cell-tools` (`:158,:171,:178`)、`ffi-lnx` (`:122-123`) 或格内 `<target> compile FAIL` | 1) `cell-tools` 报 `timeout 120 sh ./unisacc.com -b osx/... -o ... tests/candidate-qualify.cx` 非零退出,或 `surf-probe.cx` 交叉编译非零;<br>2) `ffi-lnx` 报 `win-ffi-strdup.c` / `win-ffi-pread.c` 编译或运行非零;<br>3) 格内日志出现 `<os>/<arch>	compile	FAIL	<rc> ...`。 | **确定性候选代码或测试树缺陷**(不是瞬态;若已跑过起跑前“核对三”,此类错误在派 CI 前就会在本机被拦截) | **禁止 `--redispatch` 重试**。立即停手保留 `ci-<RUN>.json/.log` 原行报 `cc`:<br>1) 若属候选编译器缺陷(A 类):由 `cc` 裁定执行替换候选回收草稿:<br>`sh ./unisacc.com tools/release-chain.cx -- retry-ci --version 0.0.107 --run-id "$RUN" --evidence-dir release/evidence/0.0.107 --candidate release/candidate.tsv --dry`(获批后去掉 `--dry`),修源后重起链;<br>2) 若属 `tests/candidate-qualify.cx` 仅测试脚本缺陷(不触 `SOURCES_SID`):由 `cc` 裁定是否走 `retry-ci` 更新 `X` 而不重起链。 |
| **类别 4:格内 `version` / `run` / `stream` / `smoke` / `ffi` / `SURF` 原生行为失败** | 六格之一的“运行原生格” / “Run the cell (Windows adapter)” / “Run L0 SURF probe” | 1) POSIX 格 (`:245-250`):`<os>/<arch>	<version|run|stream|smoke>	FAIL	<rc> ...`(例如 macOS `darwin-monotonic.c` `CLOCK_MONOTONIC` / `libc99posix.tsv` 绑定失败,或 `lnx/arm64` `-run` 失败);<br>2) Windows 格 (`:330-465`):`win/x86_64	ffi	FAIL` (`win-ffi-strdup.out` / `pread.out` 与 `ffi-ref` 字节不等)或 `win/arm64` 原生 slice 运行 `rc != 0`;<br>3) SURF 探针 (`:252-264`):`surf-probe` 返回非零或 `report` 任务 (`:715-742`) 汇总报 `SURF` 失败。 | **确定性跨平台原生产品缺陷**(`candidate-qualify` 已实际启动候选 APE 或 `slice-win-arm64` 并观测到错误退出码或错误输出) | **绝对禁止 `--redispatch` 刷绿或放宽断言**。立即停手:<br>1) 提取对应格的 `evidence-<os>-<arch>`(Windows 另取 `diagnostics-win-<arch>`)完整原行报 `cc` 与 `cdx`;<br>2) 保持草稿 `v0.0.107` 私有(绝不进 Step 4 `Y1` 或 Step 5 `publish`);<br>3) 按 `release-exec.cx:87-88` 的 `NEXT` 行执行 `release-chain.cx -- retry-ci` 归档失败 CI 证据并转修复候选。 |

---

## (4) 0.0.106 成功基准对照行(0.0.107 Step 3 通过唯一放行标准)

当 `0.0.107` 的 CI run 完成时,必须同时满足以下三条原行(`tools/release-exec.cx:1120-1148,1216-1217`),缺一不可进入 Step 4 `Y1`:

1. `report` 任务日志原行(对照 `ci-45446325698.log:17:29:46Z`):
   `CANDIDATE_QUALIFY	verdict=QUALIFIED	pass=42	fail=0	not_run=0	invalid=0`
2. `release-exec` Step 3 门禁输出原行:
   `GATE	3	CI_SUCCESS	PASS	run success, six candidate targets success`
   `GATE	3	CI_VERDICT	PASS	verdict=QUALIFIED fail=0 invalid=0`
   `INDEX	exec-3-ci	<YYYY-MM-DD HH:MM:SS +0800>	PASS	<CI_RUN_ID>	six targets success, CANDIDATE_QUALIFY no fail/invalid`
3. 证据文件落盘且无绝对家目录路径:
   `release/evidence/0.0.107/ci-<CI_RUN_ID>.json` 与 `release/evidence/0.0.107/ci-<CI_RUN_ID>.log` 均已落盘,且 `grep -c '/home/' release/evidence/0.0.107/ci-<CI_RUN_ID>.*` 均为 `0`。
