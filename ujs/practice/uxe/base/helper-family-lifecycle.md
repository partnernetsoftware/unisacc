# Helper 家族生命周期

原生端 12 个 Helper 提供 `dispose`。家族测试为每种 Helper 调用实际初始化入口，检查实例由活动变为失活，并验证第二次释放返回 `-1`。固定的 three.js r160 参照页独立监听相应 Helper 的几何体和材质释放事件，每个所有者引用对应一次事件。DirectionalLightHelper 两条线共用材质，因此该材质应报告两次。

`ArrowHelper` 另有独立生命周期入口。`ArrowHelper_init` 复制方向和尺寸姿态，绑定轴线几何体、轴线材质、锥体几何体、锥体材质四组调度器和目标 ID。`copy` 和 `clone` 保留同一组资源引用，并各自拥有独立姿态。`dispose` 按上述顺序写出四份标准事件快照。每份快照先保存目标 ID 和监听器计数，再保存匹配的监听器 ID，由调用方分派。输出容量不足返回 `-2`，引用与输出重叠或重复释放返回 `-1`。预检拒绝时保持实例及输出原状。

`GridHelper` 和 `PolarGridHelper` 的 `copy`、`clone` 共享源位置和颜色缓冲，各自维护活动状态。调用方须在引用仍存续时保留底层缓冲的地址和内容。

`ArrowHelper_poseToJSON` 与 `ArrowHelper_poseFromJSON` 往返实际活动实例的姿态。格式是两组数组，第一组为根四元数的 `x,y,z,w`，第二组为轴线的 `scale.y`、锥体的 `position.y`、锥体的 `scale.y` 及锥体共用的 `scale.x/z`。输入畸形时保持姿态原状，正常导入保留资源引用。验收入口为 `sh ./unisacc.com ujs/tests/arrowhelper-pose-json.cx --`，用 `--mutate` 检查锥宽与锥高互换的反例。

Arrow 姿态数组只覆盖现有原生姿态字段。完整对象 JSON，包括根位置、名称、可见性、图层、任意子树与资源身份，仍待接入 `Object3D`。网格公共字段往返与渲染释放仍待完成。

`AxesHelper` 的复制品与克隆品共享位置、颜色及颜色版本字。若调用方回收或重新初始化原版本字存储，须先退役所有引用它的记录。`BoxHelper` 复制源物体绑定并共享位置、索引、位置版本、包围球和材质颜色。共享几何状态所在的原存储同样须保留至所有引用退役。`Box3Helper` 复制到已初始化记录时保留目标的 Box3 绑定，克隆共享几何但初始无 Box3。调用方可用 `Box3Helper_bindBox` 重新绑定。三类共用验收 `sh ./unisacc.com ujs/tests/helper-family-shared-geometry.cx --`。追加 `--mutate` 会错误替换复制目标的 Box3 绑定。

r160 实测 CameraHelper、SkeletonHelper 与四种灯光 Helper 的无参克隆抛出 `TypeError`。这些类的已初始化对象之间仍可复制。DirectionalLightHelper、HemisphereLightHelper、SpotLightHelper 和 PlaneHelper 的默认复制会追加子节点，原生固定布局尚未覆盖。其余七类原生复制与克隆仍待补。

独立验收入口为 `sh ./unisacc.com ujs/tests/arrowhelper-lifecycle.cx --` 和 `sh ./unisacc.com ujs/tests/helper-family-grid-lifecycle.cx --`。分别追加 `--mutate` 可复现错误释放事件和遗漏源几何修改。

`sh ./unisacc.com ujs/tests/helper-family-lifecycle.cx --` 运行金值和 r160 对照，预期 `cases=12 failed=0`。在同一命令末尾加 `--mutate` 会跳过第一个原生实例的释放，预期第 0 例失败。
