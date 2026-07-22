#ifndef PQUEUE_H
#define PQUEUE_H

#include <stddef.h>

/*
 * Приоритет — целое 0..255; большее число означает более высокий
 * приоритет. "Первый в очереди" — элемент с наибольшим приоритетом,
 * а среди равных по приоритету — добавленный раньше (FIFO).
 *
 * Реализовано через односвязный список, упорядоченный по убыванию приоритета.
 * Голова списка всегда является "первой в очереди". Память под узлы
 * выделяется динамически, ограничения на размер очереди нет.
 */

#define PQ_MAX_PRIORITY 255
#define MSG_LEN 128

typedef struct {
    unsigned priority;      /* 0..255 */
    char     text[MSG_LEN]; /* полезная нагрузка сообщения */
} Message;

typedef struct PqNode {
    Message msg;
    struct PqNode *next;
} PqNode;

typedef struct {
    PqNode *head; /* наибольший приоритет */
    PqNode *tail; /* наименьший приоритет */
    size_t  count;
} PriorityQueue;

typedef enum {
    PQ_OK           =  0,
    PQ_NO_MEMORY    = -1, /* malloc не выделил память */
    PQ_BAD_PRIORITY = -2  /* приоритет вне диапазона 0..255 */
} PqResult;

void   pq_init(PriorityQueue *q);
void   pq_free(PriorityQueue *q);
size_t pq_count(const PriorityQueue *q);

/* Добавляет сообщение в очередь согласно приоритету (FIFO среди равных) */
PqResult pq_enqueue(PriorityQueue *q, unsigned priority, const char *text);

/* Первый в очереди: наибольший приоритет, среди равных — самый старый */
int pq_dequeue(PriorityQueue *q, Message *out);

/* Первый (самый старый) элемент с приоритетом == priority */
int pq_dequeue_priority(PriorityQueue *q, unsigned priority, Message *out);

/* Первый в очереди среди элементов с приоритетом НЕ НИЖЕ min_priority */
int pq_dequeue_atleast(PriorityQueue *q, unsigned min_priority,
                       Message *out);

/* Просмотр первого элемента без удаления (NULL — очередь пуста) */
const Message *pq_peek(const PriorityQueue *q);

/* Голова списка для обхода очереди (NULL — пусто). Только чтение. */
const PqNode *pq_head(const PriorityQueue *q);

#endif /* PQUEUE_H */
