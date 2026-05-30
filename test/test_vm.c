#include "minunit.h"
#include "vm.h"
#include "trit.h"
#include <string.h>
#include <stdio.h>

static int test_vm_binary_roundtrip(void) {
    VM *vm = vm_create();
    mu_assert(vm != NULL, "vm_create");
    Tryte original[4];
    tryte_from_int64(10, &original[0]);
    tryte_from_int64(20, &original[1]);
    tryte_from_int64(30, &original[2]);
    tryte_from_int64(40, &original[3]);
    int ok = vm_write_binary("/tmp/_test_vm.tbc", original, 4);
    mu_assert(ok, "vm_write_binary");
    size_t len;
    Tryte *loaded = vm_read_binary("/tmp/_test_vm.tbc", &len);
    mu_assert(loaded != NULL, "vm_read_binary");
    mu_assert(len == 4, "read back 4 trytes");
    mu_assert(tryte_to_int64(loaded[0]) == 10, "value 0");
    mu_assert(tryte_to_int64(loaded[1]) == 20, "value 1");
    mu_assert(tryte_to_int64(loaded[2]) == 30, "value 2");
    mu_assert(tryte_to_int64(loaded[3]) == 40, "value 3");
    free(loaded);
    vm_free(vm);
    remove("/tmp/_test_vm.tbc");
    return 0;
}

static int test_vm_breakpoint_set_clear(void) {
    VM *vm = vm_create();
    mu_assert(vm != NULL, "vm_create");
    mu_assert(vm->num_breakpoints == 0, "no breakpoints initially");
    vm_set_breakpoint(vm, 10);
    mu_assert(vm->num_breakpoints == 1, "one breakpoint");
    mu_assert(vm_has_breakpoint(vm, 10), "has bp at 10");
    mu_assert(!vm_has_breakpoint(vm, 20), "no bp at 20");
    vm_set_breakpoint(vm, 20);
    mu_assert(vm->num_breakpoints == 2, "two breakpoints");
    vm_clear_breakpoint(vm, 10);
    mu_assert(!vm_has_breakpoint(vm, 10), "cleared bp at 10");
    mu_assert(vm_has_breakpoint(vm, 20), "still has bp at 20");
    vm_clear_all_breakpoints(vm);
    mu_assert(vm->num_breakpoints == 0, "all cleared");
    vm_free(vm);
    return 0;
}

static int test_vm_breakpoint_run_until(void) {
    VM *vm = vm_create();
    mu_assert(vm != NULL, "vm_create");
    const char *src =
        "LOAD ACC, 10\n"
        "OUT ACC\n"
        "LOAD ACC, 20\n"
        "OUT ACC\n"
        "HLT\n";
    mu_assert(vm_load_asm(vm, src), "load asm");
    vm_set_breakpoint(vm, 2);
    int halted = vm_run_until_breakpoint(vm);
    mu_assert(!halted, "stopped at breakpoint, not halted");
    int64_t pc = tryte_to_int64(vm->cpu.pc);
    mu_assert(pc == 2, "PC at breakpoint address 2");
    vm_free(vm);
    return 0;
}

static int test_vm_breakpoint_max_count(void) {
    VM *vm = vm_create();
    mu_assert(vm != NULL, "vm_create");
    int i;
    for (i = 0; i < 35; i++)
        vm_set_breakpoint(vm, i);
    mu_assert(vm->num_breakpoints <= 32, "max 32 breakpoints");
    vm_free(vm);
    return 0;
}

static int test_vm_binary_invalid(void) {
    size_t len;
    Tryte *code = vm_read_binary("/tmp/nonexistent_file_xyz.tbc", &len);
    mu_assert(code == NULL, "read nonexistent returns NULL");
    return 0;
}

void test_vm_suite(void) {
    mu_run_test(test_vm_binary_roundtrip);
    mu_run_test(test_vm_breakpoint_set_clear);
    mu_run_test(test_vm_breakpoint_run_until);
    mu_run_test(test_vm_breakpoint_max_count);
    mu_run_test(test_vm_binary_invalid);
}
