# Trinary Computer Simulator (C)

## Overview
A static library `libtrinary.a` implementing a complete balanced-ternary CPU, plus a CLI tool for assembly, debugging, and execution. Written in C99, built with `make`.

## Data Model

**Trit:** `int8_t` with constants `TRIT_NEG = -1`, `TRIT_ZERO = 0`, `TRIT_POS = 1`.

**Tryte:** 9 trits (3⁹ = 19,683 values), least significant trit at index 0.
```
typedef struct { Trit t[9]; } Tryte;
```

**Memory:** Contiguous array of trytes.
```
typedef struct { Tryte *cells; size_t size; } Memory;
```

## Memory Map
```
0x0000 – 0x3FFF  ROM / Program (16K trytes)
0x4000 – 0x7FFF  RAM (16K trytes)
0x8000 – 0x8FFF  Stack (grows downward from 0x8FFF)
```

## Instruction Set
Instruction encoding: 5 trits for opcode (243 possible), 4 trits for operand.

| Opcode (ternary) | Mnemonic | Description |
|---|---|---|
| 00000 | NOP | No operation |
| 00001 | HLT | Halt execution |
| 00002 | ADD rd, rs | rd ← rd + rs |
| 00003 | SUB rd, rs | rd ← rd - rs |
| 00004 | MUL rd, rs | rd ← rd × rs |
| 00005 | DIV rd, rs | rd ← rd ÷ rs |
| 00006 | NEG rd | rd ← -rd |
| 00010 | AND rd, rs | rd ← min(rd, rs) |
| 00011 | OR rd, rs | rd ← max(rd, rs) |
| 00012 | XOR rd, rs | rd ← (rd + rs) mod 3 per trit |
| 00013 | NOT rd | rd ← -rd per trit |
| 00014 | LOAD rd, addr | rd ← mem[addr] |
| 00015 | STORE rs, addr | mem[addr] ← rs |
| 00016 | MOV rd, rs | rd ← rs |
| 00020 | JMP addr | PC ← addr |
| 00021 | JE addr | PC ← addr if FLAGS == 0 |
| 00022 | JNE addr | PC ← addr if FLAGS ≠ 0 |
| 00023 | JG addr | PC ← addr if FLAGS > 0 |
| 00024 | JL addr | PC ← addr if FLAGS < 0 |
| 00025 | JZ addr | PC ← addr if ACC == 0 |
| 00026 | CALL addr | Push PC; PC ← addr |
| 00100 | RET | PC ← pop() |
| 00101 | PUSH rs | Stack ← rs |
| 00102 | POP rd | rd ← stack |
| 00103 | IN rd | rd ← stdin (as tryte) |
| 00104 | OUT rs | stdout ← rs (as char) |

## Directory Structure
```
Tritium/
├── Makefile
├── README.md
├── include/
│   └── trinary.h          # Umbrella header
├── src/
│   ├── trit.h / trit.c     # Trit/Tryte types, conversions
│   ├── gate.h / gate.c     # Logic gates
│   ├── arith.h / arith.c   # Arithmetic
│   ├── mem.h / mem.c       # Memory
│   ├── isa.h / isa.c       # Opcodes, encode/decode
│   ├── cpu.h / cpu.c       # CPU state, step, run
│   ├── asm.h / asm.c       # Two-pass assembler
│   ├── disasm.h / disasm.c # Disassembler
│   ├── vm.h / vm.c         # ROM + RAM + CPU glue
│   └── cli.c               # `trc` CLI tool entry point
├── test/
│   ├── minunit.h           # Minimal test header
│   ├── test_trit.c
│   ├── test_gate.c
│   ├── test_arith.c
│   ├── test_cpu.c
│   ├── test_asm.c
│   └── test_runner.c       # Main runner
├── examples/
│   ├── hello.trc           # Print "Hello, World!"
│   └── fib.trc             # Compute Fibonacci
└── dev/
    ├── TODO.md
    └── PLAN.md              # This file
```

