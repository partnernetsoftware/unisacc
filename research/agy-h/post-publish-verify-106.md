# v0.0.106 公开后即时只读核验清单

目标版本: `0.0.106` (tag `v0.0.106`, 仓 `partnernetsoftware/unisacc`)
冻结链提交 (`targetCommitish`): `2e8a19a342251910d53010f51002b14a4ca7f916` (短写 `2e8a19a342`)
候选真源: `2e8a19a342:release/candidate.tsv` 与 `.claude/worktrees/v0.0.106/release/evidence/0.0.106/`
说明: 公开 (`exec-5-publish`) 后立即可逐条运行的本地只读命令, 单条均可在 60 秒内完成; 不改仓库工作区, 不写 `out/`。

---

## 1. Release 元数据、`isLatest` 切换与 Tag 目标提交核验

### (1) `v0.0.106` 公开态、`isLatest` 与 `targetCommitish` 核验

命令原行:

```sh
gh release view v0.0.106 -R partnernetsoftware/unisacc \
  --json tagName,isDraft,isPrerelease,isLatest,targetCommitish
```

期望原行 (JSON 字段与取值):

- `"tagName": "v0.0.106"`
- `"isDraft": false`
- `"isPrerelease": false`
- `"isLatest": true`
- `"targetCommitish": "2e8a19a342251910d53010f51002b14a4ca7f916"`

精确判定命令原行 (含 jq 断言, 非匹配退出码非零):

```sh
gh release view v0.0.106 -R partnernetsoftware/unisacc \
  --json tagName,isDraft,isPrerelease,isLatest,targetCommitish \
  --jq 'if .tagName=="v0.0.106" and .isDraft==false and .isPrerelease==false and .isLatest==true and .targetCommitish=="2e8a19a342251910d53010f51002b14a4ca7f916" then . else error("v0.0.106 metadata mismatch") end'
```

源码依据与说明:

- `tools/release-exec.cx:1087` (`release_state`) 仅查 `isDraft` (`--json isDraft --jq .isDraft`), 返回 `"true"` 为 `1`, `"false"` 为 `2` (`:1091-1092`)。
- `release/tools/c47-1-readback.cx:148-149` 仅查 `tagName,isDraft,isPrerelease,targetCommitish`, 本清单补充 `.isLatest==true` 补齐人工核验。

### (2) `v0.0.106` 为 `Latest` 与 `v0.0.105` 取消 `Latest` 的关系

命令原行:

```sh
gh release view v0.0.105 -R partnernetsoftware/unisacc \
  --json tagName,isDraft,isLatest
```

期望原行:

- `"tagName": "v0.0.105"`
- `"isDraft": false`
- `"isLatest": false`

关系说明 (源码字面量加行号):

- 发布推进: `tools/release-exec.cx:1422` 在 `step_publish` 执行 `gh -R partnernetsoftware/unisacc release edit v0.0.106 --draft=false --latest`。GitHub Releases 单仓只允许一个 `Latest` release, 因此 `v0.0.106` 带 `--latest` 公开后自动成为 `isLatest=true`, 原 `Latest` (`v0.0.105`) 自动降为 `isLatest=false` (仍保持 `isDraft=false`)。
- 发布日志行: `tools/release-exec.cx:1434` 写入 `INDEX.tsv` 行 `exec-5-publish\t<ts>\t-\tv0.0.106\tisDraft=false, Latest`。
- 回滚关系: `tools/release-exec.cx:1813` 计算 `prevtag` 为 `v0.0.105`, 若 step 5 后失败回滚 (`:292-293, :298`), 会先后执行 `gh -R partnernetsoftware/unisacc release edit v0.0.106 --draft=true` 与 `gh -R partnernetsoftware/unisacc release edit v0.0.105 --latest`, 把 `Latest` 恢复给 `v0.0.105`。

### (3) Git Tag `v0.0.106` 目标 SHA 核验

命令原行:

```sh
gh api repos/partnernetsoftware/unisacc/git/ref/tags/v0.0.106 --jq .object.sha
```

与本地轻量/附注标签一致性核验:

```sh
git fetch origin tag v0.0.106 --no-tags && git rev-parse 'refs/tags/v0.0.106^{commit}'
```

