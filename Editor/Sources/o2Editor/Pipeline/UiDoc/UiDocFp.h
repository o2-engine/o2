#pragma once

// Included last by the .cpp files of the UI layout port (never by a header): no floating-point contraction
// (a*b + c stays two roundings, as V8 computes it), so the solver, the text fitting and the exports give the
// web's numbers to the last bit
#if defined(__clang__)
#pragma clang fp contract(off)
#elif defined(_MSC_VER)
#pragma fp_contract(off)
#elif defined(__GNUC__)
#pragma GCC optimize("fp-contract=off")
#endif
