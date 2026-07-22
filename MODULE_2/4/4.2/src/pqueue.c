#include <stdlib.h>
#include <string.h>

#include "pqueue.h"

void pq_init(PriorityQueue *q)
{
    q->head = NULL;
    q->tail = NULL;
    q->count = 0;
}

void pq_free(PriorityQueue *q)
{
    PqNode *n = q->head;

    while (n != NULL) {
        PqNode *next = n->next;
        free(n);
        n = next;
    }
    pq_init(q);
}

size_t pq_count(const PriorityQueue *q)
{
    return q->count;
}

PqResult pq_enqueue(PriorityQueue *q, unsigned priority, const char *text)
{
    PqNode *node;
    PqNode *prev = NULL;
    PqNode *cur = q->head;

    if (priority > PQ_MAX_PRIORITY)
        return PQ_BAD_PRIORITY;

    node = malloc(sizeof(PqNode));
    if (node == NULL)
        return PQ_NO_MEMORY;

    node->msg.priority = priority;
    strncpy(node->msg.text, text, MSG_LEN - 1);
    node->msg.text[MSG_LEN - 1] = '\0';

    while (cur != NULL && cur->msg.priority >= priority) {
        prev = cur;
        cur = cur->next;
    }

    node->next = cur;
    if (prev != NULL)
        prev->next = node;
    else
        q->head = node;
    if (cur == NULL)
        q->tail = node;

    q->count++;
    return PQ_OK;
}

static void extract(PriorityQueue *q, PqNode *prev, PqNode *node,
                    Message *out)
{
    if (out != NULL)
        *out = node->msg;

    if (prev != NULL)
        prev->next = node->next;
    else
        q->head = node->next;

    if (node->next == NULL)
        q->tail = prev;

    free(node);
    q->count--;
}

int pq_dequeue(PriorityQueue *q, Message *out)
{
    if (q->head == NULL)
        return 0;
    extract(q, NULL, q->head, out);
    return 1;
}

int pq_dequeue_priority(PriorityQueue *q, unsigned priority, Message *out)
{
    PqNode *prev = NULL;
    PqNode *cur = q->head;

    while (cur != NULL) {
        if (cur->msg.priority == priority) {
            extract(q, prev, cur, out);
            return 1;
        }
        if (cur->msg.priority < priority)
            break;
        prev = cur;
        cur = cur->next;
    }
    return 0;
}

int pq_dequeue_atleast(PriorityQueue *q, unsigned min_priority,
                       Message *out)
{
    if (q->head == NULL || q->head->msg.priority < min_priority)
        return 0;
    extract(q, NULL, q->head, out);
    return 1;
}

const Message *pq_peek(const PriorityQueue *q)
{
    return q->head != NULL ? &q->head->msg : NULL;
}

const PqNode *pq_head(const PriorityQueue *q)
{
    return q->head;
}
