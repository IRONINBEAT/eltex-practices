#include "calcplugin.h"

const char *calc_name   = "Сумма чисел";
const char *calc_symbol = "sum";
const int   calc_arity  = CALC_NARY;

CalcStatus calc_apply(const double *a, size_t n, double *res)
{
    double s = 0.0;

    for (size_t i = 0; i < n; i++)
        s += a[i];
    return calc_finish(s, res);
}
