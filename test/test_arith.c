#include "minunit.h"
#include "arith.h"

static int test_add(void) {
    Tryte a, b, r;
    tryte_from_int64(0, &a);
    tryte_from_int64(0, &b);
    r = tryte_add(a, b);
    mu_assert(tryte_to_int64(r) == 0, "0+0=0");

    tryte_from_int64(5, &a);
    tryte_from_int64(3, &b);
    r = tryte_add(a, b);
    mu_assert(tryte_to_int64(r) == 8, "5+3=8");

    tryte_from_int64(-5, &a);
    tryte_from_int64(3, &b);
    r = tryte_add(a, b);
    mu_assert(tryte_to_int64(r) == -2, "-5+3=-2");

    tryte_from_int64(1000, &a);
    tryte_from_int64(2000, &b);
    r = tryte_add(a, b);
    mu_assert(tryte_to_int64(r) == 3000, "1000+2000=3000");

    tryte_from_int64(1, &a);
    tryte_from_int64(-1, &b);
    r = tryte_add(a, b);
    mu_assert(tryte_is_zero(r), "1+(-1)=0");

    tryte_from_int64(-5, &a);
    tryte_from_int64(-3, &b);
    r = tryte_add(a, b);
    mu_assert(tryte_to_int64(r) == -8, "-5+(-3)=-8");

    return 0;
}

static int test_sub(void) {
    Tryte a, b, r;
    tryte_from_int64(10, &a);
    tryte_from_int64(3, &b);
    r = tryte_sub(a, b);
    mu_assert(tryte_to_int64(r) == 7, "10-3=7");

    tryte_from_int64(3, &a);
    tryte_from_int64(10, &b);
    r = tryte_sub(a, b);
    mu_assert(tryte_to_int64(r) == -7, "3-10=-7");

    tryte_from_int64(-5, &a);
    tryte_from_int64(-3, &b);
    r = tryte_sub(a, b);
    mu_assert(tryte_to_int64(r) == -2, "-5-(-3)=-2");

    return 0;
}

static int test_mul(void) {
    Tryte a, b, r;
    tryte_from_int64(5, &a);
    tryte_from_int64(3, &b);
    r = tryte_mul(a, b);
    mu_assert(tryte_to_int64(r) == 15, "5*3=15");

    tryte_from_int64(-4, &a);
    tryte_from_int64(3, &b);
    r = tryte_mul(a, b);
    mu_assert(tryte_to_int64(r) == -12, "-4*3=-12");

    tryte_from_int64(0, &a);
    tryte_from_int64(5, &b);
    r = tryte_mul(a, b);
    mu_assert(tryte_to_int64(r) == 0, "0*5=0");

    tryte_from_int64(-2, &a);
    tryte_from_int64(-3, &b);
    r = tryte_mul(a, b);
    mu_assert(tryte_to_int64(r) == 6, "-2*-3=6");

    tryte_from_int64(7, &a);
    tryte_from_int64(7, &b);
    r = tryte_mul(a, b);
    mu_assert(tryte_to_int64(r) == 49, "7*7=49");

    return 0;
}

static int test_div(void) {
    Tryte a, b, r, rem;
    tryte_from_int64(15, &a);
    tryte_from_int64(3, &b);
    r = tryte_div(a, b, &rem);
    mu_assert(tryte_to_int64(r) == 5, "15/3=5");
    mu_assert(tryte_is_zero(rem), "rem=0");

    tryte_from_int64(17, &a);
    tryte_from_int64(5, &b);
    r = tryte_div(a, b, &rem);
    mu_assert(tryte_to_int64(r) == 3, "17/5=3");
    mu_assert(tryte_to_int64(rem) == 2, "rem=2");

    tryte_from_int64(-15, &a);
    tryte_from_int64(4, &b);
    r = tryte_div(a, b, &rem);
    mu_assert(tryte_to_int64(r) == -3, "-15/4=-3");

    tryte_from_int64(0, &a);
    tryte_from_int64(5, &b);
    r = tryte_div(a, b, &rem);
    mu_assert(tryte_is_zero(r), "0/5=0");

    return 0;
}

static int test_neg(void) {
    Tryte a, r;
    tryte_from_int64(5, &a);
    r = tryte_neg(a);
    mu_assert(tryte_to_int64(r) == -5, "neg 5");

    tryte_from_int64(-3, &a);
    r = tryte_neg(a);
    mu_assert(tryte_to_int64(r) == 3, "neg -3");

    tryte_from_int64(0, &a);
    r = tryte_neg(a);
    mu_assert(tryte_is_zero(r), "neg 0");

    return 0;
}

void test_arith_suite(void) {
    mu_run_test(test_add);
    mu_run_test(test_sub);
    mu_run_test(test_mul);
    mu_run_test(test_div);
    mu_run_test(test_neg);
}
