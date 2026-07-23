#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui.h"

/* Верхняя граница на число аргументов операции с переменной арностью */
#define MAX_ARGS 32

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

/* Запрашивает аргументы согласно арности и вызывает функцию плагина */
static void run_plugin(const Plugin *p)
{
    double args[MAX_ARGS];
    double r;
    CalcStatus st;
    size_t n;

    if (p->arity == CALC_NARY) {
        char buf[16];
        long k;

        printf("Сколько чисел (1-%d)? ", MAX_ARGS);
        if (!read_line(buf, sizeof(buf)))
            return;
        k = strtol(buf, NULL, 10);
        if (k < 1 || k > MAX_ARGS) {
            printf("Недопустимое количество.\n");
            return;
        }
        n = (size_t)k;
    } else {
        n = (size_t)p->arity;
    }

    for (size_t i = 0; i < n; i++) {
        char prompt[32];

        if (n == 1)
            snprintf(prompt, sizeof(prompt), "Аргумент: ");
        else
            snprintf(prompt, sizeof(prompt), "Число %zu: ", i + 1);
        if (!read_double(prompt, &args[i]))
            return;
    }

    /* вызов через указатель, полученный из библиотеки */
    st = p->apply(args, n, &r);
    if (st != CALC_OK) {
        printf("Ошибка: %s\n", calc_strerror(st));
        return;
    }

    if (n == 2)
        printf("Результат: %g %s %g = %g\n", args[0], p->symbol, args[1], r);
    else if (n == 1)
        printf("Результат: %s(%g) = %g\n", p->symbol, args[0], r);
    else
        printf("Результат: %s = %g\n", p->symbol, r);
}

void ui_run(const PluginList *pl)
{
    char choice[16];
    long n;

    for (;;) {
        printf("\n========= КАЛЬКУЛЯТОР =========\n");
        for (size_t i = 0; i < pl->count; i++)
            printf("%2zu. %s (%s)\n", i + 1,
                   pl->items[i].name, pl->items[i].symbol);
        printf(" 0. Выход\n");
        printf("Выберите действие: ");

        if (!read_line(choice, sizeof(choice)))
            return;

        n = strtol(choice, NULL, 10);
        if (n == 0)
            return;
        if (n < 1 || (size_t)n > pl->count) {
            printf("Неизвестное действие.\n");
            continue;
        }
        run_plugin(&pl->items[n - 1]);
    }
}
