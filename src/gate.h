#ifndef TRINARY_GATE_H
#define TRINARY_GATE_H

#include "trit.h"

Trit trit_not(Trit a);
Trit trit_and(Trit a, Trit b);
Trit trit_or(Trit a, Trit b);
Trit trit_xor(Trit a, Trit b);
Trit trit_nand(Trit a, Trit b);
Trit trit_nor(Trit a, Trit b);
Trit trit_consensus(Trit a, Trit b);
Trit trit_any(Trit a, Trit b);
Trit trit_implies(Trit a, Trit b);

void tryte_not(Tryte a, Tryte *out);
void tryte_and(Tryte a, Tryte b, Tryte *out);
void tryte_or(Tryte a, Tryte b, Tryte *out);
void tryte_xor(Tryte a, Tryte b, Tryte *out);

#endif
