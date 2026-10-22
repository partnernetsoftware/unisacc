# S 第0步 固定夹具：E1→e3 合同（stagecontract-e1-e3）

- 产品：HEAD 45424c65 干净导出构建的 unisacc-next.com，sha256 83e6e7e4624c1a6d06771af3347f152187468a01893665e0972860961ca2dafb
- 包：compiler.pkg sha256 71832a397eb9761af5c76a5984eead5b1b729d32aab52d89a6c2de3a239ffd67（路径 lnx/x86_64/tape/O0，e3 路由）
- 捕获：tests/middleflowrunner.c capture，源 tests/seals/middleflow-step0/accept-source.bytes，期望 0（accept）；accept/ 为整个捕获目录（input.bytes、attributes.bytes、resources.tsv、resource-*.bytes、output.bytes、status.txt）。
- 消费网络：e3.net 由 tests/modelstreamextract.cx 原样抽取，sha256 5e9ab6c45354f7ffa29c3ca8407d6330041b91dc49c0fd8a712eaa25e0ed0ea6
- 不含 e1 网络；e1 仅作为上游捕获来源。
- 2026-10-22 重捕（sizeof 行 / (**p)[N] 表刀，exec/parse2）：e3.net 由工作树候选包（compiler.pkg）经 modelstreamextract.cx 原样抽取，sha256 39429de4881c47819e8188f76d4ffc45e6a22d29faf28f4248411de3f27390b7；accept/ 以 middleflowrunner capture 重捕，八个文件与旧捕获逐字节同。
- 2026-10-22 再捕（(**p)(…) 声明符补级）：e3.net sha256 bf0c6998063f260b2a21067bb3f91616bdacb57d9c2cdad620fbb1464e4c4029；accept/ 与旧捕获逐字节同。
