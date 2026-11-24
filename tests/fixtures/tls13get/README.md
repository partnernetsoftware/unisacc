chain-badsig.bin 确定性夹具

来源:把 tests/fixtures/tls/syn-cert.bin 的叶证书(第一张)与 tests/fixtures/tls/syn-cert-other.bin
的叶证书拼成一条合法 framed TLS Certificate 握手消息(type 11, ctx len 0, 两个证书条目)。第二张的
发行者/密钥与第一张不连,verify_sig 必拒,用作"链签名错"夹具。
syn-cert*.bin 由 H4 的确定性生成器产(固定 P-256 私钥、nonce=sha256(key||digest)),无随机。

复现:把同目录 gen-chain-badsig.cx.txt 复制为任意 X.cx,在仓根执行
  sh ./unisacc.com X.cx -- tests/fixtures/tls13get/chain-badsig.bin
其只做字节拼接,输出确定性。

产物 sha256(646 bytes):
a8e0eb085a9bb53bfaaf87a1e5a9e4f73180785e1f2fe66e66f73b0d58e16684
