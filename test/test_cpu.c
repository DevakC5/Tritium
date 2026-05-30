#include "minunit.h"
#include "vm.h"
#include "asm.h"
#include "isa.h"
#include <string.h>

static int test_nop_hlt(void) {
    VM *vm = vm_create();
    mu_assert(vm != NULL, "vm created");

    const char *src = "NOP\nHLT\n";
    mu_assert(vm_load_asm(vm, src), "assembly ok");

    int cycles = vm_run(vm);
    mu_assert(vm->cpu.halted, "halted");
    mu_assert(cycles > 0, "cycles > 0");

    vm_free(vm);
    return 0;
}

static int test_add(void) {
    VM *vm = vm_create();
    mu_assert(vm != NULL, "vm created");

    const char *src =
        "LOAD ACC, 5\n"
        "LOAD B, 3\n"
        "ADD ACC, B\n"
        "HLT\n";
    mu_assert(vm_load_asm(vm, src), "assembly ok");

    vm_run(vm);
    mu_assert(tryte_to_int64(vm->cpu.acc) == 8, "ACC = 8");
    mu_assert(vm->cpu.halted, "halted");

    vm_free(vm);
    return 0;
}

static int test_sub_mul(void) {
    VM *vm = vm_create();
    const char *src =
        "LOAD ACC, 20\n"
        "LOAD B, 7\n"
        "SUB ACC, B\n"
        "HLT\n";
    mu_assert(vm_load_asm(vm, src), "assembly ok");
    vm_run(vm);
    mu_assert(tryte_to_int64(vm->cpu.acc) == 13, "20-7=13");

    vm_reset(vm);
    const char *src2 =
        "LOAD ACC, 6\n"
        "LOAD B, 7\n"
        "MUL ACC, B\n"
        "HLT\n";
    mu_assert(vm_load_asm(vm, src2), "assembly ok");
    vm_run(vm);
    mu_assert(tryte_to_int64(vm->cpu.acc) == 42, "6*7=42");

    vm_free(vm);
    return 0;
}

static int test_jump(void) {
    VM *vm = vm_create();
    const char *src =
        "LOAD ACC, 1\n"
        "JMP skip\n"
        "LOAD ACC, 99\n"
        "skip:\n"
        "HLT\n";
    mu_assert(vm_load_asm(vm, src), "assembly ok");
    vm_run(vm);
    mu_assert(tryte_to_int64(vm->cpu.acc) == 1, "jumped over load 99");
    vm_free(vm);
    return 0;
}

static int test_conditional_jump(void) {
    VM *vm = vm_create();
    const char *src =
        "LOAD ACC, 10\n"
        "LOAD B, 10\n"
        "CMP ACC, B\n"
        "JE equal\n"
        "LOAD ACC, 0\n"
        "JMP done\n"
        "equal:\n"
        "LOAD ACC, 1\n"
        "done:\n"
        "HLT\n";
    mu_assert(vm_load_asm(vm, src), "assembly ok");
    vm_run(vm);
    mu_assert(tryte_to_int64(vm->cpu.acc) == 1, "jumped to equal");
    vm_free(vm);
    return 0;
}

static int test_push_pop(void) {
    VM *vm = vm_create();
    const char *src =
        "LOAD ACC, 42\n"
        "PUSH ACC\n"
        "LOAD ACC, 0\n"
        "POP ACC\n"
        "HLT\n";
    mu_assert(vm_load_asm(vm, src), "assembly ok");
    vm_run(vm);
    mu_assert(tryte_to_int64(vm->cpu.acc) == 42, "pop got 42");
    vm_free(vm);
    return 0;
}

static int test_call_ret(void) {
    VM *vm = vm_create();
    const char *src =
        "LOAD ACC, 2\n"
        "CALL double\n"
        "HLT\n"
        "double:\n"
        "ADD ACC, ACC\n"
        "RET\n";
    mu_assert(vm_load_asm(vm, src), "assembly ok");
    vm_run(vm);
    mu_assert(tryte_to_int64(vm->cpu.acc) == 4, "2*2=4 via call");
    vm_free(vm);
    return 0;
}

static int test_io(void) {
    VM *vm = vm_create();
    const char *src =
        "LOAD ACC, 65\n"
        "OUT ACC\n"
        "HLT\n";
    mu_assert(vm_load_asm(vm, src), "assembly ok");
    vm_run(vm);
    mu_assert(tryte_to_int64(vm->cpu.acc) == 65, "ACC=65 after OUT");
    vm_free(vm);
    return 0;
}

