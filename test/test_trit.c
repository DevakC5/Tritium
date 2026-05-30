#include "minunit.h"
#include "trit.h"
#include <string.h>

static int test_trit_from_char(void) {
    mu_assert(trit_from_char('-') == TRIT_NEG, "- -> NEG");
    mu_assert(trit_from_char('0') == TRIT_ZERO, "0 -> ZERO");
    mu_assert(trit_from_char('+') == TRIT_POS, "+ -> POS");
    mu_assert(trit_from_char('N') == TRIT_NEG, "N -> NEG");
    mu_assert(trit_from_char('P') == TRIT_POS, "P -> POS");
    return 0;
}

static int test_trit_to_char(void) {
    mu_assert(trit_to_char(TRIT_NEG) == '-', "NEG -> -");
    mu_assert(trit_to_char(TRIT_ZERO) == '0', "ZERO -> 0");
    mu_assert(trit_to_char(TRIT_POS) == '+', "POS -> +");
    return 0;
}

static int test_tryte_from_int64(void) {
    Tryte t;
    tryte_from_int64(0, &t);
    mu_assert(tryte_is_zero(t), "0 is zero");

    tryte_from_int64(1, &t);
    mu_assert(t.t[0] == TRIT_POS, "1: LSB is +");
    for (int i = 1; i < TRYTE_NTRITS; i++) mu_assert(t.t[i] == TRIT_ZERO, "1: rest zero");

    tryte_from_int64(-1, &t);
    mu_assert(t.t[0] == TRIT_NEG, "-1: LSB is -");

    tryte_from_int64(5, &t);
    mu_assert(tryte_to_int64(t) == 5, "5 roundtrip");

    tryte_from_int64(-5, &t);
    mu_assert(tryte_to_int64(t) == -5, "-5 roundtrip");

    tryte_from_int64(13, &t);
    mu_assert(tryte_to_int64(t) == 13, "13 roundtrip");

    return 0;
}

static int test_tryte_to_str(void) {
    Tryte t;
    char buf[16];

    tryte_from_int64(0, &t);
    tryte_to_str(t, buf, sizeof(buf));
    mu_assert(strcmp(buf, "000000000") == 0, "0 string");

    tryte_from_int64(1, &t);
    tryte_to_str(t, buf, sizeof(buf));
    mu_assert(strcmp(buf, "00000000+") == 0, "1 string");

    tryte_from_int64(-1, &t);
    tryte_to_str(t, buf, sizeof(buf));
    mu_assert(strcmp(buf, "00000000-") == 0, "-1 string");

    return 0;
}

static int test_tryte_from_str(void) {
    Tryte t;
    tryte_from_str("+00-", &t);
    mu_assert(t.t[0] == TRIT_NEG, "LSB");
    mu_assert(t.t[1] == TRIT_ZERO, "2nd");
    mu_assert(t.t[2] == TRIT_ZERO, "3rd");
    mu_assert(t.t[3] == TRIT_POS, "4th");
    for (int i = 4; i < TRYTE_NTRITS; i++)
        mu_assert(t.t[i] == TRIT_ZERO, "rest zero");
    return 0;
}

static int test_tryte_cmp(void) {
    Tryte a, b;
    tryte_from_int64(5, &a);
    tryte_from_int64(3, &b);
    mu_assert(tryte_cmp(a, b) > 0, "5 > 3");
    mu_assert(tryte_cmp(b, a) < 0, "3 < 5");
    mu_assert(tryte_cmp(a, a) == 0, "5 == 5");
    return 0;
}

void test_trit_suite(void) {
    mu_run_test(test_trit_from_char);
    mu_run_test(test_trit_to_char);
    mu_run_test(test_tryte_from_int64);
    mu_run_test(test_tryte_to_str);
    mu_run_test(test_tryte_from_str);
    mu_run_test(test_tryte_cmp);
}
