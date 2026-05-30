#ifndef TRINARY_TRIT_H
#define TRINARY_TRIT_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#define TRIT_NEG  ((int8_t)-1)
#define TRIT_ZERO ((int8_t)0)
#define TRIT_POS  ((int8_t)1)
#define TRYTE_NTRITS 9

typedef int8_t Trit;

typedef struct {
    Trit t[TRYTE_NTRITS];
} Tryte;

char trit_to_char(Trit t);
Trit trit_from_char(char c);
void tryte_from_int64(int64_t val, Tryte *out);
int64_t tryte_to_int64(Tryte t);
void tryte_from_str(const char *s, Tryte *out);
void tryte_to_str(Tryte t, char *buf, size_t len);
int tryte_is_zero(Tryte t);
void tryte_zero(Tryte *t);
int tryte_cmp(Tryte a, Tryte b);

#endif
