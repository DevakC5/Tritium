#include "vm.h"
#include "asm.h"
#include "disasm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options] <file.trc>\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -d                Debug mode (single-step)\n");
    fprintf(stderr, "  -r                Interactive REPL\n");
    fprintf(stderr, "  --dump <file>     Disassemble (.trc or .tbc)\n");
    fprintf(stderr, "  --assemble <src>  Assemble .trc to .tbc binary\n");
    fprintf(stderr, "             <dst>\n");
}

static int parse_num(const char *s, int *val) {
    char *end;
    errno = 0;
    long v = strtol(s, &end, 0);
    if (errno || *end) return 0;
    *val = (int)v;
    return 1;
}

/* --- Help --- */

static void show_help(void) {
    printf("Commands:\n");
    printf("  s, step           single-step one instruction\n");
    printf("  n, next           step over CALL\n");
    printf("  c, continue       run until breakpoint or halt\n");
    printf("  q, quit           quit\n");
    printf("  b <addr>          set breakpoint\n");
    printf("  bc [<addr>]       clear breakpoint (all if no addr)\n");
    printf("  bl                list breakpoints\n");
    printf("  m <addr> [<n>]    show n trytes of memory (default 1)\n");
    printf("  l [<n>]           list n instructions (default 8)\n");
    printf("  R, reset          reset CPU\n");
    printf("  ?, h, help        help\n");
}

/* --- Command implementations --- */

static int next_cmd(VM *vm) {
    size_t pc = (size_t)tryte_to_int64(vm->cpu.pc);
    if (pc < ROM_SIZE) {
        Tryte instr = mem_read(vm->rom, pc);
        DecodedInstr di = decode_instr(instr);
        if (di.op == OP_CALL && pc + 2 < ROM_SIZE) {
            int ret_addr = (int)(pc + 2);
            int already_set = vm_has_breakpoint(vm, ret_addr);
            if (!already_set) vm_set_breakpoint(vm, ret_addr);
            int halted = vm_run_until_breakpoint(vm);
            if (!already_set) vm_clear_breakpoint(vm, ret_addr);
            if (halted) return 0;
            return 1;
        }
    }
    vm_step(vm);
    return 1;
}

static int breakpoint_cmd(VM *vm, const char *args) {
    int addr;
    if (parse_num(args, &addr)) {
        vm_set_breakpoint(vm, addr);
        printf("Breakpoint set at %d\n", addr);
    } else {
        printf("Usage: b <addr>\n");
    }
    return 1;
}

static int clear_bp_cmd(VM *vm, const char *args) {
    if (!*args) {
        vm_clear_all_breakpoints(vm);
        printf("All breakpoints cleared.\n");
    } else {
        int addr;
        if (parse_num(args, &addr)) {
            vm_clear_breakpoint(vm, addr);
            printf("Breakpoint cleared at %d\n", addr);
        } else {
            printf("Usage: bc [<addr>]\n");
        }
    }
    return 1;
}

static int list_bp_cmd(VM *vm) {
    if (vm->num_breakpoints == 0) {
        printf("No breakpoints set.\n");
    } else {
        printf("Breakpoints:\n");
        for (int i = 0; i < vm->num_breakpoints; i++)
            printf("  %d\n", vm->breakpoints[i]);
    }
    return 1;
}

static int memory_cmd(VM *vm, const char *args) {
    int addr, count = 1;
    char argbuf[128];
    strncpy(argbuf, args, 127);
    argbuf[127] = '\0';
    char *p = argbuf;
    while (*p == ' ') p++;
    if (!*p) { printf("Usage: m <addr> [<n>]\n"); return 1; }
    char *tok = p;
    while (*p && *p != ' ') p++;
    if (*p) *p++ = '\0';
    if (!parse_num(tok, &addr)) { printf("Invalid address.\n"); return 1; }
    while (*p == ' ') p++;
    if (*p && !parse_num(p, &count)) count = 1;
    if (count < 1) count = 1;
    if (count > 64) count = 64;

    for (int i = 0; i < count; i++) {
        int a = addr + i;
        if (a < 0 || a >= 65536) break;
        Tryte val;
        tryte_zero(&val);
        if (a < ROM_SIZE)
            val = mem_read(vm->rom, a);
        else if (a < ROM_SIZE + RAM_SIZE)
            val = mem_read(vm->ram, a - ROM_SIZE);
        else
            val = mem_read(vm->ram, a - ROM_SIZE);
        char tstr[16];
        tryte_to_str(val, tstr, 16);
        printf("[%04X] %s (%ld)\n", a, tstr, (long)tryte_to_int64(val));
    }
    return 1;
}

