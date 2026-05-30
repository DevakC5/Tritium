#include "cpu.h"
#include "arith.h"
#include "gate.h"
#include "mem.h"
#include <stdlib.h>

void cpu_init(CPU *cpu) {
    tryte_zero(&cpu->pc);
    tryte_zero(&cpu->sp);
    tryte_zero(&cpu->acc);
    tryte_zero(&cpu->b);
    tryte_zero(&cpu->flags);
    cpu->halted = 0;
    cpu->cycles = 0;

    tryte_from_int64((int64_t)(STACK_BASE + STACK_SIZE - 1), &cpu->sp);
}

void cpu_reset(CPU *cpu) {
    cpu_init(cpu);
}

Tryte cpu_get_reg(CPU *cpu, int reg) {
    switch (reg) {
        case REG_ACC: return cpu->acc;
        case REG_B:   return cpu->b;
        case REG_SP:  return cpu->sp;
        case REG_FL:  return cpu->flags;
        default:      { Tryte z; tryte_zero(&z); return z; }
    }
}

void cpu_set_reg(CPU *cpu, int reg, Tryte val) {
    switch (reg) {
        case REG_ACC: cpu->acc = val; break;
        case REG_B:   cpu->b = val;   break;
        case REG_SP:  cpu->sp = val;  break;
        case REG_FL:  cpu->flags = val; break;
        default: break;
    }
}

static Tryte mem_read_tryte(CPU *cpu, Memory *rom, Memory *ram, size_t addr) {
    (void)cpu;
    if (addr < ROM_SIZE) return mem_read(rom, addr);
    if (addr < ROM_SIZE + RAM_SIZE) return mem_read(ram, addr - ROM_SIZE);
    { Tryte z; tryte_zero(&z); return z; }
}

static void mem_write_tryte(CPU *cpu, Memory *rom, Memory *ram, size_t addr, Tryte val) {
    (void)cpu;
    if (addr < ROM_SIZE) { mem_write(rom, addr, val); return; }
    if (addr < ROM_SIZE + RAM_SIZE) { mem_write(ram, addr - ROM_SIZE, val); return; }
}

static size_t tryte_to_addr(Tryte t) {
    return (size_t)tryte_to_int64(t) & 0xFFFF;
}

static void update_flags(CPU *cpu, Tryte result) {
    int64_t v = tryte_to_int64(result);
    if (v < 0) tryte_from_int64(-1, &cpu->flags);
    else if (v > 0) tryte_from_int64(1, &cpu->flags);
    else tryte_zero(&cpu->flags);
}

static Tryte make_tryte(int64_t val) {
    Tryte t;
    tryte_from_int64(val, &t);
    return t;
}

static Tryte power3(int n) {
    int64_t v = 1;
    for (int i = 0; i < n && i < 18; i++) v *= 3;
    Tryte t;
    tryte_from_int64(v, &t);
    return t;
}

