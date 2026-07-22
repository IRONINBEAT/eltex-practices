#include <stdio.h>
#include <string.h>

#include "pqueue.h"

static int tests_run;
static int tests_failed;

#define ASSERT(cond) do {                                             \
    tests_run++;                                                      \
    if (!(cond)) {                                                    \
        tests_failed++;                                               \
        printf("FAIL: %s (%s:%d)\n", #cond, __FILE__, __LINE__);      \
    }                                                                 \
} while (0)

#define REQUIRE(cond) do {                                            \
    tests_run++;                                                      \
    if (!(cond)) {                                                    \
        tests_failed++;                                               \
        printf("FAIL: %s (%s:%d) — тест прерван\n",                   \
               #cond, __FILE__, __LINE__);                            \
        return;                                                       \
    }                                                                 \
} while (0)

/* Инвариант очереди: приоритеты не возрастают от головы к хвосту,
 * tail — последний узел, count совпадает с длиной. */
static int queue_ok(const PriorityQueue *q)
{
    size_t len = 0;
    const PqNode *last = NULL;

    for (const PqNode *n = pq_head(q); n != NULL; n = n->next) {
        if (last != NULL && last->msg.priority < n->msg.priority)
            return 0; /* приоритет вырос — нарушение порядка */
        last = n;
        len++;
    }
    return len == pq_count(q);
}

static void test_enqueue_order(void)
{
    PriorityQueue q;

    pq_init(&q);
    ASSERT(pq_enqueue(&q, 5, "a") == PQ_OK);
    ASSERT(pq_enqueue(&q, 10, "b") == PQ_OK);
    ASSERT(pq_enqueue(&q, 1, "c") == PQ_OK);
    ASSERT(pq_enqueue(&q, 10, "d") == PQ_OK);

    REQUIRE(pq_count(&q) == 4);
    ASSERT(queue_ok(&q));

    /* порядок: 10(b), 10(d), 5(a), 1(c) — по приоритету, FIFO среди равных */
    const PqNode *n = pq_head(&q);
    ASSERT(n->msg.priority == 10 && strcmp(n->msg.text, "b") == 0);
    n = n->next;
    ASSERT(n->msg.priority == 10 && strcmp(n->msg.text, "d") == 0);
    n = n->next;
    ASSERT(n->msg.priority == 5 && strcmp(n->msg.text, "a") == 0);
    n = n->next;
    ASSERT(n->msg.priority == 1 && strcmp(n->msg.text, "c") == 0);
    ASSERT(n->next == NULL);

    pq_free(&q);
}

static void test_bad_priority(void)
{
    PriorityQueue q;

    pq_init(&q);
    ASSERT(pq_enqueue(&q, 256, "x") == PQ_BAD_PRIORITY);
    ASSERT(pq_enqueue(&q, 1000, "x") == PQ_BAD_PRIORITY);
    ASSERT(pq_count(&q) == 0);
    /* границы диапазона допустимы */
    ASSERT(pq_enqueue(&q, 0, "lo") == PQ_OK);
    ASSERT(pq_enqueue(&q, 255, "hi") == PQ_OK);
    ASSERT(queue_ok(&q));
    pq_free(&q);
}

static void test_dequeue_first(void)
{
    PriorityQueue q;
    Message m;

    pq_init(&q);
    /* пустая очередь */
    ASSERT(pq_dequeue(&q, &m) == 0);

    pq_enqueue(&q, 3, "low");
    pq_enqueue(&q, 9, "high1");
    pq_enqueue(&q, 9, "high2");

    /* извлекаются по убыванию приоритета, FIFO среди равных */
    REQUIRE(pq_dequeue(&q, &m) == 1);
    ASSERT(m.priority == 9 && strcmp(m.text, "high1") == 0);
    REQUIRE(pq_dequeue(&q, &m) == 1);
    ASSERT(m.priority == 9 && strcmp(m.text, "high2") == 0);
    REQUIRE(pq_dequeue(&q, &m) == 1);
    ASSERT(m.priority == 3 && strcmp(m.text, "low") == 0);
    ASSERT(pq_dequeue(&q, &m) == 0);
    ASSERT(pq_count(&q) == 0);
    ASSERT(queue_ok(&q));

    pq_free(&q);
}