static int list_cmd(VM *vm, const char *args) {
    int count = 8;
    if (*args && !parse_num(args, &count)) count = 8;
    if (count < 1) count = 1;
    if (count > 128) count = 128;

    size_t pc = (size_t)tryte_to_int64(vm->cpu.pc);
    size_t addr = pc;
    for (int i = 0; i < count && addr < ROM_SIZE; i++) {
        Tryte instr = mem_read(vm->rom, addr);
        DecodedInstr di = decode_instr(instr);
        const Tryte *operand = NULL;
        Tryte op_val;
        if (di.has_operand && addr + 1 < ROM_SIZE) {
            op_val = mem_read(vm->rom, addr + 1);
            operand = &op_val;
        }
        char *line = disasm_one(instr, (int)addr, operand);
        printf("  %s %s\n", (addr == pc) ? ">" : " ", line ? line : "???");
        free(line);
        addr += di.has_operand ? 2 : 1;
    }
    return 1;
}

/* Dispatch a command. Returns 0 to quit, 1 to continue. */
static int dispatch_cmd(VM *vm, const char *line) {
    while (*line == ' ') line++;
    if (!*line) return 1;

    char cmd[64];
    int ci = 0;
    while (*line && *line != ' ' && ci < 63)
        cmd[ci++] = *line++;
    cmd[ci] = '\0';
    while (*line == ' ') line++;
    const char *args = line;

    if (strcmp(cmd, "s") == 0 || strcmp(cmd, "step") == 0) {
        vm_step(vm);
        return 1;
    }
    if (strcmp(cmd, "n") == 0 || strcmp(cmd, "next") == 0) {
        next_cmd(vm);
        return 1;
    }
    if (strcmp(cmd, "c") == 0 || strcmp(cmd, "r") == 0 ||
        strcmp(cmd, "continue") == 0 || strcmp(cmd, "run") == 0) {
        int halted = vm_run_until_breakpoint(vm);
        if (halted)
            printf("Halted after %d cycles.\n", vm->cpu.cycles);
        else
            printf("Stopped at breakpoint.\n");
        return 1;
    }
    if (strcmp(cmd, "q") == 0 || strcmp(cmd, "quit") == 0)
        return 0;
    if (strcmp(cmd, "R") == 0 || strcmp(cmd, "reset") == 0) {
        vm_reset(vm);
        vm_clear_all_breakpoints(vm);
        printf("Reset.\n");
        return 1;
    }
    if (strcmp(cmd, "b") == 0) return breakpoint_cmd(vm, args);
    if (strcmp(cmd, "bc") == 0) return clear_bp_cmd(vm, args);
    if (strcmp(cmd, "bl") == 0) return list_bp_cmd(vm);
    if (strcmp(cmd, "m") == 0) return memory_cmd(vm, args);
    if (strcmp(cmd, "l") == 0) return list_cmd(vm, args);
    if (strcmp(cmd, "h") == 0 || strcmp(cmd, "?") == 0 || strcmp(cmd, "help") == 0) {
        show_help();
        return 1;
    }
    printf("Unknown command: %s. Try h for help.\n", cmd);
    return 1;
}

/* --- Debug mode --- */

static int run_debug(VM *vm) {
    char buf[256];
    printf("Debug mode. h for help.\n\n");
    while (!vm->cpu.halted) {
        vm_dump_state(vm, stdout);
        printf("\n[debug] ");
        if (!fgets(buf, sizeof(buf), stdin)) break;
        buf[strcspn(buf, "\n")] = '\0';
        if (!dispatch_cmd(vm, buf)) break;
    }
    if (vm->cpu.halted)
        printf("Execution halted. %d cycles.\n", vm->cpu.cycles);
    return 1;
}

/* --- REPL mode --- */

#define REPL_BUF_SIZE 65536

