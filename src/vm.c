#include "vm.h"
#include "asm.h"
#include "disasm.h"
#include <stdlib.h>
#include <stdio.h>

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
        DecodedInstr di = decode_instr(instr);
        const Tryte *operand = NULL;
        Tryte op_val;
        if (di.has_operand && pc + 1 < ROM_SIZE) {
            op_val = mem_read(vm->rom, pc + 1);
            operand = &op_val;
        }
        char *line = disasm_one(instr, (int)pc, operand);
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

void vm_set_breakpoint(VM *vm, int addr) {
    if (vm->num_breakpoints >= 32) return;
    vm->breakpoints[vm->num_breakpoints++] = addr;
}

void vm_clear_breakpoint(VM *vm, int addr) {
    for (int i = 0; i < vm->num_breakpoints; i++) {
        if (vm->breakpoints[i] == addr) {
            for (int j = i; j < vm->num_breakpoints - 1; j++)
                vm->breakpoints[j] = vm->breakpoints[j + 1];
            vm->num_breakpoints--;
            return;
        }
    }
}

void vm_clear_all_breakpoints(VM *vm) {
    vm->num_breakpoints = 0;
}

int vm_has_breakpoint(VM *vm, int addr) {
    for (int i = 0; i < vm->num_breakpoints; i++)
        if (vm->breakpoints[i] == addr) return 1;
    return 0;
}

int vm_run_until_breakpoint(VM *vm) {
    while (!vm->cpu.halted) {
        if (!cpu_step(&vm->cpu, vm->rom, vm->ram)) break;
        if (vm_has_breakpoint(vm, (int)tryte_to_int64(vm->cpu.pc)))
            return 0;
    }
    return 1;
}

int vm_write_binary(const char *path, const Tryte *code, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;

    unsigned char magic[] = {'T', 'B', 'C', 0};
    if (fwrite(magic, 1, 4, f) != 4) { fclose(f); return 0; }

    unsigned int num = (unsigned int)len;
    unsigned char hdr[4];
    hdr[0] = (unsigned char)(num & 0xFF);
    hdr[1] = (unsigned char)((num >> 8) & 0xFF);
    hdr[2] = (unsigned char)((num >> 16) & 0xFF);
    hdr[3] = (unsigned char)((num >> 24) & 0xFF);
    if (fwrite(hdr, 1, 4, f) != 4) { fclose(f); return 0; }

    size_t written = fwrite(code, sizeof(Tryte), len, f);
    fclose(f);
    return written == len;
}

Tryte *vm_read_binary(const char *path, size_t *len) {
    *len = 0;
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    unsigned char magic[4];
    if (fread(magic, 1, 4, f) != 4 || magic[0] != 'T' || magic[1] != 'B' || magic[2] != 'C' || magic[3] != 0) {
        fclose(f);
        return NULL;
    }

    unsigned char hdr[4];
    if (fread(hdr, 1, 4, f) != 4) { fclose(f); return NULL; }
    unsigned int num = (unsigned int)hdr[0] | ((unsigned int)hdr[1] << 8)
                     | ((unsigned int)hdr[2] << 16) | ((unsigned int)hdr[3] << 24);

    Tryte *code = (Tryte *)calloc(num, sizeof(Tryte));
    if (!code) { fclose(f); return NULL; }

    size_t nread = fread(code, sizeof(Tryte), (size_t)num, f);
    fclose(f);
    if (nread != (size_t)num) { free(code); return NULL; }

    *len = (size_t)num;
    return code;
}
