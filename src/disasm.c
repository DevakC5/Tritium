#include "disasm.h"
#include "isa.h"
#include "trit.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void append_operand(char *buf, size_t buf_size, const Tryte *operand) {
    if (!operand) return;
    int64_t oval = tryte_to_int64(*operand);
    char tryte_str[16];
    tryte_to_str(*operand, tryte_str, sizeof(tryte_str));
    size_t blen = strlen(buf);
    snprintf(buf + blen, buf_size - blen, ", %ld (%s)", (long)oval, tryte_str);
}

char *disasm_one(Tryte instr, int addr, const Tryte *operand) {
    DecodedInstr di = decode_instr(instr);
    char buf[256];
    char tryte_str[16];

    tryte_to_str(instr, tryte_str, sizeof(tryte_str));

    const char *regs[] = {"ACC", "B", "SP", "FL", "??", "??", "??", "??"};
    const char *dst = (di.reg_dest >= 0 && di.reg_dest < 4) ? regs[di.reg_dest] : "?";
    const char *src = (di.reg_src >= 0 && di.reg_src < 4) ? regs[di.reg_src] : "?";

    switch (di.op) {
        case OP_NOP:
            snprintf(buf, sizeof(buf), "NOP");
            break;
        case OP_HLT:
            snprintf(buf, sizeof(buf), "HLT");
            break;
        case OP_RET:
            snprintf(buf, sizeof(buf), "RET");
            break;
        case OP_PUSH:
            snprintf(buf, sizeof(buf), "PUSH %s", src);
            break;
        case OP_POP:
            snprintf(buf, sizeof(buf), "POP %s", dst);
            break;
        case OP_IN:
            snprintf(buf, sizeof(buf), "IN %s", dst);
            break;
        case OP_OUT:
            snprintf(buf, sizeof(buf), "OUT %s", src);
            break;
        case OP_NEG:
        case OP_NOT:
            snprintf(buf, sizeof(buf), "%s %s", opcode_name(di.op), dst);
            break;
        case OP_LOAD:
            snprintf(buf, sizeof(buf), "LOAD %s", dst);
            append_operand(buf, sizeof(buf), operand);
            break;
        case OP_STORE:
            snprintf(buf, sizeof(buf), "STORE %s", src);
            append_operand(buf, sizeof(buf), operand);
            break;
        case OP_LD:
            snprintf(buf, sizeof(buf), "LD %s", dst);
            append_operand(buf, sizeof(buf), operand);
            break;
        case OP_CMP:
            snprintf(buf, sizeof(buf), "CMP %s, %s", dst, src);
            break;
        default:
            if (di.has_operand) {
                snprintf(buf, sizeof(buf), "%s", opcode_name(di.op));
                append_operand(buf, sizeof(buf), operand);
            } else if (instr_uses_src(di.op) && instr_uses_dest(di.op)) {
                snprintf(buf, sizeof(buf), "%s %s, %s", opcode_name(di.op), dst, src);
            } else if (instr_uses_src(di.op)) {
                snprintf(buf, sizeof(buf), "%s %s", opcode_name(di.op), src);
            } else if (instr_uses_dest(di.op)) {
                snprintf(buf, sizeof(buf), "%s %s", opcode_name(di.op), dst);
            } else {
                snprintf(buf, sizeof(buf), "%s", opcode_name(di.op));
            }
            break;
    }

    char full[512];
    snprintf(full, sizeof(full), "[%04d] %-24s ; %s", addr, buf, tryte_str);

    char *result = (char *)malloc(strlen(full) + 1);
    if (result) strcpy(result, full);
    return result;
}

char **disasm_all(const Tryte *code, size_t len, int start_addr) {
    if (len == 0) return NULL;
    char **lines = (char **)calloc(len + 1, sizeof(char *));
    if (!lines) return NULL;

    int out_idx = 0;
    for (size_t i = 0; i < len; ) {
        DecodedInstr di = decode_instr(code[i]);
        const Tryte *operand = NULL;
        if (di.has_operand && i + 1 < len)
            operand = &code[i + 1];
        lines[out_idx++] = disasm_one(code[i], start_addr + (int)i, operand);
        i += di.has_operand ? 2 : 1;
    }
    lines[out_idx] = NULL;
    return lines;
}
