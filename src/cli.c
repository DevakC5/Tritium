#include "vm.h"
#include "asm.h"
#include "disasm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options] <file.trc>\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -d        Debug mode (single-step)\n");
    fprintf(stderr, "  -r        Interactive REPL\n");
    fprintf(stderr, "  --dump    Disassemble bytecode\n");
}

static int run_debug(VM *vm) {
    char buf[256];
    while (!vm->cpu.halted) {
        vm_dump_state(vm, stdout);
        printf("\n[debug] step, run, reset, quit? ");
        if (!fgets(buf, sizeof(buf), stdin)) break;

        switch (buf[0]) {
            case 's':
                vm_step(vm);
                break;
            case 'r':
                vm_run(vm);
                printf("Ran %d cycles\n", vm->cpu.cycles);
                vm_dump_state(vm, stdout);
                break;
            case 'q':
                return 0;
            case 'R':
                vm_reset(vm);
                printf("Reset.\n");
                break;
            default:
                vm_step(vm);
                break;
        }
    }
    printf("Execution halted. %d cycles.\n", vm->cpu.cycles);
    return 1;
}

static void run_repl(void) {
    VM *vm = vm_create();
    if (!vm) { fprintf(stderr, "failed to create VM\n"); return; }
    printf("Trinary REPL. Type assembly instructions. Ctrl+D to quit.\n");
    printf("  ?  show state\n");
    printf("  .r reset\n");
    printf("  .q quit\n\n");

    char buf[4096];
    while (1) {
        printf("tr> ");
        if (!fgets(buf, sizeof(buf), stdin)) break;
        buf[strcspn(buf, "\n")] = '\0';

        if (strcmp(buf, ".q") == 0) break;
        if (strcmp(buf, ".r") == 0) { vm_reset(vm); printf("Reset.\n"); continue; }
        if (strcmp(buf, "?") == 0) { vm_dump_state(vm, stdout); continue; }
        if (buf[0] == '\0') continue;

        if (!vm_load_asm(vm, buf)) {
            printf("Assembly error.\n");
            continue;
        }
        vm_run(vm);
        vm_dump_state(vm, stdout);
        vm_reset(vm);
    }
    vm_free(vm);
}

static int run_dump(const char *path) {
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

    size_t len;
    Tryte *code = asm_assemble(source, &len);
    free(source);

    if (!code) {
        fprintf(stderr, "Assembly failed.\n");
        return 1;
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
        else file = argv[i];
    }

    if (!file) { print_usage(argv[0]); return 1; }
    return run_file(file, debug);
}
