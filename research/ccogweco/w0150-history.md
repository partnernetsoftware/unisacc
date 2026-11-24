# W0150 取证:build.json 核对是哪个提交引入的(只读,2026-11-24 ccogweco)

## 结论(先)

在 0.0.105 冻结提交 414a024ea5 到 107 链头 2e8a19a342 之间,**没有任何提交**新增或改动
`tools/release-exec.cx`、`tools/check-release-exec.cx`、`tools/release-run.cx` 里对
`unisacc.com.build.json` 的 SHA256SUMS 核对:两条 `git log -S`(SHA256SUMS、build.json)在这三个文件上
都为空;把范围放宽到 tools/、cx/、src/ 仍然为空。该核对早于 0.0.105,不是本窗口引入。

`check-release-exec.cx` 在本区间只有一笔提交 71c7035f9c 碰过(R27 S2 的 F_CHAIN_REV),它只把候选表替身里的
假 commit sha 换成参数,没有改 build.json/SHA256SUMS 夹具;夹具里的两个别名片在 0.0.105 时就已在
(`git show 414a024ea5:tools/check-release-exec.cx | grep -c build.json` = 12)。因此"夹具是否同一提交同步更新"
的答案是:不存在这类同步,因为核对也没有被改。

W0150 的红因此不是新门的红,而是产物侧内容不一致:草稿里 `SHA256SUMS` 的 `unisacc.com.build.json` 行与
下载回来的该文件字节不符(或该文件未下载到)。见详情第 3 节的三个待核方向。

## 详情

### 1 命令与输出原行

    git log --oneline -S 'SHA256SUMS' 414a024ea5..2e8a19a342 -- tools/release-exec.cx tools/check-release-exec.cx tools/release-run.cx
    git log --oneline -S 'build.json' 414a024ea5..2e8a19a342 -- tools/release-exec.cx tools/check-release-exec.cx tools/release-run.cx
    (两条均无输出)

    git log --oneline -S 'SHA256SUMS' 414a024ea5..2e8a19a342 -- tools/ cx/ src/     (无输出)
    git log --oneline -S 'build.json'  414a024ea5..2e8a19a342 -- tools/ cx/ src/     (无输出)
    git log --oneline -S 'build_alias' 414a024ea5..2e8a19a342                        (无输出)

### 2 核对的来源与位置(早于 0.0.105)

- tools/release-exec.cx 由 89c294e401 新建(2026-11-18 15:06,"release-run --execute:候选就绪到公开读回一条带门命令");
  `git merge-base --is-ancestor 89c294e401 414a024ea5` 成立(早于 0.0.105)。
- 比较函数:`tools/release-exec.cx:1023` 注释 + `:1033`
  `if (tlen(hex) != 64 || memcmp(hex, ls, 64)) { cx_fmt(bad, cap, "SHA256SUMS mismatch %s", nm); return -1; }`
  ——它把 SHA256SUMS 的每一行与下载目录里的同名文件重新哈希比较,W0150 的报错文案 "SHA256SUMS mismatch
  unisacc.com.build.json" 就是这里打印的。
- 调用点:`tools/release-exec.cx:1071-1073` 第 2 步 draft_readback:
  `k = sums_check(tmp, s, sn, bad, sizeof bad); if (k != 5) ... refuse("DRAFT_READBACK", k < 0 ? bad : "SHA256SUMS does not list five assets");`
- 五件资产与别名:候选表第 15 列即 `unisacc.com.build.json`(tools/check-release-exec.cx:347-348 的替身行);
  ci-early 的草稿资产表 `tools/ci-early.cx:482` 也含该名,并在 `:604-611` 用这五个文件写 SHA256SUMS。
  这两个文件在本区间均无改动(`git log 414a024ea5..2e8a19a342 -- tools/ci-early.cx tools/cxlib/releaseprepare.cx tools/release-layout.cx` 为空)。

### 3 因此要核的三个方向(不是新门)

1. 草稿复用/陈旧:第 2 步下载的是 `cand.tag` 名下已存在的草稿(`gh release download`,release-exec.cx:1058、:1069),
   若草稿由更早一轮上传、SHA256SUMS 是旧文件时写的,而 `unisacc.com.build.json` 后来重传,行与文件就会不一致。
   核 `release_state()` 与草稿创建时间、以及 `ci_run_id` 是否与当前候选一致。
2. 生成顺序:ci-early 先写五件再写 SHA256SUMS(ci-early.cx:604-611),release-run 的装配相在
   `tools/release-run.cx:573-575` 之后交给 release-layout;若别名(build.json / unisacc.com.build.json)
   在写 SHA256SUMS 之后才生成或改内容,就会命中同一比较。核产出目录里两个别名的内容与 sums 行。
3. 缺文件:sums_check 对每行都 `sha_file`,文件不在下载目录时 hex 为空,`tlen(hex)!=64` 也报同一 mismatch;
   先确认下载目录里是否真有 `unisacc.com.build.json`,再看是内容不同还是根本没下载到。

### 4 夹具同步判断(细节)

- `git log --oneline 414a024ea5..2e8a19a342 -- tools/check-release-exec.cx` 只有 `71c7035f9c`。
- 该提交 diff 中与 build.json/SHA256SUMS 相关的改动全是候选行里的假 commit sha 参数化(11 行命中),
  没有新增别名片,也没有改 SHA256SUMS 的构造(`put(lay, "unisacc.com.build.json", ...)`、`:345` 附近的 sums 生成
  在 414a024ea5 上已存在)。
- 夹具替身 rh3 与假 gh 不在本区间改动范围内;它们对第 2 步的响应用的是同一套 layout 目录,
  与 0.0.105 相同。