static int test_inc(void) {
    VM *vm = vm_create();
    const char *src = "LOAD ACC, 5\nINC ACC\nHLT\n";
    mu_assert(vm_load_asm(vm, src), "assembly ok");
    vm_run(vm);
    mu_assert(tryte_to_int64(vm->cpu.acc) == 6, "INC 5 = 6");
    vm_free(vm);
    return 0;
}

static int test_dec(void) {
    VM *vm = vm_create();
    const char *src = "LOAD ACC, 5\nDEC ACC\nHLT\n";
    mu_assert(vm_load_asm(vm, src), "assembly ok");
    vm_run(vm);
    mu_assert(tryte_to_int64(vm->cpu.acc) == 4, "DEC 5 = 4");
    vm_free(vm);
    return 0;
}

static int test_abs(void) {
    VM *vm = vm_create();
    const char *src = "LOAD ACC, -5\nABS ACC\nHLT\n";
    mu_assert(vm_load_asm(vm, src), "assembly ok");
    vm_run(vm);
    mu_assert(tryte_to_int64(vm->cpu.acc) == 5, "ABS -5 = 5");
    vm_free(vm);
    return 0;
}

static int test_abs_positive(void) {
    VM *vm = vm_create();
    const char *src = "LOAD ACC, 5\nABS ACC\nHLT\n";
    mu_assert(vm_load_asm(vm, src), "assembly ok");
    vm_run(vm);
    mu_assert(tryte_to_int64(vm->cpu.acc) == 5, "ABS 5 = 5");
    vm_free(vm);
    return 0;
}

static int test_jge(void) {
    VM *vm = vm_create();
    const char *src =
        "LOAD ACC, 5\n"
        "CMP ACC, 3\n"
        "JGE ge_ok\n"
        "LOAD ACC, 0\n"
        "JMP done\n"
        "ge_ok:\n"
        "LOAD ACC, 1\n"
        "done:\n"
        "HLT\n";
    mu_assert(vm_load_asm(vm, src), "assembly ok");
    vm_run(vm);
    mu_assert(tryte_to_int64(vm->cpu.acc) == 1, "JGE taken (5>=3)");
    vm_free(vm);
    return 0;
}

static int test_jle(void) {
    VM *vm = vm_create();
    const char *src =
        "LOAD ACC, 2\n"
        "CMP ACC, 5\n"
        "JLE le_ok\n"
        "LOAD ACC, 0\n"
        "JMP done\n"
        "le_ok:\n"
        "LOAD ACC, 1\n"
        "done:\n"
        "HLT\n";
    mu_assert(vm_load_asm(vm, src), "assembly ok");
    vm_run(vm);
    mu_assert(tryte_to_int64(vm->cpu.acc) == 1, "JLE taken (2<=5)");
    vm_free(vm);
    return 0;
}

static int test_jmpr(void) {
    VM *vm = vm_create();
    const char *src =
        "LOAD ACC, target\n"
        "JMPR ACC\n"
        "LOAD ACC, 99\n"
        "HLT\n"
        "target:\n"
        "LOAD ACC, 42\n"
        "HLT\n";
    mu_assert(vm_load_asm(vm, src), "assembly ok");
    vm_run(vm);
    mu_assert(tryte_to_int64(vm->cpu.acc) == 42, "JMPR jumped to target");
    vm_free(vm);
    return 0;
}

static int test_callr(void) {
    VM *vm = vm_create();
    const char *src =
        "LOAD ACC, 3\n"
        "LOAD B, double_fn\n"
        "CALLR B\n"
        "HLT\n"
        "double_fn:\n"
        "ADD ACC, ACC\n"
        "RET\n";
    mu_assert(vm_load_asm(vm, src), "assembly ok");
    vm_run(vm);
    mu_assert(tryte_to_int64(vm->cpu.acc) == 6, "CALLR: 3*2=6 via indirect call");
    vm_free(vm);
    return 0;
}

void test_cpu_suite(void) {
    mu_run_test(test_nop_hlt);
    mu_run_test(test_add);
    mu_run_test(test_sub_mul);
    mu_run_test(test_jump);
    mu_run_test(test_conditional_jump);
    mu_run_test(test_push_pop);
    mu_run_test(test_call_ret);
    mu_run_test(test_io);
    mu_run_test(test_inc);
    mu_run_test(test_dec);
    mu_run_test(test_abs);
    mu_run_test(test_abs_positive);
    mu_run_test(test_jge);
    mu_run_test(test_jle);
    mu_run_test(test_jmpr);
    mu_run_test(test_callr);
}
