#ifndef TRINARY_VM_H
#define TRINARY_VM_H

#include "cpu.h"
#include "mem.h"

typedef struct {
    CPU cpu;
    Memory *rom;
    Memory *ram;
    int breakpoints[32];
    int num_breakpoints;
} VM;

VM *vm_create(void);
void vm_free(VM *vm);
int vm_load_program(VM *vm, const Tryte *code, size_t len);
int vm_load_asm(VM *vm, const char *source);
int vm_step(VM *vm);
int vm_run(VM *vm);
void vm_dump_state(VM *vm, FILE *out);
void vm_reset(VM *vm);

void vm_set_breakpoint(VM *vm, int addr);
void vm_clear_breakpoint(VM *vm, int addr);
void vm_clear_all_breakpoints(VM *vm);
int vm_has_breakpoint(VM *vm, int addr);
int vm_run_until_breakpoint(VM *vm);

int vm_write_binary(const char *path, const Tryte *code, size_t len);
Tryte *vm_read_binary(const char *path, size_t *len);

#endif
