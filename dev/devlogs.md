# Tritium Devlog

## Session 1 — Initial project scaffolding

### What was built

All core modules of the balanced-ternary computer simulator from scratch:

**Data layer** (`src/trit.c`, `src/trit.h`)
- Trit type as `int8_t` with `TRIT_NEG`/`TRIT_ZERO`/`TRIT_POS` constants
- Tryte as 9-trit struct, LSB at index 0
- Conversions: tryte ↔ int64, tryte ↔ string (`-`, `0`, `+`), char parsing

**Gate layer** (`src/gate.c`, `src/gate.h`)
- Per-trit: `not`, `and`, `or`, `xor`, `nand`, `nor`, `consensus`, `any`, `implies`
- Tryte-wide: `not`, `and`, `or`, `xor`

**Arithmetic** (`src/arith.c`, `src/arith.h`)
- Full 9-trit balanced-ternary: `add`, `sub`, `mul`, `div` (with remainder), `neg`, `abs`

**Memory** (`src/mem.c`, `src/mem.h`)
- 4096 trytes ROM, 4096 trytes RAM, 512-tryte stack at top
- Read/write/load/dump with bounds checking

**ISA** (`src/isa.c`, `src/isa.h`)
- 27 opcodes (OP_NOP through OP_CMP), 5 registers
- Instruction encoding: 5 trit opcode + 4 trit register fields
- Encode/decode, opcode name lookup

**CPU** (`src/cpu.c`, `src/cpu.h`)
- 5 registers: PC, SP, ACC, B, FLAGS
- Fetch-decode-execute step cycle, run loop with HLT detection
- 1M cycle guard to prevent runaway execution

**VM** (`src/vm.c`, `src/vm.h`)
- CPU + ROM + RAM glue (VM struct)
- `vm_run`, `vm_step`, `vm_load_program`, `vm_load_asm`, `vm_reset`, `vm_dump_state`

**Assembler** (`src/asm.c`, `src/asm.h`)
- Two-pass: 1st pass collects labels and encodes, 2nd resolves forward references
- Supports `;` comments, `label:` definitions, decimal literals, register names
- Error reporting via `AsmResult`

**Disassembler** (`src/disasm.c`, `src/disasm.h`)
- `disasm_one` and `disasm_all` — decodes tryte to mnemonic string

**CLI** (`src/cli.c`)
- Entrypoint `main()`: assemble & run, debug mode (`-d`), REPL (`-r`)
- Linked as standalone binary `bin/trc` against `lib/libtrinary.a`

**Build system** (`Makefile`)
- `make` → static library + CLI binary
- `make test` → compile & run test runner
- `make examples` → run all `.trc` files through the CLI
- `make clean` → remove all build artifacts

**Test framework and suites** (`test/`)
- Custom `minunit.h` (4 macros: `mu_assert`, `mu_run_test`, `mu_run_suite`)
- 4 suites: `test_trit` (6 tests), `test_gate` (7 tests), `test_arith` (5 tests), `test_cpu` (8 tests)

**Examples** (`examples/`)
- `hello.trc` — prints "Hello!" via OUT instructions
- `fib.trc` — Fibonacci sequence (infinite loop)

**Documentation**
- `dev/PLAN.md` — architecture overview, ISA reference, memory map, module API docs
- `AGENTS.md` — agent guide for future OpenCode sessions

### Infrastructure (later session)

- **`.gitignore`** — excludes `build/`, `lib/`, `bin/`, `test_runner`, `*.o`
- **`--dump` flag** — `run_dump()` in `cli.c:77` — assembles a `.trc` source file and prints full disassembly
- **`test/test_asm.c`** — 10 standalone assembler tests covering: simple assembly, operand encoding, forward/backward labels, comments, blank lines, register operands, undefined labels, unknown instructions
- **CI workflow** (`.github/workflows/ci.yml`) — runs `make`, `make test`, `make examples` on push/PR
- **Git init** — 36 files committed as root commit, build artifacts excluded by `.gitignore`

## Session 2 — New opcodes, debugger enhancements, REPL improvements

### New opcodes (5)
- **`LD rd, addr`** (27) — load tryte from `mem[addr]` into `rd` (fills gap: was no way to load from memory)
- **`SHL rd, rs`** (28) — shift left: `rd = rd × 3^(rs)`
- **`SHR rd, rs`** (29) — shift right: `rd = rd ÷ 3^(rs)`
- **`MOD rd, rs`** (30) — modulus: `rd = rd mod rs`
- **`SWAP rd, rs`** (31) — swap `rd ↔ rs`

### Debugger enhancements (`cli.c:run_debug`)
- **Breakpoints**: set (`b <addr>`), clear (`bc [addr]`), list (`bl`), all cleared on reset
- **Continue** (`c`/`r`) — runs until breakpoint or halt via `vm_run_until_breakpoint`
- **Step over** (`n`/`next`) — sets temp breakpoint at PC+2 for CALL, runs, clears it
- **Memory inspection** (`m <addr> [n]`) — shows n trytes with ternary string + decimal
- **Disassembly listing** (`l [n]`) — shows n instructions around PC with pointer
- **Help** (`h`/`?`) — lists all commands
- Persistent breakpoints in `VM` struct (32-slot array)

### REPL improvements (`cli.c:run_repl`)
- **Multi-line input** — assembly lines accumulate until blank line triggers run
- **Persistent state** — VM and breakpoints persist across runs (no reset)
- **Dot-commands**: `.q`, `.r`, `.load <file>`, `.h`, `.end`/`.run`
- **Debug commands available** — `s`, `n`, `c`, `b`, `bc`, `bl`, `m`, `l` work in REPL
- **`.load <file>`** reads a `.trc` file into the accumulator buffer

