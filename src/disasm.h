#ifndef TRINARY_DISASM_H
#define TRINARY_DISASM_H

#include "trit.h"

char *disasm_one(Tryte instr, int addr, const Tryte *operand);
char **disasm_all(const Tryte *code, size_t len, int start_addr);

#endif
