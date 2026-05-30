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

static int test_assemble_ternary_literal(void) {
    size_t len;
    Tryte *code = asm_assemble("LOAD ACC, +0-\nHLT\n", &len);
    mu_assert(code != NULL, "ternary literal");
    mu_assert(len == 3, "LOAD + operand + HLT");
    int64_t val = tryte_to_int64(code[1]);
    Tryte expected;
    tryte_from_str("+0-", &expected);
    mu_assert(val == tryte_to_int64(expected), "ternary value matches +0-");
    free(code);
    return 0;
}

static int test_assemble_string_literal(void) {
    size_t len;
    Tryte *code = asm_assemble("LOAD ACC, \"A\"\nHLT\n", &len);
    mu_assert(code != NULL, "string literal");
    mu_assert(len == 3, "LOAD + operand + HLT");
    mu_assert(tryte_to_int64(code[1]) == 65, "string 'A' = 65");
    free(code);
    return 0;
}

static int test_assemble_str_directive(void) {
    size_t len;
    Tryte *code = asm_assemble(".str \"Hi\"\nHLT\n", &len);
    mu_assert(code != NULL, "str directive");
    mu_assert(len == 3, "\"Hi\" = 2 chars + HLT");
    mu_assert(tryte_to_int64(code[0]) == 72, "'H' = 72");
    mu_assert(tryte_to_int64(code[1]) == 105, "'i' = 105");
    free(code);
    return 0;
}

static int test_assemble_db_directive(void) {
    size_t len;
    Tryte *code = asm_assemble(".db \"ABC\"\nHLT\n", &len);
    mu_assert(code != NULL, "db directive");
    mu_assert(len == 4, "\"ABC\" = 3 chars + HLT");
    mu_assert(tryte_to_int64(code[0]) == 65, "'A'");
    mu_assert(tryte_to_int64(code[1]) == 66, "'B'");
    mu_assert(tryte_to_int64(code[2]) == 67, "'C'");
    free(code);
    return 0;
}

static int test_assemble_equ_directive(void) {
    size_t len;
    Tryte *code = asm_assemble(".equ COUNT 42\nLOAD ACC, COUNT\nHLT\n", &len);
    mu_assert(code != NULL, "equ directive");
    mu_assert(len == 3, "equ + LOAD + operand + HLT");
    mu_assert(tryte_to_int64(code[1]) == 42, "equ value 42");
    free(code);
    return 0;
}

static int test_assemble_equ_ternary(void) {
    size_t len;
    Tryte *code = asm_assemble(".equ VAL +-0\nLOAD ACC, VAL\nHLT\n", &len);
    mu_assert(code != NULL, "equ ternary");
    mu_assert(len == 3, "equ ternary length");
    mu_assert(tryte_to_int64(code[1]) == 6, "ternary +-0 = 6");
    free(code);
    return 0;
}

static int test_assemble_equ_string(void) {
    size_t len;
    Tryte *code = asm_assemble(".equ CH \"A\"\nLOAD ACC, CH\nHLT\n", &len);
    mu_assert(code != NULL, "equ string");
    mu_assert(tryte_to_int64(code[1]) == 65, "char 'A' = 65");
    free(code);
    return 0;
}

static int test_assemble_space_directive(void) {
    size_t len;
    Tryte *code = asm_assemble(".space 5\nHLT\n", &len);
    mu_assert(code != NULL, "space directive");
    mu_assert(len == 6, "5 zero trytes + HLT");
    int i;
    for (i = 0; i < 5; i++)
        mu_assert(tryte_to_int64(code[i]) == 0, "space emits zero");
    free(code);
    return 0;
}

static int test_assemble_fill_directive(void) {
    size_t len;
    Tryte *code = asm_assemble(".fill 3, 65\nHLT\n", &len);
    mu_assert(code != NULL, "fill directive");
    mu_assert(len == 4, "3 fill trytes + HLT");
    mu_assert(tryte_to_int64(code[0]) == 65, "fill value 65");
    mu_assert(tryte_to_int64(code[1]) == 65, "fill value 65");
    mu_assert(tryte_to_int64(code[2]) == 65, "fill value 65");
    free(code);
    return 0;
}

static int test_assemble_str_escapes(void) {
    size_t len;
    Tryte *code = asm_assemble(".str \"A\\nB\"\nHLT\n", &len);
    mu_assert(code != NULL, "str escapes");
    mu_assert(len == 4, "3 chars + HLT");
    mu_assert(tryte_to_int64(code[0]) == 65, "'A'");
    mu_assert(tryte_to_int64(code[1]) == 10, "'\\n' = 10");
    mu_assert(tryte_to_int64(code[2]) == 66, "'B'");
    free(code);
    return 0;
}

