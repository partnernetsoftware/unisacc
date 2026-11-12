# 0.0.58 发布尾部预备手册(链 DONE 后按序执行)

冻结 rev F=2a46e1a4275a616628f3a8965e9a749aa5bdfe40(标签 v0.0.58-src)。链目录 out/b1/F,不得改动。
所有 gh 带 `-R partnernetsoftware/unisacc`。每步 <=60 秒,超过则分片。每步前核对前一步证据。

## 0 预检(已于 2026-11-12 核过)

- gh 已登录(mgttt)。磁盘余量约 120 GB(stat -f)。13 项棘轮全绿。
- 旧草稿 v0.0.58 仍在,含旧候选(9a34 构造)七资产:build.json pkg.bin SHA256SUMS slice-win-arm64 unisacc-0.0.58-win-arm64.zip unisacc.com unisacc.com.build.json。全部过期,必须替换。
- candidate-native.yml 输入:ape_sha256(必填)、commit(必填,40 位)、release_tag(草稿 tag,优先于 ape_url)、ape_url。
- 阻塞(B1):plans/release-notes-0.0.58.md 仍写旧候选(9a34、X=931fa623、CI FAIL、未公开),整篇须按新候选重写,见第 9 步。

## 1 链状态

```
sh ./unisacc.com tools/bg-candidate.cx -- status 2a46e1a4275a616628f3a8965e9a749aa5bdfe40
sh ./unisacc.com tools/bg-candidate.cx -- promote --check --rev 2a46e1a4275a616628f3a8965e9a749aa5bdfe40
```
要求 PHASES 498/498、PROMOTE_CHECK OK、compare objects/kernels/slices/product 全 same。否则停。

## 2 打包(release-run --pack)

输入照上轮:promote --check 列出 11 件汇到 out/rel058/src(上轮目录先移 out/rel058/trash,不 rm),ROOT/release/candidate.tsv 版本改 0.0.58 后:
```
sh ./unisacc.com tools/release-run.cx -- --pack ROOT release/candidate.tsv out/rel058/src out/rel058/rr2
```
记候选 APE sha、字节、`--version` 为 unisacc 0.0.58、SOURCES_SID VERIFIED。回填的 candidate.tsv 取 out/rel058/rr2/candidate.tsv。

## 3 提交 X(只含 release/candidate.tsv)

```
git commit -m "0.0.58 候选表 X(冻结 2a46e1a427)" -- release/candidate.tsv   # 加 Co-Authored-By 尾注
git push origin main
```
X=`git rev-parse HEAD`,记入账本。注意:X 会使 cx-root-identity、dev-product 对已装 0.0.57 根转红,属预期,第 8 步安装后恢复。

## 4 草稿与七资产(替换旧资产)

```
gh release view v0.0.58 -R partnernetsoftware/unisacc --json assets -q '.assets[].name'
gh release delete-asset v0.0.58 NAME -y -R partnernetsoftware/unisacc     # 七个旧资产逐个删(这是对草稿自有资产的替换,先记账)
gh release edit v0.0.58 --target X -R partnernetsoftware/unisacc          # 若 target 仍是 931fa623
gh release upload v0.0.58 out/rel058/rr2/layout/* -R partnernetsoftware/unisacc --clobber
```
也可直接 `--clobber` 覆盖同名。上传后读回七件逐件 cmp,SHA256SUMS 6/6 OK,release-layout --check PASS,asset state 全 uploaded;不齐不触发 CI。

## 5 CI(同候选原生)

```
gh workflow run candidate-native.yml -R partnernetsoftware/unisacc -f ape_sha256=APE_SHA -f commit=X -f release_tag=v0.0.58
gh run list -R partnernetsoftware/unisacc --workflow candidate-native.yml -L 1
gh run watch RUNID -R partnernetsoftware/unisacc      # 有总 deadline,不无限轮询
```
要求 42 格全 PASS(尤其 win 两格 stream-pipe)。FAIL 则不公开,同 REL058-HOLD 处置并还原 candidate.tsv。

## 6 s9 commit 档(分相)

克隆须自带候选 APE、unisacc.com.build.json、提交 X;RUN 须在 CLONE/out 下:
```
sh ./unisacc.com tools/s9-nightly-run.cx -- --clone CLONE --evidence CLONE/out/s9-run --tier commit --max-steps N
```
按上轮 init(约 159 秒)、steps(约 200 秒,多次)、aggregate(分 4 片加 merge)分相,每次 <=60 秒外壳内用 --max-steps 续跑。上轮四项红已修(S9FIX-058),要求 FAIL=0、UNKNOWN=0;NOT_FULL 仅允许 osx 批 NOT_APPLICABLE。与 CI 可并行。

## 7 裁决

```
sh ./unisacc.com tools/release-decide.cx -- --candidate out/rel058/rr2/candidate.tsv --evidence out/rel058/evidence.tsv --assets out/rel058/rr2/layout
```
evidence.tsv 为 step/verdict/path 三列,CI 与 s9 结果据实写 PASS/FAIL,不猜。必须 QUALIFIED 才继续。

## 8 提交 Y、公开、读回、安装

```
# Y:候选表回填 CI run id,只含 release/candidate.tsv(及公开记录),推 main
git tag v0.0.58 Y && git push origin v0.0.58        # 先 tag,不先删旧标签
gh release edit v0.0.58 --draft=false --latest -R partnernetsoftware/unisacc
gh release view v0.0.58 -R partnernetsoftware/unisacc --json isDraft,isLatest,assets
# 读回:下载公开资产,sha 与候选表七项逐件相同
sh ./unisacc.com tools/install-ape.cx -- ROOT out/rel058/rr2/ape/unisacc-next.com APE_SHA --write
sh ./unisacc.com unisacc.com --version                 # 要求 unisacc 0.0.58
```
安装后跑 cx-root-identity 与 dev-product 批次确认转绿,必要时 tools/affected.cx。

## 9 发布说明(公开前)

plans/release-notes-0.0.58.md 重写:候选改为 F 链、新 X/Y、新 APE sha、CI 新 run id 与结果、s9 结果、git 范围 `v0.0.57..Y`;已知缺口保留:win cx.stream 修正(drivestream 的 bound 宽度,xs-bound-width 批)、0.0.59 草稿(ROUTES 9b107888db、link-DCE f9a79c9dbd)不在候选、LIMIT-90-TEMP 仍在、整版超 2 小时、s9 release/daily 档未跑、BASE6 与 p3 材料未重建。S9FIX-058 四项修复如实写。未实跑项写未跑。

## 10 清扫

```
sh ./unisacc.com tools/release-sweep.cx -- --manifest M.tsv --trash ~/cc-work/trash --out-manifest out/sweep.tsv --latest 0.0.58 --refs agentwork/ledger.tsv plans/release-notes-0.0.58.md --dry-run
```
dry-run 看拒绝清单,无误再 `--apply`。保留最近一版冻结克隆与链输出;out/b1-old/9a34-done-ci-fail 可归档。账本记 REL058 一行(决定、依据、证据、回滚:gh release edit --draft=true 撤回、删 tag)。
