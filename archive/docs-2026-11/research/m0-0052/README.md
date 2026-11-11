# M0 度量基线(0.0.51 发布版根,2026-11-10)

环境:克隆 /tmp/cc-m0(git clone --shared),根为 0.0.51 发布版 APE unisacc.com(6529460 字节)。机器 16 核,load average 4.7-6.4(共享机,有噪声)。

## (a) 逻辑分布普查
命令:`sh ./unisacc.com tools/logic-distribution.cx -- --sources .`(uptime 前 load 4.39)
耗时:14.9s wall(user 11.7s),低于 55s,不分片。
结果(git 跟踪的 src,cx,exec,seed,weights,tools 常规文件,总 15856911 字节):
- TABLE:1162 文件,6079120 字节,383373 ppm
- GENERATOR:73 文件,1490465 字节,branch_tokens 8705,93994 ppm
- NETWORK_REPRESENTATION:0 文件(.net/.tbl/.rows 不在跟踪源内)
- CLASSIC_MECHANISM:1219 文件,8287326 字节,branch_tokens 42823,522631 ppm
- semantic_decision_share=UNKNOWN;模块归属 UNATTRIBUTED(工具既有口径)。

## (b) 模型包体积与装载
体积命令:`sh ./unisacc.com tools/logic-distribution.cx -- --package unisacc.com`(现有工具,经包尾 footer 读出;未新增 m0-pkgload 工具)
结果:package_bytes=1763620,models_stored=1414263(40 模型),resources_stored=241070(77 资源),metadata=108287。
装载计时(各 5 次,毫秒,wall,load 约 5):
- `sh ./unisacc.com --version`:6 6 7 7 6(此路径不装载包,只是启动下限)
- `-c` 小程序冷(每次不同源):90 87 123 96 93;热(同源重复):100 89 92 103 89(-c 无结果缓存,冷热无差别)
- 默认-run 小程序冷(不同源):395 383 403 416 389;热(同源,runcache 命中):7 7 7 7 7
- 装载与编译相位未拆分:UNMEASURED(需产品内相位计时)。

## (c) 全键验证耗时
网/表材料:/home/devali/repos/unisacc/out/c49-slow-cdx-979b56e4/ctx-quiet/jobs 下 42 对 tbl-X/result.tbl 与 net-X/result.net(只读使用;来源是 c49 构造,不是 0.0.51 发布构造,未能证明与发布包同一权重,仅作度量对照)。
检查器:`sh ./unisacc.com tools/lab-loader/net-check.cx -o out/nc` 编译 3.98s;每网 `out/nc TBL NET`。
并行(xargs -P 16):总 1452ms wall,42/42 退出码 0,全部 table-equals-network;单网耗时之和 8487ms。
最慢网:warnparse 1131ms(observations=3250715 states=12553),其次 errorparse 1129ms。
串行总计:6965ms。
