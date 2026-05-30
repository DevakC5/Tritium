# Tritium

Balanced-ternary computer simulator in C99. Static library + CLI assembler/debugger.

## Build

```
make          # lib/libtrinary.a + bin/trc
make test     # run all tests (137 tests, 5 suites)
make clean    # remove build artifacts
make examples # run all .trc examples
```

## Usage

```
trc <file.trc>           # assemble and run
trc -d <file.trc>        # debug mode (single-step prompt)
trc -r                    # interactive REPL
trc --dump <file>        # disassemble (.trc source or .tbc binary)
trc --assemble <src>     # assemble .trc to .tbc binary
             <dst>
```

## Example

```asm
; hello.trc — prints "Hello!"
LOAD ACC, 72      ; 'H'
OUT ACC
; ... (one OUT per character)
HLT
```

## Architecture

- **Trit**: `int8_t` (-1, 0, 1)
- **Tryte**: 9 trits (values -9841 to 9841)
- **Memory**: 4K ROM (program), 4K RAM, 512-tryte stack
- **ISA**: 27 opcodes, 5 registers (ACC, B, SP, FL, PC)

Layered: `trit` → `gate`/`arith`/`mem` → `isa` → `cpu` → `vm` + `asm`/`disasm` → `cli`

## Documentation

See `docs/` for full ISA reference and binary format spec.
