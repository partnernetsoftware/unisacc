# 产品身份:把"候选未变"从 624 字节扩到整个产品

> 方案文档,2026-11-02,omp-go。冻结期内只写 plans/,不动手。
> 背景:`plans/layout-assumptions-2026-11-02.md` 第"附"节;cc 裁定为**闸门漏洞**,升格独立事项。

## 一、问题

dev-lnx 布局下 `unisacc.com` 是 624 字节的 shell 启动器,包在 `unisacc.com.dev/p3-package.bin`(2.0 MB)、
加载器在 `unisacc.com.dev/entry.elf`(1.6 MB)。而"产品身份"和"测试没改输入"这两条合同,
全仓**只哈希了那 624 字节**:

| 事实 | 值 |
|---|---|
| `root-identity.md` 的三身份 | 薄头 `d93c1b24…` / entry.elf `de54dd80…` / p3-package `47a2275d…` |
| `aw-m4-mat` 会话的 `candidate=` | 只覆盖第一条 |
| `tests/run-batch.cx:44,49` 的 before/after | `digest(root/unisacc.com)` |

后果:一个批次可以改写 5 MB 的包或加载器,而 `before == after` 依然成立、会话身份依然"稳定"。
这不是某个检查写错,是"产品身份"在拆分布局下只定义了一半。

## 二、身份定义:`pp_product_digest(root, hex)`

放在 `tools/cxlib/productpkg.cx`(它已经是布局的唯一判读处)。

### 单文件 APE

`hex = sha256(整个文件)` —— 与今天完全一致,退化情形的字节不变,旧证据不失效。

### 拆分布局

成员集合 = 启动器 + 旁目录成员,由 `ape-receipt.txt` 决定;没有 receipt 时用固定两名字。

```text
members = { "unisacc.com", "entry.elf", "p3-package.bin", ...receipt 列出的成员 }
按 basename 字典序排序                       <- 与 inode/目录顺序无关
canon = 对每个成员: "<basename>\t<sha256(成员字节)>\n"   依次拼接
hex   = sha256(canon)
```

要点:

- **按名字排序再哈希**,不是把字节接起来。目录顺序、复制顺序、文件系统都不影响结果;成员改名会改变结果。
- **每个成员单独哈希**,失败(`ENOENT`/`EACCES`)要**具名拒绝**,不能跳过——少一个成员就退化成今天这个洞。
- 只用 `basename`:receipt 里写的是绝对路径,产品被搬走后绝对路径失效,名字仍然有效。
- 版本化:`pp_product_digest` 另出一个 `pp_identity_kind(root)` 返回 `"ape1"` / `"split1"`,
  收据里必须带这个字段,否则"同名字段不可比"。**旧收据保持原样,不重解释。**

## 三、调用点清单

实测 90 处 `digest(...)`,35 个文件;其中**哈希产品本身**的按波次如下(材料/包对象的不在此列)。

### 第一波(闸门,必须先做)

| 位置 | 现状 | 改法 |
|---|---|---|
| `tests/run-batch.cx:44,49` | `digest(candidate)` | `pp_product_digest(root)`,并打印 `identity=<ape1\|split1>` |
| `tools/cxlib/allsession.cx:151` | `as_sha_file(root/unisacc.com)` | 同上;`as_ident.candidate` 语义变成"产品身份",收据加 `candidate_kind` |
| `tools/zero-wave.cx:27,33,36` | `candidate = root/unisacc.com` + digest | 同上 |

`sid` **不用改**:`as_sid` 跑的是 `seed/ident.c` 的产品**源码**身份(exec/unisa/src/kernel/include/weights/seed),与二进制布局无关。

### 第二波(批次级"未改输入"守卫,24 个文件)

`tests/formats.cx:15,17` · `hostseed-controls.cx:35` · `interfaces-lnx-x86_64.cx:285,294` ·
`library-abi.cx:58,76` · `optimization.cx:37,43,51,59`(同时有 `container` 变量)·
`properties.cx:53,65` · `resources.cx:95,342` · `ri-chain-controls.cx:26,40` ·
`ri-flow-controls.cx:39,53` · `ri-flow-data-controls.cx:33,46` · `sizeof-postfix.cx:26,29` ·
`stream-e1-lex.cx:490,493` · `stream-e1.cx:72,89` · `stream-e2-domain.cx:300,304` ·
`stream-e2-gold.cx:49,70` · `stream-e2.cx:74,91` · `stream-e3.cx:108,123` · `stream-e4.cx:36,48` ·
`stream-elf.cx:33,45` · `stream-lower-domain.cx:468,473` · `stream-lower-targets.cx:51,59` ·
`stream-lower.cx:36,47` · `stream-prune.cx:39,55` · `streams.cx:101,105`

