#include "minunit.h"
#include "gate.h"

static int test_not(void) {
    mu_assert(trit_not(TRIT_NEG) == TRIT_POS, "not - -> +");
    mu_assert(trit_not(TRIT_ZERO) == TRIT_ZERO, "not 0 -> 0");
    mu_assert(trit_not(TRIT_POS) == TRIT_NEG, "not + -> -");
    return 0;
}

static int test_and(void) {
    mu_assert(trit_and(TRIT_NEG, TRIT_NEG) == TRIT_NEG, "N and N = N");
    mu_assert(trit_and(TRIT_NEG, TRIT_POS) == TRIT_NEG, "N and P = N");
    mu_assert(trit_and(TRIT_POS, TRIT_POS) == TRIT_POS, "P and P = P");
    mu_assert(trit_and(TRIT_ZERO, TRIT_POS) == TRIT_ZERO, "0 and P = 0");
    mu_assert(trit_and(TRIT_NEG, TRIT_ZERO) == TRIT_NEG, "N and 0 = N");
    return 0;
}

static int test_or(void) {
    mu_assert(trit_or(TRIT_NEG, TRIT_NEG) == TRIT_NEG, "N or N = N");
    mu_assert(trit_or(TRIT_NEG, TRIT_POS) == TRIT_POS, "N or P = P");
    mu_assert(trit_or(TRIT_POS, TRIT_POS) == TRIT_POS, "P or P = P");
    mu_assert(trit_or(TRIT_ZERO, TRIT_POS) == TRIT_POS, "0 or P = P");
    mu_assert(trit_or(TRIT_NEG, TRIT_ZERO) == TRIT_ZERO, "N or 0 = 0");
    return 0;
}

static int test_xor(void) {
    mu_assert(trit_xor(TRIT_NEG, TRIT_NEG) == TRIT_POS, "N xor N = P");
    mu_assert(trit_xor(TRIT_NEG, TRIT_POS) == TRIT_ZERO, "N xor P = 0");
    mu_assert(trit_xor(TRIT_POS, TRIT_POS) == TRIT_NEG, "P xor P = N");
    mu_assert(trit_xor(TRIT_ZERO, TRIT_POS) == TRIT_POS, "0 xor P = P");
    mu_assert(trit_xor(TRIT_NEG, TRIT_ZERO) == TRIT_NEG, "N xor 0 = N");
    mu_assert(trit_xor(TRIT_ZERO, TRIT_ZERO) == TRIT_ZERO, "0 xor 0 = 0");
    return 0;
}

static int test_tryte_not(void) {
    Tryte a, r;
    tryte_from_int64(5, &a);
    tryte_not(a, &r);
    mu_assert(tryte_to_int64(r) == -5, "negate 5");
    return 0;
}

static int test_tryte_and(void) {
    Tryte a, b, r;
    tryte_from_int64(1, &a);   // 1 = P trit at LSB
    tryte_from_int64(-1, &b);  // -1 = N trit at LSB
    tryte_and(a, b, &r);
    mu_assert(tryte_to_int64(r) == -1, "per-trit min of 1 and -1 = -1");
    return 0;
}

static int test_tryte_or(void) {
    Tryte a, b, r;
    tryte_from_int64(1, &a);
    tryte_from_int64(-1, &b);
    tryte_or(a, b, &r);
    mu_assert(tryte_to_int64(r) == 1, "per-trit max of 1 and -1 = 1");
    return 0;
}

void test_gate_suite(void) {
    mu_run_test(test_not);
    mu_run_test(test_and);
    mu_run_test(test_or);
    mu_run_test(test_xor);
    mu_run_test(test_tryte_not);
    mu_run_test(test_tryte_and);
    mu_run_test(test_tryte_or);
}
