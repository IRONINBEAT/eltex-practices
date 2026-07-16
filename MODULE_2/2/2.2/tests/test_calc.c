#include <math.h>
#include <stdio.h>

#include "calc.h"

static int tests_run;
static int tests_failed;

#define ASSERT(cond) do {                                             \
    tests_run++;                                                      \
    if (!(cond)) {                                                    \
        tests_failed++;                                               \
        printf("FAIL: %s (%s:%d)\n", #cond, __FILE__, __LINE__);      \
    }                                                                 \
} while (0)

/* Точное == для дробных небезопасно */
static int approx(double a, double b)
{
    return fabs(a - b) < 1e-9;
}

static void test_add(void)
{
    double r;
    ASSERT(calc_add(2, 3, &r) == CALC_OK && approx(r, 5));
    ASSERT(calc_add(-2.5, 2.5, &r) == CALC_OK && approx(r, 0));
    ASSERT(calc_add(1e308, 1e308, &r) == CALC_RANGE);
}

static void test_sub(void)
{
    double r;
    ASSERT(calc_sub(10, 4, &r) == CALC_OK && approx(r, 6));
    ASSERT(calc_sub(0, 5, &r) == CALC_OK && approx(r, -5));
}

static void test_mul(void)
{
    double r;
    ASSERT(calc_mul(6, 7, &r) == CALC_OK && approx(r, 42));
    ASSERT(calc_mul(-3, 4, &r) == CALC_OK && approx(r, -12));
    ASSERT(calc_mul(1e200, 1e200, &r) == CALC_RANGE);
}

static void test_div(void)
{
    double r;
    ASSERT(calc_div(10, 4, &r) == CALC_OK && approx(r, 2.5));
    ASSERT(calc_div(-9, 3, &r) == CALC_OK && approx(r, -3));
    r = 123;
    ASSERT(calc_div(5, 0, &r) == CALC_DIV_ZERO && approx(r, 123));
}

static void test_pow(void)
{
    double r;
    ASSERT(calc_pow(2, 10, &r) == CALC_OK && approx(r, 1024));
    ASSERT(calc_pow(9, 0.5, &r) == CALC_OK && approx(r, 3));
    ASSERT(calc_pow(5, 0, &r) == CALC_OK && approx(r, 1));
    ASSERT(calc_pow(2, -1, &r) == CALC_OK && approx(r, 0.5));
    ASSERT(calc_pow(-8, 0.5, &r) == CALC_DOMAIN);
}

static void test_mod(void)
{
    double r;
    ASSERT(calc_mod(10, 3, &r) == CALC_OK && approx(r, 1));
    ASSERT(calc_mod(9, 3, &r) == CALC_OK && approx(r, 0));
    ASSERT(calc_mod(5.5, 2, &r) == CALC_OK && approx(r, 1.5));
    ASSERT(calc_mod(7, 0, &r) == CALC_DIV_ZERO);
}

static void test_sqrt(void)
{
    double r;
    ASSERT(calc_sqrt(16, &r) == CALC_OK && approx(r, 4));
    ASSERT(calc_sqrt(2, &r) == CALC_OK && approx(r, 1.41421356237));
    ASSERT(calc_sqrt(0, &r) == CALC_OK && approx(r, 0));
    ASSERT(calc_sqrt(-1, &r) == CALC_DOMAIN);
}

static void test_strerror(void)
{
    ASSERT(calc_strerror(CALC_OK)[0] != '\0');
    ASSERT(calc_strerror(CALC_DIV_ZERO)[0] != '\0');
    ASSERT(calc_strerror(CALC_DOMAIN)[0] != '\0');
    ASSERT(calc_strerror(CALC_RANGE)[0] != '\0');
}

int main(void)
{
    test_add();
    test_sub();
    test_mul();
    test_div();
    test_pow();
    test_mod();
    test_sqrt();
    test_strerror();

    printf("Тестов выполнено: %d, провалено: %d\n",
           tests_run, tests_failed);
    if (tests_failed == 0)
        printf("ВСЕ ТЕСТЫ ПРОЙДЕНЫ\n");
    return tests_failed != 0;
}
