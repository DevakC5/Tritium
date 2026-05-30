#ifndef TRINARY_CPU_H
#define TRINARY_CPU_H

#include "trit.h"
#include "mem.h"
#include "isa.h"

#define CPU_MAX_CYCLES 1000000

typedef struct {
    Tryte pc;
    Tryte sp;
    Tryte acc;
    Tryte b;
    Tryte flags;
    int halted;
    int cycles;
} CPU;

void cpu_init(CPU *cpu);
void cpu_reset(CPU *cpu);
Tryte cpu_get_reg(CPU *cpu, int reg);
void cpu_set_reg(CPU *cpu, int reg, Tryte val);
int cpu_step(CPU *cpu, Memory *rom, Memory *ram);
int cpu_run(CPU *cpu, Memory *rom, Memory *ram);
void cpu_dump(CPU *cpu, FILE *out);

#endif