期望原行:

```text
2e8a19a342251910d53010f51002b14a4ca7f916
```

源码依据:

- `release/tools/c47-1-readback.cx:152` 调用 `gh api repos/<repo>/git/ref/tags/<tag> --jq .object.sha`, 校验返回 `41` 字节且前 `40` 位等于 `candidate.commit` (`2e8a19a342251910d53010f51002b14a4ca7f916`) 加末尾 `\n`。
- `tools/release-chain.cx:1074-1080` (`cmd_mark_version_seq`) 使用 `git -C <repo> rev-parse --verify --end-of-options refs/tags/v0.0.106^{commit}` 核对 tag 提交可达。

---

## 2. 六件资产下载与 SHA-256 核验 (`/tmp/v106-verify`)

说明: `2e8a19a342:release/candidate.tsv` 第 17-18 列 `zip` / `zip_sha256` 为 `-` (`tools/cxlib/releasecandidate.cx:48` `rcand_skip(&c, 5)` 跳过 `zip`), 实际随 Release 上传与校验的资产共 6 件 (`SHA256SUMS` 文件本身列举前 5 件并自成第 6 件资产)。

### (1) 命令原行 (≤60 秒局部只读核验)

```sh
rm -rf /tmp/v106-verify && mkdir -p /tmp/v106-verify && \
gh release download v0.0.106 -R partnernetsoftware/unisacc -D /tmp/v106-verify --clobber && \
(cd /tmp/v106-verify && ls -1 && sha256sum -c SHA256SUMS && \
 sha256sum -- unisacc.com pkg.bin slice-win-arm64 unisacc.com.build.json build.json SHA256SUMS)
```

### (2) `sha256sum -c SHA256SUMS` 期望原行 (对照 `0.0.106` `draft-readback.txt:2-6` 与 `0.0.105` `public-readback.txt:44-49`)

```text
unisacc.com: OK
pkg.bin: OK
slice-win-arm64: OK
unisacc.com.build.json: OK
build.json: OK
```

### (3) 六件资产全量 SHA-256 期望原行 (对照 `.claude/worktrees/v0.0.106/release/candidate.tsv:2`)

```text
5f83ac93d201b252ea0b56098475be9e30045875cdf1ce692451cda095ceeb3c  unisacc.com
b49d838cf576379cfa5527f5ae7fdaba3a4294a4da21cd20b5a7cc2f5b9a870b  pkg.bin
1ca2eaa10bafc2c4f76b736796fa4b7cf20b2d5709ae5484141cb7108e97569f  slice-win-arm64
1dc2fed3b15cc02349f30ec18dad1982533f7a358c1bf6f1578b036d829601c3  unisacc.com.build.json
1dc2fed3b15cc02349f30ec18dad1982533f7a358c1bf6f1578b036d829601c3  build.json
895ad6b492fff6a75dca48c1eecbdad97c289010050050c718744fff2c074a38  SHA256SUMS
```

补注: `tools/release-exec.cx:1456-1465` 在下载后先核 `unisacc.com` 等于 `cand.sha[0]`, 再检查 `SHA256SUMS` 包含 `5f83ac93d201b252ea0b56098475be9e30045875cdf1ce692451cda095ceeb3c  unisacc.com` 这一整行; `release/tools/c47-1-readback.cx:155-159` 在 `asset-sha` 步逐字节比对前 4 件 (`unisacc.com`, `pkg.bin`, `slice-win-arm64`, `unisacc.com.build.json`), 并在 `checksums` 步 (`:179`) 用 `sha256sum --check SHA256SUMS` 校验含 `build.json` 在内的全部 5 个文件。

---

## 3. `public-readback.txt` 期望格式 (对照 `0.0.105` 与 `c47-1-readback.cx` / `release-exec.cx`)

路径: `.claude/worktrees/v0.0.106/release/evidence/0.0.106/public-readback.txt` (集成并提交 `Y2` 后落在 `release/evidence/0.0.106/public-readback.txt`)

### (1) 按行结构模板 (对照 `release/evidence/0.0.105/public-readback.txt:1-52`)

1. 首行 (`tools/release-exec.cx:1507`):

```text
# c47-1-readback (public) YYYY-MM-DD HH:MM:SS +0800 via release-exec; rc=0
```

