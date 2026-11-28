@target win/x86_64
@data -
@data_len 0
@bss 0
@relocs -
_start:
.frame 8248
nop
.frame 4096
nop
.frame 4095
nop
sub64 rsp, rsp, rax
nop
.frame -8248
ret
.end
