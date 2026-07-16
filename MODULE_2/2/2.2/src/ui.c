#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "calc.h"
#include "ui.h"

/* Возвращает 0 при EOF */
static int read_line(char *buf, size_t size)
{
    if (fgets(buf, (int)size, stdin) == NULL) {
        buf[0] = '\0';
        return 0;
    }
    buf[strcspn(buf, "\r\n")] = '\0';
    return 1;
}

/* Повторяет запрос, пока не введут число. Возвращает 0 при EOF. */
static int read_double(const char *prompt, double *out)
{
    char buf[128];
    char *end;
    double v;

    for (;;) {
        printf("%s", prompt);
        if (!read_line(buf, sizeof(buf)))
            return 0;

        v = strtod(buf, &end);
        while (*end == ' ' || *end == '\t')
            end++;
        if (end == buf || *end != '\0') {
            printf("Некорректное число, попробуйте ещё раз.\n");
            continue;
        }
        *out = v;
        return 1;
    }
}

static void do_binary(CalcStatus (*fn)(double, double, double *),
                      const char *sym)
{
    double a, b, r;
    CalcStatus st;

    if (!read_double("Первый аргумент:  ", &a))
        return;
    if (!read_double("Второй аргумент:  ", &b))
        return;

    st = fn(a, b, &r);
    if (st == CALC_OK)
        printf("Результат: %g %s %g = %g\n", a, sym, b, r);
    else
        printf("Ошибка: %s\n", calc_strerror(st));
}

static void do_sqrt(void)
{
    double a, r;
    CalcStatus st;

    if (!read_double("Аргумент: ", &a))
        return;

    st = calc_sqrt(a, &r);
    if (st == CALC_OK)
        printf("Результат: sqrt(%g) = %g\n", a, r);
    else
        printf("Ошибка: %s\n", calc_strerror(st));
}

void ui_run(void)
{
    char choice[16];

    for (;;) {
        printf("\n========= КАЛЬКУЛЯТОР =========\n"
               "1. Сложение          (a + b)\n"
               "2. Вычитание         (a - b)\n"
               "3. Умножение         (a * b)\n"
               "4. Деление           (a / b)\n"
               "5. Возведение в степень (a ^ b)\n"
               "6. Остаток от деления   (a %% b)\n"
               "7. Квадратный корень (sqrt a)\n"
               "0. Выход\n"
               "Выберите действие: ");

        if (!read_line(choice, sizeof(choice)))
            return;

        switch (choice[0]) {
        case '1': do_binary(calc_add, "+"); break;
        case '2': do_binary(calc_sub, "-"); break;
        case '3': do_binary(calc_mul, "*"); break;
        case '4': do_binary(calc_div, "/"); break;
        case '5': do_binary(calc_pow, "^"); break;
        case '6': do_binary(calc_mod, "%"); break;
        case '7': do_sqrt();                break;
        case '0': return;
        default:  printf("Неизвестное действие.\n");
        }
    }
}
