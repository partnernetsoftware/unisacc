# omp-bdy 暂停状态（2026-11-02）

## 位置
- 慢车道纪元第 12 步（两代 GEN），卡在 product-build 前置。
- 正在改 tools/cxlib/rebuildcontext.cx 的 rb_record：按角色分流 split2 聚合（product）/ 单文件 sha（bootstrap）。

## 未提交文件（工作区）
- `tools/cxlib/rebuildcontext.cx`（MM：已 staged 部分 + 工作区新增诊断 printf，含 use productpkg + rb_record 加 product_root 参数 + canonical 补 product_kind/product_digest + 9 处调用点改造）
- `plans/w5-patches/07-pp-negative.patch`（M：已标注「作废待重做」）

## 未完成步骤
- 第 12 步：rb_record 修复未验证通过（product-build 仍报 build-record-identity-or-settings，已加诊断 printf 待看 DIAG 输出）。

## 回滚点
- `tools/cxlib/rebuildcontext.cx`：git checkout HEAD -- 该文件即可还原（诊断 printf 在 /tmp/rebuildcontext.cx.bak 有备份）。
- 其余纪元产物全在 out/epoch-2026-11-02/（私有，不在主树）。

## 待命
- 不提交、不跑新命令，等 omp-cc 恢复通知。

## 关键诊断线索（bg_93 已出，暂停后补记）
- `DIAG rb_record product_root=(bootstrap) an=624 h=d93c1b24... out_artifact=41e4660d... out_source= kind=split2 n=401`
- 关键：product_root 显示 `(bootstrap)`，即 rb_record 走的是 bootstrap 分支（单文件 sha d93c1b24），
  但 build.json 的 artifact_sha256=41e4660d（split2 聚合）→ mismatch。
- 推断：product-build 报错路径不是 251 行（我传了 root 作 product_root），而是另一个读
  root/unisacc.com.build.json 的 bootstrap 角色调用点（可能 50 行 rb_kernel_context 里读 c->bootstrap，
  而 c->bootstrap 恰好也指向 root/unisacc.com）。需恢复后查 c->bootstrap 与 root/unisacc.com 是否同路径。
- 修复方向待定：bootstrap 角色若实际指向 product（root/unisacc.com），需区分「真 bootstrap（Root41-bootstrap.com）
  vs product 启动器（unisacc.com）」——两者内容相同（都 624B 薄头 d93c1b24），但 build.json 口径不同
  （bootstrap 单文件 sha vs product split2 聚合）。
