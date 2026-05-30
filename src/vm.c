#include "vm.h"
#include "asm.h"
#include "disasm.h"
#include <stdlib.h>

VM *vm_create(void) {
    VM *vm = (VM *)calloc(1, sizeof(VM));
    if (!vm) return NULL;
    vm->rom = mem_create(ROM_SIZE);
    vm->ram = mem_create(RAM_SIZE);
    if (!vm->rom || !vm->ram) { vm_free(vm); return NULL; }
    cpu_init(&vm->cpu);
    return vm;
}

void vm_free(VM *vm) {
    if (vm) {
        mem_free(vm->rom);
        mem_free(vm->ram);
        free(vm);
    }
}

int vm_load_program(VM *vm, const Tryte *code, size_t len) {
    if (len > ROM_SIZE) return 0;
    mem_load(vm->rom, 0, code, len);
    cpu_reset(&vm->cpu);
    return 1;
}

int vm_load_asm(VM *vm, const char *source) {
    size_t len;
    Tryte *code = asm_assemble(source, &len);
    if (!code) return 0;
    int ok = vm_load_program(vm, code, len);
    free(code);
    return ok;
}

int vm_step(VM *vm) {
    return cpu_step(&vm->cpu, vm->rom, vm->ram);
}

int vm_run(VM *vm) {
    return cpu_run(&vm->cpu, vm->rom, vm->ram);
}

void vm_dump_state(VM *vm, FILE *out) {
    fprintf(out, "=== CPU State ===\n");
    cpu_dump(&vm->cpu, out);

    size_t pc = (size_t)tryte_to_int64(vm->cpu.pc);
    fprintf(out, "\n=== Current Instruction ===\n");
    if (pc < ROM_SIZE) {
        Tryte instr = mem_read(vm->rom, pc);
        char *line = disasm_one(instr, (int)pc);
        fprintf(out, "  %s\n", line ? line : "???");
        free(line);
    }
    fprintf(out, "\n=== Stack (top 8) ===\n");
    size_t sp = (size_t)tryte_to_int64(vm->cpu.sp);
    size_t stack_start = (sp + 1 > 8) ? sp + 1 - 8 : 0;
    mem_dump(vm->ram, stack_start, sp + 2, out);
}

void vm_reset(VM *vm) {
    cpu_reset(&vm->cpu);
}