### Bug fixes
- **DIV remainder** (`cpu.c:118`): simplified always-true condition `REG_B == di.reg_dest || REG_B != di.reg_dest` → unconditional `cpu_set_reg(cpu, REG_B, rem)`
- **CMP disassembly** (`disasm.c:62-63`): now shows `CMP %s, %s` with both registers instead of bare `CMP`

### Files changed
- `src/isa.h` — added `OP_LD=27` through `OP_SWAP=31`, `OP_COUNT=32`
- `src/isa.c` — added 5 op_table entries
- `src/cpu.c` — added `power3()` helper, 5 new opcode cases, fixed DIV bug
- `src/disasm.c` — added `OP_LD` case, fixed `OP_CMP` to show registers
- `src/vm.h` — added breakpoint fields and function declarations
- `src/vm.c` — implemented breakpoint management + `vm_run_until_breakpoint`
- `src/cli.c` — full rewrite: unified command dispatch, enhanced debugger, multi-line REPL
- `docs/isa.md` — added opcodes 27–31 to instruction table

## Session 3 — Assembler directives, string escapes, VM tests, examples

### Assembler directives (`src/asm.c`)
- **`.equ name val`** — symbolic constants; values as decimal, ternary (`+-0`), or char (`"A"`); two-pass assembler means forward references work. Duplicate `.equ` raises error.
- **`.space N`** — emit N zero trytes (for data tables / buffers)
- **`.fill N, val`** — emit N trytes of val (decimal, ternary, or string first char)
- **String escapes** in `.str`/`.db`: `\n` (10), `\t` (9), `\r` (13), `\0` (0), `\\` (92), `\"` (34)

### New tests (`test/`)
- **`test_vm.c`** — new test suite (5 tests): binary I/O round-trip (`vm_write_binary`/`vm_read_binary`), breakpoint set/clear/has/run-until/max-count, invalid binary returns NULL
- **`test_asm.c`** — 15 new tests: `.equ` (decimal/ternary/string), `.equ` forward ref, duplicate `.equ` error, `.space`/`.fill` directives, string escapes (`\n`, `\t`, `\\`), `LD`/`SHL`/`SWAP`/`MOD` opcode encoding
- Wired `test_vm.c` into `Makefile` and `test_runner.c`

### New examples (`examples/`)
- **`bitops.trc`** — SHL/SHR/AND/OR/XOR/NOT/SWAP bitwise demo
- **`euclid.trc`** — Euclidean GCD using MOD + CALL/RET + `.equ`
- **`memory.trc`** — LD/STORE RAM data table using `.equ` constants

### Files changed
- `src/asm.c` — string escape handling, `.equ`/`.space`/`.fill` directive parsing
- `test/test_asm.c` — 15 tests added (29 total)
- `test/test_vm.c` — new VM test suite (5 tests)
- `test/test_runner.c` — wired in `test_vm_suite`
- `Makefile` — added `test/test_vm.c` to TESTS
- `examples/bitops.trc`, `examples/euclid.trc`, `examples/memory.trc` — new example programs

## Session 4 — 11 new opcodes (INC, DEC, ABS, JGE, JLE, JMPR, CALLR, OUTNUM, OUTSTR, INSTR, RND)

### New opcodes (11)

| Opcode | Mnemonic | Type | Description |
|--------|----------|------|-------------|
| 32 | INC rd | register | rd ← rd + 1 |
| 33 | DEC rd | register | rd ← rd − 1 |
| 34 | ABS rd | register | rd ← \|rd\| |
| 35 | JGE addr | branch | PC ← addr if FLAGS ≥ 0 |
| 36 | JLE addr | branch | PC ← addr if FLAGS ≤ 0 |
| 37 | JMPR rs | indirect jump | PC ← rs |
| 38 | CALLR rs | indirect call | Push PC; PC ← rs |
| 39 | OUTNUM rs | I/O | Print rs as decimal string |
| 40 | OUTSTR addr | I/O | Print null-terminated string at addr |
| 41 | INSTR addr | I/O | Read line into memory at addr |
| 42 | RND rd | register | rd ← pseudo-random tryte (full range) |

### Key implementation details

- **JMPR/CALLR/OUTNUM** take a single register argument — added to the assembler's special single-register parsing block (alongside PUSH/POP/IN/OUT), encoding the register in `reg_src`.
- **OUTSTR/INSTR** take an address operand (no registers) — handled by the existing operand-only assembler path.
- **RND** uses `rand() % 19683 - 9841` to cover the full tryte range (−9841..9841).
- **Disassembler** `default` case extended with `instr_uses_src(di.op)` check to display single-src register ops (JMPR/CALLR/OUTNUM).
- All opcodes fit in the existing 5-trit balanced ternary encoding (0–42, max theoretical 121).

### Files changed
- `src/isa.h` — added `OP_INC=32` through `OP_RND=42`, `OP_COUNT=43`
- `src/isa.c` — 11 new op_table entries
- `src/cpu.c` — 11 new switch cases, added `make_tryte()` static helper
- `src/disasm.c` — added `instr_uses_src` check to default case chain
- `src/asm.c` — added JMPR/CALLR/OUTNUM to single-register parsing
- `docs/isa.md` — added opcodes 32–42 to instruction table
- `test/test_cpu.c` — 8 new tests (inc, dec, abs, abs_positive, jge, jle, jmpr, callr)
- `test/test_asm.c` — 11 new assembler encoding tests

### Current test count

6 suites, 260 tests (78 test functions), all passing.
