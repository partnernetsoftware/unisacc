# Helper 家族生命周期

原生端 12 个 Helper 提供 `dispose`。家族测试为每种 Helper 调用实际初始化入口，检查实例由活动变为失活，并验证第二次释放返回 `-1`。固定的 three.js r160 参照页独立监听相应 Helper 的几何体和材质释放事件，每个所有者引用对应一次事件。DirectionalLightHelper 两条线共用材质，因此该材质应报告两次。

`ArrowHelper` 另有独立生命周期入口。`ArrowHelper_init` 复制方向和尺寸姿态，绑定轴线几何体、轴线材质、锥体几何体、锥体材质四组调度器和目标 ID。`copy` 和 `clone` 保留同一组资源引用，并各自拥有独立姿态。`dispose` 按上述顺序写出四份标准事件快照。每份快照先保存目标 ID 和监听器计数，再保存匹配的监听器 ID，由调用方分派。输出容量不足返回 `-2`，引用与输出重叠或重复释放返回 `-1`。预检拒绝时保持实例及输出原状。

`GridHelper` 和 `PolarGridHelper` 的 `copy`、`clone` 共享源位置和颜色缓冲，各自维护活动状态。调用方须在引用仍存续时保留底层缓冲的地址和内容。

`ArrowHelper_poseToJSON` 与 `ArrowHelper_poseFromJSON` 往返实际活动实例的姿态。格式是两组数组，第一组为根四元数的 `x,y,z,w`，第二组为轴线的 `scale.y`、锥体的 `position.y`、锥体的 `scale.y` 及锥体共用的 `scale.x/z`。输入畸形时保持姿态原状，正常导入保留资源引用。验收入口为 `sh ./unisacc.com ujs/tests/arrowhelper-pose-json.cx --`，用 `--mutate` 检查锥宽与锥高互换的反例。

Arrow 姿态数组只覆盖现有原生姿态字段。完整对象 JSON，包括根位置、名称、可见性、图层、任意子树与资源身份，仍待接入 `Object3D`。网格公共字段往返与渲染释放仍待完成，其余十类的复制与克隆同样未覆盖。

独立验收入口为 `sh ./unisacc.com ujs/tests/arrowhelper-lifecycle.cx --` 和 `sh ./unisacc.com ujs/tests/helper-family-grid-lifecycle.cx --`。分别追加 `--mutate` 可复现错误释放事件和遗漏源几何修改。

`sh ./unisacc.com ujs/tests/helper-family-lifecycle.cx --` 运行金值和 r160 对照，预期 `cases=12 failed=0`。在同一命令末尾加 `--mutate` 会跳过第一个原生实例的释放，预期第 0 例失败。
