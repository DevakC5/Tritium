# Tritium — Agents Guide

Balanced-ternary computer simulator in C99. Static library + CLI assembler/debugger.

## Build & test

| Command | What |
|---|---|
| `make` | Build `lib/libtrinary.a` + `bin/trc` |
| `make test` | Compile & run `./test_runner` (5 suites, 137 tests) |
| `make clean` | Remove `build/ lib/ bin/ test_runner` |
| `make examples` | Run all `examples/*.trc` through `bin/trc` |

Source files: `.trc` for assembly, `.c`/`.h` in `src/`.

## Architecture

- **Library** (`lib/libtrinary.a`): all `src/*.c` **except** `cli.c`. Layered as: `trit` → `gate`/`arith`/`mem` → `isa` → `cpu` → `vm` + `asm`/`disasm`.
- **CLI** (`bin/trc`): `src/cli.c` linked with `-ltrinary`. Entrypoint: `main()` in `cli.c`.
- **Umbrella header**: `include/trinary.h` includes all src headers via `#include "../src/*.h"` paths.
- **Include paths**: `-Iinclude -Isrc` — both `#include "trit.h"` (src-local) and `#include "trinary.h"` (external) work.

## Memory map

| Range | Content |
|---|---|
| `0x0000–0x3FFF` | ROM / program (4096 trytes) |
| `0x4000–0x7FFF` | RAM (4096 trytes) |
| `0x8000–0x8FFF` | Stack (grows down from top) |

## Code conventions

- **C99 strict**: `-std=c99 -Wall -Wextra -pedantic`. No `//` comments, no declarations after statements, no `for`-loop initial declarations.
- **Trit type**: `int8_t` with constants `TRIT_NEG` (-1), `TRIT_ZERO` (0), `TRIT_POS` (1).
- **Tryte**: 9-trit struct `{ Trit t[9] }`, LSB at index 0.
- **No comments in code** unless the user explicitly asks.
- **No libc math** except via `-lm` (linked for tests only, not library itself).

## Testing

Custom `minunit.h` in `test/`. Macros: `mu_assert(condition, "message")`, `mu_run_test(func)`, `mu_run_suite(suite)`.

To add a new test suite:
1. Create `test/test_<module>.c` with a `void test_<module>_suite(void)` runner.
2. Add its `.c` to `TESTS` in `Makefile`.
3. Add `void test_<module>_suite(void);` decl + `mu_run_suite(test_<module>_suite);` to `test/test_runner.c`.

Test files include headers directly from `src/` (e.g. `#include "vm.h"` resolves via `-Isrc`).

## CLI

```
trc <file.trc>            # assemble & run
trc -d <file.trc>         # debug mode (breakpoints, step, list, mem, etc.)
trc -r                    # interactive REPL (multi-line, persistent state)
trc --dump <file>         # disassemble (.trc or .tbc)
trc --assemble <src> <dst> # assemble .trc to .tbc binary
```

## ISA quick reference

32 opcodes. 5-trit opcode + 4-trit operand encoding. Registers: ACC, B, SP, FL, PC.

| Mnemonic | Operands | Description |
|----------|----------|-------------|
| `LOAD` | rd, val | rd ← val (immediate) |
| `STORE` | rs, addr | mem[addr] ← rs |
| `LD` | rd, addr | rd ← mem[addr] |
| `MOV` | rd, rs | rd ← rs |
| `SHL` | rd, rs | rd ← rd × 3^(rs) |
| `SHR` | rd, rs | rd ← rd ÷ 3^(rs) |
| `MOD` | rd, rs | rd ← rd mod rs |
| `SWAP` | rd, rs | Swap rd ↔ rs |
| `IN`/`OUT` | rd/rs | Character I/O |
| `CMP` | rd, rs | Set FLAGS for conditional jumps |
| `JMP`/`JE`/`JNE`/`JG`/`JL`/`JZ` | addr | Branching |
| `CALL`/`RET` | addr/— | Subroutine call/return |
| `PUSH`/`POP` | rs/rd | Stack ops |
| `ADD`/`SUB`/`MUL`/`DIV`/`NEG` | rd[, rs] | Arithmetic |
| `AND`/`OR`/`XOR`/`NOT` | rd[, rs] | Logic per trit |

## Debugger commands

| Command | Description |
|---------|-------------|
| `s`, `step` | Single-step one instruction |
| `n`, `next` | Step over CALL |
| `c`, `continue` | Run until breakpoint or halt |
| `b <addr>` | Set breakpoint |
| `bc [addr]` | Clear breakpoint(s) |
| `bl` | List breakpoints |
| `m <addr> [n]` | Show n trytes of memory |
| `l [n]` | List n instructions around PC |
| `R`, `reset` | Reset CPU |
| `q`, `quit` | Quit |

## REPL

- Assembly lines **accumulate** until blank line runs them
- Dot-commands: `.q`, `.r`, `.load <file>`, `.h`, `.end`
- All debug commands available without dots (e.g. `s`, `c`, `m 4000`)
- State persists across runs; set breakpoints before continuing
