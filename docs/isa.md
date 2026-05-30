# ISA Reference

## Data model

- **Trit**: `int8_t` with values `-1` (NEG), `0` (ZERO), `1` (POS)
- **Tryte**: 9 trits, LSB at index 0, range -9841 to 9841

## Registers

| Index | Name | Purpose |
|-------|------|---------|
| 0 | ACC | Accumulator (primary) |
| 1 | B | Secondary register |
| 2 | SP | Stack pointer |
| 3 | FL | Flags (set by CMP/arithmetic) |
| 4 | PC | Program counter |

## Instruction encoding

Each instruction is one tryte (9 trits): 5 trits opcode, 4 trits register fields.
Instructions with an operand (LOAD, STORE, JMP, etc.) occupy a second tryte for the operand value.

## Instruction set

| Opcode | Mnemonic | Operand | Description |
|--------|----------|---------|-------------|
| 0 | NOP | — | No operation |
| 1 | HLT | — | Halt execution |
| 2 | ADD | rd, rs | rd ← rd + rs |
| 3 | SUB | rd, rs | rd ← rd − rs |
| 4 | MUL | rd, rs | rd ← rd × rs |
| 5 | DIV | rd, rs | rd ← rd ÷ rs; B ← remainder |
| 6 | NEG | rd | rd ← −rd |
| 7 | AND | rd, rs | rd ← min(rd, rs) per trit |
| 8 | OR | rd, rs | rd ← max(rd, rs) per trit |
| 9 | XOR | rd, rs | Per-trit addition mod 3 |
| 10 | NOT | rd | rd ← −rd per trit |
| 11 | LOAD | rd, val | rd ← val (immediate) |
| 12 | STORE | rs, addr | mem[addr] ← rs |
| 13 | MOV | rd, rs | rd ← rs |
| 14 | JMP | addr | PC ← addr |
| 15 | JE | addr | PC ← addr if FLAGS == 0 |
| 16 | JNE | addr | PC ← addr if FLAGS ≠ 0 |
| 17 | JG | addr | PC ← addr if FLAGS > 0 |
| 18 | JL | addr | PC ← addr if FLAGS < 0 |
| 19 | JZ | addr | PC ← addr if ACC == 0 |
| 20 | CALL | addr | Push PC; PC ← addr |
| 21 | RET | — | PC ← pop() |
| 22 | PUSH | rs | Stack ← rs |
| 23 | POP | rd | rd ← stack |
| 24 | IN | rd | rd ← stdin (as tryte) |
| 25 | OUT | rs | stdout ← rs (as char, low byte) |
| 26 | CMP | rd, rs | Set FLAGS based on rd − rs |
| 27 | LD | rd, addr | rd ← mem[addr] |
| 28 | SHL | rd, rs | rd ← rd × 3^(rs) (shift left) |
| 29 | SHR | rd, rs | rd ← rd ÷ 3^(rs) (shift right) |
| 30 | MOD | rd, rs | rd ← rd mod rs (remainder) |
| 31 | SWAP | rd, rs | Swap rd ↔ rs |

## Memory map

| Range | Size | Content |
|-------|------|---------|
| 0x0000–0x3FFF | 4096 trytes | ROM (program) |
| 0x4000–0x7FFF | 4096 trytes | RAM |
| 0x8000–0x8FFF | 512 trytes | Stack (grows down) |

## Assembler directives

- `.str "text"` — emit ASCII bytes of `text` as consecutive trytes
- `.db "text"` — alias for `.str`
- `; comment` — line comment
- `label:` — define label at current address

## Literals

- Decimal: `42`, `-7`
- Ternary: `+0-`, `-+0`, `000` — balanced ternary trit strings
- String: `"A"` — single-char ASCII value as operand
- Labels: `loop`, `done` — resolved to addresses
