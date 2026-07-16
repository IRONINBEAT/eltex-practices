#include <math.h>

#include "calc.h"

/* NaN означает недопустимый аргумент, бесконечность — переполнение */
static CalcStatus finish(double r, double *res)
{
    if (isnan(r))
        return CALC_DOMAIN;
    if (isinf(r))
        return CALC_RANGE;
    *res = r;
    return CALC_OK;
}

CalcStatus calc_add(double a, double b, double *res)
{
    return finish(a + b, res);
}

CalcStatus calc_sub(double a, double b, double *res)
{
    return finish(a - b, res);
}

CalcStatus calc_mul(double a, double b, double *res)
{
    return finish(a * b, res);
}

CalcStatus calc_div(double a, double b, double *res)
{
    if (b == 0.0)
        return CALC_DIV_ZERO;
    return finish(a / b, res);
}

CalcStatus calc_pow(double a, double b, double *res)
{
    /* pow вернёт NaN для отрицательного основания с дробной степенью */
    return finish(pow(a, b), res);
}

CalcStatus calc_mod(double a, double b, double *res)
{
    if (b == 0.0)
        return CALC_DIV_ZERO;
    return finish(fmod(a, b), res);
}

CalcStatus calc_sqrt(double a, double *res)
{
    if (a < 0.0)
        return CALC_DOMAIN;
    return finish(sqrt(a), res);
}

const char *calc_strerror(CalcStatus st)
{
    switch (st) {
    case CALC_OK:       return "успешно";
    case CALC_DIV_ZERO: return "деление на ноль";
    case CALC_DOMAIN:   return "аргумент вне области определения";
    case CALC_RANGE:    return "результат слишком велик (переполнение)";
    default:            return "неизвестная ошибка";
    }
}
