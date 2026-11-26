# toolsreg 门禁缺口根因分析与三层修复设计

- 分析对象: `v0.1.5` prerun 停机事件 (`tests/tools-compile.cx:315` `REFUSE missing registered shard`, 缺 `tools-compile-201`; 已于 `77b35cf08` 修复并重冻)
- 状态声明: 本文在 `v0.1.5` 正式链运行期间仅经静态阅读与只读 `git` 审计完成, **未执行任何编译、测试、生成器或批次**; 运行态验证项标记为 **`UNMEASURED`**, 留待解冻后落地实施.

## (1) 精确时间线与跨线提交

通过 `git log --first-parent` 逐提交核对 `tests/tools-compile-sources.tsv` 行数与 `tests/batches.tsv` 的 `tools-compile-*` 分片数 (`shards_need = ceil(sources / 8)`):

| 提交 | 分支/类型 | `sources` 行数 | 所需分片 `need` | 实有分片 `batches` | 状态 |
|---|---|---|---|---|---|
| `8fc2393ca` | `v0.1.5` | `1606` | `201` (`000..200`) | `201` | 合法 |
| `26ce8ea88` | `main` | `1601` | `201` (`000..200`) | `201` | 合法 |
| `01d77868c` | `v0.1.5` Merge `main` | `1608` | `201` (`000..200`) | `201` | 合法 (恰好填满第 201 片 `201*8=1608`) |
| `c37d97583` | `main` (`release-keepalive`) | `1603` | `201` (`000..200`) | `201` | 合法 (新增 2 个 `.cx`, 自身未跨 1608) |
| **`d5104212f`** | **`v0.1.5` Merge `main`** | **`1610`** | **`202` (`000..201`)** | **`201`** | **首次破洞 (`1608 + 2 = 1610`, 缺 `tools-compile-201`)** |
| `f120b047f` | `main` Merge `v0.1.5` | `1610` | `202` | `201` | 破洞合入 `main` |
| `85d271c2d` | `main` C0b arm freeze | `1610` | `202` | `201` | 破洞带入冻结与 prerun |
| `77b35cf08` | `main` 修复提交 | `1610` | `202` | `202` | 经 `register --tool --sync` 补齐 `tools-compile-201` |

## (2) 精确缺失检查与三重漏放机理

### 缺口 1 (直接根因): `merge=union` 叠加跨过 8 行分片边界, 且无 `pre-merge-commit` 钩子
- `.gitattributes:9,11` 将 `tests/batches.tsv` 与 `tests/tools-compile-sources.tsv` 均设为 `merge=union`.
- 合并提交 `d5104212f` 的两个父提交 (`01d77868c` `1608` 行与 `c37d97583` `1603` 行) 各自均只需 201 片 (`tools-compile-000..200`), 各自独立提交时 `precommit/toolsreg` 全部合法通过.
- `git merge` 对两份表做无冲突并集叠加: `tools-compile-sources.tsv` 从 `1608` 增至 `1610` 行 (需要 `202` 片), 而 `batches.tsv` 两边都只有 `000..200`, 合并后仍为 `201` 片, 凭空丢失 `tools-compile-201`.
- **核心缺失**: Git 对无冲突自动 `git merge` **不会触发 `pre-commit` 钩子**; `.git/hooks/` 仅有 `commit-msg`、`pre-commit`、`pre-push`, **缺少 `pre-merge-commit` 钩子**, 因此 `tools/precommit.cx` 中的 `toolsreg` 门禁在 `d5104212f`、`bba51626b`、`f120b047f` 三次合并中一次都未执行.

### 缺口 2: 后续提交命中 `precommit.cx` 增量读集盲区
- `tools/precommit.cx:179-180` 仅当暂存区包含 `tools/*.cx` 的 `A/D/R/T` 变动或 `tests/{manifests,batches.tsv,coverage.tsv,tools-compile-sources.tsv}` 变动时才执行 `toolsreg`.
- `d5104212f` 之后的 10+ 笔提交 (`77c818d26`..`a03168fa9`, `f4537e875`, `85d271c2d`) 仅修改 `build/*`、`release/*`、`exec/facts/*`、`cx/driver/*` 或对已有 `tools/*.cx` 做 `M` 修改, 全部被判为 `not_affected`, 无法暴露仓库中已存在的分片缺口.