### 第三波(工具自己的身份/基线)

| 位置 | 用途 |
|---|---|
| `tools/package-refresh.cx:46,61` | `oldroot/unisacc.com` 的 sha 当 baseline product identity |
| `tools/object-material.cx:21,29,30` | 材料准备时的产品前后对比 |
| `tools/classic18-product-nets.cx:35,48` | "候选等于根产品"的负例控制 |
| `tools/check-p3-extract.cx:18` | 核对 manifest 里 `declared-bootstrap unisacc.com` 的 sha |
| `tools/classic18-private-evidence.cx:24-33` | 证据行里的产品 sha |

### 明确排除(哈希的不是产品)

`tools/source-package*.cx`、`tools/candidate-promote.cx`、`tools/check-p3-extract.cx` 的 `pkg` 变量——
它们哈希的是**包对象/记录文件**(`compiler.pkg`、`package.tsv`、`.build.json`),与布局无关。
`tools/source-package-adopt.cx` 的 `p` 同理。

## 四、会话身份与 `root-identity.md` 的机器校验

### 会话(sid / candidate)

`aw-m4-mat` 这类会话的 `candidate=` 字段今天只覆盖 624 字节。改法:

1. `as_ident.candidate` = `pp_product_digest(root)`。
2. 收据同时写 `candidate_kind=ape1|split1`。**两个 kind 的 `candidate` 值不可互相比较**,
   聚合遇到 kind 不同必须报 identity 不可比,而不是报 mismatch。
3. `candidate_stable` 的判定改用它,于是"跑波期间有人换了 entry.elf"会真正被抓到。

#### 旧会话目录:按 `ape1` 解释,标 `legacy-no-kind`

`out/aw-m4/` 与 `out/aw-m4-fixarg/` 已有的收据没有 `candidate_kind` 字段,`candidate_before`
的值是 624 字节启动器的 sha(实测例:`cx-l0-fmt.receipt` 的 `candidate_before=1bfdcd20…`)。
**取证用的旧会话必须能读**,规则:

| 情况 | 处理 |
|---|---|
| 收据有 `candidate_kind` | 按字段解释;两个 kind 不同 -> **identity 不可比**(不是 mismatch) |
| 收据无 `candidate_kind` | **按 `ape1` 解释**(该值就是一个文件的整文件哈希),并在聚合输出里标 `legacy-no-kind` |
| 新旧混在同一聚合 | 允许,但聚合必须显式打印 `identity_scope=legacy-no-kind+split1`(或实际组合),不许静默归一 |

`legacy-no-kind` 是**标注**,不是修补:不回写旧收据、不重算旧值、不把旧值当新 kind 的等价物。
旧会话的结论成立范围仍然只有那一个文件,这一点在输出里必须看得出来。

### `root-identity.md`

今天它是一段人手写的三行 sha,机器不可校验。建议:

- 生成块里**只写事实**(kind、成员名、每个成员的 sha256 与字节数),**判定口径的文字留在块外**。
  块内出现"有效/通过/等价"这类判断词,检查就无法逐行比对,这一条是把文档变成机器可读的前提。
- 在文档里放一个**生成块**,由 `tools/check-dev-product.cx` 或新工具 `--emit-identity` 写:

  ```text
  <!-- identity-begin -->
  product_kind	split1
  product_digest	<pp_product_digest>
  unisacc.com	<sha>	<bytes>
  entry.elf	<sha>	<bytes>
  p3-package.bin	<sha>	<bytes>
  <!-- identity-end -->
  ```

- 一个检查(建议并入 `check-dev-product`)读该块、重算、逐行比对;不一致即 FAIL 并打印差异。
  这样文档不再"只是写着",而是**可被机器否决**。
- 该检查**登记为批次**(不是随手脚本):只有进注册表,它才会在每次全量里真正跑,
  否则"可被机器否决"只是一句设计意图。
- 三身份 sha 仍保留人能读的形式,但**来源**是生成块,不是手抄。

## 五、两处布局假设修补

| 位置 | 改法 |
|---|---|
| `tools/check-productpkg.cx:62` | 现在断言 `layout=="split"`。改成:两种布局都接受,按实际布局断言不同事实(split 时断言旁目录三成员齐备;ape 时断言整文件 footer),不再把"当前是 split"写成合同 |
| `tools/check-commit-own.cx:74` | 克隆命令写死 `cp unisacc.com … && cp -r unisacc.com.dev …`。改成按 `pp_layout` 决定复制哪些成员(可直接用 `pp_copy_product`) |

