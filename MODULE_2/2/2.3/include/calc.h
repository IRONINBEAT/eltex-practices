#ifndef CALC_H
#define CALC_H

#include <stddef.h>

/* Результат операции пишется в *res и остаётся нетронутым при ошибке */

typedef enum {
    CALC_OK       =  0,
    CALC_DIV_ZERO = -1, /* деление (или остаток) на ноль */
    CALC_DOMAIN   = -2, /* аргумент вне области определения */
    CALC_RANGE    = -3  /* результат вне диапазона double */
} CalcStatus;

CalcStatus calc_add(double a, double b, double *res);
CalcStatus calc_sub(double a, double b, double *res);
CalcStatus calc_mul(double a, double b, double *res);
CalcStatus calc_div(double a, double b, double *res);
CalcStatus calc_pow(double a, double b, double *res);
CalcStatus calc_mod(double a, double b, double *res);

CalcStatus calc_sqrt(double a, double *res);

CalcStatus calc_sum(const double *args, size_t n, double *res);
CalcStatus calc_avg(const double *args, size_t n, double *res);

const char *calc_strerror(CalcStatus st);

#endif /* CALC_H */
