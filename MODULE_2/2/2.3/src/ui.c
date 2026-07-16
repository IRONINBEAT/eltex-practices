#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui.h"

/* Верхняя граница на число аргументов N-арной операции */
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

static void show_result(CalcStatus st, double r)
{
    if (st == CALC_OK)
        printf("Результат: %g\n", r);
    else
        printf("Ошибка: %s\n", calc_strerror(st));
}

static void run_command(const Command *cmd)
{
    double a, b, r;
    CalcStatus st;

    switch (cmd->arity) {
    case OP_UNARY:
        if (!read_double("Аргумент: ", &a))
            return;
        st = cmd->fn.unary(a, &r);
        if (st == CALC_OK)
            printf("Результат: %s(%g) = %g\n", cmd->symbol, a, r);
        else
            printf("Ошибка: %s\n", calc_strerror(st));
        break;

    case OP_BINARY:
        if (!read_double("Первый аргумент:  ", &a))
            return;
        if (!read_double("Второй аргумент:  ", &b))
            return;
        st = cmd->fn.binary(a, b, &r);
        if (st == CALC_OK)
            printf("Результат: %g %s %g = %g\n", a, cmd->symbol, b, r);
        else
            printf("Ошибка: %s\n", calc_strerror(st));
        break;

    case OP_NARY: {
        double args[MAX_ARGS];
        char buf[16];
        long n;

        printf("Сколько чисел (1-%d)? ", MAX_ARGS);
        if (!read_line(buf, sizeof(buf)))
            return;
        n = strtol(buf, NULL, 10);
        if (n < 1 || n > MAX_ARGS) {
            printf("Недопустимое количество.\n");
            return;
        }
        for (long i = 0; i < n; i++) {
            char prompt[32];
            snprintf(prompt, sizeof(prompt), "Число %ld: ", i + 1);
            if (!read_double(prompt, &args[i]))
                return;
        }
        st = cmd->fn.nary(args, (size_t)n, &r);
        show_result(st, r);
        break;
    }
    }
}

void ui_run(const CommandTable *table)
{
    char choice[16];
    size_t count = ct_count(table);
    long n;

    for (;;) {
        printf("\n========= КАЛЬКУЛЯТОР =========\n");
        for (size_t i = 0; i < count; i++) {
            const Command *c = ct_get(table, i);
            printf("%2zu. %s (%s)\n", i + 1, c->name, c->symbol);
        }
        printf(" 0. Выход\n");
        printf("Выберите действие: ");

        if (!read_line(choice, sizeof(choice)))
            return;

        n = strtol(choice, NULL, 10);
        if (n == 0)
            return;
        if (n < 1 || (size_t)n > count) {
            printf("Неизвестное действие.\n");
            continue;
        }
        run_command(ct_get(table, (size_t)n - 1));
    }
}