int cpu_step(CPU *cpu, Memory *rom, Memory *ram) {
    if (cpu->halted) return 0;

    size_t pc = tryte_to_addr(cpu->pc);
    Tryte instr = mem_read_tryte(cpu, rom, ram, pc);
    DecodedInstr di = decode_instr(instr);

    Tryte operand;
    tryte_zero(&operand);
    size_t next_pc = pc + 1;

    if (di.has_operand) {
        operand = mem_read_tryte(cpu, rom, ram, pc + 1);
        next_pc = pc + 2;
    }

    Tryte dest_val = cpu_get_reg(cpu, di.reg_dest);
    Tryte src_val = cpu_get_reg(cpu, di.reg_src);
    Tryte result;
    tryte_zero(&result);

    switch (di.op) {
        case OP_NOP:
            break;

        case OP_HLT:
            cpu->halted = 1;
            return 0;

        case OP_ADD:
            result = tryte_add(dest_val, src_val);
            cpu_set_reg(cpu, di.reg_dest, result);
            update_flags(cpu, result);
            break;

        case OP_SUB:
            result = tryte_sub(dest_val, src_val);
            cpu_set_reg(cpu, di.reg_dest, result);
            update_flags(cpu, result);
            break;

        case OP_MUL:
            result = tryte_mul(dest_val, src_val);
            cpu_set_reg(cpu, di.reg_dest, result);
            update_flags(cpu, result);
            break;

        case OP_DIV: {
            Tryte rem;
            result = tryte_div(dest_val, src_val, &rem);
            cpu_set_reg(cpu, di.reg_dest, result);
            cpu_set_reg(cpu, REG_B, rem);
            update_flags(cpu, result);
            break;
        }

        case OP_NEG:
            result = tryte_neg(dest_val);
            cpu_set_reg(cpu, di.reg_dest, result);
            update_flags(cpu, result);
            break;

        case OP_AND:
            tryte_and(dest_val, src_val, &result);
            cpu_set_reg(cpu, di.reg_dest, result);
            update_flags(cpu, result);
            break;

        case OP_OR:
            tryte_or(dest_val, src_val, &result);
            cpu_set_reg(cpu, di.reg_dest, result);
            update_flags(cpu, result);
            break;

        case OP_XOR:
            tryte_xor(dest_val, src_val, &result);
            cpu_set_reg(cpu, di.reg_dest, result);
            update_flags(cpu, result);
            break;

        case OP_NOT:
            tryte_not(dest_val, &result);
            cpu_set_reg(cpu, di.reg_dest, result);
            update_flags(cpu, result);
            break;

        case OP_LOAD:
            result = operand;
            cpu_set_reg(cpu, di.reg_dest, result);
            update_flags(cpu, result);
            break;

        case OP_STORE:
            mem_write_tryte(cpu, rom, ram, tryte_to_addr(operand), src_val);
            break;

        case OP_MOV:
            cpu_set_reg(cpu, di.reg_dest, src_val);
            update_flags(cpu, src_val);
            break;

        case OP_CMP:
            result = tryte_sub(dest_val, src_val);
            update_flags(cpu, result);
            break;

        case OP_LD:
            result = mem_read_tryte(cpu, rom, ram, tryte_to_addr(operand));
            cpu_set_reg(cpu, di.reg_dest, result);
            update_flags(cpu, result);
            break;

        case OP_SHL: {
            int64_t shift = tryte_to_int64(src_val);
            if (shift < 0) shift = 0;
            result = tryte_mul(dest_val, power3((int)shift));
            cpu_set_reg(cpu, di.reg_dest, result);
            update_flags(cpu, result);
            break;
        }

        case OP_SHR: {
            int64_t shift = tryte_to_int64(src_val);
            if (shift < 0) shift = 0;
            Tryte rem;
            result = tryte_div(dest_val, power3((int)shift), &rem);
            cpu_set_reg(cpu, di.reg_dest, result);
            update_flags(cpu, result);
            break;
        }

        case OP_MOD: {
            Tryte rem;
            tryte_div(dest_val, src_val, &rem);
            cpu_set_reg(cpu, di.reg_dest, rem);
            update_flags(cpu, rem);
            break;
        }

        case OP_SWAP:
            cpu_set_reg(cpu, di.reg_dest, src_val);
            cpu_set_reg(cpu, di.reg_src, dest_val);
            break;

        case OP_INC:
            result = tryte_add(dest_val, make_tryte(1));
            cpu_set_reg(cpu, di.reg_dest, result);
            update_flags(cpu, result);
            break;

        case OP_DEC: {
            Tryte one;
            tryte_from_int64(1, &one);
            result = tryte_sub(dest_val, one);
            cpu_set_reg(cpu, di.reg_dest, result);
            update_flags(cpu, result);
            break;
        }

        case OP_ABS: {
            int64_t av = tryte_to_int64(dest_val);
            if (av < 0)
                result = tryte_neg(dest_val);
            else
                result = dest_val;
            cpu_set_reg(cpu, di.reg_dest, result);
            update_flags(cpu, result);
            break;
        }

        case OP_JGE: {
            int64_t f = tryte_to_int64(cpu->flags);
            if (f >= 0) next_pc = tryte_to_addr(operand);
            break;
        }

        case OP_JLE: {
            int64_t f = tryte_to_int64(cpu->flags);
            if (f <= 0) next_pc = tryte_to_addr(operand);
            break;
        }

        case OP_JMPR:
            next_pc = tryte_to_addr(src_val);
            break;

        case OP_CALLR: {
            size_t sp = tryte_to_addr(cpu->sp);
            Tryte ret_addr;
            tryte_from_int64((int64_t)next_pc, &ret_addr);
            mem_write_tryte(cpu, rom, ram, sp, ret_addr);
            {
                Tryte new_sp;
                tryte_from_int64((int64_t)(sp - 1), &new_sp);
                cpu->sp = new_sp;
            }
            next_pc = tryte_to_addr(src_val);
            break;
        }

        case OP_OUTNUM: {
            int64_t v = tryte_to_int64(src_val);
            char buf[32];
            snprintf(buf, sizeof(buf), "%ld", (long)v);
            fputs(buf, stdout);
            fflush(stdout);
            break;
        }

        case OP_OUTSTR: {
            size_t a = tryte_to_addr(operand);
            while (1) {
                Tryte t = mem_read_tryte(cpu, rom, ram, a);
                int64_t v = tryte_to_int64(t);
                if (v == 0) break;
                fputc((int)(v & 0xFF), stdout);
                a++;
            }
            fflush(stdout);
            break;
        }

        case OP_INSTR: {
            size_t a = tryte_to_addr(operand);
            size_t max = ROM_SIZE + RAM_SIZE;
            while (a < max) {
                int c = fgetc(stdin);
                if (c == EOF || c == '\n') {
                    mem_write_tryte(cpu, rom, ram, a, make_tryte(0));
                    break;
                }
                mem_write_tryte(cpu, rom, ram, a, make_tryte((int64_t)c));
                a++;
            }
            break;
        }

        case OP_RND: {
            int64_t rv = (int64_t)(rand() % 19683) - 9841;
            result = make_tryte(rv);
            cpu_set_reg(cpu, di.reg_dest, result);
            update_flags(cpu, result);
            break;
        }

        case OP_JMP:
            next_pc = tryte_to_addr(operand);
            break;

        case OP_JE: {
            int64_t f = tryte_to_int64(cpu->flags);
            if (f == 0) next_pc = tryte_to_addr(operand);
            break;
        }

        case OP_JNE: {
            int64_t f = tryte_to_int64(cpu->flags);
            if (f != 0) next_pc = tryte_to_addr(operand);
            break;
        }

        case OP_JG: {
            int64_t f = tryte_to_int64(cpu->flags);
            if (f > 0) next_pc = tryte_to_addr(operand);
            break;
        }

        case OP_JL: {
            int64_t f = tryte_to_int64(cpu->flags);
            if (f < 0) next_pc = tryte_to_addr(operand);
            break;
        }

        case OP_JZ: {
            int64_t v = tryte_to_int64(cpu->acc);
            if (v == 0) next_pc = tryte_to_addr(operand);
            break;
        }

        case OP_CALL: {
            size_t sp = tryte_to_addr(cpu->sp);
            Tryte ret_addr;
            tryte_from_int64((int64_t)next_pc, &ret_addr);
            mem_write_tryte(cpu, rom, ram, sp, ret_addr);
            Tryte new_sp;
            tryte_from_int64((int64_t)(sp - 1), &new_sp);
            cpu->sp = new_sp;
            next_pc = tryte_to_addr(operand);
            break;
        }

        case OP_RET: {
            size_t sp = tryte_to_addr(cpu->sp);
            Tryte new_sp;
            tryte_from_int64((int64_t)(sp + 1), &new_sp);
            cpu->sp = new_sp;
            Tryte ret_addr = mem_read_tryte(cpu, rom, ram, sp + 1);
            next_pc = tryte_to_addr(ret_addr);
            break;
        }

        case OP_PUSH: {
            size_t sp = tryte_to_addr(cpu->sp);
            mem_write_tryte(cpu, rom, ram, sp, src_val);
            Tryte new_sp;
            tryte_from_int64((int64_t)(sp - 1), &new_sp);
            cpu->sp = new_sp;
            break;
        }

        case OP_POP: {
            size_t sp = tryte_to_addr(cpu->sp);
            Tryte new_sp;
            tryte_from_int64((int64_t)(sp + 1), &new_sp);
            cpu->sp = new_sp;
            Tryte val = mem_read_tryte(cpu, rom, ram, sp + 1);
            cpu_set_reg(cpu, di.reg_dest, val);
            break;
        }

        case OP_IN: {
            int c = fgetc(stdin);
            if (c == EOF) c = 0;
            Tryte val;
            tryte_from_int64((int64_t)c, &val);
            cpu_set_reg(cpu, di.reg_dest, val);
            break;
        }

        case OP_OUT: {
            int64_t v = tryte_to_int64(src_val);
            fputc((int)(v & 0xFF), stdout);
            fflush(stdout);
            break;
        }

        default:
            break;
    }

    tryte_from_int64((int64_t)next_pc, &cpu->pc);
    cpu->cycles++;
    return 1;
}