## 六、三项"对 dev-lnx 无效"的判断建议

| 位置 | 建议 | 理由 |
|---|---|---|
| `tests/resources.cx:342` | **改读 `pp_package_path`** | 它要的是"带 P3 的容器"(`\177ELF` 或 `UNIPKG1` 二者之一),包的定位与布局无关;登记成 N/A 会把一个仍然有效的断言扔掉 |
| `tests/optimization.cx:49-50` | **改读 `pp_package_path`** | 同上,`container` 变量的用途就是找 P3 |
| `tests/candidate-admission-lnx-x86_64.cx:59` | **不用改** | 更正我先前清单里的一条:它对 `root/unisacc.com` 只当**构造器**用(`sh product ident.c`),`UNIPKG1` 那条查的是 `gen/unisacc-next.com`(构造出来的候选 APE),不是根产品。分类错了,这里撤销 |

`tools/csih-ape.cx:445` 仍属 csih 车道,只登记,不在此方案内。

## 七、回归方案

1. **单元**:`check-productpkg` 增加两条断言
   - `digest-is-member-set`:对现装 split 布局,断言 digest == 手工按 receipt 三个成员算出的值;
   - `digest-rejects-missing-member`:临时删掉副本里的 `entry.elf`,断言 `pp_product_digest` **报错**而不是给出一个"合法"的哈希。
2. **负例(最关键的一条)**:复制产品到 `out/`,**只改 `p3-package.bin` 一个字节**,
   断言 `before != after`。今天这条会失败——它就是闸门漏洞的证据。修好后必须是新断言的一部分。
3. **门禁**:`tests/run-batch.cx` 改完后,用 buckets `run-batch` 自身的
   "exclusive output path / candidate unchanged" 语义重跑 24 个第二波批次,确认没有误报。
4. **会话**:用两个候选(只换薄头 / 只换 entry.elf)各 init 一次会话,断言 `candidate` 值**不同**;
   再换 `p3-package.bin` 一个字节,断言同样不同。三条都过才算身份定义生效。
5. **APE 退化**:对材料 `ape-0043`(`out/m4-backup/unisacc.com.83343ec1`)跑 `pp_product_digest`,
   必须等于 `sha256(整文件)` = `83343ec1…ea775`,保证旧证据不被重解释。

## 八、实施顺序与风险

0. **先写回归,并确认它今天失败**(改 `p3-package.bin` 一个字节而 `before == after`)。
   没有这一步,"修好了"就没有对照;这条断言本身就是漏洞的取证。**失败是第 0 步的验收标准。**
1. `pp_product_digest` + `pp_identity_kind` + `check-productpkg` 两条断言(不动任何调用点,零风险)
2. 第一波三个闸门点 + `candidate_kind` 字段
3. 第二波 24 个批次(机械替换,一次一波,每波跑 affected)
4. 第三波工具
5. `root-identity.md` 生成块 + 校验
6. 布局修补(第五节)与两项 dev-lnx 修复(第六节)

风险:

- **收据语义变更**:`candidate` 字段同一位置含义变了。必须靠 `candidate_kind` 区分,
  否则旧收据会被误读成"候选变了"。这是本方案唯一的破坏性变更。
- **性能**:split 身份要读 3.6 MB 并算三次 sha。`run-batch` 每批一次 before/after = 每个批次多约 7 MB 读取。
  实测过 `check-productpkg` 全流程 5.5 秒(含两次整产品复制),可接受;若成为热点,
  缓存按 `(path, mtime, size)` 记忆化——但那要先证明是热点,不预先做。
- **`candidate` 与 `{candidate}` 占位符**:`{candidate}` 仍应是**启动器路径**(给 `sh` 用),
  只有"身份"换成集合哈希。两者不能混。

## 附:为什么 `tests/library-abi.cx` 没改

第二波替换跳过了它。它的 `product` 变量不是 `root/unisacc.com`——
`tests/library-abi.cx:58,76` 的两次 `digest(product,…)` 哈希的是一个**由参数传入的候选**,
不是安装的产品。盲改会把"候选未变"错算成"根产品未变",那比不改更糟。

处理时机:等有人确认这两处到底是"候选自检"还是"根产品自检"再动。
判据是看 `main` 里 `product` 的赋值来源:来自 `argv` 就换成对候选文件的整文件哈希(不需要
产品身份),来自 `path(…,root,"unisacc.com")` 才用 `pp_product_digest`。
在此之前它保持原样,并在此记明是**有意未改**,不是漏掉。
