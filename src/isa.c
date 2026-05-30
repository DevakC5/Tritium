#include "isa.h"
#include <string.h>
#include <strings.h>

static const struct { Opcode op; const char *name; int has_op; int branch; int uses_dst; int uses_src; } op_table[] = {
    {OP_NOP,   "NOP",   0, 0, 0, 0},
    {OP_HLT,   "HLT",   0, 0, 0, 0},
    {OP_ADD,   "ADD",   0, 0, 1, 1},
    {OP_SUB,   "SUB",   0, 0, 1, 1},
    {OP_MUL,   "MUL",   0, 0, 1, 1},
    {OP_DIV,   "DIV",   0, 0, 1, 1},
    {OP_NEG,   "NEG",   0, 0, 1, 0},
    {OP_AND,   "AND",   0, 0, 1, 1},
    {OP_OR,    "OR",    0, 0, 1, 1},
    {OP_XOR,   "XOR",   0, 0, 1, 1},
    {OP_NOT,   "NOT",   0, 0, 1, 0},
    {OP_LOAD,  "LOAD",  1, 0, 1, 0},
    {OP_STORE, "STORE", 1, 0, 0, 1},
    {OP_MOV,   "MOV",   0, 0, 1, 1},
    {OP_JMP,   "JMP",   1, 1, 0, 0},
    {OP_JE,    "JE",    1, 1, 0, 0},
    {OP_JNE,   "JNE",   1, 1, 0, 0},
    {OP_JG,    "JG",    1, 1, 0, 0},
    {OP_JL,    "JL",    1, 1, 0, 0},
    {OP_JZ,    "JZ",    1, 1, 0, 0},
    {OP_CALL,  "CALL",  1, 1, 0, 0},
    {OP_RET,   "RET",   0, 0, 0, 0},
    {OP_PUSH,  "PUSH",  0, 0, 0, 1},
    {OP_POP,   "POP",   0, 0, 1, 0},
    {OP_IN,    "IN",    0, 0, 1, 0},
    {OP_OUT,   "OUT",   0, 0, 0, 1},
    {OP_CMP,   "CMP",   0, 0, 1, 1},
    {OP_LD,    "LD",    1, 0, 1, 0},
    {OP_SHL,   "SHL",   0, 0, 1, 1},
    {OP_SHR,   "SHR",   0, 0, 1, 1},
    {OP_MOD,   "MOD",   0, 0, 1, 1},
    {OP_SWAP,  "SWAP",  0, 0, 1, 1},
    {OP_INC,   "INC",   0, 0, 1, 0},
    {OP_DEC,   "DEC",   0, 0, 1, 0},
    {OP_ABS,   "ABS",   0, 0, 1, 0},
    {OP_JGE,   "JGE",   1, 1, 0, 0},
    {OP_JLE,   "JLE",   1, 1, 0, 0},
    {OP_JMPR,  "JMPR",  0, 1, 0, 1},
    {OP_CALLR, "CALLR", 0, 1, 0, 1},
    {OP_OUTNUM,"OUTNUM", 0, 0, 0, 1},
    {OP_OUTSTR,"OUTSTR", 1, 0, 0, 0},
    {OP_INSTR, "INSTR",  1, 0, 0, 0},
    {OP_RND,   "RND",    0, 0, 1, 0},
};

int instr_has_operand(Opcode op) {
    if (op < 0 || op >= OP_COUNT) return 0;
    return op_table[op].has_op;
}

int instr_is_branch(Opcode op) {
    if (op < 0 || op >= OP_COUNT) return 0;
    return op_table[op].branch;
}

int instr_uses_dest(Opcode op) {
    if (op < 0 || op >= OP_COUNT) return 0;
    return op_table[op].uses_dst;
}

int instr_uses_src(Opcode op) {
    if (op < 0 || op >= OP_COUNT) return 0;
    return op_table[op].uses_src;
}

int instr_uses_regs(Opcode op) {
    return instr_uses_dest(op) || instr_uses_src(op);
}

static void trits_from_reg(int reg, Trit *hi, Trit *lo) {
    int val = reg - 4;
    int lo_val = val % 3;
    int hi_val = val / 3;
    if (lo_val == 2)  { lo_val = -1; hi_val += 1; }
    if (lo_val == -2) { lo_val = 1;  hi_val -= 1; }
    *hi = (Trit)hi_val;
    *lo = (Trit)lo_val;
}

static int reg_from_trits(Trit hi, Trit lo) {
    return (int)hi * 3 + (int)lo + 4;
}

Tryte encode_instr(Opcode op, int reg_dest, int reg_src) {
    Tryte result;
    tryte_zero(&result);

    Tryte op_val;
    tryte_from_int64((int64_t)op, &op_val);

    for (int i = 0; i < 5; i++)
        result.t[i] = op_val.t[i];

    Trit hi, lo;
    trits_from_reg(reg_dest, &hi, &lo);
    result.t[5] = hi;
    result.t[6] = lo;

    trits_from_reg(reg_src, &hi, &lo);
    result.t[7] = hi;
    result.t[8] = lo;

    return result;
}

DecodedInstr decode_instr(Tryte instr) {
    DecodedInstr di;
    di.op = OP_NOP;
    di.reg_dest = 0;
    di.reg_src = 0;
    di.has_operand = 0;

    Tryte op_part;
    tryte_zero(&op_part);
    for (int i = 0; i < 5; i++)
        op_part.t[i] = instr.t[i];
    int64_t op_val = tryte_to_int64(op_part);
    if (op_val >= 0 && op_val < OP_COUNT)
        di.op = (Opcode)op_val;

    di.reg_dest = reg_from_trits(instr.t[5], instr.t[6]);
    di.reg_src = reg_from_trits(instr.t[7], instr.t[8]);
    di.has_operand = instr_has_operand(di.op);
    return di;
}

Opcode opcode_from_name(const char *name) {
    for (int i = 0; i < OP_COUNT; i++)
        if (strcasecmp(name, op_table[i].name) == 0)
            return op_table[i].op;
    return OP_HLT;
}

const char *opcode_name(Opcode op) {
    if (op < 0 || op >= OP_COUNT) return "???";
    return op_table[op].name;
}
