#include "minunit.h"

int mu_tests_run = 0;
int mu_tests_failed = 0;

void test_trit_suite(void);
void test_gate_suite(void);
void test_arith_suite(void);
void test_cpu_suite(void);
void test_asm_suite(void);
void test_vm_suite(void);

int main(void) {
    printf("=== Trinary Test Suite ===\n\n");
    mu_run_suite(test_trit_suite);
    mu_run_suite(test_gate_suite);
    mu_run_suite(test_arith_suite);
    mu_run_suite(test_cpu_suite);
    mu_run_suite(test_asm_suite);
    mu_run_suite(test_vm_suite);

    printf("\n=== Results ===\n");
    printf("Tests run: %d\n", mu_tests_run);
    printf("Failed:    %d\n", mu_tests_failed);

    return mu_tests_failed > 0 ? 1 : 0;
}
