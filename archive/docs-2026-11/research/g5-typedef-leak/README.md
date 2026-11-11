# 0.0.37 C2 g5: typedef leaks across units in a multi-unit reference build

`ua a.c b.c` fails `b.c:4:12: error: expected ';'` (cc and `ua b.c a.c` pass): the file-scope
typedef `code` from a.c is still a type name when b.c uses `code` as a parameter.
zlib 1.3.1 hits it as inftrees.h `typedef struct {...} code;` then trees.c bi_reverse
`code >>= 1, res <<= 1;` (reported at trees.c:98, the real line is 158: line mapping is off too).
Same family as L1′ (per-unit macro pool): per-unit state must reset between units.
Fix belongs in the L1′ input window (front_parse typedef table reset per unit).

对拍（cc / 未修参考 / 修后参考，退出码）：a+b 0/1/0，b+a 0/0/0；同名异型 s1+s2+sm 与反序 0/0/0；跨单元结构与签名 g1+g2 与反序 0/0/0。补丁 typedef-reset.patch 待参考窗口。
