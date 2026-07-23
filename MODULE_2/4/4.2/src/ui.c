#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* Запрашивает приоритет 0..255, повторяя при некорректном вводе.
 * Возвращает 0 при EOF. */
static int read_priority(const char *prompt, unsigned *out)
{
    char buf[32];
    char *end;
    unsigned long v;

    for (;;) {
        printf("%s", prompt);
        if (!read_line(buf, sizeof(buf)))
            return 0;

        v = strtoul(buf, &end, 10);
        while (*end == ' ' || *end == '\t')
            end++;
        if (buf[0] == '\0' || *end != '\0' || v > PQ_MAX_PRIORITY) {
            printf("Нужно целое число от 0 до %d.\n", PQ_MAX_PRIORITY);
            continue;
        }
        *out = (unsigned)v;
        return 1;
    }
}

static void print_queue(const PriorityQueue *q)
{
    if (pq_count(q) == 0) {
        printf("  (очередь пуста)\n");
        return;
    }
    for (const PqNode *n = pq_head(q); n != NULL; n = n->next)
        printf("  [prio %3u] %s\n", n->msg.priority, n->msg.text);
    printf("  Всего сообщений: %zu\n", pq_count(q));
}

/* Печать результата извлечения */
static void show_dequeue(const char *what, int ok, const Message *m)
{
    if (ok)
        printf("%s -> [prio %3u] %s\n", what, m->priority, m->text);
    else
        printf("%s -> подходящих сообщений нет\n", what);
}

/* ---------- пункты меню ручного режима ---------- */

static void action_add(PriorityQueue *q)
{
    unsigned prio;
    char text[MSG_LEN];

    if (!read_priority("Приоритет (0-255, больше = важнее): ", &prio))
        return;

    printf("Текст сообщения: ");
    if (!read_line(text, sizeof(text)))
        return;
    if (text[0] == '\0') {
        printf("Пустое сообщение не добавлено.\n");
        return;
    }

    switch (pq_enqueue(q, prio, text)) {
    case PQ_OK:
        printf("Добавлено: [prio %u] %s\n", prio, text);
        break;
    case PQ_NO_MEMORY:
        printf("Ошибка: не удалось выделить память.\n");
        break;
    default:
        printf("Ошибка: сообщение не добавлено.\n");
    }
}

static void action_dequeue(PriorityQueue *q)
{
    Message m;

    show_dequeue("Первый в очереди", pq_dequeue(q, &m), &m);
}

static void action_dequeue_priority(PriorityQueue *q)
{
    Message m;
    unsigned prio;

    if (!read_priority("Искомый приоритет (0-255): ", &prio))
        return;
    show_dequeue("Точное совпадение", pq_dequeue_priority(q, prio, &m), &m);
}

static void action_dequeue_atleast(PriorityQueue *q)
{
    Message m;
    unsigned prio;

    if (!read_priority("Минимальный приоритет (0-255): ", &prio))
        return;
    show_dequeue("Не ниже порога", pq_dequeue_atleast(q, prio, &m), &m);
}

/* Догенерировать случайные сообщения прямо из меню */
static void action_fill(PriorityQueue *q)
{
    char buf[16];
    long k;

    printf("Сколько случайных сообщений добавить? ");
    if (!read_line(buf, sizeof(buf)))
        return;
    k = strtol(buf, NULL, 10);
    if (k < 1 || k > 1000) {
        printf("Допустимо от 1 до 1000.\n");
        return;
    }

    for (long i = 0; i < k; i++) {
        unsigned prio = (unsigned)(rand() % (PQ_MAX_PRIORITY + 1));
        char text[MSG_LEN];

        snprintf(text, sizeof(text), "случайное #%ld", i + 1);
        pq_enqueue(q, prio, text);
    }
    printf("Добавлено сообщений: %ld\n", k);
}

void ui_run(PriorityQueue *q)
{
    char choice[8];

    for (;;) {
        printf("\n===== ОЧЕРЕДЬ С ПРИОРИТЕТОМ =====\n"
               "В очереди сообщений: %zu\n"
               "1. Добавить сообщение\n"
               "2. Показать очередь\n"
               "3. Извлечь первое в очереди\n"
               "4. Извлечь по точному приоритету\n"
               "5. Извлечь с приоритетом не ниже заданного\n"
               "6. Добавить случайные сообщения\n"
               "0. Выход\n"
               "Выберите действие: ", pq_count(q));

        if (!read_line(choice, sizeof(choice)))
            return;

        switch (choice[0]) {
        case '1': action_add(q);                break;
        case '2': print_queue(q);               break;
        case '3': action_dequeue(q);            break;
        case '4': action_dequeue_priority(q);   break;
        case '5': action_dequeue_atleast(q);    break;
        case '6': action_fill(q);               break;
        case '0': return;
        default:  printf("Неизвестный пункт меню.\n");
        }
    }
}

/* ---------- режим имитации ---------- */

void ui_simulate(PriorityQueue *q, unsigned long n, unsigned long seed)
{
    Message m;

    srand((unsigned)seed);

    printf("Имитация генерации %lu сообщений (зерно %lu):\n", n, seed);
    for (unsigned long i = 0; i < n; i++) {
        unsigned prio = (unsigned)(rand() % (PQ_MAX_PRIORITY + 1));
        char text[MSG_LEN];

        snprintf(text, sizeof(text), "сообщение #%lu", i + 1);
        printf("  + [prio %3u] %s\n", prio, text);
        pq_enqueue(q, prio, text);
    }

    printf("\nОчередь после добавления (по убыванию приоритета):\n");
    print_queue(q);

    printf("\n--- Выборка с различными условиями ---\n");
    show_dequeue("Первый в очереди        ", pq_dequeue(q, &m), &m);
    show_dequeue("Первый в очереди        ", pq_dequeue(q, &m), &m);

    const Message *top = pq_peek(q);
    if (top != NULL) {
        unsigned p = top->priority;
        printf("Извлечение по приоритету = %u:\n", p);
        show_dequeue("  точное совпадение     ",
                     pq_dequeue_priority(q, p, &m), &m);
    }

    show_dequeue("Приоритет ровно 300 (нет)",
                 pq_dequeue_priority(q, 300, &m), &m);
    show_dequeue("Приоритет не ниже 128    ",
                 pq_dequeue_atleast(q, 128, &m), &m);
    show_dequeue("Приоритет не ниже 200    ",
                 pq_dequeue_atleast(q, 200, &m), &m);

    printf("\nОсталось в очереди: %zu\n", pq_count(q));
    printf("Извлекаем всё подряд (первый в очереди):\n");
    while (pq_dequeue(q, &m))
        printf("  [prio %3u] %s\n", m.priority, m.text);
}
