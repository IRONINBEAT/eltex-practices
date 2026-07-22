#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "pqueue.h"

/* Печать всей очереди от головы (наибольший приоритет) к хвосту */
static void print_queue(const PriorityQueue *q)
{
    if (pq_count(q) == 0) {
        printf("  (очередь пуста)\n");
        return;
    }
    for (const PqNode *n = pq_head(q); n != NULL; n = n->next)
        printf("  [prio %3u] %s\n", n->msg.priority, n->msg.text);
}

/* Одна попытка извлечения с печатью результата */
static void show_dequeue(const char *what, int ok, const Message *m)
{
    if (ok)
        printf("%s -> [prio %3u] %s\n", what, m->priority, m->text);
    else
        printf("%s -> подходящих сообщений нет\n", what);
}

int main(int argc, char **argv)
{
    PriorityQueue q;
    unsigned long n = 12;
    unsigned long seed;
    char *end;

    if (argc > 1) {
        n = strtoul(argv[1], &end, 10);
        if (argv[1][0] == '\0' || *end != '\0' || n == 0) {
            fprintf(stderr, "Количество сообщений должно быть > 0\n");
            return 1;
        }
    }
    if (argc > 2) {
        seed = strtoul(argv[2], &end, 10);
        if (argv[2][0] == '\0' || *end != '\0') {
            fprintf(stderr, "Некорректное зерно\n");
            return 1;
        }
    } else {
        seed = (unsigned long)time(NULL);
    }
    srand((unsigned)seed);

    pq_init(&q);

    printf("Имитация генерации %lu сообщений (зерно %lu):\n", n, seed);
    for (unsigned long i = 0; i < n; i++) {
        unsigned prio = (unsigned)(rand() % (PQ_MAX_PRIORITY + 1));
        char text[MSG_LEN];

        snprintf(text, sizeof(text), "сообщение #%lu", i + 1);
        printf("  + [prio %3u] %s\n", prio, text);
        pq_enqueue(&q, prio, text);
    }

    printf("\nОчередь после добавления (по убыванию приоритета):\n");
    print_queue(&q);

    printf("\n--- Выборка с различными условиями ---\n");

    Message m;
    show_dequeue("Первый в очереди        ", pq_dequeue(&q, &m), &m);
    show_dequeue("Первый в очереди        ", pq_dequeue(&q, &m), &m);

    /* извлечь сообщение с приоритетом ровно как у текущей головы */
    const Message *top = pq_peek(&q);
    if (top != NULL) {
        unsigned p = top->priority;
        printf("Извлечение по приоритету = %u:\n", p);
        show_dequeue("  точное совпадение     ",
                     pq_dequeue_priority(&q, p, &m), &m);
    }

    /* заведомо отсутствующий приоритет */
    show_dequeue("Приоритет ровно 300 (отсутствует)", pq_dequeue_priority(&q, 300, &m), &m);

    show_dequeue("Приоритет не ниже 128    ",
                 pq_dequeue_atleast(&q, 128, &m), &m);
    show_dequeue("Приоритет не ниже 200    ",
                 pq_dequeue_atleast(&q, 200, &m), &m);

    printf("\nОсталось в очереди: %zu\n", pq_count(&q));
    printf("Извлекаем всё подряд (первый в очереди):\n");
    while (pq_dequeue(&q, &m))
        printf("  [prio %3u] %s\n", m.priority, m.text);

    pq_free(&q);
    return 0;
}