### 缺口 3: 冻结门 (`release-freeze.cx`) 与起链门 (`chain-ready.cx`) 未纳入 `tools-compile --shards`
- `tests/tools-compile.cx:466-469` 在 `--shards` 模式下会顺序执行 `registry(0,0)` 与 `batches(shards)` (热缓存耗时仅 ~0.2 s), 直接在 `:315` 拦截缺失分片.
- 查证 `tools/release-freeze.cx` 与 `tools/chain-ready.cx` 均未调用 `tests/tools-compile.cx -- --shards .`, 导致该缺口从 `23:59:36` (`d5104212f`) 潜伏至 `03:27` 正式 prerun 才暴露.

### 旁证: `tools/register.cx --tool --sync` 本身无算法缺陷
- `tools/register.cx:1243-1285` 在显式执行 `--tool --sync` 时会重算 `sources` 清单、按 `ceil(count/8)` 补齐所有 `tools-compile-000..N-1` 批次及 manifest, 并以 `tests/tools-compile.cx -- --shards` 验真 (`77b35cf08` 即靠它一键修复). 问题在于 `git merge` 自动合并不会触发 `register --tool --sync`, 也没有任何自动门禁拦截.

## (3) 三层防再发修复设计 (解冻后落地, 单次总耗时 < 0.3 s)

1. **第 1 层 (合并点即时拦截, 最高优先): 新增 `pre-merge-commit` 钩子**
   - 在钩子安装器与 `.git/hooks/pre-merge-commit` 中接入轻量检查: 每次 `git merge` 生成合并提交前, 若合并后索引相对 `HEAD` 改动了 `tools/*.cx`、`tests/tools-compile-sources.tsv` 或 `tests/batches.tsv`, 立即执行 `tests/tools-compile.cx -- --shards` (基于索引暂存态, 同 `tools/precommit.cx:3751-3757`).
   - 命中缺片时立即拒绝合并提交并提示: `run: sh ./unisacc.com tools/register.cx -- --tool --sync && git add tests/ && git commit`.
2. **第 2 层 (冻结与起链硬兜底): 接入 `release-freeze.cx` 与 `chain-ready.cx`**
   - 在 `tools/release-freeze.cx` 的 `arm` (C0b) 前置检查与 `tools/chain-ready.cx` 中新增只读门禁 `tools-compile-shards`: 执行 `sh ./unisacc.com tests/tools-compile.cx -- --shards .`, 断言 `rc=0` 且输出正整数分片数 (耗时 ~0.2 s).
   - 即使经 `--no-verify` 或外部合入绕过本地钩子, 也必在冻结前/起链前拦死.
3. **第 3 层 (`pre-push` 兜底 + 具名报错增强)**:
   - 在 `.git/hooks/pre-push` 推送 `main`/`v0.*` 前增加 `tests/tools-compile.cx -- --shards .` 只读校验 (~0.2 s).
   - 增强 `tests/tools-compile.cx:315` 报错文案为: `REFUSE missing registered shard tools-compile-%03d (inventory=%d needs shards=%d; fix: sh ./unisacc.com tools/register.cx -- --tool --sync)`.

## (4) 解冻后验收计划 (当前 `UNMEASURED`)
1. 构造最小双分支夹具: 分支 A 含 `8k` 个 `tools/*.cx` (`k` 片), 分支 B 在同基点另加 2 个 `tools/*.cx` (`k` 片), 执行无冲突 `git merge`:
   - 修复前 (先红): 无冲突 merge 成功产生 `8k+2` 行却只有 `k` 片的坏提交;
   - 修复后 (转绿): `pre-merge-commit` 当场拒收并具名缺 `tools-compile-<k>`, 跑 `register --tool --sync` 后提交通过.
2. 在坏树上分别验证 `release-freeze --arm`、`chain-ready` 与 `pre-push` 均在 1 秒内具名拒收.
