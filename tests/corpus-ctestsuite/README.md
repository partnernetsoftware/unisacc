# c-testsuite 固定外部材料

本目录保存 220 例清单、上游提交与每例源码/期望输出 SHA256，不重新分发上游源码；许可证须逐例查 `.otags`，不能用顶层 MIT 覆盖所有来源。

仓库根执行（下载材料与结果目录均在仓外）：

```sh
mkdir -p /tmp/ctestsuite-material
curl -fL --max-time 40 https://codeload.github.com/c-testsuite/c-testsuite/tar.gz/5c7275656d751de0e68b2d340a95b5681858ed07 -o /tmp/ctestsuite-material/source.tar.gz
printf '010008bf4b5671f947ae7e8d693a1c959a4a4d795fa7227c0abb1f95de45f7e7  /tmp/ctestsuite-material/source.tar.gz\n' | sha256sum -c -
tar -xzf /tmp/ctestsuite-material/source.tar.gz -C /tmp/ctestsuite-material
ROOT="$PWD"
MATERIAL=/tmp/ctestsuite-material/c-testsuite-5c7275656d751de0e68b2d340a95b5681858ed07
# START 从1开始，每次最多20例，OUT必须不存在；依次取1,21,...,201即可覆盖220例。
timeout -k 2s 55s sh ./unisacc.com tools/ctestsuite.cx -- "$ROOT" "$MATERIAL" /tmp/ctestsuite-result-001 1 20
```

每个被执行用例先核对源码与 `.c.expected` 的完整字节哈希。编译阶段单独记接受/拒绝，再使用产品默认 `-run` 执行已接受输入；期望退出0、合并输出与上游期望一致。UNKNOWN 不算拒绝或成功，WRONG 不算通过。不是六目标资格或 C99 完整符合性证明。

首轮使用 c46-3 APE `884adfef1815e87750c86c7036d3074552a66ce18dfa855588ab279c2aa99fcf`（`unisacc 0.0.46`）：220例，编译接受216、期望符合216、错误输出0、拒绝4、未知0。拒绝为00213/00214/00219的 `this is not the start of an expression` 和00216的 `reject: unexpected character`，不移除这些例、不回填PASS。逐例见 `c46-3-first-run.tsv`，首错直方图见 `c46-3-first-errors.tsv`。

`tools/check-ctestsuite.cx` 用独立夹具验证接受、源码篡改、期望篡改和真正输出不符分类；不是用清单行数或探针非空冒充语义验收。