static void run_repl(void) {
    VM *vm = vm_create();
    if (!vm) { fprintf(stderr, "failed to create VM\n"); return; }

    char accum[REPL_BUF_SIZE];
    int accum_len = 0;
    accum[0] = '\0';

    printf("Trinary REPL. Type assembly (accumulates), blank line to run.\n");
    printf("  .h  help  .q  quit  .r  reset  .load <file>\n");
    printf("  debug commands: s, n, c, b, bc, bl, m, l, ?\n\n");

    char buf[4096];
    while (1) {
        printf("tr> ");
        if (!fgets(buf, sizeof(buf), stdin)) break;
        buf[strcspn(buf, "\n")] = '\0';

        /* Empty line: run accumulated code */
        if (buf[0] == '\0') {
            if (accum_len == 0) continue;
            if (!vm_load_asm(vm, accum)) {
                printf("Assembly error.\n");
                continue;
            }
            int halted;
            if (vm->num_breakpoints > 0) {
                halted = vm_run_until_breakpoint(vm);
                if (!halted)
                    printf("Stopped at breakpoint.\n");
            } else {
                vm_run(vm);
                halted = vm->cpu.halted;
            }
            if (halted)
                printf("Halted after %d cycles.\n", vm->cpu.cycles);
            vm_dump_state(vm, stdout);
            continue;
        }

        /* Dot-commands */
        if (buf[0] == '.') {
            if (strcmp(buf, ".q") == 0 || strcmp(buf, ".quit") == 0) break;
            if (strcmp(buf, ".r") == 0 || strcmp(buf, ".reset") == 0) {
                vm_reset(vm);
                printf("Reset.\n");
                continue;
            }
            if (strcmp(buf, ".h") == 0 || strcmp(buf, ".?") == 0) {
                show_help();
                continue;
            }
            if (strcmp(buf, ".end") == 0 || strcmp(buf, ".run") == 0) {
                if (accum_len == 0) continue;
                if (!vm_load_asm(vm, accum)) {
                    printf("Assembly error.\n");
                    continue;
                }
                int halted;
                if (vm->num_breakpoints > 0) {
                    halted = vm_run_until_breakpoint(vm);
                    if (!halted)
                        printf("Stopped at breakpoint.\n");
                } else {
                    vm_run(vm);
                    halted = vm->cpu.halted;
                }
                if (halted)
                    printf("Halted after %d cycles.\n", vm->cpu.cycles);
                vm_dump_state(vm, stdout);
                continue;
            }
            if (strncmp(buf, ".load", 5) == 0) {
                const char *path = buf + 5;
                while (*path == ' ') path++;
                if (!*path) { printf("Usage: .load <file.trc>\n"); continue; }
                FILE *f = fopen(path, "r");
                if (!f) { perror("fopen"); continue; }
                fseek(f, 0, SEEK_END);
                long fsize = ftell(f);
                fseek(f, 0, SEEK_SET);
                if (fsize < 0 || fsize >= REPL_BUF_SIZE - 1) {
                    printf("File too large.\n");
                    fclose(f);
                    continue;
                }
                accum_len = 0;
                accum[0] = '\0';
                size_t nread = fread(accum, 1, (size_t)fsize, f);
                fclose(f);
                accum[nread] = '\0';
                if (nread > 0 && accum[nread - 1] != '\n') {
                    if (nread < REPL_BUF_SIZE - 1) {
                        accum[nread] = '\n';
                        accum[nread + 1] = '\0';
                        accum_len = (int)nread + 1;
                    }
                } else {
                    accum_len = (int)nread;
                }
                printf("Loaded %zu bytes from %s.\n", nread, path);
                printf("Type blank line to run.\n");
                continue;
            }
            /* Unknown dot-command; strip dot and try debug dispatch */
            {
                char dbuf[256];
                strncpy(dbuf, buf + 1, 255);
                dbuf[255] = '\0';
                if (!dispatch_cmd(vm, dbuf)) break;
            }
            continue;
        }

        /* Check if line is a debug command (no dot) */
        {
            char first[16];
            int fi = 0;
            const char *p = buf;
            while (*p == ' ') p++;
            while (*p && *p != ' ' && fi < 15) first[fi++] = *p++;
            first[fi] = '\0';

            int is_dbg = 0;
            const char *dbg_cmds[] = {"s", "step", "n", "next", "c", "r",
                                       "continue", "run", "q", "quit",
                                       "R", "reset", "b", "bc", "bl",
                                       "m", "l", "h", "?", "help", NULL};
            for (int i = 0; dbg_cmds[i]; i++) {
                if (strcmp(first, dbg_cmds[i]) == 0) {
                    is_dbg = 1;
                    break;
                }
            }
            if (is_dbg) {
                char dbuf[256];
                strncpy(dbuf, buf, 255);
                dbuf[255] = '\0';
                if (!dispatch_cmd(vm, dbuf)) break;
                continue;
            }
        }

        /* Accumulate as assembly */
        {
            int needed = (int)strlen(buf) + 2;
            if (accum_len + needed >= REPL_BUF_SIZE) {
                printf("Buffer full. Use .r to clear or run.\n");
                continue;
            }
            strcat(accum, buf);
            strcat(accum, "\n");
            accum_len += needed - 1;
        }
    }

    vm_free(vm);
}

