# S 第0步 固定夹具：E1→e3 合同（stagecontract-e1-e3）

- 产品：HEAD 45424c65 干净导出构建的 unisacc-next.com，sha256 83e6e7e4624c1a6d06771af3347f152187468a01893665e0972860961ca2dafb
- 包：compiler.pkg sha256 71832a397eb9761af5c76a5984eead5b1b729d32aab52d89a6c2de3a239ffd67（路径 lnx/x86_64/tape/O0，e3 路由）
- 捕获：tests/middleflowrunner.c capture，源 tests/seals/middleflow-step0/accept-source.bytes，期望 0（accept）；accept/ 为整个捕获目录（input.bytes、attributes.bytes、resources.tsv、resource-*.bytes、output.bytes、status.txt）。
- 消费网络：e3.net 由 tests/modelstreamextract.cx 原样抽取，sha256 5e9ab6c45354f7ffa29c3ca8407d6330041b91dc49c0fd8a712eaa25e0ed0ea6
- 不含 e1 网络；e1 仅作为上游捕获来源。
