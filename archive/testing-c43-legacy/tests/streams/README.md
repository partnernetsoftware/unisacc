# 七流独立开发产物

`runner.c` 复用通用执行器；`pack.cx` 从现有候选中裁出七个单模型包，不重建权重。当前目标为 Linux x86_64、O2，产物是可直接执行的静态 ELF（后缀为 .com）。

| 文件 | 流 | 主输入 → 主输出 |
|---|---|---|
| unisacc-stream1.com | e2 | src.c → pp.locations |
| unisacc-stream2.com | e1 | pp.locations → tokens.locations 与属性帧 |
| unisacc-stream3.com | e3 | tokens.locations 与属性帧 → tape.text |
| unisacc-stream4.com | e4 | tape.text → tape.text |
| unisacc-stream5.com | prune | tape.text → tape.text |
| unisacc-stream6.com | lower | tape.text → target.text |
| unisacc-stream7.com | elf | target.text → image.bytes |

每个程序只携一个原候选流模型。为保持资源合同，首版原样携带候选的全部74项资源，尚未按流裁剪资源闭包。流模型改变可以单独定位；共享执行器或实际共享资源改变仍须核对受影响流。

## 构建

在仓库根执行。输出目录必须不存在；复建时换新目录，工具拒绝覆盖旧证据。

```sh
mkdir -p out
tests/bound 55 sh ./unisacc.com -O2 -fno-trim-libc tests/streams/runner.c -o out/stream-runner.com
tests/bound 55 sh ./unisacc.com tests/streams/pack.cx -- out/stream-runner.com ./unisacc.com out/streams
```

pack 的第二个输入也可以是独立P3 compiler.pkg；产品输入目前只支持未签名的末尾UNIPKG1 footer。streams.json记录源包、执行器、各模型原记录及产物SHA。当前开发元数据为schema2，使用stream_units_in_source与stream_unit字段，results.tsv首列亦为stream_unit；P3与UNIPKG1字节协议不变。既有out/streams-v4的schema1保留为历史证据。

## 管道与逐层调试

无参数或 INPUT 为 - 时读stdin，结果写stdout，诊断写stderr。额外参数为逻辑源名及include目录，默认源名stdin.c。E1的既有USLATTR1属性帧随管道传递，E3将它导入显式属性资源；不能丢掉它而只传token正文。文件调试也可用UNISA_ATTRIBUTES导出实际旁路属性。

```sh
set -o pipefail
printf 'int main(void){return 7;}\n' |
  out/streams/unisacc-stream1.com |
  out/streams/unisacc-stream2.com |
  out/streams/unisacc-stream3.com |
  out/streams/unisacc-stream4.com |
  out/streams/unisacc-stream5.com |
  out/streams/unisacc-stream6.com |
  out/streams/unisacc-stream7.com > out/probe.elf
```

给各阶段固定输入即可并发独立重放，无需每次从C源码重跑上游。check.cx保存逐层输入、输出、属性和真实状态，再并发重放七层，比较H0正常-O2镜像、实际执行返回7，并验证一个具名ELF越界拒绝：

```sh
tests/bound 55 sh ./unisacc.com tests/streams/check.cx -- "$PWD" "$PWD/out/streams" "$PWD/unisacc.com" "$PWD/out/streams-check"
```

追加最后参数`--constructor`可验证真实非零属性：constructor先将全局值设为7，E1必须实际产出count=1属性记录。当前check固定0.0.43候选H0身份，覆盖plain与constructor两份源码及该负例，结果记录并前后复核七个产物SHA。这是模块开发基线，未宣称覆盖各流全部行为。七流之外的tapecheck、绑定适配及平台合同继续由对应单测和集成检查覆盖；阶段产物不作为发行验收通过证明。
