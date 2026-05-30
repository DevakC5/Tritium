#ifndef TRINARY_ISA_H
#define TRINARY_ISA_H

#include "trit.h"

typedef enum {
    OP_NOP   = 0,
    OP_HLT   = 1,
    OP_ADD   = 2,
    OP_SUB   = 3,
    OP_MUL   = 4,
    OP_DIV   = 5,
    OP_NEG   = 6,
    OP_AND   = 7,
    OP_OR    = 8,
    OP_XOR   = 9,
    OP_NOT   = 10,
    OP_LOAD  = 11,
    OP_STORE = 12,
    OP_MOV   = 13,
    OP_JMP   = 14,
    OP_JE    = 15,
    OP_JNE   = 16,
    OP_JG    = 17,
    OP_JL    = 18,
    OP_JZ    = 19,
    OP_CALL  = 20,
    OP_RET   = 21,
    OP_PUSH  = 22,
    OP_POP   = 23,
    OP_IN    = 24,
    OP_OUT   = 25,
    OP_CMP   = 26,
    OP_LD    = 27,
    OP_SHL   = 28,
    OP_SHR   = 29,
    OP_MOD   = 30,
    OP_SWAP  = 31,
    OP_COUNT = 32
} Opcode;

typedef enum {
    REG_ACC = 0,
    REG_B   = 1,
    REG_SP  = 2,
    REG_FL  = 3,
    REG_PC  = 4,
    REG_COUNT = 5
} Register;

typedef struct {
    Opcode op;
    int reg_dest;
    int reg_src;
    int has_operand;
} DecodedInstr;

int instr_has_operand(Opcode op);
int instr_is_branch(Opcode op);
int instr_uses_dest(Opcode op);
int instr_uses_src(Opcode op);
int instr_uses_regs(Opcode op);

Tryte encode_instr(Opcode op, int reg_dest, int reg_src);
DecodedInstr decode_instr(Tryte instr);
Opcode opcode_from_name(const char *name);
const char *opcode_name(Opcode op);

#endif
