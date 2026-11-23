# R72 临时编译缓存回收

私有归档基线 a6e066d0b0a29ab6caa7c25e72cb1e03474adf5f，旧件为已装 0.0.89。补丁 r72-product.patch 的 SHA256 为 d9ce67d7b65531bbcb9004ffec74daad0ba3cfce530f9c02a4231e2c4c76e7cb。补丁在共享检出通过 git apply --check，未提交。

宿主和 CX0 在缓存入口持有目录共享租约，启动时能够取得独占租约才扫描。仅回收超过 86400 秒的本用户普通缓存文件与遗留暂存文件，实际命中按需续龄。扫描有界，跳过符号链接。旧版编译器不持有该租约，新旧件并行验证使用隔离的 TMPDIR。

| 验收 | 结果 | 原始记录 |
|---|---|---|
| 已装 0.0.89 | 6/2，过期文件未删除、下次启动未回收 | r72-before.log |
| 私有开发件 | 8/0，四路冷编译产物相同，热态新增文件数为 0 | r72-good-final.log |
| 年龄阈值改为 0 并重建 | 7/1，仅近期文件保护断言转红 | age-zero.diff、r72-zero-final.log |
| 活租约、命中续龄、CX0 与符号链接 | 5/0 | lease-probe-final.log |
| 四个输入的 -c 与 -o 逐字节对照 | 8/8 SAME | ab.log |
| 两个单元的六目标交叉编译 | 12/12，运行态仅验证 Linux x86_64 | cross.log |
| CX0 双投影、包接口与摘要 | 两投影 EXACT，两包接口 bad=0 | cx0-projection-final.log、layer-plan-check.log、layer-compile-check.log、iface-hash-final.log |

dev-product 首次冷构建撞到现有 55 秒上限。使用保留的私有加载器执行 cx-dev --reload 并由 entry-build 重建实际执行入口，最终开发件 SHA256 为 652fcbb6ba9fb3caa63ce822eec9cc6155adabe753a4773402dd6fb9ae3785be。见 dev-good/log/build.log、reload-good.log、entry-good-final.log。

闭包补证待处理。基线的 0.0.87 归档能读出每目标 99 个单元，但 core-closure --check 已报 DRIFT。R72 新增 cxscratch 后旧收据被具名拒绝，evidence unit count differs from use closure。集成需使用包含新增单元的六目标真实构造收据。见 core-closure-base.log、core-closure-good.log。