/* --- run_assemble, run_dump, run_file --- */

static int run_assemble(const char *src_path, const char *dst_path) {
    FILE *f = fopen(src_path, "r");
    if (!f) { perror("fopen"); return 1; }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *source = (char *)malloc((size_t)fsize + 1);
    if (!source) { fclose(f); return 1; }
    size_t nread = fread(source, 1, (size_t)fsize, f);
    source[nread] = '\0';
    fclose(f);

    size_t len;
    Tryte *code = asm_assemble(source, &len);
    free(source);

    if (!code) {
        fprintf(stderr, "Assembly failed.\n");
        return 1;
    }

    int ok = vm_write_binary(dst_path, code, len);
    free(code);

    if (!ok) {
        fprintf(stderr, "Failed to write binary.\n");
        return 1;
    }

    printf("Assembled %zu trytes -> %s\n", len, dst_path);
    return 0;
}

static int run_dump(const char *path) {
    size_t len;
    Tryte *code = NULL;

    FILE *f = fopen(path, "rb");
    if (!f) { perror("fopen"); return 1; }

    unsigned char magic[4];
    size_t nm = fread(magic, 1, 4, f);
    fclose(f);

    if (nm == 4 && magic[0] == 'T' && magic[1] == 'B' && magic[2] == 'C' && magic[3] == 0) {
        code = vm_read_binary(path, &len);
        if (!code) { fprintf(stderr, "Failed to read binary.\n"); return 1; }
    } else {
        f = fopen(path, "r");
        if (!f) { perror("fopen"); return 1; }

        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);

        char *source = (char *)malloc((size_t)fsize + 1);
        if (!source) { fclose(f); return 1; }
        size_t nread = fread(source, 1, (size_t)fsize, f);
        source[nread] = '\0';
        fclose(f);

        code = asm_assemble(source, &len);
        free(source);

        if (!code) {
            fprintf(stderr, "Assembly failed.\n");
            return 1;
        }
    }

    char **lines = disasm_all(code, len, 0);
    if (lines) {
        for (char **l = lines; *l; l++)
            printf("%s\n", *l);
        free(lines);
    }
    free(code);
    return 0;
}

static int run_file(const char *path, int debug) {
    FILE *f = fopen(path, "r");
    if (!f) { perror("fopen"); return 1; }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *source = (char *)malloc((size_t)fsize + 1);
    if (!source) { fclose(f); return 1; }
    size_t nread = fread(source, 1, (size_t)fsize, f);
    source[nread] = '\0';
    fclose(f);

    VM *vm = vm_create();
    if (!vm) { free(source); return 1; }

    if (!vm_load_asm(vm, source)) {
        fprintf(stderr, "Assembly failed.\n");
        free(source);
        vm_free(vm);
        return 1;
    }
    free(source);

    printf("Running...\n");

    if (debug) {
        int ret = run_debug(vm);
        vm_free(vm);
        return ret ? 0 : 0;
    }

    int cycles = vm_run(vm);
    printf("\nHalted after %d cycles.\n", cycles);
    vm_dump_state(vm, stdout);
    vm_free(vm);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) { print_usage(argv[0]); return 1; }

    if (strcmp(argv[1], "-r") == 0) {
        run_repl();
        return 0;
    }

    int debug = 0;
    const char *file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) debug = 1;
        else if (strcmp(argv[i], "--dump") == 0) {
            if (i + 1 < argc)
                return run_dump(argv[++i]);
            fprintf(stderr, "--dump requires a file argument\n");
            return 1;
        }
        else if (strcmp(argv[i], "--assemble") == 0) {
            if (i + 2 < argc) {
                const char *src = argv[++i];
                const char *dst = argv[++i];
                return run_assemble(src, dst);
            }
            fprintf(stderr, "--assemble requires <src> <dst> arguments\n");
            return 1;
        }
        else file = argv[i];
    }

    if (!file) { print_usage(argv[0]); return 1; }
    return run_file(file, debug);
}
