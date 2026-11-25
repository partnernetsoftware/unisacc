# R37: netregen 基线取值核对与第一车 0.0.106 预跑核查

日期: 2026-11-24
执行窗口: agy-h (只读取证, 未运行重命令)

## 1. 结论摘要

(1) 基线表与取值方式: `netregen` 族批次读取的基线表是 `tools/old-product-assets.tsv`。`tools/old-product-material.cx` 第 43-56 行 `predecessor()` 解析该表全部非注释行的三段版本号 `a.b.c`, 筛选严格小于候选版本号 (`vless(x, c)`) 且数值最高的条目 (`vless(best, x)`), 既不是固定某一行, 也不是按文件行序机械取最末一行。`c4a154dfea` (2026-11-24 15:47:27 +0800) 在第 5 行登记 `0.0.101` (`8827155` 字节, `sha256=bb715a8bd3ed1237bb69d42d0cd1c2594c0a27c8e223b518ae39efe6b79eac82`) 后, `0.0.106` 选中 `old-version=0.0.101`, `0.0.105` 运行时表内最高小于 `0.0.105` 的版本为 `0.0.83`。

(2) 第一车 `0.0.106` 预跑实况 (`~/.cache/unisacc/prerun/v0.0.106-2e8a19a342/`): `netregen-module` 与 `netregen-opt-module` 共 12 批位于 `windows.tsv` 的 `W0016` 到 `W0027` (`heavy`, `jobs=1`), 已全部实跑 (`strict-W0016.tsv` 到 `strict-W0027.tsv`, 候选 `5f83ac93d201b252ea0b56098475be9e30045875cdf1ce692451cda095ceeb3c`, 材料实证 `old-version=0.0.101`)。12 批全部 `PASS`, `rc=0`, `marker=NETREGEN_SHARD_SAME`, `note=-`, 未产生 `image=OLD->NEW` 差异记录。

(3) `priority.tsv` 中 `note` 旧哈希核对: 第一车预跑输入 `priority.tsv` 继承自 `0.0.105` 严格预跑结果 (`candidate=934c7c0bc09a9a4d2e107b8d6bdc855553cf05f64dbb7d6c1e5fefdb42117dd1`, 12 批位于第 90-101 行与第 152-163 行), 其 `note` 形如 `image=OLD->NEW`。逐行比对 `release/evidence/0.0.101/expected-fail.tsv` 与 `release/evidence/0.0.105/expected-fail.tsv` 后确认: 左值 `OLD` 并非 `0.0.101` 的哈希, 而是 `0.0.83` 编译出的旧镜像哈希; 右值 `NEW` 才逐行等于 `0.0.101` 经 `RT_UNIT` 引入后的新镜像哈希 (与 `0.0.105` 相同)。基线表升为 `0.0.101` 后, `0.0.106` 与 `0.0.101` 逐字节相等, 12 批全部转绿。

## 2. 基线表与选择逻辑证据

### 2.1 `tools/old-product-assets.tsv` 当前全部 5 行

```tsv
version	repo	tag	asset	bytes	sha256
0.0.83	partnernetsoftware/unisacc	v0.0.83	unisacc.com	7399573	12fd0c572996d57f7b3c6df2f1390f55ec6fbff5274896f7abd2cd0dcbf0258b
0.0.80	partnernetsoftware/unisacc	v0.0.80	unisacc.com	7388948	110ab298bf25065a1ecf7ff36c82609c42d7a4eeaf02bcc8228e0e2271bc8f78
0.0.79	partnernetsoftware/unisacc	v0.0.79	unisacc.com	7387189	b328ed1f582e21bc427cbda96a047de855c56abb2127ebaec288077ef1ee25d
0.0.101	partnernetsoftware/unisacc	v0.0.101	unisacc.com	8827155	bb715a8bd3ed1237bb69d42d0cd1c2594c0a27c8e223b518ae39efe6b79eac82
```

### 2.2 `tools/old-product-material.cx` 取值逻辑与 `0.0.106` 实跑材料记录

- `tools/old-product-material.cx:43-56` (`predecessor()`): 逐行跳过空行与 `#` 注释行及表头, 用 `vparse()` 解析每行三段版本号, 仅当 `vless(x, c)` 为真 (严格小于候选 `--version`) 且未选或 `vless(best, x)` 为真时更新 `best`。然后调用 `tools/old-product.cx` 按 `version` 精确匹配、校验字节数与 `sha256` 及 `--version` 输出。
- `tests/batches.tsv:799-828` (12 批覆盖 `module` 与 `opt-module` 共 6 目标): `tests/check-netregen-bytes.cx` 接收 `{old-product}`, 逐项比较旧版产物与新候选产物的图像字节 SHA-256, 相等输出 `NETREGEN_SHARD_SAME`。
- 预跑实跑材料文件 `~/.cache/unisacc/prep-cache/old-product-material-669a28402182e821.dir/materials.tsv` (`2026-11-24 22:34`, 与 `strict-W0027.tsv` 同时落盘) 实录:

