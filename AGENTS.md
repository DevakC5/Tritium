# Tritium — Agents Guide

Balanced-ternary computer simulator in C99. Static library + CLI assembler/debugger.

## Build & test

| Command | What |
|---|---|
| `make` | Build `lib/libtrinary.a` + `bin/trc` |
| `make test` | Compile & run `./test_runner` (4 suites) |
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
trc <file.trc>       # assemble & run
trc -d <file.trc>    # debug mode (single-step prompt)
trc -r               # interactive REPL
trc --dump <file>    # disassemble bytecode (stub)
```

## ISA quick reference

27 opcodes. 5-trit opcode + 4-trit operand encoding. Registers: ACC, B, SP, FL, PC.
Key: `LOAD`/`STORE` take an address operand; `MOV` copies between registers; `IN`/`OUT` do character I/O; `CMP` sets `FLAGS` for conditional jumps (`JE`, `JNE`, `JG`, `JL`, `JZ`).

## What's missing / stale

- `docs/` and `paper/` are empty — no design docs exist beyond `dev/PLAN.md`.
- No CI, no lint/formatter/typecheck, no `.gitignore`, no pre-commit hooks.
- `--dump` flag in CLI is a stub (returns error).
- No assembly test suite — `test/test_cpu.c` tests CPU via inline assembly strings but there is no standalone assembler test file.
