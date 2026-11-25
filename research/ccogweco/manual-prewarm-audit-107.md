# 107 手工预热件格式审计(只读 stat;2026-11-25 ccogweco)

范围:cc 与各窗口为 107 做的手工预备:chain-preflight snap 缓存、sixtarget-precompile 缓存、tlib kit(ompgpt)、
prep-cache 下 old-product-cache 其它条目、run-cache、门 5 的 unisacc-cr-prep-cache、agy-flow 的
~/.unisacc-release-evidence-backups/0.0.107/src-dir。只 stat 读,未改任何文件或权限。

## 0 结论

- 大部分缓存由正规入口生成且格式合规(目录 0700、属主当前用户):tlib、prep-cache 根与 old-product 子目录、
  pf-snap-tools、chains、门 5 的 cr-prep-cache。
- 两处有手工拷贝痕迹或权限偏宽:
  (a) agy-flow 的 src-dir 顶层目录 **775**(组可写),文件混有 664(commit.txt、sources.tsv)与 600;
  (b) old-product 缓存条目 `<sha>/unisacc.com` 为 **755**,而工具写出应为 **0500**(old-product.cx:108)。
- release-run 的打包核对按 sha/字节绑定(pack_binding/need),**不核权限位**;所以以上两处不会在公开步骤被
  "权限拒绝",但会削弱证据卫生,且未来若工具收紧格式校验会被拒。

## 1 逐项 stat 结果

| 件 | 路径(简) | 权限/属主 | 是否正规入口 | 风险 |
|---|---|---|---|---|
| chain-preflight snap 工具缓存 | ~/.cache/unisacc/pf-snap-tools | 0700 目录;四个 *.native 0755 | 是(preflight 生成) | 无 |
| chains 根与 snap | ~/.cache/unisacc/chains | 0700 | 是 | 无 |
| tlib kit(ompgpt) | ~/.cache/unisacc/tlib 及 20+ 包子目录 | 0700 全部 | 是 | 无 |
| old-product 条目 5f83ac93… | prep-cache/old-product-cache/<sha>/ | 0700;fetch.lock 0600;**unisacc.com 0755** | 部分:lock 与目录正规,资产权限不符 0500 | 手工拷贝痕迹;当前不被拒(verify 只核类型/字节/sha/version),格式偏差 |
| old-product 其它条目 12fd0c57…、bb715a8b… | 同上 | 0700 目录(未逐文件读) | 未逐项核 | 低,建议同法复核 |
| run-cache | 未找到独立 run-cache 目录 | — | UNMEASURED | 无法判定 |
| sixtarget-precompile 缓存 | ~/.cache 与 /tmp/cc-chain-107 下 maxdepth 3 未见专门缓存目录 | — | UNMEASURED | 无法判定;可能在运行目录内 |
| 门 5 prep cache | /tmp/cc-chain-107/tmp/unisacc-cr-prep-cache | 0700 | 是(check-candidate-chain --prepare) | 无 |
| agy-flow src-dir | ~/.unisacc-release-evidence-backups/0.0.107/src-dir | **dir 0775**;cache-manager-*/lnx-* 0700;osx-*/win-*/p3.pkg 0600;commit.txt、sources.tsv 0664 | 疑似手工组装(权限不齐) | 组可写 + 664 文件;release-run 不核权限,但证据卫生差 |

## 2 公开/CI 前是否会被拒

- release-run 的输入绑定:`pack_binding`(release-run.cx:704-712)与 `need()` 按文件名取 sha/字节,
  不做权限或属主检查;因此 775 目录与 664 文件不会被自动拒绝。
- 风险在别处:(a) 775 目录允许同组进程改动 src-dir,公开后证据可被质疑来源;(b) old-product 资产 0755
  与工具产物 0500 不一致,若 0.1.x 收紧 verify 的权限检查,预热会 `REFUSE cache-directory/asset` 类失败;
  (c) CI 对草稿资产的核对只看 sha,不看本地权限,故 CI 不会被拒。
- 结论:当前不会因权限被拒,但不是"正规入口"的格式,属应改的卫生问题;不改判断口径。

## 3 建议(只写,不改)

1. src-dir:由 agy-flow 统一 chmod 0700 目录、0600 敏感文件(commit.txt、sources.tsv 可 0600),
   或改为由 release-run 的 --src-dir 组装入口生成,避免手工拼装。
2. old-product:重新走 `tools/old-product.cx -- 0.0.106 <ABS_CACHE>` 让工具自己 chmod 0500 并写 lock;
   或按正规入口重备后再把该条目用于预跑。
3. 对 12fd0c57…、bb715a8b… 两个条目同法核一遍 lock/资产权限与 sha。
4. run-cache 与 sixtarget-precompile 缓存本轮未找到独立目录,标 UNMEASURED;下次预热时记录实际路径与生成命令。
5. 108 候选(与前页一致):所有手工预热件加"格式核验"一步(路径、生成入口、权限、属主、bytes、sha)。

## 4 边界

- 本页只读;未改任何权限或缓存内容;未把"未被拒"写成"合规"。