```tsv
old-product-material-v1
status	PREPARED-no-test-result
candidate-version	0.0.106
old-version	0.0.101
old-product	~/.cache/unisacc/prep-cache/old-product-cache/bb715a8bd3ed1237bb69d42d0cd1c2594c0a27c8e223b518ae39efe6b79eac82/unisacc.com	bb715a8bd3ed1237bb69d42d0cd1c2594c0a27c8e223b518ae39efe6b79eac82
```

(注: 上文目录前缀实际落盘于用户主目录 `.cache`, 本表按仓库路径规范记为 `~`)。

## 3. 12 批预跑窗口、priority.tsv 旧哈希与 0.0.101 值逐行对照

| 批次 | windows.tsv 窗口 | 0.0.106 实跑 (vs 0.0.101) | priority.tsv 行号 | priority.tsv note (image=OLD->NEW) 与 0.0.101/0.0.105 expected-fail 对照 |
|---|---|---|---|---|
| `netregen-module-lnx-x86_64` | `W0016` | `PASS` (16272ms, `note=-`, `NETREGEN_SHARD_SAME`) | 90 | `a1f5dac55af80b24627b2ae6992067b237e4c9ab927226aa1043c6a6fc1a1c1b->5941cc8e4364bb94f3548a25d054ba061b9ef1b818a3f273c818cc149842361a` (`OLD=0.0.83`, `NEW=0.0.101/0.0.105`) |
| `netregen-module-lnx-arm64` | `W0017` | `PASS` (4719ms, `note=-`, `NETREGEN_SHARD_SAME`) | 92 | `c9e02c0043904cf7470b719a1230882b7cee1dbd157a7baa4570a447e92712ab->6d4ceef1613046491cec1f623d27fa574cf291024ba7b974c7e19750b1acc946` (`OLD=0.0.83`, `NEW=0.0.101/0.0.105`) |
| `netregen-module-osx-x86_64` | `W0018` | `PASS` (4922ms, `note=-`, `NETREGEN_SHARD_SAME`) | 94 | `dc04aa1b7b5453b7309cee575c974452f8dd8d2226dbe5b06a5fea05f85c9a34->70ddb131cc98ad0dfb3ab7c81e55977feec38225876d2acc8836b9789aee96c5` (`OLD=0.0.83`, `NEW=0.0.101/0.0.105`) |
| `netregen-module-osx-arm64` | `W0019` | `PASS` (4620ms, `note=-`, `NETREGEN_SHARD_SAME`) | 96 | `0fc34d295e3ef508936a3566ebffaf1e0b63bb14ac312fe458fa36c06469b721->10a6ac9c4a94f80795e9c15f9ad00cce9d061478caecdd74ac3ce8679a8680be` (`OLD=0.0.83`, `NEW=0.0.101/0.0.105`) |
| `netregen-module-win-x86_64` | `W0020` | `PASS` (6087ms, `note=-`, `NETREGEN_SHARD_SAME`) | 98 | `91d648c7b7b0dfabc0ea5743d9ed6a831034bab5de08e4a98717771d3f8cd84c->0493dcff117afd2f60bb26a713a403805fd2ea090ca65ddc48f4c93cb89e1bcd` (`OLD=0.0.83`, `NEW=0.0.101/0.0.105`) |
| `netregen-module-win-arm64` | `W0021` | `PASS` (5972ms, `note=-`, `NETREGEN_SHARD_SAME`) | 100 | `4482cc2d89b4b1ad832eb2da6c413b6be141d6d87543c987d2082f9b004e99af->d9438645ff5c587be5db5103919aa3ddd2c2cff1b1484290dba7d09e6ab48d5b` (`OLD=0.0.83`, `NEW=0.0.101/0.0.105`) |
| `netregen-opt-module-lnx-x86_64` | `W0022` | `PASS` (4722ms, `note=-`, `NETREGEN_SHARD_SAME`) | 152 | `658826a7d287c9061152d12180fd46d43206e71779bcb4a28c1d9ddedd68ee1d->02a75885aef0cf5bb3c102830e4333cd71599539640b7d54c2db17e9b93699e2` (`OLD=0.0.83`, `NEW=0.0.101/0.0.105`) |
| `netregen-opt-module-lnx-arm64` | `W0023` | `PASS` (6128ms, `note=-`, `NETREGEN_SHARD_SAME`) | 154 | `368f383a0849db82941e3cd3eb4d311262a612545e849fea8ade11b1069bdbbb->f90bfdca32de5b62b8b18a2c174e6f358cfc267744b913f5f110b811ebca621f` (`OLD=0.0.83`, `NEW=0.0.101/0.0.105`) |
| `netregen-opt-module-osx-x86_64` | `W0024` | `PASS` (4777ms, `note=-`, `NETREGEN_SHARD_SAME`) | 156 | `bac3a5f2340db9b469cee929a368229070dde8e11a8aa471f3e234d471cf5aa6->afe860d499ea9ba2668bdc751cfc577c6c137c5d238de5da8849009ad0c7fc85` (`OLD=0.0.83`, `NEW=0.0.101/0.0.105`) |
| `netregen-opt-module-osx-arm64` | `W0025` | `PASS` (4726ms, `note=-`, `NETREGEN_SHARD_SAME`) | 158 | `44c8778db970de32e849406673a1e9653d7ee5e2e38346317050b726e872b702->cd78122a3d56131e4856be8f5743f199bc55bc90ae1f67aec89cd9b0e7e3cb62` (`OLD=0.0.83`, `NEW=0.0.101/0.0.105`) |
| `netregen-opt-module-win-x86_64` | `W0026` | `PASS` (6175ms, `note=-`, `NETREGEN_SHARD_SAME`) | 160 | `5f7d660c3960400a5e9df603bf21c36c0a0c9f78545ddf03388e01b85a26b905->5077f74d13469c85bb04c1960bf12203762f39cbc2cca42248eb12e1caeff005` (`OLD=0.0.83`, `NEW=0.0.101/0.0.105`) |
| `netregen-opt-module-win-arm64` | `W0027` | `PASS` (5224ms, `note=-`, `NETREGEN_SHARD_SAME`) | 162 | `388c9b716e6e4884de3c4104b1bb0f653c83b183ab5260c655a93fcfc48c9fae->334c4bee3d54ae1c41db0e54ad9772d1a6ee3b286ea962c177fa2ea59df52349` (`OLD=0.0.83`, `NEW=0.0.101/0.0.105`) |