static int test_assemble_str_escape_tab(void) {
    size_t len;
    Tryte *code = asm_assemble(".str \"\\t\"\nHLT\n", &len);
    mu_assert(code != NULL, "str escape tab");
    mu_assert(tryte_to_int64(code[0]) == 9, "'\\t' = 9");
    free(code);
    return 0;
}

static int test_assemble_str_escape_backslash(void) {
    size_t len;
    Tryte *code = asm_assemble(".str \"\\\\\"\nHLT\n", &len);
    mu_assert(code != NULL, "str escape backslash");
    mu_assert(tryte_to_int64(code[0]) == 92, "'\\\\' = 92");
    free(code);
    return 0;
}

static int test_assemble_ld_opcode(void) {
    size_t len;
    Tryte *code = asm_assemble("LD ACC, 4000\nHLT\n", &len);
    mu_assert(code != NULL, "LD opcode");
    mu_assert(len == 3, "LD + operand + HLT");
    DecodedInstr di = decode_instr(code[0]);
    mu_assert(di.op == OP_LD, "decoded as LD");
    mu_assert(di.has_operand, "LD has operand");
    mu_assert(di.reg_dest == REG_ACC, "LD dest = ACC");
    mu_assert(tryte_to_int64(code[1]) == 4000, "LD addr = 4000");
    free(code);
    return 0;
}

static int test_assemble_shl_opcode(void) {
    size_t len;
    Tryte *code = asm_assemble("SHL ACC, B\nHLT\n", &len);
    mu_assert(code != NULL, "SHL opcode");
    mu_assert(len == 2, "SHL + HLT");
    DecodedInstr di = decode_instr(code[0]);
    mu_assert(di.op == OP_SHL, "decoded as SHL");
    mu_assert(di.reg_dest == REG_ACC, "SHL dest = ACC");
    mu_assert(di.reg_src == REG_B, "SHL src = B");
    free(code);
    return 0;
}

static int test_assemble_swap_opcode(void) {
    size_t len;
    Tryte *code = asm_assemble("SWAP ACC, B\nHLT\n", &len);
    mu_assert(code != NULL, "SWAP opcode");
    mu_assert(len == 2, "SWAP + HLT");
    DecodedInstr di = decode_instr(code[0]);
    mu_assert(di.op == OP_SWAP, "decoded as SWAP");
    free(code);
    return 0;
}

static int test_assemble_mod_opcode(void) {
    size_t len;
    Tryte *code = asm_assemble("MOD ACC, B\nHLT\n", &len);
    mu_assert(code != NULL, "MOD opcode");
    mu_assert(len == 2, "MOD + HLT");
    DecodedInstr di = decode_instr(code[0]);
    mu_assert(di.op == OP_MOD, "decoded as MOD");
    free(code);
    return 0;
}

static int test_assemble_equ_forward_ref(void) {
    size_t len;
    Tryte *code = asm_assemble("LOAD ACC, COUNT\n.equ COUNT 99\nHLT\n", &len);
    mu_assert(code != NULL, "equ forward ref");
    mu_assert(tryte_to_int64(code[1]) == 99, "forward equ resolves");
    free(code);
    return 0;
}

static int test_assemble_equ_duplicate(void) {
    size_t len;
    Tryte *code = asm_assemble(".equ X 1\n.equ X 2\nHLT\n", &len);
    mu_assert(code == NULL, "duplicate equ fails");
    return 0;
}

void test_asm_suite(void) {
    mu_run_test(test_assemble_simple);
    mu_run_test(test_assemble_operand);
    mu_run_test(test_assemble_ternary_literal);
    mu_run_test(test_assemble_string_literal);
    mu_run_test(test_assemble_str_directive);
    mu_run_test(test_assemble_db_directive);
    mu_run_test(test_assemble_label_backward);
    mu_run_test(test_assemble_forward_label);
    mu_run_test(test_assemble_comments);
    mu_run_test(test_assemble_blank_lines);
    mu_run_test(test_assemble_registers);
    mu_run_test(test_assemble_undefined_label);
    mu_run_test(test_assemble_undefined_label_ex);
    mu_run_test(test_assemble_unknown_instr);
    mu_run_test(test_assemble_equ_directive);
    mu_run_test(test_assemble_equ_ternary);
    mu_run_test(test_assemble_equ_string);
    mu_run_test(test_assemble_equ_forward_ref);
    mu_run_test(test_assemble_equ_duplicate);
    mu_run_test(test_assemble_space_directive);
    mu_run_test(test_assemble_fill_directive);
    mu_run_test(test_assemble_str_escapes);
    mu_run_test(test_assemble_str_escape_tab);
    mu_run_test(test_assemble_str_escape_backslash);
    mu_run_test(test_assemble_ld_opcode);
    mu_run_test(test_assemble_shl_opcode);
    mu_run_test(test_assemble_swap_opcode);
    mu_run_test(test_assemble_mod_opcode);
}