int cpu_run(CPU *cpu, Memory *rom, Memory *ram) {
    int steps = 0;
    while (!cpu->halted) {
        if (!cpu_step(cpu, rom, ram)) break;
        steps++;
        if (steps > CPU_MAX_CYCLES) { cpu->halted = 1; break; }
    }
    return steps;
}

void cpu_dump(CPU *cpu, FILE *out) {
    char buf[16];
    tryte_to_str(cpu->pc, buf, sizeof(buf));
    fprintf(out, "  PC: %s (%ld)\n", buf, (long)tryte_to_int64(cpu->pc));
    tryte_to_str(cpu->sp, buf, sizeof(buf));
    fprintf(out, "  SP: %s (%ld)\n", buf, (long)tryte_to_int64(cpu->sp));
    tryte_to_str(cpu->acc, buf, sizeof(buf));
    fprintf(out, " ACC: %s (%ld)\n", buf, (long)tryte_to_int64(cpu->acc));
    tryte_to_str(cpu->b, buf, sizeof(buf));
    fprintf(out, "   B: %s (%ld)\n", buf, (long)tryte_to_int64(cpu->b));
    tryte_to_str(cpu->flags, buf, sizeof(buf));
    fprintf(out, "  FL: %s (%ld)\n", buf, (long)tryte_to_int64(cpu->flags));
    fprintf(out, " HLT: %s\n", cpu->halted ? "yes" : "no");
    fprintf(out, " CYC: %d\n", cpu->cycles);
}
