#include "calcplugin.h"

const char *calc_name   = "Остаток от деления";
const char *calc_symbol = "%";
const int   calc_arity  = CALC_BINARY;

CalcStatus calc_apply(const double *a, size_t n, double *res)
{
    if (n != 2)
        return CALC_DOMAIN;
    if (a[1] == 0.0)
        return CALC_DIV_ZERO;
    return calc_finish(fmod(a[0], a[1]), res);
}
