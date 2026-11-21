# Helper 家族生命周期

原生端 12 个 Helper 提供 `dispose`。家族测试为每种 Helper 调用实际初始化入口，检查实例由活动变为失活，并验证第二次释放返回 `-1`。固定的 three.js r160 参照页独立监听相应 Helper 的几何体和材质释放事件，每个所有者引用对应一次事件。DirectionalLightHelper 两条线共用材质，因此该材质应报告两次。

`ArrowHelper` 的原生实现目前只有方向、长度和姿态记录，缺少资源对象及释放入口。原生 Helper 记录没有接入 `Object3D` 的节点存储和生命周期，尚无家族级 `copy`、`clone` 或 JSON 出入接口。测试保留这些覆盖缺口。

`sh ./unisacc.com ujs/tests/helper-family-lifecycle.cx --` 运行金值和 r160 对照，预期 `cases=12 failed=0`。在同一命令末尾加 `--mutate` 会跳过第一个原生实例的释放，预期第 0 例失败。