2. 第 1 步 `metadata` (`c47-1-readback.cx:148-149`, `0.0.105/public-readback.txt:2-3`):

```text
{"isDraft":false,"isPrerelease":false,"tagName":"v0.0.106","targetCommitish":"2e8a19a342251910d53010f51002b14a4ca7f916"}
READBACK_STEP metadata PASS
```

3. 第 2 步 `tag` (`c47-1-readback.cx:152`, `0.0.105/public-readback.txt:4-5`):

```text
2e8a19a342251910d53010f51002b14a4ca7f916
READBACK_STEP tag PASS
```

4. 第 3 步 `download` (`c47-1-readback.cx:155-156`, `0.0.105/public-readback.txt:6`):

```text
READBACK_STEP download PASS
```

5. 第 4 步 `asset-sha` (`c47-1-readback.cx:159`, `0.0.105/public-readback.txt:7-15`):

```text
5f83ac93d201b252ea0b56098475be9e30045875cdf1ce692451cda095ceeb3c  unisacc.com
b49d838cf576379cfa5527f5ae7fdaba3a4294a4da21cd20b5a7cc2f5b9a870b  pkg.bin
1ca2eaa10bafc2c4f76b736796fa4b7cf20b2d5709ae5484141cb7108e97569f  slice-win-arm64
1dc2fed3b15cc02349f30ec18dad1982533f7a358c1bf6f1578b036d829601c3  unisacc.com.build.json
READBACK_STEP asset-sha PASS
READBACK_SHA unisacc.com 5f83ac93d201b252ea0b56098475be9e30045875cdf1ce692451cda095ceeb3c
READBACK_SHA pkg.bin b49d838cf576379cfa5527f5ae7fdaba3a4294a4da21cd20b5a7cc2f5b9a870b
READBACK_SHA slice-win-arm64 1ca2eaa10bafc2c4f76b736796fa4b7cf20b2d5709ae5484141cb7108e97569f
READBACK_SHA unisacc.com.build.json 1dc2fed3b15cc02349f30ec18dad1982533f7a358c1bf6f1578b036d829601c3
```

6. 第 5 步 `version` (`c47-1-readback.cx:163`, `0.0.105/public-readback.txt:16-17`):

```text
unisacc 0.0.106
READBACK_STEP version PASS
```

7. 第 6-10 步 (`smoke`, `comm`, `layout-generate`, `layout-check`, `checksums`, `0.0.105/public-readback.txt:18-49`):

- `smoke`: 5 条 `PASS	release-ape.smoke.{candidate-version,run-c-exit-7,S-prints-tape,cx-host-c-library,cx-tool-srcshape}` + `RELEASE_APE	SMOKE	ok=5	failed=0` + `READBACK_STEP smoke PASS`
- `comm`: `CHECK_RUN_COMM	PASS old-negative-control=63 new-comm=rcprobe.c memfd,args,fd9,stdout,rc=CHECKED` + `READBACK_STEP comm PASS`
- `layout-generate` & `layout-check`: 各有 `RELEASE_LAYOUT	status=PASS` + `READBACK_STEP layout-generate PASS` / `READBACK_STEP layout-check PASS`
- `checksums`: 5 行 `<name>: OK` + `READBACK_STEP checksums PASS`

8. 第 11 步 `zip-members` 与总判行 (`c47-1-readback.cx:182,184`, `0.0.105/public-readback.txt:50-51`):

```text
READBACK_STEP zip-members NOT_APPLICABLE version=0.0.106 no-zip-contract
C47_1_READBACK PASS expected_draft=false tag=v0.0.106 version=0.0.106 commit=2e8a19a342251910d53010f51002b14a4ca7f916 output=~/.cache/unisacc/.../result
```

(注: `output=` 前缀经 `tools/release-exec.cx:1508` `scrub_home_put` 脱敏为 `~/`。)

9. 尾行 (`tools/release-exec.cx:1509`, `0.0.105/public-readback.txt:52`):

```text
READBACK_SHA unisacc.com 5f83ac93d201b252ea0b56098475be9e30045875cdf1ce692451cda095ceeb3c
```