## 4. 命令原行与输出摘录

```text
$ git show --stat c4a154dfea
commit c4a154dfea9eb63484d7fb0c94e1800ffc6aa38b
Date:   Tue Nov 24 15:47:27 2026 +0800
    old-product-assets: register v0.0.101 as new baseline (8827155B, bb715a8bd3)
 tools/old-product-assets.tsv | 1 +

$ grep -n "netregen-.*module" ~/.cache/unisacc/prerun/v0.0.106-2e8a19a342/strict-W00{16..27}.tsv
~/.cache/unisacc/prerun/v0.0.106-2e8a19a342/strict-W0016.tsv:4:netregen-module-lnx-x86_64	product-qualification	PASS	0	5f83ac93d201b252ea0b56098475be9e30045875cdf1ce692451cda095ceeb3c	16272	8.05	0	-
...
~/.cache/unisacc/prerun/v0.0.106-2e8a19a342/strict-W0027.tsv:4:netregen-opt-module-win-arm64	product-qualification	PASS	0	5f83ac93d201b252ea0b56098475be9e30045875cdf1ce692451cda095ceeb3c	5224	5.69	0	-

$ grep -n "netregen-.*module" ~/.cache/unisacc/prerun/v0.0.106-2e8a19a342/priority.tsv
90:netregen-module-lnx-x86_64	product-qualification	FAIL	2	934c7c0bc09a9a4d2e107b8d6bdc855553cf05f64dbb7d6c1e5fefdb42117dd1	4263	5.30	0	image=a1f5dac55af80b24627b2ae6992067b237e4c9ab927226aa1043c6a6fc1a1c1b->5941cc8e4364bb94f3548a25d054ba061b9ef1b818a3f273c818cc149842361a
...
162:netregen-opt-module-win-arm64	product-qualification	FAIL	2	934c7c0bc09a9a4d2e107b8d6bdc855553cf05f64dbb7d6c1e5fefdb42117dd1	4417	5.22	0	image=388c9b716e6e4884de3c4104b1bb0f653c83b183ab5260c655a93fcfc48c9fae->334c4bee3d54ae1c41db0e54ad9772d1a6ee3b286ea962c177fa2ea59df52349

$ grep -E "W00(1[6-9]|2[0-7])|netregen" ~/.cache/unisacc/prerun/v0.0.106-2e8a19a342/windows.tsv | head -n 12
W0016	1	heavy	1	netregen-module-lnx-x86_64
...
W0027	1	heavy	1	netregen-opt-module-win-arm64

$ cat ~/.cache/unisacc/prep-cache/old-product-material-669a28402182e821.dir/materials.tsv
old-product-material-v1
status	PREPARED-no-test-result
candidate-version	0.0.106
old-version	0.0.101
old-product	~/.cache/unisacc/prep-cache/old-product-cache/bb715a8bd3ed1237bb69d42d0cd1c2594c0a27c8e223b518ae39efe6b79eac82/unisacc.com	bb715a8bd3ed1237bb69d42d0cd1c2594c0a27c8e223b518ae39efe6b79eac82
```