static void test_dequeue_priority(void)
{
    PriorityQueue q;
    Message m;

    pq_init(&q);
    pq_enqueue(&q, 5, "five-a");
    pq_enqueue(&q, 8, "eight");
    pq_enqueue(&q, 5, "five-b");
    pq_enqueue(&q, 2, "two");

    /* извлечение из середины по точному приоритету, FIFO */
    REQUIRE(pq_dequeue_priority(&q, 5, &m) == 1);
    ASSERT(strcmp(m.text, "five-a") == 0);
    REQUIRE(pq_dequeue_priority(&q, 5, &m) == 1);
    ASSERT(strcmp(m.text, "five-b") == 0);
    ASSERT(queue_ok(&q));

    /* приоритета 5 больше нет */
    ASSERT(pq_dequeue_priority(&q, 5, &m) == 0);
    /* приоритет, которого не было вовсе */
    ASSERT(pq_dequeue_priority(&q, 100, &m) == 0);
    ASSERT(pq_count(&q) == 2);

    /* извлечение головы по её приоритету */
    REQUIRE(pq_dequeue_priority(&q, 8, &m) == 1);
    ASSERT(strcmp(m.text, "eight") == 0);
    ASSERT(pq_head(&q)->msg.priority == 2); /* голова обновилась */
    ASSERT(queue_ok(&q));

    /* извлечение хвоста по его приоритету */
    REQUIRE(pq_dequeue_priority(&q, 2, &m) == 1);
    ASSERT(strcmp(m.text, "two") == 0);
    ASSERT(pq_count(&q) == 0);
    ASSERT(queue_ok(&q));

    pq_free(&q);
}

static void test_dequeue_atleast(void)
{
    PriorityQueue q;
    Message m;

    pq_init(&q);
    pq_enqueue(&q, 4, "four");
    pq_enqueue(&q, 7, "seven");
    pq_enqueue(&q, 1, "one");

    /* не ниже 5 -> берётся 7 (максимум) */
    REQUIRE(pq_dequeue_atleast(&q, 5, &m) == 1);
    ASSERT(m.priority == 7 && strcmp(m.text, "seven") == 0);

    /* не ниже 5 -> теперь максимум 4, не подходит */
    ASSERT(pq_dequeue_atleast(&q, 5, &m) == 0);
    ASSERT(pq_count(&q) == 2);

    /* не ниже 4 -> берётся 4 */
    REQUIRE(pq_dequeue_atleast(&q, 4, &m) == 1);
    ASSERT(m.priority == 4);

    /* не ниже 0 -> подходит любой (осталась 1) */
    REQUIRE(pq_dequeue_atleast(&q, 0, &m) == 1);
    ASSERT(m.priority == 1);

    /* пустая очередь */
    ASSERT(pq_dequeue_atleast(&q, 0, &m) == 0);
    ASSERT(queue_ok(&q));

    pq_free(&q);
}

static void test_out_null(void)
{
    PriorityQueue q;

    pq_init(&q);
    pq_enqueue(&q, 5, "x");
    pq_enqueue(&q, 5, "y");

    /* out == NULL: элемент удаляется без копирования */
    ASSERT(pq_dequeue(&q, NULL) == 1);
    ASSERT(pq_count(&q) == 1);
    ASSERT(pq_dequeue_priority(&q, 5, NULL) == 1);
    ASSERT(pq_count(&q) == 0);

    pq_free(&q);
}

static void test_peek(void)
{
    PriorityQueue q;

    pq_init(&q);
    ASSERT(pq_peek(&q) == NULL);
    pq_enqueue(&q, 2, "lo");
    pq_enqueue(&q, 9, "hi");

    const Message *m = pq_peek(&q);
    REQUIRE(m != NULL);
    ASSERT(m->priority == 9);
    /* peek не удаляет */
    ASSERT(pq_count(&q) == 2);

    pq_free(&q);
}

static void test_free_and_reuse(void)
{
    PriorityQueue q;

    pq_init(&q);
    for (unsigned i = 0; i < 50; i++)
        pq_enqueue(&q, i % 256, "x");
    ASSERT(pq_count(&q) == 50);
    ASSERT(queue_ok(&q));

    pq_free(&q);
    ASSERT(pq_count(&q) == 0);
    ASSERT(pq_head(&q) == NULL);

    /* очередь пригодна к работе после освобождения */
    ASSERT(pq_enqueue(&q, 5, "again") == PQ_OK);
    ASSERT(pq_count(&q) == 1);
    pq_free(&q);
    pq_free(&q); /* повторный free безопасен */
}

static void test_text_truncation(void)
{
    PriorityQueue q;
    Message m;
    char big[MSG_LEN * 2];

    memset(big, 'A', sizeof(big) - 1);
    big[sizeof(big) - 1] = '\0';

    pq_init(&q);
    ASSERT(pq_enqueue(&q, 5, big) == PQ_OK);
    REQUIRE(pq_dequeue(&q, &m) == 1);
    /* длинный текст обрезан до размера буфера, строка корректна */
    ASSERT(strlen(m.text) == MSG_LEN - 1);

    pq_free(&q);
}

int main(void)
{
    test_enqueue_order();
    test_bad_priority();
    test_dequeue_first();
    test_dequeue_priority();
    test_dequeue_atleast();
    test_out_null();
    test_peek();
    test_free_and_reuse();
    test_text_truncation();

    printf("Тестов выполнено: %d, провалено: %d\n",
           tests_run, tests_failed);
    if (tests_failed == 0)
        printf("ВСЕ ТЕСТЫ ПРОЙДЕНЫ\n");
    return tests_failed != 0;
}