### (2) `evidence_readback_ok("public-readback.txt", 1)` 的四个强制字面量 (`tools/release-exec.cx:1042-1044`)

- 含 `"rc=0"`
- 含 `"\"isDraft\":false"`
- 含 `"READBACK_SHA unisacc.com 5f83ac93d201b252ea0b56098475be9e30045875cdf1ce692451cda095ceeb3c"`
- 不含 `" FAIL"` (前置空格 + `FAIL`, 因此任意一步出现 ` FAIL` 都会判 0 拒绝)

---

## 4. `release/evidence/0.0.106/INDEX.tsv` 与 `release-post` 步骤 5 门读取明细

### (1) `INDEX.tsv` 公开后应新增的四行记录 (对照 `release/evidence/0.0.105/INDEX.tsv:9-13` 与 `release-exec.cx:162-164`)

1. `exec-5-publish` (`tools/release-exec.cx:1434`):

```tsv
exec-5-publish	<YYYY-MM-DD HH:MM:SS +0800>	-	v0.0.106	isDraft=false, Latest
```

2. `exec-6-public-readback` (`tools/release-exec.cx:1514`):

```tsv
exec-6-public-readback	<YYYY-MM-DD HH:MM:SS +0800>	PASS	public-readback.txt	fresh download sha256 equals candidate, c47-1-readback rc 0
```

3. `exec-7-Y2` 与 `exec-7-Y2-done` (`tools/release-exec.cx:1590, 1595`):

```tsv
exec-7-Y2	<YYYY-MM-DD HH:MM:SS +0800>	-	pending	evidence INDEX and decision recomputed equal; Y2 commit follows
exec-7-Y2-done	<YYYY-MM-DD HH:MM:SS +0800>	-	<Y2_COMMIT_40HEX>	Y2 committed; this row rides with the next commit
```

4. `post-publish` (由 `release-post` 步骤 2 -> `release-chain.cx mark-version-seq` 在密封 `INDEX.tsv` 时写入, 对照 `0.0.105/INDEX.tsv:13`):

```tsv
post-publish	<YYYY-MM-DD HH:MM:SS +0800>	PASS	build/version-seq.tsv	version=0.0.106 marked 已公开 (Y2=<Y2_COMMIT_40HEX>)
```

### (2) 各工具具体读 `INDEX.tsv` 的哪几行

1. `tools/release-post.cx` 步骤 1 (`step_index_seal`, `:113-125`):
- 读 `release/evidence/0.0.106/INDEX.tsv` (`:115`)。
- 检查 `has_line_prefix(b, "exec-7-Y2-done")` (`:119`):
  - 通过期望原行 (`:123`): `POST_STEP	1	index-seal	OK	INDEX-has-Y2-done`
  - 缺失失败原行 (`:120`): `POST_STEP	1	index-seal	FAIL	missing-exec-7-Y2-done`

2. `tools/release-post.cx` 步骤 2 (`step_mark_version_seq`, `:127-140`) -> `tools/release-chain.cx mark-version-seq` (`:1086-1105`):
- 读 `exec-5-publish` (`has_publish_pass_row`, `tools/release-chain.cx:807-817`): 要求首列为 `exec-5-publish`, 且同一行含 `\tPASS\t`, 或同时含 `\t-\t` 与 `isDraft=false` (不满足报 `REFUSE VERSION_SEQ_MARK missing-publish-index`, `:1092`)。
- 读 `exec-7-Y2-done` 与 `post-publish` (`check_index_y2_row`, `tools/release-chain.cx:835-862`): 按 `\t` 切最多 6 列, 对首列 `exec-7-Y2-done` 校验第 4 列 (`col 3`, 0-based) 严格等于 `git log -1 --format=%H --grep "^v0.0.106 Y2: public readback$"` 查到的 `y2_sha` (不等报 `REFUSE VERSION_SEQ_MARK y2-sha-mismatch`, `:1102`), 并检测是否已有首列 `post-publish` 行 (`:857-859`)。

3. `tools/release-exec.cx` 自带幂等复跑检查:
- `index_ref("publish", "exec-5-publish", ...)` (`:1430`, 读第 4 列 `col 3` 非空且不为 `-`/`pending`)
- `index_ref("exec-7-Y2-done", 0, ...)` (`:1589`)