## Build System
```
make          → lib/libtrinary.a + bin/trc
make test     → build & run all unit tests
make clean    → remove build/ artifacts
make examples → assemble & run example .trc files
```

## CLI Usage
```
trc <file.trc>         # assemble & run
trc -d <file.trc>      # debug mode (single-step, print regs)
trc -r                 # interactive REPL
trc --dump <file.tbc>  # disassemble bytecode
```

## Modules (detail)

### trit.h / trit.c
- `trit_to_char(Trit t)` — returns `'-'`, `'0'`, or `'+'`
- `trit_from_char(char c)` — parses `-`, `0`, `+`
- `tryte_from_int64(int64_t val, Tryte *out)` — converts integer to balanced ternary
- `tryte_to_int64(Tryte t)` — converts back
- `tryte_to_str(Tryte t, char *buf, size_t len)` — e.g. `"+0-+00-+0"`
- `tryte_from_str(const char *s, Tryte *out)` — parses string

### gate.h / gate.c
- `trit_not`, `trit_and`, `trit_or`, `trit_xor` — per-trit gates
- `trit_nand`, `trit_nor`, `trit_xnor` — negated gates
- `trit_consensus(Trit a, Trit b)` — ternary AND (min)
- `trit_any(Trit a, Trit b)` — ternary OR (max)
- `tryte_not`, `tryte_and`, `tryte_or`, etc. — vector versions

### arith.h / arith.c
- `tryte_add(Tryte a, Tryte b)` — full adder across 9 trits
- `tryte_sub(Tryte a, Tryte b)` — subtract via negate + add
- `tryte_mul(Tryte a, Tryte b)` — shift-and-add
- `tryte_div(Tryte a, Tryte b, Tryte *rem)` — division with remainder
- `tryte_neg(Tryte a)` — invert all trits
- `tryte_cmp(Tryte a, Tryte b)` — returns -1, 0, or 1

### isa.h / isa.c
- `Opcode` enum with all instruction names
- `tryte_encode_instr(Opcode op, Tryte operand)` — pack opcode + operand
- `instr_decode(Tryte instr, Opcode *op, Tryte *operand)` — unpack
- `instr_name(Opcode op)` — return mnemonic string
- `instr_arity(Opcode op)` — registers vs immediate operands

### cpu.h / cpu.c
- `typedef struct { Tryte pc, sp, acc, b, flags; } CPU`
- `cpu_init(CPU *cpu)` — zero all regs
- `cpu_step(CPU *cpu, Memory *rom, Memory *ram)` — fetch, decode, execute one
- `cpu_run(CPU *cpu, Memory *rom, Memory *ram)` — loop until HLT
- `cpu_flag_*` helpers for FLAGS register

### asm.h / asm.c
- Two-pass: 1st pass collects labels and encodes, 2nd pass resolves forward refs
- Supports: `label:` definitions, `;` comments, ternary literals (`+0-`), decimal literals (`123`), string literals (`"hello"`)
- Output: `Tryte *` bytecode + error reporting

### disasm.h / disasm.c
- `disasm_instr(Tryte instr)` — returns mnemonic string
- `disasm_range(Tryte *code, size_t len)` — full disassembly listing

### vm.h / vm.c
- `typedef struct { CPU cpu; Memory rom, ram; } VM`
- `vm_create(size_t rom_size, size_t ram_size)`
- `vm_free(VM *vm)`
- `vm_load_program(VM *vm, Tryte *code, size_t len)`
- `vm_step(VM *vm)` — single step
- `vm_run(VM *vm)` — run to completion
- `vm_dump_state(VM *vm)` — print all regs + stack

## Test Framework
Single-header `minunit.h` (~30 lines) with `mu_assert`, `mu_run_test` macros.
Each module has a test file; `test_runner.c` runs all suites.
