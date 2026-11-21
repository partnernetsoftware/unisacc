# 几何生成器家族证据

`geometry-family-evidence.tsv` 列出 r160 的 21 个核心几何生成器及 2 个扩展生成器。每行指向实际产品入口、现有独立金值测试和 r160 参照页。现有的 `g_geometry.cx` 在每个网格生成测试中调用 `GeometryBuffers_asGeometry`，并通过 `BufferGeometry_clone` 调用共享的 `BufferGeometry_copy`。金值验证复制后的组、平移包围盒及源顶点。扩展类的原测试还需要 r160 扩展模块目录。

`geometry-json-struct.cx` 和 `geometry-json-load-struct.cx` 分别验证通用 BufferGeometry JSON 的写入和读回。证据索引校验门的接线与文件。逐行执行清单第四列的 `.cx` 门可复验浏览器结果，核心类直接运行 `cx -I ujs/tests <门文件>`，两项扩展类还需在 `--` 后传入 r160 扩展模块目录。23 项已实跑通过，其中 ShapeGeometry、SphereGeometry、TetrahedronGeometry、TorusGeometry 在并行构造返回 rc=2 后单路复验通过。各生成器的类型专属参数复制和 JSON 往返仍有缺口。

`PlaneGeometry_fromJSON` 是新增的参数读取入口。它接受含字面字段 `width`、`height`、`widthSegments`、`heightSegments` 的 JSON 对象，缺省值均为 1，分段值向下取整。它复用原生 `PlaneGeometry_generate` 填写调用者已初始化的 `GeometryBuffers`。输入、38 字节临时区和输出缓冲互相分离；无效值、容量不足和重叠区间返回 `-1`。`PlaneGeometry_toJSON` 另经公共对象入口写出原始未取整的分段参数、尺寸、UUID 和名称。C6 门 `geometry-plane-public.cx` 逐项对照完整参数对象、恢复网格、深复制和释放事件目标。在同一门追加 `--mutate` 与 `--mutate-dispose` 可分别验证参数和释放负控。

(1) 索引校验运行 `sh ./unisacc.com ujs/tools-ux/gen-geometry-family.cx -- --check`，预期 `core=21 addons=2 failed=0`。

(2) 平面 JSON 验收运行 `sh ./unisacc.com ujs/tests/geometry-family-plane-json.cx --`，比较 r160 参数读取后的顶点、法线、UV、索引和 15 项共享复制检查。在命令末尾加 `--mutate` 会修改输入宽度，预期由既有金值拒绝。
