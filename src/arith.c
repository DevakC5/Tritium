#include "arith.h"

Tryte tryte_add(Tryte a, Tryte b) {
    Tryte result;
    Trit carry = TRIT_ZERO;
    for (int i = 0; i < TRYTE_NTRITS; i++) {
        int sum = (int)a.t[i] + (int)b.t[i] + (int)carry;
        if (sum == 2)  { result.t[i] = TRIT_NEG;  carry = TRIT_POS; }
        else if (sum == -2) { result.t[i] = TRIT_POS;  carry = TRIT_NEG; }
        else if (sum == 3)  { result.t[i] = TRIT_ZERO; carry = TRIT_POS; }
        else if (sum == -3) { result.t[i] = TRIT_ZERO; carry = TRIT_NEG; }
        else                { result.t[i] = (Trit)sum;  carry = TRIT_ZERO; }
    }
    return result;
}

Tryte tryte_sub(Tryte a, Tryte b) {
    Tryte neg_b = tryte_neg(b);
    return tryte_add(a, neg_b);
}

Tryte tryte_mul(Tryte a, Tryte b) {
    Tryte result;
    tryte_zero(&result);
    for (int i = 0; i < TRYTE_NTRITS; i++) {
        if (b.t[i] == TRIT_NEG) {
            Tryte shifted;
            tryte_zero(&shifted);
            for (int j = 0; j + i < TRYTE_NTRITS; j++)
                shifted.t[j + i] = (Trit)(-a.t[j]);
            result = tryte_add(result, shifted);
        } else if (b.t[i] == TRIT_POS) {
            Tryte shifted;
            tryte_zero(&shifted);
            for (int j = 0; j + i < TRYTE_NTRITS; j++)
                shifted.t[j + i] = a.t[j];
            result = tryte_add(result, shifted);
        }
    }
    return result;
}

Tryte tryte_div(Tryte a, Tryte b, Tryte *rem) {
    Tryte zero;
    tryte_zero(&zero);
    if (tryte_is_zero(b)) { tryte_zero(&zero); if (rem) *rem = zero; return zero; }

    int64_t na = tryte_to_int64(a);
    int64_t nb = tryte_to_int64(b);
    Tryte q, r;
    tryte_from_int64(na / nb, &q);
    tryte_from_int64(na % nb, &r);
    if (rem) *rem = r;
    return q;
}

Tryte tryte_neg(Tryte a) {
    Tryte result;
    for (int i = 0; i < TRYTE_NTRITS; i++)
        result.t[i] = (Trit)(-a.t[i]);
    return result;
}

Tryte tryte_abs(Tryte a) {
    int64_t v = tryte_to_int64(a);
    if (v < 0) return tryte_neg(a);
    return a;
}
