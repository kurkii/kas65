lda $#0x33 ; load accumulator
sta $0x800

ldx $0x800
dex
stx $0x802
sbc ($0xFF), Y