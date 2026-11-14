# 布局假设清单:还有哪些地方隐含"根产品是单文件 APE"

> 只读调研,2026-11-02,omp-go。冻结期内不改任何被运行的文件。
> 背景:12 个结构性 FAIL 收尾见 [`v0.0.44.md`](v0.0.44.md) 的"12 个结构性 FAIL"一节;
> 那次的根因就一句话——**把产品当字节读的人假设了单文件 APE**。

## 两种布局

| 布局 | `unisacc.com` 是什么 | 包在哪 |
|---|---|---|
| APE | 自包含文件,末尾有 `UNIPKG1` footer | 产品文件本身 |
| dev-lnx | 薄壳启动器(624 字节 `/bin/sh` 脚本) | `unisacc.com.dev/p3-package.bin` |

判断只应有一处:`cx/sys` 之外的可读接口是 `tools/cxlib/productpkg.cx` 的
`pp_layout` / `pp_package_path` / `pp_read_package` / `pp_copy_product`。

## 分类一:对 dev-lnx **无效**(仍按单文件 APE 读产品)

这些在 dev-lnx 下会直接 `die`,或在读到的 624 字节启动器上做无意义判断。

| 位置 | 行为 | 后果 |
|---|---|---|
| `tests/resources.cx:342` | `container = root/unisacc.com`;若是 ELF 或末尾 `UNIPKG1` 才用,否则 `die("optional ...")` | dev-lnx 下两者都不成立 -> 拒绝 |
| `tests/optimization.cx:49-50` | 同上,`die("expected ELF driver or packaged product")` | 同上 |
| `tests/stream-model-identity.cx` | 由参数给 `CANDIDATE_PKG_OR_PRODUCT` | **不受影响**(调用方给什么读什么) |
| `tests/candidate-admission-lnx-x86_64.cx:59` | `path(product,...,root,"unisacc.com")` 后查 `UNIPKG1` | dev-lnx 下永远不成立 |
| `tests/hostseed-controls.cx:36` | 在 payload 上找 `UNIPKG1` | 与根产品无关,**不受影响** |
| `tools/csih-ape.cx:445` | `cx_read("unisacc.com")` 直接读当前目录产品 | **csih 车道文件,不由本树授权改写**,仅登记 |

判据提醒:`cx_read` 出现在文件里**不等于**它读产品。上面这张表每一条都核过读取的路径来源。

## 分类二:会在 osx/win 完整 APE 回来后**失效**(硬编码了拆分布局)

| 位置 | 行为 | 后果 |
|---|---|---|
| `tools/check-productpkg.cx:62` | 断言 `pp_layout(root) == "split"` | APE 回来后这条 FAIL —— **是我写的**,必须改成"两种布局都接受、按布局断言不同事实" |
| `tools/check-commit-own.cx:74` | 克隆命令里写死 `cp unisacc.com ... && cp -r unisacc.com.dev ...` | APE 回来后 `cp -r unisacc.com.dev` 失败 |
| `tools/check-dev-product.cx:104-107` | 依次搜产品、`DIR/p3-package.bin`、`DIR/unisacc.com.dev/p3-package.bin` | **已适配两种布局**,列在这里只为说明它是模板 |
| `tools/cxlib/productpkg.cx` | `pp_layout` 先看 `unisacc.com.dev/p3-package.bin`,再看产品 footer | **已适配**,是其他文件该抄的样子 |

## 分类三:**已适配**

| 位置 | 用什么 |
|---|---|
| `tools/check-codec.cx` | `pp_read_package` |
| `tools/check-packagefooter.cx` | `pp_read_package` |
| `tools/check-com-tree.cx` | `pp_package_path` + `pp_read_package` |
| `tests/loader-faults.cx` | `pp_package_path` + `pp_copy_product` |
| `tests/runner-faults.cx` | `pp_copy_product` |
| `tools/check-dev-product.cx` | 三条候选路径依次尝试 |
| 四个容器批次(`formats-elf-ape` / `properties` / `negative-model-loader` ×2) | 不再绑 `root/unisacc.com`,由材料 `ape-0043` 指定 APE |
| 所有 `sh <product> ...` 的调用点 | 天然布局无关,启动器自己找旁目录 |

## 附:同一个根因的覆盖缺口(不属于上面三类,但更隐蔽)

**"候选未变"的守卫只哈希了 `unisacc.com`。** dev-lnx 下那只有 624 字节;

| 位置 | 行为 |
|---|---|
| `tests/run-batch.cx:44,49` | `digest(candidate, before/after)`,`candidate = root/unisacc.com` |
| 约 25 个 `tests/*.cx`(stream-*、ri-*-controls、interfaces-lnx-x86_64、library-abi、properties、resources、optimization、sizeof-postfix、formats、hostseed-controls 等) | 同样的 `digest(product)` 前后对比 |
| `tools/package-refresh.cx:46,61` | 把 `oldroot/unisacc.com` 的 sha 当"baseline product identity" |

后果:**一个批次可以改写 5 MB 的 `p3-package.bin` 而 `before == after` 仍然成立**,
"测试不改输入"这条合同在 dev-lnx 下**只保护了启动器的 624 字节**。
这不是某个检查写错,是"产品身份"这个概念在拆分布局下少了一半。

修法方向(不在冻结期动手):把"产品身份"定义成一个**集合**(启动器 + 旁目录每个成员),
放 `productpkg` 里给一个 `pp_product_digest(root, hex)`,让 `digest(candidate)` 换成它。
`ape-receipt.txt` 正是这个集合的现成描述,receipt 在时直接按它算。

## 不在本清单内的

- 已经改过的 12 个结构性 FAIL(见 `v0.0.44.md`)。
- `apps/csih/` 与 `ujs/` 的文件:别的车道,只登记不改。
- `out/` 下的历史构建产物:不是合同。
