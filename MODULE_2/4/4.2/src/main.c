#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "pqueue.h"
#include "ui.h"

static void usage(const char *prog)
{
    fprintf(stderr,
            "Использование:\n"
            "  %s                 — ручной ввод сообщений (меню)\n"
            "  %s <N> [зерно]     — имитация: N случайных сообщений\n"
            "  %s -s <N> [зерно]  — то же явным флагом\n",
            prog, prog, prog);
}

int main(int argc, char **argv)
{
    PriorityQueue q;
    unsigned long n, seed;
    char *end;
    int i = 1;

    setvbuf(stdout, NULL, _IOLBF, 0);
    pq_init(&q);

    /* Без аргументов — интерактивный режим */
    if (argc == 1) {
        srand((unsigned)time(NULL)); /* для пункта "случайные сообщения" */
        ui_run(&q);
        pq_free(&q);
        return 0;
    }

    if (strcmp(argv[i], "-s") == 0)
        i++; /* флаг необязателен: количество можно указать сразу */

    if (i >= argc) {
        usage(argv[0]);
        pq_free(&q);
        return 1;
    }

    n = strtoul(argv[i], &end, 10);
    if (argv[i][0] == '\0' || *end != '\0' || n == 0) {
        fprintf(stderr, "Количество сообщений должно быть > 0\n");
        usage(argv[0]);
        pq_free(&q);
        return 1;
    }
    i++;

    if (i < argc) {
        seed = strtoul(argv[i], &end, 10);
        if (argv[i][0] == '\0' || *end != '\0') {
            fprintf(stderr, "Некорректное зерно\n");
            pq_free(&q);
            return 1;
        }
    } else {
        seed = (unsigned long)time(NULL);
    }

    ui_simulate(&q, n, seed);
    pq_free(&q);
    return 0;
}
