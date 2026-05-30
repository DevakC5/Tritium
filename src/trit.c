#include "trit.h"

char trit_to_char(Trit t) {
    if (t < 0) return '-';
    if (t > 0) return '+';
    return '0';
}

Trit trit_from_char(char c) {
    if (c == '-' || c == 'N' || c == 'n') return TRIT_NEG;
    if (c == '+' || c == 'P' || c == 'p') return TRIT_POS;
    return TRIT_ZERO;
}

void tryte_from_int64(int64_t val, Tryte *out) {
    int64_t n = val;
    memset(out, 0, sizeof(Tryte));
    for (int i = 0; i < TRYTE_NTRITS && n != 0; i++) {
        int rem = (int)(n % 3);
        n /= 3;
        if (rem == 2) { rem = -1; n += 1; }
        else if (rem == -2) { rem = 1; n -= 1; }
        out->t[i] = (Trit)rem;
    }
}

int64_t tryte_to_int64(Tryte t) {
    int64_t result = 0;
    int64_t place = 1;
    for (int i = 0; i < TRYTE_NTRITS; i++) {
        result += (int64_t)t.t[i] * place;
        place *= 3;
    }
    return result;
}

void tryte_from_str(const char *s, Tryte *out) {
    size_t len = strlen(s);
    memset(out, 0, sizeof(Tryte));
    for (size_t i = 0; i < len && i < (size_t)TRYTE_NTRITS; i++) {
        out->t[len - 1 - i] = trit_from_char(s[i]);
    }
}

void tryte_to_str(Tryte t, char *buf, size_t len) {
    if (len == 0) return;
    size_t i;
    for (i = 0; i < (size_t)TRYTE_NTRITS && i < len - 1; i++) {
        buf[i] = trit_to_char(t.t[TRYTE_NTRITS - 1 - i]);
    }
    buf[i] = '\0';
}

int tryte_is_zero(Tryte t) {
    for (int i = 0; i < TRYTE_NTRITS; i++)
        if (t.t[i] != 0) return 0;
    return 1;
}

void tryte_zero(Tryte *t) {
    memset(t, 0, sizeof(Tryte));
}

int tryte_cmp(Tryte a, Tryte b) {
    int64_t va = tryte_to_int64(a);
    int64_t vb = tryte_to_int64(b);
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}
