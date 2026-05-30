# Tritium — Agents Guide

Balanced-ternary computer simulator in C99. Static library + CLI assembler/debugger.

## Build & test

| Command | What |
|---|---|
| `make` | Build `lib/libtrinary.a` + `bin/trc` |
| `make test` | Compile & run `./test_runner` (260 tests, 6 suites) |
| `make clean` | Remove `build/ lib/ bin/ test_runner` |
| `make examples` | Run all `examples/*.trc` through `bin/trc` |
| `make install` | Install to `$(PREFIX)` (default `/usr/local`) |

CI (`.github/workflows/ci.yml`): `make && make test && make examples`.

## Architecture

- **Library** (`lib/libtrinary.a`): all `src/*.c` **except** `cli.c`. Layered as: `trit` → `gate`/`arith`/`mem` → `isa` → `cpu` → `vm` + `asm`/`disasm`.
- **CLI** (`bin/trc`): `src/cli.c` linked with `-ltrinary`. Entrypoint: `main()` in `cli.c`.
- **Umbrella header**: `include/trinary.h` includes all src headers via `#include "../src/*.h"` paths.
- **Include paths**: `-Iinclude -Isrc` — both `#include "trit.h"` (src-local) and `#include "trinary.h"` (external) work.

## Memory map (source of truth: `cpu.c`)

| Range | Content |
|---|---|
| `0x0000–0x0FFF` | ROM / program (4096 trytes) |
| `0x1000–0x1FFF` | RAM (4096 trytes) |
| `0x1E00–0x1FFF` | Stack within RAM (512 trytes, grows down from top) |

SP initialised to `STACK_BASE + STACK_SIZE - 1` = 0x1FFF.
Addresses beyond 0x1FFF read as zero and writes are ignored.

## Code conventions

- **C99 strict**: `-std=c99 -Wall -Wextra -pedantic`. No `//` comments, no declarations after statements, no `for`-loop initial declarations.
- **Trit type**: `int8_t` with constants `TRIT_NEG` (-1), `TRIT_ZERO` (0), `TRIT_POS` (1).
- **Tryte**: 9-trit struct `{ Trit t[9] }`, LSB at index 0. Range -9841 to 9841.
- **No comments in code** unless the user explicitly asks.
- **No libc math** except `-lm` linked for tests only.

## Testing

Custom `minunit.h` in `test/`. Macros: `mu_assert(condition, "message")`, `mu_run_test(func)`, `mu_run_suite(suite)`.

6 suites: trit, gate, arith, cpu, asm, vm.

To add a new suite:
1. Create `test/test_<module>.c` with `void test_<module>_suite(void)` runner.
2. Add its `.c` to `TESTS` in `Makefile`.
3. Add decl + `mu_run_suite(test_<module>_suite);` to `test/test_runner.c`.

Test files include headers from `src/` directly (e.g. `#include "vm.h"` resolves via `-Isrc`).

## CLI

```
trc <file.trc>            # assemble & run
trc -d <file.trc>         # debug mode (breakpoints, step, list, mem, etc.)
trc -r                    # interactive REPL (multi-line, persistent state)
trc --dump <file>         # disassemble (.trc or .tbc)
trc --assemble <src> <dst> # assemble .trc to .tbc binary
```

## ISA (43 opcodes, 5-trit opcode + 4-trit reg encoding)

Registers: ACC (0), B (1), SP (2), FL (3), PC (4).

Instructions with an operand (LOAD, STORE, JMP, CALL, etc.) occupy a second tryte.

| Mnemonic | Operands | Description |
|----------|----------|-------------|
| `NOP` | — | No operation |
| `HLT` | — | Halt execution |
| `ADD` | rd, rs | rd ← rd + rs |
| `SUB` | rd, rs | rd ← rd − rs |
| `MUL` | rd, rs | rd ← rd × rs |
| `DIV` | rd, rs | rd ← rd ÷ rs; B ← rem |
| `NEG` | rd | rd ← −rd |
| `AND`/`OR`/`XOR`/`NOT` | rd[, rs] | Per-trit logic |
| `INC`/`DEC` | rd | rd ± 1 |
| `ABS` | rd | rd ← \|rd\| |
| `CMP` | rd, rs | Set FLAGS based on rd − rs |
| `LOAD` | rd, val | rd ← val (immediate) |
| `LD` | rd, addr | rd ← mem[addr] |
| `STORE` | rs, addr | mem[addr] ← rs |
| `MOV` | rd, rs | rd ← rs |
| `SWAP` | rd, rs | Swap rd ↔ rs |
| `SHL`/`SHR` | rd, rs | rd ×/÷ 3^(rs) |
| `MOD` | rd, rs | rd ← rd mod rs |
| `JMP`/`JE`/`JNE`/`JG`/`JL`/`JZ`/`JGE`/`JLE` | addr | Conditional branches |
| `JMPR`/`CALLR` | rs | Indirect jump/call |
| `CALL`/`RET` | addr/— | Subroutine call/return |
| `PUSH`/`POP` | rs/rd | Stack ops |
| `IN`/`OUT` | rd/rs | Character I/O |
| `OUTNUM` | rs | Print rs as decimal string |
| `OUTSTR`/`INSTR` | addr | Null-terminated string I/O |
| `RND` | rd | rd ← pseudo-random tryte |

## Assembler directives

- `.str "text"` / `.db "text"` — emit ASCII bytes as consecutive trytes
- `.db 0` / `.db 1` — emit numeric trytes (decimal or ternary)
- `.equ name val` — define constant
- `.space n` — reserve n trytes of zero
- `.fill n, val` — emit val repeated n times
- `; comment` — line comment
- `label:` — define label at current address
- String escapes: `\n`, `\t`, `\\`, `\"`

Literals: decimal (`42`), ternary (`+0-`), string (`"A"`), labels.

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
| `?`, `h`, `help` | Help |

## HL Compiler (`compiler/trc_compile.py`)

Python compiler that translates C-like syntax to Tritium assembly.

Usage: `python compiler/trc_compile.py source.hl [-o output.trc]`

Supported syntax:
- `let x = expr;` / `let x;` — variable declaration (RAM, each takes 1 tryte)
- `x = expr;` — assignment
- `if cond { ... } else { ... }` — conditional, `else` optional
- `while cond { ... }` — loop
- `print expr;` / `print "string";` — output
- `input x;` — read decimal from stdin into variable
- `// comment` / `/* block comment */` — comments

Operators: `+`, `-`, `*`, `/`, `%`, `>`, `<`, `>=`, `<=`, `==`, `!=`

Ternary boolean model: zero = false, non-zero = true.
Comparisons use CMP + conditional jumps (JLE/JGE/JNE/JE/JL/JG).
`if x` / `while x` tests truthiness via JZ.

Expression evaluation uses stack for intermediate values (PUSH/POP).
Variables stored in RAM starting at address 0x1000.
Strings emitted at end of program (after HLT), null-terminated.

## REPL

- Assembly lines **accumulate** until blank line runs them
- Dot-commands: `.q`, `.r`, `.load <file>`, `.h`, `.end`
- All debug commands available without dots
- State persists across runs; set breakpoints before continuing
