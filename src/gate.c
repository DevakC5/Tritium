#include "gate.h"

Trit trit_not(Trit a) { return (Trit)(-a); }

Trit trit_and(Trit a, Trit b) { return (Trit)(a < b ? a : b); }

Trit trit_or(Trit a, Trit b) { return (Trit)(a > b ? a : b); }

Trit trit_xor(Trit a, Trit b) {
    int sum = (int)a + (int)b;
    if (sum == 2 || sum == -2) return (Trit)(-sum / 2);
    return (Trit)sum;
}

Trit trit_nand(Trit a, Trit b) { return trit_not(trit_and(a, b)); }

Trit trit_nor(Trit a, Trit b) { return trit_not(trit_or(a, b)); }

Trit trit_consensus(Trit a, Trit b) { return trit_and(a, b); }

Trit trit_any(Trit a, Trit b) { return trit_or(a, b); }

Trit trit_implies(Trit a, Trit b) {
    if (a == TRIT_POS && b == TRIT_NEG) return TRIT_NEG;
    if (a == TRIT_POS && b == TRIT_ZERO) return TRIT_ZERO;
    return TRIT_POS;
}

void tryte_not(Tryte a, Tryte *out) {
    for (int i = 0; i < TRYTE_NTRITS; i++)
        out->t[i] = trit_not(a.t[i]);
}

void tryte_and(Tryte a, Tryte b, Tryte *out) {
    for (int i = 0; i < TRYTE_NTRITS; i++)
        out->t[i] = trit_and(a.t[i], b.t[i]);
}

void tryte_or(Tryte a, Tryte b, Tryte *out) {
    for (int i = 0; i < TRYTE_NTRITS; i++)
        out->t[i] = trit_or(a.t[i], b.t[i]);
}

void tryte_xor(Tryte a, Tryte b, Tryte *out) {
    for (int i = 0; i < TRYTE_NTRITS; i++)
        out->t[i] = trit_xor(a.t[i], b.t[i]);
}
