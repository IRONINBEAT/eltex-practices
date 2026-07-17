#include <math.h>
#include <stdio.h>
#include <string.h>

#include "calc.h"
#include "command.h"

static int tests_run;
static int tests_failed;

#define ASSERT(cond) do {                                             \
    tests_run++;                                                      \
    if (!(cond)) {                                                    \
        tests_failed++;                                               \
        printf("FAIL: %s (%s:%d)\n", #cond, __FILE__, __LINE__);      \
    }                                                                 \
} while (0)

/* Как ASSERT, но прерывает тест: ставится перед разыменованием
 * указателя, иначе провалившийся тест упал бы вместо отчёта. */
#define REQUIRE(cond) do {                                            \
    tests_run++;                                                      \
    if (!(cond)) {                                                    \
        tests_failed++;                                               \
        printf("FAIL: %s (%s:%d) — тест прерван\n",                   \
               #cond, __FILE__, __LINE__);                            \
        return;                                                       \
    }                                                                 \
} while (0)

static int approx(double a, double b)
{
    return fabs(a - b) < 1e-9;
}

static void test_binary_ops(void)
{
    double r;
    ASSERT(calc_add(2, 3, &r) == CALC_OK && approx(r, 5));
    ASSERT(calc_sub(10, 4, &r) == CALC_OK && approx(r, 6));
    ASSERT(calc_mul(6, 7, &r) == CALC_OK && approx(r, 42));
    ASSERT(calc_div(10, 4, &r) == CALC_OK && approx(r, 2.5));
    ASSERT(calc_pow(2, 10, &r) == CALC_OK && approx(r, 1024));
    ASSERT(calc_mod(10, 3, &r) == CALC_OK && approx(r, 1));
}

static void test_errors(void)
{
    double r = 123;
    ASSERT(calc_div(5, 0, &r) == CALC_DIV_ZERO && approx(r, 123));
    ASSERT(calc_mod(7, 0, &r) == CALC_DIV_ZERO);
    ASSERT(calc_sqrt(-1, &r) == CALC_DOMAIN);
    ASSERT(calc_pow(-8, 0.5, &r) == CALC_DOMAIN);
    ASSERT(calc_mul(1e200, 1e200, &r) == CALC_RANGE);
}

static void test_sqrt(void)
{
    double r;
    ASSERT(calc_sqrt(16, &r) == CALC_OK && approx(r, 4));
    ASSERT(calc_sqrt(0, &r) == CALC_OK && approx(r, 0));
}

static void test_nary(void)
{
    double r;
    double a[] = {1, 2, 3, 4};

    ASSERT(calc_sum(a, 4, &r) == CALC_OK && approx(r, 10));
    ASSERT(calc_avg(a, 4, &r) == CALC_OK && approx(r, 2.5));
    ASSERT(calc_sum(a, 1, &r) == CALC_OK && approx(r, 1));
    ASSERT(calc_avg(a, 0, &r) == CALC_DIV_ZERO);
}

static void test_table_registration(void)
{
    CommandTable t;

    ct_init(&t);
    ASSERT(ct_count(&t) == 0);

    ASSERT(ct_add_binary(&t, "Сложение", "+", calc_add) == 0);
    ASSERT(ct_add_unary(&t, "Корень", "sqrt", calc_sqrt) == 0);
    ASSERT(ct_add_nary(&t, "Сумма", "sum", calc_sum) == 0);
    ASSERT(ct_count(&t) == 3);

    const Command *c = ct_get(&t, 0);
    REQUIRE(c != NULL);
    ASSERT(c->arity == OP_BINARY);
    REQUIRE(ct_get(&t, 1) != NULL && ct_get(&t, 2) != NULL);
    ASSERT(ct_get(&t, 1)->arity == OP_UNARY);
    ASSERT(ct_get(&t, 2)->arity == OP_NARY);

    ASSERT(ct_get(&t, 3) == NULL);
}

static void test_table_dispatch(void)
{
    CommandTable t;
    double r;

    ct_init(&t);
    ct_add_binary(&t, "Умножение", "*", calc_mul);
    ct_add_unary(&t, "Корень", "sqrt", calc_sqrt);
    ct_add_nary(&t, "Среднее", "avg", calc_avg);

    const Command *mul = ct_get(&t, 0);
    REQUIRE(mul != NULL);
    ASSERT(mul->fn.binary(6, 7, &r) == CALC_OK && approx(r, 42));

    const Command *sq = ct_get(&t, 1);
    REQUIRE(sq != NULL);
    ASSERT(sq->fn.unary(25, &r) == CALC_OK && approx(r, 5));

    double a[] = {2, 4, 6};
    const Command *avg = ct_get(&t, 2);
    REQUIRE(avg != NULL);
    ASSERT(avg->fn.nary(a, 3, &r) == CALC_OK && approx(r, 4));
}

