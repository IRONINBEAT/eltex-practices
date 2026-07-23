#include "calcplugin.h"

const char *calc_name   = "Умножение";
const char *calc_symbol = "*";
const int   calc_arity  = CALC_BINARY;

CalcStatus calc_apply(const double *a, size_t n, double *res)
{
    if (n != 2)
        return CALC_DOMAIN;
    return calc_finish(a[0] * a[1], res);
}