### (3) `release-post` 步骤 5 (`root-upgrade`) 与步骤 5 门 (`POST_ROOT_BACKUP_GATE`) 读什么与期望原行

说明: 两个位置均存在 `POST_ROOT_BACKUP_GATE`, 需同时区分:

1. `tools/release-exec.cx:1544-1559` (在 Y2 收尾时作为非阻塞提醒调用):
- 执行 `./unisacc.com tools/install-ape.cx -- . --backup` 与 `tools/install-ape.cx -- . --rollback` (`:1546-1547`)。
- 读回滚干跑输出首行是否含 `ROLLBACK_DRYRUN` (`:1553`)。
- 期望原行 (`:1554`): `POST_ROOT_BACKUP_GATE	PASS	INSTALL_APE	ROLLBACK_DRYRUN	<sha256>`
- 若未备份则仅警告 (`:1556`): `POST_ROOT_BACKUP_GATE	WARN	fix=sh ./unisacc.com tools/install-ape.cx -- . --backup --write	...`

2. `tools/release-post.cx` 步骤 5 (`step_root_upgrade`, `:180-199`) 门禁本身:
- 前置读取文件 (`load_state`, `:76-92`): 若使用 `--execute`, 先读取 `release/evidence/0.0.106/release-post-state.tsv` (`:262`), 解析第 1 列 `'1'..'6'` 且含 `\tDONE` 的状态行 (`:86-88`) 以决定是否跳过已完成前置步; 若未带 `--step 5`, 步骤 5 仅在步骤 1-4 (含步骤 1 读 `INDEX.tsv` 的 `exec-7-Y2-done` 与步骤 2 读 `exec-5-publish`/`exec-7-Y2-done`) 全部成功后才进入。
- 开关检查 (`:182-183`): 仅 `--execute --root-upgrade` 才真正执行步骤 5 门禁; 默认干跑或未带 `--root-upgrade` 输出:
  - `POST_STEP	5	root-upgrade	SKIP	no-root-upgrade-flag` (`:182`)
  - 或 `POST_STEP	5	root-upgrade	SKIP	dry-no-root-upgrade` (`:183`)
- 步骤 5 门命令 (`:184`):

```sh
./tools/bound 55 sh ./unisacc.com tools/install-ape.cx -- . --rollback
```

- 门禁读取的字面量 (`tools/release-post.cx:186` + `tools/install-ape.cx:476,671`):
  - 子命令返回 `rc == 0` 且输出含行前缀 `INSTALL_APE	ROLLBACK_DRYRUN` (`has_line_prefix(out, "INSTALL_APE\tROLLBACK_DRYRUN")`)。
  - 通过期望原行 (`:191`): `POST_ROOT_BACKUP_GATE	PASS`
  - 失败阻塞原行 (`:187-188, :274`):
    - `POST_ROOT_BACKUP_GATE	FAIL`
    - `POST_STEP	5	root-upgrade	FAIL	POST_ROOT_BACKUP_GATE`
    - `RELEASE_POST	status=BLOCKED:root-upgrade`
- 门禁通过后参数校验 (`:192`) 与写本机 Root (`:193-197`): 必须提供非空 `--ape` 与 `--ape-sha`, 执行 `sh ./unisacc.com tools/install-ape.cx -- . <ape> <ape_sha> --write`, 通过后期望原行:
  - `POST_STEP	5	root-upgrade	OK	root-upgraded`

### (4) 公开后快速验证 `release-post` (只读干跑计划) 命令原行

```sh
sh ./unisacc.com tools/release-post.cx -- plan --version 0.0.106
```

```sh
sh ./unisacc.com tools/release-post.cx -- run --version 0.0.106 --dry \
  --evidence-dir .claude/worktrees/v0.0.106/release/evidence/0.0.106
```

(注: 在 `Y2` 未合入 `main` 之前 `--dry` 需指明 `--evidence-dir` 为工作树路径; 一旦 `exec-7-Y2-done` 已写入且已集成进 `main`, 去掉 `--evidence-dir` 直接用默认 `release/evidence/0.0.106`, 且第 1 步 `index-seal` 应转绿为 `POST_STEP	1	index-seal	OK	INDEX-has-Y2-done`。)
