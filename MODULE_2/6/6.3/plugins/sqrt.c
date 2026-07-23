#include "calcplugin.h"

const char *calc_name   = "Квадратный корень";
const char *calc_symbol = "sqrt";
const int   calc_arity  = CALC_UNARY;

CalcStatus calc_apply(const double *a, size_t n, double *res)
{
    if (n != 1)
        return CALC_DOMAIN;
    if (a[0] < 0.0)
        return CALC_DOMAIN;
    return calc_finish(sqrt(a[0]), res);
}
