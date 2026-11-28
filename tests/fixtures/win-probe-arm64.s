@target win/arm64
@data -
@data_len 0
@bss 196880
@relocs -
_start:
spinit x7, 196880
.frame 8248
nop
.frame 4096
nop
.frame 4095
nop
sub64 x7, x7, x0
nop
.frame -8248
ret
.end
