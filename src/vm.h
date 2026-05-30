#ifndef TRINARY_VM_H
#define TRINARY_VM_H

#include "cpu.h"
#include "mem.h"

typedef struct {
    CPU cpu;
    Memory *rom;
    Memory *ram;
} VM;

VM *vm_create(void);
void vm_free(VM *vm);
int vm_load_program(VM *vm, const Tryte *code, size_t len);
int vm_load_asm(VM *vm, const char *source);
int vm_step(VM *vm);
int vm_run(VM *vm);
void vm_dump_state(VM *vm, FILE *out);
void vm_reset(VM *vm);

#endif