static void test_table_full(void)
{
    CommandTable t;
    int rc = 0;

    ct_init(&t);
    for (int i = 0; i < MAX_COMMANDS; i++)
        rc |= ct_add_binary(&t, "x", "x", calc_add);
    ASSERT(rc == 0);
    ASSERT(ct_count(&t) == MAX_COMMANDS);
    ASSERT(ct_add_binary(&t, "y", "y", calc_add) == -1);
    ASSERT(ct_count(&t) == MAX_COMMANDS);
}

static void test_edge_cases(void)
{
    double r;

    /* pow(0,0) по стандарту C равен 1, а не ошибка */
    ASSERT(calc_pow(0, 0, &r) == CALC_OK && approx(r, 1));
    ASSERT(calc_pow(0, -1, &r) == CALC_RANGE);

    /* остаток берёт знак делимого, а не делителя */
    ASSERT(calc_mod(-10, 3, &r) == CALC_OK && approx(r, -1));
    ASSERT(calc_mod(10, -3, &r) == CALC_OK && approx(r, 1));

    /* -0.0 == 0.0 по IEEE, значит тоже деление на ноль */
    ASSERT(calc_div(5, -0.0, &r) == CALC_DIV_ZERO);
    ASSERT(calc_div(0, 5, &r) == CALC_OK && approx(r, 0));

    ASSERT(calc_sub(-1e308, 1e308, &r) == CALC_RANGE);
}

static void test_nary_edge_cases(void)
{
    double r;
    double one[]   = {42};
    double big[]   = {1e308, 1e308};
    double mixed[] = {-5, 5};

    /* сумма пустого набора — ноль, а среднее не определено */
    ASSERT(calc_sum(one, 0, &r) == CALC_OK && approx(r, 0));
    ASSERT(calc_avg(one, 0, &r) == CALC_DIV_ZERO);

    ASSERT(calc_sum(one, 1, &r) == CALC_OK && approx(r, 42));
    ASSERT(calc_avg(one, 1, &r) == CALC_OK && approx(r, 42));

    /* переполнение внутри суммы доходит до вызывающего */
    ASSERT(calc_sum(big, 2, &r) == CALC_RANGE);
    ASSERT(calc_avg(big, 2, &r) == CALC_RANGE);

    ASSERT(calc_avg(mixed, 2, &r) == CALC_OK && approx(r, 0));
}

static void test_table_order_and_duplicates(void)
{
    CommandTable t;
    double r;

    ct_init(&t);
    ct_add_binary(&t, "Первая", "1", calc_add);
    ct_add_binary(&t, "Вторая", "2", calc_sub);
    /* уникальность имён не проверяется — это осознанно */
    ct_add_binary(&t, "Первая", "1", calc_mul);

    REQUIRE(ct_count(&t) == 3);

    /* порядок регистрации = порядок пунктов меню */
    ASSERT(strcmp(ct_get(&t, 0)->name, "Первая") == 0);
    ASSERT(strcmp(ct_get(&t, 1)->name, "Вторая") == 0);

    /* у тёзки свой указатель: зовётся именно calc_mul */
    ASSERT(ct_get(&t, 2)->fn.binary(6, 7, &r) == CALC_OK && approx(r, 42));
    /* а у первой — calc_add, они не перепутались */
    ASSERT(ct_get(&t, 0)->fn.binary(6, 7, &r) == CALC_OK && approx(r, 13));
}

int main(void)
{
    test_binary_ops();
    test_errors();
    test_sqrt();
    test_nary();
    test_table_registration();
    test_table_dispatch();
    test_table_full();
    test_edge_cases();
    test_nary_edge_cases();
    test_table_order_and_duplicates();

    printf("Тестов выполнено: %d, провалено: %d\n",
           tests_run, tests_failed);
    if (tests_failed == 0)
        printf("ВСЕ ТЕСТЫ ПРОЙДЕНЫ\n");
    return tests_failed != 0;
}
