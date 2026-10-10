# gate-infra 第 1 步只读定位回执（cc，2026-10-11 ~02:0x；机房主任 01:52）——不跑门、不改仓、不刷戳
## 方法
- 三个提交各建一个私有只读 worktree：/tmp/cc40-prep/gi1/wt-{b8ea2faa,ab4f75b8,a8a114a0}（HEAD 分别为 b8ea2faa543d…、ab4f75b83cf3…、a8a114a07699…，status 均 0 行）。
- **算法与声明三处相同**：`git diff b8ea2faa a8a114a0 -- tests/gatequeue.py tests/gatedeps.json tests/queuecheck.py` 为空。所以“各提交内算法”就是同一份。
- 复算脚本 /tmp/cc40-prep/gi1/inv.py（sha ff730eea…）逐行照抄 tests/gatequeue.py 第 303–315 行的清单逻辑（git 可见 + inventory_suffixes / all_files_trees / excluded_dirs 过滤）、第 126–139 行的 digest（[可执行位模式, sha256]）、第 206 行的 stamp（JSON sort_keys 的 sha256；portable() 在不设 PATH_KEYS 环境变量时是恒等，复算时 -u 了 UA/MODEL_COM/UA_RUN）。对 compilercheck 家族全部 6 个 reviewed_trees 计算。结果 JSON：inv-<提交>.json。

## 证明（读数）
### 1. 旧键核对
b8ea2faa 上复算 6 个目录的 stamp **全部等于** gatedeps.json 记录的审核戳（exec 9ae3c358…、include 30ec420a…、kernel 650b328e…、src 18ce7e3b…、unisa f485dcd7…、weights e91e1466…）。这同时验证了复算方法与 gatequeue 一致。
### 2. 三处 stamp 与成员差集
| 提交 | exec 成员数 | exec stamp | 与审核戳 | 其余 5 个目录 |
|---|---|---|---|---|
| b8ea2faa | 1408 | 9ae3c358… | 相等 | 全相等 |
| ab4f75b8 | 1409 | deec8f95… | **不等** | 全相等 |
| a8a114a0 | 1410 | 2d52b9bc… | **不等** | 全相等 |
- b8ea2faa → ab4f75b8：新增 **exec/opt/gen-delta.sh**；无删除、无变更。
- ab4f75b8 → a8a114a0：新增 exec/pp/gen-delta.sh；变更 exec/opt/gen-delta.sh、exec/pipeline/prepare.sh。
- 与 `git diff --name-status b8ea2faa a8a114a0 -- exec include kernel src unisa weights`（A opt/gen-delta.sh、M pipeline/prepare.sh、A pp/gen-delta.sh）一致。
### 3. 资格判定与 fallback（读代码，tests/gatequeue.py）
- 第 315 行：`audited = audited and inventory == entry['reviewed_trees'] and all(settings.get(k) for k in entry['required_settings'])`——任一 reviewed_tree 的 stamp 不等，audited 即为假。
- 第 334 行：tools 不缺；第 335–341 行：guards 全部匹配，且 code_trees 下的 .py 都在 guards 里。
- 未 audited 时（第 355 行起的分支）：指纹用 `global_inputs`（整树文件），所以 README.md 一改就失效。这正是 queuecheck 第 369 行测例看到的多出来的那 7 个套件。
### 4. 其他谓词逐项
- 命令：7 个 exec-driver-* 的声明命令都是 `./exec/c/compilercheck.sh <名>`，queuecheck 的 probe 直接用声明命令，命令匹配为真。
- required_settings = ['UA']：queuecheck 的 fixture 设置了 UA（第 350 行），为真。
- tools：fixture 的 PATH 里有 awk 替身，其余是宿主工具；三处 gatequeue/queuecheck 相同，b8ea2faa 时门是绿的，可推这一项为真（未单独采样，见“仍未知”）。
- guards：家族 10 个 guard 文件在 b8ea2faa..a8a114a0 之间没有变化（diff 为空）。
- code_trees：家族没有设 code_trees，这个谓词恒真。
- K2b 白名单：fixture 自己写了覆盖全部 probe 的白名单（queuecheck 第 346 行），不受仓库内容影响。
### 5. 7 个套件逐个
exec-driver-core-contracts、-core-dependencies、-core-modes、-core-build、-language-1、-language-2、-resources 同属 compilercheck 家族，共用同一组 reviewed_trees；因此它们的失效原因相同：**exec 的审核戳不等 → audited 为假 → 退回整树指纹 → 改 README 失效**。没有套件有单独的原因。

## 假设（仍未用运行证实的部分）
- queuecheck 的 fixture 是用 `source_root.rglob` 复制文件（不经 git），再在 fixture 里 git init/add 后算清单。在干净 worktree 上，复制集合与 git 可见集合相同，所以我的复算应与 fixture 内的计算一致；若在有未跟踪文件的检出上跑，fixture 会多出成员——这一点没有运行验证。
- 结论“gate-infra 红的原因是 exec/opt/gen-delta.sh 进入了 compilercheck 审核清单，使 exec 审核戳失配”是**静态复算 + 代码阅读**得到的唯一解释；ab4f75b8 上的红我此前实跑过一次（23:5x），b8ea2faa 上的绿来自当时的门日志。没有在本步重新运行。

