#include "minunit.h"
#include "asm.h"
#include "isa.h"
#include "trit.h"
#include <string.h>

static int test_assemble_simple(void) {
    size_t len;
    Tryte *code = asm_assemble("NOP\nHLT\n", &len);
    mu_assert(code != NULL, "basic assembly");
    mu_assert(len == 2, "2 instructions");
    free(code);
    return 0;
}

static int test_assemble_operand(void) {
    size_t len;
    Tryte *code = asm_assemble("LOAD ACC, 42\nHLT\n", &len);
    mu_assert(code != NULL, "assembly with operand");
    mu_assert(len == 3, "LOAD + operand + HLT");
    free(code);
    return 0;
}

static int test_assemble_label_backward(void) {
    size_t len;
    Tryte *code = asm_assemble(
        "JMP skip\n"
        "skip:\n"
        "HLT\n", &len);
    mu_assert(code != NULL, "backward label");
    mu_assert(len == 3, "3 trytes (JMP + operand + HLT)");
    free(code);
    return 0;
}

static int test_assemble_forward_label(void) {
    size_t len;
    Tryte *code = asm_assemble(
        "skip:\n"
        "JMP skip\n"
        "HLT\n", &len);
    mu_assert(code != NULL, "forward label");
    mu_assert(len == 3, "3 trytes (JMP + operand + HLT)");
    free(code);
    return 0;
}

static int test_assemble_comments(void) {
    size_t len;
    Tryte *code = asm_assemble(
        "; comment line\n"
        "NOP  ; inline comment\n"
        "HLT\n", &len);
    mu_assert(code != NULL, "assembly with comments");
    mu_assert(len == 2, "2 trytes, comments stripped");
    free(code);
    return 0;
}

static int test_assemble_blank_lines(void) {
    size_t len;
    Tryte *code = asm_assemble("\n\nNOP\n\nHLT\n\n", &len);
    mu_assert(code != NULL, "assembly with blank lines");
    mu_assert(len == 2, "2 trytes, blanks stripped");
    free(code);
    return 0;
}

static int test_assemble_registers(void) {
    size_t len;
    Tryte *code = asm_assemble(
        "LOAD ACC, 10\n"
        "LOAD B, 20\n"
        "ADD ACC, B\n"
        "HLT\n", &len);
    mu_assert(code != NULL, "register assembly");
    mu_assert(len == 6, "4 instr + 2 operands for two LOADs");
    free(code);
    return 0;
}

static int test_assemble_undefined_label(void) {
    size_t len;
    Tryte *code = asm_assemble("JMP nowhere\nHLT\n", &len);
    mu_assert(code == NULL, "undefined label returns NULL");
    return 0;
}

static int test_assemble_undefined_label_ex(void) {
    Tryte *code;
    size_t len;
    AsmResult r = asm_assemble_ex("JMP nowhere\nHLT\n", &code, &len);
    mu_assert(!r.valid, "asm_assemble_ex error reported");
    mu_assert(r.error_msg != NULL, "error message set");
    free(code);
    return 0;
}

static int test_assemble_unknown_instr(void) {
    size_t len;
    Tryte *code = asm_assemble("FOO\nHLT\n", &len);
    mu_assert(code == NULL, "unknown instr returns NULL");
    return 0;
}

void test_asm_suite(void) {
    mu_run_test(test_assemble_simple);
    mu_run_test(test_assemble_operand);
    mu_run_test(test_assemble_label_backward);
    mu_run_test(test_assemble_forward_label);
    mu_run_test(test_assemble_comments);
    mu_run_test(test_assemble_blank_lines);
    mu_run_test(test_assemble_registers);
    mu_run_test(test_assemble_undefined_label);
    mu_run_test(test_assemble_undefined_label_ex);
    mu_run_test(test_assemble_unknown_instr);
}
