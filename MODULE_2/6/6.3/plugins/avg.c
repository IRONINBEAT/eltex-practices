#include "calcplugin.h"

const char *calc_name   = "Среднее арифметическое";
const char *calc_symbol = "avg";
const int   calc_arity  = CALC_NARY;

CalcStatus calc_apply(const double *a, size_t n, double *res)
{
    double s = 0.0;

    if (n == 0)
        return CALC_DIV_ZERO; /* среднее по пустому набору не определено */
    for (size_t i = 0; i < n; i++)
        s += a[i];
    return calc_finish(s / (double)n, res);
}
