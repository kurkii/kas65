# kas65 - 6502 assembler

kas65 is an assembler for the 6502 microprocessor. It implements every instruction and addressing mode specified by the architecture

## Addressing modes

### Accumulator
`opc, a`
### Absolute
`opc $llhh`
### Immediate
`opc #$xx`
### Implied
`opc`
### Indirect
`opc ($llhh)`
### Relative
`opc @$zz`
### Zeropage
`opc %$ll`
## Labels
Labels in kas65 are specified by the `.lb` directive, like so:

`.lb <name>:`

They must be placed on a new line on its own, seperate from source code

Labels are dereferenced with `[ ]` brackets

```
0000:   ldx #0
0002:   .lb my_loop ; declare the label 
0002:   inx
0003:   cpx, #100
0005:   bne [my_loop] ; dereference the label
0007:   brk
```

## Directives

All directives in kas65 begin with the `%` character.

Constant directives are dereferenced with `{ }` curly braces 

### `%db` - Insert data

With the `%db` directive you can insert either a byte (8-bit), word (16-bit), tribyte (24-bit) or string into the executable

`%db byte|word|tribyte|string, <value>`

```
0000:   sta $0x100
0003:   %db byte, 0xff ; inserts byte 0xff at the current memory location ($0003)
0004:   ldx $0x100
0007:   %db word, 0x1000 ; inserts word 0x1000 at the current memory location ($0007)
0009:   jmp [end]
0012:   %db string, "Hello, world!\0"
0027:   .lb end:
0027:   %db tribyte, 0x8d00ff ; inserts tribyte at the current memory location ($0027)
```

### `%bc` - Byte constant

Defines a constant which holds a byte (8-bit) value:

`%bc <name>, <value>`

```
0000:   %bc my_constant, 0x23
0000:   lda $#{my_constant}
```

### `%wc` - Word constant

Defines a constant which holds a word (16-bit) value:

`%wc <name>, <value>`

```
0000:   %wc my_constant, 0xffff
0000:   ldx $#{my_constant}

```

### `%tc` - Tribyte constant

Defines a constant which holds a tribyte (24-bit) value:

`%tc <name>, <value>`

```
0000:   %wc my_constant, 0xffffff
0000:   ldx $#{my_constant}

```

### `%og` - Origin directive

Sets the offset of the addresses which labels hold

`%og <value>`

```
0000:   %og 0x100
0000:   jmp [label] ; the origin is set to 0x100, so [label] is actually set to `0x104` instead of `0x04`
0003:   brk
0004:   .lb label:
0004:   rts
```

