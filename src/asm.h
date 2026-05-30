#ifndef TRINARY_ASM_H
#define TRINARY_ASM_H

#include "trit.h"
#include "isa.h"

typedef struct {
    int valid;
    char *error_msg;
    int error_line;
} AsmResult;

Tryte *asm_assemble(const char *source, size_t *len);
AsmResult asm_assemble_ex(const char *source, Tryte **code, size_t *len);

#endif