## 仍未知（UNKNOWN）
- fixture 内实际计算出的 exec stamp（未运行 queuecheck）。
- tools 谓词在三处的实际取值（未单独采样）。

## 对之前口径的影响
inventory 里“gate-infra 与两刀有无因果未证明”——本步静态证据指向：红由 K5-1 的 ab4f75b8 新增 exec/opt/gen-delta.sh 引起（K5-1b、K5-1c 的 exec 改动让它继续失配）。是否据此改写 inventory 口径、是否需要第 2 步运行确认，交授权人定。

## 处置（不在本步）
(a) 真实审核 exec 的三处改动后更新审核戳；(b) 调整 compilercheck 闭包声明；(c) 维持红等冻结审核稳定点。均需另授。

## 收窄（cdx 02:1x）与补采
- **措辞收窄**：上文“唯一解释”“其他谓词都不是原因”**撤回**，改为：**已识别一个充分的静态原因**（exec 审核戳失配足以使 audited 为假、退回 global_inputs）；**并存原因未排除**。理由：tools 谓词没有采样，旧门绿不能证明当前工具状态；fixture 内实际 stamp 仍是 UNKNOWN。
- **补采（只读）**：
  - guards：三个提交上，家族 10 个 guard 文件的 sha256 **全部等于 gatedeps 记录值**（之前只说了“没有 diff”，现在是直接比对记录）。
  - 非普通文件：三个提交的 6 个审核目录里，除目录外**没有非普通文件**（lstat 检查）。所以 inv.py 省略 gatequeue 第 311 行“nonregular reviewed input”拒绝分支，在这三棵树上不影响结果。
- 仍 UNKNOWN：tools 谓词的实际取值；fixture 内的 exec stamp。

## 7 个套件逐列矩阵（cdx2 02:2x 要求；只读，由 gatedeps.json 与三份 inv-*.json 生成）
| 套件 | 家族 | 声明命令 | match | required_settings | guards（10 个均与记录相符） | exec 审核戳 b8ea / ab4f / a8a1 | 其余 5 目录 | tools | 推断 audited（b8ea / ab4f / a8a1） |
|---|---|---|---|---|---|---|---|---|---|
| exec-driver-core-build | compilercheck | ./exec/c/compilercheck.sh core-build | exact | UA（fixture 设 UA） | 是 | 等 / **不等** / **不等** | 全等 | UNKNOWN | 真* / **假** / **假** |
| exec-driver-core-contracts | compilercheck | ./exec/c/compilercheck.sh core-contracts | exact | UA（fixture 设 UA） | 是 | 等 / **不等** / **不等** | 全等 | UNKNOWN | 真* / **假** / **假** |
| exec-driver-core-dependencies | compilercheck | ./exec/c/compilercheck.sh core-dependencies | exact | UA（fixture 设 UA） | 是 | 等 / **不等** / **不等** | 全等 | UNKNOWN | 真* / **假** / **假** |
| exec-driver-core-modes | compilercheck | ./exec/c/compilercheck.sh core-modes | exact | UA（fixture 设 UA） | 是 | 等 / **不等** / **不等** | 全等 | UNKNOWN | 真* / **假** / **假** |
| exec-driver-language-1 | compilercheck | ./exec/c/compilercheck.sh language-1 | exact | UA（fixture 设 UA） | 是 | 等 / **不等** / **不等** | 全等 | UNKNOWN | 真* / **假** / **假** |
| exec-driver-language-2 | compilercheck | ./exec/c/compilercheck.sh language-2 | exact | UA（fixture 设 UA） | 是 | 等 / **不等** / **不等** | 全等 | UNKNOWN | 真* / **假** / **假** |
| exec-driver-resources | compilercheck | ./exec/c/compilercheck.sh resources | exact | UA（fixture 设 UA） | 是 | 等 / **不等** / **不等** | 全等 | UNKNOWN | 真* / **假** / **假** |

\* “真”还依赖 tools 谓词，tools 未采样，所以严格说是“除 tools 外的谓词都满足”。

另按 cdx2 收窄：“rglob 复制集合与 git 可见集合相同”只在**干净、无忽略文件的 worktree** 上成立，不是一般事实；fixture 的动态读数仍 UNKNOWN。

## 矩阵末列更正（cdx 02:2x）
上表最后一列在 b8ea2faa 上写的“真*”改读为 **“UNKNOWN（除 tools 外已核谓词都满足）”**：tools 没采样，b8ea2faa 的 audited 不能写成已证。ab4f75b8、a8a114a0 的“假”有充分静态依据（exec 审核戳不等即为假，与 tools 无关）。
