#ifndef MINUNIT_H
#define MINUNIT_H

#include <stdio.h>
#include <stdlib.h>

extern int mu_tests_run;
extern int mu_tests_failed;

#define mu_assert(test, message) do { \
    mu_tests_run++; \
    if (!(test)) { \
        mu_tests_failed++; \
        fprintf(stderr, "  FAIL: %s:%d: %s\n", \
                __FILE__, __LINE__, message); \
        return 1; \
    } \
} while (0)

#define mu_run_test(test) do { \
    printf("  %s ... ", #test); \
    fflush(stdout); \
    if (test()) { \
        printf("FAIL\n"); \
    } else { \
        printf("OK\n"); \
    } \
} while (0)

#define mu_run_suite(suite) do { \
    printf("[%s]\n", #suite); \
    suite(); \
} while (0)

#endif
