# src version-only 具名轻审事实材料（cdx，2026-10-10）

基于 bc1339c6 独立树。状态：材料已核，交机房主任裁；不写“已审 0.0.39”，不改 gatedeps 哈希。本材料不是套件执行或 queue 出口。

## 两端完整 src 差集

旧端 9926cff0（版本提交父）；新端 82290c17（本轮重建身份）。对每端执行 `git -c tar.umask=022 archive --format=tar <REV> src`，对全部常规文件生成 `{path:[regular-file-mode,sha256(bytes)]}`，按 `json.dumps(...,sort_keys=True)` 求 SHA256。src 文件全为 compilercheck inventory_suffixes 内的输入；未按工作树脏内容取值、未忽略删除或新增。此算法与 refresh_gatedeps 的文件模式/内容映射相同。

实算输出：
```
9926cff0: files=17 stamp=ab4be130492cf5dbfe11f08de7fa87268e630a1e3fb5fe1e20fb56ad801ff2e9
82290c17: files=17 stamp=18ce7e3bf8f281156e7561df403a7f33ae53a72d50933be90fd0d499bdb72699
changed=['src/version.h']
src/version.h [33188, 'b84cb98b4e8dd963ce58e97f125752c84b07b0514212b4d3a804c676ffdd6461'] -> [33188, 'cad5d171425a275bc51f89012553eac99a5e1b0975d9fb0298893f187649c2eb']
```

`git diff 9926cff0 82290c17 -- src` 原输出：
```diff
diff --git a/src/version.h b/src/version.h
index fce00870..4623bdf0 100644
--- a/src/version.h
+++ b/src/version.h
@@ -1,3 +1,3 @@
 /* Shared product version for the reference and model drivers.
    build_ref.sh embeds this declaration in the standalone unisacc.c. */
-#define UNISACC_VERSION "0.0.38"
+#define UNISACC_VERSION "0.0.39"
```

唯一差异由 eec82d5a 引入，src/version.h 一行 0.0.38→0.0.39；其他 src 文件路径、模式、字节摘要相等。旧戳 ab4be130 与新戳 18ce7e3b 都已独算匹配；匹配仅证明身份，不自动授予审核状态。

## 版本串用途与语义范围

unisacc.c:10 引入 src/version.h；src/main.c:98–100 的 -version/--version 分支只 printf 版本并立即 return 0。exec/c/compiler.c:7 引入同头；其 :420 版本分支也只输出并返回。这两处是运行时版本输出消费者；构造期及发布期消费者见下列全仓检索，不能把运行时两处声称为全仓仅有消费者。

tests/export_ref.sh:13–29 递归展开 unisacc.c 的 include，故将版本声明嵌入独立参考 C；tests/build_ref.sh:8–10 调该导出并拼参考 shim/foot。这是版本元数据与输出字节身份变更，必须重建；从该单行及消费者可判定没有新增编译算法分支。此内容判断不声称重跑所有编译行为、六平台或 ABI 验收。

## 构造期/发布期消费者补核

更正：buildcompiler.sh:231 读取唯一版本声明，:243 传给 seed ape `--product Unisacc "$product_version"`，:246 传给 Python `unisa ape --product-version`。用途是 APE 产品版本元数据和容器身份，**不是 compiler-tape 版本图**。该头值改变使这些元数据及容器字节/身份改变；脚本条件和构造算法在此版本行 delta 中未变。未实算 0.0.38/0.0.39 APE 元数据结构差集，记 UNKNOWN；候选全文字节差不在本材料冒充“仅版本串”证明，不重跑构建。

全仓跟踪文本检索命令（固定评审基线，不包含本材料自引用或未跟踪文件）：`git grep -n UNISACC_VERSION bc1339c6 -- .`。原输出：
```
bc1339c6:.github/workflows/windows-signing.yml:164:          versions=re.findall(r'^\s*#\s*define\s+UNISACC_VERSION\s+"([^"]+)"\s*$',Path('src/version.h').read_text(),re.M)
bc1339c6:exec/c/buildcompiler.sh:231:    product_version=$(awk -F'"' '/^#define UNISACC_VERSION "[0-9.]+"/ { n++; v = $2 } END { if (n != 1) exit 1; print v }' src/version.h)
bc1339c6:exec/c/compiler.c:420:            printf("unisacc %s\n",UNISACC_VERSION); return 0;
bc1339c6:release/README.md:48:`product_version` 必须精确等于源码 `src/version.h` 中唯一的 `UNISACC_VERSION`；
bc1339c6:release/RELEASE-PIPELINE.md:6:- 改 `src/version.h` 唯一的 `UNISACC_VERSION`；VERSIONINFO/产品版本由构建读取，`ape-version` 门禁比对候选。
bc1339c6:release/macosbundle.py:53:        versions = re.findall(r'#define UNISACC_VERSION "([0-9.]+)"', (ROOT/'src/version.h').read_text())
bc1339c6:release/tools/precheck.sh:19:V=$(sed -n 's/.*UNISACC_VERSION "\(.*\)".*/\1/p' src/version.h)
bc1339c6:release/tools/precheck.sh:42:V=$(sed -n 's/.*UNISACC_VERSION "\(.*\)".*/\1/p' src/version.h)
bc1339c6:release/tools/unsigned_receipt.py:6:ver=re.findall(r'#define UNISACC_VERSION "([0-9.]+)"',(R/'src/version.h').read_text());assert len(ver)==1
bc1339c6:src/main.c:99:            printf("unisacc %s\n", UNISACC_VERSION);
bc1339c6:src/version.h:3:#define UNISACC_VERSION "0.0.39"
bc1339c6:tests/apeversioncheck.py:99:    declared=re.findall(r'^\s*#define\s+UNISACC_VERSION\s+"([0-9]+(?:\.[0-9]+){2,3})"\s*$',
```

消费者分类：src/main.c 与 exec/c/compiler.c 为运行时输出；exec/c/buildcompiler.sh 为 APE 构造元数据；windows-signing.yml 为签名流程版本提取；release/macosbundle.py 为包版本；release/tools/precheck.sh 两处为预检版本定位；unsigned_receipt.py 为未签回执版本；tests/apeversioncheck.py 为声明/工件版本核对。src/version.h 是定义，README/RELEASE-PIPELINE 是说明，非执行消费者。该清单仅针对字面宏检索；不宣称列尽由 product_version 等中间变量传递的所有下游。

## 已发生的同源重建（与审核时序分列）

cc 在干净 /tmp/cc39-freeze2 @82290c17 重新 build_ref/build_candidate 并 comboot；原日志 /tmp/cc39-b5b/rebuild.log。cdx 已只读实核候选/pair SHA256 2b20f4b2fd9a496d2e36d47577741420f9e4aba47be0d377e3866f5f91aa5ed9；seed c244c2e488d4ec00f3cb81a18f96647fdaffeb89004a666342bdd749fb5d7ae5。pair build.json commit=82290c17b4a57dba77a81ade6d7fdd1c311f8917，sources_sha256=0bd83174d54ba9af762e256b6f66fce47ddd5402c06705eba32ee5cfdcccf214，artifact 等于候选。日志定点 rc0，完成 16:09:33+08。

净 B.5 起点 16:10:29+08，裁②约16:12，本材料其后：不能回写为“先轻审盖戳后 queue”。裁②允许 queue 继续，pending src 戳与队列出口分列，最终盖戳由机房主任决定。未运行构建/queue，未动 Draft；旧593作废账及 UNKNOWN 限定不变。
