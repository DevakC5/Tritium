#ifndef TRINARY_ARITH_H
#define TRINARY_ARITH_H

#include "trit.h"

Tryte tryte_add(Tryte a, Tryte b);
Tryte tryte_sub(Tryte a, Tryte b);
Tryte tryte_mul(Tryte a, Tryte b);
Tryte tryte_div(Tryte a, Tryte b, Tryte *rem);
Tryte tryte_neg(Tryte a);
Tryte tryte_abs(Tryte a);

#endif
