# 扩展包清单格式（CXWORK2，草稿）

日期：2026-11-02。基线：`cx/workspace/packages.tsv` 的 `CXWORK1`（P/I/C/D/T/G）。
结论：**不原地改 CXWORK1**；新增 `CXWORK2`，只加包元数据 `E`、core 依赖 `R`、库产出 `A`，其余记录原样复用。

## 1. 记录集

```text
CXWORK2
E  package  namespace  version  kind  entry-module  binding
R  package  core       module
A  package  artifact-id  kind  path
P  package  owned-entry  artifact  recipe  seconds        # 复用
I  package  implementation|interface|test  logical-name  path  # 复用
C  package  source                                      # 复用
D  package  normal|build  package                       # 复用
T  package  test-id  source  seconds                    # 复用
G  package  implementation|interface|test  logical-name  dir  # 复用
```

- `E` 每包恰好一条，且出现在该包所有引用之前；`package` 全局唯一，建议 `crypto`/`sysx`/`memx`/`textx`/`csih`。
- `namespace` 是导入前缀（`crypto.`、`sysx.`…），必须等于该包每个 `I` 模块名的第一段。
- `entry-module` 是首选冒烟入口，不排斥同包其他模块；必须有一条 `I ... implementation <entry-module> <path>`。
- `kind`：`lib|app`；`binding`：`source|ffi|mixed`（见第 4 节）。
- `version`：SemVer（`0.1.0` 起）；发布后不可原地改同一版本。
- `R`：只允许冻结 core 清单里的模块；`core` 固定字面量；模块名用导入名（`cx0`、`mem.heap`、`sys.io`）。
- `A`：`P` 之外的产出（`lib|cxif|data`）；库包没有 `P` 时必须用 `A` 声明可交付物，纯源码包可暂缺。
- `D`：ext→ext 依赖；`normal|build` 语义不变；必须无环。
- `T`：`test-id` 必须已登记 `tests/batches.tsv`；未登记只能标草稿，不能记 PASS。
- `P`/`I`/`C`/`G` 的路径一律相对仓根；包目录固定 `ext/<package>/`。

## 2. 字段映射：哪些复用，哪些新增

| 需求 | 直接复用 | 新增/约定 |
|---|---|---|
| 包名 | `P.package` 同名字段 | `E.package` 为唯一权威；`P` 可省略（纯库） |
| 命名空间 | 无 | `E.namespace`；校验 `I` 模块前缀 |
| 版本 | 无 | `E.version`（SemVer） |
| 入口模块 | `I` 可放 entry | `E.entry-module` 明示首选入口 |
| 模块清单 | `I implementation <module> <path>` | `logical-name` 用完整模块名 |
| 接口清单 | `I interface <iface-id> <path>` | 接口文件/CXIF 仍按 `I` 声明 |
| core 依赖 | 无 | `R package core <module>` |
| ext 依赖 | `D normal <package>` | 依赖包必须有自己的 `E` |
| 产出物 | `P.artifact`（单入口/可执行） | `A` 声明额外库/CXIF |
| 测试批次 | `T`（test-id/source/seconds） | test-id 与 batches.tsv 对齐 |
| 构建配方 | `P.recipe`、`P.seconds` | 不变 |
| 目录资源 | `G` | 不变 |

## 3. 校验规则（实现前先冻结）

1. 只接受 `CXWORK1` 与 `CXWORK2`；1 的语义不变，`E/R/A` 仅在 2 生效。
2. 每个 package 一条 `E`；`P/I/R/C/D/T/G` 引用的 package 必须已声明。
3. `entry-module`、每个 `I` 模块必须以 `namespace + "."` 开头；入口必须存在。
4. `R` 的 core 模块必须在冻结 allowlist；`ext` 不得出现在 `R`（用 `D`）。
5. 产品层与 core 的 `#pragma unisa use ext.*` 为硬错误，由新棘轮 `cx-ext-boundary` 检查。
6. ext 包不得 `C` 编译产品源（`src/`、`cx/runtime`、`cx/compiler`、`cx/driver`），不得直接写平台 `#if`/`__syscallN`；平台差异只留 core 的 P0 与 sysx 包内。
7. `T` 的 source、seconds 必须存在且 ≤55；测试批次未登记只算 PREPARED，不算 PASS。
8. `A`/`P.artifact` 不得重名；`D` 无环；`E.version` 变更才允许改变包 API 基线。

## 4. 后绑定选择规则（摘要）

能编译期解析的 cx 源码包，走 `ext/<pkg>/` 路径 + `#pragma unisa use <ns>.<module>`；
需要宿主 C ABI、且重写成本/风险更高的能力，走 core `sys.ffi` 运行期绑定（FFI 是 core 的对外窗口，不在 sysx）。完整规则见
`out/ext-draft/binding-choice.md`。ext 包不得把宿主库暴露给产品层，也不得让产品链接 ext。

## 5. 清单示例（只示意，不是现状）
```text
CXWORK2
E crypto crypto 0.1.0 lib crypto.sha256 source
R crypto core cx0
I crypto implementation crypto.sha256 ext/crypto/sha256.cx
I crypto interface crypto.sha256.ext ext/crypto/sha256.ext
T crypto cx-l0-crypto-sha256 tools/check-crypto-sha256.cx 55

E textx textx 0.1.0 lib textx.fmt source
R textx core cx0
R textx core mem.heap
D textx normal memx
I textx implementation textx.fmt ext/textx/fmt.cx
T textx cx-l0-fmt tools/check-cx-fmt.cx 55

E csih csih 0.1.0 app csih source
P csih apps/csih/csih.cx csih unisacc 55
I csih implementation csih apps/csih/csih.cx
D csih normal sysx
D csih normal textx
T csih TODO-csih-selftest apps/csih/check.cx 55
```

## 6. 迁移顺序与边界
1. 先实现 `CXWORK2` 解析/校验（只读 `CXWORK1` 兼容），再加 `cx-ext-boundary` 棘轮。
2. `crypto`（15 模块，产品 0 引用）→ `sysx`（12 模块，apps/csih 先用）→ `memx` → `textx`。
3. `text.build` 是第一个跨包依赖：`textx -> memx`；`memx.arena` 若仍直接 libc，迁移时改走 core `mem.heap`。
4. `csih` 只作 app 占位，最后迁出；不在产品链，不参与产品资格。
5. 每步字节等价 + 七个棘轮；测试批次未登记的先补 `register`，不删断言、不缩覆盖。

## 7. 未决

- `sys.sig`/`sys.dlcall` 与 `core.*` 间接使用需用最近 receipt 的实际链接集合再定层。
- `A` 的 CXIF/库格式未定，首版允许纯源码包无 `A`；ext 依赖首版只允许同仓精确版本。
