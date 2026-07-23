#ifndef CALCPLUGIN_H
#define CALCPLUGIN_H

#include <math.h>
#include <stddef.h>

typedef enum {
    CALC_OK       =  0,
    CALC_DIV_ZERO = -1, /* деление (или остаток) на ноль */
    CALC_DOMAIN   = -2, /* аргумент вне области определения */
    CALC_RANGE    = -3  /* результат вне диапазона double */
} CalcStatus;

enum {
    CALC_NARY   = 0, /* количество аргументов задаёт пользователь */
    CALC_UNARY  = 1,
    CALC_BINARY = 2
};

/* Единая сигнатура для всех операций: аргументы массивом.
 * Это позволяет держать один тип указателя вместо union по арности. */
typedef CalcStatus (*CalcFn)(const double *args, size_t n, double *res);

/* Имена символов, которые приложение ищет через dlsym */
#define CALC_SYM_NAME   "calc_name"
#define CALC_SYM_SYMBOL "calc_symbol"
#define CALC_SYM_ARITY  "calc_arity"
#define CALC_SYM_APPLY  "calc_apply"

/* Общая проверка результата. Объявлена static inline прямо в
 * заголовке: у каждого плагина своя копия, связывание не нужно. */
static inline CalcStatus calc_finish(double r, double *res)
{
    if (isnan(r))
        return CALC_DOMAIN;
    if (isinf(r))
        return CALC_RANGE;
    *res = r;
    return CALC_OK;
}

#endif /* CALCPLUGIN_H */
